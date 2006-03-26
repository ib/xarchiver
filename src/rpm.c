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
 
#include "rpm.h"
 
extern int input_fd , output_fd , error_fd, child_pid;
FILE *stream;
int fd;
gchar *tmp = NULL;
gchar buffer[2048];
gsize bytes_read = 0;
gsize bytes_written = 0;
GIOStatus status;
GError *error = NULL;
GIOChannel *ioc_cpio , *input_ioc;

void OpenRPM ( gboolean mode , gchar *path )
{
	unsigned char bytes[8];
    int dl,il,sigsize,offset;
    gchar ibs[4];
    
    action = inactive;
    stream = fopen ( path , "r" );
	if (stream == NULL)
    {
        gchar *msg = g_strdup_printf (_("Can't open archive %s:\n%s") , path , g_strerror (errno) ); 
		response = ShowGtkMessageDialog (GTK_WINDOW (MainWindow) , GTK_DIALOG_MODAL,GTK_MESSAGE_ERROR,GTK_BUTTONS_OK,
		msg);
		g_free (msg);
		return;
    }
	char *names[]= {(_("Filename")),(_("Permission")),(_("Links")),(_("Owner")),(_("Group")),(_("Size"))};
	GType types[]= {G_TYPE_STRING,G_TYPE_STRING,G_TYPE_STRING,G_TYPE_STRING,G_TYPE_STRING,G_TYPE_STRING};
	CreateListStore ( 6, names , (GType *)types );

    if (fseek ( stream, 104 , SEEK_CUR ) )
    {
        fclose (stream);
        response = ShowGtkMessageDialog (GTK_WINDOW (MainWindow),GTK_DIALOG_MODAL,GTK_MESSAGE_ERROR,GTK_BUTTONS_OK,(char *)g_strerror(errno));
        return;
    }
    if ( fread ( bytes, 1, 8, stream ) == 0 )
	{
		fclose ( stream );
		response = ShowGtkMessageDialog (GTK_WINDOW (MainWindow),GTK_DIALOG_MODAL,GTK_MESSAGE_ERROR,GTK_BUTTONS_OK,(char *)g_strerror(errno));
		return;
    }
    il = 256 * ( 256 * ( 256 * bytes[0] + bytes[1]) + bytes[2] ) + bytes[3];
    dl = 256 * ( 256 * ( 256 * bytes[4] + bytes[5]) + bytes[6] ) + bytes[7];
    sigsize = 8 + 16 * il + dl;
    offset = 104 + sigsize + ( 8 - ( sigsize % 8 ) ) % 8 + 8;
    if (fseek ( stream, offset  , SEEK_SET ) )
    {
        fclose (stream);
        response = ShowGtkMessageDialog (GTK_WINDOW (MainWindow),GTK_DIALOG_MODAL,GTK_MESSAGE_ERROR,GTK_BUTTONS_OK,(char *)g_strerror(errno));
        return;
    }
    if ( fread ( bytes, 1, 8, stream ) == 0 )
	{
		fclose ( stream );
		response = ShowGtkMessageDialog (GTK_WINDOW (MainWindow),GTK_DIALOG_MODAL,GTK_MESSAGE_ERROR,GTK_BUTTONS_OK,(char *)g_strerror(errno));
		return;
    }
    il = 256 * ( 256 * ( 256 * bytes[0] + bytes[1]) + bytes[2] ) + bytes[3];
    dl = 256 * ( 256 * ( 256 * bytes[4] + bytes[5]) + bytes[6] ) + bytes[7];
    sigsize = 8 + 16 * il + dl;
    offset = offset + sigsize;
    fclose (stream);

    tmp = g_strdup ("/tmp/xarchiver-XXXXXX");
	fd = g_mkstemp ( tmp );
    sprintf ( ibs , "%u" , offset );

    //Now I run dd to have the bzip2 / gzip compressed cpio archive in /tmp
    gchar *command = g_strconcat ( "dd if=" , path, " ibs=" , ibs , " skip=1 of=" , tmp , NULL );
	compressor_pid = SpawnAsyncProcess ( command , 0 , 0);
    g_free ( command );
	if ( compressor_pid == 0 )
    {
        fclose ( stream );
        unlink ( tmp );
        g_free (tmp);
        return;
    }
    SetIOChannel (error_fd, G_IO_IN|G_IO_PRI|G_IO_ERR|G_IO_HUP|G_IO_NVAL,GenError, NULL );
    g_child_watch_add ( compressor_pid , (GChildWatchFunc) DecompressCPIO , tmp );
}

GChildWatchFunc *DecompressCPIO (GPid pid , gint status , gpointer data)
{
    if ( WIFEXITED(status) )
    {   
	    if ( WEXITSTATUS (status) )
    	{
	    	archive_error = TRUE;
    		SetButtonState (1,1,0,0,0);
	    	gtk_window_set_title ( GTK_WINDOW (MainWindow) , "Xarchiver " VERSION );
		    response = ShowGtkMessageDialog (GTK_WINDOW 		(MainWindow),GTK_DIALOG_MODAL,GTK_MESSAGE_QUESTION,GTK_BUTTONS_YES_NO,_("An error occurred while extracting the cpio archive\nfrom the rpm one. Do you want to view the shell output ?") );
            if (response == GTK_RESPONSE_YES) ShowShellOutput();
            unlink ( tmp );
            g_free (tmp);
            return;
    	}
    }
    if ( DetectArchiveType ( data ) == 1) tmp = OpenTempFile ( 1 , data );
        else tmp = OpenTempFile ( 0 , data );
    g_child_watch_add ( compressor_pid , (GChildWatchFunc) OpenCPIO , data );
}

