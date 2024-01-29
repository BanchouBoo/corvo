CC ?= cc

SRC = main.c

CFLAGS = -Wall -Wextra -pedantic
DEPENDS = xcb xcb-xfixes
LDFLAGS = `pkg-config --cflags --libs $(DEPENDS)`

PREFIX ?= /usr/local
TARGET ?= corvo

all:
	$(CC) $(SRC) -o $(TARGET) $(CFLAGS) $(LDFLAGS)

install: all
	install -D -m 755 $(TARGET) "$(DESTDIR)$(PREFIX)/bin/$(TARGET)"

uninstall:
	rm -f "$(DESTDIR)$(PREFIX)/bin/$(TARGET)"
