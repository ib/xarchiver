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

#include "config.h"
#include <errno.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <gdk/gdkkeysyms.h>
#include <glib/gprintf.h>
#include <glib/gstdio.h>
#include <sys/stat.h>
#include "window.h"
#include "add_dialog.h"
#include "exe.h"
#include "extract_dialog.h"
#include "gzip_et_al.h"
#include "interface.h"
#include "main.h"
#include "mime.h"
#include "new_dialog.h"
#include "open-with-dlg.h"
#include "pref_dialog.h"
#include "string_utils.h"
#include "support.h"
#include "tar.h"

#ifdef __APPLE__
#include <libkern/OSByteOrder.h>
#define le32toh(x) OSSwapLittleToHostInt32(x)
#elif defined(__FreeBSD__)
#include <sys/endian.h>
#else
#include <endian.h>
#endif

#ifdef HAVE_SOCKET
#include "socket.h"
#endif

static const gchar * const XDS_FILENAME = "xds.txt";
static const gchar * const XDS_STR_XDND_DIRECT_SAVE0 = "XdndDirectSave0";
static const gchar * const XDS_STR_TEXT_PLAIN = "text/plain";

#define bswap(word) (((word << 8) & 0xff00) | ((word >> 8) & 0x00ff))

gchar *current_open_directory;

static GtkWidget *comment_dialog;

static void xa_remove_columns (XArchive *archive)
{
	GList *columns = gtk_tree_view_get_columns ( GTK_TREE_VIEW (archive->treeview));
	while (columns != NULL)
	{
		gtk_tree_view_remove_column (GTK_TREE_VIEW (archive->treeview),columns->data);
		columns = columns->next;
	}
	g_list_free (columns);
}

static gchar *xa_open_file_dialog ()
{
	static GtkWidget *File_Selector = NULL;
	static GtkFileFilter *open_file_filter;
	GtkFileFilter *filter;
	GSList *sorted;
	gchar *path = NULL;
	int response;

	if (File_Selector == NULL)
	{
		File_Selector = gtk_file_chooser_dialog_new ( _("Open an archive"),
						GTK_WINDOW (xa_main_window),
						GTK_FILE_CHOOSER_ACTION_OPEN,
						GTK_STOCK_CANCEL,
						GTK_RESPONSE_CANCEL,
						"gtk-open",
						GTK_RESPONSE_ACCEPT,
						NULL);

		gtk_dialog_set_default_response (GTK_DIALOG (File_Selector),GTK_RESPONSE_ACCEPT);
		gtk_window_set_destroy_with_parent (GTK_WINDOW (File_Selector) ,TRUE);

		filter = gtk_file_filter_new ();
		gtk_file_filter_set_name ( filter ,_("All files"));
		gtk_file_filter_add_pattern ( filter,"*");
		gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (File_Selector),filter);

		filter = gtk_file_filter_new ();
		gtk_file_filter_set_name ( filter ,_("Only archives"));
		gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (File_Selector),filter);

		sorted = xa_file_filter_add_archiver_pattern_sort(filter);

		while (sorted)
		{
			filter = gtk_file_filter_new();
			gtk_file_filter_set_name(filter, sorted->data);
			gtk_file_filter_add_pattern(filter, sorted->data);
			gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(File_Selector), filter);
			sorted = sorted->next;
		}

		g_slist_free(sorted);

		gtk_window_set_modal (GTK_WINDOW (File_Selector),TRUE);
	}
	if (open_file_filter != NULL)
		gtk_file_chooser_set_filter (GTK_FILE_CHOOSER (File_Selector) ,open_file_filter);

	if (current_open_directory != NULL)
		gtk_file_chooser_set_current_folder ( GTK_FILE_CHOOSER (File_Selector) ,current_open_directory);

	response = gtk_dialog_run (GTK_DIALOG (File_Selector));

	if (current_open_directory != NULL)
		g_free (current_open_directory);

	current_open_directory = gtk_file_chooser_get_current_folder ( GTK_FILE_CHOOSER (File_Selector));
	open_file_filter = gtk_file_chooser_get_filter ( GTK_FILE_CHOOSER (File_Selector));

	if (response == GTK_RESPONSE_ACCEPT)
		path = gtk_file_chooser_get_filename ( GTK_FILE_CHOOSER (File_Selector));
	else if ( (response == GTK_RESPONSE_CANCEL) || (response == GTK_RESPONSE_DELETE_EVENT))
		path = NULL;

	/* Hiding the window instead of destroying it will preserve the pointers to the file chooser stuff */
	gtk_widget_hide (File_Selector);
	return path;
}

static gboolean xa_detect_archive_comment (int type, gchar *filename, XArchive *archive)
{
	FILE *stream;
	char sig = '1';
	guint cmt_len = 0;
	int byte;
	unsigned char eocds[] = { 0x50,0x4b,0x05,0x06 };
	guint64 eocds_position = 0;

	unsigned short int len = 0;
	int eof;
	size_t seqptr = 0;

	stream = fopen (filename,"r");
	if (stream == NULL)
		return FALSE;

	if (type == XARCHIVETYPE_ZIP)
	{
		/* Let's position the file indicator to 64KB before the end of the archive */
		if (fseek(stream, -65535 - 22, SEEK_END) == -1)
			fseek(stream, 0L, SEEK_SET);
		/* Let's reach the end of central directory signature now */
		while( ! feof(stream))
		{
			byte = (eof = fgetc(stream));
			if (eof == EOF)
				break;
			if (byte == eocds[seqptr])
			{
				if (++seqptr == sizeof(eocds))
				{
					eocds_position = ftell(stream) + 16 ;
					seqptr = 0;
				}
				continue;
			}
			else
			{
				if (seqptr)
					seqptr = 0;
			}
		}
		fseek (stream,eocds_position,SEEK_SET);
		if (fread(&len, sizeof(len), 1, stream) != 1 || len == 0)
		{
			fclose(stream);
			return FALSE;
		}
		else
		{
			archive->comment = g_string_new("");
			while (cmt_len != len)
			{
				if (fread(&sig, sizeof(sig), 1, stream) != 1)
				{
					fclose(stream);
					g_string_free(archive->comment, TRUE);
					archive->comment = NULL;
					return FALSE;
				}
				g_string_append_c (archive->comment,sig);
				cmt_len++;
			}
			fclose(stream);
			return TRUE;
		}
	}
	else if (type == XARCHIVETYPE_ARJ)
	{
		/* Let's avoid the archive name */
		fseek ( stream,39 ,SEEK_SET);
		while (sig != 0)
		{
			if (fread(&sig, sizeof(sig), 1, stream) != 1)
			{
				fclose(stream);
				return FALSE;
			}
			cmt_len++;
		}
		fseek ( stream,39 + cmt_len ,SEEK_SET);
		sig = 1;
		/* Let's read the archive comment byte after byte now */
		archive->comment = g_string_new("");
		while (sig != 0)
		{
			if (fread(&sig, sizeof(sig), 1, stream) != 1 || (sig == 0 && archive->comment->len == 0))
			{
				g_string_free (archive->comment,FALSE);
				archive->comment = NULL;
				fclose(stream);
				return FALSE;
			}
			else
				g_string_append_c (archive->comment,sig);
		}
		fclose(stream);
		return TRUE;
	}
	fclose(stream);
	return FALSE;
}

static gchar *xa_set_size_string (guint64 file_size)
{
	gchar *message = NULL;
	gchar *measure;
	double content_size;

	if (file_size > 1024*1024*1024)
	{
		content_size = (double)file_size / (1024*1024*1024);
		measure = "GB";
	}
	else if (file_size > 1024*1024)
	{
		content_size = (double)file_size / (1024*1024);
		measure = "MB";
	}

    else if (file_size > 1024)
	{
		content_size = (double)file_size / 1024;
		measure = "KB";
	}
	else
	{
		measure = "Bytes";
		content_size = file_size;
	}
    message = g_strdup_printf ("%.1f %s",content_size,measure);
	return message;
}

static void xa_print_entry_in_file (XEntry *entry, gint idx, FILE *stream, int bp)
{
	gchar *path, *path_utf8;
	static int x = 1;
	guint i;
	gpointer current_column;
	guint64 file_size = 0;

	if (!entry)
		return;

    if (entry->filename && !entry->is_dir)
    {
			current_column = entry->columns;
		/* Let's retrieve the sizes of the entry from its column */
		if (!g_utf8_validate(entry->filename, -1, NULL))
		{
			gchar *entry_utf8 = g_filename_display_name(entry->filename);
			g_free(entry->filename);
			entry->filename = entry_utf8;
		}
		path = xa_build_full_path_name_from_entry(entry);
		path_utf8 = g_filename_display_name(path);
		g_free(path);
		if (strlen(path_utf8) == 0)
			goto here;
		for (i = 2; i < archive[idx]->columns - 1; i++)
		{
			switch(archive[idx]->column_types[i])
			{
				case G_TYPE_STRING:
					current_column += sizeof(gchar *);
				break;

				case G_TYPE_UINT64:
					file_size = (*((guint64 *)current_column));
					current_column += sizeof(guint64);
				break;
			}
		}
		if (bp)
		{
			g_fprintf(stream,"<tr class=\"row%d\">",x);
			g_fprintf(stream, "<td>%s</td><td>%" G_GUINT64_FORMAT "</td></tr>", path_utf8, file_size);
			if (x == 2)
				x = 1;
			else
				x = 2;
		}
		else
			g_fprintf(stream, "%-90s %" G_GUINT64_FORMAT "\n", path_utf8, file_size);

		g_free(path_utf8);
	}
here:
	xa_print_entry_in_file(entry->child,idx,stream,bp);
	xa_print_entry_in_file(entry->next,idx,stream,bp);
}

static gchar *xa_open_sfx_file_selector ()
{
	gchar *sfx_name = NULL;
	GtkWidget *sfx_file_selector = NULL;
	int response;

	sfx_file_selector = gtk_file_chooser_dialog_new ( _("Save the self-extracting archive as"),
						GTK_WINDOW (xa_main_window),
						GTK_FILE_CHOOSER_ACTION_SAVE,
						GTK_STOCK_CANCEL,
						GTK_RESPONSE_CANCEL,
						"gtk-save",
						GTK_RESPONSE_ACCEPT,
						NULL);

	gtk_dialog_set_default_response (GTK_DIALOG (sfx_file_selector),GTK_RESPONSE_ACCEPT);
	gtk_file_chooser_set_do_overwrite_confirmation(GTK_FILE_CHOOSER (sfx_file_selector),TRUE);
	response = gtk_dialog_run ( GTK_DIALOG(sfx_file_selector));

	if (response == GTK_RESPONSE_ACCEPT)
		sfx_name = gtk_file_chooser_get_filename ( GTK_FILE_CHOOSER (sfx_file_selector));

	gtk_widget_destroy (sfx_file_selector);
	return sfx_name;
}

static void xa_activate_link (GtkAboutDialog *about, const gchar *link, gpointer user_data)
{
	if (!xdg_open)
	{
		gchar *browser_path = NULL;
		browser_path = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(prefs_window->combo_prefered_web_browser));
		if (!browser_path || !*browser_path)
		{
			xa_show_message_dialog (GTK_WINDOW (xa_main_window),GTK_DIALOG_MODAL,GTK_MESSAGE_INFO,GTK_BUTTONS_OK,_("You didn't set which browser to use!"),_("Please go to Preferences->Advanced and set it."));
			g_free (browser_path);
			return;
		}
		xa_launch_external_program(browser_path, link);
		g_free(browser_path);
	}
	else
		xa_launch_external_program(xdg_open, link);
}

static void xa_rename_cell_edited_canceled (GtkCellRenderer *renderer, gpointer user_data)
{
	g_object_set(renderer,"editable",FALSE,NULL);
	gtk_widget_add_accelerator(delete_menu, "activate", accel_group, GDK_KEY_Delete, (GdkModifierType) 0, GTK_ACCEL_VISIBLE);
}

static void xa_rename_cell_edited (GtkCellRendererText *cell, const gchar *path_string, const gchar *new_name, XArchive *archive)
{
	GtkTreeIter iter;
	GtkTreeModel *model;
	XEntry *entry;
	gchar *old_name, *q_old_name, *q_new_name, *command;
	GSList *names = NULL, *file_list;
	gboolean result = FALSE;

	model = gtk_tree_view_get_model(GTK_TREE_VIEW(archive->treeview));
	if (gtk_tree_model_get_iter_from_string(model,&iter,path_string))
	{
		gtk_tree_model_get(model, &iter, archive->columns - 1, &entry, -1);
		if (entry->is_encrypted)
		{
			if (!xa_check_password(archive))
				goto done;
		}
		g_free(archive->extraction_dir);
		xa_create_working_directory(archive);
		archive->extraction_dir = g_strdup(archive->working_dir);
		old_name = xa_build_full_path_name_from_entry(entry);
		q_old_name = g_shell_quote(old_name);
		names = g_slist_append(names,old_name);

		archive->do_overwrite = archive->do_full_path = TRUE;

		archive->status = XARCHIVESTATUS_EXTRACT;
		result = (*archive->archiver->extract) (archive,names);

		if (result == FALSE)
		{
			g_free(q_old_name);
			goto done;
		}
		/* Rename the file in the tmp dir as the new file entered by the user */
		q_new_name = g_shell_quote(new_name);
		if (strcmp(q_new_name, q_old_name) == 0)
		{
			g_free(q_old_name);
			g_free(q_new_name);
			goto done;
		}
		command = g_strconcat("mv -f ", archive->working_dir, "/", q_old_name, " ", archive->working_dir, "/", q_new_name, NULL);
		xa_run_command(archive, command);
		g_free(command);
		g_free(q_old_name);
		g_free(q_new_name);

		/* Delete the selected file from the archive */
		old_name = xa_build_full_path_name_from_entry(entry);
		file_list = g_slist_append(NULL, old_name);

		archive->status = XARCHIVESTATUS_DELETE;
		(*archive->archiver->delete)(archive, file_list);

		/* Add the renamed file to the archive */
		file_list = g_slist_append(NULL, g_strdup(new_name));
		archive->child_dir = g_strdup(archive->working_dir);
		xa_execute_add_commands(archive, file_list, FALSE);
	}
done:
	xa_rename_cell_edited_canceled(GTK_CELL_RENDERER(cell), NULL);
}

static const gchar *xa_get_archive_format (XArchive *archive)
{
	gint pos;

	pos = g_slist_index(archiver[archive->type].tags, GUINT_TO_POINTER(archive->tag));

	if (pos >= 0)
		return g_slist_nth_data(archiver[archive->type].tags, pos + 1);
	else
		return archiver[archive->type].type->data;
}

