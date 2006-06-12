/*
 *  Copyright (C) 2006 Giuseppe Torelli - <colossus73@gmail.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Street #330, Boston, MA 02111-1307, USA.
 */

#include <gtk/gtk.h>
#include "add_dialog.h"
#include "interface.h"
#include "callbacks.h"
#include "support.h"

extern gboolean cli;

Add_dialog_data *xa_create_add_dialog (XArchive *archive)
{
	Add_dialog_data *add_dialog;
	add_dialog = g_new0 (Add_dialog_data, 1);
	add_dialog->dialog1 = gtk_dialog_new ();
	gtk_window_set_title (GTK_WINDOW (add_dialog->dialog1), _("Add Dialog"));
	gtk_window_set_type_hint (GTK_WINDOW (add_dialog->dialog1), GDK_WINDOW_TYPE_HINT_DIALOG);
	gtk_window_set_transient_for ( GTK_WINDOW (add_dialog->dialog1) , GTK_WINDOW (MainWindow) );

	add_dialog->add_option_tooltip = gtk_tooltips_new ();
	add_dialog->dialog_vbox1 = GTK_DIALOG (add_dialog->dialog1)->vbox;
	gtk_widget_show (add_dialog->dialog_vbox1);

	add_dialog->vbox1 = gtk_vbox_new (FALSE, 0);
	gtk_widget_show (add_dialog->vbox1);
	gtk_box_pack_start (GTK_BOX (add_dialog->dialog_vbox1), add_dialog->vbox1, TRUE, TRUE, 0);

	add_dialog->frame1 = gtk_frame_new (NULL);
	gtk_widget_show (add_dialog->frame1);
	gtk_box_pack_start (GTK_BOX (add_dialog->vbox1), add_dialog->frame1, TRUE, TRUE, 5);
	gtk_widget_set_size_request (add_dialog->frame1, 380, -1);
	gtk_frame_set_shadow_type (GTK_FRAME (add_dialog->frame1),  GTK_SHADOW_OUT);

	add_dialog->alignment1 = gtk_alignment_new (0.5, 0.5, 1, 1);
	gtk_widget_show (add_dialog->alignment1);
	gtk_container_add (GTK_CONTAINER (add_dialog->frame1), add_dialog->alignment1);
	gtk_alignment_set_padding (GTK_ALIGNMENT (add_dialog->alignment1), 2, 5, 5, 5);

	add_dialog->scrolledwindow1 = gtk_scrolled_window_new (NULL, NULL);
	gtk_widget_show (add_dialog->scrolledwindow1);
	gtk_container_add (GTK_CONTAINER (add_dialog->alignment1), add_dialog->scrolledwindow1);
	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (add_dialog->scrolledwindow1), GTK_SHADOW_IN);
	gtk_scrolled_window_set_policy ( GTK_SCROLLED_WINDOW (add_dialog->scrolledwindow1) , GTK_POLICY_AUTOMATIC,GTK_POLICY_AUTOMATIC);

	add_dialog->file_list_treeview = gtk_tree_view_new ();
	gtk_widget_show (add_dialog->file_list_treeview);
	gtk_container_add (GTK_CONTAINER (add_dialog->scrolledwindow1), add_dialog->file_list_treeview);
	gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (add_dialog->file_list_treeview), FALSE);

	add_dialog->files_label = gtk_label_new (_("<b>Files and directories to add </b>"));
	gtk_widget_show (add_dialog->files_label);
	gtk_frame_set_label_widget (GTK_FRAME (add_dialog->frame1), add_dialog->files_label);
	gtk_label_set_use_markup (GTK_LABEL (add_dialog->files_label), TRUE);

	add_dialog->hbuttonbox1 = gtk_hbutton_box_new ();
	gtk_widget_show (add_dialog->hbuttonbox1);
	gtk_box_pack_start (GTK_BOX (add_dialog->vbox1), add_dialog->hbuttonbox1, FALSE, TRUE, 0);
	gtk_container_set_border_width (GTK_CONTAINER (add_dialog->hbuttonbox1), 5);
	gtk_button_box_set_layout (GTK_BUTTON_BOX (add_dialog->hbuttonbox1), GTK_BUTTONBOX_END);
	gtk_box_set_spacing (GTK_BOX (add_dialog->hbuttonbox1), 8);

	add_dialog->remove_files_button = gtk_button_new_from_stock ("gtk-remove");
	gtk_widget_show (add_dialog->remove_files_button);
	gtk_container_add (GTK_CONTAINER (add_dialog->hbuttonbox1), add_dialog->remove_files_button);
	GTK_WIDGET_SET_FLAGS (add_dialog->remove_files_button, GTK_CAN_DEFAULT);

	add_dialog->add_files_button = gtk_button_new_from_stock ("gtk-add");
	gtk_widget_show (add_dialog->add_files_button);
	gtk_container_add (GTK_CONTAINER (add_dialog->hbuttonbox1), add_dialog->add_files_button);
	GTK_WIDGET_SET_FLAGS (add_dialog->add_files_button, GTK_CAN_DEFAULT);

	add_dialog->frame2 = gtk_frame_new (NULL);
	gtk_widget_show (add_dialog->frame2);
	gtk_box_pack_start (GTK_BOX (add_dialog->vbox1), add_dialog->frame2, TRUE, TRUE, 0);
	gtk_frame_set_shadow_type (GTK_FRAME (add_dialog->frame2),  GTK_SHADOW_OUT);

	add_dialog->alignment2 = gtk_alignment_new (0.5, 0.5, 1, 1);
	gtk_widget_show (add_dialog->alignment2);
	gtk_container_add (GTK_CONTAINER (add_dialog->frame2), add_dialog->alignment2);
	gtk_alignment_set_padding (GTK_ALIGNMENT (add_dialog->alignment2), 0, 0, 12, 0);

	add_dialog->vbox2 = gtk_vbox_new (FALSE, 0);
	gtk_widget_show (add_dialog->vbox2);
	gtk_container_add (GTK_CONTAINER (add_dialog->alignment2), add_dialog->vbox2);

	add_dialog->checkbutton1 = gtk_check_button_new_with_mnemonic (_("checkbutton1"));
	gtk_widget_show (add_dialog->checkbutton1);
	gtk_box_pack_start (GTK_BOX (add_dialog->vbox2), add_dialog->checkbutton1, FALSE, FALSE, 0);

	add_dialog->checkbutton2 = gtk_check_button_new_with_mnemonic (_("checkbutton2"));
	gtk_widget_show (add_dialog->checkbutton2);
	gtk_box_pack_start (GTK_BOX (add_dialog->vbox2), add_dialog->checkbutton2, FALSE, FALSE, 0);

	add_dialog->checkbutton3 = gtk_check_button_new_with_mnemonic (_("checkbutton3"));
	gtk_widget_show (add_dialog->checkbutton3);
	gtk_box_pack_start (GTK_BOX (add_dialog->vbox2), add_dialog->checkbutton3, FALSE, FALSE, 0);

	add_dialog->add_option_label = gtk_label_new (_("<b>Options </b>"));
	gtk_widget_show (add_dialog->add_option_label);
	gtk_frame_set_label_widget (GTK_FRAME (add_dialog->frame2), add_dialog->add_option_label);
	gtk_label_set_use_markup (GTK_LABEL (add_dialog->add_option_label), TRUE);

	add_dialog->dialog_action_area1 = GTK_DIALOG (add_dialog->dialog1)->action_area;
	gtk_widget_show (add_dialog->dialog_action_area1);
	gtk_button_box_set_layout (GTK_BUTTON_BOX (add_dialog->dialog_action_area1), GTK_BUTTONBOX_END);

	add_dialog->cancelbutton1 = gtk_button_new_from_stock ("gtk-cancel");
	gtk_widget_show (add_dialog->cancelbutton1);
	gtk_dialog_add_action_widget (GTK_DIALOG (add_dialog->dialog1), add_dialog->cancelbutton1, GTK_RESPONSE_CANCEL);
	GTK_WIDGET_SET_FLAGS (add_dialog->cancelbutton1, GTK_CAN_DEFAULT);

	add_dialog->add_button = gtk_button_new();
	add_dialog->add_image = xa_main_window_find_image("add_button.png", GTK_ICON_SIZE_SMALL_TOOLBAR);
	add_dialog->add_hbox = gtk_hbox_new(FALSE, 4);
	add_dialog->add_label = gtk_label_new_with_mnemonic(_("_Add"));
	gtk_box_pack_start(GTK_BOX(add_dialog->add_hbox), add_dialog->add_image, FALSE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(add_dialog->add_hbox), add_dialog->add_label, FALSE, TRUE, 0);
	gtk_widget_show_all (add_dialog->add_hbox);
	gtk_container_add(GTK_CONTAINER(add_dialog->add_button), add_dialog->add_hbox);
	gtk_widget_show (add_dialog->add_button);
	gtk_dialog_add_action_widget (GTK_DIALOG (add_dialog->dialog1), add_dialog->add_button, GTK_RESPONSE_OK);
	GTK_WIDGET_SET_FLAGS (add_dialog->add_button, GTK_CAN_DEFAULT);
	return add_dialog;
}

gchar *xa_parse_add_dialog_options ( XArchive *archive , Add_dialog_data *add_dialog, GtkTreeSelection *selection)
{
	gchar *command;
	return command;
}

gchar *xa_add_single_files ( XArchive *archive , GString *files, gchar *path)
{
    gchar *command;
	return command;
}

