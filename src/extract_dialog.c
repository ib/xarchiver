/*
 *  Copyright (C)2008 Giuseppe Torelli - <colossus73@gmail.com>
 *  Copyright (C) 2016 Ingo Brückl
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

static const GtkTargetEntry drop_targets[] =
{
	{"text/uri-list", 0, 0}
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

static void xa_toggle_all_files_radio (GtkToggleButton *button, ExtractDialog *extract_dialog)
{
	gtk_widget_set_sensitive(extract_dialog->ensure_directory, gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(extract_dialog->all_files)));
}

static void xa_toggle_relative_path_radio (GtkToggleButton *button, ExtractDialog *extract_dialog)
{
	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(extract_dialog->relative_path)))
	{
		gtk_widget_set_sensitive(extract_dialog->touch, FALSE);
		gtk_widget_set_sensitive(extract_dialog->freshen, FALSE);
	}
	else
	{
		gtk_widget_set_sensitive(extract_dialog->touch, extract_dialog->archive->can_touch);
		gtk_widget_set_sensitive(extract_dialog->freshen, extract_dialog->archive->can_freshen[0]);
	}
}

static void xa_activate_entry (GtkToggleButton *button, ExtractDialog *extract_dialog)
{
	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(extract_dialog->specified_files)))
	{
		gtk_widget_set_sensitive(extract_dialog->specified_files_entry, TRUE);
		gtk_widget_grab_focus(extract_dialog->specified_files_entry);
	}
	else
		gtk_widget_set_sensitive(extract_dialog->specified_files_entry, FALSE);
}

static void xa_multi_extract_dialog_drag_data_received (GtkWidget *widget, GdkDragContext *context, gint x, gint y, GtkSelectionData *data, guint info, guint time, gpointer dialog_data)
{
	gchar **uris;
	gchar *filename;
	unsigned int n = 0;

	uris = gtk_selection_data_get_uris(data);

	if (!uris)
	{
		xa_show_message_dialog(GTK_WINDOW(xa_main_window), GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK, "", _("Sorry, I could not perform the operation!"));
		gtk_drag_finish(context, FALSE, FALSE, time);
		return;
	}

	while (uris[n])
	{
		filename = g_filename_from_uri(uris[n], NULL, NULL);

		if (filename)
			xa_multi_extract_dialog_add_file(filename, dialog_data);

		g_free(filename);
		n++;
	}

	g_strfreev(uris);
	gtk_drag_finish(context, TRUE, FALSE, time);
}

static void xa_multi_extract_dialog_select_files_to_add (GtkButton *button, MultiExtractDialog *multi_extract)
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
		g_slist_foreach(dummy, (GFunc) xa_multi_extract_dialog_add_file, multi_extract);
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

static void remove_foreach_func (GtkTreeModel *model, GtkTreePath *path, GtkTreeIter *iter, GSList **rowref_list)
{
	GtkTreeRowReference *rowref;

	rowref = gtk_tree_row_reference_new(model,path);
	*rowref_list = g_slist_append(*rowref_list, rowref);
}

static void xa_multi_extract_dialog_remove_files (GtkButton *button, MultiExtractDialog *multi_extract)
{
	GtkTreeModel *model;
	GtkTreeSelection *sel;
	GtkTreePath *path;
	GtkTreeIter iter;
	GSList *rr_list = NULL;
	GSList *node;

	model = gtk_tree_view_get_model(GTK_TREE_VIEW(multi_extract->treeview));
	sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(multi_extract->treeview));
	gtk_tree_selection_selected_foreach(sel,(GtkTreeSelectionForeachFunc)remove_foreach_func,&rr_list);

	for (node = rr_list; node != NULL; node = node->next)
	{
		path = gtk_tree_row_reference_get_path((GtkTreeRowReference *)node->data);
		if (path)
		{
			if (gtk_tree_model_get_iter(model, &iter, path))
			{
				gboolean overwrite, full_path;

				gtk_tree_model_get(model, &iter, 5, &overwrite, 6, &full_path, -1);
				gtk_list_store_remove(multi_extract->liststore, &iter);

				multi_extract->nr_total--;

				if (!overwrite)
					multi_extract->nr_no_overwrite--;

				if (!full_path)
					multi_extract->nr_no_full_path--;
			}
			gtk_tree_path_free(path);
		}
	}

	gtk_widget_set_sensitive(GTK_WIDGET(button), multi_extract->nr_total);
	gtk_widget_set_sensitive(multi_extract->overwrite, multi_extract->nr_no_overwrite == 0);
	gtk_widget_set_sensitive(multi_extract->full_path, multi_extract->nr_no_full_path == 0);

	g_slist_free_full(rr_list, (GDestroyNotify) gtk_tree_row_reference_free);
}

static gchar *xa_multi_extract_one_archive (MultiExtractDialog *multi_extract, gchar *filename, gboolean overwrite, gboolean full_path, gchar *dest_path)
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
	multi_extract->archive = archive;
	archive->do_overwrite = overwrite;
	archive->do_update = FALSE;
	archive->do_freshen = FALSE;
	archive->do_touch = FALSE;
	archive->do_full_path = full_path;
	archive->path[0] = g_strdup(filename);
	archive->path[1] = xa_escape_bad_chars(filename, ESCAPES);
	archive->extraction_dir = xa_escape_bad_chars(dest_path, ESCAPES);

	/* temporarily enter batch mode */
	main_window = xa_main_window;
	xa_main_window = NULL;

	xa_detect_encrypted_archive(archive);

	if (archive->has_password && !xa_check_password(archive))
		error = _("You missed the password!");

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

