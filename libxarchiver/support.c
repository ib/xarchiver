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
#include <sys/types.h>
#include <signal.h>
#include <glib.h>
#include <glib-object.h>
#include "archive.h"
#include "support.h"

static guint xa_support_signals[4];

void
xa_support_init(XASupport *support);
void
xa_support_class_init(XASupportClass *supportclass);

GType
xa_support_get_type ()
{
	static GType xa_support_type = 0;

 	if (!xa_support_type)
	{
 		static const GTypeInfo xa_support_info = 
		{
			sizeof (XASupportClass),
			(GBaseInitFunc) NULL,
			(GBaseFinalizeFunc) NULL,
			(GClassInitFunc) xa_support_class_init,
			(GClassFinalizeFunc) NULL,
			NULL,
			sizeof (XASupport),
			0,
			(GInstanceInitFunc) xa_support_init,
		};

		xa_support_type = g_type_register_static (G_TYPE_OBJECT, "XASupport", &xa_support_info, 0);
	}
	return xa_support_type;
}

gint
xa_support_verify (XAArchive *archive)
{
	g_critical("Verify not supported by this support object");
	return -1;
}

gint
xa_support_add (XASupport *support, XAArchive *archive, GSList *list)
{
	g_critical("Add not supported by this support object");
	return -1;
}

gint
xa_support_extract (XASupport *support, XAArchive *archive, gchar *a, GSList *list, gboolean ab)
{
	g_critical("Extract not supported by this support object");
	return -1;
}

gint
xa_support_remove (XASupport *support, XAArchive *archive, GSList *list)
{
	g_critical("Remove not supported by this support object");
	return -1;
}

gint
xa_support_testing (XASupport *support, XAArchive *archive)
{
	g_critical("Testing not supported by this support object");
	return -1;
}

gint
xa_support_open (XASupport *support, XAArchive *archive)
{
	g_critical("Open not supported by this support object");
	return -1;
}

gint
xa_support_view (XASupport *support, XAArchive *archive, gchar *filename)
{
	g_critical("View not supported by this support object");
	return -1;
}

gint
xa_support_cancel (XASupport *support)
{
	if(kill (support->exec.child_pid, SIGABRT ) < 0)
	{
		return 1;
	}
	return 0;
}


void
xa_support_init(XASupport *support)
{
	support->add     = xa_support_add;
	support->verify  = xa_support_verify;
	support->remove  = xa_support_remove;
	support->extract = xa_support_extract;
	support->testing = xa_support_testing;
	support->open    = xa_support_open;
	support->view    = xa_support_view;
}

void
xa_support_class_init(XASupportClass *supportclass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS (supportclass);
	XASupportClass *klass = XA_SUPPORT_CLASS (supportclass);

	xa_support_signals[XA_SUPPORT_SIGNAL_UPDATE_ROWS] = g_signal_new("xa_rows_updated",
			G_TYPE_FROM_CLASS(supportclass),
			G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
			0,
			NULL,
			NULL,
			g_cclosure_marshal_VOID__POINTER,
			G_TYPE_NONE,
			1,
			G_TYPE_POINTER,
			NULL);

	xa_support_signals[XA_SUPPORT_SIGNAL_APPEND_ROWS] = g_signal_new("xa_rows_appended",
			G_TYPE_FROM_CLASS(supportclass),
			G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
			0,
			NULL,
			NULL,
			g_cclosure_marshal_VOID__POINTER,
			G_TYPE_NONE,
			1,
			G_TYPE_POINTER,
			NULL);

	xa_support_signals[XA_SUPPORT_SIGNAL_ARCHIVE_MODIFIED] = g_signal_new("xa_archive_modified",
			G_TYPE_FROM_CLASS(supportclass),
			G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
			0,
			NULL,
			NULL,
			g_cclosure_marshal_VOID__POINTER,
			G_TYPE_NONE,
			1,
			G_TYPE_POINTER,
			NULL);

	xa_support_signals[XA_SUPPORT_SIGNAL_OPERATION_COMPLETE] = g_signal_new("xa_operation_complete",
			G_TYPE_FROM_CLASS(supportclass),
			G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
			0,
			NULL,
			NULL,
			g_cclosure_marshal_VOID__POINTER,
			G_TYPE_NONE,
			1,
			G_TYPE_POINTER,
			NULL);
}

