MAINTAINERCLEANFILES = Makefile.in

SUBDIRS = images

EDJE_CC = @edje_cc@
EDJE_FLAGS = -id $(top_srcdir)/data/themes/default/images

filesdir = $(datadir)/$(PACKAGE)/themes
files_DATA = enventor.edj

EXTRA_DIST = enventor.edc \
             content.edc \
             images.edc \
             layout.edc \
             menu.edc \
             theme_ext.edc

enventor.edj: Makefile $(EXTRA_DIST)
	$(EDJE_CC) $(EDJE_FLAGS) \
	$(top_srcdir)/data/themes/default/enventor.edc \
	$(top_builddir)/data/themes/default/enventor.edj

clean-local:
	rm -f *.edj
