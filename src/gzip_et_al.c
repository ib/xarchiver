/*
 *  Copyright (C) 2008 Giuseppe Torelli - <colossus73@gmail.com>
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

#ifdef __APPLE__
#include <libkern/OSByteOrder.h>
#define le32toh(x) OSSwapLittleToHostInt32(x)
#define htole32(x) OSSwapHostToLittleInt32(x)
#elif defined(__FreeBSD__)
#include <sys/endian.h>
#else
#include <endian.h>
#endif
#include <stdint.h>
#include <stdio.h>
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

#define bzip     (archive->type == XARCHIVETYPE_BZIP)
#define bzip3    (archive->type == XARCHIVETYPE_BZIP3)
#define compress (archive->type == XARCHIVETYPE_COMPRESS)
#define lrzip    (archive->type == XARCHIVETYPE_LRZIP)
#define lz4      (archive->type == XARCHIVETYPE_LZ4)
#define mozlz4   (archive->type == XARCHIVETYPE_LZ4 && archive->tag == 'm')
#define rzip     (archive->type == XARCHIVETYPE_RZIP)
#define xz       (archive->type == XARCHIVETYPE_XZ)
#define zstd     (archive->type == XARCHIVETYPE_ZSTD)

static gpointer item[7];
static gchar *filename;
static gboolean first_line, data_line, last_line;
static gboolean lrzip_can_password;
static gboolean zstd_can_list, zstd_can_test;

void xa_gzip_et_al_check_lrzip (const gchar *path)
{
	gchar *command, *output = NULL;

	command = g_strconcat(path, " -h", NULL);
	g_spawn_command_line_sync(command, NULL, &output, NULL, NULL);
	g_free(command);

	lrzip_can_password = (output && strstr(output, "-e, --encrypt[=pass"));
	g_free(output);
}

static gboolean xa_gzip_et_al_zstd_option (const gchar *output, const gchar *option)
{
	const gchar *nl, *delim;
	size_t op;

	if (!output)
		return FALSE;

	nl = output;
	op = strlen(option);

	while ((nl = strchr(nl, '\n')))
	{
		nl++;

		/* skip multiple leading spaces added since v1.5.4 */
		while (*nl && (*nl == ' '))
			nl++;

		if (!*nl)
			break;

		if (strncmp(nl, option, op) == 0)
		{
			delim = nl + op;

			if (*delim && (*delim == ' ' || (*delim == ',' && *++delim == ' ')))
				return TRUE;
		}
	}

	return FALSE;
}

gchar *xa_gzip_et_al_check_zstd (const gchar *compressor, const gchar *decompressor, gboolean *is_compressor)
{
	gchar *path, *command, *output = NULL;
	gboolean found_compressor = FALSE;

	path = g_find_program_in_path(compressor);

	if (path)
		found_compressor = TRUE;
	else
		path = g_find_program_in_path(decompressor);

	if (!path)
		return NULL;

	command = g_strconcat(path, " -H", NULL);
	g_spawn_command_line_sync(command, &output, NULL, NULL, NULL);
	g_free(command);

	/* check whether decompression is available */
	if (xa_gzip_et_al_zstd_option(output, "-d"))
	{
		if (found_compressor)
			*is_compressor = xa_gzip_et_al_zstd_option(output, "-#");

		zstd_can_list = xa_gzip_et_al_zstd_option(output, "-l");
		zstd_can_test = (xa_gzip_et_al_zstd_option(output, "--test") || /* check short test option just in case */ xa_gzip_et_al_zstd_option(output, "-t"));
	}
	else   // useless
	{
		g_free(path);
		path = NULL;
	}

	g_free(output);

	return path;
}

static gchar *xa_gzip_et_al_password_str (const gchar *password, XArchiveType type)
{
	gchar *escaped, *quoted, *password_str;

	if (password)
	{
		if (type == XARCHIVETYPE_LRZIP)
		{
			escaped = xa_escape_bad_chars(password, "\"");
			quoted = g_shell_quote(escaped);
			password_str = g_strconcat(" --encrypt=", quoted, NULL);
			g_free(quoted);
			g_free(escaped);

			return password_str;
		}
	}

	return g_strdup("");
}

