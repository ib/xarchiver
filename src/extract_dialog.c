/*
 *  Copyright (C) 2008 Giuseppe Torelli - <colossus73@gmail.com>
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
#include <stdlib.h>
#include "extract_dialog.h"
#include "window.h"
#include "string_utils.h"
#include "support.h"

extern gboolean unrar;
gchar *rar;

Extract_dialog_data *xa_create_extract_dialog (gint selected,XArchive *archive)
{
	GSList *radiobutton1_group = NULL;
	Extract_dialog_data *dialog_data;
	gboolean flag = TRUE;

	home_dir = g_get_home_dir();
	dialog_data = g_new0 (Extract_dialog_data, 1);
	dialog_data->dialog1 = gtk_dialog_new ();
	if (archive->type == XARCHIVETYPE_BZIP2 || archive->type == XARCHIVETYPE_GZIP || archive->type == XARCHIVETYPE_LZMA)
		gtk_window_set_title (GTK_WINDOW (dialog_data->dialog1), _("Decompress file"));
	else
		gtk_window_set_title (GTK_WINDOW (dialog_data->dialog1), _("Extract files from archive"));
	gtk_window_set_type_hint (GTK_WINDOW (dialog_data->dialog1), GDK_WINDOW_TYPE_HINT_DIALOG);
	gtk_window_set_transient_for (GTK_WINDOW(dialog_data->dialog1),GTK_WINDOW (xa_main_window));
	gtk_dialog_set_has_separator (GTK_DIALOG(dialog_data->dialog1),FALSE);
	gtk_widget_set_size_request (dialog_data->dialog1,492,372);

	option_tooltip = gtk_tooltips_new ();
	dialog_data->dialog_vbox1 = GTK_DIALOG (dialog_data->dialog1)->vbox;

	vbox1 = gtk_vbox_new (FALSE, 2);
	gtk_box_pack_start (GTK_BOX (dialog_data->dialog_vbox1), vbox1, TRUE, TRUE, 0);
	gtk_container_set_border_width (GTK_CONTAINER (vbox1), 2);

	label1 = gtk_label_new (_("Extract to:"));
	gtk_box_pack_start (GTK_BOX (vbox1), label1, FALSE, FALSE, 0);
	gtk_misc_set_alignment (GTK_MISC (label1), 0, 0.5);

	dialog_data->destination_path_entry = gtk_entry_new ();
	gtk_entry_set_text (GTK_ENTRY(dialog_data->destination_path_entry),home_dir);
	gtk_entry_set_editable(GTK_ENTRY(dialog_data->destination_path_entry),FALSE);
	gtk_box_pack_start (GTK_BOX (vbox1), dialog_data->destination_path_entry, FALSE, FALSE, 0);

	hbox1 = gtk_hbox_new (TRUE, 10);
	gtk_box_pack_start (GTK_BOX (vbox1), hbox1, TRUE, TRUE, 0);

	vbox2 = gtk_vbox_new (FALSE, 5);
	gtk_box_pack_start (GTK_BOX (hbox1), vbox2, TRUE, TRUE, 0);

	frame1 = gtk_frame_new (NULL);
	gtk_box_pack_start (GTK_BOX (vbox2), frame1, TRUE, TRUE, 0);
	gtk_frame_set_shadow_type (GTK_FRAME (frame1), GTK_SHADOW_OUT);

	alignment1 = gtk_alignment_new (0.5, 0.5, 1, 1);
	gtk_container_add (GTK_CONTAINER (frame1), alignment1);
	gtk_alignment_set_padding (GTK_ALIGNMENT (alignment1), 0, 0, 12, 0);

	vbox3 = gtk_vbox_new (FALSE, 0);
	gtk_container_add (GTK_CONTAINER (alignment1), vbox3);

	dialog_data->all_files_radio = gtk_radio_button_new_with_mnemonic (NULL, _("All files"));
	gtk_box_pack_start (GTK_BOX (vbox3), dialog_data->all_files_radio, FALSE, FALSE, 0);
	gtk_radio_button_set_group (GTK_RADIO_BUTTON (dialog_data->all_files_radio), radiobutton1_group);
	radiobutton1_group = gtk_radio_button_get_group (GTK_RADIO_BUTTON (dialog_data->all_files_radio));

	dialog_data->selected_radio = gtk_radio_button_new_with_mnemonic (NULL, _("Selected files"));
	gtk_box_pack_start (GTK_BOX (vbox3), dialog_data->selected_radio, FALSE, FALSE, 0);
	gtk_radio_button_set_group (GTK_RADIO_BUTTON (dialog_data->selected_radio), radiobutton1_group);
	radiobutton1_group = gtk_radio_button_get_group (GTK_RADIO_BUTTON (dialog_data->selected_radio));

	if (archive->type != XARCHIVETYPE_RPM)
	{
		if (selected)
			gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (dialog_data->selected_radio),TRUE);
		else
			gtk_widget_set_sensitive (dialog_data->selected_radio,FALSE);
	}
	else
		gtk_widget_set_sensitive (dialog_data->selected_radio,FALSE);

	hbox2 = gtk_hbox_new (FALSE, 0);
	gtk_box_pack_start (GTK_BOX (vbox3), hbox2, FALSE, FALSE, 0);

	dialog_data->files_radio = gtk_radio_button_new_with_mnemonic (NULL, _("Files: "));
	gtk_box_pack_start (GTK_BOX (hbox2), dialog_data->files_radio, FALSE, FALSE, 0);
	gtk_radio_button_set_group (GTK_RADIO_BUTTON (dialog_data->files_radio), radiobutton1_group);
	radiobutton1_group = gtk_radio_button_get_group (GTK_RADIO_BUTTON (dialog_data->files_radio));

	dialog_data->entry2 = gtk_entry_new ();
	gtk_box_pack_start (GTK_BOX (hbox2),dialog_data->entry2, TRUE,TRUE,0);
	gtk_entry_set_width_chars (GTK_ENTRY (dialog_data->entry2),10);
	gtk_widget_set_sensitive(dialog_data->entry2,FALSE);
	g_signal_connect (G_OBJECT (dialog_data->files_radio),"toggled",G_CALLBACK(xa_activate_entry),dialog_data);

	label2 = gtk_label_new (_("Files "));
	gtk_frame_set_label_widget (GTK_FRAME (frame1), label2);

	frame2 = gtk_frame_new (NULL);
	gtk_box_pack_start (GTK_BOX (vbox2),frame2,TRUE,TRUE,0);
	gtk_frame_set_shadow_type (GTK_FRAME (frame2),GTK_SHADOW_OUT);

	alignment2 = gtk_alignment_new (0.5, 0.5, 1, 1);
	gtk_container_add (GTK_CONTAINER (frame2),alignment2);
	gtk_alignment_set_padding (GTK_ALIGNMENT (alignment2),0,0,12,0);

	vbox5 = gtk_vbox_new (FALSE,0);
	gtk_container_add (GTK_CONTAINER (alignment2),vbox5);

	dialog_data->overwrite_check = gtk_check_button_new_with_mnemonic (_("Overwrite existing files"));
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (dialog_data->overwrite_check),archive->overwrite);
	gtk_box_pack_start (GTK_BOX (vbox5), dialog_data->overwrite_check,FALSE,FALSE,0);

	dialog_data->extract_full = gtk_check_button_new_with_mnemonic (_("Extract files with full path"));
	if (archive->type == XARCHIVETYPE_GZIP || archive->type == XARCHIVETYPE_LZMA || archive->type == XARCHIVETYPE_BZIP2 || archive->type == XARCHIVETYPE_RPM)
		flag = FALSE;

	gtk_widget_set_sensitive (dialog_data->extract_full,flag);
	if (archive->type == XARCHIVETYPE_RPM)
		flag = ! flag;

	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (dialog_data->extract_full),flag);
	gtk_tooltips_set_tip(option_tooltip,dialog_data->extract_full , _("The archive's directory structure is recreated in the extraction directory."), NULL );
	gtk_box_pack_start (GTK_BOX (vbox5), dialog_data->extract_full, FALSE, FALSE, 0);

	if (archive->type != XARCHIVETYPE_TAR && archive->type != XARCHIVETYPE_TAR_GZ && archive->type != XARCHIVETYPE_TAR_LZMA && archive->type != XARCHIVETYPE_TAR_BZ2 && archive->type != XARCHIVETYPE_DEB)
		flag = FALSE;
	else
		flag = TRUE;

	dialog_data->touch = gtk_check_button_new_with_mnemonic (_("Touch files"));
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (dialog_data->touch),archive->tar_touch);
	gtk_tooltips_set_tip (option_tooltip,dialog_data->touch, _("When this option is used, tar leaves the data modification times of the files it extracts as the times when the files were extracted, instead of setting it to the times recorded in the archive."), NULL );
	gtk_box_pack_start (GTK_BOX (vbox5), dialog_data->touch, FALSE,FALSE,0);
	gtk_widget_set_sensitive (dialog_data->touch,flag);

	if (archive->type == XARCHIVETYPE_RAR || archive->type == XARCHIVETYPE_ZIP || archive->type == XARCHIVETYPE_ARJ)
		flag = TRUE;
	else
		flag = FALSE;	

	dialog_data->fresh = gtk_check_button_new_with_mnemonic (_("Freshen existing files"));
	gtk_tooltips_set_tip (option_tooltip,dialog_data->fresh , _("Extract only those files that already exist on disk and that are newer than the disk copies."), NULL );
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (dialog_data->fresh), archive->freshen);
	gtk_box_pack_start (GTK_BOX (vbox5), dialog_data->fresh, FALSE, FALSE, 0);
	g_signal_connect (G_OBJECT (dialog_data->fresh),"toggled",G_CALLBACK (fresh_update_toggled_cb) , dialog_data);

	dialog_data->update = gtk_check_button_new_with_mnemonic (_("Update existing files"));
	gtk_tooltips_set_tip (option_tooltip,dialog_data->update , _("This option performs the same function as the freshen one, extracting files that are newer than those with the same name on disk, and in addition it extracts those files that do not already exist on disk."), NULL );
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (dialog_data->update), archive->update);
	gtk_box_pack_start (GTK_BOX (vbox5), dialog_data->update, FALSE, FALSE, 0);
	g_signal_connect (G_OBJECT (dialog_data->update),"toggled",G_CALLBACK (update_fresh_toggled_cb) , dialog_data);
	gtk_widget_set_sensitive(dialog_data->fresh,flag);
	gtk_widget_set_sensitive(dialog_data->update,flag);

	label3 = gtk_label_new (_("Options "));
	gtk_frame_set_label_widget (GTK_FRAME (frame2), label3);

	vbox4 = gtk_vbox_new (FALSE, 4);
	gtk_box_pack_start (GTK_BOX (hbox1), vbox4, TRUE, TRUE, 0);

	label4 = gtk_label_new (_("Directories Tree:"));
	gtk_box_pack_start (GTK_BOX (vbox4), label4, FALSE, FALSE, 0);
	gtk_misc_set_alignment (GTK_MISC (label4), 0, 0.5);

	scrolledwindow1 = gtk_scrolled_window_new (NULL, NULL);
	gtk_box_pack_start (GTK_BOX (vbox4), scrolledwindow1, TRUE, TRUE, 0);
	g_object_set (G_OBJECT (scrolledwindow1),"hscrollbar-policy", GTK_POLICY_AUTOMATIC,"shadow-type", GTK_SHADOW_IN,"vscrollbar-policy", GTK_POLICY_AUTOMATIC, NULL);

	model = gtk_tree_store_new (3,G_TYPE_STRING,G_TYPE_STRING,G_TYPE_STRING);
	dialog_data->treeview3 = gtk_tree_view_new_with_model(GTK_TREE_MODEL(model));
	gtk_container_add (GTK_CONTAINER (scrolledwindow1), dialog_data->treeview3);
	gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (dialog_data->treeview3), FALSE);
	gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(model),1,GTK_SORT_ASCENDING);

	g_signal_connect (G_OBJECT (dialog_data->treeview3),"row-expanded",G_CALLBACK(xa_expand_dir),dialog_data->destination_path_entry);
	g_signal_connect (G_OBJECT (dialog_data->treeview3),"row-activated",G_CALLBACK(xa_row_activated),dialog_data->destination_path_entry);
	GtkTreeSelection *sel = gtk_tree_view_get_selection(GTK_TREE_VIEW (dialog_data->treeview3));
	g_signal_connect (sel,"changed",G_CALLBACK (xa_tree_view_row_selected),dialog_data->destination_path_entry);

	column = gtk_tree_view_column_new();
	dialog_data->renderer = gtk_cell_renderer_pixbuf_new();
	gtk_tree_view_column_pack_start(column,dialog_data->renderer,FALSE);
	gtk_tree_view_column_set_attributes(column,dialog_data->renderer, "stock-id",0,NULL);

	dialog_data->renderer = gtk_cell_renderer_text_new();
	gtk_tree_view_column_pack_start(column,dialog_data->renderer,TRUE);
	gtk_tree_view_column_set_attributes( column,dialog_data->renderer,"text",1,NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW (dialog_data->treeview3),column);
	g_signal_connect (dialog_data->renderer, "editing-canceled",G_CALLBACK (xa_cell_edited_canceled),dialog_data);
	g_signal_connect (dialog_data->renderer, "edited",G_CALLBACK (xa_cell_edited),dialog_data);

	hbox4 = gtk_hbox_new (FALSE, 0);
	gtk_box_pack_end (GTK_BOX (vbox1), hbox4, FALSE, FALSE, 0);
  
	dialog_data->create_dir = gtk_button_new_with_mnemonic (_("Create New Dir"));
	gtk_box_pack_end (GTK_BOX (hbox4),dialog_data->create_dir,FALSE, FALSE,0);
	g_signal_connect (G_OBJECT(dialog_data->create_dir),"clicked",G_CALLBACK(xa_create_dir_button_pressed),dialog_data);

	hbox3 = gtk_hbox_new (FALSE, 0);
	gtk_box_pack_start (GTK_BOX (vbox5),hbox3,TRUE,TRUE,0);

	label_password = gtk_label_new (_("Password:"));
	gtk_box_pack_start (GTK_BOX (hbox3), label_password,FALSE,FALSE,0);

	dialog_data->password_entry = gtk_entry_new ();
	gtk_box_pack_start (GTK_BOX (hbox3), dialog_data->password_entry,TRUE,TRUE,0);
	gtk_entry_set_visibility (GTK_ENTRY (dialog_data->password_entry),FALSE);
	if (archive->has_passwd)
    {
		gtk_widget_set_sensitive (label_password,TRUE);
		gtk_widget_set_sensitive (dialog_data->password_entry,TRUE);
		if (archive->passwd != NULL)
			gtk_entry_set_text (GTK_ENTRY(dialog_data->password_entry),archive->passwd);
    }
	else
	{
		gtk_widget_set_sensitive (label_password,FALSE);
		gtk_widget_set_sensitive (dialog_data->password_entry,FALSE);
	}

	dialog_action_area1 = GTK_DIALOG (dialog_data->dialog1)->action_area;
	gtk_button_box_set_layout (GTK_BUTTON_BOX (dialog_action_area1), GTK_BUTTONBOX_END);

	cancel_button = gtk_button_new_from_stock(GTK_STOCK_CANCEL);
	gtk_dialog_add_action_widget (GTK_DIALOG (dialog_data->dialog1),cancel_button, GTK_RESPONSE_CANCEL);
	GTK_WIDGET_SET_FLAGS (cancel_button, GTK_CAN_DEFAULT);

	extract_button = gtk_button_new();
	extract_image = xa_main_window_find_image("xarchiver-extract.png", GTK_ICON_SIZE_SMALL_TOOLBAR);
	extract_hbox = gtk_hbox_new(FALSE, 4);
	extract_label = gtk_label_new_with_mnemonic(_("_Extract"));

	alignment3 = gtk_alignment_new (0.5, 0.5, 0, 0);
	gtk_container_add (GTK_CONTAINER (alignment3),extract_hbox);
	gtk_box_pack_start(GTK_BOX(extract_hbox),extract_image,FALSE,FALSE,0);
	gtk_box_pack_start(GTK_BOX(extract_hbox),extract_label,FALSE,FALSE,0);
	gtk_container_add(GTK_CONTAINER(extract_button),alignment3);

	gtk_dialog_add_action_widget (GTK_DIALOG (dialog_data->dialog1),extract_button,GTK_RESPONSE_OK);
	GTK_WIDGET_SET_FLAGS (extract_button, GTK_CAN_DEFAULT);
	gtk_dialog_set_default_response (GTK_DIALOG (dialog_data->dialog1),GTK_RESPONSE_OK);
	xa_browse_dir(model,NULL,NULL);
	gtk_widget_show_all(dialog_data->dialog1);
	return dialog_data;
}

void xa_create_dir_button_pressed (GtkButton *button,gpointer data)
{
	Extract_dialog_data *dialog = data;
	GtkTreeModel *model;
	GtkTreeSelection *selection;
	GtkTreeIter child,iter;
	GtkTreePath *path;
	GtkTreeViewColumn *column;

	g_object_set(dialog->renderer,"editable",TRUE,NULL);
	model = gtk_tree_view_get_model (GTK_TREE_VIEW(dialog->treeview3));
	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(dialog->treeview3));

	if (selection)
	{
		gtk_tree_selection_get_selected(selection,NULL,&iter);
		gtk_widget_set_sensitive(dialog->create_dir,FALSE);
		gtk_tree_store_append(GTK_TREE_STORE(model),&child,&iter);
		gtk_tree_view_get_cursor(GTK_TREE_VIEW(dialog->treeview3),&path,&column);
		gtk_tree_view_expand_to_path(GTK_TREE_VIEW(dialog->treeview3),path);

		gtk_tree_path_down(path);
		gtk_tree_selection_select_iter(selection,&child);
		gtk_tree_view_set_cursor(GTK_TREE_VIEW(dialog->treeview3),path,column,TRUE);
		gtk_tree_path_free (path);
	}
}

void xa_cell_edited_canceled(GtkCellRenderer *renderer,gpointer data)
{
	Extract_dialog_data *dialog = data;
	GtkTreeModel *model;
	GtkTreeIter iter;
	GtkTreeSelection *selection;
	
	model = gtk_tree_view_get_model(GTK_TREE_VIEW(dialog->treeview3));
	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(dialog->treeview3));
	if (selection)
	{
		g_object_set(dialog->renderer,"editable",FALSE,NULL);
		gtk_tree_selection_get_selected(selection,&model,&iter);
		gtk_tree_store_remove(GTK_TREE_STORE(model),&iter);
		gtk_widget_set_sensitive(dialog->create_dir,TRUE);
	}
}

void xa_cell_edited (GtkCellRendererText *cell,const gchar *path_string,const gchar *new_text,gpointer data)
{
	Extract_dialog_data *dialog = data;
	GtkTreeModel *model;
	GtkTreeIter iter;
	gchar *previous_dir;
	gchar *fullname;
	gint result;

	model = gtk_tree_view_get_model (GTK_TREE_VIEW(dialog->treeview3));
	GtkTreePath *path = gtk_tree_path_new_from_string (path_string);
	gtk_tree_model_get_iter (model, &iter,path);
	
	previous_dir = g_strdup(gtk_entry_get_text(GTK_ENTRY(dialog->destination_path_entry)));
	fullname = g_strconcat(previous_dir,"/",new_text,NULL);
	g_free(previous_dir);

	gtk_tree_store_set(GTK_TREE_STORE(model),&iter,0,"gtk-directory",1,new_text,2,fullname,-1);

	result = mkdir (fullname,S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IXGRP | S_IROTH | S_IXGRP);
	if (result == -1)
	{
		gchar *msg = g_strdup_printf(_("Can't create directory \"%s\""),fullname);
		result = xa_show_message_dialog (GTK_WINDOW (xa_main_window),GTK_DIALOG_MODAL,GTK_MESSAGE_ERROR,GTK_BUTTONS_OK,msg,g_strerror(errno ));
		g_free (msg);
		gtk_tree_store_remove(GTK_TREE_STORE(model),&iter);
	}
	else
	{
		dialog->string = gtk_tree_path_to_string (gtk_tree_model_get_path (model, &iter));
		dialog->signal_id = g_signal_connect_after(G_OBJECT(GTK_TREE_VIEW(dialog->treeview3)),"expose-event",G_CALLBACK(xa_treeview_exposed),dialog);
		gtk_entry_set_text(GTK_ENTRY(dialog->destination_path_entry),fullname);
	}
	g_free(fullname);
	gtk_tree_path_free(path);
	gtk_widget_set_sensitive(dialog->create_dir,TRUE);
	g_object_set(dialog->renderer,"editable",FALSE,NULL);
}

void xa_activate_entry(GtkToggleButton *button,gpointer data)
{
	Extract_dialog_data *dialog = data;

	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(dialog->files_radio)))
	{
		gtk_widget_set_sensitive (dialog->entry2,TRUE);
		gtk_widget_grab_focus (dialog->entry2);
	}
	else
		gtk_widget_set_sensitive (dialog->entry2,FALSE);
}

void fresh_update_toggled_cb (GtkToggleButton *button,Extract_dialog_data *data)
{
	gboolean active = gtk_toggle_button_get_active(button);
	if (active)
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (data->update),FALSE);
}

void update_fresh_toggled_cb (GtkToggleButton *button, Extract_dialog_data *data)
{
	if (data->fresh == NULL)
		return;
	gboolean active = gtk_toggle_button_get_active (button);
	if (active)
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (data->fresh),FALSE);
}

void xa_parse_extract_dialog_options (XArchive *archive,Extract_dialog_data *dialog_data,GtkTreeSelection *selection)
{
	gchar *destination_path = NULL;
	gboolean done = FALSE;
	GSList *names = NULL;
	int response;

	if (unrar)
		rar = "unrar";
	else
		rar = "rar";

    while (! done)
	{
		switch (gtk_dialog_run(GTK_DIALOG(dialog_data->dialog1)))
		{
			case GTK_RESPONSE_CANCEL:
			case GTK_RESPONSE_DELETE_EVENT:
			done = TRUE;
			if (xa_main_window && (archive->type == XARCHIVETYPE_GZIP || archive->type == XARCHIVETYPE_LZMA || archive->type == XARCHIVETYPE_BZIP2) )
			{
				gtk_widget_set_sensitive (Stop_button,FALSE);
				xa_set_button_state (1,1,GTK_WIDGET_IS_SENSITIVE(save1),GTK_WIDGET_IS_SENSITIVE(close1),0,0,0,0,0);
				archive->status = XA_ARCHIVESTATUS_IDLE;
			}
			break;

			case GTK_RESPONSE_OK:
			destination_path = g_strdup (gtk_entry_get_text (GTK_ENTRY (dialog_data->destination_path_entry)));
			archive->extraction_path = xa_escape_bad_chars (destination_path , "$\'`\"\\!?* ()&|@#:;");

			if (strlen(archive->extraction_path) == 0)
			{
				response = xa_show_message_dialog (GTK_WINDOW (xa_main_window),GTK_DIALOG_MODAL,GTK_MESSAGE_ERROR,GTK_BUTTONS_OK, _("You missed where to extract the files!"),_("Please enter the extraction path.") );
				break;
			}
			if (archive->extraction_path[0] != '/')
			{
				gchar *cur_dir = g_get_current_dir();
				archive->extraction_path = g_strconcat(cur_dir, "/",archive->extraction_path,NULL);
				g_free (cur_dir);
			}
			if (archive->has_passwd)
				archive->passwd  = g_strdup (gtk_entry_get_text (GTK_ENTRY(dialog_data->password_entry)));

			if (archive->has_passwd && strlen(archive->passwd) == 0 )
			{
				response = xa_show_message_dialog (GTK_WINDOW (xa_main_window),GTK_DIALOG_MODAL,GTK_MESSAGE_ERROR,GTK_BUTTONS_OK, _("This archive is encrypted!"),_("Please enter the password.") );
				break;
			}

			if (g_file_test (destination_path,G_FILE_TEST_IS_DIR) && access (destination_path, R_OK | W_OK | X_OK ))
			{
				gchar *utf8_path;
				gchar  *msg;

                utf8_path = g_filename_to_utf8 (destination_path, -1, NULL, NULL, NULL);
                msg = g_strdup_printf (_("You don't have the right permissions to extract the files to the directory \"%s\"."), utf8_path);
				response = xa_show_message_dialog (GTK_WINDOW (xa_main_window),GTK_DIALOG_MODAL,GTK_MESSAGE_ERROR,GTK_BUTTONS_OK, _("Can't perform extraction!"),msg );
                g_free (utf8_path);
				g_free (msg);
				g_free (destination_path);
				break;
			}
			done = TRUE;
			archive->overwrite = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(dialog_data->overwrite_check));
			archive->tar_touch = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(dialog_data->touch));
			archive->full_path = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(dialog_data->extract_full));
			archive->freshen   = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(dialog_data->fresh));
			archive->update    = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(dialog_data->update));

			gtk_widget_hide (dialog_data->dialog1);
			archive->status = XA_ARCHIVESTATUS_EXTRACT;
			/* Is the radiobutton Files selected? */
			if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON (dialog_data->files_radio)))
			{
				if (xa_main_window)
					gtk_widget_set_sensitive (Stop_button,TRUE);
			}
			else if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON (dialog_data->selected_radio)))
				gtk_tree_selection_selected_foreach(selection,(GtkTreeSelectionForeachFunc) xa_concat_filenames,&names);

			(*archive->extract) (archive,names);
		}
	}
	g_free (destination_path);
}

