/*
 *  Copyright (c) 2006 Giuseppe Torelli <colossus73@gmail.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "config.h"
#include <glib.h>
#include <gtk/gtk.h>
#include "archive.h"
#include "support.h"
#include "window.h"

static gboolean xa_process_output (GIOChannel *ioc, GIOCondition cond, gpointer data);

XArchive *xa_init_archive_structure ()
{
	XArchive *archive = NULL;
	archive = g_new0(XArchive,1);
	return archive;
}

void xa_spawn_async_process (XArchive *archive , gchar *command , gboolean input)
{
	GIOChannel *ioc,*err_ioc;
	gchar **argv;
	gint argcp, response;
	GError *error = NULL;

	g_shell_parse_argv ( command , &argcp , &argv , NULL);
	if ( ! g_spawn_async_with_pipes (
		NULL,
		argv,
		NULL,
		G_SPAWN_LEAVE_DESCRIPTORS_OPEN | G_SPAWN_SEARCH_PATH | G_SPAWN_DO_NOT_REAP_CHILD,
		NULL,
		NULL,
		&archive->child_pid,
		input ? &archive->input_fd : NULL,
		&archive->output_fd,
		&archive->error_fd,
		&error) )
	{
		Update_StatusBar (_("Operation failed."));
		response = xa_show_message_dialog (NULL,GTK_DIALOG_MODAL,GTK_MESSAGE_ERROR,GTK_BUTTONS_OK, _("Can't run the archiver executable:"),error->message);
		g_error_free (error);
		g_strfreev ( argv );
		archive->child_pid = 0;
		xa_set_button_state (1,1,1,archive->can_add,archive->can_extract,archive->has_sfx,archive->has_test,archive->has_properties);
		return;
	}
	g_strfreev ( argv );

	if (archive->pb_source == 0)
		archive->pb_source = g_timeout_add (200, xa_progressbar_pulse, NULL );

	if (archive->error_output != NULL)
	{
		g_slist_foreach (archive->error_output, (GFunc) g_free, NULL);
		g_slist_free (archive->error_output);
		archive->error_output = NULL;
	}

	ioc = g_io_channel_unix_new ( archive->output_fd );
	g_io_channel_set_encoding (ioc, locale , NULL);
	g_io_channel_set_flags ( ioc , G_IO_FLAG_NONBLOCK , NULL );

	if ( archive->parse_output )
	{
		filename_paths_buffer = g_hash_table_new (g_str_hash, g_str_equal);
		g_io_add_watch (ioc, G_IO_IN|G_IO_PRI|G_IO_ERR|G_IO_HUP|G_IO_NVAL, xa_process_output, archive);
	}
	g_child_watch_add ( archive->child_pid, (GChildWatchFunc)xa_watch_child, archive);

	err_ioc = g_io_channel_unix_new ( archive->error_fd );
	g_io_channel_set_encoding (err_ioc, locale , NULL);
	g_io_channel_set_flags ( err_ioc , G_IO_FLAG_NONBLOCK , NULL );
	g_io_add_watch (err_ioc, G_IO_IN|G_IO_PRI|G_IO_ERR|G_IO_HUP|G_IO_NVAL, xa_dump_child_error_messages, archive);
}

static gboolean xa_process_output (GIOChannel *ioc, GIOCondition cond, gpointer data)
{
	XArchive *archive = data;
	GIOStatus status;
	gchar *line = NULL;

	if (cond & (G_IO_IN | G_IO_PRI))
	{
		do
		{
			status = g_io_channel_read_line (ioc, &line, NULL, NULL, NULL);
			if (line != NULL)
			{
				(*archive->parse_output) (line,archive);
				archive->error_output = g_slist_prepend (archive->error_output,g_strdup(line) );
				g_free (line);
			}
			while (gtk_events_pending())
				gtk_main_iteration();
		}
		while (status == G_IO_STATUS_NORMAL);
		if (status == G_IO_STATUS_ERROR || status == G_IO_STATUS_EOF)
			goto done;
	}
	else if (cond & (G_IO_ERR | G_IO_HUP | G_IO_NVAL) )
	{
	done:
		g_io_channel_shutdown (ioc, TRUE, NULL);
		g_io_channel_unref (ioc);

		xa_update_window_with_archive_entries (archive,"/");
		gtk_tree_view_set_model (GTK_TREE_VIEW(archive->treeview), archive->model);
		g_object_unref (archive->model);
		return FALSE;
	}
	return TRUE;
}

gboolean xa_dump_child_error_messages (GIOChannel *ioc, GIOCondition cond, gpointer data)
{
	XArchive *archive = data;
	GIOStatus status;
	gchar *line = NULL;

	if (cond & (G_IO_IN | G_IO_PRI))
	{
		do
		{
			status = g_io_channel_read_line (ioc, &line, NULL, NULL, NULL);
			if (line != NULL)
			{
				archive->error_output = g_slist_prepend (archive->error_output,g_strdup(line) );
				g_free (line);
			}
		}
		while (status == G_IO_STATUS_NORMAL);
		if (status == G_IO_STATUS_ERROR || status == G_IO_STATUS_EOF)
			goto done;
	}
	else if (cond & (G_IO_ERR | G_IO_HUP | G_IO_NVAL) )
	{
	done:
		g_io_channel_shutdown (ioc, TRUE, NULL);
		g_io_channel_unref (ioc);
		return FALSE;
	}
	return TRUE;
}

void xa_clean_archive_structure (XArchive *archive)
{
	gchar *dummy_string;
	gchar *msg;
	int response;

	if (archive == NULL)
		return;

	//TODO: to free entry, g_hash_table,entries
	if (archive->column_types != NULL)
		g_free(archive->column_types);

	if (archive->error_output != NULL)
	{
		g_slist_foreach (archive->error_output, (GFunc) g_free, NULL);
		g_slist_free (archive->error_output);
		archive->error_output = NULL;
	}

	if (archive->path != NULL)
	{
		g_free(archive->path);
		archive->path = NULL;
	}

	if (archive->escaped_path != NULL)
	{
		g_free(archive->escaped_path);
		archive->escaped_path = NULL;
	}

	if (archive->tmp != NULL)
	{
		if ( strncmp (archive->tmp,"/tmp/xa-",8 ) == 0 )
		{
			unlink (archive->tmp);
			dummy_string = remove_level_from_path (archive->tmp);
			if (remove (dummy_string) < 0)
			{
				msg = g_strdup_printf (_("Couldn't remove temporary directory %s:"),dummy_string);
				response = xa_show_message_dialog (GTK_WINDOW (MainWindow),GTK_DIALOG_MODAL,GTK_MESSAGE_ERROR,GTK_BUTTONS_OK,	msg,g_strerror(errno) );
				g_free (msg);
			}
			g_free (dummy_string);
		}
		else
			unlink (archive->tmp);

		g_free(archive->tmp);
		archive->tmp = NULL;
	}

	if (archive->passwd != NULL)
	{
		g_free (archive->passwd);
		archive->passwd = NULL;
	}
	//TODO: to remove this
	if ( archive->extraction_path != NULL )
	{
		if ( strcmp (archive->extraction_path , "/tmp/") != 0)
			g_free (archive->extraction_path);
	}

	if (archive->has_comment)
	{
		if (archive->comment != NULL)
		{
			g_string_free (archive->comment,FALSE);
			archive->comment = NULL;
		}
	}
	g_free (archive);
}

gint xa_find_archive_index ( gint page_num )
{
	GtkWidget *scrollwindow;
	gint i;

	scrollwindow = gtk_notebook_get_nth_page(notebook, page_num);
	for (i = 0; i < 99; i++)
	{
		if (archive[i] != NULL && archive[i]->scrollwindow == scrollwindow)
			return i;
	}
	return -1;
}

gint xa_get_new_archive_idx()
{
	gint i;

	for(i = 0; i < 99; i++)
	{
		if (archive[i] == NULL)
			return i;
	}
	return -1;
}

/* This switch is taken from Squeeze source code */
XEntry *xa_alloc_memory_for_each_row (guint nc,GType column_types[])
{
	XEntry *entry = NULL;
	unsigned short int i;
	gint size = 0;

	entry = g_new0(XEntry,1);
	if (entry == NULL)
		return NULL;

	for (i = 0; i < nc+2; i++)
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

XEntry *xa_set_archive_entries_for_each_row (XArchive *archive,gchar *filename,gpointer *items)
{
	gchar **path_items = NULL;
	XEntry *home_entry = NULL;
	XEntry *child_entry= NULL;
	XEntry *last_entry = NULL;
	unsigned short int x = 1;

	if (strchr(filename,'/') != NULL)
	{
		path_items = g_strsplit_set(filename,"/",-1);
		home_entry = g_hash_table_lookup(filename_paths_buffer,path_items[0]);
		if (home_entry == NULL)
		{
			home_entry = xa_alloc_memory_for_each_row(archive->nc,archive->column_types);
			home_entry->filename = g_strdup(path_items[0]);
			home_entry->columns = xa_fill_archive_entry_columns_for_each_row(archive,home_entry,items);
			g_hash_table_insert (filename_paths_buffer,home_entry->filename,home_entry);
			archive->entries = g_list_prepend (archive->entries,home_entry);
		}
		last_entry = home_entry;
		while (path_items[x])
		{
			if (strlen (path_items[x]) == 0)
				goto here;
			child_entry = g_hash_table_lookup(filename_paths_buffer,path_items[x]);
			if (child_entry == NULL)
			{
				child_entry = xa_alloc_memory_for_each_row (archive->nc,archive->column_types);
				child_entry->filename = g_strdup(path_items[x]);
				child_entry->columns = xa_fill_archive_entry_columns_for_each_row(archive,child_entry,items);
				g_hash_table_insert (filename_paths_buffer,child_entry->filename,child_entry);

				child_entry->next = last_entry->child;
				last_entry->child = child_entry;
			}
			last_entry = child_entry;
here:
			x++;
		}
		g_strfreev(path_items);
	}
	else
	{
		home_entry = xa_alloc_memory_for_each_row (archive->nc,archive->column_types);
		if (home_entry == NULL)
			return NULL;
		home_entry->filename = g_strdup(filename);
		home_entry->columns = xa_fill_archive_entry_columns_for_each_row(archive,home_entry,items);
		home_entry->child = NULL;

		archive->entries = g_list_prepend (archive->entries,home_entry);
		if (archive->entries->prev != NULL)
			home_entry->next = archive->entries->prev->data;
	}
	return home_entry;
}

gpointer *xa_fill_archive_entry_columns_for_each_row (XArchive *archive,XEntry *entry,gpointer *items)
{
	unsigned int i;
	gpointer current_column;

	current_column = entry->columns;

	for (i = 0; i < archive->nc; i++)
	{
		switch(archive->column_types[i+2])
		{
			case G_TYPE_STRING:
				(*((gchar **)current_column)) = g_strdup((gchar*)items[i]);
				//g_message ("%d - %s",i,(*((gchar **)current_column)));
				current_column += sizeof(gchar *);
			break;

			case G_TYPE_UINT64:
				(*((guint64 *)current_column)) = atol(items[i]);
				//g_message ("*%d - %lu",i,(*((guint64 *)current_column)));
				current_column += sizeof(guint64);
			break;
		}
	}
	return entry->columns;
}

void xa_update_window_with_archive_entries (XArchive *archive,gchar *path)
{
	GList *s = NULL;
	XEntry *entry = NULL;
	gchar *location_path = NULL;
	GtkTreeIter iter;
	unsigned short int i;
	gpointer current_column;

	if (*path == '/')
	{
		gtk_list_store_clear(archive->liststore);
		s = archive->entries;
		for (; s; s = s->next)
		{
			entry = s->data;
			current_column = entry->columns;
			gtk_list_store_append (archive->liststore, &iter);
			gtk_list_store_set (archive->liststore,&iter,0,GTK_STOCK_DIRECTORY,1,entry->filename,-1);

			for (i = 0; i < archive->nc; i++)
			{
				switch(archive->column_types[i+2])
				{
					case G_TYPE_STRING:
						//g_message ("%d - %s",i,(*((gchar **)current_column)));
						gtk_list_store_set (archive->liststore,&iter,i+2,(*((gchar **)current_column)),-1);
						current_column += sizeof(gchar *);
					break;

					case G_TYPE_UINT64:
						//g_message ("*%d - %lu",i,(*((guint64 *)current_column)));
						gtk_list_store_set (archive->liststore,&iter,i+2,(*((guint64 *)current_column)),-1);
						current_column += sizeof(guint64);
					break;
				}
			}
		}
		gtk_entry_set_text(GTK_ENTRY(location_entry),"");
		return;
	}
	else
	{
		entry = g_hash_table_lookup (filename_paths_buffer,path);
		if (entry == NULL || entry->child == NULL)
			return;

		location_path = g_strconcat (gtk_entry_get_text(GTK_ENTRY(location_entry)), entry->filename, "/", NULL);
		gtk_entry_set_text(GTK_ENTRY(location_entry),location_path);
		g_free (location_path);

		entry = entry->child;
	}
	gtk_list_store_clear(archive->liststore);

	while (entry)
	{
		current_column = entry->columns;
		gtk_list_store_append (archive->liststore, &iter);
		gtk_list_store_set (archive->liststore,&iter,0,GTK_STOCK_DIRECTORY,1,entry->filename,-1);

		for (i = 0; i < archive->nc; i++)
		{
			switch(archive->column_types[i+2])
			{
				case G_TYPE_STRING:
					//g_message ("%d - %s",i,(*((gchar **)current_column)));
					gtk_list_store_set (archive->liststore,&iter,i+2,(*((gchar **)current_column)),-1);
					current_column += sizeof(gchar *);
				break;

				case G_TYPE_UINT64:
					//g_message ("*%d - %lu",i,(*((guint64 *)current_column)));
					gtk_list_store_set (archive->liststore,&iter,i+2,(*((guint64 *)current_column)),-1);
					current_column += sizeof(guint64);
				break;
			}
		}
		entry=entry->next;
	}
}

void _xa_update_window_with_archive_entries (XArchive *archive,gpointer column)
{

}
