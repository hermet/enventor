dnl use: EFL_ENABLE_BETA_API_SUPPORT
AC_DEFUN([EFL_ENABLE_BETA_API_SUPPORT],
[
  AC_DEFINE([EFL_BETA_API_SUPPORT], [1], [Enable access to unstable EFL API that are still in beta])
])

dnl use: ENVENTOR_ENABLE_BETA_API_SUPPORT
AC_DEFUN([ENVENTOR_ENABLE_BETA_API_SUPPORT],
[
  AC_DEFINE([ENVENTOR_BETA_API_SUPPORT], [1], [Enable access to unstable ENVENTOR API that are still in beta])
])
