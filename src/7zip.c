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
#include "main.h"
#include "string_utils.h"
#include "support.h"
#include "window.h"

static gboolean data_line, encrypted, last_line;

void xa_7zip_ask (XArchive *archive)
{
	archive->can_test = TRUE;
	archive->can_extract = TRUE;
	archive->can_add = TRUE;
	archive->can_delete = TRUE;
	archive->can_sfx = TRUE;
	archive->can_passwd = TRUE;
	archive->can_overwrite = TRUE;
	archive->can_full_path = TRUE;
	archive->can_update = TRUE;
	archive->can_solid = TRUE;
}

static gchar *xa_7zip_passwd_str (XArchive *archive)
{
	if (archive->passwd)
		return g_strconcat(" -p", archive->passwd, NULL);
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
			encrypted = TRUE;

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
	archive->nr_of_files++;

	/* Date */
	line[10] = '\0';
	item[4] = line;

	/* Time */
	line[19] = '\0';
	item[3] = line + 11;

	/* Permissions */
	line[25] = '\0';
	item[2] = line + 20;

	dir = (*(char *) item[2] == 'D');

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

void xa_7zip_open (XArchive *archive)
{
	guint i;

	data_line = FALSE;
	last_line = FALSE;
	encrypted = FALSE;
	gchar *command = g_strconcat(archiver[archive->type].program[0], " l ", archive->path[1], NULL);
	archive->files_size = 0;
	archive->nr_of_files = 0;
	archive->nc = 6;
	archive->parse_output = xa_7zip_parse_output;
	xa_spawn_async_process (archive,command);
	g_free ( command );

	GType types[]= {GDK_TYPE_PIXBUF,G_TYPE_STRING,G_TYPE_UINT64,G_TYPE_UINT64,G_TYPE_STRING,G_TYPE_STRING,G_TYPE_STRING,G_TYPE_POINTER};
	archive->column_types = g_malloc0(sizeof(types));
	for (i = 0; i < 8; i++)
		archive->column_types[i] = types[i];

	char *names[]= {(_("Original")),(_("Compressed")),(_("Attr")),(_("Time")),(_("Date")),NULL};
	xa_create_liststore (archive,names);
}

void xa_7zip_test (XArchive *archive)
{
	gchar *passwd_str, *command;
	GSList *list = NULL;

	passwd_str = xa_7zip_passwd_str(archive);
	command = g_strconcat(archiver[archive->type].program[0], " t", passwd_str, " -bd -y ", archive->path[1], NULL);
	g_free(passwd_str);

	list = g_slist_append(list,command);
	xa_run_command (archive,list);
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
	gchar *passwd_str, *command;
	GSList *list = NULL;

	files = xa_quote_filenames(file_list, NULL, TRUE);
	passwd_str = xa_7zip_passwd_str(archive);
	command = g_strconcat(archiver[archive->type].program[0],
	                      archive->do_full_path ? " x" : " e",
	                      archive->do_overwrite ? " -aoa" : " -aos",
	                      passwd_str, " -bd -spd -y ",
	                      archive->path[1], files->str,
	                      " -o", archive->extraction_path, NULL);
	g_free(passwd_str);
	g_string_free(files,TRUE);
	list = g_slist_append(list,command);

	return xa_run_command(archive, list);
}

void xa_7zip_add (XArchive *archive, GSList *file_list, gchar *compression)
{
	GString *files;
	gchar *passwd_str, *command;
	GSList *list = NULL;

	if (archive->location_entry_path != NULL)
		archive->working_dir = g_strdup(archive->tmp);

	if (!compression)
		compression = "5";

	files = xa_quote_filenames(file_list, NULL, TRUE);
	passwd_str = xa_7zip_passwd_str(archive);
	command = g_strconcat(archiver[archive->type].program[0],
	                      archive->do_update ? " u" : " a",
	                      " -ms=", archive->do_solid ? "on" : "off",
	                      " -mx=", compression,
	                      passwd_str, " -bd -spd -y ",
	                      archive->path[1], files->str, NULL);
	g_free(passwd_str);
	g_string_free(files,TRUE);
	list = g_slist_append(list,command);

	xa_run_command (archive,list);
}

void xa_7zip_delete (XArchive *archive, GSList *file_list)
{
	GString *files;
	gchar *passwd_str, *command;
	GSList *list = NULL;

	files = xa_quote_filenames(file_list, NULL, TRUE);
	passwd_str = xa_7zip_passwd_str(archive);
	command = g_strconcat(archiver[archive->type].program[0], " d", passwd_str, " -bd -spd -y ", archive->path[1], files->str, NULL);
	g_free(passwd_str);
	g_string_free(files,TRUE);
	list = g_slist_append(list,command);

	xa_run_command (archive,list);
}