gchar *xa_gzip_et_al_get_command (const gchar *program, gchar *workfile, gchar *archive, const gchar *password, XArchiveType type)
{
	gchar *password_str, *command;

	password_str = xa_gzip_et_al_password_str(password, type);
	workfile = xa_escape_bad_chars(workfile, "\"");
	archive = xa_quote_shell_command(archive, TRUE);
	command = g_strconcat("sh -c \"exec ", program, " ", workfile, password_str, type == XARCHIVETYPE_LRZIP ? " -fo " : (type == XARCHIVETYPE_RZIP ? " -kfo " : " -c > "), archive, "\"", NULL);
	g_free(archive);
	g_free(workfile);
	g_free(password_str);

	return command;
}

static compressor_t xa_gzip_et_al_compressor (XArchive *archive)
{
	compressor_t bzip_compressor = {FALSE, 1, 9, 9, 1};
	compressor_t bzip2_compressor = {FALSE, 1, 9, 9, 1};
	compressor_t bzip3_compressor = {FALSE, 1, 16, 511, 1};
	compressor_t compress_compressor = {FALSE, 9, 16, 16, 1};
	compressor_t gzip_compressor = {FALSE, 1, 6, 9, 1};
	compressor_t lrzip_compressor = {TRUE, 1, 7, 9, 1};
	compressor_t lz4_compressor = {FALSE, 1, 1, 9, 1};
	compressor_t lzma_compressor = {FALSE, 1, 7, 9, 1};
	compressor_t lzop_compressor = {FALSE, 1, 3, 9, 1};
	compressor_t rzip_compressor = {TRUE, 1, 6, 9, 1};
	compressor_t xz_compressor = {FALSE, 0, 6, 9, 1};
	compressor_t zstd_compressor = {FALSE, 1, 3, 19, 1};
	compressor_t void_compressor = {FALSE, 0, 0, 0, 0};

	switch (archive->type)
	{
		case XARCHIVETYPE_BZIP:
			return bzip_compressor;

		case XARCHIVETYPE_BZIP2:
			return bzip2_compressor;

		case XARCHIVETYPE_BZIP3:
			return bzip3_compressor;

		case XARCHIVETYPE_COMPRESS:
			return compress_compressor;

		case XARCHIVETYPE_GZIP:
			return gzip_compressor;

		case XARCHIVETYPE_LZIP:
		case XARCHIVETYPE_XZ:
			return xz_compressor;

		case XARCHIVETYPE_LRZIP:
			return lrzip_compressor;

		case XARCHIVETYPE_LZ4:
			return lz4_compressor;

		case XARCHIVETYPE_LZMA:
			return lzma_compressor;

		case XARCHIVETYPE_LZOP:
			return lzop_compressor;

		case XARCHIVETYPE_RZIP:
			return rzip_compressor;

		case XARCHIVETYPE_ZSTD:
			return zstd_compressor;

		default:
			return void_compressor;
	}
}

