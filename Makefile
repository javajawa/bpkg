#!/usr/bin/make -f

.DEFAULT_GOAL = build
.PHONY = clean build real-build bootstrap

MAKEFILE := $(lastword $(MAKEFILE_LIST))
TARGETS=$(addprefix usr/bin/,make-package ar-stream tar-stream bpkg-build)

CFLAGS=-std=c11 -Wall -Wextra -Werror -pedantic -O2

%.o :: src/%.c
	$(CC) -c $(CFLAGS) $< -o $@

usr/bin/tar-stream: tar-stream.o null-stream.o
	$(CC) $(CFLAGS) -o $@ $^

usr/bin/ar-stream: ar-stream.o null-stream.o
	$(CC) $(CFLAGS) -o $@ $^

usr/bin/make-package: make-package.o null-stream.o
	$(CC) $(CFLAGS) -o $@ $^

usr/bin/bpkg-build: src/bpkg-build
	cp $< $@

build:
	mkdir -p usr/bin
	$(MAKE) -f $(MAKEFILE) --no-print-directory real-build

real-build: $(TARGETS)

manifest: build
	find ./etc ./usr >$@

bootstrap: build
	PATH=./usr/bin:${PATH} bpkg-build .
	echo "Run \`sudo dpkg -i /srv/www/bpkg/pool/bpkg_1.0-1.deb\` to install"

clean:
	rm -Rf *.o usr manifest
