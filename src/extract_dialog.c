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
#include "extract_dialog.h"
#include "interface.h"
#include "support.h"

Extract_dialog_data *create_extract_dialog (gint selected)
{
	Extract_dialog_data *dialog_data;

	dialog_data = g_new (Extract_dialog_data, 1);
	dialog_data->radio_group = NULL;
	dialog_data->dialog1 = gtk_dialog_new ();
	gtk_window_set_title (GTK_WINDOW (dialog_data->dialog1), _("Extract Dialog"));
	gtk_window_set_type_hint (GTK_WINDOW (dialog_data->dialog1), GDK_WINDOW_TYPE_HINT_DIALOG);
	gtk_window_set_transient_for ( GTK_WINDOW (dialog_data->dialog1) , GTK_WINDOW (MainWindow) );

	dialog_data->dialog_vbox1 = GTK_DIALOG (dialog_data->dialog1)->vbox;
	gtk_widget_show (dialog_data->dialog_vbox1);

	dialog_data->vbox1 = gtk_vbox_new (FALSE, 0);
	gtk_widget_show (dialog_data->vbox1);
	gtk_box_pack_start (GTK_BOX (dialog_data->dialog_vbox1), dialog_data->vbox1, TRUE, TRUE, 0);

	dialog_data->hbox1 = gtk_hbox_new (FALSE, 0);
	gtk_widget_show (dialog_data->hbox1);
	gtk_box_pack_start (GTK_BOX (dialog_data->vbox1), dialog_data->hbox1, TRUE, TRUE, 0);

	dialog_data->vbox2 = gtk_vbox_new (FALSE, 0);
	gtk_widget_show (dialog_data->vbox2);
	gtk_box_pack_start (GTK_BOX (dialog_data->hbox1), dialog_data->vbox2, FALSE, FALSE, 0);

	dialog_data->hbox2 = gtk_hbox_new (FALSE, 0);
	gtk_widget_show (dialog_data->hbox2);
	gtk_box_pack_start (GTK_BOX (dialog_data->vbox2), dialog_data->hbox2, FALSE, TRUE, 17);

	dialog_data->hbox3 = gtk_hbox_new (FALSE, 0);
	gtk_widget_show (dialog_data->hbox3);
	gtk_box_pack_start (GTK_BOX (dialog_data->hbox2), dialog_data->hbox3, FALSE, FALSE, 0);

	dialog_data->extract_to_label = gtk_label_new (_("Extract to:"));
	gtk_widget_show (dialog_data->extract_to_label);
	gtk_box_pack_start (GTK_BOX (dialog_data->hbox3), dialog_data->extract_to_label, FALSE, TRUE, 0);

	dialog_data->destination_path_entry = gtk_entry_new ();
	gtk_widget_show (dialog_data->destination_path_entry);
	gtk_box_pack_start (GTK_BOX (dialog_data->hbox3), dialog_data->destination_path_entry, FALSE, TRUE, 0);
	gtk_widget_set_size_request (dialog_data->destination_path_entry, 385, -1);
	gtk_entry_set_activates_default (GTK_ENTRY (dialog_data->destination_path_entry), TRUE);

	dialog_data->image1 = gtk_image_new_from_stock ("gtk-open", GTK_ICON_SIZE_BUTTON);
	gtk_widget_show (dialog_data->image1);
	gtk_box_pack_start (GTK_BOX (dialog_data->hbox3), dialog_data->image1, FALSE, TRUE, 0);
	gtk_widget_set_size_request (dialog_data->image1, 30, -1);

	dialog_data->hbox4 = gtk_hbox_new (TRUE, 7);
	gtk_widget_show (dialog_data->hbox4);
	gtk_box_pack_start (GTK_BOX (dialog_data->vbox1), dialog_data->hbox4, FALSE, FALSE, 0);

	dialog_data->frame1 = gtk_frame_new (NULL);
	gtk_widget_show (dialog_data->frame1);
	gtk_box_pack_start (GTK_BOX (dialog_data->hbox4), dialog_data->frame1, TRUE, TRUE, 0);
	gtk_widget_set_size_request (dialog_data->frame1, 1, -1);
	gtk_frame_set_shadow_type (GTK_FRAME (dialog_data->frame1), GTK_SHADOW_OUT);

	dialog_data->alignment1 = gtk_alignment_new (0.5, 0.5, 1, 1);
	gtk_widget_show (dialog_data->alignment1);
	gtk_container_add (GTK_CONTAINER (dialog_data->frame1), dialog_data->alignment1);
	gtk_alignment_set_padding (GTK_ALIGNMENT (dialog_data->alignment1), 0, 0, 12, 0);

	dialog_data->vbox3 = gtk_vbox_new (FALSE, 0);
	gtk_widget_show (dialog_data->vbox3);
	gtk_container_add (GTK_CONTAINER (dialog_data->alignment1), dialog_data->vbox3);

	dialog_data->all_files_radio = gtk_radio_button_new_with_mnemonic (NULL, _("All"));
	gtk_widget_show (dialog_data->all_files_radio);
	gtk_box_pack_start (GTK_BOX (dialog_data->vbox3), dialog_data->all_files_radio, FALSE, FALSE, 0);
	gtk_radio_button_set_group (GTK_RADIO_BUTTON (dialog_data->all_files_radio), dialog_data->radio_group);
	dialog_data->radio_group = gtk_radio_button_get_group (GTK_RADIO_BUTTON (dialog_data->all_files_radio));

	dialog_data->selected_files_radio = gtk_radio_button_new_with_mnemonic (NULL, _("Only selected"));
	gtk_widget_show (dialog_data->selected_files_radio);
	gtk_box_pack_start (GTK_BOX (dialog_data->vbox3), dialog_data->selected_files_radio, FALSE, FALSE, 0);
	gtk_radio_button_set_group (GTK_RADIO_BUTTON (dialog_data->selected_files_radio), dialog_data->radio_group);
	dialog_data->radio_group = gtk_radio_button_get_group (GTK_RADIO_BUTTON (dialog_data->selected_files_radio));

	if (selected)
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (dialog_data->selected_files_radio), TRUE);
	dialog_data->files_frame_label = gtk_label_new (_("<b>Files to extract </b>"));
	gtk_widget_show (dialog_data->files_frame_label);
	gtk_frame_set_label_widget (GTK_FRAME (dialog_data->frame1), dialog_data->files_frame_label);
	gtk_label_set_use_markup (GTK_LABEL (dialog_data->files_frame_label), TRUE);

	dialog_data->frame2 = gtk_frame_new (NULL);
	gtk_widget_show (dialog_data->frame2);
	gtk_box_pack_start (GTK_BOX (dialog_data->hbox4), dialog_data->frame2, TRUE, TRUE, 0);
	gtk_frame_set_shadow_type (GTK_FRAME (dialog_data->frame2), GTK_SHADOW_OUT);

	dialog_data->alignment2 = gtk_alignment_new (0.5, 0.5, 1, 1);
	gtk_widget_show (dialog_data->alignment2);
	gtk_container_add (GTK_CONTAINER (dialog_data->frame2), dialog_data->alignment2);
	gtk_alignment_set_padding (GTK_ALIGNMENT (dialog_data->alignment2), 0, 0, 12, 0);

	dialog_data->vbox4 = gtk_vbox_new (FALSE, 0);
	gtk_widget_show (dialog_data->vbox4);
	gtk_container_add (GTK_CONTAINER (dialog_data->alignment2), dialog_data->vbox4);

	dialog_data->overwrite_check = gtk_check_button_new_with_mnemonic (_("Overwrite existing files"));
	gtk_widget_show (dialog_data->overwrite_check);
	gtk_box_pack_start (GTK_BOX (dialog_data->vbox4), dialog_data->overwrite_check, FALSE, FALSE, 0);

	dialog_data->exclude_path_check = gtk_check_button_new_with_mnemonic (_("Exclude path from names"));
	gtk_widget_show (dialog_data->exclude_path_check);
	gtk_box_pack_start (GTK_BOX (dialog_data->vbox4), dialog_data->exclude_path_check, FALSE, FALSE, 0);

	dialog_data->exclude_base_dir_check = gtk_check_button_new_with_mnemonic (_("Exclude base dir from names"));
	gtk_widget_show (dialog_data->exclude_base_dir_check);
	gtk_box_pack_start (GTK_BOX (dialog_data->vbox4), dialog_data->exclude_base_dir_check, FALSE, FALSE, 0);

	dialog_data->hbox5 = gtk_hbox_new (FALSE, 2);
	gtk_widget_show (dialog_data->hbox5);
	gtk_box_pack_start (GTK_BOX (dialog_data->vbox4), dialog_data->hbox5, FALSE, FALSE, 0);

	dialog_data->label_password = gtk_label_new (_("Password:"));
	gtk_widget_show (dialog_data->label_password);
	gtk_box_pack_start (GTK_BOX (dialog_data->hbox5), dialog_data->label_password, FALSE, FALSE, 0);

	dialog_data->password_entry = gtk_entry_new ();
	gtk_widget_show (dialog_data->password_entry);
	gtk_box_pack_start (GTK_BOX (dialog_data->hbox5), dialog_data->password_entry, FALSE, FALSE, 0);
	gtk_widget_set_size_request (dialog_data->password_entry, 136, -1);
	gtk_entry_set_visibility (GTK_ENTRY (dialog_data->password_entry), FALSE);

	dialog_data->options_frame_label = gtk_label_new (_("<b>Options </b>"));
	gtk_widget_show (dialog_data->options_frame_label);
	gtk_frame_set_label_widget (GTK_FRAME (dialog_data->frame2), dialog_data->options_frame_label);
	gtk_label_set_use_markup (GTK_LABEL (dialog_data->options_frame_label), TRUE);

	dialog_data->dialog_action_area1 = GTK_DIALOG (dialog_data->dialog1)->action_area;
	gtk_widget_show (dialog_data->dialog_action_area1);
	gtk_button_box_set_layout (GTK_BUTTON_BOX (dialog_data->dialog_action_area1), GTK_BUTTONBOX_END);

	dialog_data->cancel_button = gtk_button_new_with_mnemonic (_("Cancel"));
	gtk_widget_show (dialog_data->cancel_button);
	gtk_dialog_add_action_widget (GTK_DIALOG (dialog_data->dialog1), dialog_data->cancel_button, GTK_RESPONSE_CANCEL);
	GTK_WIDGET_SET_FLAGS (dialog_data->cancel_button, GTK_CAN_DEFAULT);

	dialog_data->extract_button = gtk_button_new_with_mnemonic (_("Extract"));
	gtk_widget_show (dialog_data->extract_button);
	gtk_dialog_add_action_widget (GTK_DIALOG (dialog_data->dialog1), dialog_data->extract_button, GTK_RESPONSE_OK);
	GTK_WIDGET_SET_FLAGS (dialog_data->extract_button, GTK_CAN_DEFAULT);
	return dialog_data;
}

