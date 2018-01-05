#!/usr/bin/make -f

.DEFAULT_GOAL = build
.PHONY = clean build debug bootstrap deps

TARGETS := $(addprefix usr/bin/,make-package ar-stream tar-stream bpkg-build bpkg-checkbuilddeps bpkg-make-list)

PACKAGE := $(shell grep '^Package:' 'debian/control')
PACKAGE := $(subst Package: ,,$(PACKAGE))
VERSION := $(shell grep '^Version:' 'debian/control')
VERSION := $(subst Version: ,,$(VERSION))

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
	rm -vf /srv/www/bpkg/pool/$(PACKAGE)_$(VERSION).deb
	rm -vf /srv/www/bpkg/pool/$(PACKAGE)_$(VERSION).deb.dat
	PATH=./usr/bin:${PATH} bpkg-build .
	@printf "\nRun \`sudo apt install /srv/www/bpkg/pool/$(PACKAGE)_$(VERSION).deb\`\n"

valgrind: debug manifest
	rm -f valgrind
	valgrind --leak-check=full --track-origins=yes --trace-children=yes --trace-children-skip=\*sum,\*xz usr/bin/make-package . valgrind

deps:
	# Build-Depends:
	sudo apt --no-install-recommends c-compiler make
	# Depends:
	sudo apt --no-install-recommends libc6 make xz-utils libdpkg-perl libdpkg-parse-perl

valgrind: debug manifest
	rm -f valgrind
	valgrind --leak-check=full --track-origins=yes --trace-children=yes --trace-children-skip=\*sum,\*xz usr/bin/make-package . valgrind

clean:
	rm -Rf $(BUILD) usr manifest
