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
#include "deb.h"
#include <string.h>

extern void xa_create_liststore (XArchive *archive,gchar *columns_names[]);

void xa_open_deb (XArchive *archive)
{
	gchar *command = NULL;
	unsigned short int i;

	command = g_strconcat ("ar tv ",archive->escaped_path,NULL);
	archive->has_properties = archive->can_extract = TRUE;
	archive->can_add = archive->has_test = archive->has_sfx = FALSE;
	archive->dummy_size = 0;
	archive->nr_of_files = 0;
	archive->nc = 5;
	archive->format = "DEB";
	archive->parse_output = xa_get_ar_line_content;
	xa_spawn_async_process (archive,command);
	g_free (command);

	if (archive->child_pid == 0)
		return;

	char *names[]= {(_("Permissions")),(_("Owner/Group")),(_("Size")),(_("Date modified")),NULL};
	GType types[]= {GDK_TYPE_PIXBUF,G_TYPE_STRING,G_TYPE_STRING,G_TYPE_STRING,G_TYPE_UINT64,G_TYPE_STRING,G_TYPE_POINTER};
	archive->column_types = g_malloc0(sizeof(types));
	for (i = 0; i < 7; i++)
		archive->column_types[i] = types[i];
	xa_create_liststore (archive,names);
}

void xa_get_ar_line_content (gchar *line, gpointer data)
{
	XArchive *archive = data;
	XEntry *entry;
	gchar *filename;
	gpointer item[4];
	gint n = 0, a = 0 ,linesize = 0;

	linesize = strlen(line);

	/* Permissions */
	line[9] = '\0';
	item[0] = line;
	
	/* Owner */
	for(n=12; n < linesize; ++n)
		if(line[n] == ' ')
			break;
	line[n] = '\0';
	item[1] = line+10;

	/* Size */	
	for(++n; n < linesize; ++n)
		if(line[n] >= '0' && line[n] <= '9')
			break;
	a = n;

	for(; n < linesize; ++n)
		if(line[n] == ' ')
			break;

	line[n] = '\0';
	item[2] = line + a;
	archive->dummy_size += g_ascii_strtoull(item[2],NULL,0);
	a = ++n;

	/* Date Modified */	
	for(; n < linesize; ++n)
	{
		if(n == 38)
			break;
	}
	if (line[n] != ' ')
	{
		for(; n < linesize; ++n)
		{
			if(line[n] == ' ')
			break;
		}
	}
	line[n] = '\0';
	item[3] = line + a;

	n++;
	line[linesize-1] = '\0';

	archive->nr_of_files++;
	filename = g_strdup(line + n);
	entry = xa_set_archive_entries_for_each_row (archive,filename,item);
	g_free(filename);
}

gboolean xa_deb_extract(XArchive *archive,GSList *files)
{
	gchar *command;
	GSList *list = NULL,*_files = NULL;
	GString *names = g_string_new("");
	gboolean result = FALSE;

	_files = files;
	while (_files)
	{
		g_string_prepend (names,(gchar*)_files->data);
		g_string_prepend_c (names,' ');
		_files = _files->next;
	}
	g_slist_foreach(files,(GFunc)g_free,NULL);
	g_slist_free(files);

	chdir (archive->extraction_path);
	command = g_strconcat ("ar x ",archive->escaped_path," ",names->str,NULL);
	if (command != NULL)
	{
		g_string_free(names,FALSE);
		list = g_slist_append(list,command);
		result = xa_run_command (archive,list);
	}
	return result;
}
