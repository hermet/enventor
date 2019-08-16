MAINTAINERCLEANFILES = Makefile.in

EET_EET = @eet_eet@

EXTRA_DIST = autocomp.src

filesdir = $(datadir)/$(PACKAGE)/autocomp
files_DATA = autocomp.eet

%.eet: %.src
	$(EET_EET) -e \
	$(top_builddir)/data/autocomp/$@ node \
	$< 1

clean-local:
	rm -rf *.eet *~