static gchar *xa_get_statusbar_message (guint64 total_size, gint n_elem, gint dirs, gboolean selection)
{
	gchar *measure = NULL,*info = NULL;
	gchar *text = "";

	measure = xa_set_size_string(total_size);
	if (selection)
		text = _("selected");

	if (dirs)
	{
		if (n_elem)
		{
			gchar *format = g_strconcat(ngettext("%d file", "%d files", n_elem), " ", _("and"), " ", ngettext("%d dir %s (%s)", "%d dirs %s (%s)", dirs), NULL);
			info = g_strdup_printf(format, n_elem, dirs, text, measure);
			g_free(format);
		}
		else
			info = g_strdup_printf(ngettext ("%d dir %s (%s)","%d dirs %s (%s)",dirs),dirs,text,measure);
	}
	else
		info = g_strdup_printf(ngettext ("%d file %s (%s)","%d files %s (%s)",n_elem),n_elem,text,measure);

	g_free(measure);
	return info;
}

static void xa_set_environment (gpointer display)
{
	if (!g_getenv("WAYLAND_DISPLAY"))
		g_setenv("DISPLAY", display, TRUE);
}

static void xa_determine_program_to_run (gchar *file)
{
	gchar *program;

	if (xdg_open)
		program = g_strdup(xdg_open);
	else
	{
		gchar *basename;
		const char *type;

		basename = g_path_get_basename(file);
		type = xa_get_stock_mime_icon(basename);

		if (strcmp(type, "text-html") == 0)
		{
			program = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(prefs_window->combo_prefered_web_browser));
		}
		else if (strcmp(type, "text-x-generic") == 0)
		{
			program = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(prefs_window->combo_prefered_editor));
		}
		else if (strcmp(type, "image-x-generic") == 0)
			program = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(prefs_window->combo_prefered_viewer));
		else if (strcmp(type, "package-x-generic") == 0)
			program = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(prefs_window->combo_prefered_archiver));
		else
		{
			gchar *basename_utf8 = g_filename_display_name(basename);
			xa_create_open_with_dialog(basename_utf8, g_shell_quote(file), 1);
			g_free(basename_utf8);
			g_free(basename);
			return;
		}

		g_free(basename);
	}

	if (program && *program)
	{
		gchar *q_file = g_shell_quote(file);
		xa_launch_external_program(program, q_file);
		g_free(q_file);
	}
	else
		xa_show_message_dialog(GTK_WINDOW(xa_main_window), GTK_DIALOG_MODAL, GTK_MESSAGE_INFO, GTK_BUTTONS_OK, _("You didn't set which program to use for opening this file!"), _("Please go to Preferences->Advanced and set it."));

	g_free(program);
}

static void xa_clear_comment_window (GtkButton *button, gpointer buf)
{
	GtkTextIter start,end;

	gtk_text_buffer_get_iter_at_offset(buf,&start,0);
	gtk_text_buffer_get_end_iter(buf,&end);
	gtk_text_buffer_delete(buf,&start,&end);
}

static void xa_load_comment_window_from_file (GtkButton *button, gpointer buf)
{
	GtkTextMark *textmark;
	GtkTextIter iter;
	GtkWidget *file;
	gchar *path = NULL;
	gchar *utf8_data = NULL;
	gchar *content = NULL;
	GError *error = NULL;
	gboolean response;
	gsize bytes;

	file = gtk_file_chooser_dialog_new (_("Open a text file"),
						GTK_WINDOW (xa_main_window),
						GTK_FILE_CHOOSER_ACTION_OPEN,
						GTK_STOCK_CANCEL,
						GTK_RESPONSE_CANCEL,
						"gtk-open",
						GTK_RESPONSE_ACCEPT,
						NULL);

	response = gtk_dialog_run (GTK_DIALOG(file));
	if (response == GTK_RESPONSE_ACCEPT)
		path = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER(file));
	gtk_widget_destroy (file);
	if (path != NULL)
	{
		response = g_file_get_contents(path, &content, &bytes, &error);

		if (response == FALSE)
		{
			gchar *msg = g_strdup_printf (_("Can't open file \"%s\":"),path);
			g_free(path);
			response = xa_show_message_dialog (GTK_WINDOW (xa_main_window),GTK_DIALOG_MODAL,GTK_MESSAGE_ERROR,GTK_BUTTONS_OK,
			msg,error->message);
			g_free (msg);
			g_error_free(error);
			return;
		}
		g_free(path);

		if (g_utf8_validate(content, -1, NULL))
			utf8_data = content;
		else
		{
			utf8_data = g_locale_to_utf8(content, -1, NULL, &bytes, NULL);
			g_free(content);
		}

		textmark = gtk_text_buffer_get_insert(buf);
		gtk_text_buffer_get_iter_at_mark(buf,&iter,textmark);
		gtk_text_buffer_insert_with_tags_by_name (buf,&iter,utf8_data,bytes,"font",NULL);
		g_free (utf8_data);
	}
}

static void xa_comment_window_insert_in_archive (GtkButton *button, gpointer buf)
{
	GtkTextIter start,end;
	FILE *stream;
	gint idx;
	gchar *command, *content, *tmp = NULL;

	idx = xa_find_archive_index(gtk_notebook_get_current_page(notebook));

	gtk_text_buffer_get_iter_at_offset(buf,&start,0);
	gtk_text_buffer_get_end_iter(buf,&end);
	content = gtk_text_buffer_get_text(buf,&start,&end,FALSE);

	xa_create_working_directory(archive[idx]);
	tmp = g_strconcat(archive[idx]->working_dir, "/xa-tmp.comment", NULL);
	gtk_widget_destroy(comment_dialog);

	if (archive[idx]->comment == NULL)
	{
		archive[idx]->comment = g_string_new("");
		archive[idx]->has_comment = TRUE;
	}
	/* Return if the user hasn't modified the comment */
	if (strcmp(archive[idx]->comment->str,content) == 0)
		return;

	stream = fopen (tmp,"w");

	if (stream == NULL)
	{
		g_free(tmp);
		return;
	}

	fwrite(content, strlen(content), 1, stream);
	fclose (stream);

	switch (archive[idx]->type)
	{
		case XARCHIVETYPE_ARJ:
		command = g_strconcat ("arj c ",archive[idx]->path[1]," -z",tmp,NULL);
		break;

		case XARCHIVETYPE_RAR:
		command = g_strconcat ("rar c ",archive[idx]->path[1]," -z",tmp,NULL);
		break;

		case XARCHIVETYPE_ZIP:
		command = g_strconcat ("sh -c \"exec zip ",archive[idx]->path[1]," -z <",tmp,"\"",NULL);
		break;

		default:
		command = NULL;
		break;
	}
	if (strlen(archive[idx]->comment->str) > 0)
		g_string_erase(archive[idx]->comment,0,strlen(archive[idx]->comment->str));
	if (strlen(content) > 0)
		g_string_append(archive[idx]->comment,content);

	if (command != NULL)
	{
		xa_run_command(archive[idx], command);
		g_free(command);
	}
	g_free(tmp);
}

static XAClipboard *xa_clipboard_data_new ()
{
	XAClipboard *data = NULL;

	data = g_new0(XAClipboard,1);

	return data;
}

static XAClipboard *xa_get_paste_data_from_clipboard_selection (const guchar *data)
{
	gchar **uris;
	gint i;
	XAClipboard *clipboard_data;

	clipboard_data = xa_clipboard_data_new();
	uris = g_strsplit((const gchar *) data,"\r\n",-1);
	clipboard_data->mode = (strcmp(uris[0], "copy") == 0 ? XA_CLIPBOARD_COPY : XA_CLIPBOARD_CUT);
	sscanf(uris[1], "%p", &clipboard_data->target);
	for (i = 2; uris[i]; i++)
		if (uris[i][0] != '\0')
			clipboard_data->files = g_slist_prepend (clipboard_data->files,g_strdup (uris[i]));
	clipboard_data->files = g_slist_reverse (clipboard_data->files);
	g_strfreev(uris);
	return clipboard_data;
}

static void xa_clipboard_get (GtkClipboard *clipboard, GtkSelectionData *selection_data, guint info, XArchive *archive)
{
	GSList *files = archive->clipboard->files;
	GString *params = g_string_new("");
	if (gtk_selection_data_get_target(selection_data) != XA_INFO_LIST)
		return;

	g_string_append(params, archive->clipboard->mode == XA_CLIPBOARD_COPY ? "copy" : "cut");
	g_string_append (params,"\r\n");
	g_string_append_printf (params,"%p",archive);
	g_string_append (params,"\r\n");

	while (files)
	{
		g_string_append(params, files->data);
		g_string_append (params,"\r\n");
		files = files->next;
	}
	gtk_selection_data_set(selection_data, gtk_selection_data_get_target(selection_data), 8, (guchar *) params->str, strlen(params->str));
	g_string_free (params,TRUE);
}

static void xa_clipboard_cut_copy_operation (XArchive *archive, XAClipboardMode mode)
{
	GtkClipboard *clipboard;
	XAClipboard *clipboard_data = NULL;
	GSList *files = NULL;
	GtkTreeSelection *selection;
	GtkTargetEntry targets[] =
	{
		{ "application/xarchiver-info-list",0,1 }
	};

	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(archive->treeview));
	gtk_tree_selection_selected_foreach(selection,(GtkTreeSelectionForeachFunc) xa_concat_selected_filenames,&files);

	clipboard = gtk_clipboard_get (XA_CLIPBOARD);
	clipboard_data = xa_clipboard_data_new();
	if (clipboard_data == NULL)
		return;

	clipboard_data->files = xa_slist_copy(files);
	clipboard_data->mode  = mode;
	gtk_clipboard_set_with_data(clipboard, targets, G_N_ELEMENTS(targets), (GtkClipboardGetFunc) xa_clipboard_get, (GtkClipboardClearFunc) xa_clipboard_clear, archive);
	archive->clipboard = clipboard_data;
	gtk_widget_set_sensitive(paste,TRUE);

	/* Let's extract the selected files to the archive tmp dir */
	if (archive->has_password)
	{
		if (!xa_check_password(archive))
			return;
	}
	g_free(archive->extraction_dir);
	xa_create_working_directory(archive);
	archive->extraction_dir = g_strdup(archive->working_dir);
	archive->do_full_path = TRUE;
	archive->do_overwrite = TRUE;

	archive->status = XARCHIVESTATUS_EXTRACT;
	(*archive->archiver->extract) (archive,files);
}

static void xa_set_notebook_page (GtkNotebook *notebook, XArchive *archive)
{
	gint n;

	for (n = 0; n < gtk_notebook_get_n_pages(notebook); n++)
	{
		if (archive->page == gtk_notebook_get_nth_page(notebook, n))
		{
			gtk_notebook_set_current_page(notebook, n);
			break;
		}
	}
}

static void xa_expand_containing_directory (XArchive *archive)
{
	if (xa_has_containing_directory(archive))
	{
		GtkTreePath *path;
		GtkTreeSelection *selection;

		path = gtk_tree_path_new_first();
		gtk_tree_view_expand_to_path(GTK_TREE_VIEW(archive_dir_treeview), path);
		selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(archive_dir_treeview));
		gtk_tree_selection_select_path(selection, path);
		gtk_tree_selection_unselect_path(selection, path);
		gtk_tree_path_free(path);
	}
}

void xa_child_processed (XAChildProcess process, guint8 exitstatus, XArchive *archive)
{
	static guint8 status[XA_CHILD_PROCS];

	status[process] = exitstatus;

	if (process == XA_CHILD_EXIT)
	{
		archive->child_pid = 0;

		if (xa_main_window)
		{
			gtk_widget_set_sensitive(Stop_button, FALSE);
			xa_set_button_state(1, 1, 1, 1, archive->can_test, 1, archive->can_add, archive->can_extract, archive->sorted, archive->can_sfx, archive->has_comment, archive->output, archive->has_password);
		}
	}

	if (--archive->child_ref == 0)
	{
		if (status[XA_CHILD_EXIT] == archive->exitstatus_ok) status[XA_CHILD_EXIT] = 0;

		if (archive->output)
			archive->output = g_slist_reverse(archive->output);

		if (status[XA_CHILD_EXIT] == 0 && status[XA_CHILD_STDOUT] == 0 && status[XA_CHILD_STDERR] == 0)
		{
			if (xa_main_window)
			{
				if (archive->parse_output)
				{
					xa_block_signal_dir_treeview_selection(TRUE);
					gtk_widget_grab_focus(archive_dir_treeview);
					xa_block_signal_dir_treeview_selection(FALSE);
					xa_set_notebook_page(notebook, archive);
					xa_update_window_with_archive_entries(archive, NULL);
					xa_expand_containing_directory(archive);
					gtk_tree_view_set_model(GTK_TREE_VIEW(archive->treeview), archive->model);
					g_object_unref(archive->model);
				}

				xa_set_statusbar_message_for_displayed_rows(archive);

				if (archive->status == XARCHIVESTATUS_LIST && archive->has_comment && gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(prefs_window->check_show_comment)))
					xa_show_archive_comment(NULL, NULL);

				if (archive->status == XARCHIVESTATUS_TEST)
				{
					if (archive->output)
						xa_show_archive_output(GUINT_TO_POINTER(TRUE), archive);
					else
						xa_show_message_dialog(GTK_WINDOW(xa_main_window), GTK_DIALOG_MODAL, GTK_MESSAGE_INFO, GTK_BUTTONS_OK, _("Test result:"), _("The archive is okay."));
				}
			}

			archive->parse_output = NULL;
			archive->status = XARCHIVESTATUS_IDLE;
		}
		else
		{
			if (xa_main_window && archive->parse_output)
			{
				xa_update_window_with_archive_entries(archive, NULL);
				archive->parse_output = NULL;
			}

			if (xa_main_window && !gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(prefs_window->store_output)))
				xa_show_message_dialog(GTK_WINDOW(xa_main_window), GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK, _("An error occurred!"), _("Please check the 'Store archiver output' option to see it."));
			else
				xa_show_archive_output(NULL, archive);

			if (xa_main_window)
				gtk_label_set_text(GTK_LABEL(total_label), _("An error occurred!"));

			/* in case the user has supplied a wrong password, reset it so they can try again */
			if (archive->password && (archive->status == XARCHIVESTATUS_TEST || archive->status == XARCHIVESTATUS_SFX))
			{
				g_free(archive->password);
				archive->password = NULL;
			}

			archive->status = XARCHIVESTATUS_ERROR;
		}
	}
}

