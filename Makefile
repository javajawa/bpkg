#!/usr/bin/make -f

.DEFAULT_GOAL = build
.PHONY = clean build debug bootstrap deps valgrind

TARGETS := $(addprefix usr/bin/,make-package ar-stream tar-stream bpkg-build bpkg-checkbuilddeps bpkg-make-list)

PACKAGE := $(shell grep '^Package:' 'debian/control')
PACKAGE := $(subst Package: ,,$(PACKAGE))
ARCH    := $(shell grep '^Architecture:' 'debian/control')
ARCH    := $(subst Architecture: ,,$(ARCH))
VERSION ?= 1.0-1~bootstrap

BUILD   := build

COM_DEPS  := $(subst src/,$(BUILD)/,$(subst .c,.o,$(wildcard src/common/*.c)))
TAR_DEPS  := $(subst src/,$(BUILD)/,$(subst .c,.o,$(wildcard src/tar-stream/*.c)))
AR_DEPS   := $(subst src/,$(BUILD)/,$(subst .c,.o,$(wildcard src/ar-stream/*.c)))
BPKG_DEPS := $(subst src/,$(BUILD)/,$(subst .c,.o,$(wildcard src/make-package/*.c)))

CFLAGS := -std=c11 -D_GNU_SOURCE -D_FORTIFY_SOURCE=2 -Wall -Wextra -Werror -pedantic -Isrc/ $(CFLAGS)

build: CFLAGS += -O2 -s
build: $(TARGETS)

debug: CFLAGS += -O0 -g -DDEBUG
debug: $(TARGETS)

build/%.o : src/%.c
	@mkdir -vp $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

obj/make-package/%.o : src/make-package/%.c
	@mkdir -vp $(dir $@)
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

usr/bin/bpkg-build: src/bpkg-build/bpkg-build
	@mkdir -vp $(dir $@)
	cp $< $@

usr/bin/bpkg-checkbuilddeps: src/checkbuilddeps/checkbuilddeps
	@mkdir -vp $(dir $@)
	cp $< $@

usr/bin/bpkg-make-list: src/bpkg-make-list/bpkg-make-list
	@mkdir -vp $(dir $@)
	cp $< $@

manifest: build
	find etc usr >$@

bootstrap: build
	rm -vf '$(PACKAGE)_$(VERSION)_$(ARCH).deb'
	rm -vf '$(PACKAGE)_$(VERSION)_$(ARCH).deb.dat'
	PATH=./usr/bin:${PATH} bpkg-build . $(VERSION)
	@printf "\nRun \`sudo apt install './$(PACKAGE)_$(VERSION)_$(ARCH).deb'\`\n"

deps:
	# Build-Depends:
	command -v cc || sudo apt --no-install-recommends install gcc
	command -v make || sudo apt --no-install-recommends install make
	# Depends:
	sudo apt --no-install-recommends install libc6 make xz-utils libdpkg-perl libdpkg-parse-perl

valgrind: debug manifest
	rm -f ./test-bpkg.deb ./test-bpkg.deb.dat
	PATH=./usr/bin:${PATH} valgrind --quiet --leak-check=full --track-origins=yes --trace-children=yes --trace-children-skip=\*sum,\*xz usr/bin/make-package . test-bpkg.deb
	dpkg -c test-bpkg.deb

clean:
	rm -Rf $(BUILD) usr manifest
	rm -f test-bpkg.deb test-bpkg.deb.dat '$(PACKAGE)_$(VERSION)_$(ARCH).deb' '$(PACKAGE)_$(VERSION)_$(ARCH).deb.dat'
