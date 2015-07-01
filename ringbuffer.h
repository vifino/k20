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

// Why isn't there a float ringbuffer in JACK?!

#ifndef RINGBUFFER_H
#define RINGBUFFER_H

#include <jack/ringbuffer.h>

jack_ringbuffer_t *ringbuffer_create(size_t sz);
size_t ringbuffer_peek(jack_ringbuffer_t *rb, float *dest, size_t cnt);
void ringbuffer_read_advance(jack_ringbuffer_t *rb, size_t cnt);
size_t ringbuffer_write(jack_ringbuffer_t *rb, float *src, size_t cnt);

#endif
