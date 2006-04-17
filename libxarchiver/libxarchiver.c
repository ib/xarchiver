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
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.  */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <glib.h>
#include <glib-object.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <fcntl.h>
#include <signal.h>
#include <errno.h>
#include <unistd.h>
#include "archive.h"
#include "support.h"
#include "support-gzip.h"
#include "support-bzip2.h"
#include "support-gnu-tar.h"

#include "internals.h"
#include "libxarchiver.h"

static GSList *support_list = NULL;


static gint
lookup_support( gconstpointer support , gconstpointer archive)
{
	if(support == 0)
		return 1;
	if(((const XASupport *)support)->verify((XAArchive *)archive) == TRUE)
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

		support_list = g_slist_prepend(support_list, xa_support_gzip_new());
		support_list = g_slist_prepend(support_list, xa_support_gnu_tar_new());

	}
}

int
xarchiver_destroy()
{
	GSList *_support = support_list;
	while(_support)
	{
		if(_support->data)
			g_object_unref(_support->data);
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
XAArchive *
xarchiver_archive_new(gchar *path, XAArchiveType type)
{
	if((type == XARCHIVETYPE_UNKNOWN) && (!g_file_test(path, G_FILE_TEST_EXISTS)))
		return NULL;

	XAArchive *archive = xa_archive_new(path, type);

	g_slist_find_custom(support_list, archive, lookup_support);

	if(archive->type == XARCHIVETYPE_UNKNOWN)
	{
		g_object_unref(archive);
		archive = NULL;
	}
	return archive;
}

XASupport *
xarchiver_find_archive_support(XAArchive *archive)
{
	GSList *support = NULL;
	support = g_slist_find_custom(support_list, archive, lookup_support);
	return (XASupport *)(support->data);
}

gboolean
xarchiver_set_channel ( gint fd, GIOCondition cond, GIOFunc func, gpointer data )
{
	GIOChannel *ioc = NULL;
	ioc = g_io_channel_unix_new ( fd );
	//g_io_channel_set_flags ( ioc , G_IO_FLAG_NONBLOCK , NULL );
	g_io_add_watch (ioc, cond, func, data);
	if (ioc == NULL) 
		return FALSE;
	else 
		return TRUE;
}

void
xarchiver_support_connect(gchar *signal, GCallback fp)
{
	GSList *_support = support_list;
	while(_support)
	{
		if(_support->data)
			g_signal_connect(G_OBJECT(_support->data), signal, G_CALLBACK(fp), NULL);
		_support = _support->next;
	}
}
