AUTOMAKE_OPTIONS = 1.4 foreign
MAINTAINERCLEANFILES = Makefile.in

CLEANFILES =

EOLIAN_FLAGS = @DEPS_EOLIAN_FLAGS@ \
               -I$(top_srcdir)/src/lib

include $(top_srcdir)/Makefile_Eolian_Helper.am
include $(top_srcdir)/Makefile_Eolian_Cxx_Helper.am

AM_CPPFLAGS = -DPACKAGE_DATA_DIR=\"$(pkgdatadir)\" \
              -DPACKAGE_BIN_DIR=\"$(bindir)\" \
              -DPACKAGE_LIB_DIR=\"$(libdir)\" \
              -I$(top_srcdir)/src/bin \
              -I$(top_srcdir)/src/include \
              @ENVENTOR_CFLAGS@

lib_LTLIBRARIES = libenventor.la

includes_HEADERS = \
   Enventor_Legacy.h \
   Enventor.h \
   Enventor_Eo.h
includesdir = $(includedir)/enventor-@ENVENTOR_VERSION_MAJOR@

libenventor_la_SOURCES = \
   enventor_private.h \
   auto_comp_code.h \
   template_code.h \
   enventor_main.c \
   enventor_smart.c \
   edc_parser.c \
   indent.c \
   redoundo.c \
   syntax_helper.c \
   syntax_color.c \
   ctxpopup.c \
   edc_editor.c \
   auto_comp.c \
   build.c \
   template.c \
   edj_mgr.c \
   edj_viewer.c \
   dummy_obj.c

libenventor_la_CFLAGS = @ENVENTOR_CFLAGS@
libenventor_la_LIBADD = @ENVENTOR_LIBS@

libenventor_la_LDFLAGS = $(LTLIBINTL) -version-info @version_info@

BUILT_SOURCES = \
   enventor_object.eo.c \
   enventor_object.eo.h \
   enventor_object.eo.legacy.h

enventoreolianfilesdir = $(datadir)/eolian/include/enventor-@ENVENTOR_VERSION_MAJOR@
enventoreolianfiles_DATA = enventor_object.eo
EXTRA_DIST = ${enventoreolianfiles_DATA}
