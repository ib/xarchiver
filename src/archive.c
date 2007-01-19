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
#include <sys/wait.h>
#include "archive.h"
#include "support.h"
#include "window.h"

XArchive *xa_init_archive_structure ()
{
	XArchive *archive = NULL;
	archive = g_new0(XArchive,1);
	return archive;
}

void xa_spawn_async_process (XArchive *archive , gchar *command , gboolean input)
{
	gchar **argv;
	gint argcp, response;
	GError *error = NULL;

	g_shell_parse_argv ( command , &argcp , &argv , NULL);
	if ( ! g_spawn_async_with_pipes (
		NULL,
		argv,
		NULL,
		G_SPAWN_SEARCH_PATH | G_SPAWN_DO_NOT_REAP_CHILD,
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

	if (archive->cmd_line_output != NULL)
	{
		g_list_foreach (archive->cmd_line_output, (GFunc) g_free, NULL);
		g_list_free (archive->cmd_line_output);
		archive->cmd_line_output = NULL;
	}

	fcntl (archive->output_fd, F_SETFL, O_NONBLOCK);
	fcntl (archive->error_fd, F_SETFL, O_NONBLOCK);
	archive->source = g_timeout_add (20,xa_watch_child,archive);
}

gboolean xa_dump_output (XArchive *archive)
{
	gchar *line = NULL;
	gint br;

	gtk_progress_bar_pulse(GTK_PROGRESS_BAR(progressbar) );
	br = xa_read_line ( archive->output_fd,&line);
	if (line != NULL)
	{
		archive->cmd_line_output = g_list_append (archive->cmd_line_output,g_strdup(line) );
		if (archive->parse_output)
			(*archive->parse_output) (line,archive);
		g_free (line);
	}
	return br > 0;
}

gboolean xa_dump_errors (XArchive *archive)
{
	gchar *line = NULL;
	gint br;

	br = xa_read_line ( archive->error_fd,&line);

	if (line != NULL)
		g_free (line);
	return br > 0;
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

gint xa_read_line (int fd, gchar **return_string)
{
	gint n;
	gchar c;
	GString *string;

	string = g_string_new ("");

  	do
	{
		n = read(fd, &c, 1);
		if ( n > 0)
			string = g_string_append_c(string,c);
	}
	while ( (n>0) && (c != '\n') );

	if (n>0)
		*return_string = g_strndup (string->str, string->len);
	else
		*return_string = NULL;

	g_string_free(string,FALSE);
	return (n>0);
}