static void xa_multi_extract_dialog_clear (MultiExtractDialog *multi_extract)
{
	gtk_list_store_clear(multi_extract->liststore);

	multi_extract->nr_total = 0;
	multi_extract->nr_no_overwrite = 0;
	multi_extract->nr_no_full_path = 0;

	gtk_widget_set_sensitive(multi_extract->remove, FALSE);
	gtk_widget_set_sensitive(multi_extract->overwrite, TRUE);
	gtk_widget_set_sensitive(multi_extract->full_path, TRUE);
}

static void toggle_overwrite_update_freshen (GtkToggleButton *button, ExtractDialog *extract_dialog)
{
	gboolean active = gtk_toggle_button_get_active(button);

	if (active)
	{
		if (button != GTK_TOGGLE_BUTTON(extract_dialog->overwrite))
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(extract_dialog->overwrite), FALSE);

		if (button != GTK_TOGGLE_BUTTON(extract_dialog->update))
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(extract_dialog->update), FALSE);

		if (button != GTK_TOGGLE_BUTTON(extract_dialog->freshen))
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(extract_dialog->freshen), FALSE);
	}
}

ExtractDialog *xa_create_extract_dialog ()
{
	GTK_COMPAT_TOOLTIPS;
	ExtractDialog *extract_dialog;
	GtkWidget *vbox, *vbox2, *label, *alignment, *hbox, *frame, *button, *image;
	GSList *group;

	extract_dialog = g_new0(ExtractDialog, 1);

	extract_dialog->dialog = gtk_dialog_new();
	gtk_window_set_title(GTK_WINDOW(extract_dialog->dialog), _("Extract files"));
	gtk_window_set_position(GTK_WINDOW(extract_dialog->dialog), GTK_WIN_POS_CENTER_ON_PARENT);
	gtk_window_set_type_hint(GTK_WINDOW(extract_dialog->dialog), GDK_WINDOW_TYPE_HINT_DIALOG);
	xa_set_xarchiver_icon(GTK_WINDOW(extract_dialog->dialog));

	vbox = gtk_vbox_new(FALSE, 4);
	gtk_box_pack_start(GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(extract_dialog->dialog))), vbox, TRUE, TRUE, 0);
	gtk_container_set_border_width(GTK_CONTAINER(vbox), 2);

	vbox2 = gtk_vbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), vbox2, FALSE, FALSE, 0);

	label = gtk_label_new(_("Extract to:"));

	alignment = gtk_alignment_new(0, 0.5, 0, 1);
	gtk_container_add(GTK_CONTAINER(alignment), label);
	gtk_alignment_set_padding(GTK_ALIGNMENT(alignment), 2, 4, 0, 0);
	gtk_box_pack_start(GTK_BOX(vbox2), alignment, FALSE, FALSE, 0);

	extract_dialog->destination_path_entry = GTK_COMPAT_ENTRY_ICON_NEW();
	GTK_COMPAT_ENTRY_ICON(extract_dialog->destination_path_entry, xa_select_where_to_extract, extract_dialog);
	gtk_box_pack_start(GTK_BOX(vbox2), extract_dialog->destination_path_entry, FALSE, FALSE, 0);
	gtk_entry_set_activates_default(GTK_ENTRY(extract_dialog->destination_path_entry), TRUE);

	extract_dialog->ensure_directory = gtk_check_button_new_with_mnemonic(_("Ensure a containing directory"));
	gtk_widget_set_tooltip_text(extract_dialog->ensure_directory, _("Ensure that the contents of the extracted archive is always in a containing directory."));
	gtk_button_set_focus_on_click(GTK_BUTTON(extract_dialog->ensure_directory), FALSE);

	alignment = gtk_alignment_new(0, 0.5, 1, 1);
	gtk_container_add(GTK_CONTAINER(alignment), extract_dialog->ensure_directory);
	gtk_alignment_set_padding(GTK_ALIGNMENT(alignment), 2, 0, 14, 2);
	gtk_box_pack_start(GTK_BOX(vbox2), alignment, FALSE, FALSE, 0);

	frame = gtk_frame_new(_("Files"));
	gtk_box_pack_start(GTK_BOX(vbox), frame, TRUE, TRUE, 0);
	gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_OUT);

	alignment = gtk_alignment_new(0.5, 0.5, 1, 1);
	gtk_container_add(GTK_CONTAINER(frame), alignment);
	gtk_alignment_set_padding(GTK_ALIGNMENT(alignment), 0, 2, 12, 2);

	vbox2 = gtk_vbox_new(TRUE, 0);
	gtk_container_add(GTK_CONTAINER(alignment), vbox2);

	extract_dialog->all_files = gtk_radio_button_new_with_mnemonic(NULL, _("All files"));
	gtk_box_pack_start(GTK_BOX(vbox2), extract_dialog->all_files, TRUE, FALSE, 0);
	gtk_button_set_focus_on_click(GTK_BUTTON(extract_dialog->all_files), FALSE);
	g_signal_connect(extract_dialog->all_files, "toggled", G_CALLBACK(xa_toggle_all_files_radio), extract_dialog);

	group = gtk_radio_button_get_group(GTK_RADIO_BUTTON(extract_dialog->all_files));

	extract_dialog->selected_files = gtk_radio_button_new_with_mnemonic(group, _("Selected files"));
	gtk_box_pack_start(GTK_BOX(vbox2), extract_dialog->selected_files, TRUE, FALSE, 0);
	gtk_button_set_focus_on_click(GTK_BUTTON(extract_dialog->selected_files), FALSE);

	group = gtk_radio_button_get_group(GTK_RADIO_BUTTON(extract_dialog->selected_files));

	hbox = gtk_hbox_new(FALSE, 4);
	gtk_box_pack_start(GTK_BOX(vbox2), hbox, TRUE, FALSE, 0);

	extract_dialog->specified_files = gtk_radio_button_new_with_mnemonic(group, _("Files:"));
	gtk_box_pack_start(GTK_BOX(hbox), extract_dialog->specified_files, FALSE, FALSE, 0);
	gtk_button_set_focus_on_click(GTK_BUTTON(extract_dialog->specified_files), FALSE);

	extract_dialog->specified_files_entry = gtk_entry_new();
	gtk_box_pack_start(GTK_BOX(hbox), extract_dialog->specified_files_entry, TRUE, TRUE, 0);
	gtk_widget_set_sensitive(extract_dialog->specified_files_entry, FALSE);
	g_signal_connect(G_OBJECT(extract_dialog->specified_files), "toggled", G_CALLBACK(xa_activate_entry), extract_dialog);

	frame = gtk_frame_new(_("File Paths"));
	gtk_box_pack_start(GTK_BOX(vbox), frame, TRUE, TRUE, 0);
	gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_OUT);

	alignment = gtk_alignment_new(0.5, 0.5, 1, 1);
	gtk_container_add(GTK_CONTAINER(frame), alignment);
	gtk_alignment_set_padding(GTK_ALIGNMENT(alignment), 0, 2, 12, 2);

	vbox2 = gtk_vbox_new(TRUE, 0);
	gtk_container_add(GTK_CONTAINER(alignment), vbox2);

	extract_dialog->full_path = gtk_radio_button_new_with_mnemonic(NULL, _("With full path"));
	gtk_widget_set_tooltip_text(extract_dialog->full_path, _("The archive's complete directory structure is recreated in the extraction directory."));
	gtk_box_pack_start(GTK_BOX(vbox2), extract_dialog->full_path, TRUE, FALSE, 0);
	gtk_button_set_focus_on_click(GTK_BUTTON(extract_dialog->full_path), FALSE);

	group = gtk_radio_button_get_group(GTK_RADIO_BUTTON(extract_dialog->full_path));

	extract_dialog->relative_path = gtk_radio_button_new_with_mnemonic(group, _("Without parent path"));
	gtk_widget_set_tooltip_text(extract_dialog->relative_path, _("The archive's directory structure is recreated in the extraction directory, but with the parent directories of the selected files removed."));
	gtk_box_pack_start(GTK_BOX(vbox2), extract_dialog->relative_path, TRUE, FALSE, 0);
	gtk_button_set_focus_on_click(GTK_BUTTON(extract_dialog->relative_path), FALSE);
	g_signal_connect(extract_dialog->relative_path, "toggled", G_CALLBACK(xa_toggle_relative_path_radio), extract_dialog);

	group = gtk_radio_button_get_group(GTK_RADIO_BUTTON(extract_dialog->relative_path));

	extract_dialog->without_path = gtk_radio_button_new_with_mnemonic(group, _("Without any path"));
	gtk_widget_set_tooltip_text(extract_dialog->without_path, _("The archive's directory structure is not recreated; all files are placed in the extraction directory."));
	gtk_box_pack_start(GTK_BOX(vbox2), extract_dialog->without_path, TRUE, FALSE, 0);
	gtk_button_set_focus_on_click(GTK_BUTTON(extract_dialog->without_path), FALSE);

	frame = gtk_frame_new(_("Options"));
	gtk_box_pack_start(GTK_BOX(vbox), frame, TRUE, TRUE, 0);
	gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_OUT);

	alignment = gtk_alignment_new(0.5, 0.5, 1, 1);
	gtk_container_add(GTK_CONTAINER(frame), alignment);
	gtk_alignment_set_padding(GTK_ALIGNMENT(alignment), 0, 2, 12, 2);

	vbox2 = gtk_vbox_new(TRUE, 0);
	gtk_container_add(GTK_CONTAINER(alignment), vbox2);

	extract_dialog->touch = gtk_check_button_new_with_mnemonic(_("Touch files"));
	gtk_widget_set_tooltip_text(extract_dialog->touch, _("When this option is used, the modification times of the files will be the times of extraction instead of the times recorded in the archive."));
	gtk_box_pack_start(GTK_BOX(vbox2), extract_dialog->touch, TRUE, FALSE, 0);
	gtk_button_set_focus_on_click(GTK_BUTTON(extract_dialog->touch), FALSE);

	extract_dialog->overwrite = gtk_check_button_new_with_mnemonic(_("Overwrite existing files"));
	gtk_box_pack_start(GTK_BOX(vbox2), extract_dialog->overwrite, TRUE, FALSE, 0);
	gtk_button_set_focus_on_click(GTK_BUTTON(extract_dialog->overwrite), FALSE);
	g_signal_connect(G_OBJECT(extract_dialog->overwrite), "toggled", G_CALLBACK(toggle_overwrite_update_freshen), extract_dialog);

	extract_dialog->update = gtk_check_button_new_with_mnemonic(_("Update existing files"));
	gtk_widget_set_tooltip_text(extract_dialog->update, _("This option performs the same function as the freshen one, extracting files that are newer than those with the same name on disk, and in addition it extracts those files that do not already exist on disk."));
	gtk_box_pack_start(GTK_BOX(vbox2), extract_dialog->update, TRUE, FALSE, 0);
	gtk_button_set_focus_on_click(GTK_BUTTON(extract_dialog->update), FALSE);
	g_signal_connect(G_OBJECT(extract_dialog->update), "toggled", G_CALLBACK(toggle_overwrite_update_freshen), extract_dialog);

	extract_dialog->freshen = gtk_check_button_new_with_mnemonic(_("Freshen existing files only"));
	gtk_widget_set_tooltip_text(extract_dialog->freshen, _("Extract only those files that already exist on disk and that are newer than the disk copies."));
	gtk_box_pack_start(GTK_BOX(vbox2), extract_dialog->freshen, TRUE, FALSE, 0);
	gtk_button_set_focus_on_click(GTK_BUTTON(extract_dialog->freshen), FALSE);
	g_signal_connect(G_OBJECT(extract_dialog->freshen), "toggled", G_CALLBACK(toggle_overwrite_update_freshen), extract_dialog);

	hbox = gtk_hbox_new(FALSE, 4);
	gtk_box_pack_start(GTK_BOX(vbox2), hbox, TRUE, FALSE, 0);

	extract_dialog->label_password = gtk_label_new(_("Password:"));
	gtk_misc_set_padding(GTK_MISC(extract_dialog->label_password), 3, 0);
	gtk_box_pack_start(GTK_BOX(hbox), extract_dialog->label_password, FALSE, FALSE, 0);

	extract_dialog->password_entry = gtk_entry_new();
	gtk_box_pack_start(GTK_BOX(hbox), extract_dialog->password_entry, TRUE, TRUE, 0);
	gtk_entry_set_visibility(GTK_ENTRY(extract_dialog->password_entry), FALSE);
	gtk_entry_set_activates_default(GTK_ENTRY(extract_dialog->password_entry), TRUE);

	button = gtk_button_new_from_stock("gtk-cancel");
	gtk_dialog_add_action_widget(GTK_DIALOG(extract_dialog->dialog), button, GTK_RESPONSE_CANCEL);

	button = gtk_button_new();

	image = xa_main_window_find_image("xarchiver-extract.png", GTK_ICON_SIZE_BUTTON);
	label = gtk_label_new_with_mnemonic(_("_Extract"));

	hbox = gtk_hbox_new(FALSE, 4);
	gtk_box_pack_start(GTK_BOX(hbox), image, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);

	alignment = gtk_alignment_new(0.5, 0.5, 0, 1);
	gtk_container_add(GTK_CONTAINER(alignment), hbox);
	gtk_container_add(GTK_CONTAINER(button), alignment);

	gtk_dialog_add_action_widget(GTK_DIALOG(extract_dialog->dialog), button, GTK_RESPONSE_OK);
	gtk_widget_set_can_default(button, TRUE);

	gtk_dialog_set_default_response(GTK_DIALOG(extract_dialog->dialog), GTK_RESPONSE_OK);

	return extract_dialog;
}

