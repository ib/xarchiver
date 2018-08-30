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

#include <string.h>
#include "add_dialog.h"
#include "interface.h"
#include "main.h"
#include "pref_dialog.h"
#include "string_utils.h"
#include "support.h"
#include "window.h"

static GTK_COMPAT_ADJUSTMENT_TYPE compression_value;

static void add_fresh_update_toggled_cb (GtkToggleButton *button, Add_dialog_data *data)
{
	gboolean active = gtk_toggle_button_get_active (button);
	if (active)
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (data->update), FALSE);
}

static void password_toggled_cb (GtkToggleButton *button, Add_dialog_data *add_dialog)
{
	if ( gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(add_dialog->add_password)) )
	{
		gtk_widget_set_sensitive (add_dialog->add_password_entry, TRUE);
		gtk_widget_grab_focus (add_dialog->add_password_entry);
	}
	else
		gtk_widget_set_sensitive (add_dialog->add_password_entry, FALSE);
}

static void add_update_fresh_toggled_cb (GtkToggleButton *button, Add_dialog_data *data)
{
	if (data->freshen == NULL)
		return;
	gboolean active = gtk_toggle_button_get_active (button);
	if (active)
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (data->freshen), FALSE);
}

static void fix_adjustment_value (GtkAdjustment *adjustment, gpointer user_data)
{
	unsigned short int digit = gtk_adjustment_get_value (adjustment);
	if (digit & 1)
		return;
	else
		gtk_adjustment_set_value (adjustment, digit-1);
}

