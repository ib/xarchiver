/*
 *  Copyright (C) 2006 Giuseppe Torelli <colossus73@gmail.com>
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
 
#include "bzip2.h"
#include "extract_dialog.h"

extern gboolean TarOpen (GIOChannel *ioc, GIOCondition cond, gpointer data);
extern int output_fd;

FILE *stream = NULL;

gchar *tmp = NULL;
int fd;
gboolean type;

void OpenBzip2 ( XArchive *archive )
{
    if ( g_str_has_suffix ( archive->escaped_path , ".tar.bz2") || g_str_has_suffix ( archive->escaped_path , ".tar.bz") || g_str_has_suffix ( archive->escaped_path , ".tbz") || g_str_has_suffix ( archive->escaped_path , ".tbz2" ) )
	{
	    gchar *command = g_strconcat ("tar tfjv " , archive->escaped_path, NULL );
	   	archive->dummy_size = 0;
		archive->nr_of_files = 0;
		archive->nr_of_dirs = 0;
		archive->parse_output = TarOpen;
		SpawnAsyncProcess ( archive , command , 0);
		g_free ( command );
		if ( archive->child_pid == 0 )
			return;

		char *names[]= {(_("Filename")),(_("Permissions")),(_("Owner/Group")),(_("Size")),(_("Date")),(_("Time"))};
		GType types[]= {G_TYPE_STRING,G_TYPE_STRING,G_TYPE_STRING,G_TYPE_UINT64,G_TYPE_STRING,G_TYPE_STRING};
		xa_create_liststore ( 6, names , (GType *)types );
        archive->type = XARCHIVETYPE_TAR_BZ2;
    }
    else 
		Bzip2Extract ( archive , 0 );
}

void Bzip2Extract ( XArchive *archive , gboolean flag )
{
    gchar *text;
    gchar *new_path;
	gchar *command = NULL;
    extract_window = xa_create_extract_dialog ( 0 , archive->type);
	gtk_dialog_set_default_response (GTK_DIALOG (extract_window->dialog1), GTK_RESPONSE_OK);
	done = FALSE;
	while ( ! done )
	{
		switch (gtk_dialog_run ( GTK_DIALOG (extract_window->dialog1 ) ) )
		{
			case GTK_RESPONSE_CANCEL:
			case GTK_RESPONSE_DELETE_EVENT:
			done = TRUE;
			break;
			
			case GTK_RESPONSE_OK:
			extract_path = g_strdup (gtk_entry_get_text ( GTK_ENTRY (extract_window->destination_path_entry) ));
			if ( strlen ( extract_path ) > 0 )
			{
				done = TRUE;
				gchar *archive_name = StripPathFromFilename ( archive->escaped_path );
				archive->parse_output = 0;
				command = g_strconcat ( flag ? "gzip -dc " : "bzip2 -dc " , archive->escaped_path , NULL );
				SpawnAsyncProcess ( archive , command , 0);
				if ( archive->child_pid == 0 )
				{
					g_free ( command );
					return;
				}
				//This to remove the suffix from the archive name
                if (g_str_has_suffix ( archive_name , flag ? ".gz" : ".bz2") ) archive_name [strlen(archive_name) - ( flag ? 3 : 4 ) ] = '\0';
                if (archive_name == NULL)
					new_path = JoinPathArchiveName ( extract_path , archive->escaped_path );
                else
					new_path = JoinPathArchiveName ( extract_path , archive_name );
				stream = fopen ( new_path , "w" );
				g_free ( new_path );
				if ( stream == NULL )
				{
					response = ShowGtkMessageDialog (GTK_WINDOW (MainWindow),GTK_DIALOG_MODAL,GTK_MESSAGE_ERROR,GTK_BUTTONS_OK,strerror(errno));
                    done = FALSE;
                    break;					
				}
                text = g_strconcat (_("Extracting ") , flag ? "gzip" : "bzip2" , _(" file to "), extract_path, NULL );
                Update_StatusBar ( text );
                g_free (text);
				GIOChannel *ioc = g_io_channel_unix_new ( output_fd );
				g_io_channel_set_encoding (ioc, NULL , NULL);
				g_io_channel_set_flags ( ioc , G_IO_FLAG_NONBLOCK , NULL );
				g_io_add_watch (ioc, G_IO_IN|G_IO_PRI|G_IO_ERR|G_IO_HUP|G_IO_NVAL, ExtractToDifferentLocation, stream);
			}
			else
				response = ShowGtkMessageDialog (GTK_WINDOW (MainWindow),GTK_DIALOG_MODAL,GTK_MESSAGE_ERROR,GTK_BUTTONS_OK, _("Please select where to extract files!") );
			break;
    	}
	}
	gtk_widget_destroy ( extract_window->dialog1 );
	g_free (extract_window);
	extract_window = NULL;
	xa_set_button_state (1,1,0,0,0);
	if (command != NULL)
	{
		g_free ( command );
		g_child_watch_add ( archive->child_pid, (GChildWatchFunc)xa_watch_child, archive);    
	}
}

gchar *OpenTempFile ( gboolean dummy , gchar *temp_path )
{
	gchar *command = NULL;
	tmp = g_strdup ("/tmp/xarchiver-XXXXXX");
	fd = g_mkstemp ( tmp );
	stream = fdopen ( fd , "w" );
	if ( stream == NULL)
	{
		response = ShowGtkMessageDialog (GTK_WINDOW (MainWindow),GTK_DIALOG_MODAL,GTK_MESSAGE_ERROR,GTK_BUTTONS_OK,strerror(errno) );
		g_free (tmp);
		return NULL;
	}
	if ( temp_path == NULL)
		command = g_strconcat ( dummy ? "gzip -dc " : "bzip2 -dc " , archive->escaped_path , NULL );
	else
		command = g_strconcat ( dummy ? "gzip -dc " : "bzip2 -dc " , temp_path , NULL );
	//g_print ("1) %s > %s\n",command,tmp);
	archive->parse_output = 0;
	SpawnAsyncProcess ( archive , command , 0 );
	g_free ( command );
	if ( archive->child_pid == 0 )
	{
		fclose ( stream );
		unlink ( tmp );
		g_free (tmp);
		return NULL;
	}
	GIOChannel *ioc = g_io_channel_unix_new ( output_fd );
	g_io_channel_set_encoding (ioc, NULL , NULL);
	g_io_channel_set_flags ( ioc , G_IO_FLAG_NONBLOCK , NULL );
	g_io_add_watch (ioc, G_IO_IN|G_IO_PRI|G_IO_ERR|G_IO_HUP|G_IO_NVAL, ExtractToDifferentLocation, stream);
	return tmp;
}

gboolean ExtractToDifferentLocation (GIOChannel *ioc, GIOCondition cond, gpointer data)
{
	FILE *stream = data;
	gchar buffer[65536];
	gsize bytes_read;
	GError *error = NULL;
    
	if (cond & (G_IO_IN | G_IO_PRI) )
	{
		while (gtk_events_pending() )
			gtk_main_iteration();
		if ( g_io_channel_read_chars ( ioc, buffer, sizeof(buffer), &bytes_read, &error ) != G_IO_STATUS_NORMAL )
		{
			response = ShowGtkMessageDialog (GTK_WINDOW (MainWindow),GTK_DIALOG_MODAL,GTK_MESSAGE_ERROR,GTK_BUTTONS_OK, error->message);
			g_error_free (error);
			return FALSE;
		}
		//Write the content of the bzip/gzip extracted file to the file pointed by the file stream
		fwrite ( buffer, 1 , bytes_read , stream );
		return TRUE;
	}
	else if (cond & (G_IO_ERR | G_IO_HUP | G_IO_NVAL) )
	{
		fclose ( stream );
		g_io_channel_shutdown ( ioc,TRUE,NULL );
		g_io_channel_unref (ioc);
		return FALSE;
	}
	return TRUE;
}

void DecompressBzipGzip ( GString *list , XArchive *archive , gboolean dummy , gboolean add )
{
	gchar *command, *msg;
	pid_t child_pid = 0;
	int status;
	int waiting = TRUE;
	int ps;
	
	tmp = OpenTempFile ( dummy , NULL );
	if ( tmp == NULL )
		return;
	msg = g_strconcat ( _("Decompressing tar file with ") , dummy ? "gzip" : "bzip2" , ", please wait..." , NULL );
	Update_StatusBar ( msg );
	g_free (msg);
	gtk_widget_show (viewport2);
	while (waiting)
	{
		ps = waitpid ( child_pid, &status, WNOHANG);
		if (ps < 0)
			waiting = FALSE;
		else
		{
			//gtk_progress_bar_pulse ( GTK_PROGRESS_BAR (progressbar) );
			while (gtk_events_pending())
				gtk_main_iteration();
		}
	}
	if ( WIFEXITED(status) )
	{
		if ( WEXITSTATUS (status) )
		{
			xa_set_button_state (1,1,0,0,0);
			gtk_window_set_title ( GTK_WINDOW (MainWindow) , "Xarchiver " VERSION );
			response = ShowGtkMessageDialog (GTK_WINDOW (MainWindow),GTK_DIALOG_MODAL,GTK_MESSAGE_QUESTION,GTK_BUTTONS_YES_NO,_("An error occurred while decompressing the archive.\nDo you want to open the error messages window?") );
			if (response == GTK_RESPONSE_YES)
				ShowShellOutput (NULL,FALSE);
			unlink ( tmp );
			g_free (tmp);
			OffTooltipPadlock();
			return;
		}
	}
	if ( add )
		command = g_strconcat ( "tar rvvf " , tmp , list->str , NULL );
    else
		command = g_strconcat ( "tar --delete -f " , tmp , list->str , NULL );
	waiting = TRUE;
	g_print ("2) %s\n",command);
	archive->parse_output = 0;
	SpawnAsyncProcess ( archive , command , 0);
	g_free ( command );
	if ( archive->child_pid == 0 )
	{
		unlink ( tmp );
		g_free (tmp);
		return;
	}
	GIOChannel *ioc = g_io_channel_unix_new ( output_fd );
	g_io_channel_set_encoding (ioc, NULL , NULL);
	g_io_channel_set_flags ( ioc , G_IO_FLAG_NONBLOCK , NULL );
	while (waiting)
	{
		ps = waitpid ( child_pid, &status, WNOHANG);
		if (ps < 0)
			waiting = FALSE;
		else
		{
			//gtk_progress_bar_pulse ( GTK_PROGRESS_BAR (progressbar) );
			while (gtk_events_pending())
				gtk_main_iteration();
		}
	}
	if ( WIFEXITED(status) )
	{
		if ( WEXITSTATUS (status) )
		{
			xa_set_button_state (1,1,0,0,0);
			gtk_window_set_title ( GTK_WINDOW (MainWindow) , "Xarchiver " VERSION );
			response = ShowGtkMessageDialog (GTK_WINDOW (MainWindow),GTK_DIALOG_MODAL,GTK_MESSAGE_QUESTION,GTK_BUTTONS_YES_NO, add ? _("An error occurred while adding to the tar archive.\nDo you want to open the error messages window?") : _("An error occurred while deleting from the tar archive.\nDo you want to open the error messages window?") );
			if (response == GTK_RESPONSE_YES)
				ShowShellOutput (NULL,FALSE);
            unlink ( tmp );
            g_free (tmp);
            OffTooltipPadlock();
            return;
        }
    }
    msg = g_strconcat ( _("Recompressing tar file with ") , dummy ? "gzip" : "bzip2" , ", please wait..." , NULL );
    Update_StatusBar ( msg );
    g_free (msg);
    RecompressArchive ( archive , status , dummy );
}

void RecompressArchive (XArchive *archive , gint status , gboolean dummy)
{
    if ( WIFEXITED(status) )
	{
		if ( WEXITSTATUS (status) )
		{
			xa_set_button_state (1,1,0,0,0);
			gtk_window_set_title ( GTK_WINDOW (MainWindow) , "Xarchiver " VERSION );
			response = ShowGtkMessageDialog (GTK_WINDOW
			(MainWindow),GTK_DIALOG_MODAL,GTK_MESSAGE_QUESTION,GTK_BUTTONS_YES_NO,_("An error occurred while recompressing the tar archive.\nDo you want to open the error messages window?") );
			if (response == GTK_RESPONSE_YES)
				ShowShellOutput (NULL,FALSE);
			unlink ( tmp );
            g_free (tmp);
            OffTooltipPadlock();
            return;
		}
	}
	//Recompress the temp archive in the original archive overwriting it
	stream = fopen ( archive->path , "w" ) ;
	if ( stream == NULL)
	{
		response = ShowGtkMessageDialog (GTK_WINDOW (MainWindow),GTK_DIALOG_MODAL,GTK_MESSAGE_ERROR,GTK_BUTTONS_OK,strerror(errno) );
		unlink ( tmp );
		g_free (tmp);
		return;
	}
	gchar *command = g_strconcat ( dummy ? "gzip -c " : "bzip2 -c " , tmp , NULL );
	//g_print ("3) %s > %s\n",command,archive->escaped_path);
	archive->parse_output = 0;
	SpawnAsyncProcess ( archive , command , 0 );
	g_free ( command );
	if ( archive->child_pid == 0 )
	{
		unlink ( tmp );
		g_free (tmp);
		return;
	}
	GIOChannel *ioc = g_io_channel_unix_new ( output_fd );
	g_io_channel_set_encoding (ioc, NULL , NULL);
	g_io_channel_set_flags ( ioc , G_IO_FLAG_NONBLOCK , NULL );
	g_io_add_watch (ioc, G_IO_IN|G_IO_PRI|G_IO_ERR|G_IO_HUP|G_IO_NVAL, ExtractToDifferentLocation, stream );
	
	archive->tmp = tmp;
	g_child_watch_add ( archive->child_pid, (GChildWatchFunc)xa_watch_child, archive);
}

void Bzip2Add ( gchar *filename , XArchive *archive , gboolean flag )
{
    stream = fopen ( archive->path , "w" );
	if ( stream == NULL )
	{
		response = ShowGtkMessageDialog (GTK_WINDOW (MainWindow),GTK_DIALOG_MODAL,GTK_MESSAGE_ERROR,GTK_BUTTONS_OK,strerror(errno));
        done = FALSE;
        return;					
	}
    gtk_widget_show ( viewport2 );
    gchar *command = g_strconcat ( flag ? "gzip -c " : "bzip2 -c " , filename , NULL );
	archive->parse_output = 0;
	SpawnAsyncProcess ( archive , command , 0);
	g_free ( command );
	if ( archive->child_pid == 0 )
		return;

	GIOChannel *ioc = g_io_channel_unix_new ( output_fd );
    g_io_channel_set_encoding (ioc, NULL , NULL);
	g_io_channel_set_flags ( ioc , G_IO_FLAG_NONBLOCK , NULL );
    g_io_add_watch (ioc, G_IO_IN|G_IO_PRI|G_IO_ERR|G_IO_HUP|G_IO_NVAL, ExtractToDifferentLocation, stream );
	g_child_watch_add ( archive->child_pid, (GChildWatchFunc)xa_watch_child, archive);
}

