/*
 *  Copyright (C)2008 Giuseppe Torelli - <colossus73@gmail.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 *  MA 02110-1301 USA.
 */

#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <glib/gstdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include "extract_dialog.h"
#include "interface.h"
#include "main.h"
#include "pref_dialog.h"
#include "string_utils.h"
#include "support.h"
#include "window.h"

static GtkWidget *label_password;

static const GtkTargetEntry drop_targets[] =
{
	{ "text/uri-list",0,0 },
};

static void xa_select_where_to_extract (GtkEntry *entry, gint icon_pos, GTK_COMPAT_ENTRY_ICON_TYPE button_release, gpointer user_data)
{
	GtkWidget *file_selector;
	gchar *dest_dir, *dest_dir_utf8;
	const char *current_path;
	gint response;

	file_selector = gtk_file_chooser_dialog_new (_("Please select the destination directory"),
							GTK_WINDOW (xa_main_window),
							GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER,
							GTK_STOCK_CANCEL,
							GTK_RESPONSE_CANCEL,
							GTK_STOCK_OPEN,
							GTK_RESPONSE_ACCEPT,
							NULL);

	xa_set_xarchiver_icon(GTK_WINDOW(file_selector));

	current_path = gtk_entry_get_text(GTK_ENTRY(entry));
	if (strlen(current_path) > 0)
		gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER (file_selector),current_path);

	response = gtk_dialog_run (GTK_DIALOG(file_selector));
	if (response == GTK_RESPONSE_ACCEPT)
	{
		dest_dir = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER (file_selector));
		dest_dir_utf8 = g_filename_display_name(dest_dir);
		gtk_entry_set_text(GTK_ENTRY(entry), dest_dir_utf8);
		g_free(dest_dir_utf8);
		g_free(dest_dir);
	}
	gtk_widget_destroy(file_selector);
}

static void xa_toggle_all_files_radio (GtkToggleButton *button, Extract_dialog_data *dialog)
{
	gtk_widget_set_sensitive(dialog->ensure_directory, gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(dialog->all_files_radio)));
}

static void xa_activate_entry (GtkToggleButton *button, Extract_dialog_data *dialog)
{
	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(dialog->files_radio)))
	{
		gtk_widget_set_sensitive (dialog->entry2,TRUE);
		gtk_widget_grab_focus (dialog->entry2);
	}
	else
		gtk_widget_set_sensitive (dialog->entry2,FALSE);
}

static void xa_multi_extract_dialog_selection_changed (GtkTreeSelection *selection, Multi_extract_data *dialog_data)
{
	GtkTreeIter iter;
	GtkTreeModel *model;
	XArchiveType type;
	guint tag;
	XArchive archive = {0};

	if (gtk_tree_selection_get_selected (selection,&model,&iter))
	{
		gtk_tree_model_get(model, &iter, 3, &type, 4, &tag, -1);

		archive.type = type;
		archive.tag = tag;

		(*archiver[type].ask)(&archive);

		gtk_widget_set_sensitive(dialog_data->full_path, archive.can_full_path[0]);
	}
}

static void xa_multi_extract_dialog_drag_data_received (GtkWidget *widget, GdkDragContext *context, gint x, gint y, GtkSelectionData *data, guint info, guint time, gpointer dialog_data)
{
	gchar **array = NULL;
	gchar *filename;
	unsigned int len = 0;

	gtk_tree_view_get_model(GTK_TREE_VIEW(widget));
	array = gtk_selection_data_get_uris (data);
	if (array == NULL)
	{
		xa_show_message_dialog(GTK_WINDOW (xa_main_window),GTK_DIALOG_MODAL,GTK_MESSAGE_ERROR,GTK_BUTTONS_OK,"",_("Sorry, I could not perform the operation!"));
		gtk_drag_finish (context,FALSE,FALSE,time);
		return;
	}
	gtk_drag_finish (context,TRUE,FALSE,time);
	while (array[len])
	{
		filename = g_filename_from_uri (array[len],NULL,NULL);
		xa_multi_extract_dialog_add_file(filename, dialog_data);
		g_free (filename);
		len++;
	}
	g_strfreev (array);
}

static void xa_multi_extract_dialog_select_files_to_add (GtkButton *button, Multi_extract_data *dialog)
{
	GtkWidget *file_selector;
	GSList *dummy = NULL;
	gint response;

	file_selector = gtk_file_chooser_dialog_new (_("Please select the archives you want to extract"),
							GTK_WINDOW (xa_main_window),
							GTK_FILE_CHOOSER_ACTION_OPEN,
							GTK_STOCK_CANCEL,
							GTK_RESPONSE_CANCEL,
							GTK_STOCK_OPEN,
							GTK_RESPONSE_ACCEPT,
							NULL);
	gtk_file_chooser_set_select_multiple (GTK_FILE_CHOOSER(file_selector),TRUE);
	response = gtk_dialog_run (GTK_DIALOG(file_selector));
	if (response == GTK_RESPONSE_ACCEPT)
	{
		dummy = gtk_file_chooser_get_filenames (GTK_FILE_CHOOSER (file_selector));
		g_slist_foreach(dummy, (GFunc) xa_multi_extract_dialog_add_file, dialog);
	}
	if (dummy != NULL)
		g_slist_free (dummy);
	gtk_widget_destroy(file_selector);
	return;
}

static void xa_activate_remove_button (GtkTreeModel *tree_model, GtkTreePath *path, GtkTreeIter *iter, GtkWidget *remove_button)
{
	if (gtk_tree_model_get_iter_first(tree_model,iter)== TRUE)
		gtk_widget_set_sensitive (remove_button,TRUE );
}

