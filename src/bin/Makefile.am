MAINTAINERCLEANFILES = Makefile.in

bin_PROGRAMS = enventor enventorql

AM_CPPFLAGS = -DPACKAGE_DATA_DIR=\"$(pkgdatadir)\" \
	-DPACKAGE_BIN_DIR=\"$(bindir)\" \
	-DPACKAGE_LIB_DIR=\"$(libdir)\" \
	-I$(top_srcdir)/src/bin \
	-I$(top_srcdir)/src/include \
        @ENVENTOR_CFLAGS@

enventor_SOURCES = \
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
	shared_res.c

# QuickLaunch                                                
enventorql_SOURCES = $(enventor_SOURCES)
enventorql_LTLIBRARIES = enventorql.la
enventorql_CFLAGS = -DELM_LIB_QUICKLAUNCH=1
enventorql_LDFLAGS = @ENVENTOR_LIBS@

enventorqldir = $(libdir)
enventorql_la_LIBADD = @ENVENTOR_LIBS@
enventorql_la_CFLAGS =
enventorql_la_LDFLAGS = -module -avoid-version -no-undefined
enventorql_la_SOURCES = $(enventor_SOURCES)

enventor_LDADD = @ENVENTOR_LIBS@
enventor_LDFLAGS = $(LTLIBINTL)

EXTRA_DIST =
