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

#include "config.h"
#include "arj.h"
#include <unistd.h>

extern void xa_reload_archive_content(XArchive *archive);
extern void xa_create_liststore (XArchive *archive, gchar *columns_names[]);

void xa_open_arj (XArchive *archive)
{
	unsigned short int i;
    jump_header = encrypted = last_line = FALSE;
	arj_line = 0;
	gchar *command = g_strconcat ("arj v ",archive->escaped_path, NULL);
	archive->has_sfx = archive->has_properties = archive->can_add = archive->can_extract = archive->has_test = TRUE;
	archive->dummy_size = 0;
	archive->nr_of_files = 0;
	archive->nc = 8;
	archive->format ="ARJ";
	archive->parse_output = xa_get_arj_line_content;
	xa_spawn_async_process (archive,command);
	g_free (command);
	if (archive->child_pid == 0)
		return;

	GType types[]= {GDK_TYPE_PIXBUF,G_TYPE_STRING,G_TYPE_UINT64,G_TYPE_UINT64,G_TYPE_STRING,G_TYPE_STRING,G_TYPE_STRING,G_TYPE_STRING,G_TYPE_STRING,G_TYPE_POINTER};
	archive->column_types = g_malloc0(sizeof(types));
	for (i = 0; i < 10; i++)
		archive->column_types[i] = types[i];

	char *names[]= {(_("Original")),(_("Compressed")),(_("Ratio")),(_("Date")),(_("Time")),(_("Attributes")),("GUA"),NULL};
	xa_create_liststore (archive,names);
}

void xa_get_arj_line_content (gchar *line, gpointer data)
{
	XArchive *archive = data;
	XEntry *entry;
	gpointer item[7];
	unsigned int linesize,n,a;
	static gchar *filename;

	if (last_line || strstr(line,"HardLink"))
		return;

	if (arj_line == 3)
	{
		arj_line++;
		return;
	}
	if (arj_line == 4)
	{
		arj_line = 1;	
		return;
	}

	if (jump_header == FALSE)
	{
		if (line[0] == '-')
		{
			jump_header = TRUE;
			arj_line = 1;
			return;
		}
		return;
	}
	if (arj_line == 1)
	{
		linesize = strlen(line);
		if(line[0] == '*')
		{
			archive->has_passwd = TRUE;
			encrypted = TRUE;
		}
		else if (line[0] == '-')
		{
			last_line = TRUE;
			return;
		}
		line[linesize - 1] = '\0';
		filename = g_strdup(line+5);
		archive->nr_of_files++;
		arj_line++;
		return;
	}
	else if (arj_line == 2)
	{
		linesize = strlen(line);
		/* Size */
		for(n=0; n < linesize && line[n] == ' '; n++);
		a = n;
		for(; n < linesize && line[n] != ' '; n++);
		line[n]='\0';
		item[0] = line + a;
		archive->dummy_size += g_ascii_strtoull(item[0],NULL,0);
		n++;

		/* Compressed */
		for(; n < linesize && line[n] == ' '; n++);
		a = n;
		for(; n < linesize && line[n] != ' '; n++);
		line[n]='\0';
		item[1] = line + a;
		n++;

		/* Ratio */
    	line[40] = '\0';
    	item[2] = line + 35;

		/* Date */
		line[49] = '\0';
		item[3] = line + 41;

		/* Time */
		line[58] = '\0';
		item[4] = line + 50;

		/* Permissions */
		line[69] = '\0';
		item[5] = line + 59;

		/* GUA */
		line[73] = '\0';
		item[6] = line + 70;

		/* BPMGS */
		line[78] = '\0';
		encrypted = (g_ascii_strcasecmp (line+76, "11") == 0);
		entry = xa_set_archive_entries_for_each_row (archive,filename,item);
		if (entry != NULL)
			entry->is_encrypted	= encrypted;
		g_free(filename);
		arj_line++;
	}
}

void xa_arj_delete (XArchive *archive,GSList *names)
{
	gchar *command,*e_filename = NULL;
	GSList *list = NULL;
	GSList *_names;
	GString *files = g_string_new("");

 	_names = names;
 	while (_names)
	{
		e_filename  = xa_escape_filename((gchar*)_names->data,"$'`\"\\!?* ()[]&|:;<>#");
		g_string_prepend   (files,e_filename);
		g_string_prepend_c (files,' ');
		_names = _names->next;
	}
	g_slist_foreach(names,(GFunc)g_free,NULL);
	g_slist_free(names);

	command = g_strconcat ("arj d ",archive->escaped_path," ",files->str,NULL);
	g_string_free(files,TRUE);
	list = g_slist_append(list,command);

	xa_run_command (archive,list);
	if (archive->status == XA_ARCHIVESTATUS_DELETE)
		xa_reload_archive_content(archive);
}

void xa_arj_add (XArchive *archive,GString *files,gchar *compression_string)
{
	GSList *list = NULL;
	gchar *command = NULL;

	if (archive->location_entry_path != NULL)
		archive->working_dir = g_strdup(archive->tmp);

	if (compression_string == NULL)
		compression_string = "1";
	if (archive->passwd != NULL)
		command = g_strconcat ( "arj a ",
								archive->update ? "-u " : "",
								archive->freshen ? "-f " : "",
								archive->remove_files ? "-d1 " : "",
								"-g" , archive->passwd , " -i ",
								"-m",compression_string," ",
								archive->escaped_path,files->str,NULL);
	else
		command = g_strconcat ( "arj a ",
								archive->update ? "-u " : "",
								archive->freshen ? "-f " : "",
								archive->remove_files ? "-d1 " : "",
								" -i ",
								"-m",compression_string," ",
								archive->escaped_path,files->str,NULL);
	g_string_free(files,TRUE);
	list = g_slist_append(list,command);

	xa_run_command (archive,list);
	xa_reload_archive_content(archive);
}

gboolean xa_arj_extract(XArchive *archive,GSList *files)
{
	gchar *command,*e_filename = NULL;
	GSList *list	= NULL,*_files = NULL;
	GString *names  = g_string_new("");
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
		command = g_strconcat ( "arj ",archive->full_path ? "x" : "e",
								" -g",archive->passwd,
								archive->overwrite ? "" : " -n" ,
								" -i " ,
								archive->freshen ? "-f " : "" ,
								archive->update ? "-u " : " ",
								"-y ",
								archive->escaped_path , " ",archive->extraction_path,names->str,NULL);
	else
		command = g_strconcat ( "arj ",archive->full_path ? "x" : "e",
								archive->overwrite ? "" : " -n" ,
								" -i " , archive->freshen ? "-f " : "",
								archive->update ? "-u " : " ",
								"-y ",
								archive->escaped_path , " ",archive->extraction_path,names->str,NULL);
	g_string_free(names,TRUE);
	list = g_slist_append(list,command);

	result = xa_run_command (archive,list);
	return result;
}

void xa_arj_test (XArchive *archive)
{
	gchar *command = NULL;
	GSList *list = NULL;

	archive->status = XA_ARCHIVESTATUS_TEST;
	if (archive->passwd != NULL)
		command = g_strconcat ("arj t -g" , archive->passwd , " -i " , archive->escaped_path, NULL);
	else
		command = g_strconcat ("arj t -i -y - " , archive->escaped_path, NULL);

	list = g_slist_append(list,command);
	xa_run_command (archive,list);
 }

