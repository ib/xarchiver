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
#include <config.h>
#include "main-window.h"

static void
xa_main_window_class_init (XAMainWindowClass *_class);

static void
xa_main_window_init (XAMainWindow *window);

static GtkWidgetClass *xa_main_window_parent_class;


GType
xa_main_window_get_type()
{
	static GType xa_main_window_type = 0;

	if(xa_main_window_type == 0)
	{
		static const GTypeInfo xa_main_window_info = 
		{
			sizeof(XAMainWindowClass),
			(GBaseInitFunc) NULL,
			(GBaseFinalizeFunc) NULL,
			(GClassInitFunc) xa_main_window_class_init,
			(GClassFinalizeFunc) NULL,
			NULL, sizeof(XAMainWindow),
			0,
			(GInstanceInitFunc) xa_main_window_init,
			NULL,
		};
		xa_main_window_type = g_type_register_static(GTK_TYPE_WINDOW, "XAMainWindow", &xa_main_window_info, 0);
	}
	return xa_main_window_type;
}


GtkWidget *
xa_main_window_new()
{
	XAMainWindow *window;

	window = g_object_new(xa_main_window_get_type(), NULL);

	return GTK_WIDGET(window);
}

static void
xa_main_window_destroy (GtkObject *object)
{
	XAMainWindow *window;

	g_return_if_fail (object != NULL);
	g_return_if_fail (IS_XA_MAIN_WINDOW(object));

	window = XA_MAIN_WINDOW(object);

	if (GTK_OBJECT_CLASS (xa_main_window_parent_class)->destroy)
		(* GTK_OBJECT_CLASS (xa_main_window_parent_class)->destroy) (object);
}

static void
xa_main_window_show_all(GtkWidget *widget)
{
	gtk_widget_show(XA_MAIN_WINDOW(widget)->vbox);
	gtk_widget_show(XA_MAIN_WINDOW(widget)->menubar);
	gtk_widget_show(XA_MAIN_WINDOW(widget)->toolbar);
	
	gtk_widget_show(widget);
}

static void
xa_main_window_class_init (XAMainWindowClass *_class)
{
	GtkObjectClass *object_class = (GtkObjectClass *) _class;
	GtkWidgetClass *widget_class = (GtkWidgetClass *) _class;

	xa_main_window_parent_class = gtk_type_class(gtk_window_get_type());

	object_class->destroy = xa_main_window_destroy;

	widget_class->show_all = xa_main_window_show_all;
}

static void
xa_main_window_init (XAMainWindow *window)
{
	gtk_window_set_title(GTK_WINDOW(window), PACKAGE_STRING);

	window->vbox = gtk_vbox_new(FALSE, 0);
	window->menubar = gtk_menu_bar_new();
	window->toolbar = gtk_toolbar_new();
	window->notebook = gtk_notebook_new();

	gtk_container_add(GTK_CONTAINER(window), window->vbox);

	gtk_box_pack_start(GTK_BOX(window->vbox), window->menubar, FALSE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(window->vbox), window->toolbar, FALSE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(window->vbox), window->notebook, TRUE, TRUE, 0);

}

