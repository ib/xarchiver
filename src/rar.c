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

gchar *line = NULL;
extern int output_fd,error_fd,child_pid;

void OpenRar ( gboolean mode , gchar *path)
{
	flag = FALSE;
    gchar *command = g_strconcat ( "rar vl -c- " , path, NULL );
	compressor_pid = SpawnAsyncProcess ( command , 1 , 0 );
    g_free ( command );
	if ( compressor_pid == 0 ) return;
	char *names[]	= {("Filename"),("Size"),("Size now"),("Ratio"),("Date"),("Time"),("Permissions"),("CRC"),("Method"),("Version")};
	GType types[]= {G_TYPE_STRING,G_TYPE_STRING,G_TYPE_STRING,G_TYPE_STRING,G_TYPE_STRING,G_TYPE_STRING,G_TYPE_STRING,G_TYPE_STRING,G_TYPE_STRING,G_TYPE_STRING};
    PasswordProtectedArchive = FALSE;
	CreateListStore ( 10, names , (GType *)types );
	SetIOChannel (output_fd, G_IO_IN|G_IO_PRI|G_IO_ERR|G_IO_HUP|G_IO_NVAL,RarOpen, (gpointer) mode );
	SetIOChannel (error_fd, G_IO_IN|G_IO_PRI|G_IO_ERR|G_IO_HUP|G_IO_NVAL,GenError, NULL );
}

static gboolean RarOpen (GIOChannel *ioc, GIOCondition cond, gpointer data)
{
	gchar **fields;
	gchar *filename;
	if (cond & (G_IO_IN | G_IO_PRI) )
	{
        //This to avoid inserting in the list RAR's copyright message
		if (flag == FALSE )
        {
			for ( x = 0; x <= 8; x++)
			{
				g_io_channel_read_line ( ioc, &line, NULL, NULL, NULL );
                if (line != NULL && data ) gtk_text_buffer_insert(textbuf, &enditer, line, strlen ( line ) );
                if  (line !=NULL && strncmp (line , "  0 files" , 9) == 0)
			    {
				    response = ShowGtkMessageDialog (GTK_WINDOW (MainWindow),GTK_DIALOG_MODAL,GTK_MESSAGE_INFO,	GTK_BUTTONS_OK,"This archive doesn't contain any files !" );
                    g_io_channel_shutdown ( ioc,TRUE,NULL );
	    	        g_io_channel_unref (ioc);
		            g_spawn_close_pid ( child_pid );
                    g_free (line);
                    return FALSE;
			    }
                g_free (line);
			}
			flag = TRUE;
        }
		if ( flag )
		{
			//Now read the filename
			g_io_channel_read_line ( ioc, &line, NULL, NULL, NULL );
		    if ( line == NULL) return TRUE;
            if ( data ) gtk_text_buffer_insert(textbuf, &enditer, line, strlen( line ) );
            //This to avoid inserting in the liststore the last line of Rar output
            if (strncmp (line, "--------", 8) == 0 || strncmp (line, "\x0a",1) == 0)
			{
                g_free (line);
				g_io_channel_read_line ( ioc, &line, NULL, NULL, NULL );
                if ( line != NULL && data ) gtk_text_buffer_insert (textbuf, &enditer, line, strlen( line ) );
                return TRUE;
			}
			gtk_list_store_append (liststore, &iter);
			line[ strlen(line) - 1 ] = '\0';
			if (line[0] == '*') PasswordProtectedArchive = TRUE;
            //This to avoid the white space before the first char of the filename
            line++;
			gtk_list_store_set (liststore, &iter,0,line,-1);
            //Restore the pointer before freeing it
            line--;
            g_free (line);            
			//Now read the rest
			g_io_channel_read_line ( ioc, &line, NULL, NULL, NULL );
			if (line != NULL && data ) gtk_text_buffer_insert ( textbuf, &enditer, line, strlen( line ) );
			fields = split_line (line,9);
			for ( x = 0; x < 9; x++)
				gtk_list_store_set (liststore, &iter,x+1,fields[x],-1);
			g_strfreev ( fields );
			g_free (line);
			return TRUE;
		}
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

