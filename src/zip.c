/*
 *  Copyright (C) 2007 Giuseppe Torelli - <colossus73@gmail.com>
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
#include "zip.h"


extern void xa_create_liststore ( XArchive *archive, gchar *columns_names[]);
void xa_get_zip_line_content (gchar *line, gpointer data);

void xa_open_zip (XArchive *archive)
{
	unsigned short int i;

	gchar *command = g_strconcat ("zipinfo -t -l ",archive->escaped_path, NULL);
	archive->has_sfx = archive->has_properties = archive->can_add = archive->can_extract = archive->has_test = TRUE;
	archive->dummy_size  = 0;
    archive->nr_of_files = 0;
    archive->nr_of_dirs  = 0;
    archive->nc = 9;
	archive->parse_output = xa_get_zip_line_content;
	archive->format ="ZIP";
	xa_spawn_async_process (archive,command);
	g_free ( command );

	if (archive->child_pid == 0)
		return;

	GType types[] = {GDK_TYPE_PIXBUF,G_TYPE_STRING,G_TYPE_STRING,G_TYPE_STRING,G_TYPE_STRING,G_TYPE_UINT64,G_TYPE_UINT64,G_TYPE_STRING,G_TYPE_STRING,G_TYPE_STRING,G_TYPE_POINTER};
	archive->column_types = g_malloc0(sizeof(types));
	for (i = 0; i < 11; i++)
		archive->column_types[i] = types[i];

	char *names[]= {(_("Permissions")),(_("Version")),(_("OS")),(_("Size")),(_("Compressed")),(_("Method")),(_("Date")),(_("Time")),NULL};
	xa_create_liststore (archive,names);
}

void xa_get_zip_line_content (gchar *line, gpointer data)
{
	XArchive *archive = data;
	XEntry *entry = NULL;

	gchar *filename;
	gpointer item[8];
	unsigned short int i = 0;
	unsigned int linesize,n,a;
	gboolean encrypted,dir;

	encrypted = dir = FALSE;

	if ((line[0] != 'd') && (line[0] != '-'))
		return;

	linesize = strlen(line);

	/* permissions */
	for(n=0; n < linesize && line[n] == ' '; n++);
	a = n;
	for(; n < linesize && line[n] != ' '; n++);

	line[n]='\0';
	item[i] = line + a;
	if ( (line+a)[0] == 'd')
		dir = TRUE;
	else
		archive->nr_of_files++;
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
	archive->dummy_size += strtoll(item[i],NULL,0);
	i++;
	n++;

	/* tx/bx */
	for(; n < linesize && line[n] == ' '; n++);
	a = n;
	for(; n < linesize && line[n] != ' '; n++);

	line[n]='\0';
	if ((line+a)[0] == 'B' || (line+a)[0] == 'T')
		encrypted = TRUE;
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
