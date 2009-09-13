CC = gcc
GLIBFLAGS = `pkg-config --cflags glib-2.0 gthread-2.0`
GLIBLIBS = `pkg-config --libs glib-2.0 gthread-2.0`
CFLAGS = -Isrc -DG_LOG_DOMAIN=\"falcon\" -Wall -W -fPIC -g -pg
CLIBS = -Lsrc -fPIC -g -pg
SOURCES = src/cache.o \
          src/common.o \
          src/events.o \
          src/falcon.o \
          src/handler.o \
          src/object.o \
          src/walker.o \
          tests/main.o

all: falcon

falcon: $(SOURCES)
	$(CC) $(GLIBLIBS) $(CLIBS) $(SOURCES) -o $@

$(SOURCES): %.o: %.c
	$(CC) $(GLIBFLAGS) $(CFLAGS) -c $< -o $@

.PHONY: clean
clean:
	rm -f tests/*.o src/*.o falcon *.out