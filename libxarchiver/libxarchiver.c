/*
 *  Copyright (c) 2006 Stephan Arts <stephan.arts@hva.nl>
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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <glib.h>
#include <sys/types.h>
#include <fcntl.h>
#include <signal.h>
#include <errno.h>
#include <unistd.h>
#include <libxarchiver/libxarchiver.h>
#include "internals.h"
#include "support-bzip2.h"
#include "support-tar.h"
#include "support-rar.h"
#include "support-gzip.h"

static GSList *support_list = NULL;


static gint
lookup_support( gconstpointer support , gconstpointer archive)
{
	if(support == 0)
		return 1;
	if(((const XArchiveSupport *)support)->verify((XArchive *)archive) == TRUE)
		return 0;
	else
		return 1;
}

void
xarchiver_init()
{
	// open current-working directory
	if(!n_cwd)
		n_cwd = open(".", 'r');
	if(support_list == NULL)
	{
		support_list = g_slist_alloc();

		g_slist_append(support_list, xarchive_gzip_support_new());
		g_slist_append(support_list, xarchive_bzip2_support_new());
		g_slist_append(support_list, xarchive_tar_support_new());
		g_slist_append(support_list, xarchive_rar_support_new());
	}
}

int
xarchiver_destroy()
{
	GSList *_support = support_list;
	while(_support)
	{
		if(_support->data)
			g_free(_support->data);
		_support = _support->next;
	}
	g_slist_free (support_list);
	support_list = NULL;
	close(n_cwd);
	n_cwd = 0;
	return 0;
}

/*
 * xarchiver_archive_new
 *
 * returns: 0 on failure, pointer to Xarchive_structure otherwise
 */
XArchive *
xarchiver_archive_new(gchar *path, XArchiveType type)
{
	if((type == XARCHIVETYPE_UNKNOWN) && (!g_file_test(path, G_FILE_TEST_EXISTS)))
		return NULL;

	XArchive *archive = g_new0(XArchive, 1);
	archive->type = type;
	if(path)
		archive->path = g_strdup(path);
	else
		archive->path = NULL;

	g_slist_find_custom(support_list, archive, lookup_support);

	if(archive->type == XARCHIVETYPE_UNKNOWN)
	{
		if(archive->path)
			g_free(archive->path);

		g_free(archive);
		archive = NULL;
	}
	
	return archive;
}

void
xarchiver_archive_destroy(XArchive *archive)
{
	if(archive->path)
		g_free(archive->path);

	g_free(archive);
	archive = NULL;
}

XArchiveSupport *
xarchiver_find_archive_support(XArchive *archive)
{
	GSList *support = NULL;
	support = g_slist_find_custom(support_list, archive, lookup_support);
	return (XArchiveSupport *)(support->data);
}

gint
xarchiver_async_process ( XArchive *archive , gchar *command, gboolean input)
{
	gchar **argvp;
	int argcp;

	g_shell_parse_argv(command, &argcp, &argvp, NULL);
	if ( ! g_spawn_async_with_pipes (
			NULL,
			argvp,
			NULL,
			G_SPAWN_SEARCH_PATH | G_SPAWN_DO_NOT_REAP_CHILD,
			NULL,
			NULL,
			&archive->child_pid,
			input ? &archive->input_fd : NULL,
			&archive->output_fd,
			&archive->error_fd,
			&archive->error) )
		return 0;
	else 
		return archive->child_pid;
}

gboolean
xarchiver_cancel_operation ( XArchive *archive , gint pid )
{
	//gtk_widget_set_sensitive ( Stop_button , FALSE );
	//Update_StatusBar (_("Waiting for the process to abort..."));
	if ( kill ( pid , SIGABRT ) < 0 )
	{
		g_message ( g_strerror(errno) );
		return FALSE;
	}
	//This in case the user cancel the opening of a password protected archive
	if (archive->status != ADD || archive->status != DELETE);
	
	if (archive->has_passwd)
	{
		archive->has_passwd = FALSE;
		archive->passwd = 0;
	}
	archive->type = XARCHIVETYPE_UNKNOWN;
	archive->status = INACTIVE;
	return TRUE;
}

gboolean
xarchiver_set_channel ( gint fd, GIOCondition cond, GIOFunc func, gpointer data )
{
	GIOChannel *ioc = NULL;
	ioc = g_io_channel_unix_new ( fd );
	g_io_add_watch (ioc, cond, func, data);
	g_io_channel_set_encoding (ioc, "ISO8859-1" , NULL);
	g_io_channel_set_flags ( ioc , G_IO_FLAG_NONBLOCK , NULL );
	if (ioc == NULL) 
		return FALSE;
	else 
		return TRUE;
}

gboolean
xarchiver_error_function (GIOChannel *ioc, GIOCondition cond, gpointer data)
{
	if (cond & (G_IO_IN | G_IO_PRI) )
	{
		gchar *line = NULL;
		g_io_channel_read_line ( ioc, &line, NULL, NULL, NULL );
		//TODO: handle GUI redrawing and filling the gtk_text_buffer with the shell error output
		//while (gtk_events_pending() )
		//gtk_main_iteration();
		if (line != NULL && strcmp (line,"\n") )
		{
 			//gtk_text_buffer_insert_with_tags_by_name (textbuf, &enditer, line , -1, "red_foreground", NULL);
			g_free (line);
		}
		return TRUE;
	}
	else if (cond & (G_IO_ERR | G_IO_HUP | G_IO_NVAL) )
	{
		g_io_channel_shutdown ( ioc,TRUE,NULL );
 		g_io_channel_unref (ioc);
	}
	return FALSE;
}

gboolean
xarchiver_output_function (GIOChannel *ioc, GIOCondition cond, gpointer data)
{
	gchar *line = NULL;
	if (cond & (G_IO_IN | G_IO_PRI) )
	{
 		g_io_channel_read_line ( ioc, &line, NULL, NULL, NULL );
		//TODO: handle GUI redrawing and filling the gtk_text_buffer with the shell output
		//while (gtk_events_pending() )
		//gtk_main_iteration();
		if (line != NULL )
		{
			//gtk_text_buffer_insert (textbuf, &enditer, line, strlen ( line ) );
			g_free (line);
		}
	}
	else if (cond & (G_IO_ERR | G_IO_HUP | G_IO_NVAL) )
	{
		g_io_channel_shutdown ( ioc,TRUE,NULL );
		g_io_channel_unref (ioc);
		return FALSE;
	}
	return TRUE;
}
