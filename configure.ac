##--##--##--##--##--##--##--##--##--##--##--##--##--##--##--##--##
##--##--##--##--##--##--##--##--##--##--##--##--##--##--##--##--##
m4_define([v_maj], [0])
m4_define([v_min], [8])
m4_define([v_mic], [99])
#m4_define([v_rev], m4_esyscmd([(git rev-list --count HEAD 2>/dev/null || echo 0) | tr -d '\n']))dnl
##--   When released, remove the dnl on the below line
dnl m4_undefine([v_rev])
##--   When doing snapshots - change soname. remove dnl on below line
dnl m4_define([relname], [ver-pre-svn-07])
dnl m4_define([v_rel], [-release relname])
##--##--##--##--##--##--##--##--##--##--##--##--##--##--##--##--##
m4_ifdef([v_rev], [m4_define([v_ver], [v_maj.v_min.v_mic.v_rev])], [m4_define([v_ver], [v_maj.v_min.v_mic])])
m4_define([lt_cur], m4_eval(v_maj + v_min))
m4_define([lt_rev], v_mic)
m4_define([lt_age], v_min)
##--##--##--##--##--##--##--##--##--##--##--##--##--##--##--##--##
##--##--##--##--##--##--##--##--##--##--##--##--##--##--##--##--##

AC_INIT(enventor, [v_ver], hermet@hermet.pe.kr)
AC_PREREQ(2.52)
AC_CONFIG_SRCDIR(configure.ac)
AC_CONFIG_MACRO_DIR([m4])
AC_PROG_CC
AC_CANONICAL_BUILD
AC_CANONICAL_HOST
AC_CANONICAL_TARGET

AM_INIT_AUTOMAKE(1.6 dist-bzip2 dist-xz)
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])
AC_CONFIG_HEADERS(config.h)
AC_GNU_SOURCE
AC_PROG_LIBTOOL
AM_PROG_CC_C_O
ELM_QUICKLAUNCH

AM_PROG_AS

##-- localization with using gettext
AM_GNU_GETTEXT_VERSION(0.18.1)
AM_GNU_GETTEXT([external])

#===============================================================================
if test "x${prefix}" = "xNONE"; then
   LOCALE_DIR="${ac_default_prefix}/share/locale"
else
   LOCALE_DIR="${prefix}/share/locale"
fi
AC_SUBST(LOCALE_DIR)
#===============================================================================

##--##--##--##--##--##--##--##--##--##--##--##--##--##--##--##--##
##--##--##--##--##--##--##--##--##--##--##--##--##--##--##--##--##
m4_ifdef([v_rev], , [m4_define([v_rev], [0])])
m4_ifdef([v_rel], , [m4_define([v_rel], [])])
AC_DEFINE_UNQUOTED(VMAJ, [v_maj], [Major version])
AC_DEFINE_UNQUOTED(VMIN, [v_min], [Minor version])
AC_DEFINE_UNQUOTED(VMIC, [v_mic], [Micro version])
AC_DEFINE_UNQUOTED(VREV, [v_rev], [Revison])

version_info="lt_cur:lt_rev:lt_age"
release_info="v_rel"
AC_SUBST(version_info)
AC_SUBST(release_info)
##--##--##--##--##--##--##--##--##--##--##--##--##--##--##--##--##
##--##--##--##--##--##--##--##--##--##--##--##--##--##--##--##--##
ENVENTOR_VERSION_MAJOR=v_maj
ENVENTOR_VERSION_MINOR=v_min
AC_SUBST(ENVENTOR_VERSION_MAJOR)
AC_SUBST(ENVENTOR_VERSION_MINOR)

EFL_ENABLE_EO_API_SUPPORT
EFL_ENABLE_BETA_API_SUPPORT
AC_DEFINE_UNQUOTED(ENVENTOR_WIN32_BUILD_SUPPORT, 1, "Support Win32 dllexport")
AC_DEFINE_UNQUOTED(ENVENTOR_BETA_API_SUPPORT, 1, "Enable access to unstable ENVENTOR API that are still in beta")

efl_version="1.16.0"
requirement_enventor=" \
eo >= ${efl_version} \
eina >= ${efl_version} \
eet >= ${efl_version} \
elementary >= ${efl_version} \
evas >= ${efl_version} \
ecore >= ${efl_version} \
ecore-file >= ${efl_version} \
edje >= ${efl_version} \
eio >= ${efl_version} \
efreet >= ${efl_version}"

AC_SUBST(requirement_enventor)

PKG_CHECK_MODULES([ENVENTOR], [${requirement_enventor}])

### Checks for linker characteristics

# Needs to be moved into a macro, and also, needs a way to automatically fetch from all the dependencies using the Requires.
DEPS_EOLIAN_FLAGS=`${PKG_CONFIG} --variable=eolian_flags eo evas edje ecore elementary efl`
AC_SUBST([DEPS_EOLIAN_FLAGS])

EFL_ATTRIBUTE_UNUSED

# Check edje_cc
EFL_WITH_BIN([eet], [eet-eet], [eet])
EFL_WITH_BIN([edje], [edje-cc], [edje_cc])
EFL_WITH_BIN([eolian], [eolian-gen], [eolian_gen])
EFL_WITH_BIN([eolian-cxx], [eolian-cxx], [eolian_cxx])

# Force the helper to try external eolian generator
AM_CONDITIONAL([HAVE_EOLIAN_GEN], [true])
AM_CONDITIONAL([HAVE_EOLIAN_CXX], [true])

AC_CONFIG_FILES([
po/Makefile.in
Makefile
src/Makefile
src/include/Makefile
src/lib/Makefile
src/lib/Enventor.h
src/bin/Makefile
data/Makefile
data/desktop/Makefile
data/icon/Makefile
data/images/Makefile
data/themes/Makefile
data/themes/default/Makefile
data/themes/default/images/Makefile
data/themes/default/sounds/Makefile
data/templates/Makefile
data/color/Makefile
data/autocomp/Makefile
data/sounds/Makefile
data/help/Makefile
pc/enventor.pc
])

AC_OUTPUT
