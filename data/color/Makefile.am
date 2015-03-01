MAINTAINERCLEANFILES = Makefile.in

EET_EET = @eet_eet@

EXTRA_DIST = \
color.src \
syntax_template.dat

filesdir = $(datadir)/$(PACKAGE)/color
files_DATA = \
color.eet \
syntax_template.dat

%.eet: %.src
	$(EET_EET) -e \
	$(top_builddir)/data/color/$@ color \
	$< 1

clean-local:
	rm -rf *.eet *~
