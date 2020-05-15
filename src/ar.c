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
#include "ar.h"
#include "date_utils.h"
#include "main.h"
#include "string_utils.h"
#include "support.h"
#include "window.h"

void xa_ar_ask (XArchive *archive)
{
	archive->can_extract = TRUE;
	archive->can_touch = TRUE;
}

static void xa_ar_parse_output (gchar *line, XArchive *archive)
{
	XEntry *entry;
	gchar *filename, time[6];
	gpointer item[5];
	gint n = 0, a = 0 ,linesize = 0;

	linesize = strlen(line);

	/* Permissions */
	line[9] = '\0';
	item[3] = line;

	/* Owner */
	for(n=12; n < linesize; ++n)
		if(line[n] == ' ')
			break;
	line[n] = '\0';
	item[4] = line+10;

	/* Size */
	for(++n; n < linesize; ++n)
		if(line[n] >= '0' && line[n] <= '9')
			break;
	a = n;

	for(; n < linesize; ++n)
		if(line[n] == ' ')
			break;

	line[n] = '\0';
	item[0] = line + a;
	a = ++n;

	/* Date Modified */
	for(; n < linesize; ++n)
	{
		if (n - a == 17)
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

	/* Time */
	strncpy(time, line + a + 7, 5);
	time[5] = 0;
	item[2] = time;

	/* Date */
	strncpy(line + a + 7, line + a + 13, 5);
	item[1] = date_MMM_dD_HourYear(line + a);

	n++;
	line[linesize-1] = '\0';

	filename = g_strdup(line + n);

	entry = xa_set_archive_entries_for_each_row(archive, filename, item);

	if (entry)
	{
		if (!entry->is_dir)
			archive->files++;

		archive->files_size += g_ascii_strtoull(item[0], NULL, 0);
	}

	g_free(filename);
}

void xa_ar_list (XArchive *archive)
{
	const GType types[] = {GDK_TYPE_PIXBUF, G_TYPE_STRING, G_TYPE_UINT64, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_POINTER};
	const gchar *titles[] = {_("Original Size"), _("Date"), _("Time"), _("Permissions"), _("UID/GID")};
	gchar *command = NULL;
	guint i;

	command = g_strconcat(archiver[archive->type].program[0], " tv ", archive->path[1], NULL);
	archive->files_size = 0;
	archive->files = 0;
	archive->parse_output = xa_ar_parse_output;
	xa_spawn_async_process (archive,command);
	g_free (command);

	archive->columns = 8;
	archive->size_column = 2;
	archive->column_types = g_malloc0(sizeof(types));

	for (i = 0; i < archive->columns; i++)
		archive->column_types[i] = types[i];

	xa_create_liststore(archive, titles);
}

gboolean xa_ar_extract (XArchive *archive, GSList *file_list)
{
	GString *files;
	gchar *command;
	gboolean result;

	files = xa_quote_filenames(file_list, NULL, TRUE);
	archive->child_dir = g_strdup(archive->extraction_dir);
	command = g_strconcat(archiver[archive->type].program[0], " x",
	                      archive->do_touch ? " " : "o ",
	                      archive->path[1], files->str, NULL);
	g_string_free(files, TRUE);

	result = xa_run_command(archive, command);
	g_free(command);

	g_free(archive->child_dir);
	archive->child_dir = NULL;

	return result;
}
