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
#include "zip.h"
#include "date_utils.h"
#include "main.h"
#include "parser.h"
#include "string_utils.h"
#include "support.h"
#include "window.h"

void xa_zip_ask (XArchive *archive)
{
	gboolean read_only;
	gchar *sfx;
	compressor_t zip_compressor = {TRUE, 1, 6, 9, 1};

	read_only = (archive->tag == 'a' || archive->tag == 'x');   // apk, exe

	sfx = g_find_program_in_path("unzipsfx");

	archive->can_test = TRUE;
	archive->can_extract = TRUE;
	archive->can_add = (archiver[archive->type].is_compressor && !read_only);
	archive->can_delete = (archiver[archive->type].is_compressor && !read_only);
	archive->can_sfx = (sfx && archiver[archive->type].is_compressor);
	archive->can_password = TRUE;
	archive->can_full_path[0] = TRUE;
	archive->can_full_path[1] = archiver[archive->type].is_compressor;
	archive->can_touch = TRUE;
	archive->can_overwrite = TRUE;
	archive->can_update[0] = TRUE;
	archive->can_update[1] = archiver[archive->type].is_compressor;
	archive->can_freshen[0] = TRUE;
	archive->can_freshen[1] = archiver[archive->type].is_compressor;
	archive->can_move = archiver[archive->type].is_compressor;
	archive->can_compress = archiver[archive->type].is_compressor;
	archive->compressor = zip_compressor;
	archive->compression = archive->compressor.preset;

	g_free(sfx);
}

static gchar *xa_zip_password_str (XArchive *archive)
{
	if (archive->password)
		return g_strconcat(" -P", archive->password, NULL);
	else
		return g_strdup("");
}

static void xa_zip_parse_output (gchar *line, XArchive *archive)
{
	XEntry *entry;
	gpointer item[9];
	gchar *filename, *attr;
	gboolean dir, encrypted;
	guint64 size;

	USE_PARSER;

	if (line[0] != '-' && line[0] != 'd' && line[0] != 'l' && line[0] != '?')
	{
		/*
		 * Unzip can compensate a central directory length reported as too long,
		 * but then exits with status (aka error level) 2, which is why we accept
		 * this exit status as okay in this case.
		 */
		if (strcmp(line, "  zipfile?).  Compensating...\n") == 0)
			archive->exitstatus_ok = 2;

		return;
	}

	/* permissions */
	NEXT_ITEM(item[5]);

	dir = (*(char *) item[5] == 'd');

	/* version */
	NEXT_ITEM(item[8]);

	/* OS */
	NEXT_ITEM(item[7]);

	/* size */
	NEXT_ITEM(item[0]);
	size = g_ascii_strtoull(item[0], NULL, 0);

	/* internal file attributes */
	NEXT_ITEM(attr);

	encrypted = (*attr == 'B' || *attr == 'T');

	if (encrypted)
		archive->has_password = TRUE;

	/* compressed size */
	NEXT_ITEM(item[1]);

	/* method */
	NEXT_ITEM(item[6]);

	/* date */
	NEXT_ITEM(item[3]);
	item[3] = date_YY_MMM_DD(item[3]);

	/* time */
	NEXT_ITEM(item[4]);

	/* name */
	LAST_ITEM(filename);

	/* saving */
	if (size)
		item[2] = g_strdup_printf("%.1f%%", 100.0 - 100.0 * g_ascii_strtoull(item[1], NULL, 0) / size);
	else
		item[2] = g_strdup("-");

	entry = xa_set_archive_entries_for_each_row(archive, filename, item);

	if (entry)
	{
		if (dir)
			entry->is_dir = TRUE;

		entry->is_encrypted = encrypted;

		if (!entry->is_dir)
			archive->files++;

		archive->files_size += size;
	}

	g_free(item[2]);
}

