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
#include <unistd.h>
#include <glib/gstdio.h>
#include "pref_dialog.h"
#include "add_dialog.h"
#include "archive.h"
#include "extract_dialog.h"
#include "interface.h"
#include "main.h"
#include "support.h"

gchar *config_file;
GtkIconTheme *icon_theme;

static gint preferred_format;

static gchar *xa_prefs_choose_program (gboolean flag)
{
	gchar *filename = NULL;
	GtkWidget *dialog;

	dialog = gtk_file_chooser_dialog_new (flag ? _("Choose the directory to use") : _("Choose the application to use"),
				      GTK_WINDOW(xa_main_window),
				      flag ? GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER : GTK_FILE_CHOOSER_ACTION_OPEN,
				      GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
				      GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
				      NULL);

	if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT)
		filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog));

	gtk_widget_destroy (dialog);

	return filename;
}

static void xa_prefs_combo_changed (GtkComboBox *widget, gpointer data)
{
	gchar *filename, *filename_utf8;
	unsigned short int flag = GPOINTER_TO_UINT(data);

	if (gtk_combo_box_get_active(GTK_COMBO_BOX (widget)) == 1)
	{
		filename = xa_prefs_choose_program(flag);
		if (filename != NULL)
		{
			filename_utf8 = g_filename_display_name(filename);
			gtk_combo_box_text_remove(GTK_COMBO_BOX_TEXT(GTK_WIDGET(widget)), 0);
			gtk_combo_box_text_insert_text(GTK_COMBO_BOX_TEXT(GTK_WIDGET(widget)), 0, filename_utf8);
			g_free(filename_utf8);
			g_free(filename);
		}
		gtk_combo_box_set_active (GTK_COMBO_BOX (widget),0);
	}
}

static void xa_prefs_dialog_set_default_options (PrefsDialog *prefs_dialog)
{
	gtk_combo_box_set_active(GTK_COMBO_BOX(prefs_dialog->preferred_format), 0);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(prefs_dialog->prefer_unzip), TRUE);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(prefs_dialog->confirm_deletion), TRUE);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(prefs_dialog->advanced_isearch), TRUE);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(prefs_dialog->auto_expand), TRUE);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(prefs_dialog->store_output), FALSE);

	gtk_combo_box_set_active(GTK_COMBO_BOX(prefs_dialog->icon_size), 0);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(prefs_dialog->show_toolbar), TRUE);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(prefs_dialog->show_location_bar), TRUE);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(prefs_dialog->show_sidebar), TRUE);

	if (!xdg_open)
	{
		gtk_combo_box_set_active(GTK_COMBO_BOX(prefs_dialog->preferred_browser), 0);
		gtk_combo_box_set_active(GTK_COMBO_BOX(prefs_dialog->preferred_editor), 0);
		gtk_combo_box_set_active(GTK_COMBO_BOX(prefs_dialog->preferred_viewer), 0);
		gtk_combo_box_set_active(GTK_COMBO_BOX(prefs_dialog->preferred_archiver), 0);
	}
	gtk_combo_box_set_active(GTK_COMBO_BOX(prefs_dialog->preferred_custom_cmd), 0);
	gtk_combo_box_set_active(GTK_COMBO_BOX(prefs_dialog->preferred_temp_dir), 0);
	gtk_combo_box_set_active(GTK_COMBO_BOX(prefs_dialog->preferred_extract_dir), 0);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(prefs_dialog->save_geometry), FALSE);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(prefs_dialog->allow_sub_dir), FALSE);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(prefs_dialog->extended_dnd), g_getenv("WAYLAND_DISPLAY") != NULL);
	/* Set the default options in the extract dialog */
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(extract_window->ensure_directory), TRUE);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(extract_window->full_path), TRUE);
	/* Set the default options in the add dialog */
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(add_window->update), TRUE);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (add_window->recurse),TRUE);
}

void xa_prefs_iconview_changed (GtkIconView *iconview, PrefsDialog *prefs_dialog)
{
	GList *list;
	GtkTreePath *path;
	GtkTreeIter iter;
	guint column = 0;

	list = gtk_icon_view_get_selected_items (iconview);
	if (list == NULL)
		return;

	list = g_list_first (list);
	path = (GtkTreePath*)list->data;

	gtk_tree_model_get_iter(GTK_TREE_MODEL(prefs_dialog->liststore), &iter, path);
	gtk_tree_model_get(GTK_TREE_MODEL(prefs_dialog->liststore), &iter, 2, &column, -1);

	gtk_tree_path_free(path);
	g_list_free (list);

	gtk_notebook_set_current_page(GTK_NOTEBOOK(prefs_dialog->notebook), column);
}

