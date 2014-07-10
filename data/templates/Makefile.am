MAINTAINERCLEANFILES = Makefile.in

EDJE_CC = @edje_cc@
EDJE_FLAGS = -id $(top_srcdir)/data/images

filesdir = $(datadir)/$(PACKAGE)/templates
files_DATA = basic.edc \
             basic.edj \
             empty.edc \
             empty.edj \
             image.edc \
             image.edj \
             map.edc \
             map.edj \
             rect.edc \
             rect.edj \
             text.edc \
             text.edj \
             textblock.edc \
             textblock.edj \
             transition.edc \
             transition.edj \
             tween.edc \
             tween.edj

EXTRA_DIST = $(files_DATA)

basic.edj: Makefile $(EXTRA_DIST)
	$(EDJE_CC) $(EDJE_FLAGS) \
	$(top_srcdir)/data/templates/basic.edc \
	$(top_builddir)/data/templates/basic.edj

empty.edj: Makefile $(EXTRA_DIST)
	$(EDJE_CC) $(EDJE_FLAGS) \
	$(top_srcdir)/data/templates/empty.edc \
	$(top_builddir)/data/templates/empty.edj

map.edj: Makefile $(EXTRA_DIST)
	$(EDJE_CC) $(EDJE_FLAGS) \
	$(top_srcdir)/data/templates/map.edc \
	$(top_builddir)/data/templates/map.edj

textblock.edj: Makefile $(EXTRA_DIST)
	$(EDJE_CC) $(EDJE_FLAGS) \
	$(top_srcdir)/data/templates/textblock.edc \
	$(top_builddir)/data/templates/textblock.edj

transition.edj: Makefile $(EXTRA_DIST)
	$(EDJE_CC) $(EDJE_FLAGS) \
	$(top_srcdir)/data/templates/transition.edc \
	$(top_builddir)/data/templates/transition.edj

tween.edj: Makefile $(EXTRA_DIST)
	$(EDJE_CC) $(EDJE_FLAGS) \
	$(top_srcdir)/data/templates/tween.edc \
	$(top_builddir)/data/templates/tween.edj

rect.edj: Makefile $(EXTRA_DIST)
	$(EDJE_CC) $(EDJE_FLAGS) \
	$(top_srcdir)/data/templates/rect.edc \
	$(top_builddir)/data/templates/rect.edj

image.edj: Makefile $(EXTRA_DIST)
	$(EDJE_CC) $(EDJE_FLAGS) \
	$(top_srcdir)/data/templates/image.edc \
	$(top_builddir)/data/templates/image.edj

text.edj: Makefile $(EXTRA_DIST)
	$(EDJE_CC) $(EDJE_FLAGS) \
	$(top_srcdir)/data/templates/text.edc \
	$(top_builddir)/data/templates/text.edj

clean-local:
	rm -f *.edj
