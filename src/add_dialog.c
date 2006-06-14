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
	unsigned short int default_value, max_value;
	default_value = max_value = 0;
	gchar *compression_msg = NULL;

	add_dialog = g_new0 (Add_dialog_data, 1);
	add_dialog->file_dir_radio_group = NULL;
	add_dialog->option_tooltip = gtk_tooltips_new ();

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

	add_dialog->file_liststore = gtk_list_store_new (2, G_TYPE_STRING, G_TYPE_STRING);

	add_dialog->file_list_treeview = gtk_tree_view_new_with_model(GTK_TREE_MODEL(add_dialog->file_liststore));
	add_dialog->column = gtk_tree_view_column_new();

	add_dialog->selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(add_dialog->file_list_treeview) );
	gtk_tree_selection_set_mode(add_dialog->selection, GTK_SELECTION_MULTIPLE);

	add_dialog->renderer = gtk_cell_renderer_pixbuf_new();
	gtk_tree_view_column_pack_start (add_dialog->column, add_dialog->renderer, FALSE);
	gtk_tree_view_column_add_attribute (add_dialog->column, add_dialog->renderer, "stock-id", 0);
	
	add_dialog->renderer = gtk_cell_renderer_text_new();
	gtk_tree_view_column_pack_start(add_dialog->column, add_dialog->renderer, TRUE);
	gtk_tree_view_column_add_attribute(add_dialog->column, add_dialog->renderer, "text", 1);
	
	gtk_tree_view_append_column(GTK_TREE_VIEW(add_dialog->file_list_treeview), add_dialog->column);
	
	gtk_tree_view_set_rules_hint(GTK_TREE_VIEW(add_dialog->file_list_treeview), TRUE);
	gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (add_dialog->file_list_treeview), FALSE);
	gtk_container_add (GTK_CONTAINER (add_dialog->scrolledwindow3), add_dialog->file_list_treeview);
	gtk_widget_show (add_dialog->file_list_treeview);
	
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
	g_signal_connect ( (gpointer) add_dialog->remove_button, "clicked", G_CALLBACK (remove_files_liststore) , add_dialog );

	add_dialog->add_files_button = gtk_button_new_from_stock ("gtk-add");
	gtk_widget_show (add_dialog->add_files_button);
	gtk_tooltips_set_tip (add_dialog->add_option_tooltip,add_dialog->add_files_button , _("Choose the files or directories to add to the archive"), NULL );
	gtk_container_add (GTK_CONTAINER (add_dialog->hbuttonbox2), add_dialog->add_files_button);
	GTK_WIDGET_SET_FLAGS (add_dialog->add_files_button, GTK_CAN_DEFAULT);
	g_signal_connect ( (gpointer) add_dialog->add_files_button, "clicked", G_CALLBACK (xa_select_files_to_add) , add_dialog );
	
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

	add_dialog->recurse = gtk_check_button_new_with_mnemonic (_("Recurse subdirectories"));
	gtk_widget_show (add_dialog->recurse);
	gtk_box_pack_start (GTK_BOX (add_dialog->vbox6), add_dialog->recurse, FALSE, FALSE, 0);

	add_dialog->add_full_path = gtk_check_button_new_with_mnemonic (_("Do not add file paths"));
	gtk_widget_show (add_dialog->add_full_path);
	gtk_box_pack_start (GTK_BOX (add_dialog->vbox6), add_dialog->add_full_path, FALSE, FALSE, 0);
	gtk_tooltips_set_tip (add_dialog->option_tooltip,add_dialog->add_full_path , _("Store just the name of a saved file (junk the path), and do not directory names."), NULL);

	if (archive->type == XARCHIVETYPE_ZIP || archive->type == XARCHIVETYPE_RAR || archive->type == XARCHIVETYPE_ARJ || archive->type == XARCHIVETYPE_7ZIP )
	{
		if (archive->type != XARCHIVETYPE_7ZIP)
		{
			add_dialog->freshen = gtk_check_button_new_with_mnemonic (_("Freshen an existing entry in the archive"));
			gtk_widget_show (add_dialog->freshen);
			gtk_box_pack_start (GTK_BOX (add_dialog->vbox6), add_dialog->freshen, FALSE, FALSE, 0);
			gtk_tooltips_set_tip (add_dialog->option_tooltip,add_dialog->freshen , _("This options affects the archive only if it has been modified more recently than the version already in the zip archive; unlike the update option it will not add files that are not already in the zip archive."), NULL );
			g_signal_connect (G_OBJECT (add_dialog->freshen),"toggled",G_CALLBACK (add_fresh_update_toggled_cb) , add_dialog);
		}
		
		add_dialog->update = gtk_check_button_new_with_mnemonic (_("Update an existing entry in the archive"));
		gtk_widget_show (add_dialog->update);
		gtk_box_pack_start (GTK_BOX (add_dialog->vbox6), add_dialog->update, FALSE, FALSE, 0);
		gtk_tooltips_set_tip (add_dialog->option_tooltip,add_dialog->update, _("This option will add any new files and update any files which have been modified since the archive was last created/modified."), NULL );
		if (archive->type != XARCHIVETYPE_7ZIP)
			g_signal_connect (G_OBJECT (add_dialog->update),"toggled",G_CALLBACK (add_update_fresh_toggled_cb) , add_dialog);

		add_dialog->hbox2 = gtk_hbox_new (FALSE, 6);
		gtk_widget_show (add_dialog->hbox2);
		gtk_box_pack_start (GTK_BOX (add_dialog->vbox6), add_dialog->hbox2, TRUE, TRUE, 0);

		if (archive->type == XARCHIVETYPE_7ZIP)
		{
			compression_msg = _("0 = no compression, 5 is default, 9 = best compression but slowest");
			default_value = 5;
			max_value = 9;
		}
		
		else if (archive->type == XARCHIVETYPE_ZIP)
		{
			compression_msg = _("0 = no compression, 6 is default, 9 = best compression but slowest");
			default_value = 6;
			max_value = 9;
		}

		else if (archive->type == XARCHIVETYPE_RAR)
		{
			compression_msg = _("0 = no compression, 3 is default, 5 = best compression but slowest");
			default_value = 3;
			max_value = 5;
		}

		else if (archive->type == XARCHIVETYPE_ARJ)
		{
			compression_msg = _("0 = no compression, 1 is default, 4 = fastest but least compression.");
			default_value = 1;
			max_value = 4;
		}

		add_dialog->label4 = gtk_label_new (_("Compression level:"));
		gtk_widget_show (add_dialog->label4);
		gtk_box_pack_start (GTK_BOX (add_dialog->hbox2), add_dialog->label4, FALSE, FALSE, 0);
	
		if (archive->type == XARCHIVETYPE_7ZIP)
			add_dialog->compression_value = gtk_adjustment_new (default_value, 0, max_value, 2, 2, 0);
		else
			add_dialog->compression_value = gtk_adjustment_new (default_value, 0, max_value, 0, 0, 0);

		add_dialog->compression_scale = gtk_hscale_new ( GTK_ADJUSTMENT (add_dialog->compression_value) );
		gtk_widget_show (add_dialog->compression_scale);
		gtk_box_pack_start (GTK_BOX (add_dialog->hbox2), add_dialog->compression_scale, TRUE, TRUE, 0);
		gtk_scale_set_value_pos (GTK_SCALE (add_dialog->compression_scale), GTK_POS_LEFT);
		gtk_scale_set_digits (GTK_SCALE (add_dialog->compression_scale), 0);
		if (archive->type == XARCHIVETYPE_ARJ)
		{
			gtk_range_set_inverted (GTK_RANGE (add_dialog->compression_scale), TRUE);
			gtk_tooltips_set_tip (add_dialog->option_tooltip,add_dialog->compression_scale, compression_msg, NULL );
		}
		else if (archive->type == XARCHIVETYPE_7ZIP)
			g_signal_connect (G_OBJECT (add_dialog->compression_value),"value-changed",G_CALLBACK (fix_adjustment_value), NULL);
	}

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

