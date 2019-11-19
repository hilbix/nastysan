#

BINS=nasty-nbd
LIBS=libnl-3.0 libnl-genl-3.0
DEVS=libnl-3-dev libnl-genl-3-dev

CFLAGS=-Wall -O3 $(shell pkg-config --cflags $(LIBS))
LDLIBS=$(shell pkg-config --libs $(LIBS))

love:	all

all:	$(BINS)

clean:
	rm -f $(BINS)

debian:
	sudo apt-get install $(DEVS)