void xa_set_extract_dialog_options (ExtractDialog *extract_dialog, gint selected, XArchive *archive)
{
	extract_dialog->archive = archive;

	if (progress)
		gtk_widget_hide(progress->window);

	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(prefs_window->save_geometry))&& prefs_window->extract_win_size[0] != -1)
		gtk_window_set_default_size(GTK_WINDOW(extract_dialog->dialog), prefs_window->extract_win_size[0], prefs_window->extract_win_size[1]);

	prefs_window->size_changed[0] = TRUE;

	gtk_widget_set_sensitive(extract_dialog->specified_files, archive->files > 1);

		if (selected)
		{
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(extract_dialog->selected_files), TRUE);
			gtk_widget_set_sensitive(extract_dialog->selected_files, TRUE);
		}
		else
		{
			gtk_widget_set_sensitive(extract_dialog->selected_files, FALSE);
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(extract_dialog->all_files), TRUE);
		}

	gtk_widget_set_sensitive(extract_dialog->full_path, archive->can_full_path[0]);
	gtk_widget_set_sensitive(extract_dialog->relative_path, archive->can_full_path[0]);
	gtk_widget_set_sensitive(extract_dialog->touch, archive->can_touch);
	gtk_widget_set_sensitive(extract_dialog->overwrite, archive->can_overwrite);
	gtk_widget_set_sensitive(extract_dialog->update, archive->can_update[0]);
	gtk_widget_set_sensitive(extract_dialog->freshen, archive->can_freshen[0]);

	if (!archive->can_full_path[0])
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(extract_dialog->without_path), TRUE);

	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(extract_dialog->relative_path)))
		xa_toggle_relative_path_radio(NULL, extract_dialog);

	if ((!gtk_widget_is_sensitive(extract_dialog->update) && gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(extract_dialog->update))) ||
	    (!gtk_widget_is_sensitive(extract_dialog->freshen) && gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(extract_dialog->freshen))))
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(extract_dialog->overwrite), TRUE);

	if (!archive->destination_path)
	{
		gchar *archive_dir = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(prefs_window->preferred_extract_dir));

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
	gtk_entry_set_text(GTK_ENTRY(extract_dialog->destination_path_entry), archive->destination_path);
	gtk_entry_set_text(GTK_ENTRY(extract_dialog->password_entry), archive->password ? archive->password : "");

	gtk_widget_set_sensitive(extract_dialog->label_password, archive->has_password);
	gtk_widget_set_sensitive(extract_dialog->password_entry, archive->has_password);

	gtk_widget_show_all(extract_dialog->dialog);
}

