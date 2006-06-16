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
	gtk_window_set_resizable (GTK_WINDOW (add_dialog->dialog1), FALSE);

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
	
	add_dialog->hbox1 = gtk_hbox_new (FALSE, 70);
	gtk_widget_show (add_dialog->hbox1);
	gtk_box_pack_start (GTK_BOX (add_dialog->vbox7), add_dialog->hbox1, TRUE, TRUE, 0);

	add_dialog->vbox8 = gtk_vbox_new (FALSE, 0);
	gtk_widget_show (add_dialog->vbox8);
	gtk_box_pack_start (GTK_BOX (add_dialog->hbox1), add_dialog->vbox8, TRUE, FALSE, 0);

	add_dialog->files_radio = gtk_radio_button_new_with_mnemonic (NULL, _("Files"));
	gtk_widget_show (add_dialog->files_radio);
	gtk_box_pack_start (GTK_BOX (add_dialog->vbox8), add_dialog->files_radio, FALSE, FALSE, 0);
	gtk_button_set_focus_on_click (GTK_BUTTON (add_dialog->files_radio), FALSE);
	gtk_radio_button_set_group (GTK_RADIO_BUTTON (add_dialog->files_radio), add_dialog->file_dir_radio_group);
	add_dialog->file_dir_radio_group = gtk_radio_button_get_group (GTK_RADIO_BUTTON (add_dialog->files_radio));

	add_dialog->directories_radio = gtk_radio_button_new_with_mnemonic (NULL, _("Directories"));
	gtk_widget_show (add_dialog->directories_radio);
	gtk_box_pack_start (GTK_BOX (add_dialog->vbox8), add_dialog->directories_radio, FALSE, FALSE, 0);
	gtk_button_set_focus_on_click (GTK_BUTTON (add_dialog->directories_radio), FALSE);
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

	if ( (archive->type == XARCHIVETYPE_RAR) || (archive->type == XARCHIVETYPE_7ZIP && archive->nr_of_files == 0 && archive->nr_of_dirs == 0))
	{
		add_dialog->solid_archive = gtk_check_button_new_with_mnemonic (_("Generate a solid archive"));
		gtk_widget_show (add_dialog->solid_archive);
		gtk_box_pack_start (GTK_BOX (add_dialog->vbox6), add_dialog->solid_archive, FALSE, FALSE, 0);
		gtk_tooltips_set_tip (add_dialog->option_tooltip,add_dialog->solid_archive , _("In a solid archive the files are grouped together featuring a better compression ratio."), NULL);
	}

	if (archive->type == XARCHIVETYPE_TAR || archive->type == XARCHIVETYPE_TAR_GZ || archive->type == XARCHIVETYPE_TAR_BZ2 || archive->type == XARCHIVETYPE_RAR || archive->type == XARCHIVETYPE_ARJ || archive->type == XARCHIVETYPE_ZIP)
	{
		add_dialog->remove_files = gtk_check_button_new_with_mnemonic (_("Remove files after adding"));
		gtk_widget_show (add_dialog->remove_files);
		gtk_box_pack_start (GTK_BOX (add_dialog->vbox6), add_dialog->remove_files, FALSE, FALSE, 0);
	}

	if (archive->type != XARCHIVETYPE_7ZIP && archive->type != XARCHIVETYPE_TAR && archive->type != XARCHIVETYPE_TAR_GZ && archive->type != XARCHIVETYPE_TAR_BZ2)
	{
		add_dialog->add_full_path = gtk_check_button_new_with_mnemonic (_("Do not add file paths"));
		gtk_widget_show (add_dialog->add_full_path);
		gtk_box_pack_start (GTK_BOX (add_dialog->vbox6), add_dialog->add_full_path, FALSE, FALSE, 0);
		gtk_tooltips_set_tip (add_dialog->option_tooltip,add_dialog->add_full_path , _("Store just the name of a file without its directory names."), NULL);

		add_dialog->freshen = gtk_check_button_new_with_mnemonic (_("Freshen an existing entry in the archive"));
		gtk_widget_show (add_dialog->freshen);
		gtk_box_pack_start (GTK_BOX (add_dialog->vbox6), add_dialog->freshen, FALSE, FALSE, 0);
		gtk_tooltips_set_tip (add_dialog->option_tooltip,add_dialog->freshen , _("This options affects the archive only if it has been modified more recently than the version already in the archive; unlike the update option it will not add files that are not already in the archive."), NULL );
		g_signal_connect (G_OBJECT (add_dialog->freshen),"toggled",G_CALLBACK (add_fresh_update_toggled_cb) , add_dialog);
	}
		
	add_dialog->update = gtk_check_button_new_with_mnemonic (_("Update an existing entry in the archive"));
	gtk_widget_show (add_dialog->update);
	gtk_box_pack_start (GTK_BOX (add_dialog->vbox6), add_dialog->update, FALSE, FALSE, 0);
	gtk_tooltips_set_tip (add_dialog->option_tooltip,add_dialog->update, _("This option will add any new files and update any files which have been modified since the archive was last created/modified."), NULL );

	if (archive->type != XARCHIVETYPE_7ZIP)
		g_signal_connect (G_OBJECT (add_dialog->update),"toggled",G_CALLBACK (add_update_fresh_toggled_cb) , add_dialog);

	if (archive->type != XARCHIVETYPE_TAR && archive->type != XARCHIVETYPE_TAR_GZ && archive->type != XARCHIVETYPE_TAR_BZ2)
	{
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

		add_dialog->hbox3 = gtk_hbox_new (FALSE, 0);
		gtk_widget_show (add_dialog->hbox3);
		gtk_box_pack_start (GTK_BOX (add_dialog->vbox6), add_dialog->hbox3, TRUE, TRUE, 0);

		add_dialog->add_password = gtk_check_button_new_with_mnemonic (_("Password:"));
		gtk_widget_show (add_dialog->add_password);
		gtk_box_pack_start (GTK_BOX (add_dialog->hbox3), add_dialog->add_password, FALSE, FALSE, 0);
		g_signal_connect (G_OBJECT (add_dialog->add_password), "toggled",G_CALLBACK (password_toggled_cb) , add_dialog);

		add_dialog->add_password_entry = gtk_entry_new ();
		gtk_widget_show (add_dialog->add_password_entry);
		gtk_box_pack_start (GTK_BOX (add_dialog->hbox3), add_dialog->add_password_entry, FALSE, FALSE, 0);
		gtk_entry_set_visibility (GTK_ENTRY (add_dialog->add_password_entry), FALSE);
		gtk_widget_set_sensitive (add_dialog->add_password_entry, FALSE);
			
		add_dialog->hbox2 = gtk_hbox_new (FALSE, 6);
		gtk_widget_show (add_dialog->hbox2);
		gtk_box_pack_start (GTK_BOX (add_dialog->vbox6), add_dialog->hbox2, TRUE, TRUE, 0);
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
			gtk_range_set_inverted (GTK_RANGE (add_dialog->compression_scale), TRUE);
		else if (archive->type == XARCHIVETYPE_7ZIP)
			g_signal_connect (G_OBJECT (add_dialog->compression_value),"value-changed",G_CALLBACK (fix_adjustment_value), NULL);
		gtk_tooltips_set_tip (add_dialog->option_tooltip,add_dialog->compression_scale, compression_msg, NULL );
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
	gtk_dialog_set_default_response (GTK_DIALOG (add_dialog->dialog1), GTK_RESPONSE_OK);
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

void password_toggled_cb ( GtkButton* button , gpointer _add_dialog )
{
	Add_dialog_data *add_dialog = _add_dialog;
	if ( gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(add_dialog->add_password)) )
	{
		gtk_widget_set_sensitive (add_dialog->add_password_entry, TRUE);
		gtk_widget_grab_focus (add_dialog->add_password_entry);
	}
	else
		gtk_widget_set_sensitive (add_dialog->add_password_entry, FALSE);
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

gchar *xa_parse_add_dialog_options ( XArchive *archive , Add_dialog_data *add_dialog )
{
	GtkTreeIter iter;
	gchar *command = NULL;
	gchar *temp_password = NULL;
	gchar *compression_string = NULL;
	gchar *first_item = NULL;
	gboolean done = FALSE;

	while ( ! done )
	{
		switch (gtk_dialog_run ( GTK_DIALOG (add_dialog->dialog1 ) ) )
		{
			case GTK_RESPONSE_CANCEL:
			case GTK_RESPONSE_DELETE_EVENT:
			done = TRUE;
			break;

			case GTK_RESPONSE_OK:
			if (gtk_tree_model_get_iter_first(GTK_TREE_MODEL(add_dialog->file_liststore), &iter) == FALSE)
			{
				response = ShowGtkMessageDialog (GTK_WINDOW (MainWindow),GTK_DIALOG_MODAL,GTK_MESSAGE_ERROR,GTK_BUTTONS_OK, _("You haven't selected any files to add!") );
				break;
			}
			if ( add_dialog->add_password != NULL && gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(add_dialog->add_password)) )
			{
				temp_password  = g_strdup (gtk_entry_get_text ( GTK_ENTRY (add_dialog->add_password_entry) ));
				if (strlen(temp_password) == 0)
				{
					response = ShowGtkMessageDialog (GTK_WINDOW (MainWindow),GTK_DIALOG_MODAL,GTK_MESSAGE_ERROR,GTK_BUTTONS_OK, _("Please enter the password!") );
					g_free (temp_password);
					break;
				}
				else
					archive->passwd = temp_password;
			}
			done = TRUE;
			archive->add_recurse = gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON ( add_dialog->recurse ));
			if (add_dialog->add_full_path != NULL)
				archive->full_path = gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON ( add_dialog->add_full_path ));
			
			archive->update = gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON ( add_dialog->update ));
			
			if (add_dialog->remove_files != NULL)
				archive->remove_files = gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON ( add_dialog->remove_files ));

			if (add_dialog->freshen != NULL)
				archive->freshen = gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON ( add_dialog->freshen ));
			
			if (add_dialog->solid_archive)
				archive->solid_archive = gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON ( add_dialog->solid_archive ));

			if (add_dialog->compression_scale != NULL)
			{
				archive->compression_level = gtk_adjustment_get_value (GTK_ADJUSTMENT ( add_dialog->compression_value ));
				compression_string = g_strdup_printf ( "%d",archive->compression_level );
			}

			//Set the current dir so to avoid archiving the leading directory inside the archive
			gtk_tree_model_get_iter_first(GTK_TREE_MODEL(add_dialog->file_liststore), &iter);
			gtk_tree_model_get (GTK_TREE_MODEL(add_dialog->file_liststore), &iter, 1, &first_item, -1);
			gchar *current_dir = g_path_get_dirname ( first_item );
			g_free ( first_item );
			chdir ( current_dir );
			g_free ( current_dir );

			 /* Let's concatenate the files to add */
			names = g_string_new ( " " );
			while (gtk_tree_model_get_iter_first(GTK_TREE_MODEL(add_dialog->file_liststore), &iter) )
			{
				ConcatenateFileNames3 ( GTK_TREE_MODEL(add_dialog->file_liststore), NULL, &iter, names );
				gtk_list_store_remove (add_dialog->file_liststore, &iter);
			}
			gtk_widget_set_sensitive (Stop_button , TRUE);			
			xa_set_button_state (0,0,0,0);
			archive->status = XA_ARCHIVESTATUS_ADD;

			if (archive->type != XARCHIVETYPE_BZIP2 && archive->type != XARCHIVETYPE_GZIP)
				Update_StatusBar ( _("Adding files to the archive, please wait..."));
			switch (archive->type)
			{
				case XARCHIVETYPE_BZIP2:
				Update_StatusBar ( _("Compressing file with bzip2, please wait..."));
				Bzip2Add ( names->str , archive , 0 );
				break;

				case XARCHIVETYPE_GZIP:
				Update_StatusBar ( _("Compressing file with gzip, please wait..."));
				Bzip2Add ( names->str , archive , 1 );
				break;
			
				case XARCHIVETYPE_RAR:
				if (archive->passwd != NULL)
					command = g_strconcat ( "rar a ",
											archive->update ? "-u " : "",
											archive->freshen ? "-f " : "",
											archive->solid_archive ? "-s " : "",
											archive->remove_files ? "-df " : "",
											"-p" , archive->passwd,
											archive->add_recurse ? "-r " : "",
											archive->full_path ? "-ep " : "",
											"-idp ",
											"-m",compression_string," ",
											archive->escaped_path,
											names->str , NULL );
				else
					command = g_strconcat ( "rar a ",
											archive->update ? "-u " : "",
											archive->freshen ? "-f " : "",
											archive->solid_archive ? "-s " : "",
											archive->remove_files ? "-df " : "",
											archive->add_recurse ? "-r " : "",
											archive->full_path ? "-ep " : "",
											"-idp ",
											"-m",compression_string," ",
											archive->escaped_path,
											names->str , NULL );
				break;

				case XARCHIVETYPE_TAR:
				if ( g_file_test ( archive->escaped_path , G_FILE_TEST_EXISTS ) )
					command = g_strconcat ( "tar ",
											archive->add_recurse ? "" : "--no-recursion ",
											archive->remove_files ? "--remove-files " : "",
											archive->update ? "-uvvf " : "-rvvf ",
											archive->escaped_path,
											names->str , NULL );
				else
					command = g_strconcat ( "tar ",
											archive->add_recurse ? "" : "--no-recursion ",
											archive->remove_files ? "--remove-files " : "",
											"-cvvf ",archive->escaped_path,
											names->str , NULL );
				break;

				case XARCHIVETYPE_TAR_BZ2:
				if ( g_file_test ( archive->escaped_path , G_FILE_TEST_EXISTS ) )
					DecompressBzipGzip ( names , archive, 0 , 1 );
				else
					command = g_strconcat ( "tar ",
											archive->add_recurse ? "" : "--no-recursion ",
											archive->remove_files ? "--remove-files " : "",
											"-cvvfj ",archive->escaped_path,
											names->str , NULL );
				break;

				case XARCHIVETYPE_TAR_GZ:
				if ( g_file_test ( archive->escaped_path , G_FILE_TEST_EXISTS ) )
					DecompressBzipGzip ( names , archive, 1 , 1 );
				else
					command = g_strconcat ( "tar ",
											archive->add_recurse ? "" : "--no-recursion ",
											archive->remove_files ? "--remove-files " : "",
											"-cvvfz ",archive->escaped_path,
											names->str , NULL );
				break;

				case XARCHIVETYPE_ZIP:
				if (archive->passwd != NULL)
					command = g_strconcat ( "zip ",
											archive->update ? "-u " : "",
											archive->freshen ? "-f " : "",
											archive->add_recurse ? "-r " : "",
											archive->remove_files ? "-m " : "",
											archive->full_path ? "-j " : "",
											"-P ", archive->passwd," ",
											"-",compression_string," ",
											archive->escaped_path,
											names->str , NULL );
				else
					command = g_strconcat ( "zip ",
											archive->update ? "-u " : "",
											archive->freshen ? "-f " : "",
											archive->add_recurse ? "-r " : "",
											archive->remove_files ? "-m " : "",
											archive->full_path ? "-j " : "",
											"-",compression_string," ",
											archive->escaped_path,
											names->str , NULL );
				break;

				case XARCHIVETYPE_7ZIP:
				if (archive->passwd != NULL)
					command = g_strconcat ( "7za ",
											archive->update ? "u " : "a ",
											archive->solid_archive ? "-ms=on " : "-ms=off ",
											"-p" , archive->passwd, " ",
											archive->escaped_path,
											archive->add_recurse ? " -r " : " ",
											"-mx=",compression_string,"",
											names->str , NULL );
				else
					command = g_strconcat ( "7za ",
											archive->update ? "u " : "a ",
											archive->solid_archive ? "-ms=on " : "-ms=off ",
											archive->escaped_path,
											archive->add_recurse ? " -r " : " ",
											"-mx=",compression_string,"",
											names->str , NULL );
				break;

				case XARCHIVETYPE_ARJ:
				if (archive->passwd != NULL)
					command = g_strconcat ( "arj a ",
											archive->update ? "-u " : "",
											archive->freshen ? "-f " : "",
											archive->add_recurse ? "-r " : "",
											archive->remove_files ? "-d1 " : "",
											archive->full_path ? "-e " : "",
											"-g" , archive->passwd , " -i ",
											"-m",compression_string," ",
											archive->escaped_path,
											names->str , NULL );
				else
					command = g_strconcat ( "arj a ",
											archive->update ? "-u " : "",
											archive->freshen ? "-f " : "",
											archive->add_recurse ? "-r " : "",
											archive->remove_files ? "-d1 " : "",
											archive->full_path ? "-e " : "",
											" -i ",
											"-m",compression_string," ",
											archive->escaped_path,
											names->str , NULL );
				break;

				default:
				command = NULL;            
			}
			if (compression_string != NULL)
				g_free (compression_string);
			g_string_free (names , FALSE );
		}
	}
	return command;
}

