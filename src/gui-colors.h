#ifndef __GUI_COLORS_H
#define __GUI_COLORS_H

#define DEFAULT_COLORS 16
#define MIRC_COLORS 16

enum {
	COLOR_BOLD = DEFAULT_COLORS,

	COLORS
};

extern GdkColor colors[COLORS];
extern GdkColor mirc_colors[MIRC_COLORS];

#endif