void xa_parse_extract_dialog_options (XArchive *archive, ExtractDialog *extract_dialog, GtkTreeSelection *selection)
{
	gchar *destination_path, *string;
	gboolean strip, done = FALSE;
	GSList *names = NULL;
	GtkTreeModel *model;

    while (! done)
	{
		switch (gtk_dialog_run(GTK_DIALOG(extract_dialog->dialog)))
		{
			case GTK_RESPONSE_CANCEL:
			case GTK_RESPONSE_DELETE_EVENT:
			done = TRUE;
			break;

			case GTK_RESPONSE_OK:
			g_free(archive->destination_path);
			archive->destination_path = g_strdup(gtk_entry_get_text(GTK_ENTRY(extract_dialog->destination_path_entry)));
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
				archive->password = g_strdup(gtk_entry_get_text(GTK_ENTRY(extract_dialog->password_entry)));

			if (archive->has_password && strlen(archive->password)== 0 )
			{
				xa_show_message_dialog (GTK_WINDOW (xa_main_window),GTK_DIALOG_MODAL,GTK_MESSAGE_ERROR,GTK_BUTTONS_OK,_("This archive is encrypted!"),_("Please enter the password."));
				break;
			}

			if (g_file_test (destination_path,G_FILE_TEST_IS_DIR)&& access (destination_path,R_OK | W_OK | X_OK ))
			{
				gchar *path_utf8;
				gchar  *msg;

                path_utf8 = g_filename_display_name(destination_path);
                msg = g_strdup_printf (_("You don't have the right permissions to extract the files to the directory \"%s\"."),path_utf8);
                xa_show_message_dialog (GTK_WINDOW (xa_main_window),GTK_DIALOG_MODAL,GTK_MESSAGE_ERROR,GTK_BUTTONS_OK,_("Can't perform extraction!"),msg );
                g_free (path_utf8);
				g_free (msg);
				g_free (destination_path);
				break;
			}
			done = TRUE;

			if (gtk_widget_is_sensitive(extract_dialog->ensure_directory) &&
			    gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(extract_dialog->ensure_directory)))
			{
				if (!gtk_widget_is_sensitive(extract_dialog->full_path) ||
				    !gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(extract_dialog->full_path)) ||
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

			if (gtk_widget_is_sensitive(extract_dialog->overwrite))
				archive->do_overwrite = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(extract_dialog->overwrite));
			if (gtk_widget_is_sensitive(extract_dialog->touch))
				archive->do_touch = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(extract_dialog->touch));

			if (gtk_widget_is_sensitive(extract_dialog->full_path) && gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(extract_dialog->full_path)))
			{
				archive->do_full_path = TRUE;
				strip = FALSE;
			}
			else if (gtk_widget_is_sensitive(extract_dialog->relative_path) && gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(extract_dialog->relative_path)))
			{
				archive->do_full_path = TRUE;
				strip = TRUE;
			}
			else
			{
				archive->do_full_path = FALSE;
				strip = FALSE;
			}

			if (gtk_widget_is_sensitive(extract_dialog->freshen))
				archive->do_freshen = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(extract_dialog->freshen));
			if (gtk_widget_is_sensitive(extract_dialog->update))
				archive->do_update = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(extract_dialog->update));

			gtk_widget_hide(extract_dialog->dialog);
			/* Is the radiobutton Files selected? */
			if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(extract_dialog->specified_files)))
			{
				model = gtk_tree_view_get_model(GTK_TREE_VIEW(archive->treeview));
				string = g_strdup(gtk_entry_get_text(GTK_ENTRY(extract_dialog->specified_files_entry)));
				gtk_tree_model_foreach(model, select_matched_rows, string);
				selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(archive->treeview));
				gtk_tree_selection_selected_foreach(selection,(GtkTreeSelectionForeachFunc)xa_concat_selected_filenames,&names);
				g_free(string);
			}
			else if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(extract_dialog->selected_files)))
				gtk_tree_selection_selected_foreach(selection,(GtkTreeSelectionForeachFunc)xa_concat_selected_filenames,&names);
			if (xa_main_window)
				xa_set_button_state(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, NULL, 0);

			xa_execute_extract_commands(archive, names, strip);
		}
	}
	gtk_widget_hide(extract_dialog->dialog);
}

