/*
 gui-colors.c : irssi

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
#include "gui-colors.h"

GdkColor colors[COLORS] = {
	{ 0, 0, 0, 0 }, /* black */
	{ 0, 0, 0, 0xcccc }, /* blue */
	{ 0, 0, 0xcccc, 0 }, /* green */
	{ 0, 0, 0xbbbb, 0xbbbb }, /* cyan */
	{ 0, 0xcccc, 0, 0 }, /* red */
	{ 0, 0xbbbb, 0, 0xbbbb }, /* magenta */
	{ 0, 0xbbbb, 0xbbbb, 0 }, /* brown */
	{ 0, 0xaaaa, 0xaaaa, 0xaaaa }, /* grey */
	{ 0, 0x7777, 0x7777, 0x7777 }, /* dark grey */
	{ 0, 0, 0, 0xffff }, /* light blue */
	{ 0, 0, 0xffff, 0 }, /* light green */
	{ 0, 0, 0xffff, 0xffff }, /* light cyan */
	{ 0, 0xffff, 0, 0 }, /* light red */
	{ 0, 0xffff, 0, 0xffff }, /* light magenta */
	{ 0, 0xffff, 0xffff, 0 }, /* yellow */
	{ 0, 0xffff, 0xffff, 0xffff }, /* white */

	{ 0, 0xffff, 0xffff, 0xffff } /* bold */
};


GdkColor mirc_colors[MIRC_COLORS] = {
	{ 0, 0xffff, 0xffff, 0xffff }, /* white */
	{ 0, 0, 0, 0 }, /* black */
	{ 0, 0, 0, 0x7b00 }, /* blue */
	{ 0, 0, 0x9100, 0 }, /* green */
	{ 0, 0xffff, 0, 0 }, /* light red */
	{ 0, 0x7b00, 0, 0 }, /* red */
	{ 0, 0x9c00, 0, 0x9c00 }, /* magenta */
	{ 0, 0xffff, 0x7d00, 0 }, /* orange */
	{ 0, 0xffff, 0xffff, 0 }, /* yellow */
	{ 0, 0, 0xfa00, 0 }, /* light green */
	{ 0, 0, 0x9100, 0x8b00 }, /* cyan */
	{ 0, 0, 0xffff, 0xffff }, /* light cyan */
	{ 0, 0, 0, 0xffff }, /* light blue */
	{ 0, 0xffff, 0, 0xffff }, /* light magenta */
	{ 0, 0x7b00, 0x7b00, 0x7b00 }, /* grey */
	{ 0, 0xcd00, 0xd200, 0xcd00 } /* dark grey */
};
