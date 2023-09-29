/*
 *  Copyright (c) 2008 Giuseppe Torelli <colossus73@gmail.com>
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

#include "config.h"
#include <errno.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "archive.h"
#include "interface.h"
#include "main.h"
#include "pref_dialog.h"
#include "support.h"
#include "window.h"

#ifndef NCARGS
#define NCARGS _POSIX_ARG_MAX
#endif

#define MAX_CMD_LEN (NCARGS * 2 / 3)

#define MAX_XARCHIVES 100

XArchive *archive[MAX_XARCHIVES];

static gboolean xa_process_stdout (GIOChannel *ioc, GIOCondition cond, XArchive *archive)
{
	GIOStatus status;
	gchar *line;

	if (cond & G_IO_IN)
	{
		status = g_io_channel_read_line(ioc, &line, NULL, NULL, NULL);

		if (status == G_IO_STATUS_NORMAL)
		{
			if (xa_main_window)
			{
				if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(prefs_window->store_output)))
					archive->output = g_slist_prepend(archive->output, g_strdup(line));
			}

			if (archive->parse_output)
				(*archive->parse_output)(line, archive);

			g_free(line);

			process_gtk_events();

			return TRUE;
		}
		else if (status == G_IO_STATUS_AGAIN)
			return TRUE;
		else
			cond &= ~G_IO_IN;
	}

	g_io_channel_shutdown(ioc, FALSE, NULL);
	g_io_channel_unref(ioc);

	xa_child_processed(XA_CHILD_STDOUT, cond != G_IO_HUP, archive);

	return FALSE;
}

static gboolean xa_process_stderr (GIOChannel *ioc, GIOCondition cond, XArchive *archive)
{
	GIOStatus status;
	gchar *line;

	if (cond & G_IO_IN)
	{
		status = g_io_channel_read_line(ioc, &line, NULL, NULL, NULL);

		if (status == G_IO_STATUS_NORMAL)
		{
			if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(prefs_window->store_output)))
			{
				if (!g_utf8_validate(line, -1, NULL))
				{
					gchar *utf8 = g_locale_to_utf8(line, -1, NULL, NULL, NULL);

					if (utf8)
					{
						g_free(line);
						line = utf8;
					}
				}

				archive->output = g_slist_prepend(archive->output, g_strdup(line));
			}

			g_free(line);

			return TRUE;
		}
		else if (status == G_IO_STATUS_AGAIN)
			return TRUE;
		else
			cond &= ~G_IO_IN;
	}

	g_io_channel_shutdown(ioc, FALSE, NULL);
	g_io_channel_unref(ioc);

	xa_child_processed(XA_CHILD_STDERR, cond != G_IO_HUP, archive);

	return FALSE;
}

static void xa_process_exit (GPid pid, gint status, XArchive *archive)
{
	if (WIFEXITED(status))
	{
		if ((archive->status == XARCHIVESTATUS_RELOAD) && !g_file_test(archive->path[0], G_FILE_TEST_EXISTS))
			status = 0;
		else
			status = WEXITSTATUS(status);
	}
	else
		status = 1;

	g_spawn_close_pid(pid);
	xa_child_processed(XA_CHILD_EXIT, status, archive);
}

static void xa_delete_working_directory (XArchive *archive)
{
	if (xa_main_window)
	{
		gchar *working_dir = g_shell_quote(archive->working_dir);

		xa_launch_external_program("rm -rf", working_dir);
		g_free(working_dir);
	}
	else
	{
		char *argv[4];
		argv[0] = "rm";
		argv[1] = "-rf";
		argv[2] = archive->working_dir;
		argv[3] = NULL;
		g_spawn_sync (NULL, argv, NULL,G_SPAWN_SEARCH_PATH,NULL, NULL,NULL,NULL, NULL,NULL);
	}
}

static XEntry *xa_alloc_memory_for_each_row (guint columns, GType column_types[])
{
	XEntry *entry = NULL;
	guint i;
	gint size = 0;

	entry = g_new0(XEntry,1);
	if (entry == NULL)
		return NULL;

	for (i = 2; i < columns - 1; i++)
	{
		switch(column_types[i])
		{
			case G_TYPE_STRING:
				size += sizeof(gchar *);
			break;

			case G_TYPE_UINT64:
				size += sizeof(guint64);
			break;
		}
	}
	entry->columns = g_malloc0 (size);
	return entry;
}

static XEntry *xa_find_directory_entry (XEntry *entry, const gchar *name)
{
	gchar *filename;

	if (entry == NULL)
		return NULL;

	if (g_utf8_validate(entry->filename, -1, NULL))
		filename = g_filename_display_name(name);
	else
		filename = g_strdup(name);

	if (entry->is_dir && strcmp(entry->filename, filename) == 0)
	{
		g_free(filename);
		return entry;
	}

	g_free(filename);

  return xa_find_directory_entry(entry->next, name);
}

static gpointer *xa_fill_archive_entry_columns_for_each_row (XArchive *archive, XEntry *entry, gpointer *items)
{
	guint i;
	gpointer current_column;

	current_column = entry->columns;

	for (i = 2; i < archive->columns - 1; i++)
	{
		switch(archive->column_types[i])
		{
			case G_TYPE_STRING:
				*(gchar **) current_column = g_strdup((gchar *) items[i - 2]);
				//g_message ("%d - %s",i,(*((gchar **)current_column)));
				current_column += sizeof(gchar *);
			break;

			case G_TYPE_UINT64:
				*(guint64 *) current_column = g_ascii_strtoull(items[i - 2], NULL, 0);
				//g_message ("*%d - %lu",i,(*((guint64 *)current_column)));
				current_column += sizeof(guint64);
			break;
		}
	}
	return entry->columns;
}

static void xa_build_dir_sidebar (XEntry *entry, GtkTreeStore *model, gchar *path, GtkTreeIter *containing_iter)
{
	GtkTreeIter child_iter;

	if (!entry)
		return;

	if (strlen(entry->filename) == 0)
		return xa_build_dir_sidebar(entry->child, model, path, containing_iter);

	if (entry->is_dir)
	{
		gtk_tree_store_append(model,&child_iter,containing_iter);

		if (!g_utf8_validate(entry->filename, -1, NULL))
		{
			gchar *entry_utf8 = g_filename_display_name(entry->filename);
			g_free(entry->filename);
			entry->filename = entry_utf8;
		}

		gtk_tree_store_set(model,&child_iter,0,"gtk-directory",1,entry->filename,2,entry,-1);
	}
	xa_build_dir_sidebar(entry->child, model, NULL, &child_iter);
	xa_build_dir_sidebar(entry->next, model, NULL, containing_iter);

}

static gboolean xa_dir_sidebar_find_row (GtkTreeModel *model, GtkTreePath *path, GtkTreeIter *iter, gpointer entry)
{
	XEntry *entry2;
	GtkTreeIter parent;
	gboolean value;

	gtk_tree_model_get (model,iter,2,&entry2,-1);
	if (entry == entry2)
	{
		gtk_tree_model_iter_parent(model,&parent,iter);
		if ( ! gtk_tree_view_row_expanded(GTK_TREE_VIEW(archive_dir_treeview),path))
			gtk_tree_view_expand_to_path(GTK_TREE_VIEW(archive_dir_treeview),path);

		gtk_tree_selection_select_iter(gtk_tree_view_get_selection (GTK_TREE_VIEW (archive_dir_treeview)),iter);
		gtk_tree_view_scroll_to_cell (GTK_TREE_VIEW(archive_dir_treeview),path,NULL,FALSE,0,0);
		value = TRUE;
	}
	else
		value = FALSE;

	return value;
}

gboolean xa_get_compressed_tar_type (XArchiveType *type)
{
	switch (*type)
	{
		case XARCHIVETYPE_BZIP:
			*type = XARCHIVETYPE_TAR_BZIP;
			break;

		case XARCHIVETYPE_BZIP2:
			*type = XARCHIVETYPE_TAR_BZIP2;
			break;

		case XARCHIVETYPE_BZIP3:
			*type = XARCHIVETYPE_TAR_BZIP3;
			break;

		case XARCHIVETYPE_COMPRESS:
			*type = XARCHIVETYPE_TAR_COMPRESS;
			break;

		case XARCHIVETYPE_GZIP:
			*type = XARCHIVETYPE_TAR_GZIP;
			break;

		case XARCHIVETYPE_LRZIP:
			*type = XARCHIVETYPE_TAR_LRZIP;
			break;

		case XARCHIVETYPE_LZ4:
			*type = XARCHIVETYPE_TAR_LZ4;
			break;

		case XARCHIVETYPE_LZIP:
			*type = XARCHIVETYPE_TAR_LZIP;
			break;

		case XARCHIVETYPE_LZMA:
			*type = XARCHIVETYPE_TAR_LZMA;
			break;

		case XARCHIVETYPE_LZOP:
			*type = XARCHIVETYPE_TAR_LZOP;
			break;

		case XARCHIVETYPE_RZIP:
			*type = XARCHIVETYPE_TAR_RZIP;
			break;

		case XARCHIVETYPE_XZ:
			*type = XARCHIVETYPE_TAR_XZ;
			break;

		case XARCHIVETYPE_ZSTD:
			*type = XARCHIVETYPE_TAR_ZSTD;
			break;

		default:
			return FALSE;
	}

	return TRUE;
}

XArchive *xa_init_archive_structure (ArchiveType xa)
{
	XArchive *archive;
	XEntry *entry;

	archive = g_new0(XArchive, 1);

	if (!archive)
		return NULL;

	archive->type = xa.type;
	archive->tag = xa.tag;

	entry = g_new0(XEntry, 1);

	if (!entry)
	{
		g_free(archive);
		return NULL;
	}

	entry->filename = "";
	archive->root_entry = entry;

	archive->archiver = &archiver[xa.type];

	if (archive_dir_treestore)
		gtk_tree_store_clear(archive_dir_treestore);

	(*archive->archiver->ask)(archive);

	return archive;
}

void xa_spawn_async_process (XArchive *archive, const gchar *command)
{
	gint argc;
	gchar **argv;
	GError *error = NULL;
	GIOChannel *ioc;

	g_shell_parse_argv(command, &argc, &argv, NULL);

	if (!g_spawn_async_with_pipes(archive->child_dir,
	                              argv,
	                              NULL,
	                              G_SPAWN_LEAVE_DESCRIPTORS_OPEN | G_SPAWN_DO_NOT_REAP_CHILD | G_SPAWN_SEARCH_PATH,
	                              NULL,
	                              NULL,
	                              &archive->child_pid,
	                              NULL,
	                              &archive->child_fdout,
	                              &archive->child_fderr,
	                              &error))
	{
		g_strfreev(argv);

		xa_show_message_dialog(NULL, GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK, _("Can't run the archiver executable:"), error->message);
		g_error_free(error);

		if (xa_main_window)
			xa_set_button_state(1, 1, 1, 1, archive->can_test, 1, archive->can_add, archive->can_extract, archive->sorted, archive->can_sfx, archive->has_comment, archive->output, archive->has_password);

		archive->status = XARCHIVESTATUS_ERROR;
		return;
	}

	g_strfreev(argv);

	g_free(archive->command);
	archive->command = g_strdup(command);

	archive->child_ref = XA_CHILD_PROCS;

	if (xa_main_window)
	{
		gtk_widget_set_sensitive(Stop_button, TRUE);
		xa_show_archive_status(archive);

		if (!archive->timeout)
			archive->timeout = g_timeout_add(350, (GSourceFunc) xa_flash_led_indicator, archive);
	}
	else if (!progress || !progress->multi_extract)
	{
		xa_show_progress_bar(archive);

		if (!archive->timeout)
			archive->timeout = g_timeout_add(100, (GSourceFunc) xa_pulse_progress_bar, archive);
	}

	g_slist_free_full(archive->output, g_free);
	archive->output = NULL;

	ioc = g_io_channel_unix_new(archive->child_fdout);
	g_io_channel_set_encoding(ioc, NULL, NULL);
	g_io_channel_set_flags(ioc, G_IO_FLAG_NONBLOCK, NULL);
	g_io_add_watch(ioc, G_IO_IN | G_IO_ERR | G_IO_HUP | G_IO_NVAL, (GIOFunc) xa_process_stdout, archive);

	ioc = g_io_channel_unix_new(archive->child_fderr);
	g_io_channel_set_encoding(ioc, NULL, NULL);
	g_io_channel_set_flags(ioc, G_IO_FLAG_NONBLOCK, NULL);
	g_io_add_watch(ioc, G_IO_IN | G_IO_ERR | G_IO_HUP | G_IO_NVAL, (GIOFunc) xa_process_stderr, archive);

	if (archive->parse_output)
		g_child_watch_add_full(G_PRIORITY_LOW, archive->child_pid, (GChildWatchFunc) xa_process_exit, archive, NULL);
}

/*	TODO: workaround for bug #3235
 *
 * gchar *xa_split_command_line(XArchive *archive,GSList *list)
{
	gchar *command = NULL;
	GSList *chunks = NULL;
	GSList *scan = NULL;
	int length;

	for (scan = list; scan != NULL;)
	{
		length = 0;
		while ((scan != NULL) && (length < 5000)) //MAX_CMD_LEN
		{
			length += strlen (scan->data);
			chunks = g_slist_prepend(chunks,scan->data);
			scan = scan->next;
		}
		chunks = g_slist_prepend(chunks,"****** ");
	}
	chunks = g_slist_reverse(chunks);
	return command;
}
*/

