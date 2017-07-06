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
#include "tar.h"
#include "gzip_et_al.h"
#include "main.h"
#include "string_utils.h"
#include "support.h"
#include "window.h"

gboolean isTar (FILE *file)
{
	unsigned char magic[8];
	gboolean result;

	fseek(file, 0, SEEK_SET);

	if (fseek(file, 257, SEEK_CUR) != 0 ||
	    fread(magic, 1, sizeof(magic), file) != sizeof(magic))
	{
		fseek(file, 0, SEEK_SET);
		return FALSE;
	}

	result = (memcmp(magic, "ustar" "\x00" "00", sizeof(magic)) == 0 ||
	          memcmp(magic, "ustar" "  " "\x00", sizeof(magic)) == 0);

	if (!result &&
	    memcmp(magic, "\x00\x00\x00\x00\x00\x00\x00\x00", sizeof(magic)) == 0)
	{
		/* next block */
		if (fseek(file, 512, SEEK_SET) == 0 && fread(magic, 1, 1, file) == 1)
			result = (*magic != 0);
	}

	fseek(file, 0, SEEK_SET);

	return result;
}

static XArchiveType xa_tar_get_compressor_type (XArchive *archive)
{
	switch (archive->type)
	{
		case XARCHIVETYPE_TAR_BZ2:
			return XARCHIVETYPE_BZIP2;

		case XARCHIVETYPE_TAR_GZ:
			return XARCHIVETYPE_GZIP;

		case XARCHIVETYPE_TAR_LZ:
			return XARCHIVETYPE_LZIP;

		case XARCHIVETYPE_TAR_LZ4:
			return XARCHIVETYPE_LZ4;

		case XARCHIVETYPE_TAR_LZMA:
			return XARCHIVETYPE_LZMA;

		case XARCHIVETYPE_TAR_LZOP:
			return XARCHIVETYPE_LZOP;

		case XARCHIVETYPE_TAR_XZ:
			return XARCHIVETYPE_XZ;

		case XARCHIVETYPE_TAR_Z:
			return XARCHIVETYPE_COMPRESS;

		default:
			return XARCHIVETYPE_TAR;
	}
}

void xa_tar_ask (XArchive *archive)
{
	archive->can_extract = TRUE;
	archive->can_add = archiver[xa_tar_get_compressor_type(archive)].is_compressor;
	archive->can_delete = archiver[xa_tar_get_compressor_type(archive)].is_compressor;
	archive->can_full_path[0] = TRUE;
	archive->can_full_path[1] = archiver[xa_tar_get_compressor_type(archive)].is_compressor;
	archive->can_touch = TRUE;
	archive->can_overwrite = TRUE;
	archive->can_update[0] = TRUE;
	archive->can_update[1] = archiver[xa_tar_get_compressor_type(archive)].is_compressor;
	archive->can_move = archiver[xa_tar_get_compressor_type(archive)].is_compressor;

	/* this is solely for display with the archive properties */
	if (!archiver[archive->type].program[0])
	{
		archiver[archive->type].program[0] = g_strdup(archiver[xa_tar_get_compressor_type(archive)].program[0]);
		archiver[archive->type].program[1] = g_strdup(archiver[XARCHIVETYPE_TAR].program[0]);
	}
}

static void xa_tar_parse_output (gchar *line, XArchive *archive)
{
	XEntry *entry;
	gchar *filename;
	gpointer item[6];
	gint n = 0, a = 0 ,linesize = 0;

	linesize = strlen(line);

	/* Permissions */
	line[10] = '\0';
	item[4] = line;

	/* Owner */
	for(n=13; n < linesize; ++n)
		if(line[n] == ' ')
			break;
	line[n] = '\0';
	item[5] = line+11;

	/* Size */
	for(++n; n < linesize; ++n)
		if(line[n] >= '0' && line[n] <= '9')
			break;
	a = n;

	for(; n < linesize; ++n)
		if(line[n] == ' ')
			break;

	line[n] = '\0';
	item[1] = line + a;
	a = ++n;

	/* Date */
	for(; n < linesize; n++)
		if(line[n] == ' ')
			break;

	line[n] = '\0';
	item[2] = line + a;

	/* Time */
	a = ++n;
	for (; n < linesize; n++)
		if (line[n] == ' ')
			break;

	line[n] = '\0';
	item[3] = line + a;
	n++;
	line[linesize-1] = '\0';

	filename = line + n;

	/* Symbolic link */
	gchar *temp = g_strrstr (filename,"->");
	if (temp)
	{
		gint len = strlen(filename) - strlen(temp);
		item[0] = (filename +=3) + len;
		filename[strlen(filename) - strlen(temp)-1] = '\0';
	}
	else
		item[0] = NULL;

	if(line[0] == 'd')
	{
		/* Work around for gtar, which does not output / with directories */
		if(line[linesize-2] != '/')
			filename = g_strconcat(line + n, "/", NULL);
		else
			filename = g_strdup(line + n);
	}
	else
		filename = g_strdup(line + n);

	entry = xa_set_archive_entries_for_each_row(archive, filename, item);

	if (entry)
	{
		if (!entry->is_dir)
			archive->files++;

		archive->files_size += g_ascii_strtoull(item[1], NULL, 0);
	}

	g_free(filename);
}

