#!/usr/bin/make -f

.DEFAULT_GOAL = build
.PHONY = clean build real-build bootstrap

MAKEFILE := $(lastword $(MAKEFILE_LIST))
TARGETS=$(addprefix usr/bin/,make-package ar-stream tar-stream bpkg-build)

PACKAGE:=$(shell grep '^Package:' 'debian/control')
PACKAGE:=$(subst Package: ,,$(PACKAGE))
VERSION:=$(shell grep '^Version:' 'debian/control')
VERSION:=$(subst Version: ,,$(VERSION))

CFLAGS=-std=c11 -Wall -Wextra -Werror -pedantic -O2

%.o :: src/%.c
	$(CC) -c $(CFLAGS) $< -o $@

usr/bin/tar-stream: tar-stream.o null-stream.o
	@mkdir -vp $(dir $@)
	$(CC) $(CFLAGS) -o $@ $^

usr/bin/ar-stream: ar-stream.o null-stream.o
	@mkdir -vp $(dir $@)
	$(CC) $(CFLAGS) -o $@ $^

usr/bin/make-package: make-package.o null-stream.o
	@mkdir -vp $(dir $@)
	$(CC) $(CFLAGS) -o $@ $^

usr/bin/bpkg-build: src/bpkg-build
	@mkdir -vp $(dir $@)
	cp $< $@


build: $(TARGETS)

manifest: build
	find etc usr >$@

bootstrap: build
	rm -vf /srv/www/bpkg/pool/$(PACKAGE)_$(VERSION).deb
	PATH=./usr/bin:${PATH} bpkg-build .
	@printf "\nRun \`sudo apt install /srv/www/bpkg/pool/$(PACKAGE)_$(VERSION).deb\`\n"

clean:
	rm -Rf *.o usr manifest