void xa_clean_archive_structure (XArchive *archive)
{
	xa_free_entry(archive, archive->root_entry);

	if (archive->working_dir)
	{
		xa_delete_working_directory(archive);
		g_free(archive->working_dir);
	}

	if (archive->comment)
		g_string_free(archive->comment, TRUE);

	g_slist_free_full(archive->output, g_free);

	if (archive == XA_Clipboard.archive)
		 xa_clipboard_clear();

	g_free(archive->column_types);
	g_free(archive->path[0]);
	g_free(archive->path[1]);
	g_free(archive->path[2]);
	g_free(archive->path[3]);
	g_free(archive->destination_path);
	g_free(archive->extraction_dir);
	g_free(archive->password);
	g_free(archive->child_dir);
	g_free(archive->command);
	g_free(archive);
}

gboolean xa_create_working_directory (XArchive *archive)
{
	gchar *tmp_dir;
	gchar *value, *value_local = NULL;
	const gchar *tmp;

	if (archive->working_dir)
		return TRUE;

	value = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(prefs_window->combo_prefered_temp_dir));

	if (value && *value)
	{
		value_local = g_filename_from_utf8(value, -1, NULL, NULL, NULL);
		tmp = value_local;
	}
	else
		tmp = g_get_tmp_dir();

	tmp_dir = g_strconcat(tmp, "/xa-XXXXXX", NULL);

	g_free(value_local);
	g_free(value);

	if (!g_mkdtemp(tmp_dir))
	{
		g_free(tmp_dir);
		xa_show_message_dialog (GTK_WINDOW (xa_main_window),GTK_DIALOG_MODAL,GTK_MESSAGE_ERROR,GTK_BUTTONS_OK,_("Can't create temporary directory:"),g_strerror(errno));

		return FALSE;
	}

	archive->working_dir = tmp_dir;

	return TRUE;
}

