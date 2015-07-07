LDFLAGS+=-lm -lpthread -lncurses -ljack
PREFIX=/usr/local
objects=ringbuffer.o jack.o options.o
all: options.h k20

k20: $(objects)
	$(CC) $(CFLAGS$) $(CPPFLAGS)  -o k20 k20.c $(objects) $(LDFLAGS)

options.o: options.h options.c
options.h: options.opts
	./opg $<
clean:
	rm -f k20 *.o options.[hc]

install: all
	install -d $(PREFIX)/bin
	install -t $(PREFIX)/bin k20

uninstall:
	rm -f $(PREFIX)/bin/k20

.PHONY: clean install
