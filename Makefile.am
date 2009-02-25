## $Id$

SUBDIRS = libgstbuzztard src docs tests

ACLOCAL_AMFLAGS = -I m4
DISTCHECK_CONFIGURE_FLAGS = --enable-gtk-doc
CONFIG_CLEAN_FILES = shave·shave-libtool

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libgstbuzztard.pc

EXTRA_DIST = \
	libgstbuzztard.pc.in \
	TODO

.PHONY: help coverage valgrind

if USE_LCOV
coverage::
	mkdir -p ./coverage
	lcov --compat-libtool --directory . --zerocounters
	-$(MAKE) check
	lcov --compat-libtool --directory ./src --capture --output-file ./coverage/$(PACKAGE)_tmp.info
	lcov --compat-libtool --extract ./coverage/$(PACKAGE)_tmp.info "$(PWD)/src*" -o ./coverage/$(PACKAGE).info
	$(RM) ./coverage/$(PACKAGE)_tmp.info
	genhtml -o ./coverage --num-spaces 2 ./coverage/$(PACKAGE).info
endif

if USE_VALGRIND
valgrind::
	$(MAKE) -C tests valgrind
endif

