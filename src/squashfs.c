/*
 *  Copyright (C) 2022 Ingo Brückl
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 *  MA 02110-1301 USA.
 */

#include <string.h>
#include "squashfs.h"
#include "main.h"
#include "parser.h"
#include "string_utils.h"
#include "support.h"
#include "window.h"

void xa_squashfs_ask (XArchive *archive)
{
	archive->can_extract = TRUE;
	archive->can_add = archiver[archive->type].is_compressor;
	archive->can_full_path[0] = TRUE;
	archive->can_full_path[1] = archiver[archive->type].is_compressor;
	archive->can_overwrite = TRUE;
	archive->can_recurse = archiver[archive->type].is_compressor;
}

static void xa_squashfs_parse_output (gchar *line, XArchive *archive)
{
	XEntry *entry;
	gpointer item[6];
	gchar *filename;
	gboolean dir, link;

	USE_PARSER;

	/* permissions */
	NEXT_ITEM(item[4]);

	dir = (*(char *) item[4] == 'd');
	link = (*(char *) item[4] == 'l');

	/* owner/group */
	NEXT_ITEM(item[5]);

	/* size */
	NEXT_ITEM(item[1]);

	/* date */
	NEXT_ITEM(item[2]);

	/* time */
	NEXT_ITEM(item[3]);

	/* name */
	LAST_ITEM(filename);

	item[0] = NULL;

	if (link)
	{
		gchar *lnk = g_strrstr(filename, " -> ");

		if (lnk)
		{
			item[0] = lnk + 4;
			*lnk = 0;
		}
	}

	/* drop default destination */
	if (strncmp(filename, "squashfs-root/", 14) == 0)
		filename += 14;

	entry = xa_set_archive_entries_for_each_row(archive, filename, item);

	if (entry)
	{
		if (dir)
			entry->is_dir = TRUE;

		if (!entry->is_dir)
			archive->files++;

		archive->files_size += g_ascii_strtoull(item[1], NULL, 0);
	}
}

void xa_squashfs_list (XArchive *archive)
{
	const GType types[] = {GDK_TYPE_PIXBUF, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_UINT64, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_POINTER};
	const gchar *titles[] = {_("Points to"), _("Original Size"), _("Date"), _("Time"), _("Permissions"), _("Owner/Group")};
	gchar *command;
	guint i;

	archive->files = 0;
	archive->files_size = 0;

	command = g_strconcat(archiver[archive->type].program[0], " -llc ", archive->path[1], NULL);
	archive->parse_output = xa_squashfs_parse_output;
	xa_spawn_async_process(archive, command);
	g_free(command);

	archive->columns = 9;
	archive->size_column = 3;
	archive->column_types = g_malloc0(sizeof(types));

	for (i = 0; i < archive->columns; i++)
		archive->column_types[i] = types[i];

	xa_create_liststore(archive, titles);
}

gboolean xa_squashfs_extract (XArchive *archive, GSList *file_list)
{
	GString *files;
	gchar *extract_to, *command;
	gboolean result;

	/* extract to a new temporary directory because extraction might fail to change permissions for extraction_dir */
	extract_to = xa_create_working_subdirectory(archive);

	if (!extract_to)
		return FALSE;

	files = xa_quote_filenames(file_list, "*?[]", TRUE);
	archive->child_dir = extract_to;
	command = g_strconcat(archiver[archive->type].program[0],
	/* do_overwrite can be ignored in new temporary directory */
	                      " -d . -no-progress ",
	                      archive->path[1], files->str, NULL);

	result = xa_run_command(archive, command);
	g_free(command);

	/* get all files that have been extracted to move them to the destination */
	if (result)
	{
		GString *all_files;

		if (archive->do_full_path)
			all_files = xa_quote_dir_contents(extract_to);
		else
			all_files = xa_collect_files_in_dir(extract_to);

		command = g_strconcat("mv",
		                      archive->do_overwrite ? " -f" : " -n",
		                      " --", all_files->str, " ", archive->extraction_dir, NULL);
		g_string_free(all_files, TRUE);

		archive->status = XARCHIVESTATUS_EXTRACT;   // restore status
		result = xa_run_command(archive, command);
		g_free(command);
	}

	archive->child_dir = NULL;
	g_free(extract_to);
	g_string_free(files, TRUE);

	return result;
}

void xa_squashfs_add (XArchive *archive, GSList *file_list)
{
	GString *files;
	gchar *command;

	files = xa_quote_filenames(file_list, "-", TRUE);
	command = g_strconcat(archiver[archive->type].program[1],
	                      files->str, " ", archive->path[1],
	                      " -no-strip -no-progress", NULL);
	g_string_free(files, TRUE);

	xa_run_command(archive, command);
	g_free(command);
}
