##--##--##--##--##--##--##--##--##--##--##--##--##--##--##--##--##
##--##--##--##--##--##--##--##--##--##--##--##--##--##--##--##--##
m4_define([v_maj], [0])
m4_define([v_min], [2])
m4_define([v_mic], [0])
m4_define([v_rev], m4_esyscmd([(git rev-list --count HEAD 2>/dev/null || echo 0) | tr -d '\n']))dnl
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

AC_INIT(enventor, [v_ver], tom@stosb.com)
AC_PREREQ(2.52)
AC_CONFIG_SRCDIR(configure.ac)
AC_CONFIG_MACRO_DIR([m4])
AC_CANONICAL_BUILD
AC_CANONICAL_HOST
AC_CANONICAL_TARGET

AM_INIT_AUTOMAKE(1.6 dist-bzip2)
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])
AC_CONFIG_HEADERS(config.h)

AC_GNU_SOURCE
AC_PROG_LIBTOOL
AM_PROG_CC_C_O

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
VMAJ=v_maj
AC_SUBST(VMAJ)

PKG_CHECK_MODULES([ENVENTOR],
  [
   eina >= 1.7.99
   eet >= 1.7.99
   elementary >= 1.7.99
   evas >= 1.7.99
   ecore >= 1.7.99
   ecore-ipc >= 1.7.99
   edje >= 1.7.99
   eo >= 1.7.99
   eio >= 1.7.99
   ]
 )

### Checks for linker characteristics
lt_enable_auto_import=""
case "${host_os}" in
   mingw*)
      lt_enable_auto_import="-Wl,--enable-auto-import"
   ;;
esac
case "$host_vendor" in
  ps3*)
      # Escape had already been checked to exist
    ;;
esac
AC_SUBST([lt_enable_auto_import])

EFL_ATTRIBUTE_UNUSED

# Check edje_cc
EFL_WITH_BIN([edje], [edje-cc], [edje_cc])

AC_CONFIG_FILES([
Makefile
src/Makefile
src/include/Makefile
src/bin/Makefile
data/Makefile
data/images/Makefile
data/themes/Makefile
data/themes/default/Makefile
data/themes/dark/Makefile
data/.proto/Makefile
])

AC_OUTPUT
