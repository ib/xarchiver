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
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include "gzip_et_al.h"
#include "main.h"
#include "parser.h"
#include "string_utils.h"
#include "support.h"
#include "tar.h"
#include "window.h"

static gpointer item[3];
static gchar *filename;

static void xa_gzip_et_al_can (XArchive *archive, gboolean can)
{
	archive->can_test = can;
	archive->can_extract = can;

	/* only if archive is new and empty */
	archive->can_add = (can && archiver[archive->type].is_compressor);
}

void xa_gzip_et_al_ask (XArchive *archive)
{
	xa_gzip_et_al_can(archive, TRUE);
}

static void xa_gzip_et_al_parse_output (gchar *line, XArchive *archive)
{
	guint idx0 = 0, idx1 = 1;

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
			SKIP_ITEM;
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
			SKIP_ITEM;
			/* number of blocks */
			SKIP_ITEM;
		}
	}
	else
		return;

	/* compressed (uncompressed for lzip) */
	NEXT_ITEM(item[idx1]);

	/* uncompressed (compressed for lzip) */
	NEXT_ITEM(item[idx0]);
	archive->files_size = g_ascii_strtoull(item[0], NULL, 0);

	/* ratio */
	NEXT_ITEM(item[2]);

	if (archive->type != XARCHIVETYPE_XZ)
	{
		/* uncompressed_name */
		LAST_ITEM(filename);

		if ((archive->type == XARCHIVETYPE_LZIP) && g_str_has_suffix(filename, ".lz"))
			*(line - 4) = 0;

		filename = g_path_get_basename(filename);
	}

	xa_set_archive_entries_for_each_row(archive, filename, item);
	g_free(filename);
}

static void xa_gzip_et_al_globally_stored_entry (gchar *line, XArchive *archive)
{
	gchar *filename;
	char *dot;

	filename = g_path_get_basename(archive->path[0]);
	dot = strrchr(filename, '.');

	if (dot)
		*dot = 0;

	xa_set_archive_entries_for_each_row(archive, filename, item);

	g_free(item[0]);
	g_free(item[1]);
	g_free(item[2]);
	g_free(filename);
}

void xa_gzip_et_al_list (XArchive *archive)
{
	const GType types[] = {GDK_TYPE_PIXBUF, G_TYPE_STRING, G_TYPE_UINT64, G_TYPE_UINT64, G_TYPE_STRING, G_TYPE_POINTER};
	const gchar *titles[] = {_("Original"), _("Compressed"), _("Ratio")};
	const gchar *decompfile = "xa-tmp.decompressed";
	gchar *archive_path, *command, *workfile;
	FILE *file;
	guint i;

	if (!xa_create_working_directory(archive))
		return;

	archive_path = xa_quote_shell_command(archive->path[0], TRUE);

	archive->child_dir = g_strdup(archive->working_dir);
	command = g_strconcat("sh -c \"", archiver[archive->type].program[0], " -d ", archive_path, " -c > ", decompfile, "\"", NULL);
	xa_run_command(archive, command);
	g_free(command);

	g_free(archive->child_dir);
	archive->child_dir = NULL;

	g_free(archive_path);

	workfile = g_strconcat(archive->working_dir, "/", decompfile, NULL);

	/* check if uncompressed file is tar archive */

	file = fopen(workfile, "r");

	if (file)
	{
		gboolean is_tar;

		is_tar = isTar(file);
		fclose(file);

		if (is_tar && archiver[XARCHIVETYPE_TAR].program[0])
		{
			switch (archive->type)
			{
				case XARCHIVETYPE_BZIP2:
					archive->type = XARCHIVETYPE_TAR_BZ2;
					break;

				case XARCHIVETYPE_GZIP:
					archive->type = XARCHIVETYPE_TAR_GZ;
					break;

				case XARCHIVETYPE_LZ4:
					archive->type = XARCHIVETYPE_TAR_LZ4;
					break;

				case XARCHIVETYPE_LZIP:
					archive->type = XARCHIVETYPE_TAR_LZ;
					break;

				case XARCHIVETYPE_LZMA:
					archive->type = XARCHIVETYPE_TAR_LZMA;
					break;

				case XARCHIVETYPE_LZOP:
					archive->type = XARCHIVETYPE_TAR_LZOP;
					break;

				case XARCHIVETYPE_XZ:
					archive->type = XARCHIVETYPE_TAR_XZ;
					break;

				default:
					return;
			}

			archive->path[2] = g_shell_quote(workfile);
			g_free(workfile);

			xa_gzip_et_al_can(archive, FALSE);

			archive->ask = ask[XARCHIVETYPE_TAR];
			archive->list = list[XARCHIVETYPE_TAR];
			archive->test = test[XARCHIVETYPE_TAR];
			archive->extract = extract[XARCHIVETYPE_TAR];
			archive->add = add[XARCHIVETYPE_TAR];
			archive->delete = delete[XARCHIVETYPE_TAR];

			(*archive->ask)(archive);
			(*archive->list)(archive);

			return;
		}
	}

	/* continue listing gzip et al. archive type */

	archive->files = 1;
	archive->can_add = FALSE;

	switch (archive->type)
	{
		case XARCHIVETYPE_GZIP:
		case XARCHIVETYPE_LZIP:
		case XARCHIVETYPE_LZOP:
		case XARCHIVETYPE_XZ:
			command = g_strconcat(archiver[archive->type].program[0], " -l", archive->type == XARCHIVETYPE_XZ ? " --robot " : " ", archive->path[1], NULL);
			archive->parse_output = xa_gzip_et_al_parse_output;
			break;

		case XARCHIVETYPE_BZIP2:
		case XARCHIVETYPE_LZ4:
		case XARCHIVETYPE_LZMA:
		{
			struct stat st;
			off_t compressed;

			/* compressed */
			stat(archive->path[0], &st);
			compressed = st.st_size;
			item[1] = g_strdup_printf("%" G_GUINT64_FORMAT, (guint64) compressed);

			/* uncompressed */
			stat(workfile, &st);
			archive->files_size = (guint64) st.st_size;
			item[0] = g_strdup_printf("%" G_GUINT64_FORMAT, archive->files_size);

			/* ratio */
			item[2] = g_strdup_printf("%2.1f%%", 100.0 - 100.0 * compressed / st.st_size);

			/* trigger pseudo-parser once */
			command = g_strdup("sh -c echo");
			archive->parse_output = xa_gzip_et_al_globally_stored_entry;

			break;
		}

		default:
			return;
	}

	g_free(workfile);

	xa_spawn_async_process(archive, command);
	g_free(command);

	archive->columns = 6;
	archive->column_types = g_malloc0(sizeof(types));

	for (i = 0; i < archive->columns; i++)
		archive->column_types[i] = types[i];

	xa_create_liststore(archive, titles);
}