void xa_zip_list (XArchive *archive)
{
	const GType types[] = {GDK_TYPE_PIXBUF, G_TYPE_STRING, G_TYPE_UINT64, G_TYPE_UINT64, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_POINTER};
	const gchar *titles[] = {_("Original Size"), _("Compressed"), _("Saving"), _("Date"), _("Time"), _("Permissions"), _("Method"), _("OS"), _("Version")};
	guint i;

	gchar *command = g_strconcat(archiver[archive->type].program[0], " -Z -l ", archive->path[1], NULL);
	archive->files_size  = 0;
	archive->files = 0;
	archive->parse_output = xa_zip_parse_output;
	xa_spawn_async_process (archive,command);
	g_free ( command );

	archive->columns = 12;
	archive->size_column = 2;
	archive->column_types = g_malloc0(sizeof(types));

	for (i = 0; i < archive->columns; i++)
		archive->column_types[i] = types[i];

	xa_create_liststore(archive, titles);
}

void xa_zip_test (XArchive *archive)
{
	gchar *password_str, *command;

	password_str = xa_zip_password_str(archive);
	command = g_strconcat(archiver[archive->type].program[0], " -t", password_str, " ", archive->path[1], NULL);
	g_free(password_str);

	xa_run_command(archive, command);
	g_free(command);
}

gboolean xa_zip_extract (XArchive *archive, GSList *file_list)
{
	GString *files;
	gchar *password_str, *command;
	gboolean result;

	files = xa_quote_filenames(file_list, "*?[]", TRUE);
	password_str = xa_zip_password_str(archive);
	command = g_strconcat(archiver[archive->type].program[0],
	                      archive->do_full_path ? "" : " -j",
	                      archive->do_touch ? " -DD" : "",
	                      archive->do_overwrite ? " -o" : (archive->do_update ? " -ou" : (archive->do_freshen ? " -of" : " -n")),
	                      password_str, " ",
	                      archive->path[1], files->str,
	                      " -d ", archive->extraction_dir, NULL);
	g_free(password_str);
	g_string_free(files,TRUE);

	result = xa_run_command(archive, command);
	g_free(command);

	return result;
}

void xa_zip_add (XArchive *archive, GSList *file_list)
{
	gboolean epub;
	GString *files;
	gchar *compression, *password_str, *command;

	compression = g_strdup_printf("%hu", archive->compression);

	epub = (archive->tag == 'e');

	/* epub requires mimetype to be uncompressed, unencrypted, and the first file in the archive */
	if (epub)
	{
		GSList *flist, *fname = NULL;

		flist = file_list;

		while (flist)
		{
			if (strcmp(flist->data, "mimetype") == 0)
			{
				fname = flist;
				break;
			}

			flist = flist->next;
		}

		if (fname)
		{
			command = g_strconcat(archiver[archive->type].program[1], " -q",
			                      archive->do_update ? " -ou" : (archive->do_freshen ? " -of" : ""),
			                      archive->do_move ? " -m" : "",
			                      " -0X ",
			                      archive->path[1], " mimetype", NULL);
			xa_run_command(archive, command);
			g_free(command);

			archive->status = XARCHIVESTATUS_ADD;   // preserve status

			file_list = g_slist_remove(file_list, fname);
		}
	}

	files = xa_quote_filenames(file_list, NULL, TRUE);   // no escaping for adding!
	password_str = xa_zip_password_str(archive);
	command = g_strconcat(archiver[archive->type].program[1], " -q",
	                      archive->do_update ? " -ou" : (archive->do_freshen ? " -of" : ""),
	                      archive->do_move ? " -m" : "",
	                      " -", compression,
	                      epub ? "XD" : "",
	                      password_str, " ",
	                      archive->path[1], " --", files->str, NULL);
	g_free(password_str);
	g_string_free(files,TRUE);
	g_free(compression);

	xa_run_command(archive, command);
	g_free(command);
}

void xa_zip_delete (XArchive *archive, GSList *file_list)
{
	GString *files;
	gchar *command;

	files = xa_quote_filenames(file_list, "*?[]", TRUE);
	command = g_strconcat(archiver[archive->type].program[1], " -d ", archive->path[1], " --", files->str, NULL);
	g_string_free(files,TRUE);

	xa_run_command(archive, command);
	g_free(command);
}
