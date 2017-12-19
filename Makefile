#!/usr/bin/make -f

.DEFAULT_GOAL = build
.PHONY = clean build debug bootstrap deps

TARGETS := $(addprefix usr/bin/,make-package ar-stream tar-stream bpkg-build bpkg-checkbuilddeps)

PACKAGE := $(shell grep '^Package:' 'debian/control')
PACKAGE := $(subst Package: ,,$(PACKAGE))
VERSION := $(shell grep '^Version:' 'debian/control')
VERSION := $(subst Version: ,,$(VERSION))

COM_DEPS  := $(subst .c,.o,$(wildcard common/*.c))
TAR_DEPS  := $(subst .c,.o,$(wildcard tar-stream/*.c))
AR_DEPS   := $(subst .c,.o,$(wildcard ar-stream/*.c))
BPKG_DEPS := $(subst .c,.o,$(wildcard make-package/*.c))


CFLAGS := -std=c11 -Wall -Wextra -Werror -pedantic -I.

build: CFLAGS += -O2 -s
build: $(TARGETS)

debug: CFLAGS += -O0 -g -DDEBUG
debug: $(TARGETS)

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

manifest: build
	find etc usr >$@

bootstrap: build
	rm -vf /srv/www/bpkg/pool/$(PACKAGE)_$(VERSION).deb
	rm -vf /srv/www/bpkg/pool/$(PACKAGE)_$(VERSION).deb.dat
	PATH=./usr/bin:${PATH} bpkg-build .
	@printf "\nRun \`sudo apt install /srv/www/bpkg/pool/$(PACKAGE)_$(VERSION).deb\`\n"

deps:
	sudo apt --no-install-recommends c-compiler make
	sudo apt --no-install-recommends libc6 make xz-utils libdpkg-perl libdpkg-parse-perl

valgrind: debug manifest
	rm -f valgrind
	valgrind --leak-check=full --track-origins=yes --trace-children=yes --trace-children-skip=\*sum,\*xz usr/bin/make-package . valgrind

clean:
	rm -Rf $(COM_DEPS) $(TAR_DEPS) $(AR_DEPS) $(BPKG_DEPS) usr manifest
