SUBDIRS = libgstbuzztard src docs tests

ACLOCAL_AMFLAGS = -I m4
DISTCHECK_CONFIGURE_FLAGS = --enable-gtk-doc

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libgstbuzztard.pc

EXTRA_DIST = \
	libgstbuzztard.pc.in \
	TODO

.PHONY: coverage valgrind

# make coverage        -- run all checks and generate html coverage report (only availble when built using --enable-coverage)
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

# make valgrind        -- run all checks under valgrind
if USE_VALGRIND
valgrind::
	$(MAKE) -C tests valgrind
endif

CLEANFILES = $(pkgconfig_DATA)

include $(top_srcdir)/common.mak
