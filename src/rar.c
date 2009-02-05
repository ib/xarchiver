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
#include "rar.h"
#include <unistd.h>

extern gboolean unrar;
extern void xa_reload_archive_content(XArchive *archive);
extern void xa_create_liststore ( XArchive *archive, gchar *columns_names[]);

void xa_open_rar (XArchive *archive)
{
	unsigned short int i;
	gchar *command = NULL;
	gchar *rar = NULL;
	jump_header = read_filename = last_line = encrypted = FALSE;

	if (unrar)
	{
		rar = "unrar";
		archive->can_add = archive->has_sfx = FALSE;
	}
	else
	{
		rar = "rar";
		archive->can_add = archive->has_sfx = TRUE;
	}

	command = g_strconcat ( rar," v " , archive->escaped_path, NULL );
	archive->can_extract = archive->has_test = archive->has_properties = TRUE;
	archive->dummy_size = 0;
    archive->nr_of_files = 0;
    archive->nc = 10;
	archive->parse_output = xa_get_rar_line_content;
	archive->format = "RAR";
	xa_spawn_async_process (archive,command);
	g_free ( command );

	if ( archive->child_pid == 0 )
		return;

	GType types[]= {GDK_TYPE_PIXBUF,G_TYPE_STRING,G_TYPE_UINT64,G_TYPE_UINT64,G_TYPE_STRING,G_TYPE_STRING,G_TYPE_STRING,G_TYPE_STRING,G_TYPE_STRING,G_TYPE_STRING,G_TYPE_STRING,G_TYPE_POINTER};
	archive->column_types = g_malloc0(sizeof(types));
	for (i = 0; i < 12; i++)
		archive->column_types[i] = types[i];

	char *names[]= {(_("Original")),(_("Compressed")),(_("Ratio")),(_("Date")),(_("Time")),(_("Permissions")),(_("CRC")),(_("Method")),(_("Version")),NULL};
	xa_create_liststore (archive,names);
}

void xa_get_rar_line_content (gchar *line, gpointer data)
{
	XArchive *archive = data;
	XEntry *entry;
	gpointer item[9];
	unsigned short int i = 0;
	unsigned int linesize,n,a;
	gboolean dir = FALSE;
	static gchar *filename;

	if (last_line)
		return;

	if (jump_header == FALSE)
	{
		if (strncmp(line,"Comment:",8) == 0)
		{
			jump_comment = archive->has_comment = TRUE;
			archive->comment = g_string_new("");
			archive->comment = g_string_append(archive->comment,&line[9]);
			return;
		}
		if (jump_comment == TRUE)
		{
			if (strncmp(line,"Pathname/Comment",16) != 0)
			{	archive->comment = g_string_append(archive->comment,line);
				return;
			}
			jump_comment = FALSE;
		}
		if (strncmp(line, "-------------------------------------------------------------------------------",78) == 0)
			jump_header = TRUE;

		return;
	}

	if (read_filename == FALSE)
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
		filename = g_strdup(line+1);
		read_filename = TRUE;
	}
	else
	{
		linesize = strlen(line);
		archive->nr_of_files++;
		/* Size */
		for(n=0; n < linesize && line[n] == ' '; n++);
		a = n;
		for(; n < linesize && line[n] != ' '; n++);
		line[n]='\0';
		item[i] = line + a;
		archive->dummy_size += g_ascii_strtoull(item[i],NULL,0);
		i++;
		n++;
		
		/* Compressed */
		for(; n < linesize && line[n] == ' '; n++);
		a = n;
		for(; n < linesize && line[n] != ' '; n++);
		line[n]='\0';
		item[i] = line + a;
		i++;
		n++;

		/* Ratio */
		for(; n < linesize && line[n] == ' '; n++);
		a = n;
		for(; n < linesize && line[n] != ' '; n++);
		line[n] = '\0';
		item[i] = line + a;
		i++;
		n++;

		/* Date */
		for(; n < linesize && line[n] == ' '; n++);
		a = n;
		for(; n < linesize && line[n] != ' '; n++);
		line[n] = '\0';
		item[i] = line + a;
		i++;
		n++;

		/* Time */
		for(; n < linesize && line[n] == ' '; n++);
		a = n;
		for(; n < linesize && line[n] != ' '; n++);
		line[n] = '\0';
		item[i] = line + a;
		i++;
		n++;

		/* Permissions */
		for(; n < linesize && line[n] == ' '; n++);
		a = n;
		for(; n < linesize && line[n] != ' '; n++);
		line[n] = '\0';
		if ((line+a)[0] == 'd')
			dir = TRUE;
		item[i] = line + a;
		i++;
		n++;

		/* CRC */
		for(; n < linesize && line[n] == ' '; n++);
		a = n;
		for(; n < linesize && line[n] != ' '; n++);
		line[n] = '\0';
		item[i] = line + a;
		i++;
		n++;

		/* Method */
		for(; n < linesize && line[n] == ' '; n++);
		a = n;
		for(; n < linesize && line[n] != ' '; n++);
		line[n] = '\0';
		item[i] = line + a;
		i++;
		n++;

		/* version */
		for(; n < linesize && line[n] == ' '; n++);
		a = n;
		for(; n < linesize && line[n] != ' ' && line[n] != '\n'; n++);
		line[n] = '\0';
		item[i] = line + a;

		/* Work around for rar which doesn't
		 * output / with directories */
		if (dir)
		{
			gchar *filename_with_slash = g_strconcat (filename,"/",NULL);
			g_free (filename);
			filename = filename_with_slash;
		}
		entry = xa_set_archive_entries_for_each_row (archive,filename,item);
		if (entry != NULL)
			entry->is_encrypted = encrypted;
		g_free(filename);
		read_filename = FALSE;
		encrypted = FALSE;
	}
}