Add_dialog_data *xa_create_add_dialog()
{
	GTK_COMPAT_TOOLTIPS;
	GtkWidget *dialog_vbox1,*label1,*label2,*label4,*label5,*label7,*hbox1,*hbox2,*hbox3,*hbox4;
	GtkWidget *dialog_action_area1,*alignment1,*alignment2,*alignment3,*vbox3,*frame2,*frame3,*frame4,*alignment4;
	GtkWidget *vbox1;
	Add_dialog_data *add_dialog;
	GSList *group;

	add_dialog = g_new0 (Add_dialog_data, 1);

	add_dialog->dialog1 = gtk_dialog_new ();
	gtk_window_set_title (GTK_WINDOW (add_dialog->dialog1), _("Add files"));
	gtk_window_set_position (GTK_WINDOW (add_dialog->dialog1),GTK_WIN_POS_CENTER_ON_PARENT);
	gtk_window_set_type_hint (GTK_WINDOW (add_dialog->dialog1), GDK_WINDOW_TYPE_HINT_DIALOG);

	dialog_vbox1 = gtk_dialog_get_content_area(GTK_DIALOG(add_dialog->dialog1));

	add_dialog->notebook1 = gtk_notebook_new ();
	gtk_container_add(GTK_CONTAINER(dialog_vbox1), add_dialog->notebook1);
	gtk_container_set_border_width (GTK_CONTAINER (add_dialog->notebook1),4);

	/* Selection page */
	vbox1 = gtk_vbox_new (FALSE,2);
	gtk_container_add (GTK_CONTAINER (add_dialog->notebook1), vbox1);

	label1 = gtk_label_new (_("Selection"));
	gtk_notebook_set_tab_label (GTK_NOTEBOOK (add_dialog->notebook1), gtk_notebook_get_nth_page (GTK_NOTEBOOK (add_dialog->notebook1), 0), label1);

	add_dialog->filechooserwidget1 = gtk_file_chooser_widget_new (GTK_FILE_CHOOSER_ACTION_OPEN);
	gtk_box_pack_start (GTK_BOX (vbox1), add_dialog->filechooserwidget1, TRUE, TRUE,2);

	add_dialog->frame1 = gtk_frame_new (NULL);
	gtk_box_pack_start (GTK_BOX (vbox1), add_dialog->frame1, FALSE, TRUE,5);
	gtk_container_set_border_width (GTK_CONTAINER (add_dialog->frame1),5);

	alignment1 = gtk_alignment_new (0.5, 0.5, 1, 1);
	gtk_container_add (GTK_CONTAINER (add_dialog->frame1), alignment1);
	gtk_alignment_set_padding (GTK_ALIGNMENT (alignment1), 0, 0, 20, 20);

	add_dialog->label = gtk_label_new(_("File Paths: "));
	gtk_frame_set_label_widget(GTK_FRAME(add_dialog->frame1), add_dialog->label);

	hbox1 = gtk_hbox_new (TRUE, 0);
	gtk_container_add (GTK_CONTAINER (alignment1), hbox1);

	add_dialog->store_path = gtk_radio_button_new_with_mnemonic (NULL, _("Store full paths"));
	gtk_box_pack_start (GTK_BOX (hbox1), add_dialog->store_path, FALSE, FALSE, 0);
	group = gtk_radio_button_get_group(GTK_RADIO_BUTTON(add_dialog->store_path));
	gtk_button_set_focus_on_click (GTK_BUTTON (add_dialog->store_path), FALSE);

	add_dialog->no_store_path = gtk_radio_button_new_with_mnemonic(group, _("Do not store paths"));
	gtk_box_pack_start (GTK_BOX (hbox1), add_dialog->no_store_path, FALSE, FALSE, 0);
	gtk_button_set_focus_on_click (GTK_BUTTON (add_dialog->no_store_path), FALSE);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(add_dialog->no_store_path),TRUE);

	/* Options page */
	add_dialog->option_notebook_vbox = gtk_vbox_new (FALSE, 0);
	gtk_container_add (GTK_CONTAINER (add_dialog->notebook1), add_dialog->option_notebook_vbox);

	label2 = gtk_label_new (_("Options"));
	gtk_notebook_set_tab_label (GTK_NOTEBOOK (add_dialog->notebook1), gtk_notebook_get_nth_page (GTK_NOTEBOOK (add_dialog->notebook1), 1), label2);

	hbox2 = gtk_hbox_new (TRUE, 10);
	gtk_box_pack_start (GTK_BOX (add_dialog->option_notebook_vbox), hbox2, TRUE, TRUE, 0);
	gtk_container_set_border_width (GTK_CONTAINER (hbox2), 5);

	frame4 = gtk_frame_new (NULL);
	gtk_box_pack_start (GTK_BOX (hbox2), frame4, TRUE, TRUE, 0);
	gtk_frame_set_shadow_type (GTK_FRAME (frame4), GTK_SHADOW_OUT);

	alignment4 = gtk_alignment_new (0.5, 0.5, 1, 1);
	gtk_container_add (GTK_CONTAINER (frame4),alignment4);
	gtk_alignment_set_padding (GTK_ALIGNMENT(alignment4), 0, 0, 12, 0);

	vbox3 = gtk_vbox_new (TRUE, 0);
	gtk_container_add (GTK_CONTAINER (alignment4),vbox3);

	add_dialog->update = gtk_check_button_new_with_mnemonic (_("Update and add"));
	gtk_button_set_focus_on_click (GTK_BUTTON (add_dialog->update), FALSE);
	gtk_widget_set_tooltip_text(add_dialog->update, _("This option will add any new files and update any files which are already in the archive but older there"));
	gtk_box_pack_start (GTK_BOX (vbox3), add_dialog->update, FALSE, FALSE, 0);

	add_dialog->freshen = gtk_check_button_new_with_mnemonic (_("Freshen and replace"));

	gtk_button_set_focus_on_click (GTK_BUTTON (add_dialog->freshen), FALSE);
	gtk_widget_set_tooltip_text(add_dialog->freshen, _("This option will only add files which are already in the archive but older there; unlike the update option it will not add any new files"));
	gtk_box_pack_start (GTK_BOX (vbox3), add_dialog->freshen, FALSE, FALSE, 0);
	g_signal_connect (G_OBJECT (add_dialog->freshen),"toggled",G_CALLBACK (add_fresh_update_toggled_cb) , add_dialog);

	add_dialog->recurse = gtk_check_button_new_with_mnemonic (_("Include subdirectories"));
	gtk_button_set_focus_on_click (GTK_BUTTON (add_dialog->recurse), FALSE);
	gtk_box_pack_start (GTK_BOX (vbox3), add_dialog->recurse, FALSE, FALSE, 0);

	add_dialog->remove_files = gtk_check_button_new_with_mnemonic (_("Delete files after adding"));
	gtk_button_set_focus_on_click (GTK_BUTTON (add_dialog->remove_files), FALSE);
	gtk_box_pack_start (GTK_BOX (vbox3),add_dialog->remove_files, FALSE, FALSE, 0);

	add_dialog->solid_archive = gtk_check_button_new_with_mnemonic (_("Create a solid archive"));
	gtk_button_set_focus_on_click (GTK_BUTTON (add_dialog->solid_archive), FALSE);
	gtk_widget_set_tooltip_text(add_dialog->solid_archive, _("In a solid archive the files are grouped together resulting in a better compression ratio"));
	gtk_box_pack_start (GTK_BOX (vbox3), add_dialog->solid_archive, FALSE, FALSE, 0);

	label7 = gtk_label_new (_("Actions: "));
	gtk_frame_set_label_widget (GTK_FRAME (frame4), label7);

	hbox3 = gtk_hbox_new (TRUE, 10);
	gtk_box_pack_start (GTK_BOX (add_dialog->option_notebook_vbox), hbox3, TRUE, TRUE, 0);
	gtk_container_set_border_width (GTK_CONTAINER (hbox3), 5);

	frame2 = gtk_frame_new (NULL);
	gtk_box_pack_start (GTK_BOX (hbox3), frame2, TRUE, TRUE, 0);
	gtk_frame_set_shadow_type (GTK_FRAME (frame2), GTK_SHADOW_OUT);

	add_dialog->alignment2 = gtk_alignment_new(0.5, 0.5, 1, 0);
	gtk_container_add (GTK_CONTAINER (frame2), add_dialog->alignment2);
	gtk_alignment_set_padding (GTK_ALIGNMENT (add_dialog->alignment2), 0, 0, 5, 5);

	label4 = gtk_label_new (_("Compression: "));
	gtk_frame_set_label_widget (GTK_FRAME (frame2), label4);

	frame3 = gtk_frame_new (NULL);
	gtk_box_pack_start (GTK_BOX (hbox3), frame3, TRUE, TRUE, 0);
	gtk_frame_set_shadow_type (GTK_FRAME (frame3), GTK_SHADOW_OUT);

	alignment3 = gtk_alignment_new(0.5, 0.5, 1, 0);
	gtk_container_add (GTK_CONTAINER (frame3), alignment3);
	gtk_alignment_set_padding (GTK_ALIGNMENT (alignment3), 0, 0, 5, 5);

	hbox4 = gtk_hbox_new (FALSE, 0);
	gtk_container_add (GTK_CONTAINER (alignment3), hbox4);

	add_dialog->add_password = gtk_check_button_new_with_mnemonic (_("Password:"));
	gtk_box_pack_start (GTK_BOX (hbox4), add_dialog->add_password, FALSE, FALSE, 0);
	g_signal_connect (G_OBJECT (add_dialog->add_password), "toggled",G_CALLBACK (password_toggled_cb) , add_dialog);

	add_dialog->add_password_entry = gtk_entry_new ();
	gtk_box_pack_start (GTK_BOX (hbox4), add_dialog->add_password_entry, FALSE, FALSE, 0);
	gtk_entry_set_visibility (GTK_ENTRY (add_dialog->add_password_entry), FALSE);
	gtk_entry_set_width_chars (GTK_ENTRY (add_dialog->add_password_entry), 15);
	gtk_widget_set_sensitive (add_dialog->add_password_entry, FALSE);

	label5 = gtk_label_new (_("Encryption: "));
	gtk_frame_set_label_widget (GTK_FRAME (frame3), label5);

	dialog_action_area1 = gtk_dialog_get_action_area(GTK_DIALOG(add_dialog->dialog1));
	gtk_button_box_set_layout (GTK_BUTTON_BOX (dialog_action_area1), GTK_BUTTONBOX_END);

	add_dialog->cancel_button = gtk_button_new_from_stock ("gtk-cancel");
	gtk_dialog_add_action_widget (GTK_DIALOG (add_dialog->dialog1), add_dialog->cancel_button, GTK_RESPONSE_CANCEL);
	gtk_widget_set_can_default(add_dialog->cancel_button, TRUE);

	add_dialog->add_button = gtk_button_new();
	add_dialog->add_image = xa_main_window_find_image("xarchiver-add.png", GTK_ICON_SIZE_SMALL_TOOLBAR);
	add_dialog->add_hbox = gtk_hbox_new(FALSE, 4);
	add_dialog->add_label = gtk_label_new_with_mnemonic(_("_Add"));

	alignment2 = gtk_alignment_new (0.5, 0.5, 0, 0);
	gtk_container_add (GTK_CONTAINER (alignment2), add_dialog->add_hbox);

	gtk_box_pack_start(GTK_BOX(add_dialog->add_hbox), add_dialog->add_image, FALSE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(add_dialog->add_hbox), add_dialog->add_label, FALSE, TRUE, 0);
	gtk_container_add(GTK_CONTAINER(add_dialog->add_button), alignment2);

	gtk_dialog_add_action_widget (GTK_DIALOG (add_dialog->dialog1), add_dialog->add_button, GTK_RESPONSE_OK);
	gtk_widget_set_can_default(add_dialog->add_button, TRUE);
	gtk_dialog_set_default_response (GTK_DIALOG (add_dialog->dialog1), GTK_RESPONSE_OK);
	return add_dialog;
}

