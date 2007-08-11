/*
 *  Copyright (C) 2006 Giuseppe Torelli - <colossus73@gmail.com>
 *  Copyright (C) 2006 Benedikt Meurer - <benny@xfce.org>
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
#include "tar.h"

extern void xa_create_liststore ( XArchive *archive, gchar *columns_names[]);

void xa_open_tar (XArchive *archive)
{
	gchar *command;
	gchar *tar;
	unsigned short int i;

	tar = g_find_program_in_path ("gtar");
	if (tar == NULL)
		tar = g_strdup ("tar");

	command = g_strconcat (tar, " tfv " , archive->escaped_path, NULL);
	archive->has_properties = archive->can_add = archive->can_extract = TRUE;
	archive->has_test = archive->has_sfx = FALSE;
	archive->dummy_size = 0;
	archive->nr_of_files = 0;
	archive->nr_of_dirs = 0;
	archive->nc = 6;
	archive->parse_output = xa_get_tar_line_content;
	archive->format ="TAR";
	xa_spawn_async_process (archive,command,0);

	g_free (command);
	g_free (tar);

	if (archive->child_pid == 0)
		return;

	GType types[]= {G_TYPE_STRING,G_TYPE_STRING,G_TYPE_STRING,G_TYPE_STRING,G_TYPE_STRING,G_TYPE_UINT64,G_TYPE_STRING,G_TYPE_STRING};
	archive->column_types = g_malloc0(sizeof(types));
	for (i = 0; i < 8; i++)
		archive->column_types[i] = types[i];

	char *names[]= {(_("Points to")),(_("Permissions")),(_("Owner/Group")),(_("Size")),(_("Date")),(_("Time"))};
	xa_create_liststore (archive,names);
}

void xa_get_tar_line_content (gchar *line, gpointer data)
{
	XArchive *archive = data;
	XEntry *entry;
	gchar *filename;
	gpointer item[6];
	gsize linesize = 0;
	gint n = 0, a = 0;
	gboolean dir = FALSE;

	linesize = strlen(line);

	/* Permissions */
	line[10] = '\0';
	item[1] = line;
	
	/* Owner */
	for(n=13; n < linesize; ++n)
		if(line[n] == ' ')
			break;
	line[n] = '\0';
	item[2] = line+11;

	/* Size */	
	for(++n; n < linesize; ++n)
		if(line[n] >= '0' && line[n] <= '9')
			break;
	a = n;

	for(; n < linesize; ++n)
		if(line[n] == ' ')
			break;

	line[n] = '\0';
	item[3] = line + a;
	archive->dummy_size += strtoll(item[3],NULL,0);
	a = ++n;

	/* Date */	
	for(; n < linesize; n++)
		if(line[n] == ' ')
			break;

	line[n] = '\0';
	item[4] = line + a;

	/* Time */
	a = ++n;
	for (; n < linesize; n++)
		if (line[n] == ' ')
			break;

	line[n] = '\0';
	item[5] = line + a;
	n++;
	line[linesize-1] = '\0';

	filename = line + n;
	
	/* Symbolic link */
	gchar *temp = g_strrstr (filename,"->"); 
	if (temp ) 
	{
		gint len = strlen(filename) - strlen(temp);
		item[0] = (filename +=3) + len;
		filename[strlen(filename) - strlen(temp)] = '\0';
	}
	else
		item[0] = NULL;

	if(line[0] == 'd')
	{
		dir = TRUE;
		/* Work around for gtar, which does
		 * not output / with directories */

		if(line[linesize-2] != '/')
			filename = g_strconcat(line + n, "/", NULL); 
		else
			filename = g_strdup(line + n); 
	}
	else
	{
		archive->nr_of_files++;
		filename = g_strdup(line + n); 
	}

	entry = xa_set_archive_entries_for_each_row (archive,filename,FALSE,item);
	g_free(filename);
}

gboolean isTar ( FILE *ptr )
{
	unsigned char magic[7];
	fseek ( ptr, 0 , SEEK_SET );
    if ( fseek ( ptr , 257 , SEEK_CUR) < 0 )
		return FALSE;
    if ( fread ( magic, 1, 7, ptr ) == 0 )
		return FALSE;
    if ( memcmp ( magic,"\x75\x73\x74\x61\x72\x00\x30",7 ) == 0 || memcmp (magic,"\x75\x73\x74\x61\x72\x20\x20",7 ) == 0)
		return TRUE;
    else
		return FALSE;
}
