/*
 *  Copyright (C) 2007 Giuseppe Torelli - <colossus73@gmail.com>
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
	GtkWidget *hbox3, *hbox4, *label6, *label7, *hbox5, *label8, *label9, *hbox6, *hbox7;
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
	hbox1 = gtk_hbox_new (FALSE, 6);
	gtk_box_pack_start (GTK_BOX (vbox1), hbox1, TRUE, TRUE, 6);

	scrolledwindow1 = gtk_scrolled_window_new (NULL, NULL);
	gtk_box_pack_start (GTK_BOX (hbox1), scrolledwindow1, TRUE, TRUE, 6);
	g_object_set (G_OBJECT (scrolledwindow1),"hscrollbar-policy", GTK_POLICY_NEVER,"shadow-type", GTK_SHADOW_IN,"vscrollbar-policy", GTK_POLICY_NEVER, NULL);
	
	prefs_data->prefs_liststore = gtk_list_store_new ( 3, GDK_TYPE_PIXBUF, G_TYPE_STRING, G_TYPE_UINT);
	gtk_list_store_append (prefs_data->prefs_liststore, &iter);
	icon_pixbuf = gdk_pixbuf_new_from_file ("./pixmaps/xarchiver-behaviour.svg", NULL);
	gtk_list_store_set (prefs_data->prefs_liststore, &iter, 0, icon_pixbuf, 1, _("Behaviour"),2,0,-1);
	g_object_unref (icon_pixbuf);

	gtk_list_store_append (prefs_data->prefs_liststore, &iter);
	icon_pixbuf = gtk_widget_render_icon (prefs_data->dialog1, "gtk-find", GTK_ICON_SIZE_DND, NULL);
	gtk_list_store_set (prefs_data->prefs_liststore, &iter, 0, icon_pixbuf, 1, _("View"),2,1,-1);
	g_object_unref (icon_pixbuf);

	gtk_list_store_append (prefs_data->prefs_liststore, &iter);
    icon_pixbuf = gtk_widget_render_icon (prefs_data->dialog1, "gtk-execute", GTK_ICON_SIZE_DND, NULL);
	gtk_list_store_set (prefs_data->prefs_liststore, &iter, 0, icon_pixbuf, 1, _("Advanced"),2,2,-1);
	g_object_unref (icon_pixbuf);

	prefs_iconview = gtk_icon_view_new_with_model(GTK_TREE_MODEL(prefs_data->prefs_liststore));
	g_object_unref (prefs_data->prefs_liststore);	

	gtk_widget_set_size_request(prefs_iconview, 140, 190);
	gtk_icon_view_set_orientation (GTK_ICON_VIEW (prefs_iconview), GTK_ORIENTATION_VERTICAL);
	gtk_icon_view_set_item_width (GTK_ICON_VIEW (prefs_iconview),130);
	gtk_icon_view_set_columns (GTK_ICON_VIEW (prefs_iconview),1);
	gtk_icon_view_set_pixbuf_column (GTK_ICON_VIEW (prefs_iconview), 0);
	gtk_icon_view_set_text_column(GTK_ICON_VIEW (prefs_iconview), 1);
	gtk_container_add (GTK_CONTAINER (scrolledwindow1), prefs_iconview);
	
	prefs_data->prefs_notebook = gtk_notebook_new ();
	g_object_set (G_OBJECT (prefs_data->prefs_notebook),"show-border", FALSE,"show-tabs", FALSE,"enable-popup",FALSE,NULL);
	gtk_box_pack_start (GTK_BOX (hbox1), prefs_data->prefs_notebook, TRUE, TRUE, 0);
	GTK_WIDGET_UNSET_FLAGS (prefs_data->prefs_notebook, GTK_CAN_FOCUS);
	g_signal_connect (G_OBJECT (prefs_iconview), "selection-changed",G_CALLBACK (xa_prefs_iconview_changed), prefs_data);

	/* Behaviour page*/
	frame1 = gtk_frame_new (NULL);
	gtk_container_add (GTK_CONTAINER (prefs_data->prefs_notebook), frame1);
	gtk_frame_set_shadow_type (GTK_FRAME (frame1), GTK_SHADOW_NONE);

	alignment1 = gtk_alignment_new (0.5, 0.5, 1, 1);
	gtk_container_add (GTK_CONTAINER (frame1), alignment1);
	gtk_alignment_set_padding (GTK_ALIGNMENT (alignment1), 0, 0, 12, 0);

	vbox4 = gtk_vbox_new (FALSE, 2);
	gtk_container_add (GTK_CONTAINER (alignment1), vbox4);

	hbox1 = gtk_hbox_new (FALSE, 5);
	gtk_box_pack_start (GTK_BOX (vbox4), hbox1, FALSE, TRUE, 0);

	label4 = gtk_label_new (_("Preferred format for new archives:"));
	gtk_box_pack_start (GTK_BOX (hbox1), label4, FALSE, FALSE, 0);

	prefs_data->combo_prefered_format = gtk_combo_box_new_text();
	gtk_box_pack_start (GTK_BOX (hbox1), prefs_data->combo_prefered_format, FALSE, TRUE, 0);
	archive_type = g_list_first ( ArchiveType );
	while ( archive_type != NULL )
	{
		if (archive_type->data == "tgz" || archive_type->data == "rpm" || archive_type->data == "iso" || archive_type->data == "gz" || archive_type->data == "bz2" || (archive_type->data == "rar" && unrar) )
			goto next;
		else
			gtk_combo_box_append_text (GTK_COMBO_BOX (prefs_data->combo_prefered_format),archive_type->data );
		next:
			archive_type = g_list_next ( archive_type );
	}

	prefs_data->check_save_add_dialog = gtk_check_button_new_with_mnemonic (_("Save settings for add dialog"));
	gtk_box_pack_start (GTK_BOX (vbox4), prefs_data->check_save_add_dialog, FALSE, FALSE, 0);
	gtk_button_set_focus_on_click (GTK_BUTTON (prefs_data->check_save_add_dialog), FALSE);

	prefs_data->check_save_extract_dialog = gtk_check_button_new_with_mnemonic (_("Save settings for extract dialog"));
	gtk_box_pack_start (GTK_BOX (vbox4), prefs_data->check_save_extract_dialog, FALSE, FALSE, 0);
	gtk_button_set_focus_on_click (GTK_BUTTON (prefs_data->check_save_extract_dialog), FALSE);

	prefs_data->allow_dir_extract_with_dnd = gtk_check_button_new_with_mnemonic (_("Allow extracting directories by drag and drop"));
	gtk_box_pack_start (GTK_BOX (vbox4), prefs_data->allow_dir_extract_with_dnd, FALSE, FALSE, 0);
	gtk_button_set_focus_on_click (GTK_BUTTON (prefs_data->allow_dir_extract_with_dnd), FALSE);

	prefs_data->confirm_deletion = gtk_check_button_new_with_mnemonic (_("Confirm deletion of files inside the archive"));
	gtk_box_pack_start (GTK_BOX (vbox4), prefs_data->confirm_deletion, FALSE, FALSE, 0);
	gtk_button_set_focus_on_click (GTK_BUTTON (prefs_data->confirm_deletion), FALSE);

	label1 = gtk_label_new ("");
	gtk_notebook_set_tab_label (GTK_NOTEBOOK (prefs_data->prefs_notebook), gtk_notebook_get_nth_page (GTK_NOTEBOOK (prefs_data->prefs_notebook), 0), label1);

	/* View page*/
	frame2 = gtk_frame_new (NULL);
	gtk_container_add (GTK_CONTAINER (prefs_data->prefs_notebook), frame2);
	gtk_frame_set_shadow_type (GTK_FRAME (frame2), GTK_SHADOW_NONE);

	alignment2 = gtk_alignment_new (0.5, 0.5, 1, 1);
	gtk_container_add (GTK_CONTAINER (frame2), alignment2);
	gtk_alignment_set_padding (GTK_ALIGNMENT (alignment2), 0, 0, 12, 0);

	vbox2 = gtk_vbox_new (FALSE, 2);
	gtk_container_add (GTK_CONTAINER (alignment2), vbox2);

	hbox3 = gtk_hbox_new (FALSE, 5);
	gtk_box_pack_start (GTK_BOX (vbox2), hbox3, FALSE, TRUE, 0);

	label5 = gtk_label_new (_("View archive content as"));
	gtk_box_pack_start (GTK_BOX (hbox3), label5, FALSE, FALSE, 0);

	prefs_data->combo_archive_view = gtk_combo_box_new_text();
	gtk_box_pack_start (GTK_BOX (hbox3), prefs_data->combo_archive_view, FALSE, TRUE, 0);
	gtk_combo_box_append_text (GTK_COMBO_BOX (prefs_data->combo_archive_view), _("list") );
	gtk_combo_box_append_text (GTK_COMBO_BOX (prefs_data->combo_archive_view), _("icon") );
	
	hbox7 = gtk_hbox_new (FALSE, 5);
	gtk_box_pack_start (GTK_BOX (vbox2), hbox7, FALSE, TRUE, 0);
	
	label9 = gtk_label_new (_("Size of the mimetype icons"));
	gtk_box_pack_start (GTK_BOX (hbox7), label9, FALSE, FALSE, 0);

	prefs_data->combo_icon_size = gtk_combo_box_new_text();
	gtk_box_pack_start (GTK_BOX (hbox7), prefs_data->combo_icon_size, FALSE, TRUE, 0);
	gtk_combo_box_append_text (GTK_COMBO_BOX (prefs_data->combo_icon_size), _("Large") );
	gtk_combo_box_append_text (GTK_COMBO_BOX (prefs_data->combo_icon_size), _("Small") );

	prefs_data->check_show_comment = gtk_check_button_new_with_mnemonic (_("Show archive comment after loading it"));
	gtk_box_pack_start (GTK_BOX (vbox2), prefs_data->check_show_comment, FALSE, FALSE, 0);
	gtk_button_set_focus_on_click (GTK_BUTTON (prefs_data->check_show_comment), FALSE);

	prefs_data->check_show_iso_info = gtk_check_button_new_with_mnemonic (_("Show ISO info after loading the image"));
	gtk_box_pack_start (GTK_BOX (vbox2), prefs_data->check_show_iso_info, FALSE, FALSE, 0);
	gtk_button_set_focus_on_click (GTK_BUTTON (prefs_data->check_show_iso_info), FALSE);

	prefs_data->check_sort_filename_column = gtk_check_button_new_with_mnemonic (_("Sort content by filename"));
	gtk_box_pack_start (GTK_BOX (vbox2), prefs_data->check_sort_filename_column, FALSE, FALSE, 0);
	gtk_button_set_focus_on_click (GTK_BUTTON (prefs_data->check_sort_filename_column), FALSE);
	gtk_tooltips_set_tip(tooltips, prefs_data->check_sort_filename_column, _("The filename column is sort after loading the archive"), NULL);

	prefs_data->show_location_bar = gtk_check_button_new_with_mnemonic (_("Show location bar"));
	gtk_box_pack_start (GTK_BOX (vbox2), prefs_data->show_location_bar, FALSE, FALSE, 0);
	gtk_button_set_focus_on_click (GTK_BUTTON (prefs_data->show_location_bar), FALSE);

	label2 = gtk_label_new ("");
	gtk_notebook_set_tab_label (GTK_NOTEBOOK (prefs_data->prefs_notebook), gtk_notebook_get_nth_page (GTK_NOTEBOOK (prefs_data->prefs_notebook), 1), label2);

	/* Advanced page*/
	frame3 = gtk_frame_new (NULL);
	gtk_container_add (GTK_CONTAINER (prefs_data->prefs_notebook), frame3);
	gtk_frame_set_shadow_type (GTK_FRAME (frame3), GTK_SHADOW_NONE);

	alignment3 = gtk_alignment_new (0.5, 0.5, 1, 1);
	gtk_container_add (GTK_CONTAINER (frame3), alignment3);
	gtk_alignment_set_padding (GTK_ALIGNMENT (alignment3), 0, 0, 12, 0);

	vbox3 = gtk_vbox_new (FALSE, 2);
	gtk_container_add (GTK_CONTAINER (alignment3), vbox3);

	hbox4 = gtk_hbox_new (FALSE, 5);
	gtk_box_pack_start (GTK_BOX (vbox3), hbox4, FALSE, TRUE, 0);

	label6 = gtk_label_new (_("View HTML help with:"));
	gtk_box_pack_start (GTK_BOX (hbox4), label6, FALSE, FALSE, 0);

	prefs_data->combo_prefered_web_browser = gtk_combo_box_new_text();
	gtk_box_pack_start (GTK_BOX (hbox4), prefs_data->combo_prefered_web_browser, FALSE, TRUE, 0);
	gtk_combo_box_append_text (GTK_COMBO_BOX (prefs_data->combo_prefered_web_browser), _("Firefox") );
	gtk_combo_box_append_text (GTK_COMBO_BOX (prefs_data->combo_prefered_web_browser), _("choose...") );

	hbox5 = gtk_hbox_new (FALSE, 5);
	gtk_box_pack_start (GTK_BOX (vbox3), hbox5, FALSE, TRUE, 0);

	label7 = gtk_label_new (_("Open text files with:"));
	gtk_box_pack_start (GTK_BOX (hbox5), label7, FALSE, FALSE, 0);

	prefs_data->combo_prefered_editor = gtk_combo_box_new_text();
	gtk_box_pack_start (GTK_BOX (hbox5), prefs_data->combo_prefered_editor, FALSE, TRUE, 0);
	gtk_combo_box_append_text (GTK_COMBO_BOX (prefs_data->combo_prefered_editor), _("choose...") );
	
	hbox6 = gtk_hbox_new (FALSE, 5);
	gtk_box_pack_start (GTK_BOX (vbox3), hbox6, FALSE, TRUE, 0);

	label8 = gtk_label_new (_("Preferred temp directory:"));
	gtk_box_pack_start (GTK_BOX (hbox6), label8, FALSE, FALSE, 0);
	
	prefs_data->combo_prefered_temp_dir = gtk_combo_box_new_text();
	gtk_box_pack_start (GTK_BOX (hbox6), prefs_data->combo_prefered_temp_dir, FALSE, TRUE, 0);
	gtk_combo_box_append_text (GTK_COMBO_BOX (prefs_data->combo_prefered_temp_dir), _("/tmp") );
	gtk_combo_box_append_text (GTK_COMBO_BOX (prefs_data->combo_prefered_temp_dir), _("choose...") );

	prefs_data->check_save_geometry = gtk_check_button_new_with_mnemonic (_("Save window geometry on exit"));
	gtk_box_pack_start (GTK_BOX (vbox3), prefs_data->check_save_geometry, FALSE, FALSE, 0);
	gtk_button_set_focus_on_click (GTK_BUTTON (prefs_data->check_save_geometry), FALSE);

	label3 = gtk_label_new ("");
	gtk_notebook_set_tab_label (GTK_NOTEBOOK (prefs_data->prefs_notebook), gtk_notebook_get_nth_page (GTK_NOTEBOOK (prefs_data->prefs_notebook), 2), label3);
	return prefs_data;
}