static void remove_foreach_func (GtkTreeModel *model, GtkTreePath *path, GtkTreeIter *iter, GList **rowref_list)
{
	GtkTreeRowReference *rowref;

	rowref = gtk_tree_row_reference_new(model,path);
	*rowref_list = g_list_append(*rowref_list,rowref);
}

static void xa_multi_extract_dialog_remove_files (GtkButton *button, Multi_extract_data *multi_extract_data)
{
	GtkTreeModel *model;
	GtkTreeSelection *sel;
	GtkTreePath *path;
	GtkTreeIter iter;
	GList *rr_list = NULL;
	GList *node;

	model = gtk_tree_view_get_model(GTK_TREE_VIEW(multi_extract_data->files_treeview));
	sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(multi_extract_data->files_treeview));
	gtk_tree_selection_selected_foreach(sel,(GtkTreeSelectionForeachFunc)remove_foreach_func,&rr_list);

	for (node = rr_list; node != NULL; node = node->next)
	{
		path = gtk_tree_row_reference_get_path((GtkTreeRowReference *)node->data);
		if (path)
		{
			if ( gtk_tree_model_get_iter(GTK_TREE_MODEL(model),&iter,path))
			{
				gtk_list_store_remove(multi_extract_data->files_liststore,&iter);
				multi_extract_data->nr--;
			}
			gtk_tree_path_free(path);
		}
	}
	if (gtk_tree_model_get_iter_first(GTK_TREE_MODEL(model),&iter)== FALSE)
		gtk_widget_set_sensitive(GTK_WIDGET(button), FALSE);
	g_list_free_full(rr_list, (GDestroyNotify) gtk_tree_row_reference_free);
}

static gchar *xa_multi_extract_one_archive (Multi_extract_data *dialog, gchar *filename, gboolean overwrite, gboolean full_path, gchar *dest_path)
{
	XArchive *archive = NULL;
	gchar *dirname = NULL;
	gchar *new_path = NULL;
	gchar *_filename = NULL;
	gchar *error = NULL;
	ArchiveType xa;
	GtkWidget *main_window;

	if (dest_path == NULL)
	{
		_filename = g_strrstr(filename, ".");
		if (_filename)
			_filename = g_strndup(filename,(_filename-filename));
		else
			_filename = filename;
		dest_path	= xa_remove_level_from_path(_filename);
		dirname = g_path_get_basename(_filename);
		new_path	= g_strconcat(dest_path,"/",dirname,NULL);
		g_free(dirname);
		g_free(dest_path);
		if (g_mkdir(new_path,0700)< 0)
		{
			g_free(new_path);
			return strerror(errno);
		}
		dest_path = new_path;
	}
	xa = xa_detect_archive_type(filename);
	archive = xa_init_archive_structure(xa);
	dialog->archive = archive;
	archive->do_overwrite = overwrite;
	archive->do_full_path = full_path;
	archive->path[0] = g_strdup(filename);
	archive->path[1] = xa_escape_bad_chars(filename, ESCAPES);
	archive->extraction_dir = xa_escape_bad_chars(dest_path, ESCAPES);

	/* temporarily enter batch mode */
	main_window = xa_main_window;
	xa_main_window = NULL;

	xa_detect_encrypted_archive(archive);

	if (archive->has_password)
	{
		if (!xa_check_password(archive))
			error = _("You missed the password!");
	}

	if (!error)
	{
		archive->status = XARCHIVESTATUS_EXTRACT;
		(*archive->archiver->extract)(archive,NULL);
	}

	/* return from batch mode */
	xa_main_window = main_window;

	xa_clean_archive_structure(archive);

	return error;
}

static void toggle_overwrite_update_freshen (GtkToggleButton *button, Extract_dialog_data *data)
{
	gboolean active = gtk_toggle_button_get_active(button);

	if (active)
	{
		if (button != GTK_TOGGLE_BUTTON(data->overwrite_check))
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(data->overwrite_check), FALSE);

		if (button != GTK_TOGGLE_BUTTON(data->update))
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(data->update), FALSE);

		if (button != GTK_TOGGLE_BUTTON(data->fresh))
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(data->fresh), FALSE);
	}
}

