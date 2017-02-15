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
#include "string_utils.h"
#include "support.h"

extern const gchar *sevenz;
extern void xa_reload_archive_content(XArchive *archive);
extern void xa_create_liststore ( XArchive *archive, gchar *columns_names[]);

void xa_open_7zip (XArchive *archive)
{
	jump_header = encrypted = last_line = FALSE;
	unsigned short int i = 0;

	gchar *command = g_strconcat(sevenz, " l ", archive->escaped_path, NULL);
	archive->can_sfx = archive->can_add = archive->can_extract = archive->can_test = TRUE;
	archive->can_solid = TRUE;
	archive->can_passwd = TRUE;
	archive->can_overwrite = TRUE;
	archive->can_full_path = TRUE;
	archive->can_update = TRUE;
	archive->files_size = 0;
	archive->nr_of_files = 0;
	archive->nc = 6;
	archive->parse_output = xa_get_7zip_line_content;
	xa_spawn_async_process (archive,command);
	g_free ( command );
	if ( archive->child_pid == 0 )
		return;

	GType types[]= {GDK_TYPE_PIXBUF,G_TYPE_STRING,G_TYPE_UINT64,G_TYPE_UINT64,G_TYPE_STRING,G_TYPE_STRING,G_TYPE_STRING,G_TYPE_POINTER};
	archive->column_types = g_malloc0(sizeof(types));
	for (i = 0; i < 8; i++)
		archive->column_types[i] = types[i];

	char *names[]= {(_("Original")),(_("Compressed")),(_("Attr")),(_("Time")),(_("Date")),NULL};
	xa_create_liststore (archive,names);
}

void xa_get_7zip_line_content (gchar *line, gpointer data)
{
	XArchive *archive = data;
	XEntry *entry;
	gchar *filename;
	gpointer item[5];
	gint linesize = 0,a = 0;

	if (last_line)
		return;

	if (jump_header == FALSE)
	{
		if (strncmp(line, "Method = ", 9) == 0 && strstr(line, "7zAES"))
			encrypted = TRUE;

		if ((line[0] == '-') && line[3])
		{
			jump_header = TRUE;
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
		entry->is_encrypted = encrypted;

	g_free(filename);
}

void xa_7zip_delete (XArchive *archive,GSList *names)
{
	gchar *command, *e_filename = NULL;
	GSList *list = NULL,*_names;
	GString *files = g_string_new("");

 	_names = names;
 	while (_names)
	{
		e_filename  = xa_escape_filename((gchar*)_names->data,"$'`\"\\!?* ()[]&|:;<>#");
		g_string_prepend (files,e_filename);
		g_string_prepend_c (files,' ');
		_names = _names->next;
	}
	g_slist_foreach(names,(GFunc)g_free,NULL);
	g_slist_free(names);
	command = g_strconcat(sevenz, " d ", archive->escaped_path, " ", files->str, NULL);
	g_string_free(files,TRUE);
	list = g_slist_append(list,command);

	xa_run_command (archive,list);
	if (archive->status == XA_ARCHIVESTATUS_DELETE)
		xa_reload_archive_content(archive);
}

void xa_7zip_add (XArchive *archive,GString *files,gchar *compression_string)
{
	GSList *list = NULL;
	gchar *command;

	if (archive->location_entry_path != NULL)
		archive->working_dir = g_strdup(archive->tmp);

	if (compression_string == NULL)
		compression_string = "5";
	if (archive->passwd != NULL)
		command = g_strconcat (sevenz,
								archive->update ? " u " : " a ",
								archive->add_solid ? "-ms=on " : "-ms=off ",
								"-p" , archive->passwd, " ",
								archive->escaped_path," ",
								"-mx=",compression_string,"",
								files->str,NULL);
	else
		command = g_strconcat (sevenz,
								archive->update ? " u " : " a ",
								archive->add_solid ? "-ms=on " : "-ms=off ",
								archive->escaped_path," ",
								"-mx=",compression_string,"",
								files->str,NULL);
	g_string_free(files,TRUE);
	list = g_slist_append(list,command);

	xa_run_command (archive,list);
	xa_reload_archive_content(archive);
}

gboolean xa_7zip_extract(XArchive *archive,GSList *files)
{
	gchar *command,*e_filename = NULL;
	GSList *list = NULL,*_files = NULL;
	GString *names = g_string_new("");
	gboolean result = FALSE;

	_files = files;
	while (_files)
	{
		e_filename  = xa_escape_filename((gchar*)_files->data,"$'`\"\\!?* ()[]&|:;<>#");
		g_string_prepend (names,e_filename);
		g_string_prepend_c (names,' ');
		_files = _files->next;
	}
	g_slist_foreach(_files,(GFunc)g_free,NULL);
	g_slist_free(_files);

	if (archive->passwd != NULL)
		command = g_strconcat (sevenz, archive->full_path ? " x" : " e",
								" -p",archive->passwd,
								archive->overwrite ? " -aoa" : " -aos",
								" -bd ",
								archive->escaped_path,names->str , " -o",archive->extraction_path,NULL);
	else
		command = g_strconcat (sevenz, archive->full_path ? " x" : " e",
								archive->overwrite ? " -aoa" : " -aos",
								" -bd ",
								archive->escaped_path,names->str , " -o",archive->extraction_path,NULL);
	g_string_free(names,TRUE);
	list = g_slist_append(list,command);

	result = xa_run_command (archive,list);
	return result;
}

void xa_7zip_test (XArchive *archive)
{
	gchar *command = NULL;
	GSList *list = NULL;

	archive->status = XA_ARCHIVESTATUS_TEST;
	if (archive->passwd != NULL)
		command = g_strconcat(sevenz, " t -p", archive->passwd, " ", archive->escaped_path, NULL);
	else
		command = g_strconcat(sevenz, " t ", archive->escaped_path, NULL);

	list = g_slist_append(list,command);
	xa_run_command (archive,list);
}
