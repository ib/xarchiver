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
}

void xa_gzip_et_al_ask (XArchive *archive)
{
	xa_gzip_et_al_can(archive, TRUE);
}

static void xa_gzip_et_al_parse_output (gchar *line, XArchive *archive)
{
	if (archive->type == XARCHIVETYPE_GZIP)
	{
		/* heading? */
		if (line[9] == 'c')
			return;
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

	/* compressed */
	NEXT_ITEM(item[1]);

	/* uncompressed */
	NEXT_ITEM(item[0]);
	archive->files_size = g_ascii_strtoull(item[0], NULL, 0);

	/* ratio */
	NEXT_ITEM(item[2]);

	if (archive->type != XARCHIVETYPE_XZ)
	{
		/* uncompressed_name */
		LAST_ITEM(filename);

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

void xa_gzip_et_al_open (XArchive *archive)
{
	gchar *filename = NULL;;
	gchar *_filename;
	gboolean result;
	gint len = 0;

	archive->delete =	delete[archive->type];
	archive->add = 		add[archive->type];
	archive->extract = 	extract[archive->type];

	if (g_str_has_suffix(archive->path[1],".tar.bz2") || g_str_has_suffix (archive->path[1],".tar.bz")
    	|| g_str_has_suffix ( archive->path[1] , ".tbz") || g_str_has_suffix (archive->path[1],".tbz2") )
	{
		archive->type = XARCHIVETYPE_TAR_BZ2;
		xa_open_tar_compressed_file(archive);
	}
	else if (g_str_has_suffix(archive->path[1],".tar.gz") || g_str_has_suffix (archive->path[1],".tgz"))
	{
		archive->type = XARCHIVETYPE_TAR_GZ;
		xa_open_tar_compressed_file(archive);
	}
	else if (g_str_has_suffix(archive->path[1],".tar.lzma") || g_str_has_suffix (archive->path[1],".tlz"))
	{
		archive->type = XARCHIVETYPE_TAR_LZMA;
		xa_open_tar_compressed_file(archive);
	}
	else if (g_str_has_suffix(archive->path[1],".tar.xz") || g_str_has_suffix (archive->path[1],".txz"))
	{
		archive->type = XARCHIVETYPE_TAR_XZ;
		xa_open_tar_compressed_file(archive);
	}
	else if (g_str_has_suffix(archive->path[1],".tar.lzop") ||
		g_str_has_suffix (archive->path[1],".tzo") ||
		g_str_has_suffix(archive->path[1],".tar.lzo"))
	{
		archive->type = XARCHIVETYPE_TAR_LZOP;
		xa_open_tar_compressed_file(archive);
	}
	else if (archive->type == XARCHIVETYPE_GZIP || archive->type == XARCHIVETYPE_LZOP || archive->type == XARCHIVETYPE_XZ)
	{
		const GType types[] = {GDK_TYPE_PIXBUF, G_TYPE_STRING, G_TYPE_UINT64, G_TYPE_UINT64, G_TYPE_STRING, G_TYPE_POINTER};
		const gchar *titles[] = {_("Original"), _("Compressed"), _("Ratio")};
		gchar *command;
		guint i;

		archive->parse_output = xa_gzip_et_al_parse_output;
		archive->files = 1;


		archive->columns = 6;
		archive->column_types = g_malloc0(sizeof(types));

		for (i = 0; i < archive->columns; i++)
			archive->column_types[i] = types[i];

		xa_create_liststore(archive, titles);

		command = g_strconcat(archiver[archive->type].program[0], " -l", archive->type == XARCHIVETYPE_XZ ? " --robot " : " ", archive->path[1], NULL);
		xa_spawn_async_process (archive,command);
		g_free (command);
	}
	else
	{
		const GType types[] = {GDK_TYPE_PIXBUF, G_TYPE_STRING, G_TYPE_UINT64, G_TYPE_UINT64, G_TYPE_POINTER};
		const gchar *titles[] = {_("Original"), _("Compressed")};
		struct stat my_stat;
		gchar *compressed = NULL;
		gchar *size, *command[2], *dot, *fullname;
		guint i;

		archive->files = 1;

		if (archive->type == XARCHIVETYPE_BZIP2)
			len = 4;
		else if (archive->type == XARCHIVETYPE_LZMA)
			len = 5;
		else if (archive->type == XARCHIVETYPE_XZ)
			len = 3;
		else if (archive->type == XARCHIVETYPE_LZOP)
			len = 4;

		archive->columns = 5;
		archive->column_types = g_malloc0(sizeof(types));

		for (i = 0; i < archive->columns; i++)
			archive->column_types[i] = types[i];

		xa_create_liststore(archive, titles);

		result = xa_create_working_directory(archive);
		if (!result)
			return;

		/* Let's copy the bzip2 file in the tmp dir */
		command[0] = g_strconcat("cp -f ",archive->path[1]," ",archive->working_dir,NULL);
		/* Let's get its compressed file size */
		stat (archive->path[1],&my_stat);
		compressed = g_strdup_printf("%" G_GUINT64_FORMAT, (guint64) my_stat.st_size);
		item[1] = compressed;

		/* Let's extract it */
		_filename = g_path_get_basename(archive->path[1]);
		if (_filename[0] == '.')
			command[1] = g_strconcat(archiver[archive->type].program[0], " -f -d ", archive->working_dir, "/", archive->path[1], NULL);
		else
			command[1] = g_strconcat(archiver[archive->type].program[0], " -f -d ", archive->working_dir, "/", _filename, NULL);

		xa_run_command(archive, command[0]);
		g_free(command[0]);

		xa_run_command(archive, command[1]);
		g_free(command[1]);

		/* and let's get its uncompressed file size */
		dot = strrchr(_filename,'.');
		if (_filename || G_LIKELY(dot))
		{
			filename = g_strndup(_filename,strlen(_filename) - len);
			fullname = g_strconcat(archive->working_dir, "/", filename, NULL);
		}
		else
		{
			fullname = g_strconcat(archive->working_dir, "/", archive->path[1], NULL);
			filename = g_strdup(archive->path[1]);
		}
		stat(fullname, &my_stat);
		g_free(fullname);
		size = g_strdup_printf("%" G_GUINT64_FORMAT, (guint64) my_stat.st_size);
		archive->files_size = my_stat.st_size;
		item[0] = size;

		g_free(filename);

		archive->parse_output = xa_gzip_et_al_globally_stored_entry;
		/* trigger pseudo-parser once */
		command[0] = g_strdup("sh -c echo");
		xa_spawn_async_process(archive, command[0]);
		g_free(command[0]);
	}
}

void xa_gzip_et_al_test (XArchive *archive)
{
	gchar *command;

	command = g_strconcat(archiver[archive->type].program[0], " -tv ", archive->path[1], NULL);

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