void xa_browse_dir(GtkTreeStore *model,gchar *path, GtkTreeIter *parent)
{
	DIR *dir;
	struct dirent *list;
	gchar *fullname;
	GtkTreeIter iter;
	GtkTreeIter dummy;

	dir = opendir(path ? path : "/");

	if (dir == NULL)
		return;
	while ((list = readdir(dir)))
	{
		if (list->d_name[0] == '.')
			continue;
		fullname = g_strconcat (path ? path : "","/",list->d_name,NULL);
		if (g_file_test(fullname,G_FILE_TEST_IS_DIR))
		{
			gtk_tree_store_append(model,&iter,parent);
			gtk_tree_store_set(model,&iter,0,"gtk-directory",1,list->d_name,2,fullname,-1);
			gtk_tree_store_append(model,&dummy,&iter);
		}
		g_free(fullname);
	}
	closedir(dir);
}

void xa_tree_view_row_selected(GtkTreeSelection *selection, gpointer data)
{
	GtkEntry *entry = data;
	gchar *dir;
	GtkTreeIter iter;
	GtkTreeModel *model;

	if (gtk_tree_selection_get_selected (selection,&model,&iter))
	{
		gtk_tree_model_get (model,&iter,2,&dir,-1);
		if (dir != NULL)
		{
			gtk_entry_set_text(entry,dir);
			g_free(dir);
		}
	}
}

