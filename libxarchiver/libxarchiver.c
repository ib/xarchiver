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
#include <glib.h>
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
		n_cwd = open(".", "r");
	if(support_list == NULL) {
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
	g_slist_foreach (support_list, (GFunc) g_free, NULL);
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
    else return archive->child_pid;
}
