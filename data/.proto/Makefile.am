MAINTAINERCLEANFILES = Makefile.in

EDJE_CC = @edje_cc@
EDJE_FLAGS = -id $(top_srcdir)/data/images

filesdir = $(datadir)/$(PACKAGE)/.proto
files_DATA = proto.edj proto.edc

proto.edj: Makefile proto.edc
	$(EDJE_CC) $(EDJE_FLAGS) \
	$(top_srcdir)/data/.proto/proto.edc \
	$(top_builddir)/data/.proto/proto.edj

clean-local:
	rm -f *.edj