Extract_dialog_data *xa_create_extract_dialog()
{
	GTK_COMPAT_TOOLTIPS;
	GSList *radiobutton1_group;
	Extract_dialog_data *dialog_data;
	GtkWidget *hbox1, *hbox2, *hbox3, *vbox1, *vbox2, *vbox3, *vbox5, *alignment, *alignment1, *alignment2, *alignment3, *label1, *label2, *label3;
	GtkWidget *frame1, *frame2;
	GtkWidget *cancel_button, *extract_button, *extract_image, *extract_hbox, *extract_label;

	dialog_data = g_new0 (Extract_dialog_data,1);
	dialog_data->dialog1 = gtk_dialog_new();

	gtk_window_set_position (GTK_WINDOW (dialog_data->dialog1),GTK_WIN_POS_CENTER_ON_PARENT);
	gtk_window_set_type_hint (GTK_WINDOW (dialog_data->dialog1),GDK_WINDOW_TYPE_HINT_DIALOG);
	gtk_window_set_destroy_with_parent(GTK_WINDOW (dialog_data->dialog1),TRUE);

	xa_set_xarchiver_icon(GTK_WINDOW(dialog_data->dialog1));

	dialog_data->dialog_vbox1 = gtk_dialog_get_content_area(GTK_DIALOG(dialog_data->dialog1));

	vbox1 = gtk_vbox_new (FALSE,2);
	gtk_container_add(GTK_CONTAINER(dialog_data->dialog_vbox1), vbox1);
	gtk_container_set_border_width (GTK_CONTAINER (vbox1),2);

	label1 = gtk_label_new (_("Extract to:"));
	gtk_box_pack_start (GTK_BOX (vbox1),label1,FALSE,FALSE,0);
	gtk_misc_set_alignment (GTK_MISC (label1),0,0.5);

	dialog_data->destination_path_entry = GTK_COMPAT_ENTRY_ICON_NEW();
	GTK_COMPAT_ENTRY_ICON(dialog_data->destination_path_entry, xa_select_where_to_extract, dialog_data);
	gtk_box_pack_start (GTK_BOX (vbox1),dialog_data->destination_path_entry,FALSE,FALSE,0);
	gtk_entry_set_activates_default(GTK_ENTRY(dialog_data->destination_path_entry), TRUE);

	alignment = gtk_alignment_new(0.5, 0.5, 1, 1);
	gtk_alignment_set_padding(GTK_ALIGNMENT(alignment), 0, 0, 14, 0);

	dialog_data->ensure_directory = gtk_check_button_new_with_mnemonic(_("Ensure a containing directory"));
	gtk_widget_set_tooltip_text(dialog_data->ensure_directory, _("Ensure that the contents of the extracted archive is always in a containing directory"));
	gtk_container_add(GTK_CONTAINER(alignment), dialog_data->ensure_directory);
	gtk_box_pack_start(GTK_BOX(vbox1), alignment, FALSE, FALSE, 0);

	hbox1 = gtk_hbox_new (TRUE,10);
	gtk_box_pack_start (GTK_BOX (vbox1),hbox1,TRUE,TRUE,0);

	vbox2 = gtk_vbox_new (FALSE,5);
	gtk_box_pack_start (GTK_BOX (hbox1),vbox2,TRUE,TRUE,0);

	frame1 = gtk_frame_new (NULL);
	gtk_box_pack_start (GTK_BOX (vbox2),frame1,TRUE,TRUE,0);
	gtk_frame_set_shadow_type (GTK_FRAME (frame1),GTK_SHADOW_OUT);

	alignment1 = gtk_alignment_new (0.5,0.5,1,1);
	gtk_container_add (GTK_CONTAINER (frame1),alignment1);
	gtk_alignment_set_padding(GTK_ALIGNMENT(alignment1), 0, 2, 12, 0);

	vbox3 = gtk_vbox_new (FALSE,0);
	gtk_container_add (GTK_CONTAINER (alignment1),vbox3);

	dialog_data->all_files_radio = gtk_radio_button_new_with_mnemonic (NULL,_("All files"));
	gtk_box_pack_start (GTK_BOX (vbox3),dialog_data->all_files_radio,FALSE,FALSE,0);
	radiobutton1_group = gtk_radio_button_get_group (GTK_RADIO_BUTTON (dialog_data->all_files_radio));
	g_signal_connect(dialog_data->all_files_radio, "toggled", G_CALLBACK(xa_toggle_all_files_radio), dialog_data);

	dialog_data->selected_radio = gtk_radio_button_new_with_mnemonic(radiobutton1_group, _("Selected files"));
	gtk_box_pack_start (GTK_BOX (vbox3),dialog_data->selected_radio,FALSE,FALSE,0);
	radiobutton1_group = gtk_radio_button_get_group (GTK_RADIO_BUTTON (dialog_data->selected_radio));

	hbox2 = gtk_hbox_new (FALSE,0);
	gtk_box_pack_start (GTK_BOX (vbox3),hbox2,FALSE,FALSE,0);

	dialog_data->files_radio = gtk_radio_button_new_with_mnemonic(radiobutton1_group, _("Files: "));
	gtk_box_pack_start (GTK_BOX (hbox2),dialog_data->files_radio,FALSE,FALSE,0);

	dialog_data->entry2 = gtk_entry_new ();
	gtk_box_pack_start (GTK_BOX (hbox2),dialog_data->entry2,TRUE,TRUE,0);
	gtk_entry_set_width_chars (GTK_ENTRY (dialog_data->entry2),10);
	gtk_widget_set_sensitive(dialog_data->entry2,FALSE);
	g_signal_connect (G_OBJECT (dialog_data->files_radio),"toggled",G_CALLBACK(xa_activate_entry),dialog_data);

	label2 = gtk_label_new (_("Files "));
	gtk_frame_set_label_widget (GTK_FRAME (frame1),label2);

	frame2 = gtk_frame_new (NULL);
	gtk_box_pack_start (GTK_BOX (vbox2),frame2,TRUE,TRUE,0);
	gtk_frame_set_shadow_type (GTK_FRAME (frame2),GTK_SHADOW_OUT);

	alignment2 = gtk_alignment_new (0.5,0.5,1,1);
	gtk_container_add (GTK_CONTAINER (frame2),alignment2);
	gtk_alignment_set_padding(GTK_ALIGNMENT(alignment2), 0, 4, 12, 0);

	vbox5 = gtk_vbox_new (FALSE,0);
	gtk_container_add (GTK_CONTAINER (alignment2),vbox5);

	dialog_data->extract_full = gtk_check_button_new_with_mnemonic (_("Extract files with full path"));
	gtk_widget_set_tooltip_text(dialog_data->extract_full, _("The archive's directory structure is recreated in the extraction directory"));
	gtk_box_pack_start (GTK_BOX (vbox5),dialog_data->extract_full,FALSE,FALSE,0);

	dialog_data->touch = gtk_check_button_new_with_mnemonic (_("Touch files"));
	gtk_widget_set_tooltip_text(dialog_data->touch, _("When this option is used, the modification times of the files will be the times of extraction instead of the times recorded in the archive"));
	gtk_box_pack_start (GTK_BOX (vbox5),dialog_data->touch,FALSE,FALSE,0);

	dialog_data->overwrite_check = gtk_check_button_new_with_mnemonic (_("Overwrite existing files"));
	gtk_box_pack_start (GTK_BOX (vbox5),dialog_data->overwrite_check,FALSE,FALSE,0);
	g_signal_connect(G_OBJECT(dialog_data->overwrite_check), "toggled", G_CALLBACK(toggle_overwrite_update_freshen), dialog_data);

	dialog_data->update = gtk_check_button_new_with_mnemonic (_("Update existing files"));
	gtk_widget_set_tooltip_text(dialog_data->update, _("This option performs the same function as the freshen one, extracting files that are newer than those with the same name on disk, and in addition it extracts those files that do not already exist on disk"));
	gtk_box_pack_start (GTK_BOX (vbox5),dialog_data->update,FALSE,FALSE,0);
	g_signal_connect(G_OBJECT(dialog_data->update), "toggled", G_CALLBACK(toggle_overwrite_update_freshen), dialog_data);

	dialog_data->fresh = gtk_check_button_new_with_mnemonic (_("Freshen existing files"));
	gtk_widget_set_tooltip_text(dialog_data->fresh, _("Extract only those files that already exist on disk and that are newer than the disk copies"));
	gtk_box_pack_start (GTK_BOX (vbox5),dialog_data->fresh,FALSE,FALSE,0);
	g_signal_connect(G_OBJECT(dialog_data->fresh), "toggled", G_CALLBACK(toggle_overwrite_update_freshen), dialog_data);

	label3 = gtk_label_new (_("Options "));
	gtk_frame_set_label_widget (GTK_FRAME (frame2),label3);

	hbox3 = gtk_hbox_new(FALSE, 4);
	gtk_box_pack_start (GTK_BOX (vbox5),hbox3,TRUE,TRUE,0);

	label_password = gtk_label_new (_("Password:"));
	gtk_box_pack_start (GTK_BOX (hbox3),label_password,FALSE,FALSE,0);

	dialog_data->password_entry = gtk_entry_new ();
	gtk_box_pack_start (GTK_BOX (hbox3),dialog_data->password_entry,TRUE,TRUE,0);
	gtk_entry_set_visibility (GTK_ENTRY (dialog_data->password_entry),FALSE);
	gtk_entry_set_activates_default(GTK_ENTRY(dialog_data->password_entry), TRUE);

	cancel_button = gtk_button_new_from_stock(GTK_STOCK_CANCEL);
	gtk_dialog_add_action_widget (GTK_DIALOG (dialog_data->dialog1),cancel_button,GTK_RESPONSE_CANCEL);

	extract_button = gtk_button_new();
	extract_image = xa_main_window_find_image("xarchiver-extract.png",GTK_ICON_SIZE_SMALL_TOOLBAR);
	extract_hbox = gtk_hbox_new(FALSE,4);
	extract_label = gtk_label_new_with_mnemonic(_("_Extract"));

	alignment3 = gtk_alignment_new (0.5,0.5,0,0);
	gtk_container_add (GTK_CONTAINER (alignment3),extract_hbox);
	gtk_box_pack_start(GTK_BOX(extract_hbox),extract_image,FALSE,FALSE,0);
	gtk_box_pack_start(GTK_BOX(extract_hbox),extract_label,FALSE,FALSE,0);
	gtk_container_add(GTK_CONTAINER(extract_button),alignment3);

	gtk_dialog_add_action_widget (GTK_DIALOG (dialog_data->dialog1),extract_button,GTK_RESPONSE_OK);
	gtk_widget_set_can_default(extract_button, TRUE);
	gtk_dialog_set_default_response (GTK_DIALOG (dialog_data->dialog1),GTK_RESPONSE_OK);
	return dialog_data;
}

