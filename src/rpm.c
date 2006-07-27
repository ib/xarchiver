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
#include "rpm.h"

extern int output_fd, input_fd;
FILE *stream;
int fd;
gchar *cpio_tmp = NULL;
gchar buffer[2048];
gsize bytes_read = 0;
gsize bytes_written = 0;
GIOStatus status;
GError *error = NULL;
GIOChannel *ioc_cpio , *input_ioc, *output_ioc;

void OpenRPM ( XArchive *archive )
{
	unsigned char bytes[8];
    int dl,il,sigsize,offset;
    gchar *ibs;
    
    signal (SIGPIPE, SIG_IGN);
    stream = fopen ( archive->path , "r" );
	if (stream == NULL)
    {
        gchar *msg = g_strdup_printf (_("Can't open RPM file %s:") , archive->path); 
		response = ShowGtkMessageDialog (GTK_WINDOW (MainWindow) , GTK_DIALOG_MODAL,GTK_MESSAGE_ERROR,GTK_BUTTONS_OK,
		msg,g_strerror (errno));
		g_free (msg);
		return;
    }
    archive->dummy_size = 0;
    archive->nr_of_files = 0;
    archive->nr_of_dirs = 0;
	archive->format ="RPM";
	char *names[]= {(_("Filename")),(_("Permission")),(_("Hard Link")),(_("Owner")),(_("Group")),(_("Size"))};
	GType types[]= {G_TYPE_STRING,G_TYPE_STRING,G_TYPE_STRING,G_TYPE_STRING,G_TYPE_STRING,G_TYPE_UINT64};
	xa_create_liststore ( 6, names , (GType *)types );
    if (fseek ( stream, 104 , SEEK_CUR ) )
    {
        fclose (stream);
        response = ShowGtkMessageDialog (GTK_WINDOW (MainWindow),GTK_DIALOG_MODAL,GTK_MESSAGE_ERROR,GTK_BUTTONS_OK,_("Can't fseek to position 104:"),g_strerror(errno));
        return;
    }
    if ( fread ( bytes, 1, 8, stream ) == 0 )
	{
		fclose ( stream );
		response = ShowGtkMessageDialog (GTK_WINDOW (MainWindow),GTK_DIALOG_MODAL,GTK_MESSAGE_ERROR,GTK_BUTTONS_OK,_("Can't read data from file:"),g_strerror(errno));
		return;
    }
    il = 256 * ( 256 * ( 256 * bytes[0] + bytes[1]) + bytes[2] ) + bytes[3];
    dl = 256 * ( 256 * ( 256 * bytes[4] + bytes[5]) + bytes[6] ) + bytes[7];
    sigsize = 8 + 16 * il + dl;
    offset = 104 + sigsize + ( 8 - ( sigsize % 8 ) ) % 8 + 8;
    if (fseek ( stream, offset  , SEEK_SET ) )
    {
        fclose (stream);
        response = ShowGtkMessageDialog (GTK_WINDOW (MainWindow),GTK_DIALOG_MODAL,GTK_MESSAGE_ERROR,GTK_BUTTONS_OK,_("Can't fseek in file:"),g_strerror(errno));
        return;
    }
    if ( fread ( bytes, 1, 8, stream ) == 0 )
	{
		fclose ( stream );
		response = ShowGtkMessageDialog (GTK_WINDOW (MainWindow),GTK_DIALOG_MODAL,GTK_MESSAGE_ERROR,GTK_BUTTONS_OK,_("Can't read data from file:"),g_strerror(errno));
		return;
    }
    il = 256 * ( 256 * ( 256 * bytes[0] + bytes[1]) + bytes[2] ) + bytes[3];
    dl = 256 * ( 256 * ( 256 * bytes[4] + bytes[5]) + bytes[6] ) + bytes[7];
	sigsize = 8 + 16 * il + dl;
	offset = offset + sigsize;
	fclose (stream);

	cpio_tmp = g_strdup ("/tmp/xarchiver-XXXXXX");
	fd = g_mkstemp ( cpio_tmp );
	ibs = g_strdup_printf ( "%u" , offset );

	//Now I run dd to have the bzip2 / gzip compressed cpio archive in /tmp
	gchar *command = g_strconcat ( "dd if=" , archive->escaped_path, " ibs=" , ibs , " skip=1 of=" , cpio_tmp , NULL );
	g_free (ibs);
	archive->parse_output = 0;
	SpawnAsyncProcess ( archive , command , 0, 0);
	g_free ( command );
	if ( archive->child_pid == 0 )
	{
		fclose ( stream );
		unlink ( cpio_tmp );
		g_free (cpio_tmp);
		return;
	}
	g_child_watch_add ( archive->child_pid , (GChildWatchFunc) DecompressCPIO , cpio_tmp );
}