PrefsDialog *xa_create_prefs_dialog ()
{
	GTK_COMPAT_TOOLTIPS;
	PrefsDialog *prefs_dialog;
	GtkWidget *hbox, *window, *alignment, *vbox, *label, *table;
	GdkPixbuf *icon_pixbuf;
	GtkTreeIter iter;
	GtkTreePath *top;

	prefs_dialog = g_new0(PrefsDialog, 1);

	prefs_dialog->dialog = gtk_dialog_new_with_buttons(_("Preferences"), NULL,
	                                                   GTK_DIALOG_MODAL,
	                                                   GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
	                                                   GTK_STOCK_OK, GTK_RESPONSE_OK,
	                                                   NULL);
	gtk_window_set_position(GTK_WINDOW(prefs_dialog->dialog), GTK_WIN_POS_CENTER_ON_PARENT);
	gtk_dialog_set_default_response(GTK_DIALOG(prefs_dialog->dialog), GTK_RESPONSE_OK);

	icon_theme = gtk_icon_theme_get_default();

	hbox = gtk_hbox_new(FALSE, 6);
	gtk_box_pack_start(GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(prefs_dialog->dialog))), hbox, TRUE, TRUE, 10);

	window = gtk_scrolled_window_new(NULL, NULL);
	gtk_box_pack_start(GTK_BOX(hbox), window, FALSE, FALSE, 6);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(window), GTK_POLICY_NEVER, GTK_POLICY_NEVER);
	gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(window), GTK_SHADOW_IN);

	prefs_dialog->liststore = gtk_list_store_new(3, GDK_TYPE_PIXBUF, G_TYPE_STRING, G_TYPE_UINT);

	icon_pixbuf = gtk_icon_theme_load_icon(icon_theme, "package-x-generic", 32, GTK_ICON_LOOKUP_FORCE_SIZE, NULL);

	gtk_list_store_append(prefs_dialog->liststore, &iter);
	gtk_list_store_set(prefs_dialog->liststore, &iter, 0, icon_pixbuf, 1, _("Archive"), 2, 0, -1);

	if (icon_pixbuf)
		g_object_unref(icon_pixbuf);

	icon_pixbuf = gtk_icon_theme_load_icon(icon_theme, "view-fullscreen", 32, GTK_ICON_LOOKUP_FORCE_SIZE, NULL);

	gtk_list_store_append(prefs_dialog->liststore, &iter);
	gtk_list_store_set(prefs_dialog->liststore, &iter, 0, icon_pixbuf, 1, _("Window"), 2, 1, -1);

	if (icon_pixbuf)
		g_object_unref(icon_pixbuf);

	icon_pixbuf = gtk_icon_theme_load_icon(icon_theme, "system-run", 32, GTK_ICON_LOOKUP_FORCE_SIZE, NULL);

	gtk_list_store_append(prefs_dialog->liststore, &iter);
	gtk_list_store_set(prefs_dialog->liststore, &iter, 0, icon_pixbuf, 1, _("Advanced"), 2, 2, -1);

	if (icon_pixbuf)
		g_object_unref(icon_pixbuf);

	prefs_dialog->iconview = gtk_icon_view_new_with_model(GTK_TREE_MODEL(prefs_dialog->liststore));
	g_object_unref(prefs_dialog->liststore);

	gtk_container_add(GTK_CONTAINER(window), prefs_dialog->iconview);
	gtk_icon_view_set_pixbuf_column(GTK_ICON_VIEW(prefs_dialog->iconview), 0);
	gtk_icon_view_set_text_column(GTK_ICON_VIEW(prefs_dialog->iconview), 1);
	gtk_icon_view_set_item_orientation(GTK_ICON_VIEW(prefs_dialog->iconview), GTK_ORIENTATION_VERTICAL);
	gtk_icon_view_set_columns(GTK_ICON_VIEW(prefs_dialog->iconview), 1);

	top = gtk_tree_path_new_from_string("0");
	gtk_icon_view_select_path(GTK_ICON_VIEW(prefs_dialog->iconview), top);
	gtk_tree_path_free(top);

	prefs_dialog->notebook = gtk_notebook_new();
	gtk_notebook_set_show_tabs(GTK_NOTEBOOK(prefs_dialog->notebook), FALSE);
	gtk_notebook_set_show_border(GTK_NOTEBOOK(prefs_dialog->notebook), FALSE);
	g_signal_connect(G_OBJECT(prefs_dialog->iconview), "selection-changed", G_CALLBACK(xa_prefs_iconview_changed), prefs_dialog);

	alignment = gtk_alignment_new(0.5, 0.5, 1, 1);
	gtk_container_add(GTK_CONTAINER(alignment), prefs_dialog->notebook);
	gtk_box_pack_start(GTK_BOX(hbox), alignment, TRUE, TRUE, 0);
	gtk_alignment_set_padding(GTK_ALIGNMENT(alignment), 0, 0, 0, 5);

	/* Archive page*/

	vbox = gtk_vbox_new(FALSE, 2);
	gtk_notebook_append_page(GTK_NOTEBOOK(prefs_dialog->notebook), vbox, NULL);

	hbox = gtk_hbox_new(FALSE, 5);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);

	label = gtk_label_new(_("Preferred archive format:"));
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);

	prefs_dialog->preferred_format = gtk_combo_box_text_new();
	gtk_box_pack_start(GTK_BOX(hbox), prefs_dialog->preferred_format, FALSE, FALSE, 0);
	gtk_combo_box_set_focus_on_click(GTK_COMBO_BOX(prefs_dialog->preferred_format), FALSE);

	prefs_dialog->prefer_unzip = gtk_check_button_new_with_mnemonic(_("Prefer unzip for zip files (requires restart)"));
	gtk_widget_set_tooltip_text(prefs_dialog->prefer_unzip, _("Even if other, perhaps more powerful programs are installed to handle zip archives, the traditional \"unzip\" and \"zip\" will still be used."));
	gtk_box_pack_start(GTK_BOX(vbox), prefs_dialog->prefer_unzip, FALSE, FALSE, 0);
	gtk_button_set_focus_on_click(GTK_BUTTON(prefs_dialog->prefer_unzip), FALSE);

	prefs_dialog->confirm_deletion = gtk_check_button_new_with_mnemonic(_("Confirm deletion of files"));
	gtk_box_pack_start(GTK_BOX(vbox), prefs_dialog->confirm_deletion, FALSE, FALSE, 0);
	gtk_button_set_focus_on_click(GTK_BUTTON(prefs_dialog->confirm_deletion), FALSE);

	prefs_dialog->sort_by_filenames = gtk_check_button_new_with_mnemonic(_("Sort archive by filename"));
	gtk_widget_set_tooltip_text(prefs_dialog->sort_by_filenames, _("The filename column is sorted after loading the archive."));
	gtk_box_pack_start(GTK_BOX(vbox), prefs_dialog->sort_by_filenames, FALSE, FALSE, 0);
	gtk_button_set_focus_on_click(GTK_BUTTON(prefs_dialog->sort_by_filenames), FALSE);

	prefs_dialog->advanced_isearch = gtk_check_button_new_with_mnemonic(_("Advanced incremental search (requires restart)"));
	gtk_widget_set_tooltip_text(prefs_dialog->advanced_isearch, _("The incremental search is also performed within filenames rather than just at the beginning of them."));
	gtk_box_pack_start(GTK_BOX(vbox), prefs_dialog->advanced_isearch, FALSE, FALSE, 0);
	gtk_button_set_focus_on_click(GTK_BUTTON(prefs_dialog->advanced_isearch), FALSE);

	prefs_dialog->auto_expand = gtk_check_button_new_with_mnemonic(_("Automatically expand archive tree nodes on selection"));
	gtk_widget_set_tooltip_text(prefs_dialog->auto_expand, _("Archive tree nodes already expand when the node name is selected by mouse click or keyboard shortcut."));
	gtk_box_pack_start(GTK_BOX(vbox), prefs_dialog->auto_expand, FALSE, FALSE, 0);
	gtk_button_set_focus_on_click(GTK_BUTTON(prefs_dialog->auto_expand), FALSE);

	prefs_dialog->store_output = gtk_check_button_new_with_mnemonic(_("Store archiver output"));
	gtk_widget_set_tooltip_text(prefs_dialog->store_output, _("Command-line output is captured and can be reviewed, but this consumes additional memory."));
	gtk_box_pack_start(GTK_BOX(vbox), prefs_dialog->store_output, FALSE, FALSE, 0);
	gtk_button_set_focus_on_click(GTK_BUTTON(prefs_dialog->store_output), FALSE);

	/* Window page*/

	vbox = gtk_vbox_new(FALSE, 2);
	gtk_notebook_append_page(GTK_NOTEBOOK(prefs_dialog->notebook), vbox, NULL);

	hbox = gtk_hbox_new(FALSE, 5);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);

	label = gtk_label_new(_("Icons size (requires restart):"));
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);

	prefs_dialog->icon_size = gtk_combo_box_text_new();
	gtk_box_pack_start(GTK_BOX(hbox), prefs_dialog->icon_size, FALSE, FALSE, 0);
	gtk_combo_box_set_focus_on_click(GTK_COMBO_BOX(prefs_dialog->icon_size), FALSE);

	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(prefs_dialog->icon_size), _("small"));
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(prefs_dialog->icon_size), _("small/medium"));
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(prefs_dialog->icon_size), _("medium"));
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(prefs_dialog->icon_size), _("large"));
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(prefs_dialog->icon_size), _("very large"));

	prefs_dialog->show_comment = gtk_check_button_new_with_mnemonic(_("Show archive comment"));
	gtk_widget_set_tooltip_text(prefs_dialog->show_comment, _("If checked the archive comment is shown after the archive is loaded."));
	gtk_box_pack_start(GTK_BOX(vbox), prefs_dialog->show_comment, FALSE, FALSE, 0);
	gtk_button_set_focus_on_click(GTK_BUTTON(prefs_dialog->show_comment), FALSE);

	prefs_dialog->show_sidebar = gtk_check_button_new_with_mnemonic(_("Show archive tree sidebar"));
	gtk_box_pack_start(GTK_BOX(vbox), prefs_dialog->show_sidebar, FALSE, FALSE, 0);
	gtk_button_set_focus_on_click(GTK_BUTTON(prefs_dialog->show_sidebar), FALSE);

	prefs_dialog->show_location_bar = gtk_check_button_new_with_mnemonic(_("Show archive location bar"));
	gtk_box_pack_start(GTK_BOX(vbox), prefs_dialog->show_location_bar, FALSE, FALSE, 0);
	gtk_button_set_focus_on_click(GTK_BUTTON(prefs_dialog->show_location_bar), FALSE);

	prefs_dialog->show_toolbar = gtk_check_button_new_with_mnemonic(_("Show toolbar"));
	gtk_box_pack_start(GTK_BOX(vbox), prefs_dialog->show_toolbar, FALSE, FALSE, 0);
	gtk_button_set_focus_on_click(GTK_BUTTON(prefs_dialog->show_toolbar), FALSE);

	/* Advanced page*/

	vbox = gtk_vbox_new(FALSE, 0);
	gtk_notebook_append_page(GTK_NOTEBOOK(prefs_dialog->notebook), vbox, NULL);

	table = gtk_table_new(10, 2, FALSE);
	gtk_box_pack_start(GTK_BOX(vbox), table, TRUE, TRUE, 0);
	gtk_table_set_row_spacings(GTK_TABLE(table), 1);

	if (!xdg_open)
	{
		const gchar *xdg_tip = _("If you install \"xdg-open\" from the \"xdg-utils\" package, each file will be opened with its default application.");

		label = gtk_label_new(_("Web browser to use:"));
		gtk_table_attach(GTK_TABLE(table), label,
		                 0, 1, 0, 1, GTK_EXPAND | GTK_FILL, GTK_SHRINK, 0, 0);
		gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);

		prefs_dialog->preferred_browser = gtk_combo_box_text_new();
		gtk_widget_set_tooltip_text(prefs_dialog->preferred_browser, xdg_tip);
		gtk_table_attach(GTK_TABLE(table), prefs_dialog->preferred_browser,
		                 1, 2, 0, 1, GTK_EXPAND | GTK_FILL, GTK_SHRINK, 0, 0);
		gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(prefs_dialog->preferred_browser), "");
		gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(prefs_dialog->preferred_browser), _("choose..."));
		gtk_combo_box_set_focus_on_click(GTK_COMBO_BOX(prefs_dialog->preferred_browser), FALSE);
		g_signal_connect(prefs_dialog->preferred_browser, "changed", G_CALLBACK(xa_prefs_combo_changed), NULL);

		label = gtk_label_new(_("Open text files with:"));
		gtk_table_attach(GTK_TABLE(table), label,
		                 0, 1, 1, 2, GTK_EXPAND | GTK_FILL, GTK_SHRINK, 0, 0);
		gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);

		prefs_dialog->preferred_editor = gtk_combo_box_text_new();
		gtk_widget_set_tooltip_text(prefs_dialog->preferred_editor, xdg_tip);
		gtk_table_attach(GTK_TABLE(table), prefs_dialog->preferred_editor,
		                 1, 2, 1, 2, GTK_EXPAND | GTK_FILL, GTK_SHRINK, 0, 0);
		gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(prefs_dialog->preferred_editor), "");
		gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(prefs_dialog->preferred_editor), _("choose..."));
		gtk_combo_box_set_focus_on_click(GTK_COMBO_BOX(prefs_dialog->preferred_editor), FALSE);
		g_signal_connect(prefs_dialog->preferred_editor, "changed", G_CALLBACK(xa_prefs_combo_changed), NULL);

		label = gtk_label_new(_("Open image files with:"));
		gtk_table_attach(GTK_TABLE(table), label,
		                 0, 1, 2, 3, GTK_EXPAND | GTK_FILL, GTK_SHRINK, 0, 0);
		gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);

		prefs_dialog->preferred_viewer = gtk_combo_box_text_new();
		gtk_widget_set_tooltip_text(prefs_dialog->preferred_viewer, xdg_tip);
		gtk_table_attach(GTK_TABLE(table), prefs_dialog->preferred_viewer,
		                 1, 2, 2, 3, GTK_EXPAND | GTK_FILL, GTK_SHRINK, 0, 0);
		gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(prefs_dialog->preferred_viewer), "");
		gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(prefs_dialog->preferred_viewer), _("choose..."));
		gtk_combo_box_set_focus_on_click(GTK_COMBO_BOX(prefs_dialog->preferred_viewer), FALSE);
		g_signal_connect(prefs_dialog->preferred_viewer, "changed", G_CALLBACK(xa_prefs_combo_changed), NULL);

		label = gtk_label_new(_("Open archive files with:"));
		gtk_table_attach(GTK_TABLE(table), label,
		                 0, 1, 3, 4, GTK_EXPAND | GTK_FILL, GTK_SHRINK, 0, 0);
		gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);

		prefs_dialog->preferred_archiver = gtk_combo_box_text_new();
		gtk_widget_set_tooltip_text(prefs_dialog->preferred_archiver, xdg_tip);
		gtk_table_attach(GTK_TABLE(table), prefs_dialog->preferred_archiver,
		                 1, 2, 3, 4, GTK_EXPAND | GTK_FILL, GTK_SHRINK, 0, 0);
		gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(prefs_dialog->preferred_archiver), "");
		gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(prefs_dialog->preferred_archiver), _("choose..."));
		gtk_combo_box_set_focus_on_click(GTK_COMBO_BOX(prefs_dialog->preferred_archiver), FALSE);
		g_signal_connect(prefs_dialog->preferred_archiver, "changed", G_CALLBACK(xa_prefs_combo_changed), NULL);

		gtk_table_set_row_spacing(GTK_TABLE(table), 3, 10);
	}

	label = gtk_label_new(_("Default custom command:"));
	gtk_table_attach(GTK_TABLE(table), label,
	                 0, 1, 4, 5, GTK_EXPAND | GTK_FILL, GTK_SHRINK, 0, 0);
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);

	prefs_dialog->preferred_custom_cmd = gtk_combo_box_text_new();
	gtk_table_attach(GTK_TABLE(table), prefs_dialog->preferred_custom_cmd,
	                 1, 2, 4, 5, GTK_EXPAND | GTK_FILL, GTK_SHRINK, 0, 0);
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(prefs_dialog->preferred_custom_cmd), "");
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(prefs_dialog->preferred_custom_cmd), _("choose..."));
	gtk_combo_box_set_focus_on_click(GTK_COMBO_BOX(prefs_dialog->preferred_custom_cmd), FALSE);
	g_signal_connect(prefs_dialog->preferred_custom_cmd, "changed", G_CALLBACK(xa_prefs_combo_changed), NULL);

	label = gtk_label_new(_("Preferred temp directory:"));
	gtk_table_attach(GTK_TABLE(table), label,
	                 0, 1, 5, 6, GTK_EXPAND | GTK_FILL, GTK_SHRINK, 0, 0);
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);

	prefs_dialog->preferred_temp_dir = gtk_combo_box_text_new();
	gtk_table_attach(GTK_TABLE(table), prefs_dialog->preferred_temp_dir,
	                 1, 2, 5, 6, GTK_EXPAND | GTK_FILL, GTK_SHRINK, 0, 0);
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(prefs_dialog->preferred_temp_dir), _("/tmp"));
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(prefs_dialog->preferred_temp_dir), _("choose..."));
	gtk_combo_box_set_focus_on_click(GTK_COMBO_BOX(prefs_dialog->preferred_temp_dir), FALSE);
	g_signal_connect(prefs_dialog->preferred_temp_dir, "changed", G_CALLBACK(xa_prefs_combo_changed), GUINT_TO_POINTER(1));

	label = gtk_label_new(_("Preferred extraction directory:"));
	gtk_table_attach(GTK_TABLE(table), label,
	                 0, 1, 6, 7, GTK_EXPAND | GTK_FILL, GTK_SHRINK, 0, 0);
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);

	prefs_dialog->preferred_extract_dir = gtk_combo_box_text_new();
	gtk_table_attach(GTK_TABLE(table), prefs_dialog->preferred_extract_dir,
	                 1, 2, 6, 7, GTK_EXPAND | GTK_FILL, GTK_SHRINK, 0, 0);
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(prefs_dialog->preferred_extract_dir), _("/tmp"));
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(prefs_dialog->preferred_extract_dir), _("choose..."));
	gtk_combo_box_set_focus_on_click(GTK_COMBO_BOX(prefs_dialog->preferred_extract_dir), FALSE);
	g_signal_connect(prefs_dialog->preferred_extract_dir, "changed", G_CALLBACK(xa_prefs_combo_changed), GUINT_TO_POINTER(1));

	prefs_dialog->save_geometry = gtk_check_button_new_with_mnemonic(_("Save window geometry"));
	gtk_table_attach(GTK_TABLE(table), prefs_dialog->save_geometry,
	                 0, 2, 7, 8, GTK_EXPAND | GTK_FILL, GTK_SHRINK, 0, 4);
	gtk_button_set_focus_on_click(GTK_BUTTON(prefs_dialog->save_geometry), FALSE);

	prefs_dialog->allow_sub_dir = gtk_check_button_new_with_mnemonic(_("Allow subdirs with clipboard and drag-and-drop"));
	gtk_widget_set_tooltip_text(prefs_dialog->allow_sub_dir, _("This option includes the subdirectories when you add files from the clipboard or with drag-and-drop."));
	gtk_table_attach(GTK_TABLE(table), prefs_dialog->allow_sub_dir,
	                 0, 2, 8, 9, GTK_EXPAND | GTK_FILL, GTK_SHRINK, 0, 0);
	gtk_button_set_focus_on_click(GTK_BUTTON(prefs_dialog->allow_sub_dir), FALSE);

	prefs_dialog->extended_dnd = gtk_check_button_new_with_mnemonic(_("Extended drag-and-drop support (requires restart)"));
	gtk_widget_set_tooltip_text(prefs_dialog->extended_dnd, _("This enables drag-and-drop transfers of type \"text/uri-list\", which is only necessary if you are not using the X Window System or if the targets do not support XDirectSave.\nIf you are using the X Window System and drag-and-drop works without this extension, do not enable it to avoid drawbacks."));
	gtk_table_attach(GTK_TABLE(table), prefs_dialog->extended_dnd,
	                 0, 2, 9, 10, GTK_EXPAND | GTK_FILL, GTK_SHRINK, 0, 4);
	gtk_button_set_focus_on_click(GTK_BUTTON(prefs_dialog->extended_dnd), FALSE);

	return prefs_dialog;
}

