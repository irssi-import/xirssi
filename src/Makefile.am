bin_PROGRAMS = xirssi

INCLUDES = \
	-I$(top_srcdir)/pixmaps \
	-I$(IRSSI_INCLUDE) \
	-I$(IRSSI_INCLUDE)/src \
	-I$(IRSSI_INCLUDE)/src/core/ \
	-I$(IRSSI_INCLUDE)/src/fe-common/core/ \
	$(GTK_CFLAGS) \
	-DSYSCONFDIR=\""$(sysconfdir)"\"

xirssi_DEPENDENCIES = \
	@COMMON_LIBS@ \
	@PERL_LINK_LIBS@ \
	@PERL_FE_LINK_LIBS@

xirssi_LDADD = \
	@COMMON_LIBS@ \
	@PERL_LINK_LIBS@ \
	@PERL_FE_LINK_LIBS@ \
	@PERL_LINK_FLAGS@ \
        @GTK_LIBS@

xirssi_SOURCES = \
	gui-channel.c \
	gui-context-nick.c \
	gui-context-url.c \
	gui-entry.c \
	gui-frame.c \
	gui-keyboard.c \
	gui-menu.c \
	gui-menu-nick.c \
	gui-menu-url.c \
	gui-nicklist.c \
	gui-nicklist-view.c \
	gui-tab.c \
	gui-window.c \
	gui-window-activity.c \
	gui-window-context.c \
	gui-window-view.c \
        irssi.c

noinst_HEADERS = \
	gui-channel.h \
	gui-entry.h \
	gui-frame.h \
	gui-menu.h \
	gui-nicklist.h \
	gui-nicklist-view.h \
	gui-tab.h \
	gui-window.h \
	gui-window-context.h \
	gui-window-item-rec.h \
	gui-window-view.h \
	module.h
