CC       = gcc
INCLUDES = -Ilib
CFLAGS   = -g -Wall $(INCLUDES)
LDFLAGS  = -g -Llib
LDLIBS   = -lmylist

.PHONY: default
default: list wordle

.PHONY: list
list:
	cd lib && $(MAKE)

.PHONY: clean
clean:
	rm -f *.o a.out wordle
