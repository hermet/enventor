MAINTAINERCLEANFILES = Makefile.in

EDJE_CC = @edje_cc@
EDJE_FLAGS = -id $(top_srcdir)/data/images

filesdir = $(datadir)/$(PACKAGE)/templates
files_DATA = basic.edc \
				 basic.edj \
				 empty.edc \
				 empty.edj \
				 textblock.edc \
				 textblock.edj

EXTRA_DIST = $(files_DATA)

basic.edj: Makefile $(EXTRA_DIST)
	$(EDJE_CC) $(EDJE_FLAGS) \
	$(top_srcdir)/data/templates/basic.edc \
	$(top_builddir)/data/templates/basic.edj

empty.edj: Makefile $(EXTRA_DIST)
	$(EDJE_CC) $(EDJE_FLAGS) \
	$(top_srcdir)/data/templates/empty.edc \
	$(top_builddir)/data/templates/empty.edj

textblock.edj: Makefile $(EXTRA_DIST)
	$(EDJE_CC) $(EDJE_FLAGS) \
	$(top_srcdir)/data/templates/textblock.edc \
	$(top_builddir)/data/templates/textblock.edj

clean-local:
	rm -f *.edj
