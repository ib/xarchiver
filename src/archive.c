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
#include <glib-object.h>
#include <sys/wait.h>
#include "archive.h"
#include "support.h"

extern void xa_watch_child ( GPid pid, gint status, gpointer data);
extern int xa_progressbar_pulse ();
extern int ShowGtkMessageDialog ( GtkWindow *window, int mode,int type,int button, const gchar *message1,const gchar *message2);
extern gboolean xa_report_child_stderr (GIOChannel *ioc, GIOCondition cond, gpointer data);
extern const gchar *locale;

XArchive *xa_init_archive_structure (XArchive *archive)
{
	if (archive != NULL)
	{
		//TODO: memory leak with other fields ??
		xa_clean_archive_structure ( archive );
	}
	archive = g_new0(XArchive,1);
	return archive;
}

void SpawnAsyncProcess ( XArchive *archive , gchar *command , gboolean input, gboolean output_flag)
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
		response = ShowGtkMessageDialog (NULL,GTK_DIALOG_MODAL,GTK_MESSAGE_ERROR,GTK_BUTTONS_OK, _("Can't run the archiver executable:"),error->message);
		g_error_free (error);
		g_strfreev ( argv );
        archive->child_pid = 0;

		return;
	}
	g_strfreev ( argv );
	g_timeout_add (200, xa_progressbar_pulse, NULL );

	if ( archive->parse_output )
	{
		ioc = g_io_channel_unix_new ( output_fd );
		g_io_channel_set_encoding (ioc, locale , NULL);
		g_io_channel_set_flags ( ioc , G_IO_FLAG_NONBLOCK , NULL );
		g_io_add_watch (ioc, G_IO_IN|G_IO_PRI|G_IO_ERR|G_IO_HUP|G_IO_NVAL, archive->parse_output, archive);

		g_child_watch_add ( archive->child_pid, (GChildWatchFunc)xa_watch_child, archive);
	}
	if (output_flag)
	{
		out_ioc = g_io_channel_unix_new ( output_fd );
		g_io_channel_set_encoding (out_ioc, locale , NULL);
		g_io_channel_set_flags ( out_ioc , G_IO_FLAG_NONBLOCK , NULL );
		g_io_add_watch (out_ioc, G_IO_IN|G_IO_PRI|G_IO_ERR|G_IO_HUP|G_IO_NVAL, xa_report_child_stderr, NULL);
	}

	err_ioc = g_io_channel_unix_new ( error_fd );
	g_io_channel_set_encoding (err_ioc, locale , NULL);
	g_io_channel_set_flags ( err_ioc , G_IO_FLAG_NONBLOCK , NULL );
	g_io_add_watch (err_ioc, G_IO_IN|G_IO_PRI|G_IO_ERR|G_IO_HUP|G_IO_NVAL, xa_report_child_stderr, NULL);
}

void xa_clean_archive_structure ( XArchive *archive)
{
	if (archive == NULL)
		return;
	
	if(archive->path != NULL)
	{
		g_free(archive->path);
		archive->path = NULL;
	}

	if(archive->escaped_path != NULL)
	{	
		g_free(archive->escaped_path);
		archive->escaped_path = NULL;
	}
		
	if (archive->tmp != NULL)
	{
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
