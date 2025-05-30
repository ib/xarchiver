/*
 *  Copyright (C) 2008 Giuseppe Torelli - <colossus73@gmail.com>
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

#include <string.h>
#include "add_dialog.h"
#include "interface.h"
#include "main.h"
#include "pref_dialog.h"
#include "string_utils.h"
#include "support.h"
#include "window.h"

static GTK_COMPAT_ADJUSTMENT_TYPE compression_value;

static gboolean no_focus (GtkWidget *widget, GtkDirectionType direction, gpointer user_data)
{
	return TRUE;
}

static void toggle_update_freshen (GtkToggleButton *button, AddDialog *add_dialog)
{
	gboolean active = gtk_toggle_button_get_active(button);

	if (active)
	{
		if (button != GTK_TOGGLE_BUTTON(add_dialog->update))
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(add_dialog->update), FALSE);

		if (button != GTK_TOGGLE_BUTTON(add_dialog->freshen))
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(add_dialog->freshen), FALSE);
	}
}

static void password_toggled_cb (GtkToggleButton *button, AddDialog *add_dialog)
{
	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(add_dialog->password)))
	{
		gtk_widget_set_sensitive(add_dialog->password_entry, TRUE);
		gtk_widget_grab_focus(add_dialog->password_entry);
		gtk_widget_set_sensitive(add_dialog->encrypt, add_dialog->can_encrypt);
	}
	else
	{
		gtk_widget_set_sensitive(add_dialog->password_entry, FALSE);
		gtk_widget_set_sensitive(add_dialog->encrypt, FALSE);
	}
}

static void toggle_compression (GtkToggleButton *button, AddDialog *add_dialog)
{
	gboolean toggle_button_active = gtk_toggle_button_get_active(button);

	gtk_widget_set_sensitive(add_dialog->label_least, !toggle_button_active);
	gtk_widget_set_sensitive(add_dialog->label_best, !toggle_button_active);
	gtk_widget_set_sensitive(add_dialog->compression_scale, !toggle_button_active);
}

static void fix_adjustment_value (GtkAdjustment *adjustment, XArchive *archive)
{
	gushort value = gtk_adjustment_get_value(adjustment);

	if ((value - archive->compressor.least) % archive->compressor.steps == 0)
		return;
	else
		gtk_adjustment_set_value(adjustment, value - 1);
}

AddDialog *xa_create_add_dialog ()
{
	GTK_COMPAT_TOOLTIPS;
	AddDialog *add_dialog;
	GtkWidget *vbox, *label, *frame, *alignment, *hbox, *table, *button, *image;
	GSList *group;

	add_dialog = g_new0(AddDialog, 1);

	add_dialog->dialog = gtk_dialog_new();
	gtk_window_set_title(GTK_WINDOW(add_dialog->dialog), _("Add files"));
	gtk_window_set_position(GTK_WINDOW(add_dialog->dialog), GTK_WIN_POS_CENTER_ON_PARENT);
	gtk_window_set_type_hint(GTK_WINDOW(add_dialog->dialog), GDK_WINDOW_TYPE_HINT_DIALOG);
	xa_set_xarchiver_icon(GTK_WINDOW(add_dialog->dialog));

	add_dialog->notebook = gtk_notebook_new();
	gtk_container_add(GTK_CONTAINER(gtk_dialog_get_content_area(GTK_DIALOG(add_dialog->dialog))), add_dialog->notebook);
	gtk_container_set_border_width(GTK_CONTAINER(add_dialog->notebook), 4);
	gtk_widget_set_can_focus(add_dialog->notebook, FALSE);

	/* Selection page */

	vbox = gtk_vbox_new(FALSE, 2);
	label = gtk_label_new(_("Selection"));
	gtk_notebook_append_page(GTK_NOTEBOOK(add_dialog->notebook), vbox, label);

	add_dialog->filechooser = gtk_file_chooser_widget_new(GTK_FILE_CHOOSER_ACTION_OPEN);
	gtk_box_pack_start(GTK_BOX(vbox), add_dialog->filechooser, TRUE, TRUE, 2);

	frame = gtk_frame_new(_("File Paths"));
	gtk_box_pack_start(GTK_BOX(vbox), frame, FALSE, FALSE, 4);
	gtk_container_set_border_width(GTK_CONTAINER(frame), 4);

	alignment = gtk_alignment_new(0.5, 0.5, 1, 1);
	gtk_container_add(GTK_CONTAINER(frame), alignment);
	gtk_alignment_set_padding(GTK_ALIGNMENT(alignment), 0, 6, 48, 48);

	hbox = gtk_hbox_new(TRUE, 0);
	gtk_container_add(GTK_CONTAINER(alignment), hbox);

	add_dialog->full_path = gtk_radio_button_new_with_mnemonic(NULL, _("With full path"));
	gtk_box_pack_start(GTK_BOX(hbox), add_dialog->full_path, TRUE, FALSE, 0);
	gtk_button_set_focus_on_click(GTK_BUTTON(add_dialog->full_path), FALSE);
	g_signal_connect(G_OBJECT(add_dialog->full_path), "focus", G_CALLBACK(no_focus), NULL);

	group = gtk_radio_button_get_group(GTK_RADIO_BUTTON(add_dialog->full_path));

	add_dialog->relative_path = gtk_radio_button_new_with_mnemonic(group, _("Without parent path"));
	gtk_box_pack_start(GTK_BOX(hbox), add_dialog->relative_path, TRUE, FALSE, 0);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(add_dialog->relative_path), TRUE);
	gtk_button_set_focus_on_click(GTK_BUTTON(add_dialog->relative_path), FALSE);
	g_signal_connect(G_OBJECT(add_dialog->relative_path), "focus", G_CALLBACK(no_focus), NULL);

	/* Options page */

	vbox = gtk_vbox_new(FALSE, 0);
	label = gtk_label_new(_("Options"));
	gtk_notebook_append_page(GTK_NOTEBOOK(add_dialog->notebook), vbox, label);

	hbox = gtk_hbox_new(TRUE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, TRUE, TRUE, 0);
	gtk_container_set_border_width(GTK_CONTAINER(hbox), 6);

	frame = gtk_frame_new(_("Actions"));
	gtk_box_pack_start(GTK_BOX(hbox), frame, TRUE, TRUE, 0);
	gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_OUT);

	alignment = gtk_alignment_new(0.5, 0.5, 1, 1);
	gtk_container_add(GTK_CONTAINER(frame), alignment);
	gtk_alignment_set_padding(GTK_ALIGNMENT(alignment), 0, 0, 12, 0);

	hbox = gtk_hbox_new(TRUE, 8);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, TRUE, TRUE, 0);
	gtk_container_set_border_width(GTK_CONTAINER(hbox), 6);

	vbox = gtk_vbox_new(TRUE, 0);
	gtk_container_add(GTK_CONTAINER(alignment), vbox);

	add_dialog->update = gtk_check_button_new_with_mnemonic(_("Update existing files and add new ones"));
	gtk_widget_set_tooltip_text(add_dialog->update, _("This option will add any new files and update any files which are already in the archive but older there."));
	gtk_box_pack_start(GTK_BOX(vbox), add_dialog->update, TRUE, FALSE, 0);
	gtk_button_set_focus_on_click(GTK_BUTTON(add_dialog->update), FALSE);
	g_signal_connect(G_OBJECT(add_dialog->update), "toggled", G_CALLBACK(toggle_update_freshen), add_dialog);
	g_signal_connect(G_OBJECT(add_dialog->update), "focus", G_CALLBACK(no_focus), NULL);

	add_dialog->freshen = gtk_check_button_new_with_mnemonic(_("Freshen existing files only"));
	gtk_widget_set_tooltip_text(add_dialog->freshen, _("This option will only add files which are already in the archive but older there; unlike the update option it will not add any new files."));
	gtk_box_pack_start(GTK_BOX(vbox), add_dialog->freshen, TRUE, FALSE, 0);
	gtk_button_set_focus_on_click(GTK_BUTTON(add_dialog->freshen), FALSE);
	g_signal_connect(G_OBJECT(add_dialog->freshen), "toggled", G_CALLBACK(toggle_update_freshen), add_dialog);
	g_signal_connect(G_OBJECT(add_dialog->freshen), "focus", G_CALLBACK(no_focus), NULL);

	add_dialog->recurse = gtk_check_button_new_with_mnemonic (_("Include subdirectories"));
	gtk_box_pack_start(GTK_BOX(vbox), add_dialog->recurse, TRUE, FALSE, 0);
	gtk_button_set_focus_on_click (GTK_BUTTON (add_dialog->recurse), FALSE);
	g_signal_connect(G_OBJECT(add_dialog->recurse), "focus", G_CALLBACK(no_focus), NULL);

	add_dialog->remove = gtk_check_button_new_with_mnemonic(_("Delete files after adding"));
	gtk_box_pack_start(GTK_BOX(vbox), add_dialog->remove, TRUE, FALSE, 0);
	gtk_button_set_focus_on_click(GTK_BUTTON(add_dialog->remove), FALSE);
	g_signal_connect(G_OBJECT(add_dialog->remove), "focus", G_CALLBACK(no_focus), NULL);

	add_dialog->solid = gtk_check_button_new_with_mnemonic(_("Create a solid archive"));
	gtk_widget_set_tooltip_text(add_dialog->solid, _("In a solid archive the files are grouped together resulting in a better compression ratio."));
	gtk_box_pack_start(GTK_BOX(vbox), add_dialog->solid, TRUE, FALSE, 0);
	gtk_button_set_focus_on_click(GTK_BUTTON(add_dialog->solid), FALSE);
	g_signal_connect(G_OBJECT(add_dialog->solid), "focus", G_CALLBACK(no_focus), NULL);

	frame = gtk_frame_new(_("Compression"));
	gtk_box_pack_start(GTK_BOX(hbox), frame, TRUE, TRUE, 0);
	gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_OUT);

	vbox = gtk_vbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(frame), vbox);

	add_dialog->alignment = gtk_alignment_new(0.5, 1, 1, 0);
	gtk_container_add(GTK_CONTAINER(vbox), add_dialog->alignment);
	gtk_alignment_set_padding(GTK_ALIGNMENT(add_dialog->alignment), 16, 0, 12, 12);

	table = gtk_table_new(2, 2, FALSE);
	gtk_container_add(GTK_CONTAINER(vbox), table);

	add_dialog->label_least = gtk_label_new(_("least"));
	gtk_table_attach(GTK_TABLE(table), add_dialog->label_least, 0, 1, 0, 1, GTK_EXPAND | GTK_FILL, GTK_SHRINK, 0, 0);
	gtk_misc_set_alignment(GTK_MISC(add_dialog->label_least), 0, 0);
	gtk_misc_set_padding(GTK_MISC(add_dialog->label_least), 10, 0);

	add_dialog->label_best = gtk_label_new(_("best"));
	gtk_table_attach(GTK_TABLE(table), add_dialog->label_best, 1, 2, 0, 1, GTK_EXPAND | GTK_FILL, GTK_SHRINK, 0, 0);
	gtk_misc_set_alignment(GTK_MISC(add_dialog->label_best), 1, 0);
	gtk_misc_set_padding(GTK_MISC(add_dialog->label_best), 10, 0);

	add_dialog->uncompressed = gtk_check_button_new_with_label(_("No compression"));
	gtk_table_attach(GTK_TABLE(table), add_dialog->uncompressed, 0, 2, 1, 2, GTK_SHRINK, GTK_SHRINK, 0, 16);
	gtk_button_set_focus_on_click(GTK_BUTTON(add_dialog->uncompressed), FALSE);
	g_signal_connect(G_OBJECT(add_dialog->uncompressed), "toggled", G_CALLBACK(toggle_compression), add_dialog);

	frame = gtk_frame_new(_("Encryption"));
	gtk_box_pack_start(GTK_BOX(hbox), frame, TRUE, TRUE, 0);
	gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_OUT);

	alignment = gtk_alignment_new(0.5, 0.5, 1, 0);
	gtk_container_add(GTK_CONTAINER(frame), alignment);
	gtk_alignment_set_padding(GTK_ALIGNMENT(alignment), 16, 0, 12, 12);

	table = gtk_table_new(2, 1, FALSE);
	gtk_container_add(GTK_CONTAINER(alignment), table);

	hbox = gtk_hbox_new(FALSE, 4);
	gtk_table_attach(GTK_TABLE(table), hbox, 0, 1, 0, 1, GTK_EXPAND | GTK_FILL, GTK_SHRINK, 0, 0);

	add_dialog->password = gtk_check_button_new_with_mnemonic(_("Password:"));
	gtk_box_pack_start(GTK_BOX(hbox), add_dialog->password, FALSE, FALSE, 0);
	gtk_button_set_focus_on_click(GTK_BUTTON(add_dialog->password), FALSE);
	g_signal_connect(G_OBJECT(add_dialog->password), "toggled", G_CALLBACK(password_toggled_cb), add_dialog);

	add_dialog->password_entry = gtk_entry_new();
	gtk_box_pack_start(GTK_BOX(hbox), add_dialog->password_entry, TRUE, TRUE, 0);
	gtk_entry_set_visibility(GTK_ENTRY(add_dialog->password_entry), FALSE);
	gtk_widget_set_sensitive(add_dialog->password_entry, FALSE);
	gtk_entry_set_activates_default(GTK_ENTRY(add_dialog->password_entry), TRUE);

	add_dialog->encrypt = gtk_check_button_new_with_mnemonic(_("Encrypt filenames"));
	gtk_table_attach(GTK_TABLE(table), add_dialog->encrypt, 0, 1, 1, 2, GTK_SHRINK, GTK_SHRINK, 0, 16);
	gtk_button_set_focus_on_click(GTK_BUTTON(add_dialog->encrypt), FALSE);

	button = gtk_button_new_from_stock("gtk-cancel");
	gtk_dialog_add_action_widget(GTK_DIALOG(add_dialog->dialog), button, GTK_RESPONSE_CANCEL);

	button = gtk_button_new();

	image = xa_main_window_find_image("xarchiver-add.png", GTK_ICON_SIZE_BUTTON);
	label = gtk_label_new_with_mnemonic(_("_Add"));

	hbox = gtk_hbox_new(FALSE, 4);
	gtk_box_pack_start(GTK_BOX(hbox), image, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);

	alignment = gtk_alignment_new(0.5, 0.5, 0, 1);
	gtk_container_add(GTK_CONTAINER(alignment), hbox);
	gtk_container_add(GTK_CONTAINER(button), alignment);

	gtk_dialog_add_action_widget(GTK_DIALOG(add_dialog->dialog), button, GTK_RESPONSE_OK);
	gtk_widget_set_can_default(button, TRUE);

	gtk_dialog_set_default_response(GTK_DIALOG(add_dialog->dialog), GTK_RESPONSE_OK);

	return add_dialog;
}

