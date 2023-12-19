/*
 *  Copyright (C) 2008 Giuseppe Torelli - <colossus73@gmail.com>
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
#include "arj.h"
#include "date_utils.h"
#include "main.h"
#include "parser.h"
#include "string_utils.h"
#include "support.h"
#include "window.h"

static gboolean data_line, fname_line;

void xa_arj_ask (XArchive *archive)
{
	gboolean read_only;
	compressor_t arj_compressor = {TRUE, 4, 1, 1, 1};

	read_only = (archive->tag == 'x');   // exe

	archive->can_test = TRUE;
	archive->can_extract = TRUE;
	archive->can_add = (archiver[archive->type].is_compressor && !read_only);
	archive->can_delete = (archiver[archive->type].is_compressor && !read_only);
	archive->can_sfx = archiver[archive->type].is_compressor;
	archive->can_password = archiver[archive->type].is_compressor;
	archive->can_full_path[0] = archiver[archive->type].is_compressor;
	archive->can_full_path[1] = archiver[archive->type].is_compressor;
	archive->can_overwrite = TRUE;
	archive->can_update[0] = TRUE;
	archive->can_update[1] = archiver[archive->type].is_compressor;
	archive->can_freshen[0] = archiver[archive->type].is_compressor;
	archive->can_freshen[1] = archiver[archive->type].is_compressor;
	archive->can_recurse[0] = TRUE;
	archive->can_recurse[1] = archiver[archive->type].is_compressor;
	archive->can_move = archiver[archive->type].is_compressor;
	archive->can_descend = archiver[archive->type].is_compressor;
	archive->can_compress = archiver[archive->type].is_compressor;
	archive->compressor = arj_compressor;
	archive->compression = archive->compressor.preset;
}

static gchar *xa_arj_password_str (XArchive *archive)
{
	gchar *escaped, *password_str;

	if (archive->password && archiver[archive->type].is_compressor)
	{
		escaped = xa_escape_bad_chars(archive->password, ESCAPES);
		password_str = g_strconcat(" -g", escaped, NULL);
		g_free(escaped);

		return password_str;
	}
	else
		return g_strdup("");
}

/*
 * Note: Windows archives with directory structure cannot be displayed
 * correctly with the open source version of the archiver because both
 * directory attributes and directory flags are missing in its output.
 */

static void xa_arj_parse_output (gchar *line, XArchive *archive)
{
	static gchar *filename;
	XEntry *entry;
	gpointer item[7];
	gboolean unarj, lfn, dir, encrypted;
	gint linesize;
	gchar *attr, *flags;
	size_t len;

	USE_PARSER;

	unarj = !archiver[archive->type].is_compressor;

	if (!data_line)
	{
		if (line[0] == '-')
		{
			data_line = TRUE;
			return;
		}

		return;
	}

	if (!fname_line)
	{
		linesize = strlen(line) - 1;

		if (!unarj && (line[0] == ' '))
			return;

		if (line[0] == '-' && linesize == (unarj ? 58 : 40))
		{
			data_line = FALSE;
			return;
		}

		/* name */

		if (unarj)
		{
			/* simple column separator check */
			lfn = (linesize < 68 || line[34] != ' ' || line[40] != ' ' || line[49] != ' ' || line[58] != ' ' || line[67] != ' ');

			if (lfn)
			{
				LAST_ITEM(filename);
				filename = g_strdup(filename);
			}
			else
			{
				filename = g_strchomp(g_strndup(line, 12));

				if (!*filename)
					strcpy(filename, " ");   // just a wild guess in order to have an entry
			}
		}
		else
		{
			lfn = TRUE;
			SKIP_ITEM;
			LAST_ITEM(filename);
			filename = g_strdup(filename);
		}

		fname_line = TRUE;

		if (lfn)
			return;
	}

	if (fname_line)
	{
		LINE_SKIP(12);

		/* size */
		NEXT_ITEM(item[0]);

		/* compressed */
		NEXT_ITEM(item[1]);

		/* ratio */
		NEXT_ITEM(item[2]);

		/* date */
		NEXT_ITEM(item[3]);
		item[3] = date_YY_MM_DD(item[3]);

		/* time */
		NEXT_ITEM(item[4]);

		/* CRC */
		if (unarj)
			NEXT_ITEM(item[6]);

		/* attributes (and perhaps GUA) */
		attr = line;

		LINE_SKIP(unarj ? 4 : 15);

		/* BTPMGVX / BPMGS */
		LAST_ITEM(flags);
		len = strlen(flags);

		/* attributes */

		line = attr;

		if (*line != ' ')
			NEXT_ITEM(item[5]);
		else
			item[5] = NULL;

		/* BTPMGVX */
		if (unarj)
		{
			dir = (len > 1 && flags[1] == 'D');
			encrypted = (len > 4 && flags[4] != ' ');
		}
		/* BPMGS */
		else
		{
			dir = (item[5] && (*(char *) item[5] == 'd'));
			encrypted = (len > 3 && flags[3] != ' ');
		}

		if (encrypted)
			archive->has_password = TRUE;

		if (unarj && dir)
			/* skip entry since unarj lacks directory structure information */
			entry = NULL;
		else
			entry = xa_set_archive_entries_for_each_row(archive, filename, item);

		if (entry)
		{
			if (dir)
				entry->is_dir = TRUE;

			entry->is_encrypted	= encrypted;

			if (!entry->is_dir)
				archive->files++;

			archive->files_size += g_ascii_strtoull(item[0], NULL, 0);
		}

		g_free(filename);
		fname_line = FALSE;
	}
}

