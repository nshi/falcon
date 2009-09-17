CC = gcc
GLIBFLAGS = `pkg-config --cflags glib-2.0 gthread-2.0 gio-2.0 gobject-2.0`
GLIBLIBS = `pkg-config --libs glib-2.0 gthread-2.0 gio-2.0 gobject-2.0`
CFLAGS = -Isrc -DG_LOG_DOMAIN=\"falcon\" -Wall -Wextra -Wformat -Werror \
         -fPIC -g -pg
CLIBS = -Lsrc -fPIC -g -pg
SOURCES = src/cache.o \
          src/common.o \
          src/events.o \
          src/falcon.o \
          src/handler.o \
          src/object.o \
          src/walker.o \
          src/watcher.o
FALCON = tests/main.o
LOADER = tests/loader.o

all: falcon

loader: $(LOADER) $(SOURCES)
	$(CC) $(GLIBLIBS) $(CLIBS) $(LOADER) $(SOURCES) -o $@

falcon: $(FALCON) $(SOURCES)
	$(CC) $(GLIBLIBS) $(CLIBS) $(FALCON) $(SOURCES) -o $@

$(LOADER): %.o: %.c
	$(CC) $(GLIBFLAGS) $(CFLAGS) -c $< -o $@

$(FALCON): %.o: %.c
	$(CC) $(GLIBFLAGS) $(CFLAGS) -c $< -o $@

$(SOURCES): %.o: %.c
	$(CC) $(GLIBFLAGS) $(CFLAGS) -c $< -o $@

.PHONY: clean
clean:
	rm -f tests/*.o src/*.o falcon loader *.out