void xa_set_add_dialog_options (AddDialog *add_dialog, XArchive *archive)
{
	GTK_COMPAT_TOOLTIPS;
	gboolean epub, full_path, normal, use_password;
	gchar *compression_msg;

	if (progress)
		gtk_widget_hide(progress->window);

	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(prefs_window->save_geometry)) && prefs_window->add_win_size[0] != -1)
		gtk_window_set_default_size(GTK_WINDOW(add_dialog->dialog), prefs_window->add_win_size[0], prefs_window->add_win_size[1]);

	prefs_window->size_changed[1] = TRUE;

	epub = (archive->tag == 'e');

	gtk_file_chooser_set_select_multiple(GTK_FILE_CHOOSER(add_dialog->filechooser), !SINGLE_FILE_COMPRESSOR(archive));

	if ((archive->location_path && *archive->location_path) || !archive->can_full_path[1] || epub)
	{
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(add_dialog->relative_path), TRUE);
		full_path = FALSE;
	}
	else
		full_path = TRUE;

	gtk_widget_set_sensitive(add_dialog->full_path, full_path);
	gtk_widget_set_sensitive(add_dialog->update, archive->can_update[1]);
	gtk_widget_set_sensitive(add_dialog->freshen, archive->can_freshen[1]);
	gtk_widget_set_sensitive(add_dialog->recurse, archive->can_recurse[1] != FORCED && !SINGLE_FILE_COMPRESSOR(archive));
	gtk_widget_set_sensitive(add_dialog->remove, archive->can_remove);
	gtk_widget_set_sensitive(add_dialog->solid, archive->can_solid);

	normal = (archive->compressor.least <= archive->compressor.best);

	if (normal)
		compression_value = gtk_adjustment_new(archive->compression, archive->compressor.least, archive->compressor.best, archive->compressor.steps, archive->compressor.steps, 0);
	else
		compression_value = gtk_adjustment_new(archive->compression, archive->compressor.best, archive->compressor.least, archive->compressor.steps, archive->compressor.steps, 0);

	add_dialog->compression_scale = gtk_hscale_new(GTK_ADJUSTMENT(compression_value));

	if (!gtk_bin_get_child(GTK_BIN(add_dialog->alignment)))
		gtk_container_add(GTK_CONTAINER(add_dialog->alignment), add_dialog->compression_scale);

	gtk_scale_set_value_pos (GTK_SCALE (add_dialog->compression_scale), GTK_POS_TOP);
	gtk_scale_set_digits (GTK_SCALE (add_dialog->compression_scale), 0);
	gtk_range_set_inverted(GTK_RANGE(add_dialog->compression_scale), !normal);

	gtk_widget_set_sensitive(add_dialog->label_least, archive->can_compress);
	gtk_widget_set_sensitive(add_dialog->label_best, archive->can_compress);
	gtk_widget_set_sensitive(add_dialog->compression_scale, archive->can_compress);

	gtk_widget_set_sensitive(add_dialog->uncompressed, archive->can_compress && archive->compressor.can_uncompressed);

	if (gtk_widget_get_sensitive(add_dialog->uncompressed) && gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(add_dialog->uncompressed)))
	{
		gtk_widget_set_sensitive(add_dialog->label_least, FALSE);
		gtk_widget_set_sensitive(add_dialog->label_best, FALSE);
		gtk_widget_set_sensitive(add_dialog->compression_scale, FALSE);
	}

	if (archive->compressor.steps > 1)
		g_signal_connect(G_OBJECT(compression_value), "value-changed", G_CALLBACK(fix_adjustment_value), archive);

	compression_msg = g_strdup_printf(_("%hu is least compression\n%hu is default compression\n%hu is best compression"), archive->compressor.least, archive->compressor.preset, archive->compressor.best);
	gtk_widget_set_tooltip_text(add_dialog->compression_scale, compression_msg);
	g_free(compression_msg);

	gtk_widget_set_sensitive(add_dialog->password, archive->can_password && !epub);
	use_password = (gtk_widget_get_sensitive(add_dialog->password) && gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(add_dialog->password)));
	gtk_widget_set_sensitive(add_dialog->password_entry, use_password);
	gtk_widget_set_sensitive(add_dialog->encrypt, use_password && archive->can_encrypt);

	add_dialog->can_encrypt = archive->can_encrypt;   // for access in password_toggled_cb()

	gtk_widget_show_all(add_dialog->dialog);
}

