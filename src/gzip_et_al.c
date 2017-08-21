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
#include <time.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include "gzip_et_al.h"
#include "interface.h"
#include "main.h"
#include "parser.h"
#include "string_utils.h"
#include "support.h"
#include "tar.h"
#include "window.h"

#define compress (archive->type == XARCHIVETYPE_COMPRESS)
#define lrzip    (archive->type == XARCHIVETYPE_LRZIP)
#define lz4      (archive->type == XARCHIVETYPE_LZ4)
#define xz       (archive->type == XARCHIVETYPE_XZ)

static gpointer item[7];
static gchar *filename;
static gboolean data_line, last_line;
static gboolean lrzip_can_password;

void xa_gzip_et_al_check_lrzip (const gchar *path)
{
	gchar *command, *output;

	command = g_strconcat(path, " -h", NULL);
	g_spawn_command_line_sync(command, NULL, &output, NULL, NULL);
	g_free(command);

	lrzip_can_password = (strstr(output, "-e, --encrypt[=pass") != NULL);
	g_free(output);
}

static gchar *xa_gzip_et_al_password_str (const gchar *password, XArchiveType type)
{
	if (password)
		if (type == XARCHIVETYPE_LRZIP)
			return g_strconcat(" --encrypt=", password, NULL);

	return g_strdup("");
}

gchar *xa_gzip_et_al_get_command (const gchar *program, gchar *workfile, gchar *archive, const gchar *password, XArchiveType type)
{
	gchar *password_str, *command;

	password_str = xa_gzip_et_al_password_str(password, type);
	workfile = xa_escape_bad_chars(workfile, "\"");
	archive = xa_quote_shell_command(archive, TRUE);
	command = g_strconcat("sh -c \"", program, " ", workfile, password_str, type == XARCHIVETYPE_LRZIP ? " -fo " : " -c > ", archive, "\"", NULL);
	g_free(archive);
	g_free(workfile);
	g_free(password_str);

	return command;
}

static void xa_gzip_et_al_can (XArchive *archive, gboolean can)
{
	archive->can_test = (can && !compress);
	archive->can_extract = can;
	archive->can_password = (can && lrzip && lrzip_can_password);
	archive->can_overwrite = can;
	archive->can_move = can;

	/* only if archive is new and empty */
	archive->can_add = (can && archiver[archive->type].is_compressor);
}

void xa_gzip_et_al_ask (XArchive *archive)
{
	xa_gzip_et_al_can(archive, TRUE);
}

static void xa_gzip_et_al_parse_output (gchar *line, XArchive *archive)
{
	XEntry *entry;
	guint idx0 = 0, idx1 = 1;
	const gchar *streams = NULL, *blocks = NULL;

	USE_PARSER;

	if (archive->type == XARCHIVETYPE_GZIP)
	{
		/* heading? */
		if (line[9] == 'c')
			return;
	}
	else if (archive->type == XARCHIVETYPE_LZIP)
	{
		/* heading? */
		if (line[3] == 'u')
			return;
		else
		{
			idx0 = 1;
			idx1 = 0;
		}
	}
	else if (archive->type == XARCHIVETYPE_LZOP)
	{
		/* heading? */
		if (line[12] == 'c')
			return;
		else
			/* method */
			NEXT_ITEM(item[5]);
	}
	else if (archive->type == XARCHIVETYPE_XZ)
	{
		/* heading? */
		if (*line == 't')
			return;
		else if (*line == 'n')
		{
			/* "name" */
			SKIP_ITEM;
			LAST_ITEM(filename);

			if (g_str_has_suffix(filename, ".xz"))
				*(line - 4) = 0;

			filename = g_path_get_basename(filename);

			return;
		}
		else
		{
			/* "file" */
			SKIP_ITEM;

			/* number of streams */
			NEXT_ITEM(streams);

			/* number of blocks */
			NEXT_ITEM(blocks);
		}
	}
	else
		return;

	/* compressed (uncompressed for lzip) */
	NEXT_ITEM(item[idx1]);

	/* uncompressed (compressed for lzip) */
	NEXT_ITEM(item[idx0]);

	/* ratio */
	NEXT_ITEM(item[2]);

	if (archive->type == XARCHIVETYPE_XZ)
	{
		const gchar *padding;

		/* check type */
		NEXT_ITEM(item[5]);

		/* stream padding */
		NEXT_ITEM(padding);

		item[6] = g_strconcat(streams, "/", blocks, "/", padding, NULL);
	}
	else
	{
		item[6] = NULL;

		/* uncompressed_name */
		LAST_ITEM(filename);

		if ((archive->type == XARCHIVETYPE_LZIP) && g_str_has_suffix(filename, ".lz"))
			*(line - 4) = 0;

		filename = g_path_get_basename(filename);
	}

	entry = xa_set_archive_entries_for_each_row(archive, filename, item);

	if (entry)
	{
		archive->files = 1;
		archive->files_size = g_ascii_strtoull(item[0], NULL, 0);
	}

	g_free(item[3]);
	g_free(item[4]);
	g_free(item[6]);
	g_free(filename);
}

