/*
 *  Copyright (C) 2008 Giuseppe Torelli - <colossus73@gmail.com>
 *  Copyright (C) 2006 Lukasz 'Sil2100' Zemczak - <sil2100@vexillium.org>
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
#include "deb.h"
#include "main.h"
#include "support.h"
#include "window.h"

void xa_deb_ask (XArchive *archive)
{
	archive->can_extract = TRUE;
	archive->can_touch = TRUE;
}

static void xa_ar_parse_output (gchar *line, XArchive *archive)
{
	gchar *filename;
	gpointer item[4];
	gint n = 0, a = 0 ,linesize = 0;

	linesize = strlen(line);

	/* Permissions */
	line[9] = '\0';
	item[0] = line;

	/* Owner */
	for(n=12; n < linesize; ++n)
		if(line[n] == ' ')
			break;
	line[n] = '\0';
	item[1] = line+10;

	/* Size */
	for(++n; n < linesize; ++n)
		if(line[n] >= '0' && line[n] <= '9')
			break;
	a = n;

	for(; n < linesize; ++n)
		if(line[n] == ' ')
			break;

	line[n] = '\0';
	item[2] = line + a;
	archive->files_size += g_ascii_strtoull(item[2],NULL,0);
	a = ++n;

	/* Date Modified */
	for(; n < linesize; ++n)
	{
		if(n == 38)
			break;
	}
	if (line[n] != ' ')
	{
		for(; n < linesize; ++n)
		{
			if(line[n] == ' ')
			break;
		}
	}
	line[n] = '\0';
	item[3] = line + a;

	n++;
	line[linesize-1] = '\0';

	archive->files++;
	filename = g_strdup(line + n);
	xa_set_archive_entries_for_each_row (archive,filename,item);
	g_free(filename);
}

void xa_deb_open (XArchive *archive)
{
	const GType types[] = {GDK_TYPE_PIXBUF, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_UINT64, G_TYPE_STRING, G_TYPE_POINTER};
	const gchar *titles[] = {_("Permissions"), _("Owner/Group"), _("Size"), _("Date modified")};
	gchar *command = NULL;
	guint i;

	command = g_strconcat(archiver[archive->type].program[0], " tv ", archive->path[1], NULL);
	archive->files_size = 0;
	archive->files = 0;
	archive->parse_output = xa_ar_parse_output;
	xa_spawn_async_process (archive,command);
	g_free (command);

	archive->columns = 7;
	archive->column_types = g_malloc0(sizeof(types));

	for (i = 0; i < archive->columns; i++)
		archive->column_types[i] = types[i];

	xa_create_liststore(archive, titles);
}

gboolean xa_deb_extract (XArchive *archive, GSList *file_list)
{
	gchar *command;
	GSList *_files = NULL;
	GString *files = g_string_new("");
	gboolean result;

	_files = file_list;
	while (_files)
	{
		g_string_prepend (files,(gchar*)_files->data);
		g_string_prepend_c (files,' ');
		_files = _files->next;
	}
	g_slist_foreach(file_list,(GFunc)g_free,NULL);
	g_slist_free(file_list);

	chdir(archive->extraction_dir);
	command = g_strconcat(archiver[archive->type].program[0], " x",
	                      archive->do_touch ? " " : "o ",
	                      archive->path[1], files->str, NULL);
	g_string_free(files,FALSE);

	result = xa_run_command(archive, command);
	g_free(command);

	return result;
}