GChildWatchFunc *OpenCPIO (GPid pid , gint exit_code , gpointer data)
{
    if ( WIFEXITED( exit_code ) )
    {   
	    if ( WEXITSTATUS ( exit_code ) )
    	{
	    	archive_error = TRUE;
    		SetButtonState (1,1,0,0,0);
	    	gtk_window_set_title ( GTK_WINDOW (MainWindow) , "Xarchiver " VERSION );
		    response = ShowGtkMessageDialog (GTK_WINDOW 		(MainWindow),GTK_DIALOG_MODAL,GTK_MESSAGE_QUESTION,GTK_BUTTONS_YES_NO,_("An error occurred while decompressing the cpio archive.\nDo you want to view the shell output ?") );
            if (response == GTK_RESPONSE_YES) ShowShellOutput();
            unlink ( tmp );
            g_free (tmp);
            return;
    	}
    }
    //Delete the tmp gz/bzip2 compressed CPIO archive since we decompressed it
    unlink ( data );
  
    //Now I have to open the CPIO temp file in read mode and spawn the command cpio -tv with an input pipe so to receive the
    //output from the opened CPIO temp file
    SpawnCPIO ( "cpio -tv" , tmp , 0 , 1 );
}

void SpawnCPIO ( gchar *command , gchar* tmp , gboolean add_flag , gboolean input_flag )
{
    compressor_pid = SpawnAsyncProcess ( command , add_flag , input_flag );
    if ( compressor_pid == 0 )
    {
        unlink ( tmp );
        g_free (tmp);
        return;
    }
    SetIOChannel (error_fd, G_IO_IN|G_IO_PRI|G_IO_ERR|G_IO_HUP|G_IO_NVAL , GenError, NULL );
    SetIOChannel (output_fd, G_IO_IN|G_IO_PRI|G_IO_ERR|G_IO_HUP|G_IO_NVAL , ReadCPIOOutput, NULL );
    input_ioc = SetIOChannelEncondingNULL (input_fd, G_IO_IN|G_IO_OUT|G_IO_PRI|G_IO_ERR|G_IO_HUP|G_IO_NVAL , WriteCPIOInput, NULL);

    ioc_cpio = g_io_channel_new_file ( tmp , "r" , NULL );
	g_io_channel_set_encoding (ioc_cpio , NULL , NULL);
    g_io_channel_set_flags ( ioc_cpio , G_IO_FLAG_NONBLOCK , NULL );
    g_child_watch_add ( compressor_pid , (GChildWatchFunc) ExitStatus , NULL );
}

//input pipe
gboolean WriteCPIOInput (GIOChannel *ioc, GIOCondition cond, gpointer data)
{
    if (cond & (G_IO_IN | G_IO_PRI | G_IO_OUT) )
    {
        //Doing so I write to the input pipe of the g_spawned "cpio -tv" so to produce the list of archived files
        status = g_io_channel_read_chars ( ioc_cpio , buffer, sizeof(buffer), &bytes_read, &error );
        //g_print ("Read status: %d\t",status);
        if ( status != G_IO_STATUS_EOF)
        {
            status = g_io_channel_write_chars ( ioc , buffer , bytes_read , &bytes_written , &error );
            //g_print ("Red: %d\tWritten:%d\n",bytes_read,count);
            while ( bytes_read != bytes_written )
            {
                status = g_io_channel_write_chars ( ioc , buffer + bytes_written , bytes_read - bytes_written , &bytes_written , &error );
                //g_print ("*Written:%d\tStatus:%d\n",count,status);
            }
            if (status == G_IO_STATUS_ERROR) 
            {
                response = ShowGtkMessageDialog (GTK_WINDOW (MainWindow),GTK_DIALOG_MODAL,GTK_MESSAGE_ERROR,GTK_BUTTONS_OK,error->message);
                CloseChannels ( ioc_cpio );
                CloseChannels ( ioc );
		        return FALSE; 
            }
            g_io_channel_flush ( ioc , NULL );
            return TRUE;
        }
        else
        {
            CloseChannels ( ioc_cpio );
            CloseChannels ( ioc );
		    return FALSE;
        }
	}
}

//output pipe
gboolean ReadCPIOOutput (GIOChannel *ioc, GIOCondition cond, gpointer data)
{
    num_cols = 0;
	gchar **fields;
    gchar *line;
    gchar *filename;
    //Is there output from "cpio -tv" to read ?
	if (cond & (G_IO_IN | G_IO_PRI) )
	{
		g_io_channel_read_line ( ioc, &line, NULL, NULL , NULL );
		if (line == NULL) return TRUE;
        gtk_text_buffer_insert (textbuf, &enditer, line, strlen ( line ) );
		fields = split_line (line,5);
		filename = get_last_field (line,8);
		gtk_list_store_append (liststore, &iter);
        if ( filename[strlen(filename) - 1] != '/')
		{
		    for ( x = 0; x < 5; x++)
			{
				gtk_list_store_set (liststore, &iter,num_cols+1,fields[x],-1);
				num_cols++;
			}
            filename = strchr (filename, '.');
            gtk_list_store_set (liststore, &iter,0,filename,-1);
		}
		g_strfreev ( fields );
        g_free (line);
		return TRUE;
	}
	else if (cond & (G_IO_ERR | G_IO_HUP | G_IO_NVAL) )
	{
        CloseChannels ( ioc );
        g_spawn_close_pid ( child_pid );
		return FALSE;
	}
}

void CloseChannels ( GIOChannel *ioc )
{
    g_io_channel_shutdown ( ioc,TRUE,NULL );
    g_io_channel_unref (ioc);
}
