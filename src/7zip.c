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
extern void xa_create_liststore ( XArchive *archive, gchar *columns_names[]);
gchar *exe;

void xa_open_7zip (XArchive *archive)
{
	jump_header = last_line = FALSE;
	unsigned short int i = 0;

	if (sevenzr)
		exe = "7zr ";
	else
		exe = "7za ";

	gchar *command = g_strconcat ( exe,"l " , archive->escaped_path, NULL );
	g_print (command);
	archive->has_sfx = archive->has_properties = archive->can_add = archive->can_extract = archive->has_test = TRUE;
	archive->dummy_size = 0;
	archive->nr_of_files = 0;
	archive->nr_of_dirs = 0;
	archive->format ="7-ZIP";
	archive->nc = 5;
	archive->parse_output = xa_get_7zip_line_content;
	xa_spawn_async_process (archive,command,0);
	g_free ( command );
	if ( archive->child_pid == 0 )
		return;

	GType types[]= {G_TYPE_STRING,G_TYPE_STRING,G_TYPE_UINT64,G_TYPE_UINT64,G_TYPE_STRING,G_TYPE_STRING,G_TYPE_STRING};
	archive->column_types = g_malloc0(sizeof(types));
	for (i = 0; i < 7; i++)
		archive->column_types[i] = types[i];

	char *names[]= {(_("Original")),(_("Compressed")),(_("Attr")),(_("Time")),(_("Date"))};
	xa_create_liststore (archive,names);
}

void xa_get_7zip_line_content (gchar *line, gpointer data)
{
	XArchive *archive = data;
	XEntry *entry;
	gchar *filename;
	gpointer item[5];
	unsigned short int i = 0;
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
	item[1] = line + a;
	n+= 2;

	/* Filename */
	line[linesize-1] = '\0';
	filename = g_strdup(line + n);
	
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

	/* 		archive->cmd_line_output = g_list_append (archive->cmd_line_output,g_strdup(line));
			fields = split_line ( line , 5 );
			filename = get_last_field ( line , 6);
			gtk_list_store_append (archive->liststore, &iter);
			if ( g_str_has_prefix(fields[2] , "D") == FALSE)
				archive->nr_of_files++;
			else
				archive->nr_of_dirs++;
			for ( x = 0; x < 5; x++)
			{
				if (x == 3)
					gtk_list_store_set (archive->liststore, &iter,1,strtoll(fields[3],NULL,0),-1);
				else if (x == 4)
					gtk_list_store_set (archive->liststore, &iter,2,strtoll(fields[4],NULL,0),-1);
				else
					gtk_list_store_set (archive->liststore, &iter,(5-x),fields[x],-1);
			}
			archive->dummy_size += strtoll(fields[3],NULL,0);
			if ( filename == NULL )
				gtk_list_store_set (archive->liststore, &iter,0,fields[4],-1);
			else
				gtk_list_store_set (archive->liststore, &iter,0,filename,-1);
			g_strfreev ( fields );

			while (gtk_events_pending() )
				gtk_main_iteration();
			g_free (line);
		}
		while (status == G_IO_STATUS_NORMAL);
		if (status == G_IO_STATUS_ERROR || status == G_IO_STATUS_EOF)
			goto done;
	}
	else if (cond & (G_IO_ERR | G_IO_HUP) )
	{
done:	g_io_channel_shutdown ( ioc,TRUE,NULL );
		g_io_channel_unref (ioc);
		gtk_tree_view_set_model (GTK_TREE_VIEW(archive->treeview), archive->model);
		g_object_unref (archive->model);
		return FALSE;
	}
	return TRUE;*/
}

