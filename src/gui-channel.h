#ifndef __GUI_CHANNEL_H
#define __GUI_CHANNEL_H

#include "channels.h"

#define CHANNEL_GUI(channel) ((ChannelGui *) MODULE_DATA(channel))

struct _ChannelGui {
#include "gui-window-item-rec.h"
	Channel *channel;
	Nicklist *nicklist;

	GSList *titles;
};

void gui_channels_init(void);
void gui_channels_deinit(void);

#endif
