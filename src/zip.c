/*
 *  Xarchiver
 *
 *  Copyright (C) 2005 Giuseppe Torelli - Colossus
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
 
#include "zip.h"
 
extern int output_fd,error_fd,child_pid;
void OpenZip ( gboolean mode , gchar *path)
{
	gchar *command = g_strconcat ("unzip -vl -qq " , path, NULL );
	compressor_pid = SpawnAsyncProcess ( command , 1 , 0 );
	g_free ( command );
	if ( compressor_pid == 0 ) return;
	char *names[]= {(_("Filename")),(_("Original Size")),(_("Method")),(_("Size now")),(_("Ratio")),(_("Date")),(_("Time")),(_("CRC-32"))};
	GType types[]= {G_TYPE_STRING,G_TYPE_STRING,G_TYPE_STRING,G_TYPE_STRING,G_TYPE_STRING,G_TYPE_STRING,G_TYPE_STRING,G_TYPE_STRING};
	CreateListStore ( 8, names , (GType *)types );
	SetIOChannel (output_fd, G_IO_IN|G_IO_PRI|G_IO_ERR|G_IO_HUP|G_IO_NVAL,ZipOpen, (gpointer) mode );
	SetIOChannel (error_fd, G_IO_IN|G_IO_PRI|G_IO_ERR|G_IO_HUP|G_IO_NVAL,GenError, NULL );
}

static gboolean ZipOpen (GIOChannel *ioc, GIOCondition cond, gpointer data)
{
	num_cols = 0;
	gchar **fields = NULL;
	gchar *filename = NULL;
	gchar *line = NULL;
	if (cond & (G_IO_IN | G_IO_PRI) )
	{
		g_io_channel_read_line ( ioc, &line, NULL, NULL, NULL );
		if (line != NULL && data ) gtk_text_buffer_insert (textbuf, &enditer, line, strlen ( line ) );
		fields = split_line (line,7);
		filename = get_last_field (line,8);
		gtk_list_store_append (liststore, &iter);
		if ( filename[strlen(filename) - 1] != '/')
		{
			for ( x = 0; x < 7; x++)
			{
				gtk_list_store_set (liststore, &iter,num_cols+1,fields[x],-1);
				num_cols++;
			}
		}
		gtk_list_store_set (liststore, &iter,0,filename,-1);
		g_strfreev ( fields );
		g_free (line);
		return TRUE;
	}
	else if (cond & (G_IO_ERR | G_IO_HUP | G_IO_NVAL) )
	{
		g_io_channel_shutdown ( ioc,TRUE,NULL );
		g_io_channel_unref (ioc);
		g_spawn_close_pid ( child_pid );
		return FALSE;
	}
}