static void xa_gzip_et_al_parse_lrzip (gchar *line, XArchive *archive)
{
	XEntry *entry;

	USE_PARSER;

	if (last_line)
		return;

	if (!data_line)
	{
		LAST_ITEM(filename);

		filename[strlen(filename) - 1] = 0;   // remove colon

		if (g_str_has_suffix(filename, ".lrz"))
			*(line - 6) = 0;

		filename = g_path_get_basename(filename);
		data_line = TRUE;

		return;
	}

	IF_ITEM_LINE("lrzip version:")
	{
		NEXT_ITEM(item[6]);
		item[6] = g_strdup(item[6]);
	}
	else IF_ITEM_LINE("Compression:")
		DUPE_ITEM(item[5]);
	else IF_ITEM_LINE("Decompressed file size:")
		DUPE_ITEM(item[0]);
	else IF_ITEM_LINE("Compressed file size:")
		DUPE_ITEM(item[1]);
	else IF_ITEM_LINE("Compression ratio:")
	{
		NEXT_ITEM(item[2]);
		item[2] = g_strconcat(item[2], ":1", NULL);
		last_line = TRUE;
	}

	if (!last_line)
		return;

	entry = xa_set_archive_entries_for_each_row(archive, filename, item);

	if (entry)
	{
		entry->is_encrypted = archive->has_password;

		archive->files = 1;
		archive->files_size = g_ascii_strtoull(item[0], NULL, 0);
	}

	g_free(item[0]);
	g_free(item[1]);
	g_free(item[2]);
	g_free(item[3]);
	g_free(item[4]);
	g_free(item[5]);
	g_free(item[6]);
	g_free(filename);
}

static void xa_gzip_et_al_globally_stored_entry (gchar *line, XArchive *archive)
{
	XEntry *entry;
	gchar *filename;
	char *dot;

	filename = g_path_get_basename(archive->path[0]);
	dot = strrchr(filename, '.');

	if (dot)
		*dot = 0;

	entry = xa_set_archive_entries_for_each_row(archive, filename, item);

	if (entry)
	{
		entry->is_encrypted = archive->has_password;

		archive->files = 1;
	}

	g_free(item[0]);
	g_free(item[1]);
	g_free(item[2]);
	g_free(item[3]);
	g_free(item[4]);
	g_free(filename);
}