MultiExtractDialog *xa_create_multi_extract_dialog ()
{
	MultiExtractDialog *multi_extract;
	GtkWidget *vbox, *window, *alignment, *hbox, *hbox2, *button, *frame, *image, *label;
	GtkCellRenderer *renderer;
	int col;
	GSList *group;

	multi_extract = g_new0(MultiExtractDialog, 1);

	multi_extract->dialog = gtk_dialog_new();
	gtk_window_set_title(GTK_WINDOW(multi_extract->dialog), _("Multi-Extract"));
	gtk_window_set_position(GTK_WINDOW(multi_extract->dialog), GTK_WIN_POS_CENTER);
	gtk_window_set_type_hint(GTK_WINDOW(multi_extract->dialog), GDK_WINDOW_TYPE_HINT_DIALOG);
	gtk_widget_set_size_request(multi_extract->dialog, -1, 312);
	xa_set_xarchiver_icon(GTK_WINDOW(multi_extract->dialog));

	vbox = gtk_vbox_new(FALSE, 6);
	gtk_box_pack_start(GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(multi_extract->dialog))), vbox, TRUE, TRUE, 0);
	gtk_container_set_border_width(GTK_CONTAINER(vbox), 2);

	window = gtk_scrolled_window_new(NULL, NULL);
	gtk_box_pack_start(GTK_BOX(vbox), window, TRUE, TRUE, 0);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(window), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(window), GTK_SHADOW_IN);

	multi_extract->liststore = gtk_list_store_new(7, G_TYPE_STRING, G_TYPE_UINT64, G_TYPE_STRING, G_TYPE_INT, G_TYPE_UINT, G_TYPE_BOOLEAN, G_TYPE_BOOLEAN);
	multi_extract->treeview = gtk_tree_view_new_with_model(GTK_TREE_MODEL(multi_extract->liststore));

	gtk_tree_selection_set_mode(gtk_tree_view_get_selection(GTK_TREE_VIEW(multi_extract->treeview)), GTK_SELECTION_MULTIPLE);

	gtk_container_add(GTK_CONTAINER(window), multi_extract->treeview);
	gtk_tree_view_set_rules_hint(GTK_TREE_VIEW(multi_extract->treeview), TRUE);
	gtk_drag_dest_set(multi_extract->treeview, GTK_DEST_DEFAULT_ALL, drop_targets, G_N_ELEMENTS(drop_targets), GDK_ACTION_COPY);
	g_signal_connect(G_OBJECT(multi_extract->treeview), "drag-data-received", G_CALLBACK(xa_multi_extract_dialog_drag_data_received), multi_extract);

	renderer = gtk_cell_renderer_text_new();

	for (col = 0; col < 3; col++)
	{
		GtkTreeViewColumn *column;
		char *column_names[] = {_("Archive"), _("Size"), _("Path")};

		column = gtk_tree_view_column_new_with_attributes(column_names[col], renderer, "text", col, NULL);
		gtk_tree_view_append_column(GTK_TREE_VIEW(multi_extract->treeview), column);
		gtk_tree_view_column_set_resizable(column, TRUE);
	}

	alignment = gtk_alignment_new(0.5, 0.5, 1, 1);
	gtk_alignment_set_padding(GTK_ALIGNMENT(alignment), 4, 0, 0, 0);
	gtk_box_pack_start(GTK_BOX(vbox), alignment, FALSE, FALSE, 0);

	hbox = gtk_hbox_new(TRUE, 0);
	gtk_container_add(GTK_CONTAINER(alignment), hbox);

	button = gtk_button_new_from_stock("gtk-add");
	gtk_box_pack_end(GTK_BOX(hbox), button, TRUE, FALSE, 0);
	gtk_button_set_focus_on_click(GTK_BUTTON(button), FALSE);
	g_signal_connect(button, "clicked", G_CALLBACK(xa_multi_extract_dialog_select_files_to_add), multi_extract);

	multi_extract->remove = gtk_button_new_from_stock("gtk-remove");
	gtk_box_pack_end(GTK_BOX(hbox), multi_extract->remove, TRUE, FALSE, 0);
	gtk_widget_set_sensitive(multi_extract->remove, FALSE);
	gtk_button_set_focus_on_click(GTK_BUTTON(multi_extract->remove), FALSE);
	g_signal_connect(G_OBJECT(multi_extract->remove), "clicked", G_CALLBACK(xa_multi_extract_dialog_remove_files), multi_extract);

	g_signal_connect(G_OBJECT(multi_extract->liststore), "row-inserted", G_CALLBACK(xa_activate_remove_button), multi_extract->remove);

	hbox = gtk_hbox_new(TRUE, 8);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);

	frame = gtk_frame_new(_("Destination"));
	gtk_box_pack_start(GTK_BOX(hbox), frame, TRUE, TRUE, 0);
	gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_OUT);

	alignment = gtk_alignment_new(0.5, 0.5, 1, 1);
	gtk_container_add(GTK_CONTAINER(frame), alignment);
	gtk_alignment_set_padding(GTK_ALIGNMENT(alignment), 0, 2, 12, 2);

	vbox = gtk_vbox_new(TRUE, 0);
	gtk_container_add(GTK_CONTAINER(alignment), vbox);

	hbox2 = gtk_hbox_new(FALSE, 4);
	gtk_box_pack_start(GTK_BOX(vbox), hbox2, TRUE, FALSE, 0);

	multi_extract->extract_to = gtk_radio_button_new_with_mnemonic(NULL, _("Extract to:"));
	gtk_box_pack_start(GTK_BOX(hbox2), multi_extract->extract_to, FALSE, FALSE, 0);
	gtk_button_set_focus_on_click(GTK_BUTTON(multi_extract->extract_to), FALSE);

	group = gtk_radio_button_get_group(GTK_RADIO_BUTTON(multi_extract->extract_to));

	multi_extract->destination_path_entry = GTK_COMPAT_ENTRY_ICON_NEW();
	GTK_COMPAT_ENTRY_ICON(multi_extract->destination_path_entry, xa_select_where_to_extract, multi_extract);
	gtk_box_pack_start(GTK_BOX(hbox2), multi_extract->destination_path_entry, TRUE, TRUE, 0);
	gtk_entry_set_activates_default(GTK_ENTRY(multi_extract->destination_path_entry), TRUE);

	button = gtk_radio_button_new_with_mnemonic(group, _("Extract to directories with archive names"));
	gtk_box_pack_start(GTK_BOX(vbox), button, TRUE, FALSE, 0);
	gtk_button_set_focus_on_click(GTK_BUTTON(button), FALSE);

	frame = gtk_frame_new(_("Options"));
	gtk_box_pack_start(GTK_BOX(hbox), frame, TRUE, TRUE, 0);
	gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_OUT);

	alignment = gtk_alignment_new(0.5, 0.5, 1, 1);
	gtk_container_add(GTK_CONTAINER(frame), alignment);
	gtk_alignment_set_padding(GTK_ALIGNMENT(alignment), 0, 2, 12, 2);

	vbox = gtk_vbox_new(TRUE, 0);
	gtk_container_add(GTK_CONTAINER(alignment), vbox);

	multi_extract->overwrite = gtk_check_button_new_with_mnemonic(_("Overwrite existing files"));
	gtk_box_pack_start(GTK_BOX(vbox), multi_extract->overwrite, TRUE, FALSE, 0);
	gtk_button_set_focus_on_click(GTK_BUTTON(multi_extract->overwrite), FALSE);

	multi_extract->full_path = gtk_check_button_new_with_mnemonic(_("Extract with full path"));
	gtk_box_pack_start(GTK_BOX(vbox), multi_extract->full_path, TRUE, FALSE, 0);
	gtk_button_set_focus_on_click(GTK_BUTTON(multi_extract->full_path), FALSE);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(multi_extract->full_path), TRUE);

	button = gtk_button_new_from_stock("gtk-cancel");
	gtk_dialog_add_action_widget(GTK_DIALOG(multi_extract->dialog), button, GTK_RESPONSE_CANCEL);

	button = gtk_button_new();

	image = xa_main_window_find_image("xarchiver-extract.png", GTK_ICON_SIZE_BUTTON);
	label = gtk_label_new_with_mnemonic(_("_Extract"));

	hbox = gtk_hbox_new(FALSE, 4);
	gtk_box_pack_start(GTK_BOX(hbox), image, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);

	alignment = gtk_alignment_new(0.5, 0.5, 0, 1);
	gtk_container_add(GTK_CONTAINER(alignment), hbox);
	gtk_container_add(GTK_CONTAINER(button), alignment);

	gtk_dialog_add_action_widget(GTK_DIALOG(multi_extract->dialog), button, GTK_RESPONSE_OK);
	gtk_widget_set_can_default(button, TRUE);

	gtk_dialog_set_default_response(GTK_DIALOG(multi_extract->dialog), GTK_RESPONSE_OK);

	return multi_extract;
}