gchar *xa_create_working_subdirectory (XArchive *archive)
{
	gchar *subdir;

	if (!xa_create_working_directory(archive))
		return NULL;

	subdir = g_strconcat(archive->working_dir, "/xa-tmp.XXXXXX", NULL);

	if (g_mkdtemp(subdir))
		return subdir;
	else
	{
		g_free(subdir);
		return NULL;
	}
}

gchar *xa_create_containing_directory (XArchive *archive, const gchar *path)
{
	gchar *stem, *dir;
	char *dot;
	guint i;

	stem = g_path_get_basename(archive->path[0]);
	dot = strrchr(stem, '.');

	if (dot)
		*dot = 0;

	if (g_str_has_suffix(stem, ".tar"))
	{
		dot = strrchr(stem, '.');
		*dot = 0;
	}

	dir = g_strconcat(path, "/", stem, NULL);

	for (i = 0; i < 100; i++)
	{
		if (i > 0)
		{
			g_free(dir);
			dir = g_strdup_printf("%s/%s-(%d)", path, stem, i);
		}

		if (!g_file_test(dir, G_FILE_TEST_EXISTS))
		{
			if (g_mkdir_with_parents(dir, 0700) == 0)
			{
				g_free(stem);

				return dir;
			}
		}
	}

	g_free(stem);

	return NULL;
}

