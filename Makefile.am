MAINTAINERCLEANFILES = \
Makefile.in \
aclocal.m4 \
config.guess \
config.h.in \
config.sub \
configure \
install-sh \
depcomp \
libtool \
missing

filesdir = $(datadir)/$(PACKAGE)/docs
files_DATA = README

ACLOCAL_AMFLAGS = -I m4

EXTRA_DIST = \
README \
AUTHORS \
COPYING \
autogen.sh

SUBDIRS = src data . po

pkgconfigdir = $(libdir)/pkgconfig

pkgconfig_DATA = pc/enventor.pc

update-po:
	$(MAKE) $(AM_MAKEFLAGS) -C po update-po
