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

extern gboolean unrar;
Prefs_dialog_data *xa_create_prefs_dialog()
{
	GtkNotebook *prefs_notebook;
	GtkWidget *label1, *label2, *label3, *label4, *vbox1, *vbox2, *vbox3, *vbox4, *frame1, *frame2, *frame3;
	GtkWidget *hbox1, *alignment1,*alignment2, *hbox3, *label5,*alignment3, *hbox4;
	GtkWidget *hbox5, *label6, *label7;
	GList *archive_type;
	Prefs_dialog_data *prefs_data;

	prefs_data = g_new0 (Prefs_dialog_data,1);
	prefs_data->dialog1 = gtk_dialog_new_with_buttons (_("Preferences"),
									GTK_WINDOW (MainWindow), GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
									GTK_STOCK_CANCEL,GTK_RESPONSE_CANCEL,GTK_STOCK_OK,GTK_RESPONSE_OK, NULL);
	tooltips = gtk_tooltips_new ();
	prefs_notebook = GTK_NOTEBOOK(gtk_notebook_new() );
	gtk_notebook_set_tab_pos (prefs_notebook, GTK_POS_TOP);
	gtk_notebook_popup_disable (prefs_notebook);
	gtk_widget_show (GTK_WIDGET(prefs_notebook));
	gtk_dialog_set_default_response (GTK_DIALOG (prefs_data->dialog1), GTK_RESPONSE_OK);

	vbox1 = GTK_DIALOG (prefs_data->dialog1)->vbox;
	gtk_widget_show (vbox1);
	gtk_box_pack_start (GTK_BOX (vbox1), GTK_WIDGET(prefs_notebook), TRUE, TRUE, 6);

	/* Behaviour page*/
	frame1 = gtk_frame_new (NULL);
	gtk_widget_show (frame1);
	gtk_container_add (GTK_CONTAINER (prefs_notebook), frame1);
	gtk_frame_set_shadow_type (GTK_FRAME (frame1), GTK_SHADOW_NONE);

	alignment1 = gtk_alignment_new (0.5, 0.5, 1, 1);
	gtk_widget_show (alignment1);
	gtk_container_add (GTK_CONTAINER (frame1), alignment1);
	gtk_alignment_set_padding (GTK_ALIGNMENT (alignment1), 0, 0, 12, 0);

	vbox4 = gtk_vbox_new (FALSE, 2);
	gtk_widget_show (vbox4);
	gtk_container_add (GTK_CONTAINER (alignment1), vbox4);

	hbox1 = gtk_hbox_new (FALSE, 5);
	gtk_widget_show (hbox1);
	gtk_box_pack_start (GTK_BOX (vbox4), hbox1, FALSE, TRUE, 0);

	label4 = gtk_label_new (_("Preferred format for new archives:"));
	gtk_widget_show (label4);
	gtk_box_pack_start (GTK_BOX (hbox1), label4, FALSE, FALSE, 0);

	prefs_data->combo_box1 = gtk_combo_box_new_text();
	gtk_widget_show (prefs_data->combo_box1);
	gtk_box_pack_start (GTK_BOX (hbox1), prefs_data->combo_box1, FALSE, TRUE, 0);
	archive_type = g_list_first ( ArchiveType );
	while ( archive_type != NULL )
	{
		if (archive_type->data == "tgz" || archive_type->data == "rpm" || archive_type->data == "iso" || archive_type->data == "gz" || archive_type->data == "bz2" || (archive_type->data == "rar" && unrar) )
			goto next;
		else
			gtk_combo_box_append_text (GTK_COMBO_BOX (prefs_data->combo_box1),archive_type->data );
		next:
			archive_type = g_list_next ( archive_type );
	}

	prefs_data->check_save_add_dialog = gtk_check_button_new_with_mnemonic (_("Save settings for add dialog"));
	gtk_widget_show (prefs_data->check_save_add_dialog);
	gtk_box_pack_start (GTK_BOX (vbox4), prefs_data->check_save_add_dialog, FALSE, FALSE, 0);
	GTK_WIDGET_UNSET_FLAGS (prefs_data->check_save_add_dialog, GTK_CAN_FOCUS);
	gtk_button_set_focus_on_click (GTK_BUTTON (prefs_data->check_save_add_dialog), FALSE);

	prefs_data->check_save_extract_dialog = gtk_check_button_new_with_mnemonic (_("Save settings for extract dialog"));
	gtk_widget_show (prefs_data->check_save_extract_dialog);
	gtk_box_pack_start (GTK_BOX (vbox4), prefs_data->check_save_extract_dialog, FALSE, FALSE, 0);
	GTK_WIDGET_UNSET_FLAGS (prefs_data->check_save_extract_dialog, GTK_CAN_FOCUS);
	gtk_button_set_focus_on_click (GTK_BUTTON (prefs_data->check_save_extract_dialog), FALSE);

	prefs_data->allow_dir_extract_with_dnd = gtk_check_button_new_with_mnemonic (_("Allow extracting directories by drag and drop"));
	gtk_widget_show (prefs_data->allow_dir_extract_with_dnd);
	gtk_box_pack_start (GTK_BOX (vbox4), prefs_data->allow_dir_extract_with_dnd, FALSE, FALSE, 0);
	GTK_WIDGET_UNSET_FLAGS (prefs_data->allow_dir_extract_with_dnd, GTK_CAN_FOCUS);
	gtk_button_set_focus_on_click (GTK_BUTTON (prefs_data->allow_dir_extract_with_dnd), FALSE);

	prefs_data->confirm_deletion = gtk_check_button_new_with_mnemonic (_("Confirm deletion of files inside the archive"));
	gtk_widget_show (prefs_data->confirm_deletion);
	gtk_box_pack_start (GTK_BOX (vbox4), prefs_data->confirm_deletion, FALSE, FALSE, 0);
	GTK_WIDGET_UNSET_FLAGS (prefs_data->confirm_deletion, GTK_CAN_FOCUS);
	gtk_button_set_focus_on_click (GTK_BUTTON (prefs_data->confirm_deletion), FALSE);

	label1 = gtk_label_new (_("Behaviour"));
	gtk_widget_show (label1);
	gtk_notebook_set_tab_label (GTK_NOTEBOOK (prefs_notebook), gtk_notebook_get_nth_page (GTK_NOTEBOOK (prefs_notebook), 0), label1);

	/* View page*/
	frame2 = gtk_frame_new (NULL);
	gtk_widget_show (frame2);
	gtk_container_add (GTK_CONTAINER (prefs_notebook), frame2);
	gtk_frame_set_shadow_type (GTK_FRAME (frame2), GTK_SHADOW_NONE);

	alignment2 = gtk_alignment_new (0.5, 0.5, 1, 1);
	gtk_widget_show (alignment2);
	gtk_container_add (GTK_CONTAINER (frame2), alignment2);
	gtk_alignment_set_padding (GTK_ALIGNMENT (alignment2), 0, 0, 12, 0);

	vbox2 = gtk_vbox_new (FALSE, 2);
	gtk_widget_show (vbox2);
	gtk_container_add (GTK_CONTAINER (alignment2), vbox2);

	hbox3 = gtk_hbox_new (FALSE, 5);
	gtk_widget_show (hbox3);
	gtk_box_pack_start (GTK_BOX (vbox2), hbox3, FALSE, TRUE, 0);

	label5 = gtk_label_new (_("View archive content as"));
	gtk_widget_show (label5);
	gtk_box_pack_start (GTK_BOX (hbox3), label5, FALSE, FALSE, 0);

	prefs_data->combo_box2 = gtk_combo_box_new_text();
	gtk_widget_show (prefs_data->combo_box2);
	gtk_box_pack_start (GTK_BOX (hbox3), prefs_data->combo_box2, FALSE, TRUE, 0);
	gtk_combo_box_append_text (GTK_COMBO_BOX (prefs_data->combo_box2), _("list") );
	gtk_combo_box_append_text (GTK_COMBO_BOX (prefs_data->combo_box2), _("icon") );

	prefs_data->check_show_comment = gtk_check_button_new_with_mnemonic (_("Show comment in archive after loading it"));
	gtk_widget_show (prefs_data->check_show_comment);
	gtk_box_pack_start (GTK_BOX (vbox2), prefs_data->check_show_comment, FALSE, FALSE, 0);
	GTK_WIDGET_UNSET_FLAGS (prefs_data->check_show_comment, GTK_CAN_FOCUS);
	gtk_button_set_focus_on_click (GTK_BUTTON (prefs_data->check_show_comment), FALSE);

	prefs_data->check_show_iso_info = gtk_check_button_new_with_mnemonic (_("Show ISO info after loading the archive"));
	gtk_widget_show (prefs_data->check_show_iso_info);
	gtk_box_pack_start (GTK_BOX (vbox2), prefs_data->check_show_iso_info, FALSE, FALSE, 0);
	GTK_WIDGET_UNSET_FLAGS (prefs_data->check_show_iso_info, GTK_CAN_FOCUS);
	gtk_button_set_focus_on_click (GTK_BUTTON (prefs_data->check_show_iso_info), FALSE);

	prefs_data->check_sort_filename_column = gtk_check_button_new_with_mnemonic (_("Sort content by filename"));
	gtk_widget_show (prefs_data->check_sort_filename_column);
	gtk_box_pack_start (GTK_BOX (vbox2), prefs_data->check_sort_filename_column, FALSE, FALSE, 0);
	GTK_WIDGET_UNSET_FLAGS (prefs_data->check_sort_filename_column, GTK_CAN_FOCUS);
	gtk_button_set_focus_on_click (GTK_BUTTON (prefs_data->check_sort_filename_column), FALSE);
	gtk_tooltips_set_tip(tooltips, prefs_data->check_sort_filename_column, _("The filename column is sort after loading the archive"), NULL);

	label2 = gtk_label_new (_("View"));
	gtk_widget_show (label2);
	gtk_notebook_set_tab_label (GTK_NOTEBOOK (prefs_notebook), gtk_notebook_get_nth_page (GTK_NOTEBOOK (prefs_notebook), 1), label2);

	/* Advanced page*/
	frame3 = gtk_frame_new (NULL);
	gtk_widget_show (frame3);
	gtk_container_add (GTK_CONTAINER (prefs_notebook), frame3);
	gtk_frame_set_shadow_type (GTK_FRAME (frame3), GTK_SHADOW_NONE);

	alignment3 = gtk_alignment_new (0.5, 0.5, 1, 1);
	gtk_widget_show (alignment3);
	gtk_container_add (GTK_CONTAINER (frame3), alignment3);
	gtk_alignment_set_padding (GTK_ALIGNMENT (alignment3), 0, 0, 12, 0);

	vbox3 = gtk_vbox_new (FALSE, 2);
	gtk_widget_show (vbox3);
	gtk_container_add (GTK_CONTAINER (alignment3), vbox3);

	hbox4 = gtk_hbox_new (FALSE, 5);
	gtk_widget_show (hbox4);
	gtk_box_pack_start (GTK_BOX (vbox3), hbox4, FALSE, TRUE, 0);

	label6 = gtk_label_new (_("Preferred web browser:"));
	gtk_widget_show (label6);
	gtk_box_pack_start (GTK_BOX (hbox4), label6, FALSE, FALSE, 0);

	prefs_data->combo_box3 = gtk_combo_box_new_text();
	gtk_widget_show (prefs_data->combo_box3);
	gtk_box_pack_start (GTK_BOX (hbox4), prefs_data->combo_box3, FALSE, TRUE, 0);
	gtk_combo_box_append_text (GTK_COMBO_BOX (prefs_data->combo_box3), _("xfce default") );
	gtk_combo_box_append_text (GTK_COMBO_BOX (prefs_data->combo_box3), _("konqueror") );

	hbox5 = gtk_hbox_new (FALSE, 5);
	gtk_widget_show (hbox5);
	gtk_box_pack_start (GTK_BOX (vbox3), hbox5, FALSE, TRUE, 0);

	label7 = gtk_label_new (_("Preferred view application:"));
	gtk_widget_show (label7);
	gtk_box_pack_start (GTK_BOX (hbox5), label7, FALSE, FALSE, 0);

	prefs_data->combo_box4 = gtk_combo_box_new_text();
	gtk_widget_show (prefs_data->combo_box4);
	gtk_box_pack_start (GTK_BOX (hbox5), prefs_data->combo_box4, FALSE, TRUE, 0);
	gtk_combo_box_append_text (GTK_COMBO_BOX (prefs_data->combo_box4), _("internal") );
	gtk_combo_box_append_text (GTK_COMBO_BOX (prefs_data->combo_box4), _("icon") );

	prefs_data->check_save_geometry = gtk_check_button_new_with_mnemonic (_("Save window geometry on exit"));
	gtk_widget_show (prefs_data->check_save_geometry);
	gtk_box_pack_start (GTK_BOX (vbox3), prefs_data->check_save_geometry, FALSE, FALSE, 0);
	GTK_WIDGET_UNSET_FLAGS (prefs_data->check_save_geometry, GTK_CAN_FOCUS);
	gtk_button_set_focus_on_click (GTK_BUTTON (prefs_data->check_save_geometry), FALSE);

	label3 = gtk_label_new (_("Advanced"));
	gtk_widget_show (label3);
	gtk_notebook_set_tab_label (GTK_NOTEBOOK (prefs_notebook), gtk_notebook_get_nth_page (GTK_NOTEBOOK (prefs_notebook), 2), label3);
	return prefs_data;
}