gboolean xa_run_command (XArchive *archive, const gchar *command)
{
	pid_t pid = 0;
	int status;

	xa_spawn_async_process(archive, command);

	if (archive->child_pid == 0)
		status = 1;
	else
	{
		while (pid != archive->child_pid && pid != -1)
		{
			pid = waitpid(archive->child_pid, &status, WNOHANG);
			process_gtk_events();
		}

		if (WIFEXITED(status))
			status = WEXITSTATUS(status);
		else
			status = 1;
	}

	xa_child_processed(XA_CHILD_EXIT, status, archive);

	return (status == 0 || (guint8) status == archive->exitstatus_ok);
}

gboolean xa_has_containing_directory (XArchive *archive)
{
	XEntry *entry;
	guint n = 0;

	entry = archive->root_entry->child;

	while (entry)
	{
		if (++n > 1) break;

		entry = entry->next;
	}

	return (n == 1) && archive->root_entry->child->is_dir;
}

gint xa_find_archive_index (gint page_num)
{
	GtkWidget *page;
	gint i;

	page = gtk_notebook_get_nth_page(notebook, page_num);

	for (i = 0; i < MAX_XARCHIVES; i++)
	{
		if (archive[i] != NULL && archive[i]->page == page)
			return i;
	}

	return -1;
}

