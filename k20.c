/*
    Copyright (C) 2008  Hans Fugal

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include <ncurses.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include <string.h>
#include <sys/select.h>
#include <sys/time.h>
#include <fcntl.h>

#include "ringbuffer.h"
#include "options.h"
#include "k20.h"

int scale(float dbfs);

int main(int argc, char *const *argv)
{
    struct options opts = {"k20", 0, 30, 0, 0};
    parse_options(&argc, &argv, &opts);

    struct context ctx = {};
    ctx.dump = opts.d;

    // JACK initialization
    ctx.jack = jack_client_open(opts.n, 0, 0, 0);
    if (!ctx.jack)
    {
        fprintf(stderr, "Failed to create JACK client.\n");
        exit(1);
    }

    ctx.m.port = jack_port_register(ctx.jack, "in", JACK_DEFAULT_AUDIO_TYPE,
                                    JackPortIsInput|JackPortIsTerminal, 0);
    if (!ctx.m.port)
    {
        fprintf(stderr, "Failed to create input port.\n");
        exit(1);
    }

    if (jack_set_process_callback(ctx.jack, jack_process, &ctx) != 0)
    {
        fprintf(stderr, "Failed to set process callback.\n");
        exit(1);
    }

    jack_nframes_t sr = jack_get_sample_rate(ctx.jack);
    init_meter(&ctx.m, sr);
    ctx.sem = sem_open(jack_get_client_name(ctx.jack), O_CREAT, 0600, 0);
    if (ctx.sem == SEM_FAILED)
    {
        fprintf(stderr, "Failed to open semaphore.\n");
        exit(1);
    }

    jack_activate(ctx.jack);

    int i;
    for (i=0; i<argc; i++)
    {
        char *n = jack_get_client_name(ctx.jack);
        char *p = alloca(strlen(n) + 3 + 1);
        strncpy(p, n, strlen(n));
        strncpy(p+strlen(n), ":in", 3);
        int ret = jack_connect(ctx.jack, argv[i], p);
        if (ret != 0 && ret != EEXIST)
        {
            fprintf(stderr, "Couldn't connect to port %s.\n", argv[i]);
            exit(EXIT_FAILURE);
        }
    }

    if (ctx.dump)
    {
        // dump
        puts("# sec avg peak maxpeak overs (dB)");
        struct meter *m = &ctx.m;
        while (sem_wait(ctx.sem) != -1)
        {
            printf("%g %g %g %g %d\n", (float)ctx.frames / sr, m->rms+20, m->peak+20, m->maxpeak+20, m->overs);
        }
    } else {
        // meter

        if ( initscr() == NULL ) {
            fprintf(stderr, "Error initialising ncurses.\n");
            exit(EXIT_FAILURE);
        }
        noecho();

        init_pair(1, COLOR_GREEN, COLOR_GREEN); // Green.
        init_pair(2, COLOR_YELLOW, COLOR_YELLOW); // Yellow
        init_pair(3, COLOR_RED, COLOR_RED); // Red
        init_pair(4, COLOR_WHITE, COLOR_RED); // Warning
        init_pair(5, COLOR_WHITE, -1);

        if (has_colors() && !opts.C)
            start_color();
        use_default_colors();

        attron(A_BOLD);
        mvaddstr(0, 0, "-70   60   50   40   30        20   15   10  6  3  0  3  6   10   15   20+");
        mvaddstr(1, 0, " |    |    |    |    |         |    |    |   |  |  |  |  |   |    |    |");
        attroff(A_BOLD);
        standend();
        move(3, 0);
        while (1)
        {
            fd_set readfds;
            FD_ZERO(&readfds);
            FD_SET(fileno(stdin), &readfds);
            struct timeval tv = {0, 1000000 * 1.0/opts.r};
            if (select(fileno(stdin)+1, &readfds, 0, 0, &tv))
            {
                char buf[1024];
                fgets(buf, 1024, stdin);
                ctx.m.overs = 0;
                ctx.m.maxpeak = ctx.m.peak = dbfs(0);
                move(3, 0); // up a line (counter the newline)
            }

            char meter[72];

            memset(meter, ' ', 71);
            meter[71] = 0;
            int p = scale(ctx.m.rms);
            if (p >= 0)
                memset(meter, '#', p);
            p = scale(ctx.m.peak)-1;
            if (p >= 0)
                meter[p] = '#';
            p = scale(ctx.m.maxpeak)-1;
            if (p >= 0)
                meter[p] = '#';

            //mvprintw(2, 0, " \e[K\e[32m%.50s\e[33m%.5s\e[31m%.16s\e[0m", meter, meter+50, meter+55);
            move(2, 0);
            //clrtoeol();
            attron(COLOR_PAIR(5)); // Green
            printw("%.50s", meter);
            //attroff(COLOR_PAIR(1));

            //attrset(COLOR_PAIR(2)); // Yellow
            printw("%.5s", meter+50);
            //attroff(COLOR_PAIR(2));

            //attrset(COLOR_PAIR(3)); // Red
            printw("%.16s", meter+55);
            //attroff(COLOR_PAIR(3));
            standend();

            if (ctx.m.overs > 0) {
                attron(COLOR_PAIR(4));
                mvprintw(2, 72, "   %d ", ctx.m.overs);
                attroff(COLOR_PAIR(4));
                ctx.m.overs = 0;
            }
            if (opts.v) // verbose
                mvprintw(2, 72, " %.1f %.1f %.1f", ctx.m.rms, ctx.m.peak, ctx.m.maxpeak);
            //endwin();
            refresh();
        }
        endwin();
        refresh();
    }

    jack_deactivate(ctx.jack);
    jack_client_close(ctx.jack);

    return EXIT_SUCCESS;
}

int scale(float dbfs)
{
    int x;
    if (dbfs < -50)
        x = round((90+dbfs)/2.0);
    else
        x = round(71.0+dbfs);

    return max(0, min(x, 71));
}
