MAINTAINERCLEANFILES = Makefile.in

EET_EET = @eet_eet@

EXTRA_DIST = \
color.src

filesdir = $(datadir)/$(PACKAGE)/color
files_DATA = \
color.eet

%.eet: %.src
	$(EET_EET) -e \
	$(top_builddir)/data/color/$@ color \
	$< 1

clean-local:
	rm -rf *.eet *~