gint xa_get_new_archive_index ()
{
	gint i;

	for (i = 0; i < MAX_XARCHIVES; i++)
	{
		if (archive[i] == NULL)
			return i;
	}

	return -1;
}

void xa_free_entry (XArchive *archive, XEntry *entry)
{
	gpointer current_column;
	guint i;

	if (entry->child)
		xa_free_entry(archive, entry->child);

	if (entry->next)
		xa_free_entry(archive, entry->next);

	if (entry->columns)
	{
		current_column = entry->columns;

		if (*entry->filename)
		{
			for (i = 2; i < archive->columns - 1; i++)
			{
				switch (archive->column_types[i])
				{
					case G_TYPE_STRING:
						g_free(*(gchar **) current_column);
						current_column += sizeof(gchar *);
						break;

					case G_TYPE_UINT64:
						current_column += sizeof(guint64);
						break;
				}
			}

			g_free(entry->columns);
			g_free(entry->filename);
		}
	}

	g_free(entry);
}

XEntry *xa_set_archive_entries_for_each_row (XArchive *archive, const gchar *filename, gpointer *items)
{
	XEntry *entry = NULL, *last = archive->root_entry;
	gchar **components;
	guint n = 0;

	components = g_strsplit(filename, "/", -1);

	if (*filename == '/')
	{
		gchar *slashdir;

		n = 1;
		slashdir = g_strconcat("/", components[n], NULL);
		g_free(components[n]);
		components[n] = slashdir;
	}

	while (components[n] && *components[n])
	{
		entry = xa_find_directory_entry(last->child, components[n]);

		if (entry == NULL)
		{
			entry = xa_alloc_memory_for_each_row(archive->columns, archive->column_types);

			if (entry == NULL)
				return NULL;

			entry->filename = g_strdup(components[n]);

			if (components[n + 1])
				entry->is_dir = TRUE;

			entry->next = last->child;
			last->child = entry;
			entry->prev = last;
		}

		if (components[n + 1] == NULL || *components[n + 1] == 0)
			entry->columns = xa_fill_archive_entry_columns_for_each_row(archive, entry, items);

		last = entry;
		n++;
	}

	g_strfreev(components);

	return entry;
}

XEntry* xa_find_entry_from_dirpath (XArchive *archive, const gchar *dirpath)
{
	XEntry *root = archive->root_entry, *entry = NULL;
	gchar **components;
	guint n = 0;

	components = g_strsplit(dirpath, "/", -1);

	while (components[n] && *components[n])
	{
		entry = xa_find_directory_entry(root->child, components[n]);
		root = entry;
		n++;
	}

	g_strfreev(components);

	return entry;
}