void xa_set_extract_dialog_options(Extract_dialog_data *dialog_data,gint selected,XArchive *archive)
{
	if (progress)
		gtk_widget_hide(progress->window);

	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(prefs_window->check_save_geometry))&& prefs_window->extract_dialog[0] != -1)
		gtk_window_set_default_size (GTK_WINDOW(dialog_data->dialog1),prefs_window->extract_dialog[0],prefs_window->extract_dialog[1]);
	else
		gtk_widget_set_size_request (dialog_data->dialog1,-1,370);

	prefs_window->size_changed[0] = TRUE;

	gtk_window_set_title(GTK_WINDOW(dialog_data->dialog1), _("Extract files"));

	gtk_widget_set_sensitive(dialog_data->files_radio, archive->files > 1);

		if (selected)
		{
			gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (dialog_data->selected_radio),TRUE);
			gtk_widget_set_sensitive (dialog_data->selected_radio,TRUE);
		}
		else
		{
			gtk_widget_set_sensitive (dialog_data->selected_radio,FALSE);
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON (dialog_data->all_files_radio),TRUE);
		}

	gtk_widget_set_sensitive(dialog_data->extract_full, archive->can_full_path[0]);
	gtk_widget_set_sensitive(dialog_data->touch, archive->can_touch);
	gtk_widget_set_sensitive(dialog_data->overwrite_check, archive->can_overwrite);
	gtk_widget_set_sensitive(dialog_data->update, archive->can_update[0]);
	gtk_widget_set_sensitive(dialog_data->fresh, archive->can_freshen[0]);

	if (!archive->destination_path)
	{
		gchar *archive_dir = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(prefs_window->combo_prefered_extract_dir));

		if (!archive_dir || !*archive_dir)
		{
			gchar *archive_dir_utf8;

			g_free(archive_dir);
			archive_dir_utf8 = g_filename_display_name(archive->path[0]);
			archive_dir = xa_remove_level_from_path(archive_dir_utf8);
			g_free(archive_dir_utf8);
		}

		archive->destination_path = g_strdup(archive_dir);
		g_free(archive_dir);
	}
	gtk_entry_set_text(GTK_ENTRY(dialog_data->destination_path_entry), archive->destination_path);
	gtk_entry_set_text(GTK_ENTRY(dialog_data->password_entry), archive->password ? archive->password : "");

	gtk_widget_set_sensitive(label_password, archive->has_password);
	gtk_widget_set_sensitive(dialog_data->password_entry, archive->has_password);

	gtk_widget_show_all(dialog_data->dialog1);
}

