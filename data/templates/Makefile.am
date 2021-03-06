MAINTAINERCLEANFILES = Makefile.in

EDJE_CC = @edje_cc@
EDJE_FLAGS = -id $(top_srcdir)/data/images -sd $(top_srcdir)/data/sounds -beta

filesdir = $(datadir)/$(PACKAGE)/templates
files_DATA = Basic.edc \
             Basic.edj \
             CustomButton.edc \
             CustomButton.edj \
             Clip.edc \
             Clip.edj \
             Empty.edc \
             Empty.edj \
             Group.edc \
             Group.edj \
             Image.edc \
             Image.edj \
             Listitem.edc \
             Listitem.edj \
             Map.edc \
             Map.edj \
             MobileLayout.edc \
             MobileLayout.edj \
             MouseEvent.edc \
             MouseEvent.edj \
             Proxy.edc \
             Proxy.edj \
             Rect.edc \
             Rect.edj \
             Rotation+Zoom.edc \
             Rotation+Zoom.edj \
             Sound.edc \
             Sound.edj \
             Spacer.edc \
             Spacer.edj \
             Text.edc \
             Text.edj \
             Textblock.edc \
             Textblock.edj \
             Transition.edc \
             Transition.edj \
             Tween.edc \
             Tween.edj \
             Watchface.edc \
             Watchface.edj \
             Watchface2.edc \
             Watchface2.edj \
             Morphing.edc \
             Morphing.edj \
             Vector.edc \
             Vector.edj \
             Interpolation.edc \
             Interpolation.edj \
             BgAnimation.edc \
             BgAnimation.edj \
             GameStage.edc \
             GameStage.edj \
             Mask.edc \
             Mask.edj

EXTRA_DIST = $(files_DATA)

srcdir = $(top_srcdir)/data/templates
builddir = $(top_builddir)/data/templates

Basic.edj: Makefile Basic.edc
	$(EDJE_CC) $(EDJE_FLAGS) $(srcdir)/Basic.edc $(builddir)/Basic.edj

CustomButton.edj: Makefile CustomButton.edc
	$(EDJE_CC) $(EDJE_FLAGS) $(srcdir)/CustomButton.edc $(builddir)/CustomButton.edj

Empty.edj: Makefile Empty.edc
	$(EDJE_CC) $(EDJE_FLAGS) $(srcdir)/Empty.edc $(builddir)/Empty.edj

Map.edj: Makefile Map.edc
	$(EDJE_CC) $(EDJE_FLAGS) $(srcdir)/Map.edc $(builddir)/Map.edj

MobileLayout.edj: Makefile MobileLayout.edc
	$(EDJE_CC) $(EDJE_FLAGS) $(srcdir)/MobileLayout.edc $(builddir)/MobileLayout.edj

MouseEvent.edj: Makefile MouseEvent.edc
	$(EDJE_CC) $(EDJE_FLAGS) $(srcdir)/MouseEvent.edc $(builddir)/MouseEvent.edj

Textblock.edj: Makefile Textblock.edc
	$(EDJE_CC) $(EDJE_FLAGS) $(srcdir)/Textblock.edc $(builddir)/Textblock.edj

Transition.edj: Makefile Transition.edc
	$(EDJE_CC) $(EDJE_FLAGS) $(srcdir)/Transition.edc $(builddir)/Transition.edj

Tween.edj: Makefile Tween.edc
	$(EDJE_CC) $(EDJE_FLAGS) $(srcdir)/Tween.edc $(builddir)/Tween.edj

Rect.edj: Makefile Rect.edc
	$(EDJE_CC) $(EDJE_FLAGS) $(srcdir)/Rect.edc $(builddir)/Rect.edj

Rotation+Zoom.edj: Makefile Rotation+Zoom.edc
	$(EDJE_CC) $(EDJE_FLAGS) $(srcdir)/Rotation+Zoom.edc $(builddir)/Rotation+Zoom.edj

Group.edj: Makefile Group.edc
	$(EDJE_CC) $(EDJE_FLAGS) $(srcdir)/Group.edc $(builddir)/Group.edj

Listitem.edj: Makefile Listitem.edc
	$(EDJE_CC) $(EDJE_FLAGS) $(srcdir)/Listitem.edc $(builddir)/Listitem.edj

Image.edj: Makefile Image.edc
	$(EDJE_CC) $(EDJE_FLAGS) $(srcdir)/Image.edc $(builddir)/Image.edj

Text.edj: Makefile Text.edc
	$(EDJE_CC) $(EDJE_FLAGS) $(srcdir)/Text.edc $(builddir)/Text.edj

Proxy.edj: Makefile Proxy.edc
	$(EDJE_CC) $(EDJE_FLAGS) $(srcdir)/Proxy.edc $(builddir)/Proxy.edj

Clip.edj: Makefile Clip.edc
	$(EDJE_CC) $(EDJE_FLAGS) $(srcdir)/Clip.edc $(builddir)/Clip.edj

Spacer.edj: Makefile Spacer.edc
	$(EDJE_CC) $(EDJE_FLAGS) $(srcdir)/Spacer.edc $(builddir)/Spacer.edj

Sound.edj: Makefile Sound.edc
	$(EDJE_CC) $(EDJE_FLAGS) $(srcdir)/Sound.edc $(builddir)/Sound.edj

Watchface.edj: Makefile Watchface.edc
	$(EDJE_CC) $(EDJE_FLAGS) $(srcdir)/Watchface.edc $(builddir)/Watchface.edj

Watchface2.edj: Makefile Watchface2.edc
	$(EDJE_CC) $(EDJE_FLAGS) $(srcdir)/Watchface2.edc $(builddir)/Watchface2.edj

Morphing.edj: Makefile Morphing.edc
	$(EDJE_CC) $(EDJE_FLAGS) $(srcdir)/Morphing.edc $(builddir)/Morphing.edj

Vector.edj: Makefile Vector.edc
	$(EDJE_CC) $(EDJE_FLAGS) $(srcdir)/Vector.edc $(builddir)/Vector.edj

Interpolation.edj: Makefile Interpolation.edc
	$(EDJE_CC) $(EDJE_FLAGS) $(srcdir)/Interpolation.edc $(builddir)/Interpolation.edj

BgAnimation.edj: Makefile BgAnimation.edc
	$(EDJE_CC) $(EDJE_FLAGS) $(srcdir)/BgAnimation.edc $(builddir)/BgAnimation.edj

GameStage.edj: Makefile GameStage.edc
	$(EDJE_CC) $(EDJE_FLAGS) $(srcdir)/GameStage.edc $(builddir)/GameStage.edj

Mask.edj: Makefile Mask.edc
	$(EDJE_CC) $(EDJE_FLAGS) $(srcdir)/Mask.edc $(builddir)/Mask.edj

clean-local:
	rm -f *.edj
	rm -f $(datadir)/$(PACKAGE)/templates/*edj
