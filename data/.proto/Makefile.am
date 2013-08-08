MAINTAINERCLEANFILES = Makefile.in

EDJE_CC = @edje_cc@
EDJE_FLAGS = -id $(top_srcdir)/data/images

filesdir = $(datadir)/$(PACKAGE)/themes
files_DATA = proto.edj

proto.edj: Makefile $(EXTRA_DIST)
	$(EDJE_CC) $(EDJE_FLAGS) \
	$(top_srcdir)/data/.proto/proto.edc \
	$(top_builddir)/data/.proto/proto.edj

clean-local:
        rm -f *.edj