void xa_parse_extract_dialog_options (XArchive *archive,Extract_dialog_data *dialog_data,GtkTreeSelection *selection)
{
	gchar *destination_path, *string;
	gboolean done = FALSE;
	GSList *names = NULL;
	GtkTreeModel *model;

    while (! done)
	{
		switch (gtk_dialog_run(GTK_DIALOG(dialog_data->dialog1)))
		{
			case GTK_RESPONSE_CANCEL:
			case GTK_RESPONSE_DELETE_EVENT:
			done = TRUE;
			break;

			case GTK_RESPONSE_OK:
			g_free(archive->destination_path);
			archive->destination_path = g_strdup(gtk_entry_get_text(GTK_ENTRY(dialog_data->destination_path_entry)));
			destination_path = g_filename_from_utf8(archive->destination_path, -1, NULL, NULL, NULL);
			g_free(archive->extraction_dir);
			archive->extraction_dir = xa_escape_bad_chars(destination_path, ESCAPES);

			if (strlen(archive->extraction_dir)== 0)
			{
				xa_show_message_dialog (GTK_WINDOW (xa_main_window),GTK_DIALOG_MODAL,GTK_MESSAGE_ERROR,GTK_BUTTONS_OK,_("You missed where to extract the files!"),_("Please enter the extraction path."));
				break;
			}
			if (archive->extraction_dir[0] != '/')
			{
				gchar *cur_dir, *extraction_dir;

				cur_dir = g_get_current_dir();
				extraction_dir = g_strconcat(cur_dir, "/", destination_path, NULL);
				g_free(archive->extraction_dir);
				archive->extraction_dir = xa_escape_bad_chars(extraction_dir, ESCAPES);
				g_free(destination_path);
				destination_path = g_strdup(extraction_dir);
				g_free(extraction_dir);
				g_free (cur_dir);
			}
			if (archive->has_password)
				archive->password  = g_strdup(gtk_entry_get_text(GTK_ENTRY(dialog_data->password_entry)));

			if (archive->has_password && strlen(archive->password)== 0 )
			{
				xa_show_message_dialog (GTK_WINDOW (xa_main_window),GTK_DIALOG_MODAL,GTK_MESSAGE_ERROR,GTK_BUTTONS_OK,_("This archive is encrypted!"),_("Please enter the password."));
				break;
			}

			if (g_file_test (destination_path,G_FILE_TEST_IS_DIR)&& access (destination_path,R_OK | W_OK | X_OK ))
			{
				gchar *utf8_path;
				gchar  *msg;

                utf8_path = g_filename_to_utf8 (destination_path,-1,NULL,NULL,NULL);
                msg = g_strdup_printf (_("You don't have the right permissions to extract the files to the directory \"%s\"."),utf8_path);
                xa_show_message_dialog (GTK_WINDOW (xa_main_window),GTK_DIALOG_MODAL,GTK_MESSAGE_ERROR,GTK_BUTTONS_OK,_("Can't perform extraction!"),msg );
                g_free (utf8_path);
				g_free (msg);
				g_free (destination_path);
				break;
			}
			done = TRUE;

			if (gtk_widget_is_sensitive(dialog_data->ensure_directory) &&
			    gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(dialog_data->ensure_directory)))
			{
				if (!gtk_widget_is_sensitive(dialog_data->extract_full) ||
				    !gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(dialog_data->extract_full)) ||
				    !xa_has_containing_directory(archive))
				{
					gchar *extraction_dir;

					extraction_dir = xa_create_containing_directory(archive, destination_path);

					if (!extraction_dir)
					{
						xa_show_message_dialog(GTK_WINDOW(xa_main_window), GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK, _("Can't create directory!"), "");
						g_free(destination_path);
						break;
					}

					g_free(archive->extraction_dir);
					archive->extraction_dir = xa_escape_bad_chars(extraction_dir, ESCAPES);
					g_free(extraction_dir);
				}
			}

			g_free (destination_path);

			if (gtk_widget_is_sensitive(dialog_data->overwrite_check))
				archive->do_overwrite = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(dialog_data->overwrite_check));
			if (gtk_widget_is_sensitive(dialog_data->touch))
				archive->do_touch = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(dialog_data->touch));
			if (xa_main_window && gtk_widget_is_sensitive(dialog_data->extract_full))
				archive->do_full_path = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(dialog_data->extract_full));
			else
				archive->do_full_path = archive->can_full_path[0];

			if (gtk_widget_is_sensitive(dialog_data->fresh))
				archive->do_freshen = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(dialog_data->fresh));
			if (gtk_widget_is_sensitive(dialog_data->update))
				archive->do_update = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(dialog_data->update));

			gtk_widget_hide (dialog_data->dialog1);
			/* Is the radiobutton Files selected? */
			if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON (dialog_data->files_radio)))
			{
				model = gtk_tree_view_get_model(GTK_TREE_VIEW(archive->treeview));
				string = g_strdup (gtk_entry_get_text(GTK_ENTRY(dialog_data->entry2)));
				gtk_tree_model_foreach(model, select_matched_rows, string);
				selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(archive->treeview));
				gtk_tree_selection_selected_foreach(selection,(GtkTreeSelectionForeachFunc)xa_concat_selected_filenames,&names);
				g_free(string);
			}
			else if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON (dialog_data->selected_radio)))
				gtk_tree_selection_selected_foreach(selection,(GtkTreeSelectionForeachFunc)xa_concat_selected_filenames,&names);
			if (xa_main_window)
			{
				xa_set_button_state(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, NULL, 0);
				gtk_label_set_text(GTK_LABEL(total_label),_("Extracting files from archive, please wait..."));
			}

			archive->status = XARCHIVESTATUS_EXTRACT;
			(*archive->archiver->extract)(archive,names);
		}
	}
	gtk_widget_hide (dialog_data->dialog1);
}

