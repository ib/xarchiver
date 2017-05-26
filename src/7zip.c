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
#include "7zip.h"
#include "gzip_et_al.h"
#include "interface.h"
#include "main.h"
#include "string_utils.h"
#include "support.h"
#include "window.h"

#define INDEX (archive->type == XARCHIVETYPE_RAR ? (archive->version == 5 ? 1 : 0) : 0)

static gboolean data_line, encrypted, last_line;

/* it can handle other archive types as well */
void xa_7zip_ask (XArchive *archive)
{
	archive->can_test = TRUE;
	archive->can_extract = TRUE;
	archive->can_add = archiver[archive->type].is_compressor;
	archive->can_delete = (archiver[archive->type].is_compressor && !SINGLE_FILE_COMPRESSOR(archive));
	archive->can_sfx = (archive->type == XARCHIVETYPE_7ZIP);
	archive->can_password = (archive->type == XARCHIVETYPE_7ZIP);
	archive->can_full_path[0] = TRUE;
	archive->can_overwrite = TRUE;
	archive->can_update[1] = archiver[archive->type].is_compressor;
	archive->can_freshen[1] = (archiver[archive->type].is_compressor && !SINGLE_FILE_COMPRESSOR(archive));
	archive->can_move = archiver[archive->type].is_compressor;
	archive->can_solid = (archive->type == XARCHIVETYPE_7ZIP);
}

static gchar *xa_7zip_password_str (XArchive *archive)
{
	if (archive->password)
		return g_strconcat(" -p", archive->password, NULL);
	else
		return g_strdup("");
}