void xa_multi_extract_dialog_add_file (gchar *file_path, MultiExtractDialog *multi_extract)
{
	GtkTreeIter iter;
	gchar *path, *path_utf8, *file_utf8;
	ArchiveType xa;
	XArchive archive = {0};
	struct stat my_stat;
	guint64 file_size;

	xa = xa_detect_archive_type(file_path);

	if (xa.type == XARCHIVETYPE_UNKNOWN || xa.type == XARCHIVETYPE_NOT_FOUND)
		return;

	if (!archiver[xa.type].list)
	{
		gchar *name, *msg;

		name = g_filename_display_basename(file_path);
		msg = g_strdup_printf(_("The proper archiver for \"%s\" is not installed!"), name);
		xa_show_message_dialog(GTK_WINDOW(xa_main_window), GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK, _("Sorry, this archive format is not supported:"), msg);
		g_free(msg);
		g_free(name);
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

	if (!archive.can_overwrite)
	{
		multi_extract->nr_no_overwrite++;
		gtk_widget_set_sensitive(multi_extract->overwrite, FALSE);
	}

	if (!archive.can_full_path[0])
	{
		multi_extract->nr_no_full_path++;
		gtk_widget_set_sensitive(multi_extract->full_path, FALSE);
	}

	stat (file_path,&my_stat);
	file_size = my_stat.st_size;
	path = xa_remove_level_from_path(file_path);
	path_utf8 = g_filename_display_name(path);
	file_utf8 = g_filename_display_basename(file_path);
	gtk_list_store_append(multi_extract->liststore, &iter);
	gtk_list_store_set(multi_extract->liststore, &iter, 0, file_utf8, 1, file_size, 2, path_utf8, 3, xa.type, 4, xa.tag, 5, archive.can_overwrite, 6, archive.can_full_path[0], -1);
	multi_extract->nr_total++;
	g_free(file_utf8);
	g_free(path_utf8);
	g_free(path);
}

void xa_multi_extract_dialog (MultiExtractDialog *multi_extract)
{
	GtkTreeIter iter;
	gchar *filename, *filename_local, *file, *path, *message, *name, *dest_path = NULL;
	GString *output = g_string_new("");
	gboolean overwrite, full_path;
	gint response;
	double percent = 0.0;

	if (!*gtk_entry_get_text(GTK_ENTRY(multi_extract->destination_path_entry)))
	{
		path = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(prefs_window->preferred_extract_dir));

		if (!path || !*path)
		{
			g_free(path);
			path = g_strdup("/tmp");
		}

		gtk_entry_set_text(GTK_ENTRY(multi_extract->destination_path_entry), path);
		g_free(path);
	}

	gtk_widget_show_all(multi_extract->dialog);
run:
	response = gtk_dialog_run(GTK_DIALOG(multi_extract->dialog));
	if (response == GTK_RESPONSE_CANCEL || response == GTK_RESPONSE_DELETE_EVENT)
	{
		xa_multi_extract_dialog_clear(multi_extract);
		gtk_widget_hide(multi_extract->dialog);
		return;
	}
	if (gtk_tree_model_get_iter_first(GTK_TREE_MODEL(multi_extract->liststore), &iter) == FALSE)
	{
		xa_show_message_dialog(GTK_WINDOW (xa_main_window),GTK_DIALOG_MODAL,GTK_MESSAGE_ERROR,GTK_BUTTONS_OK,_("Can't multi-extract archives:"),_("You haven't added any of them!"));
		goto run;
	}
	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(multi_extract->extract_to)))
	{
		dest_path = g_filename_from_utf8(gtk_entry_get_text(GTK_ENTRY(multi_extract->destination_path_entry)), -1, NULL, NULL, NULL);
		if (strlen(dest_path)== 0)
		{
			xa_show_message_dialog (GTK_WINDOW (xa_main_window),GTK_DIALOG_MODAL,GTK_MESSAGE_ERROR,GTK_BUTTONS_OK,_("You missed where to extract the files!"),_("Please fill the \"Extract to\" field!"));
			goto run;
		}
	}
	gtk_widget_hide(multi_extract->dialog);

	overwrite = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(multi_extract->overwrite));
	full_path = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(multi_extract->full_path));

	double fraction = 1.0 / multi_extract->nr_total;

	xa_show_progress_bar(NULL);

	do
	{
		gtk_tree_model_get(GTK_TREE_MODEL(multi_extract->liststore), &iter, 0, &file, 2, &path, -1);
		filename = g_strconcat (path,"/",file,NULL);
		xa_increase_progress_bar(progress, filename, percent);
		g_free(file);
		g_free(path);
		filename_local = g_filename_from_utf8(filename, -1, NULL, NULL, NULL);
		message = xa_multi_extract_one_archive(multi_extract, filename_local, overwrite, full_path, dest_path);
		g_free(filename_local);
		if (message != NULL)
		{
			name = g_strconcat(filename,": ",message,"\n",NULL);
			g_string_append(output,name);
		}
		g_free(filename);
		if (multi_extract->stop_pressed)
			break;
		percent += fraction;

		xa_increase_progress_bar(progress, NULL, percent);
	}
	while (gtk_tree_model_iter_next(GTK_TREE_MODEL(multi_extract->liststore), &iter));

	/* let the user see the progress of 100% */
	gtk_main_iteration();
	g_usleep(G_USEC_PER_SEC / 4);

	if (strlen(output->str)> 0)
		xa_show_message_dialog (GTK_WINDOW (xa_main_window),GTK_DIALOG_MODAL,GTK_MESSAGE_ERROR,GTK_BUTTONS_OK,_("Some errors occurred:"),output->str );

	g_string_free(output,TRUE);
	if (dest_path != NULL)
		g_free(dest_path);

	xa_multi_extract_dialog_clear(multi_extract);
}