Multi_extract_data *xa_create_multi_extract_dialog()
{
	GTK_COMPAT_TOOLTIPS;
	Multi_extract_data *dialog_data;
	GtkWidget	*dialog_vbox1,*vbox1,*scrolledwindow1,*hbox1,*frame1,*alignment1,*vbox2,*hbox3,*remove_button,*add_button,*cancelbutton1;
	GtkWidget *hbox2, *vbox3, *alignment2, *alignment3, *label1, *label2, *frame2;
	GtkWidget *extract_button, *extract_image, *extract_hbox, *extract_label;
	GtkCellRenderer *renderer;
	GtkTreeSelection *selection;
	GtkTreeViewColumn *column;
	GSList *radiobutton1_group;
	char *column_names[]= {(_("Archive Name")),(_("Size")),(_("Path")),NULL};
	int x;

	dialog_data = g_new0 (Multi_extract_data,1);
	dialog_data->multi_extract = gtk_dialog_new();

	gtk_window_set_position (GTK_WINDOW (dialog_data->multi_extract),GTK_WIN_POS_CENTER_ON_PARENT);
	gtk_window_set_type_hint (GTK_WINDOW (dialog_data->multi_extract),GDK_WINDOW_TYPE_HINT_DIALOG);
	gtk_window_set_destroy_with_parent (GTK_WINDOW (dialog_data->multi_extract),TRUE);
	gtk_widget_set_size_request(dialog_data->multi_extract,-1,300);
	gtk_window_set_title (GTK_WINDOW (dialog_data->multi_extract),_("Multi-Extract"));

	xa_set_xarchiver_icon(GTK_WINDOW(dialog_data->multi_extract));

	dialog_vbox1 = gtk_dialog_get_content_area(GTK_DIALOG(dialog_data->multi_extract));
	vbox1 = gtk_vbox_new (FALSE,5);
	gtk_box_pack_start (GTK_BOX (dialog_vbox1),vbox1,TRUE,TRUE,0);
	scrolledwindow1 = gtk_scrolled_window_new (NULL,NULL);
	gtk_box_pack_start (GTK_BOX (vbox1),scrolledwindow1,TRUE,TRUE,0);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolledwindow1),GTK_POLICY_AUTOMATIC,GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolledwindow1),GTK_SHADOW_IN);

	dialog_data->files_liststore = gtk_list_store_new(5, G_TYPE_STRING, G_TYPE_UINT64, G_TYPE_STRING, G_TYPE_INT, G_TYPE_UINT);
	dialog_data->files_treeview = gtk_tree_view_new_with_model(GTK_TREE_MODEL(dialog_data->files_liststore));
	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(dialog_data->files_treeview));
	g_signal_connect (selection,"changed",G_CALLBACK (xa_multi_extract_dialog_selection_changed),dialog_data);

	for (x = 0; x < 3; x++)
	{
		renderer = gtk_cell_renderer_text_new();
		column = gtk_tree_view_column_new_with_attributes ( column_names[x],renderer,"text",x,NULL);
		gtk_tree_view_column_set_resizable (column,TRUE);
		gtk_tree_view_append_column(GTK_TREE_VIEW(dialog_data->files_treeview),column);
	}
	column = gtk_tree_view_column_new();
	gtk_tree_view_column_set_visible(column,FALSE);
	gtk_tree_view_append_column(GTK_TREE_VIEW(dialog_data->files_treeview),column);

	gtk_tree_view_set_rules_hint(GTK_TREE_VIEW(dialog_data->files_treeview),TRUE);
	gtk_container_add (GTK_CONTAINER (scrolledwindow1),dialog_data->files_treeview);

	gtk_drag_dest_set(dialog_data->files_treeview, GTK_DEST_DEFAULT_ALL, drop_targets, 1, GDK_ACTION_COPY);
	g_signal_connect (G_OBJECT (dialog_data->files_treeview),"drag-data-received",G_CALLBACK (xa_multi_extract_dialog_drag_data_received),dialog_data);

	hbox2 = gtk_hbox_new (TRUE,5);
	gtk_box_pack_start (GTK_BOX (vbox1),hbox2,FALSE,TRUE,0);

	add_button = gtk_button_new_from_stock ("gtk-add");
	gtk_box_pack_end (GTK_BOX (hbox2),add_button,FALSE,FALSE,0);
	gtk_button_set_focus_on_click (GTK_BUTTON (add_button),FALSE);
	g_signal_connect(add_button, "clicked", G_CALLBACK(xa_multi_extract_dialog_select_files_to_add), dialog_data);

	remove_button = gtk_button_new_from_stock ("gtk-remove");
	gtk_widget_set_sensitive (remove_button,FALSE);
	gtk_box_pack_end (GTK_BOX (hbox2),remove_button,FALSE,FALSE,0);
	gtk_button_set_focus_on_click (GTK_BUTTON (remove_button),FALSE);
	g_signal_connect(G_OBJECT(remove_button), "clicked", G_CALLBACK(xa_multi_extract_dialog_remove_files), dialog_data);
	g_signal_connect (G_OBJECT (dialog_data->files_liststore),"row-inserted",G_CALLBACK (xa_activate_remove_button),remove_button);

	/* Destination dirs frame */
	hbox1 = gtk_hbox_new (TRUE,8);
	gtk_box_pack_start (GTK_BOX (vbox1),hbox1,FALSE,TRUE,0);
	frame1 = gtk_frame_new (NULL);
	gtk_box_pack_start (GTK_BOX (hbox1),frame1,TRUE,TRUE,0);
	gtk_frame_set_shadow_type (GTK_FRAME (frame1),GTK_SHADOW_OUT);
	alignment1 = gtk_alignment_new (0.5,0.5,1,1);
	gtk_container_add (GTK_CONTAINER (frame1),alignment1);
	gtk_alignment_set_padding (GTK_ALIGNMENT (alignment1),0,0,12,0);

	vbox2 = gtk_vbox_new (TRUE,0);
	gtk_container_add (GTK_CONTAINER (alignment1),vbox2);
	hbox3 = gtk_hbox_new (FALSE,2);
	gtk_box_pack_start (GTK_BOX (vbox2),hbox3,FALSE,FALSE,0);
	dialog_data->extract_to = gtk_radio_button_new_with_mnemonic (NULL,_("Extract to:"));
	gtk_box_pack_start (GTK_BOX (hbox3),dialog_data->extract_to,FALSE,FALSE,0);
	radiobutton1_group = gtk_radio_button_get_group (GTK_RADIO_BUTTON (dialog_data->extract_to));

	dialog_data->entry1 = GTK_COMPAT_ENTRY_ICON_NEW();
	GTK_COMPAT_ENTRY_ICON(dialog_data->entry1, xa_select_where_to_extract, dialog_data);
	gtk_box_pack_start (GTK_BOX (hbox3),dialog_data->entry1,TRUE,TRUE,0);
	gtk_entry_set_activates_default(GTK_ENTRY(dialog_data->entry1), TRUE);

	dialog_data->extract_to_archive_name = gtk_radio_button_new_with_mnemonic(radiobutton1_group, _("Extract to dir \"Archive Name\""));
	gtk_widget_set_tooltip_text(dialog_data->extract_to_archive_name, _("This option extracts archives in directories named with the archive names"));
	gtk_box_pack_start (GTK_BOX (vbox2),dialog_data->extract_to_archive_name,FALSE,FALSE,0);
	label1 = gtk_label_new (_("Destination dirs:"));
	gtk_frame_set_label_widget (GTK_FRAME (frame1),label1);

	/* Option frame */
	frame2 = gtk_frame_new (NULL);
	gtk_box_pack_start (GTK_BOX (hbox1),frame2,TRUE,TRUE,0);
	gtk_frame_set_shadow_type (GTK_FRAME (frame2),GTK_SHADOW_OUT);
	alignment2 = gtk_alignment_new (0.5,0.5,1,1);
	gtk_container_add (GTK_CONTAINER (frame2),alignment2);
	gtk_alignment_set_padding (GTK_ALIGNMENT (alignment2),0,0,12,0);
	vbox3 = gtk_vbox_new (TRUE,0);
	gtk_container_add (GTK_CONTAINER (alignment2),vbox3);
	dialog_data->overwrite = gtk_check_button_new_with_mnemonic (_("Overwrite existing files"));
	gtk_box_pack_start (GTK_BOX (vbox3),dialog_data->overwrite,FALSE,FALSE,0);
	dialog_data->full_path = gtk_check_button_new_with_mnemonic (_("Extract pathnames"));
	gtk_box_pack_start (GTK_BOX (vbox3),dialog_data->full_path,FALSE,FALSE,0);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (dialog_data->full_path), TRUE);
	label2 = gtk_label_new (_("Options:"));
	gtk_frame_set_label_widget(GTK_FRAME(frame2),label2);

	cancelbutton1 = gtk_button_new_from_stock ("gtk-cancel");
	gtk_dialog_add_action_widget (GTK_DIALOG (dialog_data->multi_extract),cancelbutton1,GTK_RESPONSE_CANCEL);

	extract_button = gtk_button_new();
	extract_image = xa_main_window_find_image("xarchiver-extract.png",GTK_ICON_SIZE_SMALL_TOOLBAR);
	extract_hbox = gtk_hbox_new(FALSE,4);
	extract_label = gtk_label_new_with_mnemonic(_("_Extract"));

	alignment3 = gtk_alignment_new (0.5,0.5,0,0);
	gtk_container_add (GTK_CONTAINER (alignment3),extract_hbox);
	gtk_box_pack_start(GTK_BOX(extract_hbox),extract_image,FALSE,FALSE,0);
	gtk_box_pack_start(GTK_BOX(extract_hbox),extract_label,FALSE,FALSE,0);
	gtk_container_add(GTK_CONTAINER(extract_button),alignment3);

	gtk_dialog_add_action_widget (GTK_DIALOG (dialog_data->multi_extract),extract_button,GTK_RESPONSE_OK);
	gtk_widget_set_can_default(extract_button, TRUE);
	gtk_dialog_set_default_response (GTK_DIALOG (dialog_data->multi_extract),GTK_RESPONSE_OK);
	return dialog_data;
}

