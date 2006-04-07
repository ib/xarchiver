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
#include <string.h>
#include <config.h>
#include <libintl.h>
#include <libxarchiver/libxarchiver.h>
#include "property-dialog.h"
#define _(String) gettext(String)

static void
xa_property_dialog_class_init (XAPropertyDialogClass *_class);

static void
xa_property_dialog_init (XAPropertyDialog *dialog);

static GtkWidgetClass *xa_property_dialog_parent_class;

GType
xa_property_dialog_get_type()
{
	static GType xa_property_dialog_type = 0;

	if(xa_property_dialog_type == 0)
	{
		static const GTypeInfo xa_property_dialog_info = 
		{
			sizeof(XAPropertyDialogClass),
			(GBaseInitFunc) NULL,
			(GBaseFinalizeFunc) NULL,
			(GClassInitFunc) xa_property_dialog_class_init,
			(GClassFinalizeFunc) NULL,
			NULL, 
			sizeof(XAPropertyDialog),
			0,
			(GInstanceInitFunc) xa_property_dialog_init, 
			NULL
		};
		xa_property_dialog_type = g_type_register_static(GTK_TYPE_DIALOG, "XAPropertyDialog", &xa_property_dialog_info, 0);
	}
	return xa_property_dialog_type;
}


GtkWidget *
xa_property_dialog_new(GtkWindow *parent)
{
	XAPropertyDialog *dialog;

	dialog = g_object_new(xa_property_dialog_get_type(), NULL);
	gtk_window_set_transient_for(GTK_WINDOW(dialog), parent);
	gtk_window_set_title(GTK_WINDOW(dialog), _("Archive Properties"));
	return GTK_WIDGET(dialog);
}

static void
xa_property_dialog_destroy (GtkObject *object)
{
	XAPropertyDialog*window;

	g_return_if_fail (object != NULL);
	g_return_if_fail (IS_XA_PROPERTY_DIALOG(object));

	window = XA_PROPERTY_DIALOG(object);

	if (GTK_OBJECT_CLASS (xa_property_dialog_parent_class)->destroy)
		(* GTK_OBJECT_CLASS (xa_property_dialog_parent_class)->destroy) (object);
}

static void
xa_property_dialog_show_all(GtkWidget *widget)
{
	gtk_widget_show(widget);
}

static void
xa_property_dialog_class_init (XAPropertyDialogClass *_class)
{
	GtkObjectClass *object_class = (GtkObjectClass *) _class;
	GtkWidgetClass *widget_class = (GtkWidgetClass *) _class;

	xa_property_dialog_parent_class = gtk_type_class(gtk_window_get_type());

	object_class->destroy = xa_property_dialog_destroy;

	widget_class->show_all = xa_property_dialog_show_all;

}

static void
xa_property_dialog_init (XAPropertyDialog *window)
{

}
void
xa_property_dialog_add_property (gchar *name, gchar *value)
{
	g_warning("property: %s with value %s not added because it is not implemented", name, value);
}