void xa_set_add_dialog_options(Add_dialog_data *add_dialog,XArchive *archive)
{
	GTK_COMPAT_TOOLTIPS;
	gboolean flag = FALSE;
	gchar *compression_msg = NULL;
	gushort default_value, max_value;
	default_value = max_value = 0;

	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(prefs_window->check_save_geometry)) && prefs_window->add_coords[0] != -1)
		gtk_window_set_default_size (GTK_WINDOW(add_dialog->dialog1), prefs_window->add_coords[0], prefs_window->add_coords[1]);
	else
		gtk_widget_set_size_request (add_dialog->dialog1,530,420);

	prefs_window->size_changed[1] = TRUE;

	gtk_file_chooser_set_select_multiple(GTK_FILE_CHOOSER(add_dialog->filechooserwidget1), !SINGLE_FILE_COMPRESSOR(archive));

	if ((archive->location_path && *archive->location_path) || !archive->can_full_path[1])
	{
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(add_dialog->no_store_path), TRUE);
		flag = FALSE;
	}
	else
		flag = TRUE;

	gtk_widget_set_sensitive(add_dialog->label, flag);
	gtk_widget_set_sensitive(add_dialog->store_path,flag);

	g_signal_connect (G_OBJECT (add_dialog->update),"toggled",G_CALLBACK (add_update_fresh_toggled_cb) , add_dialog);

	gtk_widget_set_sensitive(add_dialog->update, archive->can_update[1]);
	gtk_widget_set_sensitive(add_dialog->freshen, archive->can_freshen[1]);
	gtk_widget_set_sensitive(add_dialog->recurse, TRUE);
	gtk_widget_set_sensitive(add_dialog->remove_files, archive->can_move);
	gtk_widget_set_sensitive(add_dialog->solid_archive, archive->can_solid);

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
			compression_msg = _("0 = no compression, 1 is default, 4 = fastest but least compression");
			default_value = 1;
			max_value = 4;
		}
		else if (archive->type == XARCHIVETYPE_LHA)
		{
			compression_msg = _("5 = default compression, 7 = max compression");
			default_value = 5;
			max_value = 7;
		}
		if (archive->type == XARCHIVETYPE_XZ)
		{
			compression_msg = _("0 = no compression, 5 is default, 9 = best compression but slowest");
			default_value = 5;
			max_value = 9;
		}

	flag = (compression_msg != NULL);

	if (archive->type == XARCHIVETYPE_7ZIP)
		compression_value = gtk_adjustment_new(default_value, 0, max_value, 2, 2, 0);
	else if (archive->type == XARCHIVETYPE_LHA)
		compression_value = gtk_adjustment_new(default_value, 5, max_value, 7, 7, 0);
	else
		compression_value = gtk_adjustment_new(default_value, 0, max_value, 0, 0, 0);

	add_dialog->compression_scale = gtk_hscale_new(GTK_ADJUSTMENT(compression_value));
	if (gtk_bin_get_child(GTK_BIN(add_dialog->alignment2)) == NULL)
		gtk_container_add (GTK_CONTAINER (add_dialog->alignment2), add_dialog->compression_scale);
	gtk_scale_set_value_pos (GTK_SCALE (add_dialog->compression_scale), GTK_POS_TOP);
	gtk_scale_set_digits (GTK_SCALE (add_dialog->compression_scale), 0);

	if (archive->compression == 0)
		archive->compression = default_value;

	gtk_widget_set_sensitive(add_dialog->compression_scale,flag);
	gtk_adjustment_set_value(GTK_ADJUSTMENT(compression_value), archive->compression);

	if (archive->type == XARCHIVETYPE_ARJ)
		gtk_range_set_inverted (GTK_RANGE (add_dialog->compression_scale), TRUE);
	else if (archive->type == XARCHIVETYPE_7ZIP)
		g_signal_connect(G_OBJECT(compression_value), "value-changed", G_CALLBACK(fix_adjustment_value), NULL);
	gtk_widget_set_tooltip_text(add_dialog->compression_scale, compression_msg);

	gtk_widget_set_sensitive(add_dialog->add_password, archive->can_password);
	gtk_widget_show_all(add_dialog->dialog1);
}