void add_fresh_update_toggled_cb (GtkToggleButton *button, Add_dialog_data *data)
{
	gboolean active = gtk_toggle_button_get_active (button);
	if (active)
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (data->update), FALSE);
}

void add_update_fresh_toggled_cb (GtkToggleButton *button, Add_dialog_data *data)
{
	if (data->freshen == NULL)
		return;
	gboolean active = gtk_toggle_button_get_active (button);
	if (active)
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (data->freshen), FALSE);
}

gchar *xa_parse_add_dialog_options ( XArchive *archive , Add_dialog_data *add_dialog, GtkTreeSelection *selection)
{
	gchar *command;
	return command;
}

void xa_select_files_to_add ( GtkButton* button , gpointer _add_dialog )
{
	Add_dialog_data *add_dialog = _add_dialog;
	GSList *dummy = NULL;
	unsigned short int flag;

	if ( gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(add_dialog->files_radio)) )
	{
		title = _("Please select the files you want to add");
		flag = GTK_FILE_CHOOSER_ACTION_OPEN;
	}
	else
	{
		title = _("Please select the directories you want to add");
		flag = GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER;
	}
	
	File_Selector = gtk_file_chooser_dialog_new ( title,
							GTK_WINDOW (MainWindow),
							flag,
							GTK_STOCK_CANCEL,
							GTK_RESPONSE_CANCEL,
							GTK_STOCK_OPEN,
							GTK_RESPONSE_ACCEPT,
							NULL);
	gtk_file_chooser_set_select_multiple ( GTK_FILE_CHOOSER (File_Selector) , TRUE );
	response = gtk_dialog_run (GTK_DIALOG (File_Selector));
	if (response == GTK_RESPONSE_ACCEPT)
	{
		dummy = gtk_file_chooser_get_filenames (GTK_FILE_CHOOSER (File_Selector));
		g_slist_foreach( dummy, (GFunc) add_files_liststore, add_dialog->file_liststore);
	}

	if (dummy != NULL)
		g_slist_free (dummy);
	gtk_widget_destroy ( File_Selector );
	return;
}