void xa_reload_archive_content (XArchive *this_archive)
{
	XEntry *entry;

	g_slist_free(this_archive->forward);
	this_archive->forward = NULL;

	g_slist_free(this_archive->back);
	this_archive->back = NULL;

	xa_free_entry(this_archive, this_archive->root_entry);

	g_free(this_archive->column_types);
	xa_remove_columns(this_archive);

	entry = g_new0(XEntry,1);
	entry->filename = "";
	this_archive->root_entry = entry;

	gtk_label_set_text(GTK_LABEL(total_label), _("Reloading archive, please wait..."));

	/* this reload will be called internally during adding and deleting */
	this_archive->status = XARCHIVESTATUS_RELOAD;
	(*this_archive->archiver->list)(this_archive);
}

void xa_show_archive_output (GtkMenuItem *menuitem, XArchive *this_archive)
{
	GSList *output = NULL;
	gchar *title = NULL, *cmd;
	GtkWidget *dialog, *show_cmd, *label, *image, *hbox, *vbox, *textview, *scrolledwindow;
	GtkTextBuffer *textbuffer;
	GtkTextIter iter;
	gint response;

	if (this_archive == NULL)
	{
		gint idx = xa_find_archive_index(gtk_notebook_get_current_page(notebook));

		if (idx == -1)
			return;
		this_archive = archive[idx];
	}

	if (xa_main_window)
		title = _("Archiver output");
	else
		title = PACKAGE_NAME;

	if (progress)
		gtk_widget_hide(progress->window);

	dialog = gtk_dialog_new_with_buttons(title, GTK_WINDOW(xa_main_window), GTK_DIALOG_MODAL, NULL, NULL);
	xa_set_xarchiver_icon(GTK_WINDOW(dialog));

	show_cmd = gtk_dialog_add_button(GTK_DIALOG(dialog), _("Show command"), GTK_RESPONSE_APPLY);
	gtk_dialog_add_button(GTK_DIALOG(dialog), GTK_STOCK_OK, GTK_RESPONSE_OK);
	gtk_dialog_set_default_response (GTK_DIALOG (dialog),GTK_RESPONSE_OK);

	gtk_container_set_border_width (GTK_CONTAINER (dialog),6);
	gtk_container_set_border_width(GTK_CONTAINER(gtk_dialog_get_content_area(GTK_DIALOG(dialog))), 6);
	gtk_box_set_spacing(GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(dialog))), 8);
	gtk_widget_set_size_request(dialog, 672, 420);

	scrolledwindow = gtk_scrolled_window_new (NULL,NULL);
	g_object_set (G_OBJECT (scrolledwindow),"hscrollbar-policy",GTK_POLICY_AUTOMATIC,"shadow-type",GTK_SHADOW_IN,"vscrollbar-policy",GTK_POLICY_AUTOMATIC,NULL);
	gtk_widget_set_size_request (scrolledwindow,-1,200);

	textbuffer = gtk_text_buffer_new (NULL);
	gtk_text_buffer_create_tag (textbuffer,"font","family","monospace",NULL);
	gtk_text_buffer_get_iter_at_offset (textbuffer,&iter,0);

	textview = gtk_text_view_new_with_buffer (textbuffer);
	g_object_unref (textbuffer);
	gtk_text_view_set_editable (GTK_TEXT_VIEW (textview),FALSE);
	gtk_text_view_set_cursor_visible (GTK_TEXT_VIEW (textview),FALSE);

	vbox = gtk_vbox_new (FALSE,6);
	gtk_container_set_border_width (GTK_CONTAINER (vbox),5);

	if (!menuitem)
	{
		image = gtk_image_new_from_stock (GTK_STOCK_DIALOG_ERROR,GTK_ICON_SIZE_DIALOG);
		gtk_misc_set_alignment (GTK_MISC (image),0.5,0.0);

		label = gtk_label_new (_("An error occurred while accessing the archive:"));
		hbox = gtk_hbox_new (FALSE,6);
		gtk_box_pack_start (GTK_BOX (hbox),image,FALSE,FALSE,0);
		gtk_box_pack_start (GTK_BOX (hbox),label,TRUE,TRUE,0);
		gtk_box_pack_start (GTK_BOX (vbox),hbox,FALSE,FALSE,0);
	}
	gtk_container_add (GTK_CONTAINER (scrolledwindow),textview);
	gtk_box_pack_start (GTK_BOX (vbox),scrolledwindow,TRUE,TRUE,0);
	gtk_box_pack_start(GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(dialog))), vbox, TRUE, TRUE, 0);

	output = this_archive->output;
	while (output)
	{
		gtk_text_buffer_insert_with_tags_by_name(textbuffer, &iter, output->data, -1, "font", NULL);
		output = output->next;
	}
	gtk_widget_show_all (vbox);

run_dialog:
	response = gtk_dialog_run(GTK_DIALOG(dialog));

	if (response == GTK_RESPONSE_APPLY)
	{
		gtk_text_buffer_get_iter_at_offset(textbuffer, &iter, 0);
		cmd = g_strdup_printf("%s: %s\n\n", _("Command"), this_archive->command);
		gtk_text_buffer_insert_with_tags_by_name(textbuffer, &iter, cmd, -1, "font", NULL);
		g_free(cmd);
		gtk_widget_hide(show_cmd);
		goto run_dialog;
	}

	gtk_widget_destroy (GTK_WIDGET (dialog));
}

void xa_new_archive (GtkMenuItem *menuitem,gpointer user_data)
{
	gint idx;

	idx = xa_get_new_archive_index();
	if (idx == -1)
		return;

	archive[idx] = xa_new_archive_dialog(NULL, archive);

	if (archive[idx] == NULL)
		return;

	xa_add_page(archive[idx]);
	xa_set_button_state(1, 1, 1, 1, archive[idx]->can_test, 1, archive[idx]->can_add, archive[idx]->can_extract, archive[idx]->sorted, archive[idx]->can_sfx, archive[idx]->has_comment, archive[idx]->output, archive[idx]->has_password);
    xa_disable_delete_buttons(FALSE);

	xa_set_window_title(xa_main_window, archive[idx]->path[0]);
	gtk_label_set_text(GTK_LABEL(total_label),"");
}

int xa_show_message_dialog (GtkWindow *window,int mode,int type,int button,const gchar *message1,const gchar *message2)
{
	GtkWidget *dialog;
	int response;

	dialog = gtk_message_dialog_new (window,mode,type,button,"%s",message1);
	xa_set_xarchiver_icon(GTK_WINDOW(dialog));

	if (!window)
		gtk_window_set_title(GTK_WINDOW(dialog), PACKAGE_NAME);

	gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dialog),"%s",message2);
	response = gtk_dialog_run (GTK_DIALOG (dialog));
	gtk_widget_destroy (GTK_WIDGET (dialog));
	return response;
}

void xa_save_archive (GtkMenuItem *menuitem, gpointer user_data)
{
	gint idx;
	GtkWidget *save = NULL;
	gchar *path = NULL, *command, *filename, *filename_utf8;
	int response;

	idx = xa_find_archive_index(gtk_notebook_get_current_page(notebook));

	save = gtk_file_chooser_dialog_new (_("Save the archive as"),
						GTK_WINDOW (xa_main_window),
						GTK_FILE_CHOOSER_ACTION_SAVE,
						GTK_STOCK_CANCEL,
						GTK_RESPONSE_CANCEL,
						"gtk-save",
						GTK_RESPONSE_ACCEPT,
						NULL);
	filename = g_path_get_basename(archive[idx]->path[1]);
	filename_utf8 = g_filename_display_name(filename);
	gtk_file_chooser_set_current_name(GTK_FILE_CHOOSER(save), filename_utf8);
	gtk_file_chooser_set_do_overwrite_confirmation(GTK_FILE_CHOOSER (save),TRUE);
	g_free(filename_utf8);
	g_free(filename);
	response = gtk_dialog_run (GTK_DIALOG(save));
	if (response == GTK_RESPONSE_ACCEPT)
		path = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER(save));
	gtk_widget_destroy (save);
	if (path != NULL)
	{
		command = g_strconcat ("cp ",archive[idx]->path[1]," ",path,NULL);
		g_free(path);
		xa_run_command(archive[idx], command);
		g_free(command);
	}
}

XArchive *xa_open_archive (GtkWidget *widget, gchar *path)
{
	gchar *utf8_path,*msg;
	gint idx;
	gint n;
	ArchiveType xa;

	if (path == NULL)
    {
		path = xa_open_file_dialog ();
		if (path == NULL)
			return NULL;
	}

	/* Let's check if the archive is already opened */
	for (n = 0; n < gtk_notebook_get_n_pages(notebook); n++)
	{
		idx = xa_find_archive_index(n);
		if (idx == -1)
			break;
		if (strcmp(path, archive[idx]->path[0]) == 0)
		{
			g_free (path);
			gtk_notebook_set_current_page(notebook, n);
			return NULL;
		}
	}
	xa = xa_detect_archive_type(path);

	if (xa.type == XARCHIVETYPE_UNKNOWN || xa.type == XARCHIVETYPE_NOT_FOUND)
	{
		utf8_path = g_filename_to_utf8 (path,-1,NULL,NULL,NULL);
		msg = g_strdup_printf (_("Can't open file \"%s\":"),utf8_path);
		if (xa.type == XARCHIVETYPE_UNKNOWN)
			xa_show_message_dialog (GTK_WINDOW (xa_main_window),GTK_DIALOG_MODAL,GTK_MESSAGE_ERROR,GTK_BUTTONS_OK,msg,_("Archive format is not recognized!"));
		else
			xa_show_message_dialog (GTK_WINDOW (xa_main_window),GTK_DIALOG_MODAL,GTK_MESSAGE_ERROR,GTK_BUTTONS_OK,msg,g_strerror(errno));

		g_free (utf8_path);
		g_free (msg);
		g_free (path);
		return NULL;
	}

	if (!archiver[xa.type].list)
	{
		gchar *name, *msg;

		name = g_path_get_basename(path);
		msg = g_strdup_printf(_("The proper archiver for \"%s\" is not installed!"), name);
		xa_show_message_dialog(GTK_WINDOW(xa_main_window), GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK, _("Sorry, this archive format is not supported:"), msg);
		g_free(msg);
		g_free(name);
		g_free(path);
		return NULL;
	}

	idx = xa_get_new_archive_index();
	if (idx == -1)
	{
		g_free (path);
		return NULL;
	}
	archive[idx] = xa_init_archive_structure(xa);
	if (archive[idx] == NULL)
	{
		xa_show_message_dialog (GTK_WINDOW (xa_main_window),GTK_DIALOG_MODAL,GTK_MESSAGE_ERROR,GTK_BUTTONS_OK,_("Can't allocate memory for the archive structure:"),_("Operation aborted!"));
		g_free (path);
		return NULL;
	}
	/* Detect archive comment,rar one is detected in rar.c */
	if (xa.type == XARCHIVETYPE_ZIP)
		archive[idx]->has_comment = xa_detect_archive_comment(XARCHIVETYPE_ZIP, path, archive[idx]);
	else if (xa.type == XARCHIVETYPE_ARJ)
		archive[idx]->has_comment = xa_detect_archive_comment(XARCHIVETYPE_ARJ, path, archive[idx]);

	if (g_path_is_absolute(path) == FALSE)
		archive[idx]->path[0] = g_strconcat(g_get_current_dir(), "/", path, NULL);
	else
		archive[idx]->path[0] = g_strdup(path);

	archive[idx]->path[1] = xa_escape_bad_chars(archive[idx]->path[0], ESCAPES);
	xa_add_page(archive[idx]);

	xa_disable_delete_buttons (FALSE);
	g_free (path);

	xa_set_button_state(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, NULL, 0);
	gtk_label_set_text(GTK_LABEL(total_label),_("Opening archive, please wait..."));

	archive[idx]->status = XARCHIVESTATUS_LIST;
	(*archive[idx]->archiver->list)(archive[idx]);

	return archive[idx];
}

void xa_test_archive (GtkMenuItem *menuitem,gpointer user_data)
{
	gint idx;

	idx = xa_find_archive_index(gtk_notebook_get_current_page(notebook));

	if (archive[idx]->has_password)
	{
		if (!xa_check_password(archive[idx]))
			return;
	}
	gtk_label_set_text(GTK_LABEL(total_label),_("Testing archive, please wait..."));

	archive[idx]->status = XARCHIVESTATUS_TEST;
	(*archive[idx]->archiver->test)(archive[idx]);
}

