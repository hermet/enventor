MAINTAINERCLEANFILES = Makefile.in

EDJE_CC = @edje_cc@
EDJE_FLAGS = -id $(top_srcdir)/data/images -sd $(top_srcdir)/data/sounds

filesdir = $(datadir)/$(PACKAGE)/templates
files_DATA = basic.edc \
             basic.edj \
             clip.edc \
             clip.edj \
             empty.edc \
             empty.edj \
             image.edc \
             image.edj \
             map.edc \
             map.edj \
             minimum.edc \
             minimum.edj \
             proxy.edc \
             proxy.edj \
             rect.edc \
             rect.edj \
             rotation+zoom.edc \
             rotation+zoom.edj \
             sound.edc \
             sound.edj \
             spacer.edc \
             spacer.edj \
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

rotation+zoom.edj: Makefile rotation+zoom.edc
	$(EDJE_CC) $(EDJE_FLAGS) $(srcdir)/rotation+zoom.edc $(builddir)/rotation+zoom.edj

image.edj: Makefile image.edc
	$(EDJE_CC) $(EDJE_FLAGS) $(srcdir)/image.edc $(builddir)/image.edj

text.edj: Makefile text.edc
	$(EDJE_CC) $(EDJE_FLAGS) $(srcdir)/text.edc $(builddir)/text.edj

proxy.edj: Makefile proxy.edc
	$(EDJE_CC) $(EDJE_FLAGS) $(srcdir)/proxy.edc $(builddir)/proxy.edj

clip.edj: Makefile clip.edc
	$(EDJE_CC) $(EDJE_FLAGS) $(srcdir)/clip.edc $(builddir)/clip.edj

spacer.edj: Makefile spacer.edc
	$(EDJE_CC) $(EDJE_FLAGS) $(srcdir)/spacer.edc $(builddir)/spacer.edj

sound.edj: Makefile sound.edc
	$(EDJE_CC) $(EDJE_FLAGS) $(srcdir)/sound.edc $(builddir)/sound.edj

minimum.edj: Makefile minimum.edc
	$(EDJE_CC) $(EDJE_FLAGS) $(srcdir)/minimum.edc $(builddir)/minimum.edj

clean-local:
	rm -f *.edj
