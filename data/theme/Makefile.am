MAINTAINERCLEANFILES = Makefile.in

EDJE_CC = @edje_cc@
EDJE_FLAGS = -id $(top_srcdir)/data/theme/images

filesdir = $(datadir)/$(PACKAGE)/theme
files_DATA = enventor.edj

EXTRA_DIST = enventor.edc

enventor.edj: Makefile $(EXTRA_DIST)
	$(EDJE_CC) $(EDJE_FLAGS) \
	$(top_srcdir)/data/theme/enventor.edc \
	$(top_builddir)/data/theme/enventor.edj

clean-local:
	rm -f *.edj