GChildWatchFunc *DecompressCPIO (GPid pid , gint status , gpointer data)
{
	gchar *gzip = data;
	if ( WIFEXITED(status) )
	{   
		if ( WEXITSTATUS (status) )
		{
            Update_StatusBar ( _("Operation failed."));
            gtk_widget_hide ( viewport2 );
	    	gtk_window_set_title ( GTK_WINDOW (MainWindow) , "Xarchiver " VERSION );
		    response = ShowGtkMessageDialog (GTK_WINDOW 		(MainWindow),GTK_DIALOG_MODAL,GTK_MESSAGE_QUESTION,GTK_BUTTONS_YES_NO,_("Error while extracting the cpio archive from the rpm one."),("Do you want to open the error messages window?") );
            if (response == GTK_RESPONSE_YES)
				ShowShellOutput (NULL);
            unlink ( cpio_tmp );
            g_free (cpio_tmp);
			xa_set_button_state (1,1,0,0,0);
			gtk_widget_set_sensitive (Stop_button,FALSE);
			archive->status = XA_ARCHIVESTATUS_IDLE;
            return FALSE;
    	}
    }
	cpio_tmp = OpenTempFile ( 1 , gzip );
    if (cpio_tmp != NULL)
		g_child_watch_add ( archive->child_pid , (GChildWatchFunc) OpenCPIO , gzip );
    else
		return FALSE;
    return NULL;
}

GChildWatchFunc *OpenCPIO (GPid pid , gint exit_code , gpointer data)
{
	gchar *gzip = data;
    if ( WIFEXITED( exit_code ) )
    {   
	    if ( WEXITSTATUS ( exit_code ) )
    	{
            Update_StatusBar ( _("Operation failed."));
            gtk_widget_hide ( viewport2 );
	    	gtk_window_set_title ( GTK_WINDOW (MainWindow) , "Xarchiver " VERSION );
		    response = ShowGtkMessageDialog (GTK_WINDOW 		(MainWindow),GTK_DIALOG_MODAL,GTK_MESSAGE_QUESTION,GTK_BUTTONS_YES_NO,_("An error occurred while decompressing the cpio archive."),_("Do you want to open the error messages window?") );
			if (response == GTK_RESPONSE_YES)
				ShowShellOutput (NULL);
			unlink ( cpio_tmp );
			unlink ( gzip );
			g_free (cpio_tmp);
			xa_set_button_state (1,1,0,0,0);
			gtk_widget_set_sensitive (Stop_button,FALSE);
			archive->status = XA_ARCHIVESTATUS_IDLE;
			return FALSE;
		}
	}
	/* Let's delete the tmp gzip compressed CPIO archive since we don't need it anymore */
	unlink ( gzip );

	/* Now I have to open the CPIO temp file in read mode and spawn the
	command cpio -tv with an input pipe so to receive the output from
	the opened CPIO temp file */

	archive->parse_output = 0;
	SpawnAsyncProcess ( archive , "cpio -tv" , 1, 0 );
	if ( archive->child_pid == 0 )
	{
		unlink ( cpio_tmp );
		g_free ( cpio_tmp );
		return FALSE;
	}
	output_ioc = g_io_channel_unix_new ( output_fd );
	g_io_add_watch (output_ioc, G_IO_IN|G_IO_PRI|G_IO_ERR|G_IO_HUP|G_IO_NVAL, ReadCPIOOutput, archive );
	g_io_channel_set_encoding (output_ioc, locale , NULL);
	g_io_channel_set_flags ( output_ioc , G_IO_FLAG_NONBLOCK , NULL );

	input_ioc = g_io_channel_unix_new ( input_fd );
	g_io_add_watch (input_ioc, G_IO_IN|G_IO_OUT|G_IO_PRI|G_IO_ERR|G_IO_HUP|G_IO_NVAL, WriteCPIOInput, NULL );
	g_io_channel_set_encoding (input_ioc, NULL , NULL);

	ioc_cpio = g_io_channel_new_file ( cpio_tmp , "r" , NULL );
	g_io_channel_set_encoding (ioc_cpio , NULL , NULL);
	g_io_channel_set_flags ( ioc_cpio , G_IO_FLAG_NONBLOCK , NULL );

	g_child_watch_add ( archive->child_pid, (GChildWatchFunc) xa_watch_child, archive);
  return NULL;
}