void xa_parse_add_dialog_options (XArchive *archive,Add_dialog_data *add_dialog)
{
	gchar *temp_password = NULL;
	gchar *compression = NULL;
	gboolean done = FALSE;
	GSList *list = NULL;

	while ( ! done )
	{
		switch (gtk_dialog_run(GTK_DIALOG(add_dialog->dialog1)))
		{
			case GTK_RESPONSE_CANCEL:
			case GTK_RESPONSE_DELETE_EVENT:
			done = TRUE;
			break;

			case GTK_RESPONSE_OK:
			list = gtk_file_chooser_get_filenames(GTK_FILE_CHOOSER(add_dialog->filechooserwidget1));
			if (g_slist_length(list) == 0)
			{
				xa_show_message_dialog (GTK_WINDOW (xa_main_window),GTK_DIALOG_MODAL,GTK_MESSAGE_ERROR,GTK_BUTTONS_OK,_("Can't add files to the archive:"), _("You haven't selected any files to add!") );
				break;
			}
			if (gtk_widget_is_sensitive(add_dialog->add_password) && gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(add_dialog->add_password)))
			{
				temp_password  = g_strdup (gtk_entry_get_text ( GTK_ENTRY (add_dialog->add_password_entry) ));
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

			done = TRUE;
			if (gtk_widget_is_sensitive(add_dialog->store_path) && gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(add_dialog->store_path)))
				archive->do_full_path = TRUE;
			else
				archive->do_full_path = FALSE;

			if (gtk_widget_is_sensitive(add_dialog->update))
				archive->do_update = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON (add_dialog->update));

			if (gtk_widget_is_sensitive(add_dialog->remove_files))
				archive->do_move = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(add_dialog->remove_files));

			if (gtk_widget_is_sensitive(add_dialog->freshen))
				archive->do_freshen = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(add_dialog->freshen));

			if (gtk_widget_is_sensitive(add_dialog->solid_archive))
				archive->do_solid = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(add_dialog->solid_archive));

			if (gtk_widget_is_sensitive(add_dialog->compression_scale))
			{
				archive->compression = gtk_adjustment_get_value(GTK_ADJUSTMENT(compression_value));
				compression = g_strdup_printf("%hu", archive->compression);
			}
			gtk_widget_hide(add_dialog->dialog1);

			if (!archive->do_full_path)
				archive->child_dir = g_path_get_dirname(list->data);

			xa_execute_add_commands(archive, list, gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(add_dialog->recurse)), compression);
			g_free(compression);
		}
	}
	gtk_widget_hide (add_dialog->dialog1);
}