void xa_list_archive (GtkMenuItem *menuitem,gpointer data)
{
	unsigned short int bp = GPOINTER_TO_UINT(data);
	gint idx;
	FILE *stream;
	GtkWidget *save = NULL;
	gchar *t, *_filename, *filename, *filename_plus, *filename_plus_utf8, *pref_path, *pref_path_local = NULL;
	int response;
	struct stat my_stat;
	guint64 file_size;

	idx = xa_find_archive_index(gtk_notebook_get_current_page(notebook));

	if (bp)
		_filename = _("Print the archive content as HTML");
	else
		_filename = _("Print the archive content as text");

	save = gtk_file_chooser_dialog_new (_filename,
						GTK_WINDOW (xa_main_window),
						GTK_FILE_CHOOSER_ACTION_SAVE,
						GTK_STOCK_CANCEL,
						GTK_RESPONSE_CANCEL,
						"gtk-save",
						GTK_RESPONSE_ACCEPT,
						NULL);

	filename = g_path_get_basename(archive[idx]->path[1]);
	_filename = strstr(filename,".");
	if (_filename)
		_filename = g_strndup(filename,(_filename-filename));
	else
		_filename = filename;

	filename_plus = g_strconcat (_filename,bp ? ".html" : ".txt",NULL);
	filename_plus_utf8 = g_filename_display_name(filename_plus);
	gtk_file_chooser_set_current_name(GTK_FILE_CHOOSER(save), filename_plus_utf8);
	gtk_file_chooser_set_do_overwrite_confirmation(GTK_FILE_CHOOSER (save),TRUE);
	g_free(filename);
	g_free(filename_plus);
	g_free(filename_plus_utf8);
	filename = NULL;

	pref_path = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(prefs_window->combo_prefered_extract_dir));

	if (pref_path && *pref_path)
		pref_path_local = g_filename_from_utf8(pref_path, -1, NULL, NULL, NULL);
	g_free(pref_path);

	if (current_open_directory || pref_path_local)
		gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(save), pref_path_local ? pref_path_local : current_open_directory);
	g_free(pref_path_local);
	response = gtk_dialog_run (GTK_DIALOG(save));

	if (current_open_directory != NULL)
		g_free (current_open_directory);

	current_open_directory = gtk_file_chooser_get_current_folder(GTK_FILE_CHOOSER(save));

	if (response == GTK_RESPONSE_ACCEPT)
		filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER(save));
	gtk_widget_destroy (save);

	if (filename != NULL)
	{
		stream = fopen (filename,"w");
		g_free(filename);

		if (stream == NULL)
			return;

		filename = g_filename_display_name(archive[idx]->path[1]);
		if (bp)
		{
			g_fprintf(stream, "<html><head><meta charset=\"UTF-8\"><meta name=GENERATOR content=\"" PACKAGE_NAME " " VERSION "\"><title>%s</title>\n", filename);
			g_fprintf (stream,"<style>\ntd     { font: normal .7em ; }\nth     { font: bold 0.7em ; color: #FFFFFF; text-align: left; background: #42578A}\n.row1  { background-color: #DDDDDD; }\n.row2  { background-color: #EEEEEE; }\n</style>\n");
			g_fprintf(stream,"</head>");
			g_fprintf (stream,"<body bgcolor=#FFFFFF>\n");
			g_fprintf (stream,"<b><u>");
		}
		g_fprintf (stream,_("Archive content:\n"));

		if (bp)
			g_fprintf(stream, "</u></b><br><br><b>");
		g_fprintf (stream,_("\nName: "));
		if (bp)
			g_fprintf(stream, "</b><a href=\"file://%s\">", filename);
		g_fprintf(stream, "%s\n", filename);
		if (bp)
			g_fprintf(stream,"</a><br><br><b>");
		stat(archive[idx]->path[0], &my_stat);
    	file_size = my_stat.st_size;
    	t = xa_set_size_string(file_size);
		g_fprintf (stream,_("Compressed   size: "));
    	if (bp)
    		g_fprintf (stream,"</b>");
    	g_fprintf (stream,"%s\n",t);
    	g_free(t);
    	if (bp)
			g_fprintf(stream,"<br><br><b>");
    	g_fprintf (stream,_("Uncompressed size: "));
    	t = xa_set_size_string(archive[idx]->files_size);
    	if (bp)
    		g_fprintf (stream,"</b>");
    	g_fprintf (stream,"%s\n",t);
    	g_free(t);
    	if (bp)
			g_fprintf(stream,"<br><br><b>");
    	g_fprintf (stream,_("Number of files: "));
    	if (bp)
			g_fprintf(stream,"</b>");
		g_fprintf(stream, "%u\n", archive[idx]->files);
		if (bp)
			g_fprintf(stream,"<br><br><b>");
		if (archive[idx]->has_comment)
		{
			g_fprintf (stream,_("Comment:\n"));
			if (bp)
				g_fprintf(stream,"</b><pre>");
			g_fprintf (stream,"%s",archive[idx]->comment->str);
			if (bp)
				g_fprintf(stream,"</pre>");
			g_fprintf (stream,"\n");
			if (bp)
				g_fprintf(stream,"<br>");
		}
		if ( ! bp)
		{
			g_fprintf (stream,"-------------------------------------------------------------------------------------------------------------\n");
			g_fprintf(stream, _("Files:%*s%s"), 80, " ", _("|Size\n"));
			g_fprintf (stream,"-------------------------------------------------------------------------------------------------------------\n");
		}
		else
		{
			g_fprintf(stream,"<br><table border=0 cellpadding=6 cellspacing=1><tr>");
			g_fprintf(stream,_("<th>Files:</th>"));
			g_fprintf(stream, _("<th>Size:</th>"));
			g_fprintf(stream, "</tr>");

		}
		xa_print_entry_in_file(archive[idx]->root_entry,idx,stream,bp);
		if (bp)
			g_fprintf (stream,"</table></body></html>");

		g_free(filename);
		fclose (stream);
	}
}

void xa_close_archive (GtkWidget *widget, gpointer page)
{
	gint page_num;
	gint idx;

	page_num = gtk_notebook_page_num(notebook, page);
	idx = xa_find_archive_index(page_num);
	gtk_notebook_remove_page(notebook, page_num);

	page_num = gtk_notebook_get_n_pages(notebook);
	if (page_num == 0)
	{
		gtk_widget_set_sensitive (up_button,FALSE);
		gtk_widget_set_sensitive (home_button,FALSE);
		gtk_widget_set_sensitive (deselect_all,FALSE);
		xa_disable_delete_buttons (FALSE);
		xa_set_button_state(1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, NULL, 0);
		xa_set_window_title (xa_main_window,NULL);
		gtk_tree_store_clear(archive_dir_treestore);
		gtk_entry_set_text(GTK_ENTRY(location_entry),"");
		gtk_label_set_text(GTK_LABEL(total_label),_("Select \"New\" to create or \"Open\" to open an archive"));
		gtk_widget_hide(selected_frame);
	}
	else if (page_num == 1)
		gtk_notebook_set_show_tabs (notebook,FALSE);
	else
		gtk_notebook_set_show_tabs (notebook,TRUE);

	xa_clean_archive_structure (archive[idx]);
	archive[idx] = NULL;
}

void xa_quit_application (GtkWidget *widget, GdkEvent *event, gpointer user_data)
{
	gint n;
	gint idx;

	idx = xa_find_archive_index(gtk_notebook_get_current_page(notebook));

	if (idx != -1 && archive[idx]->child_pid)
		return;

	for (n = 0; n < gtk_notebook_get_n_pages(notebook); n++)
	{
		idx = xa_find_archive_index(n);
		if (archive[idx] != NULL)
		{
			xa_clean_archive_structure (archive[idx]);
			archive[idx] = NULL;
		}
	}

	if (current_open_directory != NULL)
		g_free (current_open_directory);

	xa_prefs_save_options (prefs_window,config_file);
	gtk_widget_destroy(prefs_window->dialog1);
	g_free(prefs_window);

	gtk_widget_destroy (extract_window->dialog1);
	g_free(extract_window);

	gtk_widget_destroy (add_window->dialog1);
	g_free(add_window);

	gtk_widget_destroy (multi_extract_window->multi_extract);
	g_free(multi_extract_window);

	gtk_widget_destroy(xa_popup_menu);
	g_free (config_file);
	xa_free_icon_cache();

#ifdef HAVE_SOCKET
	socket_finalize();
#endif
	gtk_main_quit();
	return;
}

void xa_delete_archive (GtkMenuItem *menuitem,gpointer user_data)
{
	GList  *row_list = NULL;
	XEntry *entry = NULL;
	GtkTreeIter iter;
	GSList *list = NULL;
	gint idx, response;

	idx = xa_find_archive_index(gtk_notebook_get_current_page(notebook));

	GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(archive[idx]->treeview));

	row_list = gtk_tree_selection_get_selected_rows(selection, &archive[idx]->model);
	if (row_list != NULL)
	{
		while (row_list)
		{
			gtk_tree_model_get_iter(archive[idx]->model, &iter, row_list->data);
			gtk_tree_model_get(archive[idx]->model, &iter, archive[idx]->columns - 1, &entry, -1);
			gtk_tree_path_free (row_list->data);

			list = g_slist_prepend(list, xa_build_full_path_name_from_entry(entry));

			if (entry->is_dir)
				xa_fill_list_with_recursed_entries(entry->child, &list);

			row_list = row_list->next;
		}
		g_list_free (row_list);
	}
	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(prefs_window->confirm_deletion)))
	{
		response = xa_show_message_dialog (GTK_WINDOW (xa_main_window),GTK_DIALOG_MODAL,GTK_MESSAGE_QUESTION,GTK_BUTTONS_OK_CANCEL,_("You are about to delete entries from the archive."),_( "Are you sure you want to do this?"));
		if (response == GTK_RESPONSE_CANCEL || response == GTK_RESPONSE_DELETE_EVENT)
			return;
	}

	archive[idx]->status = XARCHIVESTATUS_DELETE;
	(*archive[idx]->archiver->delete)(archive[idx], list);
	xa_reload_archive_content(archive[idx]);
}

void xa_add_files_archive (GtkMenuItem *menuitem, gpointer user_data)
{
	gint idx;

	idx = xa_find_archive_index(gtk_notebook_get_current_page(notebook));

	xa_set_add_dialog_options(add_window,archive[idx]);
	xa_parse_add_dialog_options (archive[idx],add_window);
}

void xa_extract_archive (GtkMenuItem *menuitem,gpointer user_data)
{
	gint idx, selected;

	idx = xa_find_archive_index(gtk_notebook_get_current_page(notebook));

	GtkTreeSelection *selection = gtk_tree_view_get_selection (GTK_TREE_VIEW(archive[idx]->treeview));
	selected = gtk_tree_selection_count_selected_rows (selection);
	xa_set_extract_dialog_options(extract_window,selected,archive[idx]);
    xa_parse_extract_dialog_options(archive[idx],extract_window,selection);
}

void xa_show_prefs_dialog (GtkMenuItem *menuitem,gpointer user_data)
{
	int response;

	if (prefs_window == NULL)
		prefs_window = xa_create_prefs_dialog();

	gtk_widget_show_all (prefs_window->dialog1);
	xa_prefs_iconview_changed(GTK_ICON_VIEW(prefs_window->iconview), prefs_window);
	response = gtk_dialog_run (GTK_DIALOG(prefs_window->dialog1));
	gtk_widget_hide (prefs_window->dialog1);

	if (response == GTK_RESPONSE_OK)
		xa_prefs_apply_options(prefs_window);
}

void xa_convert_sfx (GtkMenuItem *menuitem ,gpointer user_data)
{
	gchar *command;
	gboolean result;
	gint idx;

	idx = xa_find_archive_index(gtk_notebook_get_current_page(notebook));

    archive[idx]->status = XARCHIVESTATUS_SFX;
    switch ( archive[idx]->type)
	{
		case XARCHIVETYPE_RAR:
			command = g_strconcat ("rar s -o+ ",archive[idx]->path[1],NULL);
			xa_run_command(archive[idx], command);
			g_free(command);
		break;

	    case XARCHIVETYPE_ZIP:
		{
			gchar *archive_name = NULL;
			gchar *archive_name_quoted;
			FILE *sfx_archive;
			FILE *archive_not_sfx;
			gchar *content;
            gsize length;
            GError *error = NULL;
			gchar *unzipsfx_path = NULL;
			gchar buffer[1024];

			archive_name = xa_open_sfx_file_selector();

			if (archive_name == NULL)
				return;

			archive_name_quoted = g_shell_quote(archive_name);
			unzipsfx_path = g_find_program_in_path ("unzipsfx");
			if (unzipsfx_path != NULL)
			{
				/* Load the unzipsfx executable in memory,about 50 KB */
				result = g_file_get_contents (unzipsfx_path,&content,&length,&error);
				if ( ! result)
				{
					xa_show_message_dialog (GTK_WINDOW (xa_main_window),GTK_DIALOG_MODAL,GTK_MESSAGE_ERROR,GTK_BUTTONS_OK,_("Can't convert the archive to self-extracting:"),error->message);
					g_error_free (error);
					g_free (unzipsfx_path);
					return;
				}
				g_free (unzipsfx_path);

				/* Write unzipsfx to a new file */
				sfx_archive = fopen ( archive_name ,"w");
				if (sfx_archive == NULL)
				{
					xa_show_message_dialog (GTK_WINDOW (xa_main_window),GTK_DIALOG_MODAL,GTK_MESSAGE_ERROR,GTK_BUTTONS_OK,_("Can't write the unzipsfx module to the archive:"),g_strerror(errno));
					return;
				}
				fwrite(content, length, 1, sfx_archive);
				g_free (content);

				archive_not_sfx = fopen(archive[idx]->path[0], "r");

				if (archive_not_sfx == NULL)
				{
					fclose(sfx_archive);
					return;
				}

				/* Read archive data and write it after the sfx module in the new file */
				while ( ! feof(archive_not_sfx))
				{
					if (fread(&buffer, sizeof(buffer), 1, archive_not_sfx) != 1)
						/* !!! this is as bad as not checking the return value !!! */
						/* !!! xa_convert_sfx() must report back success or failure !!! */
						break;

					fwrite(&buffer, sizeof(buffer), 1, sfx_archive);
				}
				fclose (archive_not_sfx);
				fclose (sfx_archive);

				g_chmod(archive_name, 0755);
				command = g_strconcat ("zip -A ",archive_name_quoted,NULL);
				xa_run_command(archive[idx], command);
				g_free(command);
			}
			g_free (archive_name);
			g_free (archive_name_quoted);
		}
        break;

        case XARCHIVETYPE_7ZIP:
        {
        	gchar *archive_name = NULL;
			gchar *archive_name_quoted;
			FILE *sfx_archive;
			FILE *archive_not_sfx;
			gchar *content;
            gsize length;
            GError *error = NULL;
			gchar *sfx_path = NULL;
			gchar buffer[1024];
			int response;
			GtkWidget *locate_7zcon = NULL;
			GtkFileFilter *sfx_filter;

        	archive_name = xa_open_sfx_file_selector ();

			if (archive_name == NULL)
				return;
			archive_name_quoted = g_shell_quote(archive_name);

			if (g_file_test ( "/usr/lib/p7zip/7zCon.sfx",G_FILE_TEST_EXISTS))
				sfx_path = g_strdup("/usr/lib/p7zip/7zCon.sfx");
			else if (g_file_test ( "/usr/local/lib/p7zip/7zCon.sfx",G_FILE_TEST_EXISTS))
				sfx_path = g_strdup ("/usr/local/lib/p7zip/7zCon.sfx");
			else if (g_file_test ( "/usr/libexec/p7zip/7zCon.sfx",G_FILE_TEST_EXISTS))
				sfx_path = g_strdup ("/usr/libexec/p7zip/7zCon.sfx");
			else
			{
				sfx_filter = gtk_file_filter_new ();
				gtk_file_filter_set_name (sfx_filter,"");
				gtk_file_filter_add_pattern (sfx_filter,"*.sfx");

				locate_7zcon = gtk_file_chooser_dialog_new ( _("Please select the 7zCon.sfx module"),
						GTK_WINDOW (xa_main_window),
						GTK_FILE_CHOOSER_ACTION_OPEN,
						GTK_STOCK_CANCEL,
						GTK_RESPONSE_CANCEL,
						"gtk-open",
						GTK_RESPONSE_ACCEPT,
						NULL);

				gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (locate_7zcon),sfx_filter);
				gtk_dialog_set_default_response (GTK_DIALOG (locate_7zcon),GTK_RESPONSE_ACCEPT);
				response = gtk_dialog_run (GTK_DIALOG(locate_7zcon));
				if (response == GTK_RESPONSE_ACCEPT)
				{
					sfx_path = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(locate_7zcon));
					gtk_widget_destroy (locate_7zcon);
				}
				else
				{
					gtk_widget_destroy (locate_7zcon);
					return;
				}
			}
			if ( sfx_path != NULL)
			{
				/* Load the 7zCon.sfx executable in memory ~ 500 KB; is it too much for 128 MB equipped PCs ? */
				result = g_file_get_contents (sfx_path,&content,&length,&error);
				if ( ! result)
				{
					response = xa_show_message_dialog (GTK_WINDOW (xa_main_window),GTK_DIALOG_MODAL,GTK_MESSAGE_ERROR,GTK_BUTTONS_OK,_("Can't convert the archive to self-extracting:"),error->message);
					g_error_free (error);
					g_free (sfx_path);
					return;
				}
				g_free (sfx_path);

				/* Write 7zCon.sfx to a new file */
				sfx_archive = fopen ( archive_name ,"w");
				if (sfx_archive == NULL)
				{
					response = xa_show_message_dialog (GTK_WINDOW (xa_main_window),GTK_DIALOG_MODAL,GTK_MESSAGE_ERROR,GTK_BUTTONS_OK,_("Can't write the unzipsfx module to the archive:"),g_strerror(errno));
					return;
				}
				fwrite(content, length, 1, sfx_archive);
				g_free (content);

				archive_not_sfx = fopen(archive[idx]->path[0], "r");

				if (archive_not_sfx == NULL)
				{
					fclose(sfx_archive);
					return;
				}

				/* Read archive data and write it after the sfx module in the new file */
				while ( ! feof(archive_not_sfx))
				{
					if (fread(&buffer, sizeof(buffer), 1, archive_not_sfx) != 1)
						/* !!! this is as bad as not checking the return value !!! */
						/* !!! xa_convert_sfx() must report back success or failure !!! */
						break;

					fwrite(&buffer, sizeof(buffer), 1, sfx_archive);
				}
				fclose (archive_not_sfx);
				fclose (sfx_archive);

				command = g_strconcat ("chmod 755 ",archive_name_quoted,NULL);
				xa_run_command(archive[idx], command);
				g_free(command);
			}
			g_free (archive_name);
			g_free (archive_name_quoted);
		}
		break;

		case XARCHIVETYPE_ARJ:
        	command = g_strconcat ("arj y -je1 " ,archive[idx]->path[1],NULL);
        	xa_run_command(archive[idx], command);
        	g_free(command);
		break;

		default:
		command = NULL;
	}
}