static void xa_gzip_et_al_can (XArchive *archive, gboolean can)
{
	archive->can_test = (can && !bzip && !compress && !rzip && (!zstd || zstd_can_test));
	archive->can_extract = can;
	archive->can_password = (can && lrzip && lrzip_can_password);
	archive->can_overwrite = can;
	archive->can_remove = can;

	/* only if archive is new and empty */
	archive->can_add = (can && archiver[archive->type].is_compressor);

	archive->can_compress = (can && archiver[archive->type].is_compressor && !mozlz4);
	archive->compressor = xa_gzip_et_al_compressor(archive);
	archive->compression = archive->compressor.preset;
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
		LINE_PEEK(9);
		/* heading? */
		if (line[peek] == 'c' || line[0] == 'c')   // genuine gzip or pigz
			return;
	}
	else if (archive->type == XARCHIVETYPE_LZIP)
	{
		/* heading? */
		if (*g_strchug(line) == 'u')
			return;
		else
		{
			idx0 = 1;
			idx1 = 0;
		}
	}
	else if (archive->type == XARCHIVETYPE_LZOP)
	{
		LINE_PEEK(12);
		/* heading? */
		if (line[peek] == 'c')
			return;
		else
			/* method */
			NEXT_ITEM(item[5]);
	}
	else if (archive->type == XARCHIVETYPE_XZ)
	{
		/* heading? */
		if (line[0] == 't')
			return;
		else if (line[0] == 'n')
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
		if (first_line)
		{
			first_line = FALSE;

			/* pre-v0.650 */
			if (line[0] != '\n')
			{
				gchar *colon = g_strrstr(line, ":\n");

				if (colon) *colon = '\n';   // remove colon

				goto filename;
			}
		}

		/* since v0.650 */
		IF_ITEM_LINE("File: ")
		{
filename:
			LAST_ITEM(filename);

			if (g_str_has_suffix(filename, ".lrz"))
				*(line - 5) = 0;

			filename = g_path_get_basename(filename);
			data_line = TRUE;
		}

		return;
	}

	line = g_strchug(line);

	IF_ITEM_LINE("lrzip version:")
	{
		NEXT_ITEM(item[6]);
		item[6] = g_strdup(item[6]);
	}
	/* pre-v0.650 */
	else IF_ITEM_LINE("Compression:")
		DUPE_ITEM(item[5]);
	/* since v0.650 */
	else IF_ITEM_LINE("Compression Method:")
		DUPE_ITEM(item[5]);
	else IF_ITEM_LINE("Decompressed file size:")
		DUPE_ITEM(item[0]);
	else IF_ITEM_LINE("Compressed file size:")
		DUPE_ITEM(item[1]);
	else IF_ITEM_LINE("Compression ratio:")
	{
		NEXT_ITEM(item[2]);

		/* since v0.650 */
		if (g_str_has_suffix(item[2], "x"))
			*((char *) item[2] + strlen(item[2]) - 1) = 0;
		else if (*(char *) item[2] == 'U')                 // Unavailable
			item[2] = "1";

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

static void xa_gzip_et_al_parse_zstd (gchar *line, XArchive *archive)
{
	static gchar *zstdframes;
	XEntry *entry;
	gchar *skipframes;
	char *pos;

	USE_PARSER;

	if (last_line)
		return;

	if (first_line)
		/* post-v1.3.2 (no header any more) */
		data_line = (strncmp(line, "===========================================", 43) != 0);

	if (!data_line)
	{
		first_line = FALSE;
		/* pre-v1.3.2 (end of header) */
		data_line = (strncmp(line, "Number of files listed:", 23) == 0);
		return;
	}

	if ((pos = strstr(line, first_line ? "\n" : "(1/1):\n")))
	{
		*pos = 0;

		LAST_ITEM(filename);

		if (g_str_has_suffix(filename, ".zst "))
			*(line - 5) = 0;

		filename = g_path_get_basename(filename);
	}
	else IF_ITEM_LINE("# Zstandard Frames:")
	{
		DUPE_ITEM(zstdframes);
		item[6] = g_strconcat(zstdframes, "/0", NULL);
	}
	else IF_ITEM_LINE("# Skippable Frames:")
	{
		NEXT_ITEM(skipframes);
		g_free(item[6]);
		item[6] = g_strconcat(zstdframes, "/", skipframes, NULL);
	}
	else IF_ITEM_LINE("Compressed Size:")
	{
		pos = strchr(line, '(');

		item[1] = (pos ? g_strdup(pos + 1) : pos);
	}
	else IF_ITEM_LINE("Decompressed Size:")
	{
		pos = strchr(line, '(');

		item[0] = (pos ? g_strdup(pos + 1) : pos);
	}
	else IF_ITEM_LINE("Ratio:")
	{
		NEXT_ITEM(item[2]);
		item[2] = g_strconcat(item[2], ":1", NULL);
	}
	else IF_ITEM_LINE("Check:")
	{
		DUPE_ITEM(item[5]);
		last_line = TRUE;
	}

	first_line = FALSE;

	if (!last_line)
		return;

	if (!item[0])
		item[0] = g_strdup("0");

	entry = xa_set_archive_entries_for_each_row(archive, filename, item);

	if (entry)
	{
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
	g_free(zstdframes);
}

static void xa_gzip_et_al_prepared_items (gchar *line, XArchive *archive)
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
	const gchar *decompfile = "xa-tmp.decompressed", *framefile = "xa-tmp.framed";
	gchar *password_str, *archive_path, *command, *workfile, buffer[4096];
	FILE *file, *wfile;
	struct stat st;
	guint i;

	if (!xa_create_working_directory(archive))
		return;

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

	if (archive->type == XARCHIVETYPE_LZ4 && TAGTYPE(archive->tag) == 'm')
	{
		int skip, offset;
		uint32_t uncompressed, blocksize;
		guint8 flg, bd, hc, endmark[4] = "\x00\x00\x00\x00";
		size_t bytes;

		file = fopen(archive->path[0], "r");

		if (!file)
			return;

		workfile = g_strconcat(archive->working_dir, "/", framefile, NULL);
		wfile = fopen(workfile, "w");

		if (!wfile)
		{
			fclose(file);
			g_free(workfile);
			return;
		}

		skip = (archive->tag == 'm' ? 8 : 16);   // magic

		/* original (uncompressed) data size */
		fseek(file, skip, SEEK_SET);

		if (fread(&uncompressed, sizeof(uncompressed), 1, file) != 1)
		{
			fclose(file);
			fclose(wfile);
			g_free(workfile);
			return;
		}

		/* create a lz4 frame header */

		fwrite(LZ4_MAGIC, 4, 1, wfile);

		flg = (0 << 7) + (1 << 6)   // version number
		               + (1 << 5)   // block independence flag
		               + (0 << 4)   // block checksum flag
		               + (0 << 3)   // content size flag
		               + (0 << 2)   // content checksum flag
		               + (0 << 1)   // reserved
		               + (0 << 0);  // dictionary id flag
		fwrite(&flg, 1, 1, wfile);

		if (le32toh(uncompressed) < 64 * 1024)          // 64 KB
		{
			bd = 0x40;
			hc = 0x82;
		}
		else if (le32toh(uncompressed) < 256 * 1024)    // 256 KB
		{
			bd = 0x50;
			hc = 0xfb;
		}
		else if (le32toh(uncompressed) < 1024 * 1024)   // 1 MB
		{
			bd = 0x60;
			hc = 0x51;
		}
		else                                            // 4 MB
		{
			bd = 0x70;
			hc = 0x73;
		}

		fwrite(&bd, 1, 1, wfile);   // block maximum size
		fwrite(&hc, 1, 1, wfile);   // header checksum

		offset = skip + 4;

		fseek(file, 0, SEEK_END);
		blocksize = htole32((uint32_t) (ftell(file) - offset));
		fwrite(&blocksize, sizeof(blocksize), 1, wfile);

		fseek(file, offset, SEEK_SET);

		/* copy lz4 data */
		while ((bytes = fread(buffer, 1, sizeof(buffer), file)) > 0)
			fwrite(buffer, 1, bytes, wfile);

		fwrite(endmark, sizeof(endmark), 1, wfile);

		fclose(wfile);
		fclose(file);

		archive->path[2] = g_strdup(workfile);
		archive->path[3] = xa_escape_bad_chars(workfile, ESCAPES);
		g_free(workfile);
	}
	else
	{
		archive->path[2] = g_strdup(archive->path[0]);
		archive->path[3] = g_strdup(archive->path[1]);
	}

	if (archive->has_password)
		if (!xa_check_password(archive))
			return;

	password_str = xa_gzip_et_al_password_str(archive->password, archive->type);
	archive_path = xa_quote_shell_command(archive->path[2], TRUE);

	archive->child_dir = g_strdup(archive->working_dir);
	command = g_strconcat("sh -c \"exec ", archiver[archive->type].program[0], " -d", password_str, " ", archive_path, lrzip ? " -fo " : (rzip ? " -kfo " : " -c > "), decompfile, "\"", NULL);
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
			archive->status = XARCHIVESTATUS_LIST;

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
		case XARCHIVETYPE_ZSTD:

			if (archive->type == XARCHIVETYPE_LRZIP)
			{
				if (archive->has_password)
					/* lrzip won't list content then, even if password has been given */
					break;

				first_line = TRUE;
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
			else if (archive->type == XARCHIVETYPE_ZSTD)
			{
				if (!zstd_can_list)
					break;

				/* items potentially not listed */
				item[0] = NULL;
				item[2] = NULL;

				first_line = TRUE;
				data_line = FALSE;
				last_line = FALSE;

				types[7] = G_TYPE_STRING;
				types[8] = G_TYPE_STRING;

				titles[2] = _("Compression ratio");
				titles[5] = _("Check Type");
				titles[6] = _("Data Frames/Skippable Frames");

				archive->columns = 10;
			}

			command = g_strconcat(archiver[archive->type].program[0], lrzip ? " -i" : " -l", xz ? " --robot " : (zstd ? "v " : " "), archive->path[3], NULL);

			if (archive->type == XARCHIVETYPE_LRZIP)
				archive->parse_output = xa_gzip_et_al_parse_lrzip;
			else if (archive->type == XARCHIVETYPE_ZSTD)
				archive->parse_output = xa_gzip_et_al_parse_zstd;
			else
				archive->parse_output = xa_gzip_et_al_parse_output;

			break;

		case XARCHIVETYPE_BZIP:
		case XARCHIVETYPE_BZIP2:
		case XARCHIVETYPE_BZIP3:
		case XARCHIVETYPE_COMPRESS:
		case XARCHIVETYPE_LZ4:
		case XARCHIVETYPE_LZMA:
		case XARCHIVETYPE_RZIP:
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
		archive->parse_output = xa_gzip_et_al_prepared_items;
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
	command = g_strconcat(archiver[archive->type].program[0], " -t", password_str, lrzip || lz4 ? " " : "v ", archive->path[3], NULL);
	g_free(password_str);

	xa_run_command(archive, command);
	g_free(command);
}

gboolean xa_gzip_et_al_extract (XArchive *archive, GSList *file_list)
{
	GString *files;
	gchar *command, *filename, *files_str, *out_dir, *out_file, *archive_path, *extraction_dir, *password_str;
	gboolean result;

	files = xa_quote_filenames(file_list, NULL, DIR_WITH_SLASH);

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

	archive_path = xa_quote_shell_command(archive->path[2], TRUE);
	extraction_dir = xa_quote_shell_command(archive->extraction_dir, FALSE);

	if (archive->do_overwrite || !g_file_test(out_file, G_FILE_TEST_EXISTS))
	{
		password_str = xa_gzip_et_al_password_str(archive->password, archive->type);
		command = g_strconcat("sh -c \"exec ", archiver[archive->type].program[0], " -d", password_str, " ", archive_path, lrzip ? " -fo " : (rzip ? " -kfo " : " -c > "), extraction_dir, "/", files_str, "\"", NULL);
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

void xa_gzip_et_al_add (XArchive *archive, GSList *file_list)
{
	GString *files;
	gchar *compression, *files_str, *archive_path, *move, *password_str, *args, *command;
	struct stat st;
	gboolean success;

	if (mozlz4)
	{
		gchar *file;

		if (!xa_create_working_directory(archive))
			return;

		archive->path[2] = g_strconcat(archive->working_dir, "/", "xa-tmp.lz4", NULL);

		file = g_strconcat(archive->child_dir, "/", file_list->data, NULL);
		stat(file, &st);
		g_free(file);
	}

	compression = g_strdup_printf("%hu", archive->compression);

	files = xa_quote_filenames(file_list, NULL, DIR_WITH_SLASH);
	files_str = xa_escape_bad_chars(files->str, "\"");
	archive_path = xa_quote_shell_command(archive->path[2] ? archive->path[2] : archive->path[0], TRUE);

	if (archive->do_remove)
		move = g_strconcat(" rm --", files_str, NULL);
	else
		move = g_strdup("");

	password_str = xa_gzip_et_al_password_str(archive->password, archive->type);

	if (lrzip || rzip)
		args = g_strconcat(lrzip ? " -fo " : " -kfo ", archive_path, " --", files_str, NULL);
	else
		args = g_strconcat(" -c", bzip ? "" : " --", files_str, " > ", archive_path, NULL);

	command = g_strconcat("sh -c \"exec ", archiver[archive->type].program[0], " -", compress || bzip3 ? "b " : (lrzip ? (*compression == '0' ? "n" : "L ") : ""), lrzip && (*compression == '0') ? "" : (mozlz4 ? "l" : compression), password_str, args, "\"", NULL);
	success = xa_run_command(archive, command);

	g_free(command);
	command = NULL;

	if (success && mozlz4)
	{
		FILE *in, *out;
		uint32_t size;
		size_t bytes;
		char buffer[4096];

		in = fopen(archive->path[2], "r");

		if (!in)
			goto finish;

		out = fopen(archive->path[0], "w");

		if (!out)
		{
			fclose(in);
			goto finish;
		}

		fwrite(MOZLZ4_MAGIC, 8, 1, out);
		size = htole32((uint32_t) st.st_size);
		fwrite(&size, sizeof(size), 1, out);

		if (fseek(in, 8, SEEK_SET) == 0)
		{
			/* copy lz4 data */
			while ((bytes = fread(buffer, 1, sizeof(buffer), in)) > 0)
				fwrite(buffer, bytes, 1, out);
		}

		fclose(out);
		fclose(in);
	}

	if (success)
	{
		command = g_strconcat("sh -c \"exec", move, "\"", NULL);
		archive->status = XARCHIVESTATUS_ADD;   // restore status
	}

finish:
	g_free(args);
	g_free(password_str);
	g_free(move);
	g_free(archive_path);
	g_free(files_str);
	g_string_free(files, TRUE);
	g_free(compression);

	if (command)
		xa_run_command(archive, command);

	g_free(command);
}
