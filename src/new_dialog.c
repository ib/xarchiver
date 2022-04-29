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
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 *  MA 02110-1301 USA.
 */

#include <string.h>
#include <gtk/gtk.h>
#include "new_dialog.h"
#include "interface.h"
#include "main.h"
#include "pref_dialog.h"
#include "string_utils.h"
#include "support.h"
#include "window.h"

static GtkWidget *create;
static gint new_combo_box = -1;
static gchar *ComboArchiveType;
static gchar *current_new_directory;

static gpointer xa_get_suffix (const gchar *type)
{
	int i;

	if (!type) return "";

	for (i = XARCHIVETYPE_FIRST; i < XARCHIVETYPE_TYPES; i++)
	{
		GSList *list = archiver[i].type;
		GSList *glob = archiver[i].glob;

		while (list && glob)
		{
			if (list->data && (strcmp(type, list->data) == 0))
				return glob->data + 1;

			list = list->next;
			glob = glob->next;
		}
	}

	return "";
}

static void xa_change_archive_extension (GtkComboBox *combo_box, GtkWidget *xa_file_chooser)
{
	gpointer suffix;
	gchar *fname, *file = NULL, *file_suffix = NULL, *stem, *newfile, *newfile_utf8;
	size_t longest = 0;
	int i;

	g_free(ComboArchiveType);
	ComboArchiveType = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(GTK_WIDGET(combo_box)));
	suffix = xa_get_suffix(ComboArchiveType);

	gtk_widget_set_sensitive(create, *(char *) suffix != 0);

	fname = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(xa_file_chooser));

	if (fname)
	{
		file = g_path_get_basename(fname);
		g_free(fname);
	}

	if (!file)
		file = g_strdup("");

	for (i = XARCHIVETYPE_FIRST; i < XARCHIVETYPE_TYPES; i++)
	{
		GSList *list = archiver[i].glob;

		while (list)
		{
			if (*(char *) list->data == '*')
			{
				if (g_str_has_suffix(file, list->data + 1))
				{
					size_t l = strlen(list->data + 1);

					if (l > longest)
					{
						longest = l;
						g_free(file_suffix);
						file_suffix = g_strdup(list->data + 1);
					}
				}
			}

			list = list->next;
		}
	}

	if (file_suffix)
		stem = g_strndup(file, strlen(file) - longest);
	else
		stem = g_strdup(file);

	newfile = g_strconcat(stem, suffix, NULL);
	newfile_utf8 = g_filename_display_name(newfile);

	gtk_file_chooser_set_current_name(GTK_FILE_CHOOSER(xa_file_chooser), newfile_utf8);

	g_free(newfile_utf8);
	g_free(newfile);
	g_free(stem);
	g_free(file_suffix);
	g_free(file);
}

