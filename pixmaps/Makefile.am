SUFFIXES = .png .h

.png.h:
	gdk-pixbuf-csource --raw --name=`echo $*|sed 's/-/_/'` $< > $*.h

pixmaps = \
	ball-blue.h \
	ball-lightblue.h \
	ball-pink.h \
	ball-yellow.h \
	ball-green.h \
	ball-orange.h \
	ball-red.h

# .. can't I combine these lists into just one..?
EXTRA_DIST = \
	ball-blue.png \
	ball-lightblue.png \
	ball-pink.png \
	ball-yellow.png \
	ball-green.png \
	ball-orange.png \
	ball-red.png

all: $(pixmaps)

clean-generic:
	rm -f $(pixmaps)