void xa_about (GtkMenuItem *menuitem,gpointer user_data)
{
    static GtkWidget *about = NULL;
    const char *authors[] =
    {
        "",
        "This version:",
        MAINTAINER " <ib@wupperonline.de>",
        "",
        "Original developer:",
        "Giuseppe Torelli <colossus73@gmail.com>",
        "",
        "Archive navigation code:",
        "John Berthels",
        "",
        "Code fixing:",
        "Enrico Tröger",
        "",
        "LHA and DEB support:",
        "Łukasz Zemczak <sil2100@vexillium.org>",
        "",
        "LZMA support:",
        "Thomas Dy <dysprosium66@gmail.com>",
        "",
        "LZOP support:",
        "Kevin Day",
        "",
        "RARv5, XZ, TAR.XZ support:",
        "Frederick GUERIN <fguerin01@gmail.com>",
        "",
        "GTK+ 3 port:",
        "Balló György <ballogyor@gmail.com>",
        "",
        NULL
    };
    const char *documenters[] =
    {
        "",
        "Special thanks to Bjoern Martensen for",
        "bugs hunting and " PACKAGE_NAME "'s Tango logo.",
        "",
        "Thanks to:",
        "Benedikt Meurer",
        "Stephan Arts",
        "Bruno Jesus <00cpxxx@gmail.com>",
        "Uracile for the stunning logo",
        "",
        NULL
    };

	if (about == NULL)
	{
		about = gtk_about_dialog_new ();
		GTK_COMPAT_ABOUT_DIALOG_URI(about, xa_activate_link);
		gtk_window_set_position (GTK_WINDOW (about),GTK_WIN_POS_CENTER_ON_PARENT);
		gtk_window_set_transient_for (GTK_WINDOW (about),GTK_WINDOW (xa_main_window));
		gtk_window_set_destroy_with_parent (GTK_WINDOW (about),TRUE);
		g_object_set (about,
			"program-name", PACKAGE_NAME,
			"version",PACKAGE_VERSION,
			"copyright","Copyright \xC2\xA9 " COPYRIGHT_YEAR " " COPYRIGHT_HOLDER "\n"
			            "Copyright \xC2\xA9 " MAINTAINER_YEAR " " MAINTAINER,
			"comments",_("A GTK+ only lightweight archive manager"),
			"authors",authors,
			"documenters",documenters,
			"translator_credits",_("translator-credits"),
			"logo_icon_name","xarchiver",
			"website","https://github.com/ib/xarchiver/wiki",
			"license","Copyright \xC2\xA9 " COPYRIGHT_YEAR " " COPYRIGHT_HOLDER " - Colossus <colossus73@gmail.com>\n"
			          "Copyright \xC2\xA9 " MAINTAINER_YEAR " " MAINTAINER "\n\n"
					"This program is free software; you can redistribute it and/or modify it\n"
					"under the terms of the GNU General Public License as published by the\n"
					"Free Software Foundation; either version 2 of the License, or (at your option)\n"
					"any later version.\n"
					"\n"
					"This program is distributed in the hope that it will be useful, but WITHOUT\n"
					"ANY WARRANTY; without even the implied warranty of MERCHANTABILITY\n"
					"or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public\n"
					"License for more details.\n"
					"\n"
					"You should have received a copy of the GNU General Public License\n"
					"along with this program; if not, write to the Free Software Foundation, Inc.,\n"
					"51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.",
		      NULL);
	}
	gtk_dialog_run ( GTK_DIALOG(about));
	gtk_widget_hide (about);
}

ArchiveType xa_detect_archive_type (const gchar *filename)
{
	FILE *file;
	unsigned char magic[16];
	long bytes;
	uint32_t *uint32_magic = (uint32_t *) magic;
	unsigned short *short_magic = (unsigned short *) magic;
	ArchiveType xa = {XARCHIVETYPE_UNKNOWN, 0};

	file = fopen(filename, "r");

	if (!file)
	{
		xa.type = XARCHIVETYPE_NOT_FOUND;
		return xa;
	}

	memset(magic, 0, sizeof(magic));
	bytes = fread(magic, 1, sizeof(magic), file);

	/* lz4 and zstd skippable frame */
	while (memcmp(magic + 1, "\x2a\x4d\x18", 3) == 0 && (magic[0] & 0xf0) == 0x50 && bytes >= 8)
	{
		uint32_t frame_size = le32toh(uint32_magic[1]);

		fseek(file, -bytes + 8, SEEK_CUR);

		if (frame_size > 0x7fffffff)
		{
			fseek(file, 0x7fffffff, SEEK_CUR);
			frame_size -= 0x7fffffff;
		}

		fseek(file, frame_size, SEEK_CUR);

		memset(magic, 0, sizeof(magic));
		bytes = fread(magic, 1, sizeof(magic), file);
	}

	if (memcmp(magic, "7z" "\xbc\xaf\x27\x1c", 6) == 0)
		xa.type = XARCHIVETYPE_7ZIP;
	else if (memcmp(magic, "!<arch>\n", 8) == 0)
	{
		xa.type = XARCHIVETYPE_AR;

		if (memcmp(magic + 8, "debian", 6) == 0)
			xa.tag = 'd';
	}
	else if (memcmp(magic, "\x60\xea", 2) == 0)
		xa.type = XARCHIVETYPE_ARJ;
	else if (memcmp(magic, "BZh", 3) == 0)
		xa.type = XARCHIVETYPE_BZIP2;
	else if (memcmp(magic, "\x1f\x9d", 2) == 0)
		xa.type = XARCHIVETYPE_COMPRESS;
	else if (memcmp(magic, "070701", 6) == 0 ||
	         memcmp(magic, "070702", 6) == 0 ||
	         memcmp(magic, "070707", 6) == 0 ||
	         *short_magic == 070707 ||
	         *short_magic == bswap(070707))
	{
		xa.type = XARCHIVETYPE_CPIO;

		if (*short_magic == bswap(070707))
			/* different endianness */
			xa.tag = 'E';
	}
	else if (memcmp(magic, "\x1f\x8b", 2) == 0 ||
	         memcmp(magic, "\x1f\x9e", 2) == 0)
		xa.type = XARCHIVETYPE_GZIP;
	else if ((memcmp(magic + 2, "-lh", 3) == 0 && ((magic[5] >= '0' && magic[5] <= '7') || magic[5] == 'd') && magic[6] == '-') ||
	         (memcmp(magic + 2, "-lz", 3) == 0 && (magic[5] == '4' || magic[5] == '5' || magic[5] == 's') && magic[6] == '-'))
		xa.type = XARCHIVETYPE_LHA;
	else if (memcmp(magic, "LRZI", 4) == 0)
		xa.type = XARCHIVETYPE_LRZIP;
	else if (memcmp(magic, LZ4_MAGIC, 4) == 0 ||
	         memcmp(magic, "\x02\x21\x4c\x18", 4) == 0 ||
	         memcmp(magic, "mozLz40\x00", 8) == 0 ||
	         memcmp(magic, "mozJSSCLz40v001\x00", 16) == 0)
	{
		xa.type = XARCHIVETYPE_LZ4;

		if (magic[3] == 'L')
			xa.tag = 'm';
		else if (magic[3] == 'J')
			xa.tag = 'm' + 0x100;
	}
	else if (memcmp(magic, "LZIP", 4) == 0)
		xa.type = XARCHIVETYPE_LZIP;
	else if (memcmp(magic, "\x5d\x00\x00\x80", 4) == 0)
		xa.type = XARCHIVETYPE_LZMA;
	else if (memcmp(magic, "\211LZO", 4) == 0)
		xa.type = XARCHIVETYPE_LZOP;
	else if (memcmp(magic, "Rar!" "\x1a\x07\x00", 7) == 0 ||
	         memcmp(magic, "Rar!" "\x1a\x07\x01", 7) == 0)
	{
		xa.type = XARCHIVETYPE_RAR;

		if (magic[6] == 1)
			xa.tag = 5;

		/* a rar5 archive without a rar v5 compatible executable can't be opened */
		if ((xa.tag == 5) && !g_slist_find(archiver[xa.type].tags, GUINT_TO_POINTER(xa.tag)))
			archiver[xa.type].list = NULL;
	}
	else if (memcmp(magic, "\xed\xab\xee\xdb", 4) == 0)
		xa.type = XARCHIVETYPE_RPM;
	else if (memcmp(magic, "\xfd" "7zXZ" "\x00", 6) == 0)
		xa.type = XARCHIVETYPE_XZ;
	else if (memcmp(magic, "PK" "\x03\x04", 4) == 0 ||
	         memcmp(magic, "PK" "\x05\x06", 4) == 0 ||
	         memcmp(magic + 4, "PK" "\x01\x02", 4) == 0)
	{
		xa.type = XARCHIVETYPE_ZIP;

		if (g_str_has_suffix(filename, ".apk"))
			xa.tag = 'a';
		else if (g_str_has_suffix(filename, ".cbz"))
			xa.tag = 'c';
		else if (g_str_has_suffix(filename, ".epub"))
			xa.tag = 'e';
		else if (g_str_has_suffix(filename, ".jar"))
			xa.tag = 'j';
		else if (g_str_has_suffix(filename, ".oxt"))
			xa.tag = 'o';
		else if (g_str_has_suffix(filename, ".xpi"))
			xa.tag = 'x';
	}
	else if (g_str_has_suffix(filename, ".exe") && (memcmp(magic, "MZ", 2) == 0))
		xa = exetype(file);
	else if (memcmp(magic + 1, "\xb5\x2f\xfd", 3) == 0 &&
	         (*magic == '\x1e' || (*magic >= '\x22' && *magic <= '\x28')))
		xa.type = XARCHIVETYPE_ZSTD;
	/* partly heuristic methods must come last */
	else if (isTar(file))
		xa.type = XARCHIVETYPE_TAR;

	fclose(file);

	return xa;
}

void xa_create_liststore (XArchive *archive, const gchar *titles[])
{
	guint i;
	GtkCellRenderer *renderer;
	GtkTreeViewColumn *column;

	/* check for batch mode */
	if (!xa_main_window)
		return;

	archive->liststore = gtk_list_store_newv(archive->columns, archive->column_types);
	gtk_tree_view_set_model ( GTK_TREE_VIEW (archive->treeview),GTK_TREE_MODEL (archive->liststore));

	archive->model = GTK_TREE_MODEL(archive->liststore);

	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(prefs_window->check_sort_filename_column)))
	{
		gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(archive->model), 1, GTK_SORT_ASCENDING);
		archive->sorted = TRUE;
	}

	gtk_tree_sortable_set_sort_func(GTK_TREE_SORTABLE(archive->liststore), 1, (GtkTreeIterCompareFunc) xa_sort_dirs_before_files, archive, NULL);

	g_object_ref(archive->model);
	gtk_tree_view_set_model(GTK_TREE_VIEW(archive->treeview),NULL);

	/* icon and filename */
	column = gtk_tree_view_column_new();
	archive->pixbuf_renderer = gtk_cell_renderer_pixbuf_new();
	gtk_tree_view_column_pack_start(column, archive->pixbuf_renderer, FALSE);
	gtk_tree_view_column_set_attributes(column, archive->pixbuf_renderer, "pixbuf", 0, NULL);

	archive->text_renderer = gtk_cell_renderer_text_new();
	gtk_tree_view_column_pack_start(column, archive->text_renderer, TRUE);
	gtk_tree_view_column_set_attributes(column, archive->text_renderer, "text", 1, NULL);
	gtk_tree_view_column_set_title(column,_("Filename"));
	gtk_tree_view_column_set_resizable (column,TRUE);
	gtk_tree_view_column_set_sort_column_id (column,1);
	gtk_tree_view_append_column (GTK_TREE_VIEW (archive->treeview),column);
	gtk_tree_view_column_set_sizing (column,GTK_TREE_VIEW_COLUMN_AUTOSIZE);
	g_signal_connect(archive->text_renderer, "editing-canceled", G_CALLBACK(xa_rename_cell_edited_canceled), archive);
	g_signal_connect(archive->text_renderer, "edited", G_CALLBACK(xa_rename_cell_edited), archive);

	/* archive's individual items */
	for (i = 0; i < archive->columns - 3; i++)
	{
		renderer = gtk_cell_renderer_text_new();
		column = gtk_tree_view_column_new_with_attributes(titles[i], renderer, "text", i + 2, NULL);
		gtk_tree_view_column_set_resizable(column, TRUE);
		gtk_tree_view_column_set_sort_column_id(column, i + 2);
		gtk_tree_view_append_column(GTK_TREE_VIEW(archive->treeview), column);
	}

	/* internally used pointer to XEntry (invisible) */
	column = gtk_tree_view_column_new();
	gtk_tree_view_column_set_visible(column, FALSE);
	gtk_tree_view_append_column(GTK_TREE_VIEW(archive->treeview), column);
}

