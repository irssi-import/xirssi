/*
 irssi.c : irssi

    Copyright (C) 2002 Timo Sirainen

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include "module.h"
//#include "module-formats.h"
#include "modules-load.h"
#include "args.h"
#include "signals.h"
#include "levels.h"
#include "core.h"
#include "settings.h"
#include "session.h"

#include "printtext.h"
#include "fe-common-core.h"
//#include "themes.h"

#include "gui-channel.h"
#include "gui-keyboard.h"
#include "gui-nicklist.h"
#include "gui-window.h"

#include <signal.h>

#ifdef HAVE_STATIC_PERL
void perl_core_init(void);
void perl_core_deinit(void);

void fe_perl_init(void);
void fe_perl_deinit(void);
#endif

void irc_init(void);
void irc_deinit(void);

void fe_common_irc_init(void);
void fe_common_irc_deinit(void);

void gui_context_nick_init(void);
void gui_context_nick_deinit(void);

void gui_context_url_init(void);
void gui_context_url_deinit(void);

static void sig_exit(void)
{
	gtk_main_quit();
}

static void gui_init(void)
{
	irssi_gui = IRSSI_GUI_GTK;
	core_init();
	irc_init();
	fe_common_core_init();
	fe_common_irc_init();

	//theme_register(gui_text_formats);
	signal_add_last("gui exit", (SIGNAL_FUNC) sig_exit);
}

static void gui_finish_init(void)
{
	settings_check();
	module_register("core", "fe-text");

#ifdef HAVE_STATIC_PERL
	perl_core_init();
	fe_perl_init();
#endif

	gui_windows_init();
	gui_keyboards_init();
	gui_nicklists_init();
	gui_channels_init();
        gui_context_nick_init();
        gui_context_url_init();

	fe_common_core_finish_init();

	g_log_set_handler(G_LOG_DOMAIN,
			  (GLogLevelFlags) (G_LOG_LEVEL_CRITICAL |
					    G_LOG_LEVEL_WARNING),
			  (GLogFunc) g_log_default_handler, NULL);

	signal_emit("irssi init finished", 0);
}

static void gui_deinit(void)
{
	signal(SIGINT, SIG_DFL);

	while (modules != NULL)
		module_unload(modules->data);

#ifdef HAVE_STATIC_PERL
        perl_core_deinit();
        fe_perl_deinit();
#endif

	signal_remove("gui exit", (SIGNAL_FUNC) sig_exit);

        gui_context_url_deinit();
        gui_context_nick_deinit();
	gui_channels_deinit();
	gui_nicklists_deinit();
	gui_keyboards_deinit();
	gui_windows_deinit();

	//theme_unregister();

	fe_common_irc_deinit();
	fe_common_core_deinit();
	irc_deinit();
	core_deinit();
}

int main(int argc, char **argv)
{
	gtk_init(&argc, &argv);

	core_init_paths(argc, argv);

#ifdef HAVE_SOCKS
	SOCKSinit(argv[0]);
#endif
#ifdef ENABLE_NLS
	/* initialize the i18n stuff */
	bindtextdomain(PACKAGE, LOCALEDIR);
	textdomain(PACKAGE);
#endif

	gui_init();
	args_execute(argc, argv);

	gui_finish_init();
	gtk_main();
	gui_deinit();

        session_upgrade(); /* if we /UPGRADEd, start the new process */
	return 0;
}
