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
#include "main.h"
#include "support.h"

extern gboolean unrar;
Prefs_dialog_data *xa_create_prefs_dialog()
{
	GtkWidget *vbox1, *vbox2, *vbox3, *vbox4,*hbox1, *scrolledwindow1, *prefs_iconview, *label5;
	GtkWidget *label1, *label2, *label3, *label4, *frame1, *frame2, *frame3, *alignment1, *alignment2, *alignment3;
	GtkWidget *hbox3, *hbox4, *label6, *label7, *hbox5;
	GtkTreeIter iter;
	GList *archive_type;
	GdkPixbuf *icon_pixbuf;
	Prefs_dialog_data *prefs_data;

	prefs_data = g_new0 (Prefs_dialog_data,1);
	prefs_data->dialog1 = gtk_dialog_new_with_buttons (_("Preferences"),
									GTK_WINDOW (MainWindow), GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
									GTK_STOCK_CANCEL,GTK_RESPONSE_CANCEL,GTK_STOCK_OK,GTK_RESPONSE_OK, NULL);
	tooltips = gtk_tooltips_new ();
	gtk_dialog_set_default_response (GTK_DIALOG (prefs_data->dialog1), GTK_RESPONSE_OK);

	vbox1 = GTK_DIALOG (prefs_data->dialog1)->vbox;
	gtk_widget_show (vbox1);

	hbox1 = gtk_hbox_new (FALSE, 0);
	gtk_widget_show (hbox1);
	gtk_box_pack_start (GTK_BOX (vbox1), hbox1, TRUE, TRUE, 2);

	scrolledwindow1 = gtk_scrolled_window_new (NULL, NULL);
	gtk_box_pack_start (GTK_BOX (hbox1), scrolledwindow1, FALSE, FALSE, 0);
	g_object_set (G_OBJECT (scrolledwindow1),"hscrollbar-policy", GTK_POLICY_NEVER,"shadow-type", GTK_SHADOW_IN,"vscrollbar-policy", GTK_POLICY_NEVER, NULL);
	gtk_widget_show (scrolledwindow1);

	prefs_data->prefs_liststore = gtk_list_store_new ( 2, GDK_TYPE_PIXBUF, G_TYPE_STRING);
	prefs_iconview = gtk_icon_view_new_with_model(GTK_TREE_MODEL(prefs_data->prefs_liststore));
	g_object_unref (prefs_data->prefs_liststore);

	gtk_widget_set_size_request(prefs_iconview, 80, -1);
	gtk_icon_view_set_orientation (GTK_ICON_VIEW (prefs_iconview), GTK_ORIENTATION_VERTICAL);
	gtk_icon_view_set_columns (GTK_ICON_VIEW (prefs_iconview),1);
	gtk_icon_view_set_pixbuf_column (GTK_ICON_VIEW (prefs_iconview), 0);
	gtk_icon_view_set_text_column (GTK_ICON_VIEW (prefs_iconview), 1);
	gtk_container_add (GTK_CONTAINER (scrolledwindow1), prefs_iconview);

	gtk_list_store_append (prefs_data->prefs_liststore, &iter);
	icon_pixbuf = gdk_pixbuf_new_from_file ("./pixmaps/xarchiver-extract.png", NULL);
	gtk_list_store_set (prefs_data->prefs_liststore, &iter, 0, icon_pixbuf, 1, _("Behaviour"), -1);
	g_object_unref (icon_pixbuf);

	gtk_list_store_append (prefs_data->prefs_liststore, &iter);
	icon_pixbuf = gdk_pixbuf_new_from_file ("./pixmaps/xarchiver-add.png", NULL);
	gtk_list_store_set (prefs_data->prefs_liststore, &iter, 0, icon_pixbuf, 1, _("View"), -1);
	g_object_unref (icon_pixbuf);

	gtk_list_store_append (prefs_data->prefs_liststore, &iter);
	icon_pixbuf = gdk_pixbuf_new_from_file ("./pixmaps/xarchiver-extract.png", NULL);
	gtk_list_store_set (prefs_data->prefs_liststore, &iter, 0, icon_pixbuf, 1, _("Advanced"), -1);
	g_object_unref (icon_pixbuf);
	gtk_widget_show (prefs_iconview);

	prefs_data->prefs_notebook = gtk_notebook_new ();
	g_object_set (G_OBJECT (prefs_data->prefs_notebook),"show-border", FALSE,"show-tabs", FALSE,"enable-popup",FALSE,NULL);
	gtk_widget_show (prefs_data->prefs_notebook);
	gtk_box_pack_start (GTK_BOX (hbox1), prefs_data->prefs_notebook, TRUE, TRUE, 0);
	GTK_WIDGET_UNSET_FLAGS (prefs_data->prefs_notebook, GTK_CAN_FOCUS);
	g_signal_connect (G_OBJECT (prefs_iconview), "selection-changed",G_CALLBACK (xa_prefs_iconview_changed), prefs_data);

	/* Behaviour page*/
	frame1 = gtk_frame_new (NULL);
	gtk_widget_show (frame1);
	gtk_container_add (GTK_CONTAINER (prefs_data->prefs_notebook), frame1);
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

	label1 = gtk_label_new ("");
	gtk_widget_show (label1);
	gtk_notebook_set_tab_label (GTK_NOTEBOOK (prefs_data->prefs_notebook), gtk_notebook_get_nth_page (GTK_NOTEBOOK (prefs_data->prefs_notebook), 0), label1);

	/* View page*/
	frame2 = gtk_frame_new (NULL);
	gtk_widget_show (frame2);
	gtk_container_add (GTK_CONTAINER (prefs_data->prefs_notebook), frame2);
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

	label2 = gtk_label_new ("");
	gtk_widget_show (label2);
	gtk_notebook_set_tab_label (GTK_NOTEBOOK (prefs_data->prefs_notebook), gtk_notebook_get_nth_page (GTK_NOTEBOOK (prefs_data->prefs_notebook), 1), label2);

	/* Advanced page*/
	frame3 = gtk_frame_new (NULL);
	gtk_widget_show (frame3);
	gtk_container_add (GTK_CONTAINER (prefs_data->prefs_notebook), frame3);
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

	label3 = gtk_label_new ("");
	gtk_widget_show (label3);
	gtk_notebook_set_tab_label (GTK_NOTEBOOK (prefs_data->prefs_notebook), gtk_notebook_get_nth_page (GTK_NOTEBOOK (prefs_data->prefs_notebook), 2), label3);
	return prefs_data;
}

void xa_prefs_iconview_changed (GtkIconView *iconview, gpointer data)
{
	Prefs_dialog_data *prefs = data;
	GList *list;
	GtkTreePath *path;
	GtkTreeIter iter;
	gchar *text = NULL;

	list = gtk_icon_view_get_selected_items (iconview);
	if (list == NULL)
		return;

	list = g_list_first (list);
	path = (GtkTreePath*)list->data;

	gtk_tree_model_get_iter ( GTK_TREE_MODEL(prefs->prefs_liststore), &iter, path );
	gtk_tree_model_get ( GTK_TREE_MODEL(prefs->prefs_liststore), &iter, 1, &text, -1);

	gtk_tree_path_free( (GtkTreePath*)list->data );
	g_list_free (list);

	if (strncmp ( text,"B",1) == 0 )
		gtk_notebook_set_current_page (GTK_NOTEBOOK(prefs->prefs_notebook),0);
	else if (strncmp ( text,"V",1) == 0 )
		gtk_notebook_set_current_page (GTK_NOTEBOOK(prefs->prefs_notebook),1);
	else if (strncmp ( text,"A",1) == 0 )
		gtk_notebook_set_current_page (GTK_NOTEBOOK(prefs->prefs_notebook),2);

	g_free (text);
}
