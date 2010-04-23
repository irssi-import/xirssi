SUBDIRS = glade

bin_PROGRAMS = xirssi

INCLUDES = \
	-I$(top_srcdir)/pixmaps \
	-I$(IRSSI_INCLUDE) \
	-I$(IRSSI_INCLUDE)/src \
	-I$(IRSSI_INCLUDE)/src/core/ \
	-I$(IRSSI_INCLUDE)/src/fe-common/core/ \
	$(GTK_CFLAGS) \
	-DSYSCONFDIR=\""$(sysconfdir)"\" \
	-DDATADIR=\""$(pkgdatadir)"\"

xirssi_libs = \
	glade/libirssi_glade.a

xirssi_DEPENDENCIES = \
	$(xirssi_libs) \
	@COMMON_LIBS@ \
	@PERL_LINK_LIBS@ \
	@PERL_FE_LINK_LIBS@

xirssi_LDFLAGS = -export-dynamic

xirssi_LDADD = \
	$(xirssi_libs) \
	@GTK_LIBS@ \
	@COMMON_LIBS@ \
	@PERL_LINK_LIBS@ \
	@PERL_FE_LINK_LIBS@ \
	@PERL_LINK_FLAGS@ \
	@PROG_LIBS@

xirssi_SOURCES = \
	dialog-about.c \
	gui.c \
	gui-channel.c \
	gui-colors.c \
	gui-context-nick.c \
	gui-context-url.c \
	gui-entry.c \
	gui-frame.c \
	gui-itemlist.c \
	gui-keyboard.c \
	gui-menu.c \
	gui-menu-channel-topic.c \
	gui-menu-main.c \
	gui-menu-nick.c \
	gui-menu-url.c \
	gui-nicklist.c \
	gui-nicklist-view.c \
	gui-tab.c \
	gui-tab-move.c \
	gui-url.c \
	gui-window.c \
	gui-window-activity.c \
	gui-window-context.c \
	gui-window-view.c \
	gui-windowlist.c \
	setup.c \
	setup-aliases.c \
	setup-channels.c \
	setup-channels-edit.c \
	setup-channels-list.c \
	setup-completion.c \
	setup-highlighting.c \
	setup-highlighting-edit.c \
	setup-ignores.c \
	setup-ignores-edit.c \
	setup-keyboard.c \
	setup-keyboard-edit.c \
	setup-prefs.c \
	setup-servers.c \
	setup-servers-network.c \
	setup-servers-server.c \
        irssi.c

noinst_HEADERS = \
	dialogs.h \
	gui.h \
	gui-channel.h \
	gui-colors.h \
	gui-entry.h \
	gui-frame.h \
	gui-itemlist.h \
	gui-keyboard.h \
	gui-menu.h \
	gui-nicklist.h \
	gui-nicklist-view.h \
	gui-tab.h \
	gui-tab-move.h \
	gui-url.h \
	gui-window.h \
	gui-window-context.h \
	gui-window-item-rec.h \
	gui-window-view.h \
	gui-windowlist.h \
	setup.h \
	module.h
