/* this is here only so that the first row would be displayed with
   different color - this code is a REALLY horrible kludge. GTK+ 2.2
   luckily supports better way to do this. */

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

#include <stdlib.h>
#include <gtk/gtkscrolledwindow.h>
#include "gui-nicklist-cell-renderer.h"

static void (*text_render) (GtkCellRenderer      *cell,
			    GdkWindow            *window,
			    GtkWidget            *widget,
			    GdkRectangle         *background_area,
			    GdkRectangle         *cell_area,
			    GdkRectangle         *expose_area,
			    GtkCellRendererState  flags);

static void gtk_cell_renderer_nicklist_text_class_init (GtkCellRendererNicklistTextClass *class);
static void gtk_cell_renderer_nicklist_pixbuf_class_init (GtkCellRendererNicklistPixbufClass *class);
static void gtk_cell_renderer_nicklist_render     (GtkCellRenderer            *cell,
						   GdkWindow                  *window,
						   GtkWidget                  *widget,
						   GdkRectangle               *background_area,
						   GdkRectangle               *cell_area,
						   GdkRectangle               *expose_area,
						   guint                       flags);

GtkType
gtk_cell_renderer_nicklist_text_get_type (void)
{
  static GtkType cell_nicklist_type =  0;

  if (!cell_nicklist_type)
    {
      static const GTypeInfo cell_nicklist_info =
      {
	sizeof (GtkCellRendererNicklistTextClass),
	NULL,		/* base_init */
	NULL,		/* base_finalize */
	(GClassInitFunc) gtk_cell_renderer_nicklist_text_class_init,
	NULL,		/* class_finalize */
	NULL,		/* class_data */
	sizeof (GtkCellRendererNicklistText),
	0,              /* n_preallocs */
	NULL,
      };

      cell_nicklist_type = g_type_register_static (GTK_TYPE_CELL_RENDERER_TEXT, "GtkCellRendererNicklistText", &cell_nicklist_info, 0);
    }

  return cell_nicklist_type;
}

GtkType
gtk_cell_renderer_nicklist_pixmap_get_type (void)
{
  static GtkType cell_nicklist_type =  0;

  if (!cell_nicklist_type)
    {
      static const GTypeInfo cell_nicklist_info =
      {
	sizeof (GtkCellRendererNicklistPixbufClass),
	NULL,		/* base_init */
	NULL,		/* base_finalize */
	(GClassInitFunc) gtk_cell_renderer_nicklist_pixbuf_class_init,
	NULL,		/* class_finalize */
	NULL,		/* class_data */
	sizeof (GtkCellRendererNicklistPixbuf),
	0,              /* n_preallocs */
	NULL,
      };

      cell_nicklist_type = g_type_register_static (GTK_TYPE_CELL_RENDERER_PIXBUF, "GtkCellRendererNicklistPixbuf", &cell_nicklist_info, 0);
    }

  return cell_nicklist_type;
}

static void
gtk_cell_renderer_nicklist_text_class_init (GtkCellRendererNicklistTextClass *class)
{
  GtkCellRendererClass *cell_class = GTK_CELL_RENDERER_CLASS (class);

  text_render = class->parent_render = cell_class->render;
  cell_class->render = gtk_cell_renderer_nicklist_render;
}

static void
gtk_cell_renderer_nicklist_pixbuf_class_init (GtkCellRendererNicklistPixbufClass *class)
{
  GtkCellRendererClass *cell_class = GTK_CELL_RENDERER_CLASS (class);

  class->parent_render = cell_class->render;
  cell_class->render = gtk_cell_renderer_nicklist_render;
}

GtkCellRenderer *
gtk_cell_renderer_nicklist_text_new (void)
{
  return GTK_CELL_RENDERER (gtk_type_new (gtk_cell_renderer_nicklist_text_get_type ()));
}

GtkCellRenderer *
gtk_cell_renderer_nicklist_pixmap_new (void)
{
  return GTK_CELL_RENDERER (gtk_type_new (gtk_cell_renderer_nicklist_pixmap_get_type ()));
}

static void
gtk_cell_renderer_nicklist_render (GtkCellRenderer *cell,
				   GdkWindow       *window,
				   GtkWidget       *widget,
				   GdkRectangle    *background_area,
				   GdkRectangle    *cell_area,
				   GdkRectangle    *expose_area,
				   guint            flags)
{
	GtkAdjustment *adj;
	int first;

	adj = gtk_scrolled_window_get_vadjustment(GTK_SCROLLED_WINDOW(widget->parent));
	first = adj->value+cell_area->y <= cell_area->height;

	if (GTK_CHECK_TYPE(cell, gtk_cell_renderer_nicklist_text_get_type())) {
                GtkCellRendererNicklistTextClass *c =
			GTK_CHECK_GET_CLASS(cell, gtk_cell_renderer_nicklist_text_get_type(),
					    GtkCellRendererNicklistTextClass);

		if (first) {
			g_object_set(G_OBJECT(cell), "background-gdk",
				     &widget->style->bg[GTK_STATE_NORMAL], NULL);
		}
		c->parent_render(cell, window, widget, background_area,
				 cell_area, expose_area, flags);
		if (first)
			g_object_set(G_OBJECT(cell), "background", NULL, NULL);
	} else if (first) {
		/* first row - pixbuf column */
		GdkGC *gc;

		gc = gdk_gc_new (window);
		gdk_gc_set_rgb_fg_color (gc, &widget->style->bg[GTK_STATE_NORMAL]);
		gdk_draw_rectangle (window,
				    gc,
				    TRUE,
				    background_area->x,
				    background_area->y,
				    background_area->width,
				    background_area->height);

		g_object_unref (G_OBJECT (gc));
	} else {
		GtkCellRendererNicklistPixbufClass *c =
			GTK_CHECK_GET_CLASS(cell, gtk_cell_renderer_nicklist_pixbuf_get_type(),
					    GtkCellRendererNicklistPixbufClass);
		c->parent_render(cell, window, widget, background_area,
				 cell_area, expose_area, flags);
	}
}