void xa_prefs_save_options (PrefsDialog *prefs_dialog, const char *filename)
{
	gchar *conf;
	gchar *value= NULL;
	gsize len;
	gint val;

	if (access(filename, F_OK) == 0 && access(filename, W_OK) == -1)
		return;

	GKeyFile *xa_key_file = g_key_file_new();

	g_key_file_set_integer(xa_key_file, PACKAGE, "preferred_format", gtk_combo_box_get_active(GTK_COMBO_BOX(prefs_dialog->preferred_format)));
	g_key_file_set_boolean(xa_key_file, PACKAGE, "prefer_unzip", gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(prefs_dialog->prefer_unzip)));
	g_key_file_set_boolean(xa_key_file, PACKAGE, "confirm_deletion", gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(prefs_dialog->confirm_deletion)));
	g_key_file_set_boolean(xa_key_file, PACKAGE, "sort_filename_content", gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(prefs_dialog->sort_by_filenames)));
	g_key_file_set_boolean(xa_key_file, PACKAGE, "advanced_isearch", gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(prefs_dialog->advanced_isearch)));
	g_key_file_set_boolean(xa_key_file, PACKAGE, "auto_expand", gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(prefs_dialog->auto_expand)));
	g_key_file_set_boolean(xa_key_file, PACKAGE, "store_output", gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(prefs_dialog->store_output)));

	g_key_file_set_integer(xa_key_file, PACKAGE, "icon_size", gtk_combo_box_get_active(GTK_COMBO_BOX(prefs_dialog->icon_size)) + 2);
	g_key_file_set_boolean(xa_key_file, PACKAGE, "show_archive_comment", gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(prefs_dialog->show_comment)));
	g_key_file_set_boolean(xa_key_file, PACKAGE, "show_sidebar", gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(prefs_dialog->show_sidebar)));
	g_key_file_set_boolean(xa_key_file, PACKAGE, "show_location_bar", gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(prefs_dialog->show_location_bar)));
	g_key_file_set_boolean(xa_key_file, PACKAGE, "show_toolbar", gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(prefs_dialog->show_toolbar)));

	if (!xdg_open)
	{
		value = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(prefs_dialog->preferred_browser));
		if (value != NULL)
		{
			g_key_file_set_string (xa_key_file,PACKAGE,"preferred_web_browser",value);
			g_free (value);
		}
		value = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(prefs_dialog->preferred_editor));
		if (value != NULL)
		{
			g_key_file_set_string (xa_key_file,PACKAGE,"preferred_editor",value);
			g_free(value);
		}
		value = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(prefs_dialog->preferred_viewer));
		if (value != NULL)
		{
			g_key_file_set_string (xa_key_file,PACKAGE,"preferred_viewer",value);
			g_free(value);
		}

		value = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(prefs_dialog->preferred_archiver));

		if (value != NULL)
		{
			g_key_file_set_string(xa_key_file, PACKAGE, "preferred_archiver", value);
			g_free(value);
		}
	}

	value = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(prefs_dialog->preferred_custom_cmd));

	if (value != NULL)
	{
		g_key_file_set_string(xa_key_file, PACKAGE, "preferred_custom_cmd", value);
		g_free(value);
	}

	value = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(prefs_dialog->preferred_temp_dir));
	if (value != NULL)
	{
		g_key_file_set_string (xa_key_file,PACKAGE,"preferred_temp_dir",value);
		g_free(value);
	}
	value = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(prefs_dialog->preferred_extract_dir));
	if (value != NULL)
	{
		g_key_file_set_string (xa_key_file,PACKAGE,"preferred_extract_dir",value);
		g_free(value);
	}
	g_key_file_set_integer(xa_key_file, PACKAGE, "allow_sub_dir", gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(prefs_dialog->allow_sub_dir)));
	g_key_file_set_integer(xa_key_file, PACKAGE, "extended_dnd", gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(prefs_dialog->extended_dnd)));
	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(prefs_dialog->save_geometry)))
	{
		/* Main window coords */
		gtk_window_get_position(GTK_WINDOW(xa_main_window), &prefs_dialog->main_win_geometry[0], &prefs_dialog->main_win_geometry[1]);
		gtk_window_get_size(GTK_WINDOW(xa_main_window), &prefs_dialog->main_win_geometry[2], &prefs_dialog->main_win_geometry[3]);
		prefs_dialog->main_win_geometry[4] = gtk_paned_get_position(GTK_PANED(hpaned1));
		g_key_file_set_integer_list(xa_key_file, PACKAGE, "mainwindow", prefs_dialog->main_win_geometry, 5);
		/* Extract dialog coords */
		if (prefs_dialog->size_changed[0])
			gtk_window_get_size(GTK_WINDOW(extract_window->dialog), &prefs_dialog->extract_win_size[0], &prefs_dialog->extract_win_size[1]);
		g_key_file_set_integer_list(xa_key_file, PACKAGE, "extract", prefs_dialog->extract_win_size, 2);
		/* Add dialog coords */
		if (prefs_dialog->size_changed[1])
			gtk_window_get_size(GTK_WINDOW(add_window->dialog), &prefs_dialog->add_win_size[0], &prefs_dialog->add_win_size[1]);
		g_key_file_set_integer_list(xa_key_file, PACKAGE, "add", prefs_dialog->add_win_size, 2);
	}
	/* Save the options in the extract dialog */
	g_key_file_set_boolean(xa_key_file, PACKAGE, "ensure_directory", gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(extract_window->ensure_directory)));
	g_key_file_set_boolean(xa_key_file, PACKAGE, "overwrite", gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(extract_window->overwrite)));

	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(extract_window->full_path)))
		val = 2;
	else if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(extract_window->relative_path)))
		val = 1;
	else
		val = 0;

	g_key_file_set_integer(xa_key_file, PACKAGE, "full_path", val);

	g_key_file_set_boolean (xa_key_file,PACKAGE,"touch",    gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (extract_window->touch)));
	g_key_file_set_boolean (xa_key_file,PACKAGE,"fresh",  gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (extract_window->freshen)));
	g_key_file_set_boolean (xa_key_file,PACKAGE,"update",   gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (extract_window->update)));
	/* Save the options in the add dialog */
	g_key_file_set_boolean (xa_key_file,PACKAGE,"store_path",	gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (add_window->full_path)));
	g_key_file_set_boolean (xa_key_file,PACKAGE,"updadd",		gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (add_window->update)));
	g_key_file_set_boolean (xa_key_file,PACKAGE,"freshen",		gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (add_window->freshen)));
	g_key_file_set_boolean (xa_key_file,PACKAGE,"recurse",   	gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (add_window->recurse)));
	g_key_file_set_boolean (xa_key_file,PACKAGE,"solid_archive",gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (add_window->solid)));
	g_key_file_set_boolean (xa_key_file,PACKAGE,"remove_files", gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (add_window->remove)));

	conf = g_key_file_to_data(xa_key_file, &len, NULL);
	g_file_set_contents(filename, conf, len, NULL);
	g_free (conf);
	g_key_file_free(xa_key_file);
}

