AC_INIT(src)

AM_CONFIG_HEADER(config-plugin.h)
AM_INIT_AUTOMAKE(xirssi, 0.1)

AM_MAINTAINER_MODE

AC_ISC_POSIX
AC_PROG_CC
AC_PROG_CPP
AC_STDC_HEADERS
AC_ARG_PROGRAM
AM_DISABLE_STATIC
AM_PROG_LIBTOOL

AC_ARG_WITH(irssi,
[  --with-irssi            Irssi directory],
	IRSSI_INCLUDE=$withval,
	IRSSI_INCLUDE="")

AC_ARG_WITH(irssi-lib,
[  --with-irssi-lib        Irssi library directory],
	IRSSI_LIB=$withval,
	IRSSI_LIB="")

if test x$IRSSI_INCLUDE = x; then
  dnl * --with-irssi not given, try to find irssi
  if test -f ../irssi-config; then
    IRSSI_INCLUDE=".."
  elif test -f ../irssi/irssi-config; then
    IRSSI_INCLUDE="../irssi"
  elif test -f ../../irssi/irssi-config; then
    IRSSI_INCLUDE="../../irssi"
  else
    AC_ERROR(Irssi directory not found, use --with-irssi=/path/to/irssi)
  fi
fi

if test x$IRSSI_LIB = x; then
  IRSSI_LIB=$IRSSI_INCLUDE
fi

# fix relative paths
old=`pwd`
IRSSI_INCLUDE=`eval cd $IRSSI_INCLUDE; pwd`
cd $old

AC_SUBST(IRSSI_INCLUDE)

if test ! -f $IRSSI_LIB/irssi-config; then
  AC_ERROR(Not irssi directory: $IRSSI_LIB)
fi
eval `cat $IRSSI_LIB/irssi-config|sed 's,\.\./,$IRSSI_LIB/src/,g'`

if test "x`echo $PROG_LIBS|grep glib-2.0`" = "x"; then
  AC_ERROR(Irssi needs to be configured with --with-glib2 option)
fi

if test "x$prefix" == "xNONE"; then
  prefix="${ac_default_prefix}"
fi

pkgdatadir="$prefix/share/xirssi/"
AC_DEFINE_UNQUOTED([XIRSSI_PKGDATADIR], ["$pkgdatadir"], [Data directory for xirssi])

AC_SUBST(COMMON_LIBS)
AC_SUBST(PROG_LIBS)
AC_SUBST(PERL_LINK_LIBS)
AC_SUBST(PERL_FE_LINK_LIBS)
AC_SUBST(PERL_LINK_FLAGS)

AM_PATH_GTK_2_0(2.0.0,,, gmodule)

# gcc specific options
if test "x$ac_cv_prog_gcc" = "xyes"; then
  CFLAGS="$CFLAGS -Wall"
fi

AC_OUTPUT(
Makefile
src/Makefile
src/glade/Makefile
pixmaps/Makefile
stamp.h)