void xa_gzip_et_al_test (XArchive *archive)
{
	gchar *command;

	command = g_strconcat(archiver[archive->type].program[0], " -t", archive->type == XARCHIVETYPE_LZ4 ? " " : "v ", archive->path[1], NULL);

	xa_run_command(archive, command);
	g_free(command);
}

gboolean xa_gzip_et_al_extract (XArchive *archive, GSList *file_list)
{
	GString *files;
	gchar *command, *files_str, *archive_path, *extraction_dir;
	gboolean result;

	files = xa_quote_filenames(file_list, NULL, TRUE);

	if (*files->str)
		files_str = xa_quote_shell_command(files->str + 1, FALSE);
	else
	{
		gchar *filename;
		char *dot;

		filename = g_path_get_basename(archive->path[0]);
		dot = strrchr(filename, '.');

		if (dot)
			*dot = 0;

		files_str = xa_quote_shell_command(filename, TRUE);
		g_free(filename);
	}

	archive_path = xa_quote_shell_command(archive->path[0], TRUE);
	extraction_dir = xa_quote_shell_command(archive->extraction_dir, FALSE);

	command = g_strconcat("sh -c \"", archiver[archive->type].program[0], " -d ", archive_path, " -c > ", extraction_dir, "/", files_str, "\"", NULL);

	g_free(extraction_dir);
	g_free(archive_path);
	g_free(files_str);
	g_string_free(files, TRUE);

	result = xa_run_command(archive, command);
	g_free(command);

	return result;
}

void xa_gzip_et_al_add (XArchive *archive, GSList *file_list, gchar *compression)
{
	GString *files;
	gchar *files_str, *archive_path, *command;

	if (archive->location_path != NULL)
		archive->child_dir = g_strdup(archive->working_dir);

	if (!compression)
	{
		switch (archive->type)
		{
			case XARCHIVETYPE_BZIP2:
				compression = "9";
				break;

			case XARCHIVETYPE_GZIP:
			case XARCHIVETYPE_LZIP:
			case XARCHIVETYPE_XZ:
				compression = "6";
				break;

			case XARCHIVETYPE_LZ4:
				compression = "1";
				break;

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

	command = g_strconcat("sh -c \"", archiver[archive->type].program[0], " -", compression, files_str, " -c > ", archive_path, "\"", NULL);

	g_free(archive_path);
	g_free(files_str);
	g_string_free(files, TRUE);

	xa_run_command(archive, command);
	g_free(command);
}
