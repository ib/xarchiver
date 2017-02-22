/*
 *  Copyright (C) 2008 Giuseppe Torelli - <colossus73@gmail.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Street #330, Boston, MA 02111-1307, USA.
 */

#include <string.h>
#include "gzip.h"
#include "main.h"
#include "string_utils.h"
#include "support.h"
#include "tar.h"
#include "window.h"

gboolean gzip_extract(XArchive *archive,GSList *dummy)
{
	gchar *command = NULL,*filename = NULL,*dot,*filename_noext;
	GSList *list = NULL;

	filename = xa_remove_path_from_archive_name(archive->escaped_path);
	dot = strrchr(filename,'.');
	if (G_LIKELY(dot))
	{
		filename_noext = g_strndup(filename,(dot - filename));
		g_free(filename);
	}
	else
		filename_noext = filename;

	command = g_strconcat("sh -c \"gunzip -cd ",archive->escaped_path," > ",archive->extraction_path,"/",filename_noext,"\"",NULL);
	list = g_slist_append(list,command);
	return xa_run_command (archive,list);
}
