/*
 *  Copyright (C) 2017 Ingo Brückl
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
#include "cpio.h"
#include "main.h"
#include "parser.h"
#include "string_utils.h"
#include "support.h"
#include "window.h"

void xa_cpio_ask (XArchive *archive)
{
	archive->can_extract = TRUE;
	archive->can_add = TRUE;
	archive->can_full_path[1] = TRUE;   // n.b.: adds leading slash
	archive->can_touch = TRUE;
	archive->can_overwrite = TRUE;
}

static void xa_cpio_parse_output (gchar *line, XArchive *archive)
{
	XEntry *entry;
	gpointer item[6];
	gchar *filename;
	gboolean dir, link;

	USE_PARSER;

	/* permissions */
	NEXT_ITEM(item[3]);

	dir = (*(char *) item[3] == 'd');
	link = (*(char *) item[3] == 'l');

	/* number of links */
	SKIP_ITEM;

	/* owner */
	NEXT_ITEM(item[4]);

	/* group */
	NEXT_ITEM(item[5]);

	/* size */
	NEXT_ITEM(item[1]);

	/* date and time */
	NEXT_ITEMS(3, item[2]);

	/* name */
	LAST_ITEM(filename);

	item[0] = NULL;

	if (link)
	{
		char *l = strstr(filename, " -> ");

		if (l)
		{
			item[0] = l + 4;
			*l = 0;
		}
	}

	entry = xa_set_archive_entries_for_each_row(archive, filename, item);

	if (entry)
	{
		if (dir)
			entry->is_dir = TRUE;

		if (!entry->is_dir)
			archive->files++;

		archive->files_size = g_ascii_strtoull(item[1], NULL, 0);
	}
}

void xa_cpio_list (XArchive *archive)
{
	const GType types[] = {GDK_TYPE_PIXBUF, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_UINT64, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_POINTER};
	const gchar *titles[] = {_("Points to"), _("Original Size"), _("Date and Time"), _("Permissions"), _("Owner"), _("Group")};
	gchar *command;
	guint i;

	archive->files = 0;
	archive->files_size = 0;

	command = g_strconcat(archiver[archive->type].program[0], " -tv -F ", archive->path[1], NULL);
	archive->parse_output = xa_cpio_parse_output;
	xa_spawn_async_process(archive, command);
	g_free(command);

	archive->columns = 9;
	archive->size_column = 3;
	archive->column_types = g_malloc0(sizeof(types));

	for (i = 0; i < archive->columns; i++)
		archive->column_types[i] = types[i];

	xa_create_liststore(archive, titles);
}

/*
 * Note: cpio lists ' ' as '\ ', '"' as '\"' and '\' as '\\' while it
 * extracts ' ', '"' and '\' respectively, i.e. file names containing
 * one of these three characters can't be handled entirely.
 */

gboolean xa_cpio_extract (XArchive *archive, GSList *file_list)
{
	GString *files;
	gchar *command;
	gboolean result;

	files = xa_quote_filenames(file_list, "*?[]\"", FALSE);
	command = g_strconcat(archiver[archive->type].program[0], " -id",
	                      archive->do_touch ? "" : " -m",
	                      archive->do_overwrite ? " -u" : "",
	                      " -F ", archive->path[1],
	                      " -D ", archive->extraction_dir, files->str, NULL);
	g_string_free(files, TRUE);

	result = xa_run_command(archive, command);
	g_free(command);

	return result;
}

void xa_cpio_add (XArchive *archive, GSList *file_list, gchar *compression)
{
	GString *files;
	gchar *archive_path, *command;

	if (archive->location_path != NULL)
		archive->child_dir = g_strdup(archive->working_dir);

	// no option for compression

	files = xa_quote_filenames(file_list, "\"", FALSE);
	archive_path = xa_quote_shell_command(archive->path[0], TRUE);
	command = g_strconcat("sh -c \"", "ls -d", files->str, " | ",
	                      archiver[archive->type].program[0], " -o",
	                      g_file_test(archive->path[0], G_FILE_TEST_EXISTS) ? " -A" : "",
	                      " -F ", archive_path, "\"", NULL);
	g_free(archive_path);
	g_string_free(files, TRUE);

	xa_run_command(archive, command);
	g_free(command);
}
