/*
 *  Copyright (c) 2006 Stephan Arts     <psyBSD@gmail.com>
 *                     Giuseppe Torelli <colossus73@gmail.com>
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
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <glib.h>
#include <glib-object.h>
#include "internals.h"
#include "archive.h"
#include "archive-types.h"
#include "support.h"
#include "support-gnu-tar.h"

void
xa_support_gnu_tar_init(XASupportGnuTar *support);

gint
xa_support_gnu_tar_open(XASupport *support, XAArchive *archive);

gboolean 
xa_support_gnu_tar_parse_output (GIOChannel *ioc, GIOCondition cond, gpointer data);

GType
xa_support_gnu_tar_get_type ()
{
	static GType xa_support_gnu_tar_type = 0;

 	if (!xa_support_gnu_tar_type)
	{
 		static const GTypeInfo xa_support_gnu_tar_info = 
		{
			sizeof (XASupportGnuTarClass),
			(GBaseInitFunc) NULL,
			(GBaseFinalizeFunc) NULL,
			(GClassInitFunc) NULL,
			(GClassFinalizeFunc) NULL,
			NULL,
			sizeof (XASupportGnuTar),
			0,
			(GInstanceInitFunc) xa_support_gnu_tar_init,
			NULL
		};

		xa_support_gnu_tar_type = g_type_register_static (XA_TYPE_SUPPORT, "XASupportGnuTar", &xa_support_gnu_tar_info, 0);
	}
	return xa_support_gnu_tar_type;
}

void
xa_support_gnu_tar_init(XASupportGnuTar *support)
{
	XASupport *xa_support = XA_SUPPORT(support);
	gint n_columns = 5;
	gchar **column_names  = g_new0(gchar *, n_columns);
	GType *column_types  = g_new0(GType, n_columns);

	column_names[0] = "Filename";
	column_names[1] = "Permissions";
	column_names[2] = "Owner / Group";
	column_names[3] = "Size";
	column_names[4] = "Date";
	column_types[0] = G_TYPE_STRING;
	column_types[1] = G_TYPE_STRING;
	column_types[2] = G_TYPE_STRING;
	column_types[3] = G_TYPE_UINT;   /* UINT  */
	column_types[4] = G_TYPE_STRING; /* DATE */

	xa_support_set_columns(xa_support, n_columns, column_names, column_types);
	xa_support->type = XARCHIVETYPE_TAR;
	xa_support->verify = xa_archive_type_tar_verify;
	xa_support->open = xa_support_gnu_tar_open;
	xa_support->parse_output = xa_support_gnu_tar_parse_output;

	g_free(column_names);
	g_free(column_types);
}

gboolean 
xa_support_gnu_tar_parse_output (GIOChannel *ioc, GIOCondition cond, gpointer data)
{
	gchar *line = NULL;
	gchar *filename = NULL;
	gchar *permissions = NULL;
	gchar *owner = NULL;
	gchar *size = NULL;
	gint _size = 0;
	gchar *date = NULL;

	gint i = 0, a = 0;
	XAArchive *archive = data;
	if (cond & (G_IO_IN | G_IO_PRI) )
	{
		g_io_channel_read_line ( ioc, &line, NULL, NULL, NULL );
		filename = "Filename";
		date = "1-1-2000";
		for(i = 13; i < strlen(line); i++)
			if(line[i] == ' ') break;
		permissions = g_strndup(line, 10);
		owner = g_strndup(&line[11], i-11);
		for(; i < strlen(line); i++)
			if(line[i] >= '0' && line[i] <= '9') break;
		a = i;
		for(; i < strlen(line); i++)
			if(line[i] == ' ') break;
		size = g_strndup(&line[a], i-a);
		_size = atoi(size);
		g_free(size);
		a = i++;
		for(; i < strlen(line); i++) // DATE
			if(line[i] == ' ') break;
		a = i++;
		for(; i < strlen(line); i++) // TIME
			if(line[i] == ' ') break;
		filename = g_strndup(&line[i], strlen(line)-i-1);
		archive->row = g_list_prepend(archive->row, filename);
		archive->row = g_list_prepend(archive->row, permissions);
		archive->row = g_list_prepend(archive->row, owner);
		archive->row = g_list_prepend(archive->row, _size);
		archive->row = g_list_prepend(archive->row, date);
		return TRUE;
	}
	else if (cond & (G_IO_ERR | G_IO_HUP | G_IO_NVAL) )
	{
		g_io_channel_shutdown ( ioc,TRUE,NULL );
		g_io_channel_unref (ioc);
		return FALSE;
	}
	return TRUE;
}

/*
 * xa_support_gnu_tar_open (XAArchive *archive)
 *
 * Open the archive and calls other functions to catch the output
 */
gint
xa_support_gnu_tar_open(XASupport *support, XAArchive *archive)
{
	gchar *command;
	gint child_pid;

	g_mutex_lock(support->exec.command_lock);
	support->exec.command = g_strconcat ( "tar tfv " , archive->path, NULL );
	support->exec.archive = archive;
	g_mutex_unlock(support->exec.command_lock);

	g_thread_create(xa_support_execute, support, FALSE, NULL);

}



XASupport*
xa_support_gnu_tar_new()
{
	XASupport *support;

	support = g_object_new(XA_TYPE_SUPPORT_GNU_TAR, NULL);
	
	return support;
}
