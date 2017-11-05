#!/usr/bin/make -f

.DEFAULT_GOAL = build
.PHONY = clean build bootstrap

MAKEFILE := $(lastword $(MAKEFILE_LIST))
TARGETS=$(addprefix usr/bin/,make-package ar-stream tar-stream bpkg-build bpkg-checkbuilddeps)

PACKAGE := $(shell grep '^Package:' 'debian/control')
PACKAGE := $(subst Package: ,,$(PACKAGE))
VERSION := $(shell grep '^Version:' 'debian/control')
VERSION := $(subst Version: ,,$(VERSION))

COM_DEPS  := $(subst .c,.o,$(wildcard common/*.c))
TAR_DEPS  := $(subst .c,.o,$(wildcard tar-stream/*.c))
AR_DEPS   := $(subst .c,.o,$(wildcard ar-stream/*.c))
BPKG_DEPS := $(subst .c,.o,$(wildcard make-package/*.c))

CFLAGS=-std=c11 -Wall -Wextra -Werror -pedantic -O2 -I.

%.o : src/%.c
	$(CC) $(CFLAGS) -c $< -o $@

usr/bin/tar-stream: $(TAR_DEPS) $(COM_DEPS)
	@mkdir -vp $(dir $@)
	$(CC) $(CFLAGS) -o $@ $^

usr/bin/ar-stream: $(AR_DEPS) $(COM_DEPS)
	@mkdir -vp $(dir $@)
	$(CC) $(CFLAGS) -o $@ $^

usr/bin/make-package: $(BPKG_DEPS) $(COM_DEPS)
	@mkdir -vp $(dir $@)
	$(CC) $(CFLAGS) -o $@ $^

usr/bin/bpkg-build: src/bpkg-build
	@mkdir -vp $(dir $@)
	cp $< $@

usr/bin/bpkg-checkbuilddeps: src/checkbuilddeps
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
	rm -Rf $(COM_DEPS) $(TAR_DEPS) $(AR_DEPS) $(BPKG_DEPS) usr manifest