void
xa_support_get_columns(XASupport *support, gint *n_columns, gchar ***column_names, GType **column_types)
{
	gint i = 0;
	gchar **_column_names;
	GType *_column_types;

	_column_names = g_new0(gchar *, support->n_columns);
	_column_types = g_new0(GType, support->n_columns);
	for(i = 0; i < support->n_columns; i++)
	{
		_column_names[i] = g_strdup(support->column_names[i]);
		_column_types[i] = support->column_types[i];
	}
	*n_columns = support->n_columns;
	*column_names = _column_names;
	*column_types = _column_types;
	
}

void
xa_support_set_columns(XASupport *support, gint n_columns, gchar **column_names, GType *column_types)
{
	gint i = 0;
	if(support->column_names)
	{
		for(i = 0; i < support->n_columns; i++)
			g_free(support->column_names[i]);
		g_free(support->column_names);
	}

	support->n_columns = n_columns;
	support->column_names = g_new0(gchar *, n_columns);
	support->column_types = g_new0(GType, n_columns);
	for(i = 0; i < support->n_columns; i++)
	{
		support->column_names[i] = g_strdup(column_names[i]);
		support->column_types[i] = column_types[i];
	}
	
}

void
xa_support_emit_signal(XASupport *support, gint i)
{
	g_signal_emit(G_OBJECT(support), xa_support_signals[i], 0, support->exec.archive);
}

void
xa_support_watch_child (GPid pid, gint status, XASupport *support)
{
	g_spawn_close_pid(pid);

	switch(status)
	{
		case(0):
			if(support->exec.signal >= 0)
				xa_support_emit_signal(support, support->exec.signal);
			break;
	}
	xa_support_emit_signal(support, XA_SUPPORT_SIGNAL_OPERATION_COMPLETE);
}

gpointer
xa_support_execute(gpointer data)
{
	XASupport *support = data;
	gint in_fd;
	gint out_fd;
	gint err_fd;
	gchar **argvp;
	gint argcp;
	GPid child_pid;
	GIOChannel *ioc;
	GSource *source = NULL;

	if(support->exec.archive->row)
	{
		//g_list_foreach(support->exec.archive->row, (GFunc)g_free, NULL);
		g_list_free(support->exec.archive->row);
		support->exec.archive->row = NULL;
	}

	g_shell_parse_argv(support->exec.command, &argcp, &argvp, NULL);
	if ( ! g_spawn_async_with_pipes (
			NULL,
			argvp,
			NULL,
			G_SPAWN_SEARCH_PATH | G_SPAWN_DO_NOT_REAP_CHILD,
			NULL,
			NULL,
			&child_pid,
			&in_fd,
			&out_fd,
			&err_fd,
			NULL) )
		return 0;
	support->exec.child_pid = child_pid;
	if(support->exec.parse_output)
	{
		ioc = g_io_channel_unix_new(out_fd);
		support->exec.watch_source = g_io_add_watch(ioc, G_IO_IN | G_IO_PRI | G_IO_ERR | G_IO_HUP | G_IO_NVAL, support->exec.parse_output, support);
	}
	else
		support->exec.watch_source = 0;

	support->exec.source = g_child_watch_add(child_pid, (GChildWatchFunc)xa_support_watch_child, support);
	g_free(support->exec.command);
	support->exec.command = NULL;
}

XASupport*
xa_support_new()
{
	XASupport *support;

	support = g_object_new(XA_TYPE_SUPPORT, NULL);
	
	return support;
}
