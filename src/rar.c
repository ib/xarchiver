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
 
#include "rar.h"

extern int output_fd,error_fd,child_pid,child_status;

void OpenRar ( gboolean mode , gchar *path)
{
	jump_header = FALSE;
    gchar *command = g_strconcat ( "rar vl -c- " , path, NULL );
	compressor_pid = SpawnAsyncProcess ( command , 1 , 0 );
    g_free ( command );
	if ( compressor_pid == 0 ) return;
    dummy_size = 0;
    number_of_files = 0;
    number_of_dirs = 0;
	char *names[]	= {("Filename"),("Original"),("Compressed"),("Ratio"),("Date"),("Time"),("Permissions"),("CRC"),("Method"),("Version")};
	GType types[]= {G_TYPE_STRING,G_TYPE_UINT64,G_TYPE_UINT64,G_TYPE_STRING,G_TYPE_STRING,G_TYPE_STRING,G_TYPE_STRING,G_TYPE_STRING,G_TYPE_STRING,G_TYPE_STRING};
    PasswordProtectedArchive = FALSE;
	CreateListStore ( 10, names , (GType *)types );
	SetIOChannel (output_fd, G_IO_IN|G_IO_PRI|G_IO_ERR|G_IO_HUP|G_IO_NVAL,RarOpen, (gpointer) mode );
	SetIOChannel (error_fd, G_IO_IN|G_IO_PRI|G_IO_ERR|G_IO_HUP|G_IO_NVAL,GenError, NULL );
    WaitExitStatus ( child_pid , NULL );
}

static gboolean RarOpen (GIOChannel *ioc, GIOCondition cond, gpointer data)
{
	gchar **fields = NULL;
	gchar *filename = NULL;
    gchar *line = NULL;

	if (cond & (G_IO_IN | G_IO_PRI) )
	{
        //This to avoid inserting in the list RAR's copyright message
		if (jump_header == FALSE )
        {
            g_io_channel_read_line ( ioc, &line, NULL, NULL, NULL );
            if (line == NULL) return TRUE;
            if ( data ) gtk_text_buffer_insert (textbuf, &enditer, line, strlen( line ) );
            if  (strncmp (line , "--------" , 8) == 0)
            {
                jump_header = TRUE;
                odd_line = TRUE;
            }
            g_free (line);
            return TRUE;
        }
        if ( jump_header && odd_line )
		{
			//Now read the filename
			g_io_channel_read_line ( ioc, &line, NULL, NULL, NULL );
		    if ( line == NULL ) return TRUE;
            if ( data ) gtk_text_buffer_insert (textbuf, &enditer, line, strlen( line ) );
            //This to avoid inserting in the liststore the last line of Rar output
            if (strncmp (line, "--------", 8) == 0 || strncmp (line, "\x0a",1) == 0)
			{
                g_free (line);
				g_io_channel_read_line ( ioc, &line, NULL, NULL, NULL );
                if ( line != NULL && data )
                {
                    gtk_text_buffer_insert (textbuf, &enditer, line, strlen( line ) );
                    g_free (line);
                }
                return FALSE;
			}
			gtk_list_store_append (liststore, &iter);
			line[ strlen(line) - 1 ] = '\000';
			if (line[0] == '*') PasswordProtectedArchive = TRUE;
            //This to avoid the white space or the * before the first char of the filename
            line++;
			gtk_list_store_set (liststore, &iter,0,line,-1);
            //Restore the pointer before freeing it
            line--;
            g_free (line);
            odd_line = ! odd_line;
            return TRUE;
        }
        else
        {
			//Now read the rest
			g_io_channel_read_line ( ioc, &line, NULL, NULL, NULL );
            if ( line == NULL) return TRUE;
			if ( data ) gtk_text_buffer_insert ( textbuf, &enditer, line, strlen( line ) );
			fields = split_line (line,9);
            if ( strstr (fields[5] , "d") == NULL && strstr (fields[5] , "D") == NULL ) number_of_files++;
                else number_of_dirs++;
			for ( x = 0; x < 9; x++)
            {
                if (x == 0 || x == 1) gtk_list_store_set (liststore, &iter,x+1,atoll (fields[x]),-1);
                    else gtk_list_store_set (liststore, &iter,x+1,fields[x],-1);
            }
            dummy_size += atoll (fields[0]);
            gtk_progress_bar_pulse ( GTK_PROGRESS_BAR (progressbar) );
            while (gtk_events_pending() )
			    gtk_main_iteration();
			g_strfreev ( fields );
			g_free (line);
            odd_line = ! odd_line;
			return TRUE;
		}
		return TRUE;
	}
	else if (cond & (G_IO_ERR | G_IO_HUP | G_IO_NVAL) )
	{
		g_io_channel_shutdown ( ioc,TRUE,NULL );
		g_io_channel_unref (ioc);
    	return FALSE;
	}
}