void xa_gzip_et_al_list (XArchive *archive)
{
	GType types[] = {GDK_TYPE_PIXBUF, G_TYPE_STRING, G_TYPE_UINT64, G_TYPE_UINT64, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_POINTER, G_TYPE_POINTER, G_TYPE_POINTER};
	const gchar *titles[] = {_("Original Size"), _("Compressed"), _("Saving"), _("Date"), _("Time"), NULL, NULL};
	const gchar *decompfile = "xa-tmp.decompressed";
	gchar *password_str, *archive_path, *command, *workfile, buffer[12];
	FILE *file;
	struct stat st;
	guint i;

	if (archive->type == XARCHIVETYPE_LRZIP)
	{
		file = fopen(archive->path[0], "r");

		if (file)
		{
			char encrypted = 0;

			if (fseek(file, 22, SEEK_SET) == 0)
				if (fread(&encrypted, sizeof(encrypted), 1, file) == 1)
					if (encrypted == 1)
						archive->has_password = TRUE;

			fclose(file);
		}

		if (archive->has_password && !lrzip_can_password)
		{
			command = g_strconcat("sh -c \"echo ", _("The archiver lacks necessary support for password protected decryption!"), " >&2; exit 1\"", NULL);
			xa_run_command(archive, command);
			g_free(command);
			return;
		}
	}

	if (archive->has_password)
		if (!xa_check_password(archive))
			return;

	if (!xa_create_working_directory(archive))
		return;

	password_str = xa_gzip_et_al_password_str(archive->password, archive->type);
	archive_path = xa_quote_shell_command(archive->path[0], TRUE);

	archive->child_dir = g_strdup(archive->working_dir);
	command = g_strconcat("sh -c \"", archiver[archive->type].program[0], " -d", password_str, " ", archive_path, lrzip ? " -fo " : " -c > ", decompfile, "\"", NULL);
	xa_run_command(archive, command);
	g_free(command);

	g_free(archive->child_dir);
	archive->child_dir = NULL;

	g_free(archive_path);
	g_free(password_str);

	workfile = g_strconcat(archive->working_dir, "/", decompfile, NULL);

	/* check if uncompressed file is tar archive */

	file = fopen(workfile, "r");

	if (file)
	{
		gboolean is_tar;

		is_tar = isTar(file);
		fclose(file);

		if (is_tar && (archiver[XARCHIVETYPE_TAR].ask == xa_tar_ask))
		{
			if (!xa_get_compressed_tar_type(&archive->type))
				return;

			archive->path[2] = g_shell_quote(workfile);
			g_free(workfile);

			xa_gzip_et_al_can(archive, FALSE);

			archive->archiver = &archiver[XARCHIVETYPE_TAR];

			(*archive->archiver->ask)(archive);
			(*archive->archiver->list)(archive);

			return;
		}
	}

	/* continue listing gzip et al. archive type */

	archive->can_add = FALSE;

	stat(archive->path[0], &st);

	/* date */
	strftime(buffer, sizeof(buffer), "%Y-%m-%d", localtime(&st.st_mtime));
	item[3] = g_strdup(buffer);

	/* time */
	strftime(buffer, sizeof(buffer), "%H:%M:%S", localtime(&st.st_mtime));
	item[4] = g_strdup(buffer);

	archive->columns = 8;
	archive->parse_output = NULL;

	switch (archive->type)
	{
		case XARCHIVETYPE_GZIP:
		case XARCHIVETYPE_LRZIP:
		case XARCHIVETYPE_LZIP:
		case XARCHIVETYPE_LZOP:
		case XARCHIVETYPE_XZ:

			if (archive->type == XARCHIVETYPE_LRZIP)
			{
				if (archive->has_password)
					/* no further information will be available then */
					break;

				data_line = FALSE;
				last_line = FALSE;

				types[7] = G_TYPE_STRING;
				types[8] = G_TYPE_STRING;

				titles[2] = _("Compression ratio");
				titles[5] = _("Method");
				titles[6] = _("Version");

				archive->columns = 10;
			}
			else if (archive->type == XARCHIVETYPE_LZOP)
			{
				types[7] = G_TYPE_STRING;

				titles[2] = _("Occupancy");
				titles[5] = _("Method");

				archive->columns = 9;
			}
			else if (archive->type == XARCHIVETYPE_XZ)
			{
				types[7] = G_TYPE_STRING;
				types[8] = G_TYPE_STRING;

				titles[2] = _("Ratio");
				titles[5] = _("Check Type");
				titles[6] = _("Streams/Blocks/Padding");

				archive->columns = 10;
			}

			command = g_strconcat(archiver[archive->type].program[0], lrzip ? " -i" : " -l", xz ? " --robot " : " ", archive->path[1], NULL);
			archive->parse_output = (lrzip ? xa_gzip_et_al_parse_lrzip : xa_gzip_et_al_parse_output);

			break;

		case XARCHIVETYPE_BZIP2:
		case XARCHIVETYPE_COMPRESS:
		case XARCHIVETYPE_LZ4:
		case XARCHIVETYPE_LZMA:
			break;

		default:
			return;
	}

	if (!archive->parse_output)
	{
		off_t compressed;

		/* compressed */
		compressed = st.st_size;
		item[1] = g_strdup_printf("%" G_GUINT64_FORMAT, (guint64) compressed);

		/* uncompressed */
		stat(workfile, &st);
		archive->files_size = (guint64) st.st_size;
		item[0] = g_strdup_printf("%" G_GUINT64_FORMAT, archive->files_size);

		/* saving */
		if (st.st_size)
			item[2] = g_strdup_printf("%.1f%%", 100.0 - 100.0 * compressed / st.st_size);
		else
			item[2] = g_strdup("-");

		/* trigger pseudo-parser once */
		command = g_strdup("sh -c echo");
		archive->parse_output = xa_gzip_et_al_globally_stored_entry;
	}

	g_free(workfile);

	xa_spawn_async_process(archive, command);
	g_free(command);

	archive->size_column = 2;
	archive->column_types = g_malloc0(sizeof(types));

	for (i = 0; i < archive->columns; i++)
		archive->column_types[i] = types[i];

	xa_create_liststore(archive, titles);
}

