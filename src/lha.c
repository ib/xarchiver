/*
 *  Copyright (C) 2008 Giuseppe Torelli - <colossus73@gmail.com>
 *  Copyright (C) 2006 Lukasz 'Sil2100' Zemczak - <sil2100@vexillium.org>
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
#include "lha.h"
#include <unistd.h>

extern void xa_create_liststore ( XArchive *archive, gchar *columns_names[]);

void xa_open_lha (XArchive *archive)
{
	gchar *command;
	jump_header = last_line = FALSE;
	unsigned short int i;
	command = g_strconcat ("lha l " , archive->escaped_path, NULL);
	archive->has_properties = archive->can_extract = archive->can_add = archive->has_test = TRUE;
	archive->has_sfx = FALSE;
	archive->dummy_size = 0;
	archive->nr_of_files = 0;
	archive->nr_of_dirs = 0;
	archive->format ="LHA";
	archive->nc = 5;
	archive->parse_output = xa_get_lha_line_content;
	xa_spawn_async_process (archive,command);
	g_free (command);

	if (archive->child_pid == 0)
		return;

	GType types[]= {GDK_TYPE_PIXBUF,G_TYPE_STRING,G_TYPE_STRING,G_TYPE_STRING,G_TYPE_UINT64,G_TYPE_STRING,G_TYPE_POINTER};
	archive->column_types = g_malloc0(sizeof(types));
	for (i = 0; i < 7; i++)
		archive->column_types[i] = types[i];
		
	char *names[]= {(_("Permissions")),(_("UID/GID")),(_("Size")),(_("Ratio")),(_("Timestamp"))};
	xa_create_liststore (archive,names);
}

void xa_get_lha_line_content (gchar *line, gpointer data)
{
	XArchive *archive = data;
	XEntry *entry = NULL;
	gpointer item[5];
	unsigned int linesize,n,a;
	gboolean dir = FALSE;
	gchar *filename;

	if (last_line)
		return;
	if (jump_header == FALSE)
	{
		if (line[0] == '-')
		{
			jump_header = TRUE;
			return;
		}
		return;
	}
	if (strncmp(line,"----",4) == 0)
	{
		last_line = TRUE;
		return;
	}
	linesize = strlen(line);

	/* Permission */
	line[10] = '\0';
	item[0] = line;
	if(line[0] == 'd')
		dir = TRUE;
	else
		archive->nr_of_files++;

	/* UID/GID */
	line[22] = '\0';
	item[1] = line + 11;

	//TODO verify the len of the size column with a big archive
	/* Size */
	for(n = 23;n < linesize;n++)
	if(line[n] != ' ')
		break;

	a = n;
	for(;n < linesize;n++)
	if(line[n] == ' ')
		break;

	line[a+(n-a)] = '\0';
	item[2] = line + a;
	archive->dummy_size += strtoll(item[2],NULL,0);

    /* Ratio */
    line[37] = '\0';
    item[3] = line + 31;

    /* Timestamp */
    line[50] = '\0';
    item[4] = line + 38;

	line[(linesize- 1)] = '\0';
	filename = line + 51;

	entry = xa_set_archive_entries_for_each_row (archive,filename,item);
}

gboolean isLha ( FILE *ptr )
{
	unsigned char magic[2];
	fseek(ptr, 0, SEEK_SET);
	if(fseek(ptr, 19, SEEK_CUR) < 0)
		return FALSE;
	if(fread(magic, 1, 2, ptr) == 0)
		return FALSE;

	if(magic[0] == 0x20 && magic[1] <= 0x03)
		return TRUE;
	else
		return FALSE;
}

void xa_lha_delete (XArchive *archive,GString *files)
{
	gchar *command = NULL;
	GSList *list = NULL;

	archive->status = XA_ARCHIVESTATUS_DELETE;
	command = g_strconcat ("lha t " , archive->escaped_path, NULL);
	g_string_free(files,TRUE);
	list = g_slist_append(list,command);

	xa_run_command (archive,list);
}

void xa_lha_add (XArchive *archive,GString *files,gchar *compression_string)
{
	GSList *list = NULL;
	gchar *command = NULL;

	if (archive->location_entry_path != NULL)
		chdir (archive->tmp);

	if (compression_string == NULL)
		compression_string = "5";
	command = g_strconcat( "lha ",
							archive->remove_files ? "m" : "a",
							archive->update ? "u" : "",
							"o",compression_string,
							" ",
							archive->escaped_path,
							files->str,NULL);
	g_string_free(files,TRUE);
	list = g_slist_append(list,command);

	xa_run_command (archive,list);
}

void xa_lha_extract(XArchive *archive,GString *files)
{
	gchar *command = NULL;
	GSList *list = NULL;

	if (archive->passwd != NULL)
		command = g_strconcat ( "arj ",archive->full_path ? "x" : "e",
								" -g",archive->passwd,
								archive->overwrite ? "" : " -n" ,
								" -i " ,
								archive->freshen ? "-f " : "" ,
								archive->update ? "-u " : " ",
								"-y ",
								archive->escaped_path , " ",archive->extraction_path,files->str,NULL);
	else
		command = g_strconcat ( "arj ",archive->full_path ? "x" : "e",
								archive->overwrite ? "" : " -n" ,
								" -i " , archive->freshen ? "-f " : "",
								archive->update ? "-u " : " ",
								"-y ",
								archive->escaped_path , " ",archive->extraction_path,files->str,NULL);
	g_string_free(files,TRUE);
	list = g_slist_append(list,command);

	xa_run_command (archive,list);
}

void xa_lha_test (XArchive *archive)
{
	gchar *command = NULL;
	GSList *list = NULL;

	archive->status = XA_ARCHIVESTATUS_TEST;
	command = g_strconcat ("lha t ",archive->escaped_path,NULL);

	list = g_slist_append(list,command);
	xa_run_command (archive,list);
 }
