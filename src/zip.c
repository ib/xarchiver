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
	archive->can_overwrite = TRUE;
	archive->can_full_path = TRUE;
	archive->can_freshen = TRUE;
	archive->can_update = TRUE;
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
	gpointer item[8];
	unsigned short int i = 0;
	unsigned int linesize,n,a;
	gboolean encrypted,dir;

	encrypted = dir = FALSE;
	if ((line[0] != 'd') && (line[0] != '-') && (line[0] != '?'))
		return;

	archive->files++;
	linesize = strlen(line);

	/* permissions */
	for(n=0; n < linesize && line[n] == ' '; n++);
	a = n;
	for(; n < linesize && line[n] != ' '; n++);

	line[n]='\0';
	item[i] = line + a;
	if ( (line+a)[0] == 'd')
		dir = TRUE;
	i++;
	n++;

	/* version */
	for(; n < linesize && line[n] == ' '; n++);
	a = n;
	for(; n < linesize && line[n] != ' '; n++);

	line[n] = '\0';
	item[i] = line + a;
	i++;
	n++;

	/* OS */
	for(; n < linesize && line[n] == ' '; n++);
	a = n;
	for(; n < linesize && line[n] != ' '; n++);

	line[n]='\0';
	item[i] = line + a;
	i++;
	n++;

	/* size */
	for(; n < linesize && line[n] == ' '; n++);
	a = n;
	for(; n < linesize && line[n] != ' '; n++);

	line[n]='\0';
	item[i] = line + a;
	archive->files_size += g_ascii_strtoull(item[i],NULL,0);
	i++;
	n++;

	/* tx/bx */
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
	item[i] = line + a;
	i++;
	n++;

	/* method */
	for(; n < linesize && line[n] == ' '; n++);
	a = n;
	for(; n < linesize && line[n] != ' '; n++);

	line[n]='\0';
	item[i] = line + a;
	i++;
	n++;

	/* date */
	for(; n < linesize && line[n] == ' '; n++);
	a = n;
	for(; n < linesize && line[n] != ' '; n++);

	line[n]='\0';
	item[i] = line + a;
	i++;
	n++;

	/* time */
	for(; n < linesize && line[n] == ' '; n++);
	a = n;
	for(; n < linesize && line[n] != ' '; n++);

	line[n]='\0';
	item[i] = line + a;
	n++;

	/* filename */
	line[linesize-1] = '\0';
	filename = line + n;

	entry = xa_set_archive_entries_for_each_row (archive,filename,item);
	if (entry != NULL)
	{
		if (dir)
			 entry->is_dir = TRUE;
		entry->is_encrypted = encrypted;
	}
}

void xa_zip_open (XArchive *archive)
{
	const GType types[] = {GDK_TYPE_PIXBUF, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_UINT64, G_TYPE_UINT64, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_POINTER};
	const gchar *titles[] = {_("Permissions"), _("Version"), _("OS"), _("Original"), _("Compressed"), _("Method"), _("Date"), _("Time")};
	guint i;

	gchar *command = g_strconcat(archiver[archive->type].program[0], " -Z -l ", archive->path[1], NULL);
	archive->files_size  = 0;
	archive->files = 0;
	archive->parse_output = xa_zip_parse_output;
	xa_spawn_async_process (archive,command);
	g_free ( command );

	archive->columns = 11;
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
	                      archive->do_overwrite ? " -o" : " -n",
	                      archive->do_full_path ? "" : " -j",
	                      archive->do_freshen ? " -f" : "",
	                      archive->do_update ? " -u" : "",
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
	                      archive->do_freshen ? " -f" : "",
	                      archive->do_update ? " -u" : "",
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