void xa_execute_add_commands (XArchive *archive, GSList *list, gboolean recurse, gchar *compression)
{
	gchar *new_path = NULL;
	gchar *esc2, *basedir;
	gboolean result = FALSE;
	GString *items;
	gchar *command = NULL;
	GSList *slist = NULL;
	GSList *dirlist = NULL;
	GSList *files;

	if (xa_main_window)
	{
		gtk_label_set_text(GTK_LABEL(total_label),_("Adding files to archive, please wait..."));
		/* This in case the user wants to add files in a directory in the archive tree */
		if (archive->location_path != NULL)
		{
			result = xa_create_working_directory(archive);
			if (result == FALSE)
				return;

			items = g_string_new("");
			new_path = g_strconcat(archive->working_dir, "/", archive->location_path, NULL);
			result = g_mkdir_with_parents(new_path,0700);
			if (result < 0)
			{
				g_free(new_path);
				return;
			}
			slist = list;
			basedir = g_path_get_dirname(slist->data);
			while (slist)
			{
				esc2 = g_shell_quote(slist->data);
				g_string_append(items,esc2);
				g_string_append_c(items,' ');
				g_free(esc2);
				slist = slist->next;
			}
			g_free(basedir);
			command = g_strconcat ("cp -rf ",items->str," ",new_path,NULL);
			g_free(new_path);
			g_string_free(items,TRUE);
			xa_run_command(archive, command);
			g_free(command);
		}
		xa_set_button_state(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, NULL, 0);
	}

	while (list)
	{
		xa_recurse_local_directory((gchar*) list->data, &dirlist, recurse);
		list = list->next;
	}
	files = xa_collect_filenames(archive, dirlist);
	g_slist_free_full(dirlist, g_free);

	archive->status = XARCHIVESTATUS_ADD;
	(*archive->archiver->add)(archive, files, compression);

	g_free(archive->child_dir);
	archive->child_dir = NULL;

	if (xa_main_window)
		xa_reload_archive_content(archive);
}