gboolean treeview_select_search (GtkTreeModel *model, gint column, const gchar *key, GtkTreeIter *iter, gpointer user_data)
{
    char *filename;
    gboolean result;

    gtk_tree_model_get(model,iter,1,&filename,-1);
    if (strcasestr (filename,key))
    	result = FALSE;
	else
		result = TRUE;
    g_free (filename);
    return result;
}

void xa_cancel_archive (GtkMenuItem *menuitem, gpointer user_data)
{
	gint idx, response;

	idx = xa_find_archive_index(gtk_notebook_get_current_page(notebook));
	if (gtk_widget_get_visible(multi_extract_window->multi_extract))
	{
		multi_extract_window->stop_pressed = TRUE;
		kill (multi_extract_window->archive->child_pid,SIGINT);
	}
	else
	{
		if (archive[idx]->status == XARCHIVESTATUS_ADD || archive[idx]->status == XARCHIVESTATUS_SFX)
		{
			response = xa_show_message_dialog (GTK_WINDOW(xa_main_window),GTK_DIALOG_MODAL,GTK_MESSAGE_QUESTION,GTK_BUTTONS_OK_CANCEL,_("Doing so will probably corrupt your archive!"),_("Do you really want to cancel?"));
			if (response == GTK_RESPONSE_CANCEL)
				return;
		}
		if (archive[idx]->child_pid)
			kill (archive[idx]->child_pid,SIGINT);

		gtk_label_set_text(GTK_LABEL(total_label),"");
		/* !!! setting total_label isn't enough cleanup to be done !!! */
	}
}

void xa_archive_properties (GtkMenuItem *menuitem,gpointer user_data)
{
	GtkWidget *archive_properties_window;
	struct stat my_stat;
    gchar *utf8_string ,*dummy_string,*t;
    char date[64];
	gint idx;
	guint64 file_size;
	double content_size;

	idx = xa_find_archive_index(gtk_notebook_get_current_page(notebook));
    if (stat(archive[idx]->path[0], &my_stat) == 0)
		file_size = my_stat.st_size;
	else
		file_size = 0;
    archive_properties_window = xa_create_archive_properties_window();
    dummy_string = g_path_get_basename(archive[idx]->path[1]);
    utf8_string = g_filename_display_name(dummy_string);
	gtk_label_set_text(GTK_LABEL(name_data),utf8_string);
	g_free (utf8_string);
    g_free(dummy_string);
    /* Path */
    dummy_string = xa_remove_level_from_path(archive[idx]->path[0]);
    if (strlen(dummy_string) == 0 || strcmp(dummy_string,"..") == 0 || strcmp(dummy_string,".") == 0)
		utf8_string = g_filename_display_name (g_get_current_dir());
    else
		utf8_string = g_filename_display_name (dummy_string);
    g_free ( dummy_string);

    gtk_label_set_text(GTK_LABEL(path_data),utf8_string);
    g_free ( utf8_string);
	/* Type */
	gtk_label_set_text(GTK_LABEL(type_data), xa_get_archive_format(archive[idx]));
	/* archiver */
	t = g_strconcat(archiver[archive[idx]->type].program[0], archiver[archive[idx]->type].program[1] ? "\n" : "", archiver[archive[idx]->type].program[1], NULL);
	utf8_string = g_filename_display_name(t);
	gtk_label_set_text(GTK_LABEL(archiver_data), utf8_string);
	g_free(utf8_string);
	g_free(t);
    /* Modified Date */
    strftime (date,64,"%c",localtime (&my_stat.st_mtime));
    t = g_locale_to_utf8(date, -1, NULL, NULL, NULL);
    gtk_label_set_text(GTK_LABEL(modified_data),t);
    g_free (t);
    /* Archive Size */
	t = xa_set_size_string(file_size);
    gtk_label_set_text(GTK_LABEL(size_data),t);
    g_free (t);
    /* content_size */
    t = xa_set_size_string(archive[idx]->files_size);
    gtk_label_set_text(GTK_LABEL(content_data),t);
    g_free (t);
    /* Has Comment */
    if (archive[idx]->has_comment)
		gtk_label_set_text(GTK_LABEL(comment_data),_("Yes"));
	else
		gtk_label_set_text(GTK_LABEL(comment_data),_("No"));

    /* Compression_ratio */
    if (file_size)
    {
      content_size = (double) archive[idx]->files_size / file_size;
      t = g_strdup_printf("%.2f:1", content_size);
    }
    else
      t = g_strdup("-");
    gtk_label_set_text(GTK_LABEL(compression_data),t);
    g_free (t);
    /* Number of files */
    t = g_strdup_printf("%u", archive[idx]->files);
    gtk_label_set_text(GTK_LABEL(number_of_files_data),t);
    g_free (t);

    if (archive[idx]->has_password)
    	gtk_label_set_text(GTK_LABEL(encrypted_data),_("Yes"));
	else
		gtk_label_set_text(GTK_LABEL(encrypted_data),_("No"));
    gtk_widget_show_all (archive_properties_window);
}

void xa_set_statusbar_message_for_displayed_rows(XArchive *archive)
{
	gchar *info = NULL;
	GtkTreePath *path = NULL;
	GtkTreeIter iter;
	gint n_elem = 0, dirs = 0;
	guint64 total_size = 0;
	guint64 size = 0;
	XEntry *entry = NULL;

	path = gtk_tree_path_new_first();
	if (! GTK_IS_TREE_MODEL(archive->model) || gtk_tree_model_get_iter (archive->model,&iter,path) == FALSE)
	{
		gtk_tree_path_free(path);
		return;
	}

	gtk_tree_path_free(path);
	do
	{
		gtk_tree_model_get(archive->model, &iter, archive->size_column, &size, -1);
		gtk_tree_model_get(archive->model, &iter, archive->columns - 1, &entry, -1);
		if (entry->is_dir)
			dirs++;
		else
			n_elem++;
		total_size += size;
	}
	while (gtk_tree_model_iter_next (archive->model,&iter));
	info = xa_get_statusbar_message(total_size,n_elem,dirs,FALSE);
	gtk_label_set_text (GTK_LABEL(total_label),info);
	g_free(info);
}

void xa_row_selected (GtkTreeSelection *selection,XArchive *archive)
{
	GList *list = NULL;
	gchar *msg = NULL;
	GtkTreeIter iter;
	gint selected, dirs = 0;
	guint64 total_size = 0;
	guint64 size = 0;
	XEntry *entry;

	selected = gtk_tree_selection_count_selected_rows (selection);

	if (selected == 0)
	{
		xa_disable_delete_buttons (FALSE);
		gtk_widget_hide(selected_frame);
		return;
	}

	gtk_widget_show(selected_frame);
	gtk_widget_set_sensitive(deselect_all, TRUE);
	gtk_widget_set_sensitive(delete_menu, archive->can_delete);
	gtk_widget_set_sensitive(rename_menu, selected == 1 ? can_rename(archive) : FALSE);

	selected = 0;
	list = gtk_tree_selection_get_selected_rows(selection,NULL);
	while (list)
	{
		gtk_tree_model_get_iter(archive->model, &iter, list->data);
		gtk_tree_model_get(archive->model, &iter, archive->size_column, &size, -1);
		gtk_tree_model_get(archive->model, &iter, archive->columns - 1, &entry, -1);
		if (entry->is_dir)
			dirs++;
		else
			selected++;
		gtk_tree_path_free (list->data);
		total_size += size;
		list = list->next;
	}
	g_list_free(list);
	msg = xa_get_statusbar_message(total_size,selected,dirs,TRUE);
	gtk_label_set_text (GTK_LABEL(selected_label),msg);
	g_free(msg);
}

void xa_treeview_drag_begin (GtkWidget *widget, GdkDragContext *context, XArchive *archive)
{
	gtk_drag_source_set_icon_name(widget, archive->child_pid ? "process-stop" : "xarchiver");

	gdk_property_change(gdk_drag_context_get_source_window(context),
	                    gdk_atom_intern_static_string(XDS_STR_XDND_DIRECT_SAVE0),
	                    gdk_atom_intern_static_string(XDS_STR_TEXT_PLAIN),
	                    8, GDK_PROP_MODE_REPLACE,
	                    (guchar *) XDS_FILENAME, strlen(XDS_FILENAME));
}

void xa_treeview_drag_data_get (GtkWidget *widget, GdkDragContext *context, GtkSelectionData *data, guint info, guint time, XArchive *archive)
{
	GtkTreeSelection *selection;
	GSList *names = NULL;
	gint length;
	guchar *uri = NULL;
	gchar *destination, *send, *extraction_dir;

	send = "E";

	if (archive->child_pid)
		goto done;



	gdk_property_get(gdk_drag_context_get_source_window(context),
	                 gdk_atom_intern_static_string(XDS_STR_XDND_DIRECT_SAVE0),
	                 gdk_atom_intern_static_string(XDS_STR_TEXT_PLAIN),
	                 0, 4096, FALSE, NULL, NULL, &length, &uri);

	if (!uri)
		goto done;

	uri = g_realloc(uri, length + 1);
	uri[length] = 0;


	destination = g_filename_from_uri((gchar *) uri, NULL, NULL);
	g_free(uri);

	if (!destination)
		goto done;

	extraction_dir = g_path_get_dirname(destination);
	g_free(destination);

	if (access(extraction_dir, R_OK | W_OK | X_OK) == -1)
	{
		gchar *utf8_path;
		gchar  *msg;

		utf8_path = g_filename_to_utf8(extraction_dir, -1, NULL, NULL, NULL);
		msg = g_strdup_printf (_("You don't have the right permissions to extract the files to the directory \"%s\"."),utf8_path);
		xa_show_message_dialog (GTK_WINDOW (xa_main_window),GTK_DIALOG_MODAL,GTK_MESSAGE_ERROR,GTK_BUTTONS_OK,_("Can't perform extraction!"),msg );
		g_free (utf8_path);
		g_free (msg);
	}
	else
	{
		if (archive->has_password)
		{
			if (!xa_check_password(archive))
			{
				g_free(extraction_dir);
				goto done;
			}
		}

		selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(archive->treeview));
		gtk_tree_selection_selected_foreach (selection,(GtkTreeSelectionForeachFunc) xa_concat_selected_filenames,&names);
		archive->do_full_path = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(extract_window->extract_full));
		archive->do_overwrite = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(extract_window->overwrite_check));
		g_free(archive->extraction_dir);
		archive->extraction_dir = xa_escape_bad_chars(extraction_dir, ESCAPES);

		gtk_label_set_text(GTK_LABEL(total_label), _("Extracting files from archive, please wait..."));
		archive->status = XARCHIVESTATUS_EXTRACT;
		(*archive->archiver->extract) (archive,names);

		if (archive->status == XARCHIVESTATUS_IDLE)   // no error occurred
			send = "S";
	}

	g_free(extraction_dir);

done:
	gtk_selection_data_set(data, gtk_selection_data_get_target(data), 8, (guchar *) send, 1);
}

void xa_page_drag_data_received (GtkWidget *widget, GdkDragContext *context, gint x, gint y, GtkSelectionData *data, guint info, guint time, gpointer user_data)
{
	gchar **uris;
	gchar *filename;
	unsigned int n = 0;
	GSList *list = NULL, *flist;
	ArchiveType xa;
	XArchive *opened;
	gint idx;
	gboolean archives = FALSE, files = FALSE, do_full_path;

	uris = gtk_selection_data_get_uris(data);

	if (!uris)
	{
		xa_show_message_dialog(GTK_WINDOW(xa_main_window), GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK, _("Sorry, I could not perform the operation!"), "");

failed:
		g_slist_free_full(list, g_free);
		g_strfreev(uris);
		gtk_drag_finish(context, FALSE, FALSE, time);

		return;
	}

	while (uris[n])
	{
		filename = g_filename_from_uri(uris[n], NULL, NULL);

		if (filename)
		{
			list = g_slist_append(list, filename);
			xa = xa_detect_archive_type(filename);

			if (xa.type != XARCHIVETYPE_UNKNOWN && xa.type != XARCHIVETYPE_NOT_FOUND)
				archives = TRUE;
			else
				files = TRUE;
		}

		n++;
	}

	idx = xa_find_archive_index(gtk_notebook_get_current_page(notebook));

	if (idx != -1 && archive[idx]->child_pid)
	{
		xa_show_message_dialog(GTK_WINDOW(xa_main_window), GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK, _("Sorry, I could not perform the operation!"), "");
		goto failed;
	}

	/* nothing but archives */
	if (archives && !files)
	{
		flist = list;

		while (flist)
		{
			opened = xa_open_archive(NULL, g_strdup(flist->data));

			while (opened && (opened->status == XARCHIVESTATUS_LIST))
				gtk_main_iteration_do(FALSE);

			flist = flist->next;
		}
	}
	/* nothing but files */
	else if (files && !archives)
	{
		/* no archive open */
		if (idx == -1)
		{
			idx = xa_get_new_archive_index();

			if (idx == -1)
				goto failed;

			archive[idx] = xa_new_archive_dialog(NULL, archive);

			if (!archive[idx])
				goto failed;

			if (archive[idx]->can_add)
				xa_add_page(archive[idx]);
			else
			{
				xa_clean_archive_structure(archive[idx]);
				archive[idx] = NULL;
			}
		}

		/* an open archive */

		if (!archive[idx]->can_add)
		{
			xa_show_message_dialog(GTK_WINDOW(xa_main_window), GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK, _("Can't perform this action:"), _("You can't add content to this archive type!"));
			goto failed;
		}

		do_full_path = archive[idx]->do_full_path;
		archive[idx]->do_full_path = FALSE;

		archive[idx]->child_dir = g_path_get_dirname(list->data);

		xa_execute_add_commands(archive[idx], list, gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(prefs_window->allow_sub_dir)));

		archive[idx]->do_full_path = do_full_path;
	}
	else
	{
		xa_show_message_dialog(GTK_WINDOW(xa_main_window), GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK, _("Can't perform this action:"), _("You can drop either archives to open or files to add. It is not possible to do both at the same time."));
		goto failed;
	}

	g_slist_free_full(list, g_free);
	g_strfreev(uris);

	gtk_drag_finish(context, TRUE, FALSE, time);
}

