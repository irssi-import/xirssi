#!/bin/sh

# did we have gtk-2.0.m4?
rm -f gtk-2.0.m4
error=`aclocal $ACLOCAL_FLAGS 2>&1`
if test "x`echo $error|grep AM_PATH_GTK`" != "x"; then
  cp gtk-2.0.m4_ gtk-2.0.m4
  aclocal $ACLOCAL_FLAGS -I .
fi

libtoolize --force --copy
autoheader
autoconf
automake --add-missing
