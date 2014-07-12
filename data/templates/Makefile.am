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

srcdir = $(top_srcdir)/data/templates
builddir = $(top_builddir)/data/templates

basic.edj: Makefile basic.edc
	$(EDJE_CC) $(EDJE_FLAGS) $(srcdir)/basic.edc $(builddir)/basic.edj

empty.edj: Makefile empty.edc
	$(EDJE_CC) $(EDJE_FLAGS) $(srcdir)/empty.edc $(builddir)/empty.edj

map.edj: Makefile map.edc
	$(EDJE_CC) $(EDJE_FLAGS) $(srcdir)/map.edc $(builddir)/map.edj

textblock.edj: Makefile textblock.edc
	$(EDJE_CC) $(EDJE_FLAGS) $(srcdir)/textblock.edc $(builddir)/textblock.edj

transition.edj: Makefile transition.edc
	$(EDJE_CC) $(EDJE_FLAGS) $(srcdir)/transition.edc $(builddir)/transition.edj

tween.edj: Makefile tween.edc
	$(EDJE_CC) $(EDJE_FLAGS) $(srcdir)/tween.edc $(builddir)/tween.edj

rect.edj: Makefile rect.edc
	$(EDJE_CC) $(EDJE_FLAGS) $(srcdir)/rect.edc $(builddir)/rect.edj

image.edj: Makefile image.edc
	$(EDJE_CC) $(EDJE_FLAGS) $(srcdir)/image.edc $(builddir)/image.edj

text.edj: Makefile text.edc
	$(EDJE_CC) $(EDJE_FLAGS) $(srcdir)/text.edc $(builddir)/text.edj

clean-local:
	rm -f *.edj
