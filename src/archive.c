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
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <glib.h>
#include <gtk/gtk.h>
#include <sys/wait.h>
#include "archive.h"
#include "support.h"
#include "callbacks.h"

XArchive *xa_init_archive_structure ()
{
	XArchive *archive = NULL;
	archive = g_new0(XArchive,1);
	return archive;
}

void xa_spawn_async_process ( XArchive *archive , gchar *command , gboolean input)
{
	GIOChannel *ioc , *err_ioc, *out_ioc;
	GError *error = NULL;
	gchar **argv;
	gint argcp, response;

	g_shell_parse_argv ( command , &argcp , &argv , NULL);
	if ( ! g_spawn_async_with_pipes (
		NULL,
		argv,
		NULL,
		G_SPAWN_SEARCH_PATH | G_SPAWN_DO_NOT_REAP_CHILD,
		NULL,
		NULL,
		&archive->child_pid,
		input ? &input_fd : NULL,
		&output_fd,
		&error_fd,
		&error) )
	{
		xa_hide_progress_bar_stop_button ( archive );
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

	if ( archive->parse_output )
	{
		ioc = g_io_channel_unix_new ( output_fd );
		g_io_channel_set_encoding (ioc, locale , NULL);
		g_io_channel_set_flags ( ioc , G_IO_FLAG_NONBLOCK , NULL );
		g_io_add_watch (ioc, G_IO_IN|G_IO_PRI|G_IO_ERR|G_IO_HUP|G_IO_NVAL, archive->parse_output, archive);
		g_child_watch_add ( archive->child_pid, (GChildWatchFunc)xa_watch_child, archive);
	}
	else
	{
	out_ioc = g_io_channel_unix_new ( output_fd );
	g_io_channel_set_encoding (out_ioc, locale , NULL);
	g_io_channel_set_flags ( out_ioc , G_IO_FLAG_NONBLOCK , NULL );
	g_io_add_watch (out_ioc, G_IO_IN|G_IO_PRI|G_IO_ERR|G_IO_HUP|G_IO_NVAL, xa_dump_child_output, archive);
	}

	err_ioc = g_io_channel_unix_new ( error_fd );
	g_io_channel_set_encoding (err_ioc, locale , NULL);
	g_io_channel_set_flags ( err_ioc , G_IO_FLAG_NONBLOCK , NULL );
	g_io_add_watch (err_ioc, G_IO_IN|G_IO_PRI|G_IO_ERR|G_IO_HUP|G_IO_NVAL, xa_dump_child_output, archive);
}

gboolean xa_dump_child_output (GIOChannel *ioc, GIOCondition cond, gpointer data)
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
				archive->cmd_line_output = g_list_append (archive->cmd_line_output,line);
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

	if (archive->cmd_line_output != NULL)
	{
		g_list_foreach (archive->cmd_line_output, (GFunc) g_free, NULL);
		g_list_free (archive->cmd_line_output);
		archive->cmd_line_output = NULL;
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
				msg = g_strdup_printf (_("Couldn't remove temporary directory %s"),dummy_string);
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

	if (system_id != NULL)
	{
		g_free (system_id);
		system_id = NULL;
	}

	if (volume_id != NULL)
	{
		g_free (volume_id);
		volume_id = NULL;
	}

	if (publisher_id != NULL)
	{
		g_free (publisher_id);
		publisher_id = NULL;
	}

	if (preparer_id != NULL)
	{
		g_free (preparer_id);
		preparer_id = NULL;
	}

	if (application_id != NULL)
	{
		g_free (application_id);
		application_id = NULL;
	}

	if (creation_date != NULL)
	{
		g_free (creation_date);
		creation_date = NULL;
	}

	if (modified_date != NULL)
	{
		g_free (modified_date);
		modified_date = NULL;
	}

	if (expiration_date != NULL)
	{
		g_free (expiration_date);
		expiration_date = NULL;
	}

	if (effective_date != NULL)
	{
		g_free (effective_date);
		effective_date = NULL;
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

