aclocal $ACLOCAL_FLAGS
autoheader
autoconf
automake --add-missing

cd pixmaps
./generate.sh
cd ..