void xa_prefs_load_options (PrefsDialog *prefs_dialog)
{
	gint *main_win_geometry;
	gint *extract_win_size;
	gint *add_win_size;
	gchar *value;
	gsize length;
	const gchar *config_dir;
	gchar *xarchiver_config_dir, *old_dir, *old_config;
	GKeyFile *xa_key_file = g_key_file_new();
	GError *error = NULL;
	gboolean unzip, aisearch, aexpand, toolbar, extdnd;
	gint idx, val;

	config_dir = g_get_user_config_dir();
	if (g_file_test(config_dir, G_FILE_TEST_EXISTS) == FALSE)
		g_mkdir_with_parents(config_dir,0700);

	xarchiver_config_dir = g_strconcat(config_dir, "/", PACKAGE, NULL);
	if (g_file_test(xarchiver_config_dir, G_FILE_TEST_EXISTS) == FALSE)
		g_mkdir_with_parents(xarchiver_config_dir,0700);

	config_file = g_strconcat(xarchiver_config_dir, "/", PACKAGE, "rc", NULL);
	g_free (xarchiver_config_dir);

	if (!g_file_test(config_file, G_FILE_TEST_EXISTS))
	{
		/* check legacy location of config file */
		old_dir = g_strconcat(g_get_home_dir(), "/.config/", PACKAGE, NULL);
		old_config = g_strconcat(old_dir, "/", PACKAGE, "rc", NULL);

		/* move it if it exists */
		if (g_file_test(old_config, G_FILE_TEST_EXISTS))
			if (g_rename(old_config, config_file) == 0)
				g_rmdir(old_dir);

		g_free(old_dir);
		g_free(old_config);
	}

	if ( ! g_key_file_load_from_file(xa_key_file,config_file,G_KEY_FILE_KEEP_COMMENTS,NULL) )
	{
		/* Write the config file with the default options */
		xa_prefs_dialog_set_default_options(prefs_dialog);
		xa_prefs_save_options(prefs_dialog, config_file);
	}
	else
	{
		preferred_format = g_key_file_get_integer(xa_key_file, PACKAGE, "preferred_format", NULL);
		unzip = g_key_file_get_boolean(xa_key_file, PACKAGE, "prefer_unzip", &error);

		if (error)
		{
			if (error->code == G_KEY_FILE_ERROR_KEY_NOT_FOUND)
				/* preserve default setting with existing, old config file */
				unzip = TRUE;

			g_error_free(error);
			error = NULL;
		}

		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(prefs_dialog->prefer_unzip), unzip);
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(prefs_dialog->confirm_deletion), g_key_file_get_boolean(xa_key_file, PACKAGE, "confirm_deletion", NULL));
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(prefs_dialog->store_output), g_key_file_get_boolean(xa_key_file, PACKAGE, "store_output", NULL));

		idx = g_key_file_get_integer(xa_key_file, PACKAGE, "icon_size", NULL) - 2;

		/* preserve setting with existing, old config file */
		if (idx == -2)
			idx = 3;
		if (idx < 0)
			idx = 0;

		gtk_combo_box_set_active(GTK_COMBO_BOX(prefs_dialog->icon_size), idx);
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(prefs_dialog->show_comment), g_key_file_get_boolean(xa_key_file, PACKAGE, "show_archive_comment", NULL));
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(prefs_dialog->sort_by_filenames), g_key_file_get_boolean(xa_key_file, PACKAGE, "sort_filename_content", NULL));
		aisearch = g_key_file_get_boolean(xa_key_file, PACKAGE, "advanced_isearch", &error);

		if (error)
		{
			if (error->code == G_KEY_FILE_ERROR_KEY_NOT_FOUND)
				/* preserve default setting with existing, old config file */
				aisearch = TRUE;

			g_error_free(error);
			error = NULL;
		}

		aexpand = g_key_file_get_boolean(xa_key_file, PACKAGE, "auto_expand", &error);

		if (error)
		{
			if (error->code == G_KEY_FILE_ERROR_KEY_NOT_FOUND)
				/* preserve default setting with existing, old config file */
				aexpand = TRUE;

			g_error_free(error);
			error = NULL;
		}

		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(prefs_dialog->advanced_isearch), aisearch);
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(prefs_dialog->auto_expand), aexpand);
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(prefs_dialog->show_sidebar), g_key_file_get_boolean(xa_key_file, PACKAGE, "show_sidebar", NULL));
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(prefs_dialog->show_location_bar), g_key_file_get_boolean(xa_key_file, PACKAGE, "show_location_bar", NULL));
		toolbar = g_key_file_get_boolean(xa_key_file,PACKAGE,"show_toolbar",&error);
		if (error)
		{
			if (error->code == G_KEY_FILE_ERROR_KEY_NOT_FOUND)
				/* preserve default setting with existing, old config file */
				toolbar = TRUE;

			g_error_free(error);
			error = NULL;
		}
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(prefs_dialog->show_toolbar), toolbar);
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(prefs_dialog->allow_sub_dir), g_key_file_get_boolean(xa_key_file, PACKAGE, "allow_sub_dir", NULL));
		extdnd = g_key_file_get_boolean(xa_key_file, PACKAGE, "extended_dnd", &error);
		if (error)
		{
			if (error->code == G_KEY_FILE_ERROR_KEY_NOT_FOUND)
				/* default setting with existing, old config file */
				extdnd = (g_getenv("WAYLAND_DISPLAY") != NULL);

			g_error_free(error);
			error = NULL;
		}
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(prefs_dialog->extended_dnd), extdnd);
		if (!xdg_open)
		{
			value = g_key_file_get_string(xa_key_file,PACKAGE,"preferred_web_browser",NULL);
			if (value != NULL)
			{
				gtk_combo_box_text_remove(GTK_COMBO_BOX_TEXT(prefs_dialog->preferred_browser), 0);
				gtk_combo_box_text_prepend_text(GTK_COMBO_BOX_TEXT(prefs_dialog->preferred_browser), value);
				gtk_combo_box_set_active(GTK_COMBO_BOX(prefs_dialog->preferred_browser), 0);
				g_free(value);
			}
			value = g_key_file_get_string(xa_key_file,PACKAGE,"preferred_editor",NULL);
			if (value != NULL)
			{
				gtk_combo_box_text_remove(GTK_COMBO_BOX_TEXT(prefs_dialog->preferred_editor), 0);
				gtk_combo_box_text_insert_text(GTK_COMBO_BOX_TEXT(prefs_dialog->preferred_editor), 0, value);
				gtk_combo_box_set_active(GTK_COMBO_BOX(prefs_dialog->preferred_editor), 0);
				g_free(value);
			}
			value = g_key_file_get_string(xa_key_file,PACKAGE,"preferred_viewer",NULL);
			if (value != NULL)
			{
				gtk_combo_box_text_remove(GTK_COMBO_BOX_TEXT(prefs_dialog->preferred_viewer), 0);
				gtk_combo_box_text_insert_text(GTK_COMBO_BOX_TEXT(prefs_dialog->preferred_viewer), 0, value);
				gtk_combo_box_set_active(GTK_COMBO_BOX(prefs_dialog->preferred_viewer), 0);
				g_free(value);
			}

			value = g_key_file_get_string(xa_key_file, PACKAGE, "preferred_archiver", NULL);

			if (!value)
				value = g_find_program_in_path("xarchiver");

			if (value != NULL)
			{
				gtk_combo_box_text_remove(GTK_COMBO_BOX_TEXT(prefs_dialog->preferred_archiver), 0);
				gtk_combo_box_text_insert_text(GTK_COMBO_BOX_TEXT(prefs_dialog->preferred_archiver), 0, value);
				gtk_combo_box_set_active(GTK_COMBO_BOX(prefs_dialog->preferred_archiver), 0);
				g_free(value);
			}
		}

		value = g_key_file_get_string(xa_key_file, PACKAGE, "preferred_custom_cmd", NULL);

		if (value != NULL)
		{
			gtk_combo_box_text_remove(GTK_COMBO_BOX_TEXT(prefs_dialog->preferred_custom_cmd), 0);
			gtk_combo_box_text_insert_text(GTK_COMBO_BOX_TEXT(prefs_dialog->preferred_custom_cmd), 0, value);
			gtk_combo_box_set_active(GTK_COMBO_BOX(prefs_dialog->preferred_custom_cmd), 0);
			g_free(value);
		}

		value = g_key_file_get_string(xa_key_file,PACKAGE,"preferred_temp_dir",NULL);
		if (value != NULL)
		{
			gtk_combo_box_text_remove(GTK_COMBO_BOX_TEXT(prefs_dialog->preferred_temp_dir), 0);
			gtk_combo_box_text_insert_text(GTK_COMBO_BOX_TEXT(prefs_dialog->preferred_temp_dir), 0, value);
			gtk_combo_box_set_active(GTK_COMBO_BOX(prefs_dialog->preferred_temp_dir), 0);
			g_free(value);
		}
		value = g_key_file_get_string(xa_key_file,PACKAGE,"preferred_extract_dir",NULL);
		if (value != NULL)
		{
			gtk_combo_box_text_remove(GTK_COMBO_BOX_TEXT(prefs_dialog->preferred_extract_dir), 0);
			gtk_combo_box_text_insert_text(GTK_COMBO_BOX_TEXT(prefs_dialog->preferred_extract_dir), 0, value);
			gtk_combo_box_set_active(GTK_COMBO_BOX(prefs_dialog->preferred_extract_dir), 0);
			g_free(value);
		}
		main_win_geometry = g_key_file_get_integer_list(xa_key_file, PACKAGE, "mainwindow", &length, &error);
		if (error)
		{
			prefs_dialog->main_win_geometry[0] = -1;
			g_error_free(error);
			error = NULL;
		}
		else
		{
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(prefs_dialog->save_geometry), TRUE);
			prefs_dialog->main_win_geometry[0] = main_win_geometry[0];
			prefs_dialog->main_win_geometry[1] = main_win_geometry[1];
			prefs_dialog->main_win_geometry[2] = main_win_geometry[2];
			prefs_dialog->main_win_geometry[3] = main_win_geometry[3];
			prefs_dialog->main_win_geometry[4] = main_win_geometry[4];
		}
		g_free(main_win_geometry);
		extract_win_size = g_key_file_get_integer_list(xa_key_file, PACKAGE, "extract", &length, &error);
		if (error)
		{
			prefs_dialog->extract_win_size[0] = -1;
			g_error_free(error);
			error = NULL;
		}
		else
		{
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(prefs_dialog->save_geometry), TRUE);
			prefs_dialog->extract_win_size[0] = extract_win_size[0];
			prefs_dialog->extract_win_size[1] = extract_win_size[1];
		}
		g_free(extract_win_size);
		add_win_size = g_key_file_get_integer_list(xa_key_file, PACKAGE, "add", &length, &error);
		if (error)
		{
			prefs_dialog->add_win_size[0] = -1;
			g_error_free(error);
			error = NULL;
		}
		else
		{
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(prefs_dialog->save_geometry), TRUE);
			prefs_dialog->add_win_size[0] = add_win_size[0];
			prefs_dialog->add_win_size[1] = add_win_size[1];
		}
		g_free(add_win_size);
		/* Load the options in the extract dialog */
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(extract_window->ensure_directory), g_key_file_get_boolean(xa_key_file, PACKAGE, "ensure_directory", NULL));
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(extract_window->overwrite), g_key_file_get_boolean(xa_key_file, PACKAGE, "overwrite", NULL));

		val = g_key_file_get_integer(xa_key_file, PACKAGE, "full_path", &error);

		if (error)
		{
			/* set default setting with existing, old config file */
			val = 2;
			g_error_free(error);
			error = NULL;
		}

		if (val == 2)
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(extract_window->full_path), TRUE);
		else if (val == 1)
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(extract_window->relative_path), TRUE);
		else
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(extract_window->without_path), TRUE);

		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(extract_window->touch),g_key_file_get_boolean(xa_key_file,PACKAGE,"touch",NULL));
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(extract_window->freshen),g_key_file_get_boolean(xa_key_file,PACKAGE,"fresh",NULL));
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(extract_window->update),g_key_file_get_boolean(xa_key_file,PACKAGE,"update",NULL));
		/* Load the options in the add dialog */
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(add_window->full_path),g_key_file_get_boolean(xa_key_file,PACKAGE,"store_path",NULL));
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(add_window->update),g_key_file_get_boolean(xa_key_file,PACKAGE,"updadd",NULL));
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(add_window->freshen),g_key_file_get_boolean(xa_key_file,PACKAGE,"freshen",NULL));
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(add_window->recurse),g_key_file_get_boolean(xa_key_file,PACKAGE,"recurse",NULL));
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(add_window->solid),g_key_file_get_boolean(xa_key_file,PACKAGE,"solid_archive",NULL));
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(add_window->remove),g_key_file_get_boolean(xa_key_file,PACKAGE,"remove_files",NULL));
	}
	g_key_file_free (xa_key_file);
}

void xa_prefs_adapt_options (PrefsDialog *prefs_dialog)
{
	xa_combo_box_text_append_compressor_types(GTK_COMBO_BOX_TEXT(prefs_dialog->preferred_format));
	gtk_combo_box_set_active(GTK_COMBO_BOX(prefs_dialog->preferred_format), preferred_format);
}

void xa_prefs_apply_options (PrefsDialog *prefs_dialog)
{
	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(prefs_dialog->show_toolbar)))
		gtk_widget_show (toolbar1);
	else
		gtk_widget_hide (toolbar1);

	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(prefs_dialog->show_location_bar)))
		gtk_widget_show_all (toolbar2);
	else
		gtk_widget_hide (toolbar2);

	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(prefs_dialog->show_sidebar)))
		gtk_widget_show(scrolledwindow2);
	else
		gtk_widget_hide(scrolledwindow2);
}
