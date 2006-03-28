/*
 *  Copyright (c) 2006 Stephan Arts <stephan.arts@hva.nl>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 */

#include <stdlib.h>
#include <gtk/gtk.h>
#include "main-window.h"

static void 
xarchiver_main_window_class_init (XArchiverMainWindowClass *class);

static void 
xarchiver_main_window_init       (XArchiverMainWindow *window);

static void
xarchiver_main_window_destroy    (GtkObject *object);

static void
xarchiver_main_window_realize    (GtkWidget *widget);

static GtkWidgetClass *xarchiver_main_window_parent_class = NULL;

GtkType
xarchiver_main_window_get_type()
{
	static GtkType xarchiver_main_window_type = 0;
	
	if(xarchiver_main_window_type == 0)
	{
		static const GtkTypeInfo main_window_info =
		{
			"xarchiver_main_window",
			sizeof(XArchiverMainWindow),
			sizeof(XArchiverMainWindowClass),
			(GtkClassInitFunc) xarchiver_main_window_class_init,
			(GtkObjectInitFunc) xarchiver_main_window_init,
			NULL,
			NULL,
			(GtkClassInitFunc) NULL,
		};
		xarchiver_main_window_type = gtk_type_unique(GTK_TYPE_WINDOW, &main_window_info);
	}
	return xarchiver_main_window_type;
}

static void 
xarchiver_main_window_class_init (XArchiverMainWindowClass *class)
{
	GtkObjectClass *object_class;
	GtkWidgetClass *widget_class;

	object_class = (GtkObjectClass *)class;
	widget_class = (GtkWidgetClass *)class;

	xarchiver_main_window_parent_class = gtk_type_class(gtk_widget_get_type());

	object_class->destroy = xarchiver_main_window_destroy;

	widget_class->realize = xarchiver_main_window_realize;
}

static void 
xarchiver_main_window_init       (XArchiverMainWindow *window)
{

}

GtkWidget *
xarchiver_main_window_new()
{
	XArchiverMainWindow *window;
	
	window = gtk_type_new(xarchiver_main_window_get_type());

	return GTK_WIDGET(window);
}

static void
xarchiver_main_window_destroy    (GtkObject *object)
{
	XArchiverMainWindow *window;

	g_return_if_fail(object != NULL);
	g_return_if_fail(XARCHIVER_IS_MAIN_WINDOW(object));

	window = XARCHIVER_MAIN_WINDOW(object);
	
	if(GTK_OBJECT_CLASS(xarchiver_main_window_parent_class)->destroy)
		(* GTK_OBJECT_CLASS(xarchiver_main_window_parent_class)->destroy) (object);
}

static void
xarchiver_main_window_realize    (GtkWidget *widget)
{
	XArchiverMainWindow *window;
	
	g_return_if_fail(widget != NULL);
	g_return_if_fail(XARCHIVER_IS_MAIN_WINDOW(widget));

	window = XARCHIVER_MAIN_WINDOW(widget);

	GTK_WIDGET_SET_FLAGS(widget, GTK_REALIZED);
}

