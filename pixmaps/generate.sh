#!/bin/sh

for i in *.png; do
  echo "static const unsigned char `echo $i|sed -e 's/-/_/' -e 's/.png$//'`[] = {" > $i.h
  cat $i|perl -e 'undef $/; $data = <>; for (0 .. length($data)-1) { print ord(substr($data, $_, 1)).", "; }' | sed 's/, $//' >> $i.h
  echo "};" >> $i.h
done
