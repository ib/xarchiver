/*
 *  Copyright (C) 2008 Giuseppe Torelli - <colossus73@gmail.com>
 *  Copyright (C) 2015 Ingo Brückl
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
#include "rar.h"
#include "date_utils.h"
#include "interface.h"
#include "main.h"
#include "parser.h"
#include "string_utils.h"
#include "support.h"
#include "window.h"

int rar_version;

static gboolean header_line, data_line, fname_line, last_line;

void xa_rar_check_version (gchar *path)
{
	gchar *output = NULL, *id = NULL;
	gchar version;

	g_spawn_command_line_sync(path, &output, NULL, NULL, NULL);

	if (output)
	{
		id = strstr(output, "\nRAR ");

		if (!id)
			id = strstr(output, "\nUNRAR ");
	}

	if (id)
	{
		version = *(strchr(id, ' ') + 1);

		if (version > '1' && version <= '9')
			rar_version = version - '0';
	}

	g_free(output);
}

void xa_rar_ask (XArchive *archive)
{
	gboolean read_only;
	compressor_t rar_compressor = {TRUE, 1, 3, 5, 1};

	read_only = (archive->tag == 'x');   // exe

	archive->can_test = TRUE;
	archive->can_extract = TRUE;
	archive->can_add = (archiver[archive->type].is_compressor && !read_only);
	archive->can_delete = (archiver[archive->type].is_compressor && !read_only);
	archive->can_sfx = archiver[archive->type].is_compressor;
	archive->can_password = archiver[archive->type].is_compressor;
	archive->can_full_path[0] = TRUE;
	archive->can_full_path[1] = archiver[archive->type].is_compressor;
	archive->can_touch = TRUE;
	archive->can_overwrite = TRUE;
	archive->can_update[0] = TRUE;
	archive->can_update[1] = archiver[archive->type].is_compressor;
	archive->can_freshen[0] = TRUE;
	archive->can_freshen[1] = archiver[archive->type].is_compressor;
	archive->can_recurse[1] = archiver[archive->type].is_compressor;
	archive->can_move = archiver[archive->type].is_compressor;
	archive->can_solid = archiver[archive->type].is_compressor;
	archive->can_compress = archiver[archive->type].is_compressor;
	archive->compressor = rar_compressor;
	archive->compression = archive->compressor.preset;
}

static gchar *xa_rar_password_str (XArchive *archive)
{
	gchar *escaped, *password_str;

	if (archive->password)
	{
		escaped = xa_escape_bad_chars(archive->password, ESCAPES);
		password_str = g_strconcat(" -p", escaped, NULL);
		g_free(escaped);

		return password_str;
	}
	else
		return g_strdup("");
}

static void xa_rar_v4_parse_output (gchar *line, XArchive *archive)
{
	static gchar *filename;
	static gboolean encrypted;
	XEntry *entry;
	gpointer item[9];
	size_t len;
	gboolean dir;

	USE_PARSER;

	if (last_line)
		return;

	if (!data_line)
	{
		if (!header_line)
		{
			if ((strncmp(line, "Solid ", 6) == 0 || strncmp(line, "SFX ", 4) == 0 ||
			     strncmp(line, "Volume ", 7) == 0 || strncmp(line, "Archive ", 8) == 0)
			     && strstr(line, archive->path[0]))
			{
				header_line = TRUE;

				if (archive->comment)
				{
					if (archive->comment->len > 2)
					{
						archive->has_comment = TRUE;
						archive->comment = g_string_truncate(archive->comment, archive->comment->len - 2);
						archive->comment = g_string_erase(archive->comment, 0, 1);
					}
					else
					{
						g_string_free(archive->comment, TRUE);
						archive->comment = NULL;
					}
				}
			}
			else
			{
				if (!archive->comment)
					archive->comment = g_string_new("");

				archive->comment = g_string_append(archive->comment, line);
			}

			return;
		}

		if (line[0] == '-')
		{
			data_line = TRUE;
			return;
		}

		return;
	}

	if (!fname_line)
	{
		if (line[0] == '-')
		{
			last_line = TRUE;
			return;
		}

		fname_line = TRUE;

		encrypted = (line[0] == '*');

		if (encrypted)
			archive->has_password = TRUE;

		if (line[0])
			line++;

		LAST_ITEM(filename);
		filename = g_strdup(filename);
	}
	else
	{
		/* size */
		NEXT_ITEM(item[0]);

		/* packed */
		NEXT_ITEM(item[1]);

		/* ratio */
		NEXT_ITEM(item[2]);

		/* date */
		NEXT_ITEM(item[3]);
		item[3] = date_DD_MM_YY(item[3]);

		/* time */
		NEXT_ITEM(item[4]);

		/* attributes */
		NEXT_ITEM(item[5]);
		len = strlen(item[5]);

		/* archive may originate from Unix or Windows type OS */
		dir = (*(char *) item[5] == 'd' || (len > 1 && ((char *) item[5])[1] == 'D'));

		/* CRC */
		NEXT_ITEM(item[6]);

		/* method */
		NEXT_ITEM(item[7]);

		/* version */
		NEXT_ITEM(item[8]);

		entry = xa_set_archive_entries_for_each_row(archive, filename, item);

		if (entry)
		{
			if (dir)
				entry->is_dir = TRUE;

			entry->is_encrypted = encrypted;

			if (!entry->is_dir)
				archive->files++;

			archive->files_size += g_ascii_strtoull(item[0], NULL, 0);
		}

		g_free(filename);
		fname_line = FALSE;
	}
}

