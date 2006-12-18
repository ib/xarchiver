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

#include "config.h"
#include <glib.h>
#include <gtk/gtk.h>
#include "pref_dialog.h"
#include "interface.h"
#include "main.h"
#include "support.h"

Prefs_dialog_data *xa_create_prefs_dialog()
{
	Prefs_dialog_data *prefs_data;
	GList *archive_type;

	prefs_data = g_new0 (Prefs_dialog_data,1);
	prefs_data->dialog1 = gtk_dialog_new_with_buttons (_("Preferences"),
									GTK_WINDOW (MainWindow), GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
									GTK_STOCK_CANCEL,GTK_RESPONSE_CANCEL,GTK_STOCK_OK,GTK_RESPONSE_OK, NULL);

	prefs_data->prefs_notebook = GTK_NOTEBOOK(gtk_notebook_new() );
	gtk_notebook_set_tab_pos (prefs_data->prefs_notebook, GTK_POS_TOP);
	gtk_notebook_popup_disable (prefs_data->prefs_notebook);
	gtk_widget_show (GTK_WIDGET(prefs_data->prefs_notebook));
	gtk_dialog_set_default_response (GTK_DIALOG (prefs_data->dialog1), GTK_RESPONSE_OK);

	prefs_data->vbox1 = GTK_DIALOG (prefs_data->dialog1)->vbox;
	gtk_widget_show (prefs_data->vbox1);
	gtk_box_pack_start (GTK_BOX (prefs_data->vbox1), GTK_WIDGET(prefs_data->prefs_notebook), TRUE, TRUE, 6);

	/* Behaviour page*/
	prefs_data->frame1 = gtk_frame_new (NULL);
	gtk_widget_show (prefs_data->frame1);
	gtk_container_add (GTK_CONTAINER (prefs_data->prefs_notebook), prefs_data->frame1);
	gtk_frame_set_shadow_type (GTK_FRAME (prefs_data->frame1), GTK_SHADOW_NONE);

	prefs_data->alignment1 = gtk_alignment_new (0.5, 0.5, 1, 1);
	gtk_widget_show (prefs_data->alignment1);
	gtk_container_add (GTK_CONTAINER (prefs_data->frame1), prefs_data->alignment1);
	gtk_alignment_set_padding (GTK_ALIGNMENT (prefs_data->alignment1), 0, 0, 12, 0);

	prefs_data->vbox4 = gtk_vbox_new (FALSE, 2);
	gtk_widget_show (prefs_data->vbox4);
	gtk_container_add (GTK_CONTAINER (prefs_data->alignment1), prefs_data->vbox4);

	prefs_data->hbox1 = gtk_hbox_new (FALSE, 10);
	gtk_widget_show (prefs_data->hbox1);
	gtk_box_pack_start (GTK_BOX (prefs_data->vbox4), prefs_data->hbox1, FALSE, TRUE, 0);

	prefs_data->label4 = gtk_label_new (_("Preferred format for new archives is"));
	gtk_widget_show (prefs_data->label4);
	gtk_box_pack_start (GTK_BOX (prefs_data->hbox1), prefs_data->label4, FALSE, FALSE, 0);

	prefs_data->combo_box = gtk_combo_box_new_text();
	gtk_widget_show (prefs_data->combo_box);
	gtk_box_pack_start (GTK_BOX (prefs_data->hbox1), prefs_data->combo_box, FALSE, TRUE, 0);
	archive_type = g_list_first ( ArchiveType );
	while ( archive_type != NULL )
	{
		gtk_combo_box_append_text (GTK_COMBO_BOX (prefs_data->combo_box), archive_type->data );
		archive_type = g_list_next ( archive_type );
	}

	prefs_data->check_save_add_dialog = gtk_check_button_new_with_mnemonic (_("Save settings for add dialog"));
	gtk_widget_show (prefs_data->check_save_add_dialog);
	gtk_box_pack_start (GTK_BOX (prefs_data->vbox4), prefs_data->check_save_add_dialog, FALSE, FALSE, 0);
	GTK_WIDGET_UNSET_FLAGS (prefs_data->check_save_add_dialog, GTK_CAN_FOCUS);
	gtk_button_set_focus_on_click (GTK_BUTTON (prefs_data->check_save_add_dialog), FALSE);

	prefs_data->check_save_extract_dialog = gtk_check_button_new_with_mnemonic (_("Save settings for extract dialog"));
	gtk_widget_show (prefs_data->check_save_extract_dialog);
	gtk_box_pack_start (GTK_BOX (prefs_data->vbox4), prefs_data->check_save_extract_dialog, FALSE, FALSE, 0);
	GTK_WIDGET_UNSET_FLAGS (prefs_data->check_save_extract_dialog, GTK_CAN_FOCUS);
	gtk_button_set_focus_on_click (GTK_BUTTON (prefs_data->check_save_extract_dialog), FALSE);

	prefs_data->allow_dir_extract_with_dnd = gtk_check_button_new_with_mnemonic (_("Allow extracting directories by drag and drop"));
	gtk_widget_show (prefs_data->allow_dir_extract_with_dnd);
	gtk_box_pack_start (GTK_BOX (prefs_data->vbox4), prefs_data->allow_dir_extract_with_dnd, FALSE, FALSE, 0);
	GTK_WIDGET_UNSET_FLAGS (prefs_data->allow_dir_extract_with_dnd, GTK_CAN_FOCUS);
	gtk_button_set_focus_on_click (GTK_BUTTON (prefs_data->allow_dir_extract_with_dnd), FALSE);

	prefs_data->label1 = gtk_label_new (_("Behaviour"));
	gtk_widget_show (prefs_data->label1);
	gtk_notebook_set_tab_label (GTK_NOTEBOOK (prefs_data->prefs_notebook), gtk_notebook_get_nth_page (GTK_NOTEBOOK (prefs_data->prefs_notebook), 0), prefs_data->label1);

	/* View page*/
	prefs_data->vbox2 = gtk_vbox_new (FALSE, 10);
	gtk_widget_show (prefs_data->vbox2);
	gtk_container_add (GTK_CONTAINER (prefs_data->prefs_notebook), prefs_data->vbox2);

	prefs_data->frame2 = gtk_frame_new (NULL);
	gtk_widget_show (prefs_data->frame2);
	gtk_box_pack_start (GTK_BOX (prefs_data->vbox2), prefs_data->frame2, TRUE, TRUE, 0);
	gtk_frame_set_shadow_type (GTK_FRAME (prefs_data->frame2), GTK_SHADOW_NONE);

	prefs_data->label2 = gtk_label_new (_("View"));
	gtk_widget_show (prefs_data->label2);
	gtk_notebook_set_tab_label (GTK_NOTEBOOK (prefs_data->prefs_notebook), gtk_notebook_get_nth_page (GTK_NOTEBOOK (prefs_data->prefs_notebook), 1), prefs_data->label2);

	/* Advanced page*/
	prefs_data->vbox3 = gtk_vbox_new (FALSE, 10);
	gtk_widget_show (prefs_data->vbox3);
	gtk_container_add (GTK_CONTAINER (prefs_data->prefs_notebook), prefs_data->vbox3);

	prefs_data->frame3 = gtk_frame_new (NULL);
	gtk_widget_show (prefs_data->frame3);
	gtk_box_pack_start (GTK_BOX (prefs_data->vbox3), prefs_data->frame3, TRUE, TRUE, 0);
	gtk_frame_set_shadow_type (GTK_FRAME (prefs_data->frame3), GTK_SHADOW_NONE);

	prefs_data->label3 = gtk_label_new (_("Advanced"));
	gtk_widget_show (prefs_data->label3);
	gtk_notebook_set_tab_label (GTK_NOTEBOOK (prefs_data->prefs_notebook), gtk_notebook_get_nth_page (GTK_NOTEBOOK (prefs_data->prefs_notebook), 2), prefs_data->label3);
	return prefs_data;
}