void xa_parse_add_dialog_options (XArchive *archive, AddDialog *add_dialog, GSList *list)
{
	gchar *temp_password = NULL;
	gboolean done = FALSE, recurse;

	while ( ! done )
	{
		switch (gtk_dialog_run(GTK_DIALOG(add_dialog->dialog)))
		{
			case GTK_RESPONSE_CANCEL:
			case GTK_RESPONSE_DELETE_EVENT:
			done = TRUE;
			break;

			case GTK_RESPONSE_OK:
			if (!list)
				list = gtk_file_chooser_get_filenames(GTK_FILE_CHOOSER(add_dialog->filechooser));
			if (g_slist_length(list) == 0)
			{
				xa_show_message_dialog (GTK_WINDOW (xa_main_window),GTK_DIALOG_MODAL,GTK_MESSAGE_ERROR,GTK_BUTTONS_OK,_("Can't add files to the archive:"), _("You haven't selected any files to add!") );
				break;
			}
			if (SINGLE_FILE_COMPRESSOR(archive) && g_file_test(g_slist_nth_data(list, 0), G_FILE_TEST_IS_DIR))
			{
				xa_show_message_dialog(GTK_WINDOW(xa_main_window), GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK, _("An error occurred!"), _("The archiver doesn't support compression of directories or multiple files!"));
				break;
			}
			if (gtk_widget_is_sensitive(add_dialog->password) && gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(add_dialog->password)))
			{
				temp_password  = g_strdup(gtk_entry_get_text(GTK_ENTRY(add_dialog->password_entry)));
				if (strlen(temp_password) == 0)
				{
					xa_show_message_dialog (GTK_WINDOW (xa_main_window),GTK_DIALOG_MODAL,GTK_MESSAGE_ERROR,GTK_BUTTONS_OK, _("You missed the password!"),_("Please enter it!") );
					g_free (temp_password);
					break;
				}
				else
					archive->password = temp_password;
			}
			else if (archive->password != NULL)
			{
				if (xa_main_window)
				{
					g_free(archive->password);
					archive->password = NULL;
				}
			}

			archive->do_encrypt = (gtk_widget_is_sensitive(add_dialog->encrypt) && gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(add_dialog->encrypt)));

			done = TRUE;
			if (gtk_widget_is_sensitive(add_dialog->full_path) && gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(add_dialog->full_path)))
				archive->do_full_path = TRUE;
			else
				archive->do_full_path = FALSE;

			if (gtk_widget_is_sensitive(add_dialog->update))
				archive->do_update = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON (add_dialog->update));

			if (gtk_widget_is_sensitive(add_dialog->remove))
				archive->do_remove = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(add_dialog->remove));

			if (gtk_widget_is_sensitive(add_dialog->freshen))
				archive->do_freshen = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(add_dialog->freshen));

			if (gtk_widget_is_sensitive(add_dialog->solid))
				archive->do_solid = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(add_dialog->solid));

			if (gtk_widget_is_sensitive(add_dialog->compression_scale))
				archive->compression = gtk_adjustment_get_value(GTK_ADJUSTMENT(compression_value));
			else
				archive->compression = 0;

			gtk_widget_hide(add_dialog->dialog);

			if (!archive->do_full_path)
				archive->child_dir = g_path_get_dirname(list->data);

			recurse = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(add_dialog->recurse));
			archive->do_recurse = FALSE;

			if (archive->can_recurse[1])
			{
				archive->do_recurse = (recurse || (archive->can_recurse[1] == FORCED));
				recurse = FALSE;
			}

			xa_execute_add_commands(archive, list, recurse, !archive->do_recurse);
			g_slist_free_full(list, g_free);
		}
	}
	gtk_widget_destroy(add_dialog->compression_scale);
	gtk_widget_hide(add_dialog->dialog);
}