void xa_rar_delete (XArchive *archive,GSList *names)
{
	gchar *command,*e_filename = NULL;
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
	
	command = g_strconcat ("rar d ",archive->escaped_path," ",files->str,NULL);
	g_string_free(files,TRUE);
	list = g_slist_append(list,command);

	xa_run_command (archive,list);
	if (archive->status == XA_ARCHIVESTATUS_DELETE)
		xa_reload_archive_content(archive);
}

void xa_rar_add (XArchive *archive,GString *files,gchar *compression_string)
{
	GSList *list = NULL;
	gchar *command = NULL;

	if (archive->location_entry_path != NULL)
		archive->working_dir = g_strdup(archive->tmp);

	if (compression_string == NULL)
		compression_string = "3";
	if (archive->passwd != NULL)
		command = g_strconcat ( "rar a ",
									archive->update ? "-u " : "",
									archive->freshen ? "-f " : "",
									archive->solid_archive ? "-s " : "",
									archive->remove_files ? "-df " : "",
									"-p" , archive->passwd,
									" -idp ",
									"-m",compression_string," ",
									archive->escaped_path,
									files->str,NULL);
	else
		command = g_strconcat ( "rar a ",
									archive->update ? "-u " : "",
									archive->freshen ? "-f " : "",
									archive->solid_archive ? "-s " : " ",
									archive->remove_files ? "-df " : " ",
									"-idp ",
									"-m",compression_string," ",
									archive->escaped_path,
									files->str,NULL);

	g_string_free(files,TRUE);
	list = g_slist_append(list,command);

	xa_run_command (archive,list);
	xa_reload_archive_content(archive);
}

gboolean xa_rar_extract(XArchive *archive,GSList *files)
{
	gchar *rar, *command, *e_filename = NULL;
	GSList *list = NULL,*_files = NULL;
	GString *names = g_string_new("");
	gboolean result = FALSE;

	_files = files;
	if (unrar)
		rar = "unrar";
	else
		rar = "rar";

	while (_files)
	{
		e_filename = xa_escape_filename((gchar*)_files->data,"$'`\"\\!?* ()[]&|:;<>#");
		g_string_prepend (names,e_filename);
		g_string_prepend_c (names,' ');
		_files = _files->next;
	}
	g_slist_foreach(_files,(GFunc)g_free,NULL);
	g_slist_free(_files);
	
	if (archive->passwd != NULL)
		command = g_strconcat (rar," ",archive->full_path ? "x " : "e ",
										archive->freshen ? "-f " : "" , archive->update ? "-u " : "",
										" -p",archive->passwd,
										archive->overwrite ? " -o+" : " -o-",
										" -idp ",
										archive->escaped_path,names->str," ",archive->extraction_path , NULL );
	else
		command = g_strconcat (rar," ",archive->full_path ? "x " : "e ",
										archive->freshen ? "-f " : "" , archive->update ? "-u " : "",
										archive->overwrite ? "-o+" : "-o-",
										" -idp ",
										archive->escaped_path,names->str," ",archive->extraction_path , NULL );
	g_string_free(names,TRUE);
	list = g_slist_append(list,command);

	result = xa_run_command (archive,list);
	return result;
}

void xa_rar_test (XArchive *archive)
{
	gchar *rar = NULL;
	gchar *command = NULL;
	GSList *list = NULL;

	if (unrar)
		rar = "unrar";
	else
		rar = "rar";

	archive->status = XA_ARCHIVESTATUS_TEST;
	if (archive->passwd != NULL)
		command = g_strconcat (rar," t -idp -p" , archive->passwd ," " , archive->escaped_path, NULL);
	else
		command = g_strconcat (rar," t -idp " , archive->escaped_path, NULL);

	list = g_slist_append(list,command);
	xa_run_command (archive,list);
 }