void xa_multi_extract_dialog_add_file (gchar *file_path, Multi_extract_data *dialog)
{
	GtkTreeIter iter;
	gchar *path, *path_utf8, *file, *file_utf8;
	ArchiveType xa;
	XArchive archive = {0};
	struct stat my_stat;
	guint64 file_size;

	xa = xa_detect_archive_type(file_path);

	if (xa.type == XARCHIVETYPE_UNKNOWN || xa.type == XARCHIVETYPE_NOT_FOUND)
		return;

	if (!archiver[xa.type].list)
	{
		xa_show_message_dialog(GTK_WINDOW(xa_main_window), GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK, _("Sorry, this archive format is not supported:"), _("The proper archiver is not installed!"));
		return;
	}

	archive.type = xa.type;
	archive.tag = xa.tag;

	(*archiver[xa.type].ask)(&archive);

	if (!archive.can_extract)
	{
		xa_show_message_dialog(GTK_WINDOW(xa_main_window), GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK, _("Can't extract files from the archive:"), _("The archiver doesn't support this feature!"));
		return;
	}

	stat (file_path,&my_stat);
	file_size = my_stat.st_size;
	path = xa_remove_level_from_path(file_path);
	path_utf8 = g_filename_display_name(path);
	file = g_path_get_basename(file_path);
	file_utf8 = g_filename_display_name(file);
	gtk_list_store_append(dialog->files_liststore,&iter);
	gtk_list_store_set(dialog->files_liststore, &iter, 0, file_utf8, 1, file_size, 2, path_utf8, 3, xa.type, 4, xa.tag, -1);
	dialog->nr++;
	g_free(file_utf8);
	g_free(file);
	g_free(path_utf8);
	g_free(path);
}

