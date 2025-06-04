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
#include <glib/gstdio.h>
#include "7zip.h"
#include "gzip_et_al.h"
#include "interface.h"
#include "main.h"
#include "parser.h"
#include "rar.h"
#include "string_utils.h"
#include "support.h"
#include "window.h"

#define INDEX (archive->type == XARCHIVETYPE_RAR ? (TAGTYPE(archive->tag) == 4 ? 0 : 1) : 0)
#define READ_ONLY (TAGTYPE(archive->tag) == 'x')   // exe

static gboolean data_line, encrypted, last_line;

static void xa_7zip_seek_position (const gchar *filename, GIOChannel **file, gint64 offset, GSeekType type)
{
	gchar byte;

	g_io_channel_seek_position(*file, offset, type, NULL);

	/* check whether it's a volume. i.e. whether offset is beyond the end of the file */
	if (g_io_channel_read_chars(*file, &byte, sizeof(byte), NULL, NULL) == G_IO_STATUS_NORMAL)
		/* doesn't seem so - back to requested position */
		g_io_channel_seek_position(*file, -(gint64) sizeof(byte), G_SEEK_CUR, NULL);
	else /* find the volume the offset is pointing to */
	{
		guint64 position, volsizes = 0;
		gchar *fvname, fvext[4];
		size_t ext;
		guint i;
		GStatBuf st;
		GIOChannel *fnew;

		if (!g_str_has_suffix(filename, ".001"))
			return;

		position = 12 + 8 + (guint64) offset;   // absolute position

		fvname = g_strdup(filename);
		ext = strlen(fvname) - 3;

		/* check volumes ... */
		for (i = 1; i < 1000; i++)
		{
			fvname[ext] = 0;
			sprintf(fvext, "%03u", i);
			strcat(fvname, fvext);

			if (!g_file_test(fvname, G_FILE_TEST_EXISTS) || (g_stat(fvname, &st) != 0))
				break;

			volsizes += (guint64) st.st_size;

			/* ... up to the one we're looking for */
			if (volsizes > position)
			{
				fnew = g_io_channel_new_file(fvname, "r", NULL);

				if (!fnew)
					break;

				/* switch to volume */

				g_io_channel_shutdown(*file, FALSE, NULL);

				*file = fnew;

				g_io_channel_set_encoding(*file, NULL, NULL);
				g_io_channel_seek_position(*file, position - (volsizes - (guint64) st.st_size), G_SEEK_SET, NULL);

				break;
			}
		}

		g_free(fvname);
	}
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

static gboolean is_encrypted (XArchive *archive)
{
	gchar *command, *output = NULL;
	gboolean result;

	command = g_strconcat(archiver[archive->type].program[INDEX], " l -slt ", archive->path[1], NULL);
	g_spawn_command_line_sync(command, &output, NULL, NULL, NULL);

	result = (output && strstr(output, "\nEncrypted = +\n"));

	g_free(output);
	g_free(command);

	return result;
}

/* check for header encryption */
gboolean is_7zip_mhe (const gchar *filename)
{
	GIOChannel *file;
	guint i;
	gchar byte;
	guint64 offset = 0;
	gboolean result = FALSE;

	file = g_io_channel_new_file(filename, "r", NULL);

	if (file)
	{
		g_io_channel_set_encoding(file, NULL, NULL);
		g_io_channel_set_buffered(file, FALSE);

		/* skip signature, version and header CRC32 */
		g_io_channel_seek_position(file, 12, G_SEEK_SET, NULL);

		/* next header offset (uint64_t) */
		for (i = 0; i < 8; i++)
		{
			g_io_channel_read_chars(file, &byte, sizeof(byte), NULL, NULL);
			offset |= (guint64) (guchar) byte << (8 * i);
		}

		/* skip next header size and CRC32 */
		xa_7zip_seek_position(filename, &file, 12 + offset, G_SEEK_CUR);

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
											result = (memcmp(id, "\x06\xf1\x07\x01", 4) == 0);
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
	}

	return result;
}

/* it can handle other archive types as well */
void xa_7zip_ask (XArchive *archive)
{
	compressor_t sevenz_compressor = {TRUE, 1, 5, 9, 2};
	compressor_t sevenz_gzip_et_al_compressor = {FALSE, 1, 5, 9, 2};

	archive->can_test = TRUE;
	archive->can_extract = TRUE;
	archive->can_add = (archiver[archive->type].is_compressor && !READ_ONLY);
	archive->can_delete = (archiver[archive->type].is_compressor && !SINGLE_FILE_COMPRESSOR(archive) && !READ_ONLY);
	archive->can_sfx = (archive->type == XARCHIVETYPE_7ZIP);
	archive->can_password = (archive->type == XARCHIVETYPE_7ZIP);
	archive->can_encrypt = (archive->type == XARCHIVETYPE_7ZIP);
	archive->can_full_path[0] = TRUE;
	archive->can_overwrite = TRUE;
	archive->can_update[1] = archiver[archive->type].is_compressor;
	archive->can_freshen[1] = (archiver[archive->type].is_compressor && !SINGLE_FILE_COMPRESSOR(archive));
	archive->can_recurse[0] = TRUE;
	archive->can_recurse[1] = (archiver[archive->type].is_compressor ? FORCED : FALSE);
	archive->can_remove = archiver[archive->type].is_compressor;
	archive->can_solid = (archive->type == XARCHIVETYPE_7ZIP);
	archive->can_compress = archiver[archive->type].is_compressor;
	archive->compressor = (archive->type == XARCHIVETYPE_7ZIP || archive->type == XARCHIVETYPE_ZIP ? sevenz_compressor : sevenz_gzip_et_al_compressor);
	archive->compression = archive->compressor.preset;
}

static gchar *xa_7zip_password_str (XArchive *archive)
{
	gchar *escaped, *password_str;

	if (archive->password)
	{
		escaped = xa_escape_bad_chars(archive->password, ESCAPES);
		password_str = g_strconcat(" -p", escaped, archive->do_encrypt ? " -mhe" : "", NULL);
		g_free(escaped);

		return password_str;
	}
	else
		return g_strdup("");
}

static void xa_7zip_parse_output (gchar *line, XArchive *archive)
{
	XEntry *entry;
	gpointer item[5];
	gchar *filename;
	gboolean dir;

	USE_PARSER;

	if (last_line)
		return;

	if (!data_line)
	{
		/* Since 7zip's plain list command does not indicate whether files are
		 * encrypted, let's assume that 7zAES means all files are encrypted
		 * (although some may be unencrypted).
		 */
		if (strncmp(line, "Method = ", 9) == 0 && strstr(line, "7zAES"))
		{
			encrypted = TRUE;
			archive->has_password = TRUE;
		}

		if (strncmp(line, "------------------- ", 20) == 0)
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

	if (line[0] != ' ')
	{
		/* date */
		NEXT_ITEM(item[2]);
		/* time */
		NEXT_ITEM(item[3]);
	}
	else /* invalid date/time */
	{
		item[2] = NULL;
		item[3] = NULL;
	}

	/* attributes */
	NEXT_ITEM(item[4]);

	dir = (*(char *) item[4] == 'D');

	/* size */

	LINE_PEEK(11);

	if (line[peek] == ' ')
	{
		line += peek + 1;

		if (*line)
			*line++ = 0;

		item[0] = "0";
	}
	else
		NEXT_ITEM(item[0]);

	/* compressed */

	LINE_PEEK(11);

	if (line[peek] == ' ')
	{
		line += peek + 1;

		if (*line)
			*line++ = 0;

		item[1] = "0";
	}
	else
		NEXT_ITEM(item[1]);

	/* name (follows with two characters spacing instead of one) */
	LAST_ITEM(filename);
	filename++;            // skip the additional spacing character

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

void xa_7zip_list (XArchive *archive)
{
	const GType types[] = {GDK_TYPE_PIXBUF, G_TYPE_STRING, G_TYPE_UINT64, G_TYPE_UINT64, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_POINTER};
	const gchar *titles[] = {_("Original Size"), _("Compressed"), _("Date"), _("Time"), _("Attributes")};
	gboolean header_encryption = FALSE;
	gchar *password_str, *command;
	guint i;

	if (!archive->has_password)
	{
		if (archive->type == XARCHIVETYPE_7ZIP)
		{
			archive->has_password = is_7zip_mhe(archive->path[0]);
			header_encryption = archive->has_password;
		}
		else if ((archive->type == XARCHIVETYPE_RAR) && is_rar_hp(archive->path[0]))
		{
			archive->has_password = TRUE;
			header_encryption = archive->has_password;
		}
		else
			/* Since 7zip's plain list command does not indicate whether files are
			 * encrypted, let's assume that, in archives of other types, either all
			 * or none of the files are encrypted (although some may be unencrypted).
			 */
			archive->has_password = is_encrypted(archive);
	}

	if (header_encryption && !xa_check_password(archive))
		return;

	/* a single file compressor archive is no longer new and empty now */
	archive->can_add = (archiver[archive->type].is_compressor && !SINGLE_FILE_COMPRESSOR(archive) && !READ_ONLY);

	data_line = FALSE;
	last_line = FALSE;
	encrypted = archive->has_password;

	password_str = xa_7zip_password_str(archive);

	if (archive->type == XARCHIVETYPE_CPIO && archive->tag == 'E')
		command = g_strconcat("sh -c \"echo ", _("Unsupported binary format!"), " >&2; exit 1\"", NULL);
	else
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

	files = xa_quote_filenames(file_list, NULL, DIR_WITH_SLASH);
	password_str = xa_7zip_password_str(archive);
	command = g_strconcat(archiver[archive->type].program[INDEX],
	                      archive->do_full_path ? " x" : " e",
	                      archive->do_overwrite ? " -aoa" : " -aos",
	                      password_str, " -bd -spd -y",
	                      " -o", archive->extraction_dir,
	                      " ", archive->path[1], " --", files->str, NULL);
	g_free(password_str);
	g_string_free(files,TRUE);

	result = xa_run_command(archive, command);
	g_free(command);

	return result;
}

void xa_7zip_add (XArchive *archive, GSList *file_list)
{
	GString *files;
	gchar *compression, *password_str, *solid, *command;

	compression = g_strdup_printf("%hu", archive->compression);

	files = xa_quote_filenames(file_list, NULL, DIR_WITH_SLASH);
	password_str = xa_7zip_password_str(archive);
	solid = g_strconcat(" -ms=", archive->do_solid ? "on" : "off", NULL);
	command = g_strconcat(archiver[archive->type].program[0],
	                      archive->do_update ? " u" : (archive->do_freshen ? " u -ur0" : " a"),
	                      archive->do_remove ? " -sdel" : "",
	                      archive->type == XARCHIVETYPE_7ZIP ? solid : "",
	                      " -mx=", compression,
	                      password_str, " -bd -spd -y ",
	                      archive->path[1], " --", files->str, NULL);
	g_free(solid);
	g_free(password_str);
	g_string_free(files,TRUE);
	g_free(compression);

	xa_run_command(archive, command);
	g_free(command);
}

void xa_7zip_delete (XArchive *archive, GSList *file_list)
{
	GString *files;
	gchar *password_str, *command;

	files = xa_quote_filenames(file_list, NULL, DIR_WITH_SLASH);
	password_str = xa_7zip_password_str(archive);
	command = g_strconcat(archiver[archive->type].program[0], " d", password_str, " -bd -spd -y ", archive->path[1], " --", files->str, NULL);
	g_free(password_str);
	g_string_free(files,TRUE);

	xa_run_command(archive, command);
	g_free(command);
}
