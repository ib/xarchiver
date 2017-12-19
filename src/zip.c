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
#include "zip.h"
#include "main.h"
#include "string_utils.h"
#include "support.h"
#include "window.h"

void xa_zip_ask (XArchive *archive)
{
	gchar *sfx;

	sfx = g_find_program_in_path("unzipsfx");

	archive->can_test = TRUE;
	archive->can_extract = TRUE;
	archive->can_add = archiver[archive->type].is_compressor;
	archive->can_delete = archiver[archive->type].is_compressor;
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
	XEntry *entry = NULL;

	gchar *filename;
	gpointer item[9];
	unsigned int linesize,n,a;
	gboolean encrypted,dir;
	guint64 size;

	encrypted = dir = FALSE;
	if ((line[0] != '-') && (line[0] != 'd') && (line[0] != 'l') && (line[0] != '?'))
		return;

	linesize = strlen(line);

	/* permissions */
	for(n=0; n < linesize && line[n] == ' '; n++);
	a = n;
	for(; n < linesize && line[n] != ' '; n++);

	line[n]='\0';
	item[5] = line + a;
	if ( (line+a)[0] == 'd')
		dir = TRUE;
	n++;

	/* version */
	for(; n < linesize && line[n] == ' '; n++);
	a = n;
	for(; n < linesize && line[n] != ' '; n++);

	line[n] = '\0';
	item[8] = line + a;
	n++;

	/* OS */
	for(; n < linesize && line[n] == ' '; n++);
	a = n;
	for(; n < linesize && line[n] != ' '; n++);

	line[n]='\0';
	item[7] = line + a;
	n++;

	/* size */
	for(; n < linesize && line[n] == ' '; n++);
	a = n;
	for(; n < linesize && line[n] != ' '; n++);

	line[n]='\0';
	item[0] = line + a;
	size = g_ascii_strtoull(item[0], NULL, 0);
	n++;

	/* text/binary */
	for(; n < linesize && line[n] == ' '; n++);
	a = n;
	for(; n < linesize && line[n] != ' '; n++);

	line[n]='\0';
	if ((line+a)[0] == 'B' || (line+a)[0] == 'T')
	{
		archive->has_password = TRUE;
		encrypted = TRUE;
	}
	n++;

	/* compressed size */
	for(; n < linesize && line[n] == ' '; n++);
	a = n;
	for(; n < linesize && line[n] != ' '; n++);

	line[n]='\0';
	item[1] = line + a;
	n++;

	/* method */
	for(; n < linesize && line[n] == ' '; n++);
	a = n;
	for(; n < linesize && line[n] != ' '; n++);

	line[n]='\0';
	item[6] = line + a;
	n++;

	/* date */
	for(; n < linesize && line[n] == ' '; n++);
	a = n;
	for(; n < linesize && line[n] != ' '; n++);

	line[n]='\0';
	item[3] = line + a;
	n++;

	/* time */
	for(; n < linesize && line[n] == ' '; n++);
	a = n;
	for(; n < linesize && line[n] != ' '; n++);

	line[n]='\0';
	item[4] = line + a;
	n++;

	/* filename */
	line[linesize-1] = '\0';
	filename = line + n;

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
	                      archive->do_overwrite ? " -o" : " -n",
	                      archive->do_update ? " -ou" : "",
	                      archive->do_freshen ? " -of" : "",
	                      password_str, " ",
	                      archive->path[1], files->str,
	                      " -d ", archive->extraction_dir, NULL);
	g_free(password_str);
	g_string_free(files,TRUE);

	result = xa_run_command(archive, command);
	g_free(command);

	return result;
}

void xa_zip_add (XArchive *archive, GSList *file_list, gchar *compression)
{
	GString *files;
	gchar *password_str, *command;

	if (archive->location_path != NULL)
		archive->child_dir = g_strdup(archive->working_dir);

	if (!compression)
		compression = "6";

	files = xa_quote_filenames(file_list, NULL, TRUE);   // no escaping for adding!
	password_str = xa_zip_password_str(archive);
	command = g_strconcat(archiver[archive->type].program[1],
	                      archive->do_update ? " -ou" : "",
	                      archive->do_freshen ? " -of" : "",
	                      archive->do_move ? " -m" : "",
	                      " -", compression,
	                      password_str, " ",
	                      archive->path[1], files->str, NULL);
	g_free(password_str);
	g_string_free(files,TRUE);

	xa_run_command(archive, command);
	g_free(command);
}

void xa_zip_delete (XArchive *archive, GSList *file_list)
{
	GString *files;
	gchar *command;

	files = xa_quote_filenames(file_list, "*?[]", TRUE);
	command = g_strconcat(archiver[archive->type].program[1], " -d ", archive->path[1], files->str, NULL);
	g_string_free(files,TRUE);

	xa_run_command(archive, command);
	g_free(command);
}
