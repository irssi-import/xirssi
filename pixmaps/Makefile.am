SUFFIXES = .png .h

.png.h:
	gdk-pixbuf-csource --raw --name=`echo $*|sed 's/-/_/'` $< > $*.h

xpm_pixmaps = \
	arrow-up.xpm \
	arrow-down.xpm \
	arrow-left.xpm \
	arrow-right.xpm \
	arrow-up-twist.xpm \
	arrow-down-twist.xpm \
	arrow-left-twist.xpm \
	arrow-right-twist.xpm \
	detach-box.xpm \
	lock.xpm \
	move.xpm \
	right.xpm

generated_pixmaps = \
	ball-blue.h \
	ball-lightblue.h \
	ball-pink.h \
	ball-yellow.h \
	ball-green.h \
	ball-orange.h \
	ball-red.h

# .. can't I combine these lists into just one..?
EXTRA_DIST = \
	$(xpm_pixmaps) \
	ball-blue.png \
	ball-lightblue.png \
	ball-pink.png \
	ball-yellow.png \
	ball-green.png \
	ball-orange.png \
	ball-red.png

all: $(generated_pixmaps)

clean-generic:
	rm -f $(generated_pixmaps)
