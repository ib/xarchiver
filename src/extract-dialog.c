/*
 *  Copyright (c) 2006 Stephan Arts <psybsd@gmail.com>
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
 */

#include <config.h>
#include <glib.h>
#include <glib-object.h>
#include <gtk/gtk.h>
#include <libintl.h>

#include "extract-dialog.h"

#define _(String) gettext(String)

/* TODO:
 * Fix change of extraction-type.
 */

static void
xa_extract_dialog_init(XAExtractDialog *object);

static void
xa_extract_dialog_destroy(GtkObject *object);

static void
xa_extract_dialog_class_init(XAExtractDialogClass *klass);

static GtkWidgetClass *xa_extract_dialog_parent_class = 0;

GType
xa_extract_dialog_get_type ()
{
	static GtkType xa_extract_dialog_type = 0;

 	if (!xa_extract_dialog_type)
	{
 		static const GTypeInfo xa_extract_dialog_info = 
		{
			sizeof (XAExtractDialogClass),
			(GBaseInitFunc) NULL,
			(GBaseFinalizeFunc) NULL,
			(GClassInitFunc) xa_extract_dialog_class_init,
			(GClassFinalizeFunc) NULL,
			NULL,
			sizeof (XAExtractDialog),
			0,
			(GInstanceInitFunc) xa_extract_dialog_init,
			NULL
		};

		xa_extract_dialog_type = g_type_register_static (GTK_TYPE_DIALOG, "XAExtractDialog", &xa_extract_dialog_info, 0);
	}
	return xa_extract_dialog_type;
}

static void
xa_extract_dialog_class_init(XAExtractDialogClass *_class)
{
	GtkObjectClass *object_class = (GtkObjectClass *)_class;

	xa_extract_dialog_parent_class = gtk_type_class(GTK_TYPE_DIALOG);

	object_class->destroy = xa_extract_dialog_destroy;

}

static void
xa_extract_dialog_init(XAExtractDialog *object)
{
	object->folder_chooser = gtk_file_chooser_button_new(_("Choose destination folder"), GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER);
	gtk_widget_show(object->folder_chooser);
	
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(object)->vbox), object->folder_chooser, FALSE, TRUE, 10);

	object->extract_all       = gtk_radio_button_new_with_mnemonic(NULL, "_All files");
	object->extract_select    = gtk_radio_button_new_with_mnemonic_from_widget(GTK_RADIO_BUTTON(object->extract_all), "_Selected files");

	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(object)->vbox), object->extract_all, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(object)->vbox), object->extract_select, FALSE, FALSE, 5);
	gtk_widget_show(object->extract_all);
	gtk_widget_show(object->extract_select);

	gtk_dialog_add_button(GTK_DIALOG(object), _("Cancel"), GTK_RESPONSE_CANCEL);
	gtk_dialog_add_button(GTK_DIALOG(object), _("Extract"), GTK_RESPONSE_OK);

	object->extract_optiongroup = gtk_radio_button_get_group(GTK_RADIO_BUTTON(object->extract_all));
}

static void
xa_extract_dialog_destroy(GtkObject *object)
{
	g_return_if_fail (object != NULL);
	g_return_if_fail (XA_IS_EXTRACT_DIALOG(object));

	if (GTK_OBJECT_CLASS (xa_extract_dialog_parent_class)->destroy)
		(* GTK_OBJECT_CLASS (xa_extract_dialog_parent_class)->destroy) (object);
}

XAExtractionType
xa_extract_dialog_get_extraction_type(XAExtractDialog *dialog)
{
	return dialog->extraction_type;
}

gchar *
xa_extract_dialog_get_destination_folder(XAExtractDialog *dialog)
{
	return gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog->folder_chooser));
}

GtkWidget *
xa_extract_dialog_new(GtkWindow *parent, XAExtractionType default_type)
{
	GtkWidget *dialog;
	dialog = GTK_WIDGET (g_object_new(xa_extract_dialog_get_type(), NULL));
	gtk_window_set_transient_for(GTK_WINDOW(dialog), parent);
	gtk_window_set_destroy_with_parent(GTK_WINDOW(dialog), TRUE);
	gtk_window_set_title(GTK_WINDOW(dialog), _("Extract to"));
	gtk_widget_set_size_request(dialog, 564, 300);
	gtk_window_set_resizable(GTK_WINDOW(dialog), FALSE);

	XA_EXTRACT_DIALOG(dialog)->extraction_type = default_type;
	switch(default_type)
	{
		case XA_EXTRACTION_ALL:
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(XA_EXTRACT_DIALOG(dialog)->extract_all), TRUE);
			gtk_widget_set_sensitive(XA_EXTRACT_DIALOG(dialog)->extract_select, FALSE);
			break;
		case XA_EXTRACTION_SELECT:
			gtk_widget_set_sensitive(XA_EXTRACT_DIALOG(dialog)->extract_select, TRUE);
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(XA_EXTRACT_DIALOG(dialog)->extract_select), TRUE);
			break;
	}
	return dialog;
}
