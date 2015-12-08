SHELL = /bin/bash
CC ?= gcc
BINDIR ?= /usr/bin

all: dldyn

dldyn: dldyn.c dyncall/dyncall/libdyncall_s.a
	$(CC) -Wall -g dldyn.c -o $@ dyncall/dyncall/libdyncall_s.a -ldl

dyncall/dyncall/libdyncall_s.a:
	pushd dyncall; ./configure && make; popd

install: dldyn
	cp -vf dldyn $(BINDIR)/dldyn && chmod -v 755 $(BINDIR)/dldyn

clean:
	make -C dyncall clean
	rm -f dldyn