void xa_arj_list (XArchive *archive)
{
	const GType types[] = {GDK_TYPE_PIXBUF, G_TYPE_STRING, G_TYPE_UINT64, G_TYPE_UINT64, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, archiver[archive->type].is_compressor ? G_TYPE_POINTER : G_TYPE_STRING, G_TYPE_POINTER};
	const gchar *titles[] = {_("Original Size"), _("Compressed"), _("Ratio"), _("Date"), _("Time"), archiver[archive->type].is_compressor ? _("Permissions") : _("Attributes"), archiver[archive->type].is_compressor ? NULL : _("Checksum")};
	guint i;

	data_line = FALSE;
	fname_line = FALSE;
	gchar *command = g_strconcat(archiver[archive->type].program[0], archiver[archive->type].is_compressor ? " v " : " l ", archive->path[1], NULL);
	archive->files_size = 0;
	archive->files = 0;
	archive->parse_output = xa_arj_parse_output;
	xa_spawn_async_process (archive,command);
	g_free (command);

	archive->columns = (archiver[archive->type].is_compressor ? 9 : 10);
	archive->size_column = 2;
	archive->column_types = g_malloc0(sizeof(types));

	for (i = 0; i < archive->columns; i++)
		archive->column_types[i] = types[i];

	xa_create_liststore(archive, titles);
}

void xa_arj_test (XArchive *archive)
{
	gchar *password_str, *command;

	password_str = xa_arj_password_str(archive);
	command = g_strconcat(archiver[archive->type].program[0], " t", password_str, archiver[archive->type].is_compressor ?  " -i -y " : " ", archive->path[1], NULL);
	g_free(password_str);

	xa_run_command(archive, command);
	g_free(command);
}

gboolean xa_arj_extract (XArchive *archive, GSList *file_list)
{
	GString *files;
	gchar *command;
	gboolean result = FALSE;

	files = xa_quote_filenames(file_list, NULL, DIR_WITH_ASTERISK);

	if (archiver[archive->type].is_compressor)
	{
		gchar *password_str = xa_arj_password_str(archive);
		command = g_strconcat(archiver[archive->type].program[0],
		                      archive->do_full_path ? " x" : " e",
		                      archive->do_overwrite ? "" : (archive->do_update ? " -u" : (archive->do_freshen ? " -f" : " -n")),
		                      archive->do_recurse ? " -p1" : "",
		                      password_str, " -i -y ",
		                      archive->path[1], " ",
		                      archive->extraction_dir, " --", files->str, NULL);
		g_free(password_str);
	}
	else
	{
		if (xa_create_working_directory(archive))
		{
			GString *dir_contents;

			archive->child_dir = g_strdup(archive->working_dir);
			dir_contents = xa_quote_dir_contents(archive->child_dir);

			command = g_strconcat("rm -f --", dir_contents->str, NULL);
			result = xa_run_command(archive, command);

			g_free(command);
			command = NULL;

			g_string_free(dir_contents, TRUE);

			if (result)
			{
				command = g_strconcat(archiver[archive->type].program[0], " e ", archive->path[1], NULL);
				archive->status = XARCHIVESTATUS_EXTRACT;   // restore status
				result = xa_run_command(archive, command);

				g_free(command);
				command = NULL;

				if (result)
				{
					if (strcmp(archive->extraction_dir, archive->working_dir) != 0)
					{
						dir_contents = xa_quote_dir_contents(archive->child_dir);

						command = g_strconcat("mv",
						                      archive->do_overwrite ? " -f" : (archive->do_update ? " -fu" : " -n"),
						                      " --", *files->str ? files->str : dir_contents->str, " ",
						                      archive->extraction_dir, NULL);
						archive->status = XARCHIVESTATUS_EXTRACT;   // restore status

						g_string_free(dir_contents, TRUE);
					}
				}
			}
		}
		else
			command = g_strdup("sh -c \"\"");
	}

	g_string_free(files,TRUE);

	if (command)
		result = xa_run_command(archive, command);

	g_free(command);

	g_free(archive->child_dir);
	archive->child_dir = NULL;

	return result;
}

/*
 * Note: The open source version of the archiver stores timestamps for
 * Unix files by default in Unix format unless the DOS compatibility
 * mode is specified. Not storing them in DOS format as required by
 * the specifications will result in incorrect and sometimes senseless
 * dates and times with standard-compliant programs.
 */

void xa_arj_add (XArchive *archive, GSList *file_list)
{
	GString *files;
	gchar *compression, *password_str, *command;

	compression = g_strdup_printf("%hu", archive->compression);

	files = xa_quote_filenames(file_list, NULL, DIR_WITH_SLASH);
	password_str = xa_arj_password_str(archive);
	command = g_strconcat(archiver[archive->type].program[0],
	                      archive->do_update ? " u" : (archive->do_freshen ? " f" : " a"),
	                      archive->do_recurse ? " -r" : "",
	                      archive->do_move ? " -d1" : "",
	                      " -m", compression,
	                      password_str, " -2d -i -y ",
	                      archive->path[1], " --", files->str, NULL);
	g_free(password_str);
	g_string_free(files,TRUE);
	g_free(compression);

	xa_run_command(archive, command);
	g_free(command);
}

void xa_arj_delete (XArchive *archive, GSList *file_list)
{
	GString *files;
	gchar *command;

	files = xa_quote_filenames(file_list, NULL, DIR_WITH_ASTERISK);
	command = g_strconcat(archiver[archive->type].program[0], " d", archive->do_recurse ? " -p1" : "", " -i -y ", archive->path[1], " --", files->str, NULL);
	g_string_free(files,TRUE);

	xa_run_command(archive, command);
	g_free(command);
}