void xa_tar_list (XArchive *archive)
{
	const GType types[] = {GDK_TYPE_PIXBUF, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_UINT64, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_POINTER};
	const gchar *titles[] = {_("Points to"), _("Original Size"), _("Date"), _("Time"), _("Permissions"), _("Owner/Group")};
	gchar *command;
	guint i;

	if (!archive->path[2])
		archive->path[2] = g_shell_quote(archive->path[0]);

	archive->files = 0;
	archive->files_size = 0;

	command = g_strconcat(archiver[XARCHIVETYPE_TAR].program[0], " -tvf ", archive->path[2], NULL);
	archive->parse_output = xa_tar_parse_output;
	xa_spawn_async_process(archive, command);
	g_free(command);

	archive->columns = 9;
	archive->size_column = 3;
	archive->column_types = g_malloc0(sizeof(types));

	for (i = 0; i < archive->columns; i++)
		archive->column_types[i] = types[i];

	xa_create_liststore(archive, titles);
}

/*
 * Note: tar lists '\' as '\\' while it extracts '\', i.e.
 * file names containing this character can't be handled entirely.
 */

gboolean xa_tar_extract (XArchive *archive, GSList *file_list)
{
	GString *files;
	gchar *extract_to, *command;
	gboolean result;

	if (archive->do_full_path)
		extract_to = g_strdup(archive->extraction_dir);
	else
	{
		if (!xa_create_working_directory(archive))
			return FALSE;

		extract_to = g_strconcat(archive->working_dir, "/xa-tmp.XXXXXX", NULL);

		if (!g_mkdtemp(extract_to))
		{
			g_free(extract_to);
			return FALSE;
		}
	}

	files = xa_quote_filenames(file_list, NULL, TRUE);
	command = g_strconcat(archiver[XARCHIVETYPE_TAR].program[0],
	                      " -x --no-recursion --no-wildcards",
	                      " -f ", archive->path[2],
	                      archive->do_touch ? " -m" : "",
	                      archive->do_overwrite ? "" : " -k",
	                      archive->do_update ? " --keep-newer-files" : "",
	                      " -C ", extract_to, files->str, NULL);

	result = xa_run_command(archive, command);
	g_free(command);

	/* collect all files that have been extracted to move them without full path */
	if (result && !archive->do_full_path)
	{
		GString *all_files = xa_collect_files_in_dir(extract_to);

		archive->child_dir = g_strdup(extract_to);
		command = g_strconcat("mv",
		                      archive->do_overwrite ? " -f" : " -n",
		                      archive->do_update ? " -fu" : "",
		                      all_files->str, " ", archive->extraction_dir, NULL);
		g_string_free(all_files, TRUE);

		result = xa_run_command(archive, command);
		g_free(command);

		g_free(archive->child_dir);
		archive->child_dir = NULL;
	}

	g_free(extract_to);
	g_string_free(files, TRUE);

	return result;
}

void xa_tar_add (XArchive *archive, GSList *file_list, gchar *compression)
{
	GString *files;
	gchar *command;

	if (archive->location_path != NULL)
		archive->child_dir = g_strdup(archive->working_dir);

	files = xa_quote_filenames(file_list, NULL, TRUE);

	if (!g_file_test(archive->path[0], G_FILE_TEST_EXISTS))
	{
		if (archive->type == XARCHIVETYPE_TAR)
			archive->path[2] = g_shell_quote(archive->path[0]);
		else
		{
			gchar *workfile;

			if (!xa_create_working_directory(archive))
			{
				g_string_free(files, TRUE);
				return;
			}

			workfile = g_strconcat(archive->working_dir, "/xa-tmp.tar", NULL);
			archive->path[2] = g_shell_quote(workfile);
			g_free(workfile);
		}

		command = g_strconcat(archiver[XARCHIVETYPE_TAR].program[0],
		                      " -c --no-recursion --no-wildcards",
		                      archive->do_move ? " --remove-files" : "",
		                      " -f ", archive->path[2], files->str, NULL);
	}
	else
		command = g_strconcat(archiver[XARCHIVETYPE_TAR].program[0],
		                      archive->do_update ? " -u" : " -r",
		                      " --no-recursion --no-wildcards",
		                      archive->do_move ? " --remove-files" : "",
		                      " -f ", archive->path[2], files->str, NULL);

	if (archive->type != XARCHIVETYPE_TAR)
	{
		xa_run_command(archive, command);
		g_free(command);

		command = xa_gzip_et_al_get_command(archiver[xa_tar_get_compressor_type(archive)].program[0], archive->path[2], archive->path[0]);
	}

	g_string_free(files, TRUE);

	xa_run_command(archive, command);
	g_free(command);
}

void xa_tar_delete (XArchive *archive, GSList *file_list)
{
	GString *files;
	gchar *command;

	files = xa_quote_filenames(file_list, NULL, TRUE);
	command = g_strconcat(archiver[XARCHIVETYPE_TAR].program[0], " --delete --no-recursion --no-wildcards -f ", archive->path[2], files->str, NULL);

	if (archive->type != XARCHIVETYPE_TAR)
	{
		xa_run_command(archive, command);
		g_free(command);

		command = xa_gzip_et_al_get_command(archiver[xa_tar_get_compressor_type(archive)].program[0], archive->path[2], archive->path[0]);
	}

	g_string_free(files, TRUE);

	xa_run_command(archive, command);
	g_free(command);
}
