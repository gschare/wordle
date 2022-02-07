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

.PHONY: install
install: default
	sudo chmod a+rx wordle
	sudo mkdir -p /usr/local/bin
	sudo cp ./wordle /usr/local/bin/

.PHONY: clean
clean:
	rm -f *.o a.out wordle
