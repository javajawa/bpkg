#!/usr/bin/make -f

.DEFAULT_GOAL = build
.PHONY = clean build real-build

MAKEFILE := $(lastword $(MAKEFILE_LIST))
TARGETS=usr/bin/make-package usr/bin/ar-stream usr/bin/tar-stream usr/bin/bpkg-build

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

clean:
	rm -Rf *.o usr manifest
