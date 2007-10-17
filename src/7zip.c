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
#include "7zip.h"

extern gboolean sevenzr;
extern gboolean sevenza;
extern void xa_create_liststore ( XArchive *archive, gchar *columns_names[]);
gchar *exe;

void xa_open_7zip (XArchive *archive)
{
	jump_header = last_line = FALSE;
	unsigned short int i = 0;

	if (sevenzr)
		exe = "7zr ";
	if (sevenza)
		exe = "7za ";

	gchar *command = g_strconcat ( exe,"l " , archive->escaped_path, NULL );
	archive->has_sfx = archive->has_properties = archive->can_add = archive->can_extract = archive->has_test = TRUE;
	archive->dummy_size = 0;
	archive->nr_of_files = 0;
	archive->nr_of_dirs = 0;
	archive->format ="7-ZIP";
	archive->nc = 6;
	archive->parse_output = xa_get_7zip_line_content;
	xa_spawn_async_process (archive,command,0);
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
	gint linesize = 0,n = 0,a = 0;
	gboolean dir = FALSE;

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
	if (line[0] == '-')
	{
		last_line = TRUE;
		return;
	}
	
	linesize = strlen(line);

	/* Date */
	line[10] = '\0';
	item[4] = line;

	/* Time */
	for(n=13; n < linesize; ++n)
		if(line[n] == ' ')
			break;
	line[n] = '\0';
	item[3] = line + 11;
	a = ++n;
	
	/* Permissions */
	for(; n < linesize; n++)
		if(line[n] == ' ')
			break;
	line[n] = '\0';
	if ((line+a)[0] == 'D')
		dir = TRUE;
	else
		archive->nr_of_files++;
	item[2] = line + a;
	
	/* Size */
	for(++n; n < linesize; ++n)
		if(line[n] >= '0' && line[n] <= '9')
			break;
	a = n;

	for(; n < linesize; ++n)
		if(line[n] == ' ')
			break;

	line[n] = '\0';
	item[0] = line + a;
	archive->dummy_size += strtoll(item[0],NULL,0);

	/* Compressed */
	for(++n; n < linesize; ++n)
		if(line[n] >= '0' && line[n] <= '9')
			break;
	a = n;

	for(; n < linesize; ++n)
		if(line[n] == ' ')
			break;

	line[n] = '\0';

	if (line[50] != ' ')
	{
		n+=2;
		item[1] = line + a;
		line[linesize-1] = '\0';
		filename = g_strdup(line + n);
	}
	/* Is this a solid archive? */
	else
	{
		item[1] = "0";
		line[n-1] = '\0';
		filename = g_strdup(line + 53);
	}

	/* Work around for 7za which doesn't
	* output / with directories */
	if (dir)
	{
		gchar *filename_with_slash = g_strconcat (filename,"/",NULL);
		g_free (filename);
		filename = filename_with_slash;
	}
	entry = xa_set_archive_entries_for_each_row (archive,filename,FALSE,item);
	g_free(filename);
}

