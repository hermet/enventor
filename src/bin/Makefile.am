MAINTAINERCLEANFILES = Makefile.in

bin_PROGRAMS = enventor

AM_CPPFLAGS = -DPACKAGE_DATA_DIR=\"$(pkgdatadir)\" \
	-DPACKAGE_BIN_DIR=\"$(bindir)\" \
	-DPACKAGE_LIB_DIR=\"$(libdir)\" \
	-I$(top_srcdir)/src/bin \
	-I$(top_srcdir)/src/include \
	@ENVENTOR_CFLAGS@

enventor_SOURCES = \
	base_gui.c \
	config_data.c \
	dummy_obj.c \
	edc_editor.c \
	edc_parser.c \
	edj_viewer.c \
	main.c \
	menu.c \
	panes.c \
	statusbar.c \
	syntax_color.c \
	ctxpopup.c \
	syntax_helper.c \
	indent.c \
	edj_mgr.c \
	build.c \
	tools.c \
	search.c \
	newfile.c \
	globals.c

enventor_LDADD = @ENVENTOR_LIBS@
enventor_LDFLAGS = $(LTLIBINTL)

EXTRA_DIST =
