MAINTAINERCLEANFILES = Makefile.in

EDJE_CC = @edje_cc@
EDJE_FLAGS = -id $(top_srcdir)/data/themes/dark/images

filesdir = $(datadir)/$(PACKAGE)/themes
files_DATA = enventor_dark.edj

EXTRA_DIST = enventor_dark.edc

enventor_dark.edj: Makefile $(EXTRA_DIST)
	$(EDJE_CC) $(EDJE_FLAGS) \
	$(top_srcdir)/data/themes/dark/enventor_dark.edc \
	$(top_builddir)/data/themes/dark/enventor_dark.edj

clean-local:
	rm -f *.edj