void xa_multi_extract_dialog (Multi_extract_data *dialog)
{
	GtkTreeIter iter;
	gchar *filename, *filename_local, *file, *path, *message, *name, *dest_path = NULL;
	GString *output = g_string_new("");
	gboolean overwrite = FALSE, full_path;
	gint response;
	double percent = 0.0;

	gtk_widget_show_all(dialog->multi_extract);
run:
	response = gtk_dialog_run(GTK_DIALOG(dialog->multi_extract));
	if (response == GTK_RESPONSE_CANCEL || response == GTK_RESPONSE_DELETE_EVENT)
	{
		gtk_list_store_clear(dialog->files_liststore);
		gtk_widget_hide(dialog->multi_extract);
		return;
	}
	if (gtk_tree_model_get_iter_first(GTK_TREE_MODEL(dialog->files_liststore),&iter)== FALSE)
	{
		xa_show_message_dialog(GTK_WINDOW (xa_main_window),GTK_DIALOG_MODAL,GTK_MESSAGE_ERROR,GTK_BUTTONS_OK,_("Can't multi-extract archives:"),_("You haven't added any of them!"));
		goto run;
	}
	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(dialog->extract_to)))
	{
		dest_path = g_filename_from_utf8(gtk_entry_get_text(GTK_ENTRY(dialog->entry1)), -1, NULL, NULL, NULL);
		if (strlen(dest_path)== 0)
		{
			xa_show_message_dialog (GTK_WINDOW (xa_main_window),GTK_DIALOG_MODAL,GTK_MESSAGE_ERROR,GTK_BUTTONS_OK,_("You missed where to extract the files!"),_("Please fill the \"Extract to\" field!"));
			goto run;
		}
	}
	gtk_widget_hide(dialog->multi_extract);

	if (gtk_widget_is_sensitive(dialog->overwrite))
		overwrite = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(dialog->overwrite));
	double fraction = 1.0 / dialog->nr;

	xa_show_progress_bar(NULL);

	do
	{
		gtk_tree_model_get(GTK_TREE_MODEL(dialog->files_liststore), &iter, 0, &file, 2, &path, -1);
		full_path = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(dialog->full_path));
		filename = g_strconcat (path,"/",file,NULL);
		xa_increase_progress_bar(progress, filename, percent);
		g_free(file);
		g_free(path);
		filename_local = g_filename_from_utf8(filename, -1, NULL, NULL, NULL);
		message = xa_multi_extract_one_archive(dialog, filename_local, overwrite, full_path, dest_path);
		g_free(filename_local);
		if (message != NULL)
		{
			name = g_strconcat(filename,": ",message,"\n",NULL);
			g_string_append(output,name);
		}
		g_free(filename);
		if (dialog->stop_pressed)
			break;
		percent += fraction;

		xa_increase_progress_bar(progress, NULL, percent);
	}
	while (gtk_tree_model_iter_next (GTK_TREE_MODEL(dialog->files_liststore),&iter));

	/* let the user see the progress of 100% */
	gtk_main_iteration();
	g_usleep(G_USEC_PER_SEC / 4);

	if (strlen(output->str)> 0)
		xa_show_message_dialog (GTK_WINDOW (xa_main_window),GTK_DIALOG_MODAL,GTK_MESSAGE_ERROR,GTK_BUTTONS_OK,_("Some errors occurred:"),output->str );

	g_string_free(output,TRUE);
	if (dest_path != NULL)
		g_free(dest_path);

	dialog->nr=0;
	gtk_list_store_clear(dialog->files_liststore);
}
