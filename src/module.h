#include "common.h"

#define GTK_DISABLE_DEPRECATED
#include <gtk/gtk.h>

#define MODULE_NAME "xirssi"

/* forward compatibility with 0.9 */
typedef struct _SERVER_REC Server;
typedef struct _CHANNEL_REC Channel;
typedef struct _QUERY_REC Query;
typedef struct _NICK_REC Nick;
typedef struct _WINDOW_REC Window;
typedef struct _WI_ITEM_REC WindowItem;
typedef struct _KEYBOARD_REC Keyboard;
typedef struct _TEXT_DEST_REC TextDest;

typedef struct _CHAT_PROTOCOL_REC ChatProtocol;
typedef struct _CHATNET_REC NetworkConfig;
typedef struct _SERVER_SETUP_REC ServerConfig;
typedef struct _CHANNEL_SETUP_REC ChannelConfig;

typedef struct _IGNORE_REC Ignore;

typedef struct _Entry Entry;
typedef struct _Frame Frame;
typedef struct _Itemlist Itemlist;
typedef struct _Tab Tab;
typedef struct _TabPane TabPane;
typedef struct _WindowGui WindowGui;
typedef struct _WindowView WindowView;

typedef struct _ChannelGui ChannelGui;
typedef struct _Nicklist Nicklist;
typedef struct _NicklistView NicklistView;

typedef struct {
#include "gui-window-item-rec.h"
} WindowItemGui;

void *gui_widget_find_data(GtkWidget *widget, const char *key);
