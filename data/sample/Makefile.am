MAINTAINERCLEANFILES = Makefile.in

# We should install them in some example directory, but don't know where
EXTRA_DIST = bg.jpg \
	clouds.png \
	etypers.edj \
	input_bg.png \
	sample.edc \
	sky.jpg     

EDJE_CC = @edje_cc@
EDJE_FLAGS = -id $(top_srcdir)/data/themes/images

filesdir = $(datadir)/$(PACKAGE)/sample
files_DATA = sample.edj

sample.edj: Makefile $(EXTRA_DIST)
	$(EDJE_CC) $(EDJE_FLAGS) \
	$(top_srcdir)/data/sample/sample.edc \
	$(top_builddir)/data/sample/sample.edj

clean-local:
	rm -f *.edj