static void xa_7zip_parse_output (gchar *line, XArchive *archive)
{
	XEntry *entry;
	gchar *filename;
	gpointer item[5];
	gint linesize = 0,a = 0;
	gboolean dir;

	if (last_line)
		return;

	if (!data_line)
	{
		if (strncmp(line, "Method = ", 9) == 0 && strstr(line, "7zAES"))
		{
			encrypted = TRUE;
			archive->has_password = TRUE;
		}

		if ((line[0] == '-') && line[3])
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

	linesize = strlen(line);
	archive->files++;

	/* Date */
	line[10] = '\0';
	item[2] = line;

	/* Time */
	line[19] = '\0';
	item[3] = line + 11;

	/* Permissions */
	line[25] = '\0';
	item[4] = line + 20;

	dir = (*(char *) item[4] == 'D');

	/* Size */
	for(a=26; a < linesize; ++a)
		if(line[a] >= '0' && line[a] <= '9')
			break;

	line[38] = '\0';
	item[0] = line + a;
	archive->files_size += g_ascii_strtoull(item[0],NULL,0);

	/* Compressed */
	/* Is this item solid? */
	if (line[50] == ' ')
	{
		line[linesize-1] = '\0';
		item[1] = "0";
	}
	else
	{
		for(a=39; a < linesize; ++a)
			if(line[a] >= '0' && line[a] <= '9')
				break;
		line[51] = '\0';
		item[1] = line + a;
		line[linesize-1] = '\0';
	}

	filename = g_strdup(line + 53);
	entry = xa_set_archive_entries_for_each_row (archive,filename,item);

	if (entry != NULL)
	{
		if (dir)
			entry->is_dir = TRUE;

		entry->is_encrypted = encrypted;
	}

	g_free(filename);
}

static void xa_7zip_uint64_skip (GIOChannel *file)
{
	gchar first, byte;
	guchar mask = 0x80;

	g_io_channel_read_chars(file, &first, sizeof(first), NULL, NULL);

	/* 7z uint64 is specially encoded */
	while ((mask > 1) && (first & mask))
	{
		g_io_channel_read_chars(file, &byte, sizeof(byte), NULL, NULL);
		mask >>= 1;
	}
}

void xa_7zip_list (XArchive *archive)
{
	const GType types[] = {GDK_TYPE_PIXBUF, G_TYPE_STRING, G_TYPE_UINT64, G_TYPE_UINT64, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_POINTER};
	const gchar *titles[] = {_("Original Size"), _("Compressed"), _("Date"), _("Time"), _("Attributes")};
	GIOChannel *file;
	gchar *password_str, *command;
	guint i;

	file = g_io_channel_new_file(archive->path[0], "r", NULL);

	if (file)
	{
		gchar byte;
		guint64 offset = 0;

		g_io_channel_set_encoding(file, NULL, NULL);

		/* skip signature, version and header CRC32 */
		g_io_channel_seek_position(file, 12, G_SEEK_SET, NULL);

		/* next header offset (uint64_t) */
		for (i = 0; i < 8; i++)
		{
			g_io_channel_read_chars(file, &byte, sizeof(byte), NULL, NULL);
			offset |= (guint64) byte << (8 * i);
		}

		/* skip next header size and CRC32 */
		g_io_channel_seek_position(file, 12 + offset, G_SEEK_CUR, NULL);

		/* header info */
		g_io_channel_read_chars(file, &byte, sizeof(byte), NULL, NULL);

		/* encoded header */
		if (byte == 0x17)
		{
			/* streams info */
			g_io_channel_read_chars(file, &byte, sizeof(byte), NULL, NULL);

			/* pack info */
			if (byte == 0x06)
			{
				/* skip pack position */
				xa_7zip_uint64_skip(file);
				/* skip number of pack streams */
				xa_7zip_uint64_skip(file);

				g_io_channel_read_chars(file, &byte, sizeof(byte), NULL, NULL);

				/* size info */
				if (byte == 0x09)
				{
					/* skip unpack sizes */
					xa_7zip_uint64_skip(file);

					g_io_channel_read_chars(file, &byte, sizeof(byte), NULL, NULL);

					/* pack info end */
					if (byte == 0x00)
					{
						/* coders info */
						g_io_channel_read_chars(file, &byte, sizeof(byte), NULL, NULL);

						/* unpack info */
						if (byte == 0x07)
						{
							g_io_channel_read_chars(file, &byte, sizeof(byte), NULL, NULL);

							/* folder */
							if (byte == 0x0b)
							{
								/* skip number of folders */
								xa_7zip_uint64_skip(file);

								g_io_channel_read_chars(file, &byte, sizeof(byte), NULL, NULL);

								/* coders info end */
								if (byte == 0x00)
								{
									g_io_channel_read_chars(file, &byte, sizeof(byte), NULL, NULL);

									/* header or archive properties */
									if (byte == 0x01 || byte == 0x02)
									{
										/* codec id size */
										g_io_channel_read_chars(file, &byte, sizeof(byte), NULL, NULL);

										if ((byte & 0x0f) == 4)
										{
											gchar id[4];

											/* codec id */
											g_io_channel_read_chars(file, id, sizeof(id), NULL, NULL);

											/* check for id of 7zAES */
											archive->has_password = (memcmp(id, "\x06\xf1\x07\x01", 4) == 0);
										}
									}
								}
							}
						}
					}
				}
			}
		}

		g_io_channel_shutdown(file, FALSE, NULL);

		if (archive->has_password)
			if (!xa_check_password(archive))
				return;
	}

	/* a single file compressor archive is no longer new and empty now */
	archive->can_add = (archiver[archive->type].is_compressor && !SINGLE_FILE_COMPRESSOR(archive));

	data_line = FALSE;
	last_line = FALSE;
	encrypted = FALSE;

	password_str = xa_7zip_password_str(archive);
	command = g_strconcat(archiver[archive->type].program[INDEX], " l", password_str, " ", archive->path[1], NULL);
	g_free(password_str);

	archive->files_size = 0;
	archive->files = 0;
	archive->parse_output = xa_7zip_parse_output;
	xa_spawn_async_process (archive,command);
	g_free ( command );

	archive->columns = 8;
	archive->size_column = 2;
	archive->column_types = g_malloc0(sizeof(types));

	for (i = 0; i < archive->columns; i++)
		archive->column_types[i] = types[i];

	xa_create_liststore(archive, titles);
}

void xa_7zip_test (XArchive *archive)
{
	gchar *password_str, *command;

	password_str = xa_7zip_password_str(archive);
	command = g_strconcat(archiver[archive->type].program[INDEX], " t", password_str, " -bd -y ", archive->path[1], NULL);
	g_free(password_str);

	xa_run_command(archive, command);
	g_free(command);
}

/*
 * Note: 7zip's wildcard handling (even with switch -spd) seems buggy.
 * Everything is okay as long as no file name in the working directory
 * matches. If there is a wildcard match, it asks "would you like to replace
 * the existing file" and fails, i.e. extraction of files named '?' or '*'
 * always fails (even in an empty directory) and extraction of a file named
 * 't*' fails if there is already a file name 'test', for example, in the
 * extraction path (while extraction would succeed otherwise).
 */

gboolean xa_7zip_extract (XArchive *archive, GSList *file_list)
{
	GString *files;
	gchar *password_str, *command;
	gboolean result;

	files = xa_quote_filenames(file_list, NULL, TRUE);
	password_str = xa_7zip_password_str(archive);
	command = g_strconcat(archiver[archive->type].program[INDEX],
	                      archive->do_full_path ? " x" : " e",
	                      archive->do_overwrite ? " -aoa" : " -aos",
	                      password_str, " -bd -spd -y ",
	                      archive->path[1], files->str,
	                      " -o", archive->extraction_dir, NULL);
	g_free(password_str);
	g_string_free(files,TRUE);

	result = xa_run_command(archive, command);
	g_free(command);

	return result;
}

void xa_7zip_add (XArchive *archive, GSList *file_list, gchar *compression)
{
	GString *files;
	gchar *password_str, *solid, *command;

	if (archive->location_path != NULL)
		archive->child_dir = g_strdup(archive->working_dir);

	if (!compression)
		compression = "5";

	files = xa_quote_filenames(file_list, NULL, TRUE);
	password_str = xa_7zip_password_str(archive);
	solid = g_strconcat(" -ms=", archive->do_solid ? "on" : "off", NULL);
	command = g_strconcat(archiver[archive->type].program[0],
	                      archive->do_update ? " u" : " a",
	                      archive->do_freshen ? " -ur0w0x1z1" : "",
	                      archive->do_move ? " -sdel" : "",
	                      archive->type == XARCHIVETYPE_7ZIP ? solid : "",
	                      " -mx=", compression,
	                      password_str, " -bd -spd -y ",
	                      archive->path[1], files->str, NULL);
	g_free(solid);
	g_free(password_str);
	g_string_free(files,TRUE);

	xa_run_command(archive, command);
	g_free(command);
}

void xa_7zip_delete (XArchive *archive, GSList *file_list)
{
	GString *files;
	gchar *password_str, *command;

	files = xa_quote_filenames(file_list, NULL, TRUE);
	password_str = xa_7zip_password_str(archive);
	command = g_strconcat(archiver[archive->type].program[0], " d", password_str, " -bd -spd -y ", archive->path[1], files->str, NULL);
	g_free(password_str);
	g_string_free(files,TRUE);

	xa_run_command(archive, command);
	g_free(command);
}
