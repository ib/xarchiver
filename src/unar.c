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
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 *  MA 02110-1301 USA.
 */

#include <string.h>
#include "unar.h"
#include "7zip.h"
#include "interface.h"
#include "main.h"
#include "parser.h"
#include "string_utils.h"
#include "support.h"
#include "window.h"

static gboolean data_line, last_line;

void xa_unar_ask (XArchive *archive)
{
	archive->can_test = !(archive->type == XARCHIVETYPE_AR || archive->type == XARCHIVETYPE_COMPRESS || archive->type == XARCHIVETYPE_CPIO || archive->type == XARCHIVETYPE_LZMA || archive->type == XARCHIVETYPE_TAR);
	archive->can_extract = (archiver[archive->type].program[1] != NULL);
	archive->can_full_path[0] = (archiver[archive->type].program[1] != NULL);
	archive->can_overwrite = (archiver[archive->type].program[1] != NULL);
	archive->can_recurse[0] = TRUE;
}

static gchar *xa_unar_password_str (XArchive *archive)
{
	gchar *escaped, *password_str;

	if (archive->password)
	{
		escaped = xa_escape_bad_chars(archive->password, ESCAPES);
		password_str = g_strconcat(" -p ", escaped, NULL);
		g_free(escaped);

		return password_str;
	}
	else
		return g_strdup("");
}

static void xa_unar_parse_output (gchar *line, XArchive *archive)
{
	XEntry *entry;
	gpointer item[6];
	gchar *filename, *flags;
	size_t len;
	gboolean dir, link, encrypted;

	USE_PARSER;

	if (last_line)
		return;

	if (strncmp(line, "     ", 5) == 0)
	{
		data_line = TRUE;
		return;
	}

	if (!data_line)
	{
		if (strstr(line, ": Tar in "))
		{
			XArchiveType type = archive->type;

			xa_get_compressed_tar_type(&type);
			archiver[type].program[0] = g_strdup(archiver[archive->type].program[0]);
			archiver[type].program[1] = g_strdup(archiver[archive->type].program[1]);
			archive->type = type;
			archive->can_test = FALSE;
		}

		return;
	}

	if (line[0] == '(')
	{
		last_line = TRUE;
		return;
	}

	/* index */
	SKIP_ITEM;

	/* flags */
	NEXT_ITEM(flags);
	len = strlen(flags);

	dir = (flags[0] == 'D');
	link = (len > 2 && flags[2] == 'L');
	encrypted = (len > 3 && flags[3] == 'E');

	if (encrypted)
		archive->has_password = TRUE;

	/* file size */
	NEXT_ITEM(item[1]);

	/* ratio */
	NEXT_ITEM(item[2]);

	/* mode */
	NEXT_ITEM(item[5]);

	/* date */
	NEXT_ITEM(item[3]);

	/* time */
	if (strcmp(item[3], "----------------") == 0)
	{
		char *item3 = (char *) item[3];

		item3[10] = 0;
		item[4] = item3 + 11;
	}
	else
		NEXT_ITEM(item[4]);

	/* name (follows with two characters spacing instead of one) */
	LAST_ITEM(filename);
	filename++;            // skip the additional spacing character

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

		entry->is_encrypted = encrypted;

		if (!entry->is_dir)
			archive->files++;

		archive->files_size += g_ascii_strtoull(item[1], NULL, 0);
	}
}

void xa_unar_list (XArchive *archive)
{
	const GType types[] = {GDK_TYPE_PIXBUF, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_UINT64, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_POINTER};
	const gchar *titles[] = {_("Points to"), _("Original Size"), _("Saving"), _("Date"), _("Time"), _("Method")};
	gchar *password_str, *command;
	guint i;

	if (archive->type == XARCHIVETYPE_7ZIP)
	{
		if (!archive->has_password)
			archive->has_password = is7zip_mhe(archive->path[0]);

		if (archive->has_password && !xa_check_password(archive))
			return;
	}

	data_line = FALSE;
	last_line = FALSE;

	archive->files = 0;
	archive->files_size = 0;

	password_str = xa_unar_password_str(archive);
	command = g_strconcat(archiver[archive->type].program[0], " -l", password_str, " ", archive->path[1], NULL);
	archive->parse_output = xa_unar_parse_output;
	xa_spawn_async_process(archive, command);
	g_free(command);
	g_free(password_str);

	archive->columns = 9;
	archive->size_column = 3;
	archive->column_types = g_malloc0(sizeof(types));

	for (i = 0; i < archive->columns; i++)
		archive->column_types[i] = types[i];

	xa_create_liststore(archive, titles);
}

void xa_unar_test (XArchive *archive)
{
	gchar *password_str, *command;

	password_str = xa_unar_password_str(archive);
	command = g_strconcat(archiver[archive->type].program[0], " -t", password_str, " ", archive->path[1], NULL);
	g_free(password_str);

	xa_run_command(archive, command);
	g_free(command);
}

gboolean xa_unar_extract (XArchive *archive, GSList *file_list)
{
	GString *files;
	gchar *extract_to, *password_str, *command;
	gboolean result;

	if (archive->do_full_path)
		extract_to = g_strdup(archive->extraction_dir);
	else
	{
		extract_to = xa_create_working_subdirectory(archive);

		if (!extract_to)
			return FALSE;
	}

	files = xa_quote_filenames(file_list, "*?[]\"", DIR_WITH_ASTERISK);
	password_str = xa_unar_password_str(archive);
	command = g_strconcat(archiver[archive->type].program[1],
	                      password_str, " -D -q",
	                      archive->do_overwrite ? " -f" : "",
	                      " -o ", extract_to,
	                      " ", archive->path[1], " --", files->str, NULL);

	result = xa_run_command(archive, command);
	g_free(command);

	/* collect all files that have been extracted to move them without full path */
	if (result && !archive->do_full_path)
	{
		GString *all_files = xa_collect_files_in_dir(extract_to);

		archive->child_dir = g_strdup(extract_to);
		command = g_strconcat("mv",
		                      archive->do_overwrite ? " -f" : " -n",
		                      " --", all_files->str, " ", archive->extraction_dir, NULL);
		g_string_free(all_files, TRUE);

		archive->status = XARCHIVESTATUS_EXTRACT;   // restore status
		result = xa_run_command(archive, command);
		g_free(command);

		g_free(archive->child_dir);
		archive->child_dir = NULL;
	}

	g_free(extract_to);
	g_free(password_str);
	g_string_free(files, TRUE);

	return result;
}