/* input pipe */
gboolean WriteCPIOInput (GIOChannel *ioc, GIOCondition cond, gpointer data)
{
	if (cond & (G_IO_IN | G_IO_PRI | G_IO_OUT) )
    {
		/* Doing so I write to the input pipe of the g_spawned "cpio -tv" so to produce the list of files in the cpio archive */
		status = g_io_channel_read_chars ( ioc_cpio , buffer, sizeof(buffer), &bytes_read, &error); 
		if ( status != G_IO_STATUS_EOF)
		{
			status = g_io_channel_write_chars ( ioc , buffer , bytes_read , &bytes_written , &error );
			if (status == G_IO_STATUS_ERROR) 
			{
				response = ShowGtkMessageDialog (GTK_WINDOW (MainWindow),GTK_DIALOG_MODAL,GTK_MESSAGE_ERROR,GTK_BUTTONS_OK,_("An error occurred:"),error->message);
				g_error_free (error);
				CloseChannels ( ioc_cpio );
				CloseChannels ( ioc );
				return FALSE; 
			}
			/*
			while ( bytes_read != bytes_written )
				status = g_io_channel_write_chars ( ioc , buffer + bytes_written , bytes_read - bytes_written , &bytes_written , &error );
			g_print ("*Written:%d\tStatus:%d\n",bytes_written,status);
			*/
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
  return TRUE;
}

/* output pipe */
gboolean ReadCPIOOutput (GIOChannel *ioc, GIOCondition cond, gpointer data)
{
	XArchive *archive = data;
	gchar *line = NULL;
	gchar *start = NULL;
	gchar *end = NULL;
	GValue *filename = NULL;
	GValue *size = NULL;
	gchar *_size = NULL;
	GValue *permissions = NULL;
	GValue *hard_link = NULL;
	GValue *owner = NULL;
	GValue *group = NULL;
  GIOStatus status;

    /* Is there output from "cpio -tv" to read ? */
	if (cond & (G_IO_IN | G_IO_PRI) )
	{
    do
    {
      status = g_io_channel_read_line ( ioc, &line, NULL, NULL , NULL );
      if (line == NULL)
        break;

      filename    = g_new0(GValue, 1);
      permissions = g_new0(GValue, 1);
      owner       = g_new0(GValue, 1);
      group       = g_new0(GValue, 1);
      size        = g_new0(GValue, 1);
      hard_link   = g_new0(GValue, 1);

      start = eat_spaces (line);
      end = strchr (start, ' ');
      permissions = g_value_init(permissions, G_TYPE_STRING);
      g_value_set_string ( permissions , g_strndup ( start , end - start) );

      start = eat_spaces (end);
      end = strchr (start, ' ');
      hard_link = g_value_init (hard_link, G_TYPE_STRING);
      g_value_set_string ( hard_link , g_strndup ( start , end - start) );

      start = eat_spaces (end);
      end = strchr (start, ' ');
      owner = g_value_init (owner, G_TYPE_STRING);
      g_value_set_string ( owner , g_strndup ( start , end - start) );

      start = eat_spaces (end);
      end = strchr (start, ' ');
      group = g_value_init (group, G_TYPE_STRING);
      g_value_set_string ( group , g_strndup ( start , end - start) );

      start = eat_spaces (end);
      end = strchr (start, ' ');
      size = g_value_init(size, G_TYPE_UINT64);
      _size  = g_strndup ( start , end - start);
      g_value_set_uint64 (size , atoll (_size) );
      g_free (_size);

      start = eat_spaces (end);
      end = strchr (start, ' ');
      start = eat_spaces (end);
      end = strchr (start, ' ');
      start = eat_spaces (end);
      end = strchr (start, ' ');

      start = eat_spaces (end);
      end = strchr (start, '\n');
      filename = g_value_init (filename, G_TYPE_STRING);
      g_value_set_string ( filename , g_strndup ( start , end - start) );

      archive->row = g_list_prepend (archive->row,filename);
      archive->row = g_list_prepend (archive->row,permissions);
      archive->row = g_list_prepend (archive->row,hard_link);
      archive->row = g_list_prepend (archive->row,owner);
      archive->row = g_list_prepend (archive->row,group);
      archive->row = g_list_prepend (archive->row,size);

		if (  g_str_has_prefix (g_value_get_string (permissions) , "d") == FALSE)
			archive->nr_of_files++;
        else
			archive->nr_of_dirs++;
		archive->dummy_size += g_value_get_uint64 (size);

      g_free (line);
      archive->row_cnt++;
      if (archive->row_cnt > 99)
      {
        xa_append_rows ( archive , 6 );
        archive->row_cnt = 0;
      }
    }
    while (status == G_IO_STATUS_NORMAL);

    if (status == G_IO_STATUS_ERROR || status == G_IO_STATUS_EOF)
      goto done;
	}
	else if (cond & (G_IO_ERR | G_IO_HUP | G_IO_NVAL) )
	{
done:
		g_io_channel_shutdown ( ioc,TRUE,NULL );
		g_io_channel_unref (ioc);
		xa_append_rows ( archive , 6 );
		archive->tmp = cpio_tmp;
		return FALSE;
	}
	return TRUE;
}

void CloseChannels ( GIOChannel *ioc )
{
    g_io_channel_shutdown ( ioc,TRUE,NULL );
    g_io_channel_unref (ioc);
}