void xa_concat_selected_filenames (GtkTreeModel *model,GtkTreePath *treepath,GtkTreeIter *iter,GSList **data)
{
	XEntry *entry = NULL;
	gchar *filename = NULL;
	gint idx;

	idx = xa_find_archive_index(gtk_notebook_get_current_page(notebook));

	gtk_tree_model_get(model, iter, archive[idx]->columns - 1, &entry, -1);
	if (entry->is_dir)
		xa_fill_list_with_recursed_entries(entry->child,data);
	filename = xa_build_full_path_name_from_entry(entry);
	*data = g_slist_prepend (*data,filename);
}

void xa_select_all(GtkMenuItem *menuitem,gpointer user_data)
{
	gint idx;
	gint page_num;

	page_num = gtk_notebook_get_current_page(notebook);
	idx = xa_find_archive_index(page_num);

	gtk_tree_selection_select_all ( gtk_tree_view_get_selection (GTK_TREE_VIEW (archive[idx]->treeview)));
	gtk_widget_set_sensitive (select_all,FALSE);
	gtk_widget_set_sensitive (deselect_all,TRUE);
}

void xa_deselect_all (GtkMenuItem *menuitem,gpointer user_data)
{
	gint idx;

	idx = xa_find_archive_index(gtk_notebook_get_current_page(notebook));

	gtk_tree_selection_unselect_all (gtk_tree_view_get_selection(GTK_TREE_VIEW(archive[idx]->treeview)));
	gtk_widget_set_sensitive (select_all,TRUE);
	gtk_widget_set_sensitive (deselect_all,FALSE);
}

gboolean xa_launch_external_program (const gchar *program, const gchar *arg)
{
	GtkWidget *message;
	GError *error = NULL;
	gchar *program_local, *arg_local, *command_line;
	gchar **argv;
	GdkDisplay *display;
	gboolean success = TRUE;

	if (g_utf8_validate(program, -1, NULL))
		program_local = g_filename_from_utf8(program, -1, NULL, NULL, NULL);
	else
		program_local = g_strdup(program);

	if (g_utf8_validate(arg, -1, NULL))
		arg_local = g_filename_from_utf8(arg, -1, NULL, NULL, NULL);
	else
		arg_local = g_strdup(arg);

	command_line = g_strconcat(program_local, " ", arg_local, NULL);
	g_shell_parse_argv(command_line,NULL,&argv,NULL);
	g_free(command_line);
	g_free(program_local);
	g_free(arg_local);

	display = gdk_display_get_default();

	if (!GDK_COMPAT_SPAWN(NULL, argv, NULL, G_SPAWN_SEARCH_PATH, xa_set_environment, (gpointer) gdk_display_get_name(display), NULL, &error))
	{
		message = gtk_message_dialog_new (GTK_WINDOW (xa_main_window),
										GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
										GTK_MESSAGE_ERROR,
										GTK_BUTTONS_CLOSE,
										_("Failed to launch the application!"));
		gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (message),"%s.",error->message);
		gtk_dialog_run (GTK_DIALOG (message));
		gtk_widget_destroy (message);
		g_error_free (error);
		success = FALSE;
	}
	g_strfreev(argv);
	return success;
}

void xa_show_help (GtkMenuItem *menuitem,gpointer user_data)
{
	xa_activate_link (NULL,"file://" HTMLDIR "/index.html",NULL);
}

void xa_enter_password (GtkMenuItem *menuitem ,gpointer user_data)
{
	gint idx;

	idx = xa_find_archive_index(gtk_notebook_get_current_page(notebook));

	if (archive[idx] == NULL)
		return;
	else
	{
		g_free(archive[idx]->password);
		archive[idx]->password = NULL;
	}

	xa_check_password(archive[idx]);
}

void xa_show_archive_comment (GtkMenuItem *menuitem,gpointer user_data)
{
	gchar *utf8_line;
	gsize len;
	gint idx;
	GtkWidget *textview;
	GtkWidget *dialog_vbox1;
	GtkWidget *scrolledwindow1;
	GtkWidget *tmp_image,*file,*clear,*close,*cancel,*file_hbox,*file_label;
	GtkWidget *alignment2;
	GtkTextBuffer *textbuffer;
	GtkTextIter iter;

	idx = xa_find_archive_index(gtk_notebook_get_current_page(notebook));

	comment_dialog = gtk_dialog_new_with_buttons (_("Comment"),GTK_WINDOW(xa_main_window),GTK_DIALOG_MODAL,NULL,NULL);
	gtk_window_set_position (GTK_WINDOW (comment_dialog),GTK_WIN_POS_CENTER_ON_PARENT);
	gtk_window_set_type_hint (GTK_WINDOW (comment_dialog),GDK_WINDOW_TYPE_HINT_DIALOG);
	dialog_vbox1 = gtk_dialog_get_content_area(GTK_DIALOG(comment_dialog));
	gtk_widget_set_size_request(comment_dialog, 672, 420);

	scrolledwindow1 = gtk_scrolled_window_new (NULL,NULL);
	gtk_widget_show (scrolledwindow1);
	gtk_box_pack_start (GTK_BOX (dialog_vbox1),scrolledwindow1,TRUE,TRUE,0);
	g_object_set (G_OBJECT (scrolledwindow1),"hscrollbar-policy",GTK_POLICY_AUTOMATIC,"shadow-type",GTK_SHADOW_IN,"vscrollbar-policy",GTK_POLICY_AUTOMATIC,NULL);

	textbuffer = gtk_text_buffer_new (NULL);
	gtk_text_buffer_create_tag (textbuffer,"font","family","monospace",NULL);
	gtk_text_buffer_get_iter_at_offset (textbuffer,&iter,0);

	textview = gtk_text_view_new_with_buffer (textbuffer);
	g_object_unref (textbuffer);
	gtk_container_add (GTK_CONTAINER (scrolledwindow1),textview);

	clear = gtk_button_new_from_stock ("gtk-clear");
	gtk_dialog_add_action_widget (GTK_DIALOG (comment_dialog),clear,0);
	g_signal_connect (G_OBJECT (clear),"clicked",G_CALLBACK (xa_clear_comment_window),textbuffer);

	file = gtk_button_new();
	tmp_image = gtk_image_new_from_stock ("gtk-harddisk",GTK_ICON_SIZE_BUTTON);
	file_hbox = gtk_hbox_new(FALSE,4);
	file_label = gtk_label_new_with_mnemonic(_("From File"));

	alignment2 = gtk_alignment_new (0.5,0.5,0,0);
	gtk_container_add (GTK_CONTAINER (alignment2),file_hbox);
	gtk_box_pack_start(GTK_BOX(file_hbox),tmp_image,FALSE,TRUE,0);
	gtk_box_pack_start(GTK_BOX(file_hbox),file_label,FALSE,TRUE,0);
	gtk_container_add(GTK_CONTAINER(file),alignment2);
	gtk_dialog_add_action_widget (GTK_DIALOG (comment_dialog),file,0);
	g_signal_connect (G_OBJECT (file),"clicked",G_CALLBACK (xa_load_comment_window_from_file),textbuffer);

	cancel = gtk_button_new_from_stock ("gtk-cancel");
	gtk_dialog_add_action_widget (GTK_DIALOG (comment_dialog),cancel,GTK_RESPONSE_CANCEL);
	g_signal_connect_swapped(G_OBJECT(cancel), "clicked", G_CALLBACK(gtk_widget_destroy), comment_dialog);

	close = gtk_button_new_from_stock ("gtk-ok");
	gtk_dialog_add_action_widget (GTK_DIALOG (comment_dialog),close,GTK_RESPONSE_OK);
	g_signal_connect (G_OBJECT (close),"clicked",G_CALLBACK (xa_comment_window_insert_in_archive),textbuffer);

	if (archive[idx]->comment)
	{
		if (g_utf8_validate(archive[idx]->comment->str, -1, NULL))
		{
			utf8_line = g_strdup(archive[idx]->comment->str);
			len = -1;
		}
		else
			utf8_line = g_locale_to_utf8(archive[idx]->comment->str, -1, NULL, &len, NULL);

		gtk_text_buffer_insert_with_tags_by_name (textbuffer,&iter,utf8_line,len,"font",NULL);
		g_free(utf8_line);
	}
	gtk_widget_show_all(comment_dialog);
}

void xa_location_entry_activated (GtkEntry *entry,gpointer user_data)
{
	XEntry *prev_entry = NULL;
	XEntry *new_entry  = NULL;
	gint idx;
	gchar* pathname_local;

	idx = xa_find_archive_index(gtk_notebook_get_current_page(notebook));

	if (idx == -1)
		return;

	if (strlen(gtk_entry_get_text(GTK_ENTRY(location_entry))) == 0)
	{
		xa_update_window_with_archive_entries(archive[idx],new_entry);
		return;
	}

	pathname_local = g_filename_from_utf8(gtk_entry_get_text(GTK_ENTRY(location_entry)), -1, NULL, NULL, NULL);
	new_entry = xa_find_entry_from_dirpath(archive[idx], pathname_local);
	g_free(pathname_local);
	if (new_entry == NULL)
	{
		if (archive[idx]->location_path)
		{
			gchar *entry_utf8 = g_filename_display_name(archive[idx]->location_path);
			gtk_entry_set_text(GTK_ENTRY(location_entry), entry_utf8);
			g_free(entry_utf8);
		}
		return;
	}

	if (archive[idx]->location_path)
		prev_entry = xa_find_entry_from_dirpath(archive[idx], archive[idx]->location_path);

	if (prev_entry != NULL)
		archive[idx]->back = g_slist_prepend(archive[idx]->back,prev_entry);
	else
		archive[idx]->back = g_slist_prepend(archive[idx]->back,NULL);

	xa_dir_sidebar_select_row(new_entry);
	xa_update_window_with_archive_entries(archive[idx],new_entry);
}

gboolean xa_treeview_mouse_button_press (GtkWidget *widget, GdkEventButton *event, XArchive *archive)
{
	XEntry *entry;
	GtkTreePath *path;
	GtkTreeIter iter;
	GtkTreeSelection *selection;
	gint selected;
	GtkClipboard *clipboard;
	GtkSelectionData *clipboard_selection;
	XAClipboard *paste_data;
	gboolean pasteable = FALSE;

	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(archive->treeview));
	selected = gtk_tree_selection_count_selected_rows(selection);
	gtk_tree_view_get_path_at_pos(GTK_TREE_VIEW(archive->treeview), event->x, event->y, &path, NULL, NULL, NULL);

	if (path == NULL)
		return FALSE;

	if (event->type == GDK_BUTTON_PRESS && (event->button == 2 || event->button == 3))
	{
		gtk_tree_model_get_iter(GTK_TREE_MODEL(archive->liststore), &iter, path);
		gtk_tree_model_get(archive->model, &iter, archive->columns - 1, &entry, -1);

		if (!gtk_tree_selection_iter_is_selected(selection, &iter))
		{
			gtk_tree_selection_unselect_all(selection);
			gtk_tree_selection_select_iter(selection, &iter);
			selected = 1;
		}

		if (event->button == 2 && selected <= 1)
			xa_treeview_row_activated(GTK_TREE_VIEW(archive->treeview), path, NULL, archive);
		else if (event->button == 3)
		{
			gtk_widget_set_sensitive(open_popupmenu, !entry->is_dir && archive->can_extract);
			gtk_widget_set_sensitive(view, (selected == 1) && !entry->is_dir && archive->can_extract);
			gtk_widget_set_sensitive(rrename, (selected == 1) && !entry->is_dir && can_rename(archive));

			clipboard = gtk_clipboard_get(XA_CLIPBOARD);
			clipboard_selection = gtk_clipboard_wait_for_contents(clipboard, XA_INFO_LIST);

			if (clipboard_selection != NULL)
			{
				paste_data = xa_get_paste_data_from_clipboard_selection(gtk_selection_data_get_data(clipboard_selection));
				gtk_selection_data_free(clipboard_selection);

				pasteable = (strcmp(archive->path[1], paste_data->target->path[1]) != 0);
			}

			gtk_widget_set_sensitive(eextract, archive->can_extract);
			gtk_widget_set_sensitive(cut, archive->can_extract && archive->can_delete);
			gtk_widget_set_sensitive(copy, archive->can_extract);
			gtk_widget_set_sensitive(paste, pasteable && archive->can_add);
			gtk_widget_set_sensitive(ddelete, archive->can_delete);
			gtk_menu_popup(GTK_MENU(xa_popup_menu), NULL, NULL, NULL, xa_main_window, event->button, event->time);
		}

		return TRUE;
	}

	gtk_tree_path_free(path);

	return FALSE;
}

void xa_clipboard_cut (GtkMenuItem *item, gpointer user_data)
{
	gint idx;

	idx = xa_find_archive_index(gtk_notebook_get_current_page(notebook));

	xa_clipboard_cut_copy_operation(archive[idx],XA_CLIPBOARD_CUT);
}

void xa_clipboard_copy (GtkMenuItem *item, gpointer user_data)
{
	gint idx;

	idx = xa_find_archive_index(gtk_notebook_get_current_page(notebook));
	xa_clipboard_cut_copy_operation(archive[idx],XA_CLIPBOARD_COPY);
}