/* Code from xarchive - http://xarchive.sourceforge.net */
void add_files_liststore (gchar *file_path, GtkListStore *liststore)
{
	GtkTreeIter iter;
	gchar *icon = GTK_STOCK_FILE;
	gchar *file_utf8;

	file_utf8 = g_filename_display_name (file_path);
	if ( g_file_test(file_path, G_FILE_TEST_IS_DIR) == TRUE )
		icon = GTK_STOCK_DIRECTORY;

	gtk_list_store_append(liststore, &iter);
	gtk_list_store_set (liststore, &iter, 0, icon, 1, file_utf8, -1);
	g_free (file_utf8);
}

void remove_files_liststore (GtkWidget *widget, gpointer data)
{
	Add_dialog_data *add_dialog = data;
	GtkTreeSelection *sel;
	GtkTreePath *path;
	GtkTreeIter iter;
	GList *rr_list = NULL;
	GList *node;

	sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(add_dialog->file_list_treeview) );
	gtk_tree_selection_selected_foreach(sel, (GtkTreeSelectionForeachFunc) remove_foreach_func, &rr_list);

	for (node = rr_list; node != NULL; node = node->next)
	{
		path = gtk_tree_row_reference_get_path((GtkTreeRowReference *) node->data);
		if (path)
	    {
			if ( gtk_tree_model_get_iter(GTK_TREE_MODEL(add_dialog->file_liststore), &iter, path) )
				gtk_list_store_remove(add_dialog->file_liststore, &iter);
			gtk_tree_path_free(path);
		}
	}

	g_list_foreach(rr_list, (GFunc) gtk_tree_row_reference_free, NULL);
	g_list_free(rr_list);
}

void remove_foreach_func (GtkTreeModel *model, GtkTreePath *path, GtkTreeIter *iter, GList **rowref_list)
{
	GtkTreeRowReference *rowref;

	rowref = gtk_tree_row_reference_new(model, path);
	*rowref_list = g_list_append(*rowref_list, rowref);
}

/* End code from xarchive */

void fix_adjustment_value (GtkAdjustment *adjustment, gpointer user_data)
{
	unsigned short int digit = gtk_adjustment_get_value (adjustment);
	if (digit & 1) 
		return;
	else
		gtk_adjustment_set_value (adjustment, digit-1);
}

