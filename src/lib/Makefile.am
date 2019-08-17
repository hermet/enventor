AUTOMAKE_OPTIONS = 1.4 foreign
MAINTAINERCLEANFILES = Makefile.in

CLEANFILES =

AM_CPPFLAGS = -DPACKAGE_DATA_DIR=\"$(pkgdatadir)\" \
              -DPACKAGE_BIN_DIR=\"$(bindir)\" \
              -DPACKAGE_LIB_DIR=\"$(libdir)\" \
              -I$(top_srcdir)/src/bin \
              -I$(top_srcdir)/src/include \
              @ENVENTOR_CFLAGS@

lib_LTLIBRARIES = libenventor.la

includes_HEADERS = \
   Enventor.h
includesdir = $(includedir)/enventor-@ENVENTOR_VERSION_MAJOR@

libenventor_la_SOURCES = \
   enventor_private.h \
   template_code.h \
   enventor_main.c \
   enventor_smart.c \
   edc_parser.c \
   redoundo.c \
   syntax_helper.c \
   syntax_color.c \
   syntax_indent.c \
   ctxpopup.c \
   edc_editor.c \
   auto_comp.c \
   build.c \
   template.c \
   edj_mgr.c \
   edj_viewer.c \
   dummy_obj.c \
   wireframes_obj.c \
   util.c \
   reference.c

libenventor_la_CFLAGS = @ENVENTOR_CFLAGS@
libenventor_la_LIBADD = @ENVENTOR_LIBS@

libenventor_la_LDFLAGS = $(LTLIBINTL) -version-info @version_info@ -no-undefined
