/*
 *  Copyright (C) 2008 Giuseppe Torelli - <colossus73@gmail.com>
 *  Copyright (C) 2006 Lukasz 'Sil2100' Zemczak - <sil2100@vexillium.org>
 *  Copyright (C) 2016 Ingo Brückl
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
#include "lha.h"
#include "date_utils.h"
#include "main.h"
#include "parser.h"
#include "string_utils.h"
#include "support.h"
#include "window.h"

static gboolean data_line, last_line;

gboolean xa_lha_check_program (gchar *path)
{
	gchar *output = NULL;
	gboolean full_lha;

	g_spawn_command_line_sync(path, &output, NULL, NULL, NULL);
	full_lha = (output && (g_ascii_strncasecmp("Lhasa ", output, 6) != 0));
	g_free(output);

	return full_lha;
}

void xa_lha_ask (XArchive *archive)
{
	gboolean read_only;
	compressor_t lha_compressor = {FALSE, 5, 5, 7, 1};

	read_only = (archive->tag == 'x');   // exe

	archive->can_test = TRUE;
	archive->can_extract = TRUE;
	archive->can_add = (archiver[archive->type].is_compressor && !read_only);
	archive->can_delete = (archiver[archive->type].is_compressor && !read_only);
	archive->can_full_path[0] = TRUE;
	archive->can_full_path[1] = archiver[archive->type].is_compressor;   // n.b.: adds leading slash
	archive->can_overwrite = TRUE;
	archive->can_update[1] = archiver[archive->type].is_compressor;
	archive->can_move = archiver[archive->type].is_compressor;
	archive->can_recurse = (archiver[archive->type].is_compressor ? FORCED : FALSE);
	archive->can_compress = archiver[archive->type].is_compressor;
	archive->compressor = lha_compressor;
	archive->compression = archive->compressor.preset;
}

static void xa_lha_parse_output (gchar *line, XArchive *archive)
{
	XEntry *entry;
	gpointer item[7];
	gchar *filename, time[6];
	gboolean dir, link;

	USE_PARSER;

	if (last_line)
		return;

	if (!data_line)
	{
		if (line[0] == '-')
		{
			data_line = TRUE;
			return;
		}

		return;
	}

	if (strncmp(line, "---------- -", 12) == 0)
	{
		last_line = TRUE;
		return;
	}

	/* permissions */
	NEXT_ITEM(item[5]);

	dir = (*(char *) item[5] == 'd');
	link = (*(char *) item[5] == 'l');

	/* uid/gid */
	NEXT_ITEM(item[6]);

	/* size */
	NEXT_ITEM(item[1]);

	/* ratio */
	NEXT_ITEM(item[2]);

	LINE_PEEK(9);

	/* date and time */
	NEXT_ITEMS(3, item[3]);

	/* time */
	if (((char *) item[3])[peek] == ':')
	{
		memcpy(time, item[3] + 7, 5);
		time[5] = 0;
	}
	else
		strcpy(time, "-----");

	item[3] = date_MMM_dD_HourYear(item[3]);
	item[4] = time;

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

void xa_lha_list (XArchive *archive)
{
	const GType types[] = {GDK_TYPE_PIXBUF, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_UINT64, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_POINTER};
	const gchar *titles[] = {_("Points to"), _("Original Size"), _("Occupancy"), _("Date"), _("Time"), _("Permissions"), _("UID/GID")};
	gchar *command;
	guint i;

	data_line = FALSE;
	last_line = FALSE;
	command = g_strconcat(archiver[archive->type].program[0], " l ", archive->path[1], NULL);
	archive->files_size = 0;
	archive->files = 0;
	archive->parse_output = xa_lha_parse_output;
	xa_spawn_async_process (archive,command);
	g_free (command);

	archive->columns = 10;
	archive->size_column = 3;
	archive->column_types = g_malloc0(sizeof(types));

	for (i = 0; i < archive->columns; i++)
		archive->column_types[i] = types[i];

	xa_create_liststore(archive, titles);
}

void xa_lha_test (XArchive *archive)
{
	gchar *command;

	command = g_strconcat(archiver[archive->type].program[0], " t ", archive->path[1], NULL);

	xa_run_command(archive, command);
	g_free(command);
}

/*
 * Note: lha does not seem to be able to handle wildcards in file names.
 */

gboolean xa_lha_extract (XArchive *archive, GSList *file_list)
{
	GString *files;
	gchar *command;
	gboolean result;

	files = xa_quote_filenames(file_list, NULL, TRUE);
	command = g_strconcat(archiver[archive->type].program[0],
	                      archive->do_full_path ? " x" : " xi",
	                      archive->do_overwrite ? "f" : "",
	                      "w=", archive->extraction_dir, " ",
	                      archive->path[1], files->str, NULL);
	g_string_free(files,TRUE);

	result = xa_run_command(archive, command);
	g_free(command);

	return result;
}

void xa_lha_add (XArchive *archive, GSList *file_list)
{
	GString *files;
	gchar *compression, *command;

	compression = g_strdup_printf("%hu", archive->compression);

	files = xa_quote_filenames(file_list, NULL, TRUE);
	command = g_strconcat(archiver[archive->type].program[0],
	                      archive->do_update ? " u" : " a",
	                      archive->do_move ? "d" : "",
	                      "o", compression, " ",
	                      archive->path[1], files->str, NULL);
	g_string_free(files,TRUE);
	g_free(compression);

	xa_run_command(archive, command);
	g_free(command);
}

void xa_lha_delete (XArchive *archive, GSList *file_list)
{
	GString *files;
	gchar *command;

	files = xa_quote_filenames(file_list, NULL, TRUE);
	command = g_strconcat(archiver[archive->type].program[0], " d ", archive->path[1], files->str, NULL);
	g_string_free(files,TRUE);

	xa_run_command(archive, command);
	g_free(command);
}