XArchive *xa_new_archive_dialog (gchar *path, XArchive *archive_open[])
{
	GTK_COMPAT_TOOLTIPS;
	const int XA_RESPONSE_CREATE = 1;
	XArchive *archive = NULL;
	GtkWidget *xa_file_chooser;
	GtkWidget *hbox = NULL;
	GtkWidget *combo_box = NULL;
	GtkFileFilter *xa_new_archive_dialog_filter;
	GSList *sorted;
	gchar *my_path = NULL;
	gchar *basepath, *filename;
	gchar *current_dir = NULL;
	gint idx, response;
	ArchiveType xa = {XARCHIVETYPE_UNKNOWN, 0};
	gpointer suffix;
	int i;

	xa_file_chooser = gtk_file_chooser_dialog_new ( _("Create a new archive"),
							GTK_WINDOW (xa_main_window),
							GTK_FILE_CHOOSER_ACTION_SAVE,
							GTK_STOCK_CANCEL,
							GTK_RESPONSE_CANCEL,
							NULL);

	xa_set_xarchiver_icon(GTK_WINDOW(xa_file_chooser));

	create = gtk_dialog_add_button(GTK_DIALOG(xa_file_chooser), _("Cr_eate"), XA_RESPONSE_CREATE);

	gtk_dialog_set_default_response(GTK_DIALOG(xa_file_chooser), XA_RESPONSE_CREATE);

	xa_new_archive_dialog_filter = gtk_file_filter_new ();
	gtk_file_filter_set_name ( xa_new_archive_dialog_filter , _("All files") );
	gtk_file_filter_add_pattern ( xa_new_archive_dialog_filter, "*" );
	gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (xa_file_chooser), xa_new_archive_dialog_filter);

	xa_new_archive_dialog_filter = gtk_file_filter_new ();
	gtk_file_filter_set_name ( xa_new_archive_dialog_filter , _("Only archives") );
	gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (xa_file_chooser), xa_new_archive_dialog_filter);

	sorted = xa_file_filter_add_archiver_pattern_sort(xa_new_archive_dialog_filter);

	while (sorted)
	{
		xa_new_archive_dialog_filter = gtk_file_filter_new();
		gtk_file_filter_set_name(xa_new_archive_dialog_filter, sorted->data);
		gtk_file_filter_add_pattern(xa_new_archive_dialog_filter, sorted->data);
		gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(xa_file_chooser), xa_new_archive_dialog_filter);
		sorted = sorted->next;
	}

	g_slist_free(sorted);

	hbox = gtk_hbox_new (FALSE, 6);
	gtk_box_pack_start (GTK_BOX (hbox),gtk_label_new (_("Archive type:")),FALSE,FALSE,0);

	combo_box = gtk_combo_box_text_new();
	gtk_combo_box_set_focus_on_click(GTK_COMBO_BOX(combo_box), FALSE);

	gtk_widget_set_tooltip_text(combo_box, _("Choose the archive type to create"));
	xa_combo_box_text_append_compressor_types(GTK_COMBO_BOX_TEXT(combo_box));

	if (new_combo_box == -1)
		gtk_combo_box_set_active(GTK_COMBO_BOX(combo_box),gtk_combo_box_get_active(GTK_COMBO_BOX(prefs_window->combo_prefered_format)));
	else
		gtk_combo_box_set_active (GTK_COMBO_BOX (combo_box),new_combo_box);

	gtk_box_pack_start(GTK_BOX(hbox), combo_box, FALSE, FALSE, 0);

	gtk_widget_show_all (hbox);
	gtk_file_chooser_set_extra_widget (GTK_FILE_CHOOSER (xa_file_chooser), hbox);
	ComboArchiveType = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(combo_box));
	suffix = xa_get_suffix(ComboArchiveType);
	gtk_widget_set_sensitive(create, *(char *) suffix != 0);
	g_signal_connect(G_OBJECT(combo_box), "changed", G_CALLBACK(xa_change_archive_extension), xa_file_chooser);

	if (path != NULL)
	{
		basepath = g_path_get_basename (path);
		filename = g_strconcat(basepath, suffix, NULL);

		current_dir = g_get_current_dir ();
		gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (xa_file_chooser),current_dir);

		g_free (basepath);
		g_free (current_dir);
	}
	else
	{
		filename = g_strdup(suffix);
		gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(xa_file_chooser), ".");
	}

	gtk_file_chooser_set_current_name(GTK_FILE_CHOOSER(xa_file_chooser), filename);
	g_free(filename);

	gtk_window_set_modal (GTK_WINDOW (xa_file_chooser),TRUE);
	if (current_new_directory != NULL)
		gtk_file_chooser_set_current_folder ( GTK_FILE_CHOOSER (xa_file_chooser),current_new_directory);
	response = gtk_dialog_run (GTK_DIALOG (xa_file_chooser));
	current_new_directory = gtk_file_chooser_get_current_folder (GTK_FILE_CHOOSER (xa_file_chooser));

	if (!ComboArchiveType || (response == GTK_RESPONSE_CANCEL || response == GTK_RESPONSE_DELETE_EVENT))
		gtk_widget_destroy(xa_file_chooser);
	else if (response == XA_RESPONSE_CREATE)
	{
		my_path = gtk_file_chooser_get_filename ( GTK_FILE_CHOOSER (xa_file_chooser) );

		if (xa_main_window)
		{
			gint n;

			for (n = 0; n < gtk_notebook_get_n_pages(notebook); n++)
			{
				idx = xa_find_archive_index(n);
				if (idx == -1)
					break;
				if (strcmp(my_path, archive_open[idx]->path[0]) == 0)
				{
					gchar *msg = g_strdup_printf(_("\"%s\" is already open!"),my_path);
					xa_show_message_dialog(GTK_WINDOW(xa_main_window), GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK, _("Can't create a new archive:"), msg);
					g_free (my_path);
					g_free (msg);
					gtk_widget_destroy (xa_file_chooser);
					return NULL;
				}
			}
		}

		if ( g_file_test ( my_path , G_FILE_TEST_EXISTS ) )
		{
			gchar *utf8_path;
			gchar  *msg;

			utf8_path = g_filename_to_utf8 (my_path, -1, NULL, NULL, NULL);
			msg = g_strdup_printf (_("The archive \"%s\" already exists!"), utf8_path);
			response = xa_show_message_dialog (GTK_WINDOW (xa_main_window),
							GTK_DIALOG_MODAL,
							GTK_MESSAGE_QUESTION,
							GTK_BUTTONS_OK_CANCEL,
							msg,
							_("Do you want to overwrite it?")
							);
			g_free (utf8_path);
			g_free (msg);
			if (response != GTK_RESPONSE_OK)
			{
				g_free (my_path);
				gtk_widget_destroy (xa_file_chooser);
			    return NULL;
			}
			/* The following to avoid to update the archive instead of adding to it since the filename exists */
			unlink (my_path);
		}

		new_combo_box = gtk_combo_box_get_active (GTK_COMBO_BOX (combo_box));

		for (i = XARCHIVETYPE_FIRST; i < XARCHIVETYPE_TYPES; i++)
		{
			if (archiver[i].is_compressor)
			{
				GSList *list = archiver[i].type;

				while (list)
				{
					if (list->data && (strcmp(ComboArchiveType, list->data) == 0))
					{
						gint pos;

						xa.type = i;
						pos = g_slist_index(archiver[xa.type].tags, list->data);

						if (pos > 0)
							xa.tag = (gushort) GPOINTER_TO_UINT(g_slist_nth_data(archiver[xa.type].tags, pos - 1));

						i = XARCHIVETYPE_TYPES;
						break;
					}

					list = list->next;
				}
			}
		}

		gtk_widget_destroy(xa_file_chooser);

		archive = xa_init_archive_structure(xa);

		archive->path[0] = g_strdup(my_path);
		archive->path[1] = xa_escape_bad_chars(archive->path[0], ESCAPES);

		g_free(my_path);

		return archive;
	}

	return NULL;
}