gchar *xa_build_full_path_name_from_entry (XEntry *entry)
{
	GString *path;
	gchar *path_local;

	path = g_string_new("");

	while (entry)
	{
		if (entry->is_dir)
			path = g_string_prepend_c(path, '/');

		path = g_string_prepend(path, entry->filename);

		entry = entry->prev;
	}

	if (g_utf8_validate(path->str, -1, NULL))
		path_local = g_filename_from_utf8(path->str, -1, NULL, NULL, NULL);
	else
		path_local = g_strdup(path->str);

	g_string_free(path, TRUE);

	return path_local;
}

void xa_fill_list_with_recursed_entries(XEntry *entry,GSList **p_file_list)
{
	if (entry == NULL)
		return;

	xa_fill_list_with_recursed_entries(entry->next ,p_file_list);
	xa_fill_list_with_recursed_entries(entry->child,p_file_list);
	*p_file_list = g_slist_prepend(*p_file_list, xa_build_full_path_name_from_entry(entry));
}

void xa_detect_encrypted_archive (XArchive *archive)
{
	archive->status = XARCHIVESTATUS_LIST;
	(*archive->archiver->list)(archive);

	do
		process_gtk_events();
	while (archive->child_ref);
}

void xa_fill_dir_sidebar(XArchive *archive,gboolean force_reload)
{
	GtkTreeIter iter;

	if (gtk_tree_model_get_iter_first(GTK_TREE_MODEL(archive_dir_treestore), &iter) && force_reload == FALSE)
		return;

	gtk_tree_store_clear(archive_dir_treestore);
	xa_build_dir_sidebar(archive->root_entry, archive_dir_treestore, NULL, NULL);
}

void xa_dir_sidebar_row_selected (GtkTreeSelection *selection, gpointer user_data)
{
	XEntry *entry;
	GtkTreeIter iter;
	GtkTreeIter parent;
	GtkTreePath *path;
	GtkTreeModel *model;
	GString *string = g_string_new("");
	gchar *dir;
	gint idx;

	idx = xa_find_archive_index(gtk_notebook_get_current_page(notebook));

	if ((idx != -1) && gtk_tree_selection_get_selected(selection, &model, &iter))
	{
		path = gtk_tree_model_get_path(model,&iter);
		if ( ! gtk_tree_view_row_expanded(GTK_TREE_VIEW(archive_dir_treeview),path))
			gtk_tree_view_expand_to_path(GTK_TREE_VIEW(archive_dir_treeview),path);
		gtk_tree_path_free(path);
		/* Let get the last selected dir */
		gtk_tree_model_get(model,&iter,1,&dir,-1);
		g_string_prepend_c(string,'/');
		g_string_prepend(string,dir);
		/* Get the memory address of entry so to update the main listview */
		gtk_tree_model_get(model,&iter,2,&entry,-1);
		while (gtk_tree_model_iter_parent(model,&parent,&iter))
		{
			gtk_tree_model_get(model,&parent,1,&dir,-1);
			g_string_prepend_c(string,'/');
			g_string_prepend(string,dir);
			iter = parent;
		}
		gtk_entry_set_text(GTK_ENTRY(location_entry),string->str);
		g_string_free(string,TRUE);

		xa_update_window_with_archive_entries(archive[idx],entry);
		xa_set_statusbar_message_for_displayed_rows(archive[idx]);
	}
}

void xa_dir_sidebar_select_row (XEntry *entry)
{
	gtk_tree_model_foreach(GTK_TREE_MODEL(archive_dir_treestore), xa_dir_sidebar_find_row, entry);
}

gint xa_sort_dirs_before_files (GtkTreeModel *model, GtkTreeIter *a, GtkTreeIter *b, XArchive *archive)
{
	XEntry *entry1, *entry2;

	gtk_tree_model_get(model, a, archive->columns - 1, &entry1, -1);
	gtk_tree_model_get(model, b, archive->columns - 1, &entry2, -1);
	if (entry1->is_dir != entry2->is_dir)
	{
		if (entry1->is_dir)
			return -1;
		else
			return 1;
	}
	/* This for sorting the files */
	return strcasecmp (entry1->filename,entry2->filename);
}
