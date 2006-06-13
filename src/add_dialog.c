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
	add_dialog->file_dir_radio_group = NULL;
	add_dialog->dialog1 = gtk_dialog_new ();
	gtk_window_set_title (GTK_WINDOW (add_dialog->dialog1), _("Add Dialog"));
	gtk_window_set_type_hint (GTK_WINDOW (add_dialog->dialog1), GDK_WINDOW_TYPE_HINT_DIALOG);
	gtk_window_set_transient_for ( GTK_WINDOW (add_dialog->dialog1) , GTK_WINDOW (MainWindow) );

	add_dialog->add_option_tooltip = gtk_tooltips_new ();
	add_dialog->dialog_vbox1 = GTK_DIALOG (add_dialog->dialog1)->vbox;
	gtk_widget_show (add_dialog->dialog_vbox1);

	add_dialog->frame5 = gtk_frame_new (NULL);
	gtk_widget_show (add_dialog->frame5);
	gtk_box_pack_start (GTK_BOX (add_dialog->dialog_vbox1), add_dialog->frame5, TRUE, TRUE, 5);
	gtk_widget_set_size_request (add_dialog->frame5, 380, -1);

	add_dialog->alignment5 = gtk_alignment_new (0.5, 0.5, 1, 1);
	gtk_widget_show (add_dialog->alignment5);
	gtk_container_add (GTK_CONTAINER (add_dialog->frame5), add_dialog->alignment5);
	gtk_alignment_set_padding (GTK_ALIGNMENT (add_dialog->alignment5), 2, 5, 5, 5);

	add_dialog->vbox7 = gtk_vbox_new (FALSE, 0);
	gtk_widget_show (add_dialog->vbox7);
	gtk_container_add (GTK_CONTAINER (add_dialog->alignment5), add_dialog->vbox7);

	add_dialog->scrolledwindow3 = gtk_scrolled_window_new (NULL, NULL);
	gtk_widget_show (add_dialog->scrolledwindow3);
	gtk_box_pack_start (GTK_BOX (add_dialog->vbox7), add_dialog->scrolledwindow3, TRUE, TRUE, 0);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (add_dialog->scrolledwindow3), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (add_dialog->scrolledwindow3), GTK_SHADOW_IN);

	add_dialog->file_list_treeview = gtk_tree_view_new ();
	gtk_widget_show (add_dialog->file_list_treeview);
	gtk_container_add (GTK_CONTAINER (add_dialog->scrolledwindow3), add_dialog->file_list_treeview);
	gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (add_dialog->file_list_treeview), FALSE);

	add_dialog->hbox1 = gtk_hbox_new (FALSE, 0);
	gtk_widget_show (add_dialog->hbox1);
	gtk_box_pack_start (GTK_BOX (add_dialog->vbox7), add_dialog->hbox1, TRUE, TRUE, 0);

	add_dialog->vbox8 = gtk_vbox_new (FALSE, 0);
	gtk_widget_show (add_dialog->vbox8);
	gtk_box_pack_start (GTK_BOX (add_dialog->hbox1), add_dialog->vbox8, TRUE, FALSE, 0);

	add_dialog->files_radio = gtk_radio_button_new_with_mnemonic (NULL, _("Files"));
	gtk_widget_show (add_dialog->files_radio);
	gtk_box_pack_start (GTK_BOX (add_dialog->vbox8), add_dialog->files_radio, FALSE, FALSE, 0);
	gtk_radio_button_set_group (GTK_RADIO_BUTTON (add_dialog->files_radio), add_dialog->file_dir_radio_group);
	add_dialog->file_dir_radio_group = gtk_radio_button_get_group (GTK_RADIO_BUTTON (add_dialog->files_radio));

	add_dialog->directories_radio = gtk_radio_button_new_with_mnemonic (NULL, _("Directories"));
	gtk_widget_show (add_dialog->directories_radio);
	gtk_box_pack_start (GTK_BOX (add_dialog->vbox8), add_dialog->directories_radio, FALSE, FALSE, 0);
	gtk_radio_button_set_group (GTK_RADIO_BUTTON (add_dialog->directories_radio), add_dialog->file_dir_radio_group);
	add_dialog->file_dir_radio_group = gtk_radio_button_get_group (GTK_RADIO_BUTTON (add_dialog->directories_radio));

	add_dialog->hbuttonbox2 = gtk_hbutton_box_new ();
	gtk_widget_show (add_dialog->hbuttonbox2);
	gtk_box_pack_start (GTK_BOX (add_dialog->hbox1), add_dialog->hbuttonbox2, TRUE, TRUE, 0);
	gtk_container_set_border_width (GTK_CONTAINER (add_dialog->hbuttonbox2), 5);
	gtk_button_box_set_layout (GTK_BUTTON_BOX (add_dialog->hbuttonbox2), GTK_BUTTONBOX_END);
	gtk_box_set_spacing (GTK_BOX (add_dialog->hbuttonbox2), 8);

	add_dialog->remove_button = gtk_button_new_from_stock ("gtk-remove");
	gtk_widget_show (add_dialog->remove_button);
	gtk_container_add (GTK_CONTAINER (add_dialog->hbuttonbox2), add_dialog->remove_button);
	GTK_WIDGET_SET_FLAGS (add_dialog->remove_button, GTK_CAN_DEFAULT);

	add_dialog->add_files_button = gtk_button_new_from_stock ("gtk-add");
	gtk_widget_show (add_dialog->add_files_button);
	gtk_tooltips_set_tip (add_dialog->add_option_tooltip,add_dialog->add_files_button , _("Choose the files or directories to add to the archive"), NULL );
	gtk_container_add (GTK_CONTAINER (add_dialog->hbuttonbox2), add_dialog->add_files_button);
	GTK_WIDGET_SET_FLAGS (add_dialog->add_files_button, GTK_CAN_DEFAULT);
	g_signal_connect ( (gpointer) add_dialog->add_files_button, "clicked", G_CALLBACK (Show_File_Dialog) ,  NULL );
	
	add_dialog->label3 = gtk_label_new (_("<b>Files and directories to add </b>"));
	gtk_widget_show (add_dialog->label3);
	gtk_frame_set_label_widget (GTK_FRAME (add_dialog->frame5), add_dialog->label3);
	gtk_label_set_use_markup (GTK_LABEL (add_dialog->label3), TRUE);

	add_dialog->frame4 = gtk_frame_new (NULL);
	gtk_widget_show (add_dialog->frame4);
	gtk_box_pack_start (GTK_BOX (add_dialog->dialog_vbox1), add_dialog->frame4, TRUE, FALSE, 0);

	add_dialog->alignment4 = gtk_alignment_new (0.5, 0.5, 1, 1);
	gtk_widget_show (add_dialog->alignment4);
	gtk_container_add (GTK_CONTAINER (add_dialog->frame4), add_dialog->alignment4);
	gtk_alignment_set_padding (GTK_ALIGNMENT (add_dialog->alignment4), 0, 0, 12, 0);

	add_dialog->vbox6 = gtk_vbox_new (FALSE, 0);
	gtk_widget_show (add_dialog->vbox6);
	gtk_container_add (GTK_CONTAINER (add_dialog->alignment4), add_dialog->vbox6);

	add_dialog->checkbutton1 = gtk_check_button_new_with_mnemonic (_("Add files with full path"));
	gtk_widget_show (add_dialog->checkbutton1);
	gtk_box_pack_start (GTK_BOX (add_dialog->vbox6), add_dialog->checkbutton1, FALSE, FALSE, 0);

	add_dialog->checkbutton2 = gtk_check_button_new_with_mnemonic (_("Delete files after adding"));
	gtk_widget_show (add_dialog->checkbutton2);
	gtk_box_pack_start (GTK_BOX (add_dialog->vbox6), add_dialog->checkbutton2, FALSE, FALSE, 0);

	add_dialog->checkbutton3 = gtk_check_button_new_with_mnemonic (_("checkbutton3"));
	gtk_widget_show (add_dialog->checkbutton3);
	gtk_box_pack_start (GTK_BOX (add_dialog->vbox6), add_dialog->checkbutton3, FALSE, FALSE, 0);

	add_dialog->label2 = gtk_label_new (_("<b>Options </b>"));
	gtk_widget_show (add_dialog->label2);
	gtk_frame_set_label_widget (GTK_FRAME (add_dialog->frame4), add_dialog->label2);
	gtk_label_set_use_markup (GTK_LABEL (add_dialog->label2), TRUE);
	
	add_dialog->dialog_action_area2 = GTK_DIALOG (add_dialog->dialog1)->action_area;
	gtk_widget_show (add_dialog->dialog_action_area2);
	gtk_button_box_set_layout (GTK_BUTTON_BOX (add_dialog->dialog_action_area2), GTK_BUTTONBOX_END);

	add_dialog->cancel_button = gtk_button_new_from_stock ("gtk-cancel");
	gtk_widget_show (add_dialog->cancel_button);
	gtk_dialog_add_action_widget (GTK_DIALOG (add_dialog->dialog1), add_dialog->cancel_button, GTK_RESPONSE_CANCEL);
	GTK_WIDGET_SET_FLAGS (add_dialog->cancel_button, GTK_CAN_DEFAULT);

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

