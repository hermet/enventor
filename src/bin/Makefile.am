MAINTAINERCLEANFILES = Makefile.in

LOCALE_DIR = @LOCALE_DIR@

bin_PROGRAMS = enventor

AM_CPPFLAGS = -DPACKAGE_DATA_DIR=\"$(pkgdatadir)\" \
	-DPACKAGE_BIN_DIR=\"$(bindir)\" \
	-DPACKAGE_LIB_DIR=\"$(libdir)\" \
	-I$(top_srcdir)/src/lib \
	-I$(top_srcdir)/src/bin \
	-I$(top_srcdir)/src/include \
	@ENVENTOR_CFLAGS@

enventor_SOURCES = \
	base_gui.c \
	config_data.c \
	main.c \
	menu.c \
	panes.c \
	statusbar.c \
	tools.c \
	search.c \
	goto.c \
	newfile.c \
	setting.c \
	text_setting.c \
	utils.c \
	live_edit.c \
	console.c \
	file_mgr.c \
	edc_navigator.c \
	file_browser.c \
	build_setting.c \
	preference_setting.c \
	help.c \
	file_tab.c

enventor_LDADD = \
	$(top_builddir)/src/lib/libenventor.la \
   @ENVENTOR_LIBS@
enventor_LDFLAGS = $(LTLIBINTL)

enventor_CFLAGS = \
   -I$(top_builddir)/src/lib \
	@ENVENTOR_CFLAGS@ \
	-DLOCALE_DIR=\"@LOCALE_DIR@\"

EXTRA_DIST =
