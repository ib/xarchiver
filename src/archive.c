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

#include <stdio.h>
#include <fcntl.h>
#include <glib.h>
#include <glib-object.h>
#include <sys/wait.h>
#include "archive.h"
#include "support.h"

extern void xa_watch_child ( GPid pid, gint status, gpointer data);
extern gint xa_progressbar_pulse ();

gboolean xa_catch_errors (GIOChannel *ioc, GIOCondition cond, gpointer data)
{
	XArchive *archive = data;
	gchar *line = NULL;

	if (cond & (G_IO_IN | G_IO_PRI) )
	{
		g_io_channel_read_line ( ioc, &line, NULL, NULL, NULL );
		if (line == NULL) return TRUE;
		archive->error_output = g_slist_prepend ( archive->error_output , line );
	}
	else if (cond & (G_IO_ERR | G_IO_HUP | G_IO_NVAL) )
	{
		g_io_channel_shutdown ( ioc,TRUE,NULL );
		g_io_channel_unref (ioc);
		return FALSE;
	}
	return TRUE;
}

XArchive *xa_init_structure (XArchive *archive)
{
	if (archive != NULL)
	{
		//TODO: memory leak with other fields ??
		if(archive->path)
			g_free(archive->path);

		if(archive->escaped_path)
			g_free(archive->escaped_path);
		
		g_free (archive);
	}
	archive = g_new0(XArchive,1);
	return archive;
}

void SpawnAsyncProcess ( XArchive *archive , gchar *command , gboolean input)
{
	GIOChannel *ioc , *err_ioc;
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
		response = ShowGtkMessageDialog (NULL,GTK_DIALOG_MODAL,GTK_MESSAGE_ERROR,GTK_BUTTONS_OK, error->message);
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
		g_io_channel_set_encoding (ioc, "ISO8859-1" , NULL);
		g_io_channel_set_flags ( ioc , G_IO_FLAG_NONBLOCK , NULL );
		g_io_add_watch (ioc, G_IO_IN|G_IO_PRI|G_IO_ERR|G_IO_HUP|G_IO_NVAL, archive->parse_output, archive);

		err_ioc = g_io_channel_unix_new ( error_fd );
		g_io_channel_set_encoding (ioc, NULL , NULL);
		g_io_add_watch (err_ioc, G_IO_IN|G_IO_PRI|G_IO_ERR|G_IO_HUP|G_IO_NVAL, xa_catch_errors, archive);

		g_child_watch_add ( archive->child_pid, (GChildWatchFunc)xa_watch_child, archive);
	}
}


