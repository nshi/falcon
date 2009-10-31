CC = gcc
GLIBFLAGS = `pkg-config --cflags glib-2.0 gthread-2.0 gio-2.0 gobject-2.0`
GLIBLIBS = `pkg-config --libs glib-2.0 gthread-2.0 gio-2.0 gobject-2.0`
XMMS2FLAGS = `pkg-config --cflags xmms2-client xmms2-client-glib`
XMMS2LIBS = `pkg-config --libs xmms2-client xmms2-client-glib`
CFLAGS = -Isrc -I./ -DG_LOG_DOMAIN=\"falcon\" -Wall -Wextra -Wformat \
         -Winline -Werror -fPIC -g -pg
CLIBS = -Lsrc -fPIC -g -pg
SOURCES = src/cache.o \
          src/common.o \
          src/events.o \
          src/falcon.o \
          src/handler.o \
          src/object.o \
          src/walker.o \
          src/watcher.o \
          src/filter.o \
          src/trie.o
FALCON = tests/main.o
LOADER = tests/loader.o
CACHE_READER = tests/cache_reader.o
TRIE = src/trie.o tests/trie.o
XMMS2_MONITOR = tests/xmms2_monitor.o

all: falcon

trie: $(TRIE)
	$(CC) $(TRIE) -o $@

cache_reader: $(CACHE_READER) $(SOURCES)
	$(CC) $(GLIBLIBS) $(CLIBS) $(CACHE_READER) $(SOURCES) -o $@

loader: $(LOADER) $(SOURCES)
	$(CC) $(GLIBLIBS) $(CLIBS) $(LOADER) $(SOURCES) -o $@

falcon: $(FALCON) $(SOURCES)
	$(CC) $(GLIBLIBS) $(CLIBS) $(FALCON) $(SOURCES) -o $@

xmms2_monitor: $(XMMS2_MONITOR) $(SOURCES)
	$(CC) $(GLIBLIBS) $(XMMS2LIBS) $(CLIBS) $(XMMS2_MONITOR) $(SOURCES) -o $@

$(CACHE_READER): %.o: %.c
	$(CC) $(GLIBFLAGS) $(CFLAGS) -c $< -o $@

$(LOADER): %.o: %.c
	$(CC) $(GLIBFLAGS) $(CFLAGS) -c $< -o $@

$(FALCON): %.o: %.c
	$(CC) $(GLIBFLAGS) $(CFLAGS) -c $< -o $@

$(XMMS2_MONITOR): %.o: %.c
	$(CC) $(GLIBFLAGS) $(XMMS2FLAGS) $(CFLAGS) -c $< -o $@

$(SOURCES): %.o: %.c
	$(CC) $(GLIBFLAGS) $(CFLAGS) -c $< -o $@

.PHONY: clean
clean:
	rm -f tests/*.o src/*.o falcon loader cache_reader xmms2_monitor trie *.out