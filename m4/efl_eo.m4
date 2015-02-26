dnl use: EFL_ENABLE_EO_API_SUPPORT
AC_DEFUN([EFL_ENABLE_EO_API_SUPPORT],
[
  AC_DEFINE([EFL_EO_API_SUPPORT], [1], [Enable access to unstable EFL Eo API])
])

AC_DEFUN([EFL_ENABLE_EO_LATEST],
[
  AC_DEFINE([EO_LATEST], [1], [Enable access to unstable EFL Eo Latest API])
])