void xa_prefs_iconview_changed (GtkIconView *iconview, gpointer data)
{
	Prefs_dialog_data *prefs = data;
	GList *list;
	GtkTreePath *path;
	GtkTreeIter iter;
	guint column = 0;

	list = gtk_icon_view_get_selected_items (iconview);
	if (list == NULL)
		return;

	list = g_list_first (list);
	path = (GtkTreePath*)list->data;

	gtk_tree_model_get_iter ( GTK_TREE_MODEL(prefs->prefs_liststore), &iter, path );
	gtk_tree_model_get ( GTK_TREE_MODEL(prefs->prefs_liststore), &iter, 2, &column, -1);

	gtk_tree_path_free( (GtkTreePath*)list->data );
	g_list_free (list);

	if (column == 0)
		gtk_notebook_set_current_page (GTK_NOTEBOOK(prefs->prefs_notebook),0);
	else if (column == 1)
		gtk_notebook_set_current_page (GTK_NOTEBOOK(prefs->prefs_notebook),1);
	else if (column == 2)
		gtk_notebook_set_current_page (GTK_NOTEBOOK(prefs->prefs_notebook),2);
}

void xa_prefs_dialog_set_default_options(Prefs_dialog_data *prefs_data)
{
	gtk_combo_box_set_active (GTK_COMBO_BOX(prefs_data->combo_prefered_format),2);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (prefs_data->confirm_deletion),TRUE);

	gtk_combo_box_set_active (GTK_COMBO_BOX(prefs_data->combo_archive_view),0);
	gtk_combo_box_set_active (GTK_COMBO_BOX(prefs_data->combo_icon_size),0);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (prefs_data->show_location_bar),TRUE);
	
	gtk_combo_box_set_active (GTK_COMBO_BOX(prefs_data->combo_prefered_web_browser),0);
	gtk_combo_box_set_active (GTK_COMBO_BOX(prefs_data->combo_prefered_temp_dir),0);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (prefs_data->check_save_geometry),TRUE);	
}