void xa_execute_add_commands (XArchive *archive, GSList *list, gboolean recurse, gboolean all)
{
	gchar *new_path = NULL;
	gchar *esc2, *dest, *org_path = NULL;
	gboolean success;
	gint result, n_list = 0;
	GString *items;
	gchar *command = NULL;
	GSList *slist = NULL;
	GSList *dirlist = NULL;
	GSList *files;

	if (xa_main_window)
	{
		/* This in case the user wants to add files in a directory in the archive tree */
		if (archive->location_path != NULL)
		{
			success = xa_create_working_directory(archive);

			if (!success)
				return;

			items = g_string_new("");
			new_path = g_strconcat(archive->working_dir, "/", archive->location_path, NULL);
			result = g_mkdir_with_parents(new_path,0700);

			if (result != 0)
			{
				g_free(new_path);
				return;
			}

			slist = list;

			while (slist)
			{
				esc2 = g_shell_quote(slist->data);
				g_string_append(items,esc2);
				g_string_append_c(items,' ');
				g_free(esc2);
				n_list++;
				slist = slist->next;
			}

			dest = xa_escape_bad_chars(new_path, ESCAPES);
			command = g_strconcat("cp -rfp -- ", items->str, dest, NULL);
			g_free(dest);
			g_string_free(items,TRUE);

			if ((n_list == 1) && g_file_test(list->data, G_FILE_TEST_IS_REGULAR))
				org_path = g_path_get_dirname(list->data);

			xa_remove_slash_from_path(new_path);

			if (g_strcmp0(org_path, new_path) == 0)
				success = TRUE;
			else
			{
				archive->status = XARCHIVESTATUS_ADD;
				success = xa_run_command(archive, command);
			}

			g_free(org_path);
			g_free(new_path);
			g_free(command);

			if (!success)
			{
				new_path = xa_remove_level_from_path(archive->location_path);
				g_free(archive->location_path);

				if (strcmp(new_path, ".") == 0)
					archive->location_path = NULL;
				else
					archive->location_path = g_strconcat(new_path, "/", NULL);

				xa_set_location_entry(archive);

				g_free(new_path);
				return;
			}
		}
		xa_set_button_state(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, NULL, 0);
	}

	while (list)
	{
		if (archive->can_descend)
			all = FALSE;

		xa_recurse_local_directory((gchar*) list->data, &dirlist, archive->do_full_path, recurse, all);
		list = list->next;
	}
	files = xa_collect_filenames(archive, dirlist);
	g_slist_free_full(dirlist, g_free);

	if (archive->location_path != NULL)
	{
		g_free(archive->child_dir);
		archive->child_dir = g_strdup(archive->working_dir);
	}

	archive->status = XARCHIVESTATUS_ADD;
	(*archive->archiver->add)(archive, files);

	g_free(archive->child_dir);
	archive->child_dir = NULL;

	if (xa_main_window && (archive->status == XARCHIVESTATUS_IDLE))
		xa_reload_archive_content(archive);
}