void xa_row_activated(GtkTreeView *tree_view,GtkTreePath *path,GtkTreeViewColumn *column,gpointer user_data)
{
	if (gtk_tree_view_row_expanded(tree_view,path))
		gtk_tree_view_collapse_row(tree_view,path);
	else
		gtk_tree_view_expand_to_path(tree_view,path);
}

void xa_expand_dir(GtkTreeView *tree_view,GtkTreeIter *iter,GtkTreePath *path,gpointer data)
{
	GtkEntry *entry = data;
	GtkTreeModel *model;
	gchar *dir;
	GtkTreeIter child;
	gchar *fullname;

	model = gtk_tree_view_get_model(tree_view);
    gtk_tree_model_get(model,iter,2,&dir,-1);
	gtk_tree_model_iter_nth_child(model,&child,iter,0);
	gtk_tree_model_get(model,&child,1,&fullname,-1);
	if (fullname == NULL)
	{
		xa_browse_dir(GTK_TREE_STORE(model),dir,iter);
		gtk_tree_store_remove(GTK_TREE_STORE(model),&child);
	}
	else
		g_free(fullname);
	
	gtk_entry_set_text(entry,dir);
	g_free(dir);
}

void xa_treeview_exposed (GtkWidget *widget,GdkEventExpose *event,gpointer data)
{
	Extract_dialog_data *dialog = data;

	GtkTreePath *path = gtk_tree_path_new_from_string (dialog->string);
	gtk_tree_view_scroll_to_cell(GTK_TREE_VIEW(widget),path,NULL,FALSE,0,0);
	gtk_tree_path_free (path);
	g_signal_handler_disconnect (G_OBJECT(widget),dialog->signal_id);
}
