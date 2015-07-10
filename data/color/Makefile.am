MAINTAINERCLEANFILES = Makefile.in

EET_EET = @eet_eet@

EXTRA_DIST = \
edc.src \
syntax_template.dat

filesdir = $(datadir)/$(PACKAGE)/color
files_DATA = \
edc.eet \
syntax_template.dat

%.eet: %.src
	$(EET_EET) -e \
	$(top_builddir)/data/color/$@ color \
	$< 1

clean-local:
	rm -rf *.eet *~
