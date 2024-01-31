CC=vc
#CFLAGS += -Idevkits:compilers/vbcc/0.9h.3/PosixLib/include -IFiles:dev/c/lib/tools -c99 -+ -dontwarn=214 -DDEBUG -g
CFLAGS += -Idevkits:compilers/vbcc/0.9h.3/PosixLib/include -IFiles:dev/c/lib/tools -c99 -+ -dontwarn=214
LDFLAGS += -Ldevkits:compilers/vbcc/0.9h.3/PosixLib/AmigaOS3 -lposix

SOURCES=$(wildcard *.c)
OBJECTS=$(patsubst %.c,%.o,$(SOURCES))
TESTS=$(patsubst %.c,%,$(SOURCES))


.PHONY: all clean

# Default build
all: $(TESTS)


clean:
	@-echo sources=$(SOURCES)
	@-echo objects=$(OBJECTS)
	@-echo tests=$(TESTS)
	@-delete $(TESTS) ALL QUIET
	@-delete $(OBJECTS) ALL QUIET

