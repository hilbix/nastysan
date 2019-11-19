#

LIBS=libnl-3.0 libnl-genl-3.0
DEVS=libnl-3-dev libnl-genl-3-dev

love:	all

debian:
	sudo apt-get install $(DEVS)

SRCS=$(wildcard *.c)
OBJS=$(patsubst %.c,%.o,$(SRCS))
DEPS=$(patsubst %.c,%.d,$(SRCS))
PROG=$(patsubst %.c,%,$(SRCS))

CFLAGS=-Wall -O3 $(shell pkg-config --cflags $(LIBS))
LDLIBS=$(shell pkg-config --libs $(LIBS))

all:	$(DEPS)
	@$(MAKE) $(PROG)

clean:
	$(RM) $(DEPS) $(OBJS) $(PROG)

ifneq ($(filter clean,$(MAKECMDGOALS)),clean)
include $(DEPS)
endif

%.d:	%.c
	$(info depends $@)
	@rm -f '$@' && $(CC) -MM $(CFLAGS) $< | sed 's,\($*\)\.o[ :]*,\1.o $@ : ,g' >'$@'

%: %d

