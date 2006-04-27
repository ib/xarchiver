/*
 *  Copyright (c) 2006 Stephan Arts <psyBSD@gmail.com>
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
#include "archive.h"
#include "support.h"


static void
xa_archive_class_init(XAArchiveClass *archive_class);

static void
xa_archive_init(XAArchive *archive);

static void
xa_archive_finalize(GObject *object);

GType
xa_archive_get_type ()
{
	static GType xa_archive_type = 0;

 	if (!xa_archive_type)
	{
 		static const GTypeInfo xa_archive_info = 
		{
			sizeof (XAArchiveClass),
			(GBaseInitFunc) NULL,
			(GBaseFinalizeFunc) NULL,
			(GClassInitFunc) xa_archive_class_init,
			(GClassFinalizeFunc) NULL,
			NULL,
			sizeof (XAArchive),
			0,
			(GInstanceInitFunc) xa_archive_init,
			NULL
		};

		xa_archive_type = g_type_register_static (G_TYPE_OBJECT, "XAArchive", &xa_archive_info, 0);
	}
	return xa_archive_type;
}

static void
xa_archive_class_init(XAArchiveClass *archive_class)
{
	GObjectClass *object_class = G_OBJECT_CLASS(archive_class);

	object_class->finalize = xa_archive_finalize;
}

static void
xa_archive_init(XAArchive *archive)
{
}

static void
xa_archive_finalize(GObject *object)
{
	XAArchive *archive = XA_ARCHIVE(object);
	if(archive->path)
		g_free(archive->path);
}

XAArchive *
xa_archive_new(gchar *path, XAArchiveType type)
{
	XAArchive *archive;

	archive = g_object_new(xa_archive_get_type(), NULL);
	if(path)
		archive->path = g_strdup(path);
	else
		archive->path = NULL;

	archive->type = type;
	
	return archive;
}

gboolean xa_catch_errors (GIOChannel *ioc, GIOCondition cond, gpointer data)
{
	XASupport *support = XA_SUPPORT(data);
	XAArchive *archive = support->exec.archive;
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

