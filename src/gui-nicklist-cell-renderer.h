/* Copyright (C) 2000  Red Hat, Inc.,  Jonathan Blandford <jrb@redhat.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */
#ifndef __GTK_CELL_RENDERER_NICKLIST_H__
#define __GTK_CELL_RENDERER_NICKLIST_H__

#include <gtk/gtkcellrenderertext.h>
#include <gtk/gtkcellrendererpixbuf.h>

typedef struct _GtkCellRendererNicklistText GtkCellRendererNicklistText;
typedef struct _GtkCellRendererNicklistTextClass GtkCellRendererNicklistTextClass;

struct _GtkCellRendererNicklistText
{
	GtkCellRendererText parent;
};

struct _GtkCellRendererNicklistTextClass
{
	GtkCellRendererTextClass parent_class;

	void (*parent_render) (GtkCellRenderer      *cell,
			       GdkWindow            *window,
			       GtkWidget            *widget,
			       GdkRectangle         *background_area,
			       GdkRectangle         *cell_area,
			       GdkRectangle         *expose_area,
			       GtkCellRendererState  flags);
};

GtkCellRenderer *gtk_cell_renderer_nicklist_text_new(void);

/* pixmap version */

typedef struct _GtkCellRendererNicklistPixbuf GtkCellRendererNicklistPixbuf;
typedef struct _GtkCellRendererNicklistPixbufClass GtkCellRendererNicklistPixbufClass;

struct _GtkCellRendererNicklistPixbuf
{
	GtkCellRendererPixbuf parent;
};

struct _GtkCellRendererNicklistPixbufClass
{
	GtkCellRendererPixbufClass parent_class;

	void (*parent_render) (GtkCellRenderer      *cell,
			       GdkWindow            *window,
			       GtkWidget            *widget,
			       GdkRectangle         *background_area,
			       GdkRectangle         *cell_area,
			       GdkRectangle         *expose_area,
			       GtkCellRendererState  flags);
};

GtkCellRenderer *gtk_cell_renderer_nicklist_pixmap_new(void);

#endif