static void xa_rar_parse_output (gchar *line, XArchive *archive)
{
	XEntry *entry;
	gpointer item[7];
	gchar *filename;
	size_t len;
	gboolean encrypted, dir, legacy;

	USE_PARSER;

	if (last_line)
		return;

	if (!data_line)
	{
		if (!header_line)
		{
			if ((strncmp(line, "Archive: ", 9) == 0) && strstr(line, archive->path[0]))
			{
				header_line = TRUE;

				if (archive->comment)
				{
					if (archive->comment->len > 2)
					{
						archive->has_comment = TRUE;
						archive->comment = g_string_truncate(archive->comment, archive->comment->len - 2);
						archive->comment = g_string_erase(archive->comment, 0, 1);
					}
					else
					{
						g_string_free(archive->comment, TRUE);
						archive->comment = NULL;
					}
				}
			}
			else
			{
				if (!archive->comment)
					archive->comment = g_string_new("");

				archive->comment = g_string_append(archive->comment, line);
			}

			return;
		}

		if (line[0] == '-')
		{
			data_line = TRUE;
			return;
		}

		return;
	}

	if (line[0] == '-')
	{
		last_line = TRUE;
		return;
	}

	encrypted = (line[0] == '*');

	if (encrypted)
	{
		line++;
		archive->has_password = TRUE;
	}

	/* attributes */
	NEXT_ITEM(item[5]);
	len = strlen(item[5]);

	/* archive may originate from Unix or Windows type OS */
	dir = (*(char *) item[5] == 'd' || (len > 3 && ((char *) item[5])[3] == 'D'));

	/* size */
	NEXT_ITEM(item[0]);

	/* packed */
	NEXT_ITEM(item[1]);

	/* ratio */
	NEXT_ITEM(item[2]);

	/* date */
	NEXT_ITEM(item[3]);

	legacy = (strlen(item[3]) != 10);   // date is YYYY-MM-DD since v5.30

	if (legacy)                         // and was DD-MM-YY before
		item[3] = date_DD_MM_YY(item[3]);

	/* time */
	NEXT_ITEM(item[4]);

	/* checksum */
	NEXT_ITEM(item[6]);

	/* name (follows with two characters spacing instead of one) */
	LAST_ITEM(filename);
	filename++;            // skip the additional spacing character

	/* fix filename by stripping trailing whitespace */
	if (legacy && ((len = strlen(filename)) == 12))
	{
		while (len--)
		{
			if (filename[len] == ' ')
				filename[len] = 0;
			else
				break;
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

		archive->files_size += g_ascii_strtoull(item[0], NULL, 0);
	}
}

void xa_rar_list (XArchive *archive)
{
	GIOChannel *file;
	gchar *password_str, *command;
	guint i;

	file = g_io_channel_new_file(archive->path[0], "r", NULL);

	if (file)
	{
		gchar byte[2];

		g_io_channel_set_encoding(file, NULL, NULL);

		/* skip RAR 4 and 5 common signature part */
		g_io_channel_seek_position(file, 6, G_SEEK_SET, NULL);

		g_io_channel_read_chars(file, byte, sizeof(*byte), NULL, NULL);

		/* RAR 4 archive */
		if (*byte == 0)
		{
			/* skip header CRC16 */
			g_io_channel_seek_position(file, 2, G_SEEK_CUR, NULL);

			/* block type */
			g_io_channel_read_chars(file, &byte[0], sizeof(*byte), NULL, NULL);

			/* block flag */
			g_io_channel_read_chars(file, &byte[1], sizeof(*byte), NULL, NULL);

			archive->has_password = (byte[0] == 0x73 && byte[1] & 0x80);
		}
		/* RAR 5 archive */
		else
		{
			/* skip last signature byte and header CRC32 */
			g_io_channel_seek_position(file, 5, G_SEEK_CUR, NULL);

			/* skip vint header size */
			do
				g_io_channel_read_chars(file, byte, sizeof(*byte), NULL, NULL);
			while (*byte & 0x80);

			/* header type */
			g_io_channel_read_chars(file, byte, sizeof(*byte), NULL, NULL);

			archive->has_password = (*byte == 4);
		}

		g_io_channel_shutdown(file, FALSE, NULL);

		if (archive->has_password)
			if (!xa_check_password(archive))
				return;
	}

	header_line = FALSE;
	data_line = FALSE;
	fname_line = FALSE;
	last_line = FALSE;

	password_str = xa_rar_password_str(archive);
	command = g_strconcat(archiver[archive->type].program[0], " v", password_str, " -idc ", archive->path[1], NULL);
	g_free(password_str);

	archive->files_size = 0;
	archive->files = 0;


	if (rar_version >= 5)
	{
		const GType types[] = {GDK_TYPE_PIXBUF, G_TYPE_STRING, G_TYPE_UINT64, G_TYPE_UINT64, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_POINTER};
		const gchar *titles[] = {_("Original Size"), _("Compressed"), _("Occupancy"), _("Date"), _("Time"), _("Attributes"), _("Checksum")};

		archive->parse_output = xa_rar_parse_output;
		xa_spawn_async_process (archive,command);
		g_free ( command );

		archive->columns = 10;
		archive->size_column = 2;
		archive->column_types = g_malloc0(sizeof(types));

		for (i = 0; i < archive->columns; i++)
			archive->column_types[i] = types[i];

		xa_create_liststore(archive, titles);
	}
	else
	{
		const GType types[] = {GDK_TYPE_PIXBUF, G_TYPE_STRING, G_TYPE_UINT64, G_TYPE_UINT64, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_POINTER};
		const gchar *titles[] = {_("Original Size"), _("Compressed"), _("Occupancy"), _("Date"), _("Time"), _("Attributes"), _("Checksum"), _("Method"), _("Version")};

		archive->parse_output = xa_rar_v4_parse_output;
		xa_spawn_async_process (archive,command);
		g_free ( command );

		archive->columns = 12;
		archive->size_column = 2;
		archive->column_types = g_malloc0(sizeof(types));

		for (i = 0; i < archive->columns; i++)
			archive->column_types[i] = types[i];

		xa_create_liststore(archive, titles);
	}
}

void xa_rar_test (XArchive *archive)
{
	gchar *password_str, *command;

	password_str = xa_rar_password_str(archive);
	command = g_strconcat(archiver[archive->type].program[0], " t", password_str, " -idp -y ", archive->path[1], NULL);
	g_free(password_str);

	xa_run_command(archive, command);
	g_free(command);
}

/*
 * Note: rar does not seem to be able to handle wildcards in file names.
 */

gboolean xa_rar_extract (XArchive *archive, GSList *file_list)
{
	GString *files;
	gchar *password_str, *command;
	gboolean result;

	files = xa_quote_filenames(file_list, NULL, DIR_WITHOUT_SLASH);
	password_str = xa_rar_password_str(archive);
	command = g_strconcat(archiver[archive->type].program[0],
	                      archive->do_full_path ? " x" : " e",
	                      archive->do_touch ? " -tsm-" : "",
	                      archive->do_overwrite ? " -o+" : (archive->do_update ? " -u" : (archive->do_freshen ? " -f" : " -o-")),
	                      password_str, " -idp -y ",
	                      archive->path[1], " --", files->str,
	                      " ", archive->extraction_dir, NULL);
	g_free(password_str);
	g_string_free(files,TRUE);

	result = xa_run_command(archive, command);
	g_free(command);

	return result;
}

void xa_rar_add (XArchive *archive, GSList *file_list)
{
	GString *files;
	gchar *compression, *password_str, *command, *version_switch;

	if (rar_version == 5 || rar_version == 6)
	{
		if (TAGTYPE(archive->tag) == 4)
			version_switch = " -ma4";
		else
			version_switch = " -ma5";
	}
	else
		version_switch = "";

	compression = g_strdup_printf("%hu", archive->compression);

	files = xa_quote_filenames(file_list, NULL, DIR_WITHOUT_SLASH);
	password_str = xa_rar_password_str(archive);
	command = g_strconcat(archiver[archive->type].program[0],
	                      version_switch,
	                      archive->do_update ? " u" : (archive->do_freshen ? " u -f" : " a"),
	                      archive->do_recurse ? "" : " -r-",
	                      archive->do_move ? " -df" : "",
	                      archive->do_solid ? " -s" : "",
	                      " -m", compression,
	                      password_str, " -idp -y ",
	                      archive->path[1], " --", files->str, NULL);
	g_free(password_str);
	g_string_free(files,TRUE);
	g_free(compression);

	xa_run_command(archive, command);
	g_free(command);
}

void xa_rar_delete (XArchive *archive, GSList *file_list)
{
	GString *files;
	gchar *command;

	files = xa_quote_filenames(file_list, NULL, DIR_WITHOUT_SLASH);
	command = g_strconcat(archiver[archive->type].program[0], " d -idp -y ", archive->path[1], " --", files->str, NULL);
	g_string_free(files,TRUE);

	xa_run_command(archive, command);
	g_free(command);
}