void xa_execute_extract_commands (XArchive *archive, GSList *list, gboolean strip)
{
	gchar *extract_to, *extraction_dir, *command;
	GString *dir_contents;

	if (xa_main_window)
	{
		/* normal extraction if current location isn't inside a directory */
		if (archive->location_path == NULL)
			strip = FALSE;
	}
	else
	{
		/* normal extraction if there is no containing directory */
		if (!xa_has_containing_directory(archive))
			strip = FALSE;
		else
			archive->location_path = archive->root_entry->child->filename;
	}

	if (strip)
	{
		extract_to = xa_create_working_subdirectory(archive);

		if (!extract_to)
		{
			xa_show_message_dialog(GTK_WINDOW(xa_main_window), GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK, _("Can't create directory!"), "");
			return;
		}

		extraction_dir = archive->extraction_dir;
		archive->extraction_dir = xa_escape_bad_chars(extract_to, ESCAPES);
	}

	archive->status = XARCHIVESTATUS_EXTRACT;
	(*archive->archiver->extract)(archive, list);

	if (strip)
	{
		archive->child_dir = g_strconcat(extract_to, "/", archive->location_path, NULL);
		dir_contents = xa_quote_dir_contents(archive->child_dir);

		command = g_strconcat("mv",
		                      archive->do_overwrite ? " -f" : (archive->do_update ? " -fu" : " -n"),
		                      " --", dir_contents->str, " ", extraction_dir, NULL);

		archive->status = XARCHIVESTATUS_EXTRACT;   // restore status
		xa_run_command(archive, command);
		g_free(command);

		g_free(archive->child_dir);
		archive->child_dir = NULL;

		g_free(extraction_dir);
		g_free(extract_to);
		g_string_free(dir_contents, TRUE);
	}
}
