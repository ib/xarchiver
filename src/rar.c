/*
 *  Copyright (C) 2006 Giuseppe Torelli - <colossus73@gmail.com>
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
#include <string.h>
#include "rar.h"

extern gboolean unrar;
extern void xa_create_liststore ( XArchive *archive, gchar *columns_names[]);
static void xa_get_rar_line_content (gchar *line, gpointer data);

void xa_open_rar (XArchive *archive)
{
	unsigned short int i;
	gchar *command = NULL;
	gchar *rar = NULL;
	jump_header = read_filename = last_line = FALSE;

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
    archive->nr_of_dirs = 0;
    archive->nc = 9;
	archive->parse_output = xa_get_rar_line_content;
	archive->format ="RAR";
	xa_spawn_async_process (archive,command,0);
	g_free ( command );

	if ( archive->child_pid == 0 )
		return;

	GType types[]= {G_TYPE_STRING,G_TYPE_STRING,G_TYPE_UINT64,G_TYPE_UINT64,G_TYPE_STRING,G_TYPE_STRING,G_TYPE_STRING,G_TYPE_STRING,G_TYPE_STRING,G_TYPE_STRING,G_TYPE_STRING};
	archive->column_types = g_malloc0(sizeof(types));
	for (i = 0; i < 11; i++)
		archive->column_types[i] = types[i];

	char *names[]= {(_("Original")),(_("Compressed")),(_("Ratio")),(_("Date")),(_("Time")),(_("Permissions")),(_("CRC")),(_("Method")),(_("Version"))};
    xa_create_liststore (archive,names);
}

void xa_get_rar_line_content (gchar *line, gpointer data)
{
	XArchive *archive = data;
	XEntry *entry;

	gpointer item[9];
	unsigned short int i = 0;
	unsigned int linesize,n,a;
	gboolean encrypted,dir;

	encrypted = dir = FALSE;

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
	if (read_filename == FALSE)
	{
		linesize = strlen(line);
		if(line[0] == '*')
			encrypted = TRUE;
		else if (line[0] == '-')
		{
			last_line = TRUE;
			return;
		}
		line++;
		line[linesize - 2] = '\0';
		filename = g_strdup(line);
		read_filename = TRUE;
	}
	else
	{
		linesize = strlen(line);
		/* Size */
		for(n=0; n < linesize && line[n] == ' '; n++);
		a = n;
		for(; n < linesize && line[n] != ' '; n++);
		line[n]='\0';
		item[i] = line + a;
		archive->dummy_size += strtoll(item[i],NULL,0);
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
		else
			archive->nr_of_files++;
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

		if (dir)
		{
			/* Work around for rar which doesn't
			 * output / with directories */
			gchar *filename_with_slash = g_strconcat (filename,"/",NULL);
			g_free (filename);
			filename = filename_with_slash;
		}
		entry = xa_set_archive_entries_for_each_row (archive,filename,encrypted,item);
		g_free(filename);
		read_filename = FALSE;
	}
}
