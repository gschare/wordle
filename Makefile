CC       = gcc
INCLUDES = -I../cs3157-lab3/solutions/part1
CFLAGS   = -g -Wall $(INCLUDES)
LDFLAGS  = -g -L../cs3157-lab3/solutions/part1
LDLIBS   = -lmylist

.PHONY: default
default: wordle

.PHONY: clean
clean:
	rm -f *.o a.out wordle