void xa_clipboard_paste (GtkMenuItem *item, gpointer user_data)
{
	gint idx;
	GtkClipboard *clipboard;
	GtkSelectionData *selection;
	XAClipboard *paste_data;
	GSList *list = NULL;

	idx = xa_find_archive_index(gtk_notebook_get_current_page(notebook));

	clipboard = gtk_clipboard_get(XA_CLIPBOARD);
	selection = gtk_clipboard_wait_for_contents(clipboard,XA_INFO_LIST);
	if (selection == NULL)
		return;
	paste_data = xa_get_paste_data_from_clipboard_selection(gtk_selection_data_get_data(selection));
	gtk_selection_data_free (selection);

	/* Let's add the already extracted files in the tmp dir to the current archive dir */
	list = xa_slist_copy(paste_data->files);
	archive[idx]->do_full_path = FALSE;
	archive[idx]->child_dir = g_strdup(paste_data->target->working_dir);
	xa_execute_add_commands(archive[idx], list, FALSE);
	if (archive[idx]->status == XARCHIVESTATUS_ERROR)
		return;

	if (paste_data->mode == XA_CLIPBOARD_CUT)
	{
		list = xa_slist_copy(paste_data->files);

		paste_data->target->status = XARCHIVESTATUS_DELETE;
		(*paste_data->target->archiver->delete)(paste_data->target, list);
		xa_reload_archive_content(paste_data->target);
	}
}

void xa_clipboard_clear (GtkClipboard *clipboard, XArchive *archive)
{
	if (archive->clipboard != NULL)
	{
		if (archive->clipboard->files != NULL)
		{
			g_slist_free_full(archive->clipboard->files, g_free);
			archive->clipboard->files = NULL;
		}

		g_free(archive->clipboard);
		archive->clipboard = NULL;
	}
}

void xa_rename_archive (GtkMenuItem *item, gpointer user_data)
{
	gint idx;
	GtkTreeSelection *selection;
	GtkTreeViewColumn *column;
	GtkTreeModel *model;
	GList *row_list = NULL;

	idx = xa_find_archive_index(gtk_notebook_get_current_page(notebook));

	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (archive[idx]->treeview));
	model = gtk_tree_view_get_model(GTK_TREE_VIEW (archive[idx]->treeview));
	row_list = gtk_tree_selection_get_selected_rows(selection,&model);

	g_object_set(archive[idx]->text_renderer, "editable", TRUE, NULL);
	gtk_widget_remove_accelerator(delete_menu, accel_group, GDK_KEY_Delete, (GdkModifierType) 0);
	column = gtk_tree_view_get_column(GTK_TREE_VIEW (archive[idx]->treeview),0);
	gtk_tree_view_set_cursor(GTK_TREE_VIEW(archive[idx]->treeview),row_list->data,column,TRUE);
	gtk_tree_path_free (row_list->data);
	g_list_free(row_list);
}

void xa_open_with_from_popupmenu (GtkMenuItem *item, gpointer user_data)
{
	gboolean result		= FALSE;
	gint idx, nr;
	GtkTreeSelection *selection;
	GtkTreeIter iter;
	GList *row_list = NULL;
	GSList *list = NULL;
	GSList *list_of_files = NULL;
	GString *names = g_string_new("");
	gchar *b_filename, *e_filename, *f_filename;
	XEntry *entry;

	idx = xa_find_archive_index(gtk_notebook_get_current_page(notebook));

	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (archive[idx]->treeview));
	row_list = gtk_tree_selection_get_selected_rows(selection,&archive[idx]->model);
	if (row_list == NULL)
		return;

	nr = gtk_tree_selection_count_selected_rows(selection);
	while (row_list)
	{
		gtk_tree_model_get_iter(archive[idx]->model,&iter,row_list->data);
		gtk_tree_model_get(archive[idx]->model, &iter, archive[idx]->columns - 1, &entry, -1);
		gtk_tree_path_free(row_list->data);
		if (entry->is_encrypted)
		{
			if (!xa_check_password(archive[idx]))
				return;
		}
		list = g_slist_append(list, xa_build_full_path_name_from_entry(entry));
		row_list = row_list->next;
	}
	g_list_free (row_list);
	g_free(archive[idx]->extraction_dir);
	xa_create_working_directory(archive[idx]);
	archive[idx]->extraction_dir = g_strdup(archive[idx]->working_dir);

	archive[idx]->do_full_path = FALSE;
	archive[idx]->do_overwrite = TRUE;
	list_of_files = xa_slist_copy(list);

	archive[idx]->status = XARCHIVESTATUS_EXTRACT;
	result = (*archive[idx]->archiver->extract) (archive[idx],list);
	if (result == FALSE)
		return;

	do
	{
		b_filename = g_path_get_basename(list_of_files->data);
		f_filename = g_strconcat(archive[idx]->working_dir, "/", b_filename, NULL);
		e_filename = g_shell_quote(f_filename);
		g_free(f_filename);
		g_free(b_filename);
		g_string_append (names,e_filename);
		g_string_append_c (names,' ');
		list_of_files = list_of_files->next;
	}
	while (list_of_files);
	xa_create_open_with_dialog(entry->filename,names->str,nr);
	g_string_free(names, FALSE);
	g_slist_free_full(list_of_files, g_free);
}

void xa_view_from_popupmenu (GtkMenuItem *item, gpointer user_data)
{
	GtkTreeSelection *selection;
	GtkTreeIter iter;
	GSList *list = NULL;
	GList *row_list = NULL;
	gboolean result		= FALSE;
	gint idx;
	gchar *entry_local, *filename;
	XEntry *entry;

	idx = xa_find_archive_index(gtk_notebook_get_current_page(notebook));

	xa_create_working_directory(archive[idx]);

	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (archive[idx]->treeview));
	row_list = gtk_tree_selection_get_selected_rows(selection,&archive[idx]->model);
	gtk_tree_model_get_iter(archive[idx]->model,&iter,row_list->data);
	gtk_tree_model_get(archive[idx]->model, &iter, archive[idx]->columns - 1, &entry, -1);
	gtk_tree_path_free(row_list->data);
	list = g_slist_append(list, xa_build_full_path_name_from_entry(entry));
	g_list_free(row_list);

	if (entry->is_encrypted)
	{
		if (!xa_check_password(archive[idx]))
			return;
	}
	entry_local = g_filename_from_utf8(entry->filename, -1, NULL, NULL, NULL);
	filename = g_strconcat(archive[idx]->working_dir, "/", entry_local, NULL);
	g_free(entry_local);
	if (g_file_test(filename,G_FILE_TEST_EXISTS))
		goto here;
	g_free(archive[idx]->extraction_dir);
	archive[idx]->extraction_dir = g_strdup(archive[idx]->working_dir);
	archive[idx]->do_full_path = FALSE;
	archive[idx]->do_overwrite = TRUE;

	archive[idx]->status = XARCHIVESTATUS_EXTRACT;
	result = (*archive[idx]->archiver->extract) (archive[idx],list);
	if (result == FALSE)
		return;

here:
	xa_determine_program_to_run(filename);
	g_free(filename);
}

void xa_treeview_row_activated(GtkTreeView *tree_view,GtkTreePath *path,GtkTreeViewColumn *column,XArchive *archive)
{
	XEntry *entry;
	GtkTreeIter iter;
	gchar *item, *entry_local, *file;
	GSList *names = NULL;
	gboolean result = FALSE;

	if (! gtk_tree_model_get_iter (GTK_TREE_MODEL (archive->liststore),&iter,path))
		return;

	gtk_tree_model_get(GTK_TREE_MODEL(archive->liststore), &iter, archive->columns - 1, &entry, -1);
	if (entry->is_dir)
	{
		if (archive->location_path)
			archive->back = g_slist_prepend(archive->back, xa_find_entry_from_dirpath(archive, archive->location_path));
		/* Put NULL so to display the root entry */
		else
			archive->back = g_slist_prepend(archive->back,NULL);
		xa_dir_sidebar_select_row(entry);
	}
	/* The selected entry it's not a dir so extract it to the tmp dir and send it to xa_determine_program_to_run() */
	else
	{
		if (entry->is_encrypted)
		{
	    if (!xa_check_password(archive))
	     return;
		}
	   	g_free(archive->extraction_dir);
	   	xa_create_working_directory(archive);
	   	archive->extraction_dir = g_strdup(archive->working_dir);
	   	item = xa_build_full_path_name_from_entry(entry);
	   	names = g_slist_append(names,item);
	   	archive->do_full_path = FALSE;
	   	archive->do_overwrite = TRUE;
		archive->status = XARCHIVESTATUS_EXTRACT;
		result = (*archive->archiver->extract) (archive,names);

		if (result == FALSE)
			return;
		entry_local = g_filename_from_utf8(entry->filename, -1, NULL, NULL, NULL);
		file = g_strconcat(archive->working_dir, "/", entry_local, NULL);
		g_free(entry_local);
		xa_determine_program_to_run(file);
		g_free(file);
	}
}

void xa_update_window_with_archive_entries (XArchive *archive,XEntry *entry)
{
	gboolean reload;
	GdkPixbuf *pixbuf = NULL;
	GtkTreeIter iter;
	guint i;
	gpointer current_column;
	gchar *location_path = NULL, *filename;
	gint size;
	gchar *entry_utf8;

	if (archive->status == XARCHIVESTATUS_RELOAD && archive->location_path != NULL)
	{
		entry = xa_find_entry_from_dirpath(archive, archive->location_path);

		if (entry) location_path = g_strdup(archive->location_path);
	}
	else
		archive->current_entry = entry;

	g_free(archive->location_path);
	archive->location_path = NULL;

	if (entry == NULL)
	{
		reload = TRUE;
		entry = archive->root_entry->child;
		gtk_entry_set_text(GTK_ENTRY(location_entry),"\0");
		gtk_tree_selection_unselect_all(gtk_tree_view_get_selection(GTK_TREE_VIEW(archive_dir_treeview)));

		gtk_widget_set_sensitive(back_button,FALSE);
		gtk_widget_set_sensitive(up_button,FALSE);
		gtk_widget_set_sensitive(home_button,FALSE);
	}
	else
	{
		reload = (archive->status == XARCHIVESTATUS_RELOAD);
		gtk_widget_set_sensitive(back_button, archive->back != NULL);
		gtk_widget_set_sensitive(up_button,TRUE);
		gtk_widget_set_sensitive(home_button,TRUE);
		archive->location_path = xa_build_full_path_name_from_entry(entry);
		entry_utf8 = g_filename_display_name(archive->location_path);
		gtk_entry_set_text(GTK_ENTRY(location_entry), entry_utf8);
		g_free(entry_utf8);
		entry = entry->child;
	}
	gtk_list_store_clear(archive->liststore);

	while (entry)
	{
		current_column = entry->columns;
		gtk_list_store_prepend(archive->liststore, &iter);
		if(!g_utf8_validate(entry->filename,-1,NULL))
		{
			gchar *entry_utf8 = g_filename_display_name(entry->filename);
			g_free(entry->filename);
			entry->filename = entry_utf8;
		}
		if (entry->is_dir)
			filename = "folder";
		else if (entry->is_encrypted)
			filename = "lock";
		else
			filename = entry->filename;

		switch (gtk_combo_box_get_active(GTK_COMBO_BOX(prefs_window->combo_icon_size)))
		{
			case 0:
				size = 20;
				break;

			case 1:
				size = 22;
				break;

			case 2:
				size = 24;
				break;

			case 3:
				size = 30;
				break;

			default:
				size = 32;
				break;
		}

		pixbuf = xa_get_pixbuf_icon_from_cache(filename,size);
		gtk_list_store_set(archive->liststore, &iter, archive->columns - 1, entry, -1);
		gtk_list_store_set (archive->liststore,&iter,0,pixbuf,1,entry->filename,-1);

		for (i = 2; i < archive->columns - 1; i++)
		{
			switch(archive->column_types[i])
			{
				case G_TYPE_STRING:
					//g_message ("%d - %s",i,(*((gchar **)current_column)));
					gtk_list_store_set(archive->liststore, &iter, i, *(gchar **) current_column, -1);
					current_column += sizeof(gchar *);
				break;

				case G_TYPE_UINT64:
					//g_message ("*%d - %lu",i,(*((guint64 *)current_column)));
					gtk_list_store_set(archive->liststore, &iter, i, *(guint64 *) current_column, -1);
					current_column += sizeof(guint64);
				break;
			}
		}
		entry = entry->next;
	}
	xa_fill_dir_sidebar(archive, reload);

	if (location_path)
	{
		entry = xa_find_entry_from_dirpath(archive, location_path);
		xa_block_signal_dir_treeview_selection(TRUE);
		xa_dir_sidebar_select_row(entry);
		xa_block_signal_dir_treeview_selection(FALSE);
		g_free(location_path);
	}

	xa_set_statusbar_message_for_displayed_rows(archive);
}

void xa_show_multi_extract_dialog (GtkMenuItem *menu_item, gpointer user_data)
{
	xa_multi_extract_dialog(multi_extract_window);
	if (progress)
		gtk_widget_hide(progress->window);
	//xa_close_archive (NULL,data);
}

void xa_unsort (GtkMenuItem *menu_item, gpointer user_data)
{
	gint idx;
	XArchiveStatus status;

	idx = xa_find_archive_index(gtk_notebook_get_current_page(notebook));

	if (idx == -1)
		return;

	gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(archive[idx]->model), GTK_TREE_SORTABLE_UNSORTED_SORT_COLUMN_ID, (GtkSortType) 0);
	gtk_widget_set_sensitive(unsort_menu, FALSE);
	archive[idx]->sorted = FALSE;

	status = archive[idx]->status;
	archive[idx]->status = XARCHIVESTATUS_RELOAD;
	xa_update_window_with_archive_entries(archive[idx], NULL);
	archive[idx]->status = status;
}

GtkWidget *xa_main_window_find_image (gchar *filename, GtkIconSize size)
{
  gchar *name, *ext, *path;
  GdkPixbuf *file_pixbuf = NULL;
  GtkWidget *file_image;

  name = g_strdup(filename);
  ext = g_strrstr(name, ".png");

  if (ext)
  {
    *ext = 0;
    file_pixbuf = gtk_icon_theme_load_icon(icon_theme, name, size, (GtkIconLookupFlags) 0, NULL);
  }

  g_free(name);

  if (file_pixbuf == NULL)
  {
    path = g_strconcat(PIXMAPSDIR, "/", filename, NULL);
    file_pixbuf = gdk_pixbuf_new_from_file(path, NULL);
    g_free(path);
  }

  if (file_pixbuf == NULL)
    file_image = gtk_image_new_from_stock(GTK_STOCK_MISSING_IMAGE, size);
  else
  {
    file_image = gtk_image_new_from_pixbuf(file_pixbuf);
    g_object_unref(file_pixbuf);
  }

  return file_image;
}

void xa_set_xarchiver_icon (GtkWindow *window)
{
	GdkPixbuf *pixbuf;

	pixbuf = gtk_icon_theme_load_icon(icon_theme, "xarchiver", 24, (GtkIconLookupFlags) 0, NULL);
	gtk_window_set_icon(window, pixbuf);
	g_object_unref(pixbuf);
}