void xa_prefs_save_options(Prefs_dialog_data *prefs_data, const char *filename)
{
	// leggi i valori dei widget e salva
	
}

void xa_prefs_load_options(Prefs_dialog_data *prefs_data)
{
	GKeyFile *xa_key_file = NULL;
	gchar *config_dir = NULL;
	gchar *xarchiver_config_dir = NULL;
	gchar *config_file = NULL;

	xa_key_file = g_key_file_new();
	config_dir = g_strconcat (g_get_home_dir(),"/.config",NULL);
	if (g_file_test(config_dir, G_FILE_TEST_EXISTS) == FALSE)
		g_mkdir_with_parents(config_dir,0600);
					
	xarchiver_config_dir = g_strconcat (config_dir,"/xarchiver",NULL);
	g_free (config_dir);
	if (g_file_test(xarchiver_config_dir, G_FILE_TEST_EXISTS) == FALSE)
		g_mkdir_with_parents(xarchiver_config_dir,0600);

	config_file = g_strconcat (xarchiver_config_dir,"/xarchiverrc",NULL);
	g_free (xarchiver_config_dir);
		
	if ( ! g_key_file_load_from_file(xa_key_file,config_file,G_KEY_FILE_KEEP_COMMENTS,NULL) )
	{
		g_print ("Chiamo il default\n");
		xa_prefs_dialog_set_default_options(prefs_data);
		xa_prefs_save_options(prefs_data,config_file);
	}
	else
		//imposta i widget con i valori caricati
	g_key_file_free(xa_key_file);
}

