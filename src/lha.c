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
	archive->can_test = TRUE;
	archive->can_extract = TRUE;
	archive->can_add = archiver[archive->type].is_compressor;
	archive->can_delete = archiver[archive->type].is_compressor;
	archive->can_full_path[0] = TRUE;
	archive->can_full_path[1] = archiver[archive->type].is_compressor;   // n.b.: adds leading slash
	archive->can_overwrite = TRUE;
	archive->can_update[1] = archiver[archive->type].is_compressor;
	archive->can_move = archiver[archive->type].is_compressor;
	archive->can_compress = archiver[archive->type].is_compressor;
	archive->compression = 5;
}

static void xa_lha_parse_output (gchar *line, XArchive *archive)
{
	XEntry *entry;
	gpointer item[7];
	gchar *filename, time[6];
	unsigned int linesize,n,a;
	gboolean dir;

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
	if (strncmp(line,"----",4) == 0)
	{
		last_line = TRUE;
		return;
	}
	linesize = strlen(line);

	/* Permission */
	line[10] = '\0';
	item[5] = line;
	dir = (line[0] == 'd');

	/* UID/GID */
	line[22] = '\0';
	item[6] = line + 11;

	//TODO verify the len of the size column with a big archive
	/* Size */
	for(n = 23;n < linesize;n++)
	if(line[n] != ' ')
		break;

	a = n;
	for(;n < linesize;n++)
	if(line[n] == ' ')
		break;

	line[a+(n-a)] = '\0';
	item[1] = line + a;

    /* Ratio */
    line[37] = '\0';
    item[2] = line + 31;

    /* Date and Time */
    line[50] = '\0';
    item[3] = line + 38;

	/* Time */
	if (((char *) item[3])[9] == ':')
	{
		memcpy(time, item[3] + 7, 5);
		time[5] = 0;
	}
	else
		strcpy(time, "-----");

	item[3] = date_MMM_dD_HourYear(item[3]);
	item[4] = time;

	line[(linesize- 1)] = '\0';
	filename = line + 51;

	/* Symbolic link */
	gchar *temp = g_strrstr (filename,"->");
	if (temp)
	{
		gint len = strlen(filename) - strlen(temp);
		item[0] = (filename +=3) + len;
		filename -= 3;
		filename[strlen(filename) - strlen(temp)-1] = '\0';
	}
	else
		item[0] = NULL;

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

void xa_lha_add (XArchive *archive, GSList *file_list, gchar *compression)
{
	GString *files;
	gchar *command;

	if (archive->location_path != NULL)
		archive->child_dir = g_strdup(archive->working_dir);

	if (!compression)
		compression = "5";

	files = xa_quote_filenames(file_list, NULL, TRUE);
	command = g_strconcat(archiver[archive->type].program[0],
	                      archive->do_update ? " u" : " a",
	                      archive->do_move ? "d" : "",
	                      "o", compression, " ",
	                      archive->path[1], files->str, NULL);
	g_string_free(files,TRUE);

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