void xa_gzip_et_al_test (XArchive *archive)
{
	gchar *password_str, *command;

	password_str = xa_gzip_et_al_password_str(archive->password, archive->type);
	command = g_strconcat(archiver[archive->type].program[0], " -t", password_str, lrzip || lz4 ? " " : "v ", archive->path[1], NULL);
	g_free(password_str);

	xa_run_command(archive, command);
	g_free(command);
}

gboolean xa_gzip_et_al_extract (XArchive *archive, GSList *file_list)
{
	GString *files;
	gchar *command, *filename, *files_str, *out_dir, *out_file, *archive_path, *extraction_dir, *password_str;
	gboolean result;

	files = xa_quote_filenames(file_list, NULL, TRUE);

	if (*files->str)
	{
		filename = g_shell_unquote(files->str + 1, NULL);
		files_str = xa_quote_shell_command(files->str + 1, FALSE);
	}
	else
	{
		char *dot;

		filename = g_path_get_basename(archive->path[0]);
		dot = strrchr(filename, '.');

		if (dot)
			*dot = 0;

		files_str = xa_quote_shell_command(filename, TRUE);
	}

	out_dir = g_shell_unquote(archive->extraction_dir, NULL);
	out_file = g_strconcat(out_dir, "/", filename, NULL);

	archive_path = xa_quote_shell_command(archive->path[0], TRUE);
	extraction_dir = xa_quote_shell_command(archive->extraction_dir, FALSE);

	if (archive->do_overwrite || !g_file_test(out_file, G_FILE_TEST_EXISTS))
	{
		password_str = xa_gzip_et_al_password_str(archive->password, archive->type);
		command = g_strconcat("sh -c \"", archiver[archive->type].program[0], " -d", password_str, " ", archive_path, lrzip ? " -fo " : " -c > ", extraction_dir, "/", files_str, "\"", NULL);
		g_free(password_str);
	}
	else
		command = g_strdup("sh -c \"\"");

	g_free(extraction_dir);
	g_free(archive_path);
	g_free(out_file);
	g_free(out_dir);
	g_free(filename);
	g_free(files_str);
	g_string_free(files, TRUE);

	result = xa_run_command(archive, command);
	g_free(command);

	return result;
}

void xa_gzip_et_al_add (XArchive *archive, GSList *file_list, gchar *compression)
{
	GString *files;
	gchar *move, *files_str, *archive_path, *password_str, *command;

	if (archive->location_path != NULL)
		archive->child_dir = g_strdup(archive->working_dir);

	if (!compression)
	{
		switch (archive->type)
		{
			case XARCHIVETYPE_BZIP2:
				compression = "9";
				break;

			case XARCHIVETYPE_COMPRESS:
				compression = "16";
				break;

			case XARCHIVETYPE_GZIP:
			case XARCHIVETYPE_LZIP:
			case XARCHIVETYPE_XZ:
				compression = "6";
				break;

			case XARCHIVETYPE_LZ4:
				compression = "1";
				break;

			case XARCHIVETYPE_LRZIP:
			case XARCHIVETYPE_LZMA:
				compression = "7";
				break;

			case XARCHIVETYPE_LZOP:
				compression = "3";
				break;

			default:
				break;
		}
	}

	files = xa_quote_filenames(file_list, NULL, TRUE);
	files_str = xa_escape_bad_chars(files->str, "\"");
	archive_path = xa_quote_shell_command(archive->path[0], TRUE);

	if (archive->do_move)
		move = g_strconcat(" && rm", files_str, NULL);
	else
		move = g_strdup("");

	password_str = xa_gzip_et_al_password_str(archive->password, archive->type);
	command = g_strconcat("sh -c \"", archiver[archive->type].program[0], " -", compress ? "b " : (lrzip ? "L " : ""), compression, password_str, files_str, lrzip ? " -fo " : " -c > ", archive_path, move, "\"", NULL);
	g_free(password_str);

	g_free(move);
	g_free(archive_path);
	g_free(files_str);
	g_string_free(files, TRUE);

	xa_run_command(archive, command);
	g_free(command);
}
