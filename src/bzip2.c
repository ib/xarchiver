/*
 *  Copyright (C) 2006 Giuseppe Torelli <colossus73@gmail.com>
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
#include "bzip2.h"
#include "extract_dialog.h"

extern gboolean TarOpen (GIOChannel *ioc, GIOCondition cond, gpointer data);
extern int output_fd;
extern gboolean cli;

FILE *stream = NULL;
gchar *tmp = NULL;
int fd;
int l;
gboolean error_output,result;

void OpenBzip2 ( XArchive *archive )
{
    if ( g_str_has_suffix ( archive->escaped_path , ".tar.bz2") || g_str_has_suffix ( archive->escaped_path , ".tar.bz") || g_str_has_suffix ( archive->escaped_path , ".tbz") || g_str_has_suffix ( archive->escaped_path , ".tbz2" ) )
	{
		gchar *command;
		gchar *tar;
    
		tar = g_find_program_in_path ("gtar");
		if (tar == NULL)
		tar = g_strdup ("tar");

		command = g_strconcat (tar, " tfjv " , archive->escaped_path, NULL );
		archive->dummy_size = 0;
		archive->nr_of_files = 0;
		archive->nr_of_dirs = 0;
		archive->format = "TAR.BZIP2";
		archive->parse_output = TarOpen;

		SpawnAsyncProcess ( archive , command , 0, 0);

		g_free (command);
		g_free (tar);

		if ( archive->child_pid == 0 )
			return;

		char *names[]= {(_("Filename")),(_("Soft Link")),(_("Permissions")),(_("Owner/Group")),(_("Size")),(_("Date")),(_("Time"))};
		GType types[]= {G_TYPE_STRING,G_TYPE_STRING,G_TYPE_STRING,G_TYPE_STRING,G_TYPE_UINT64,G_TYPE_STRING,G_TYPE_STRING};
		xa_create_liststore ( 7, names , (GType *)types );
        archive->type = XARCHIVETYPE_TAR_BZ2;
    }
    else
	{
		Bzip2Extract ( archive , 0 );
		archive->format ="BZIP2";
	}
}

void Bzip2Extract ( XArchive *archive , gboolean flag )
{
    gchar *text;
	gchar *command = NULL;
	extract_window = xa_create_extract_dialog ( 0 , archive);
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
			archive->extraction_path = g_strdup (gtk_entry_get_text ( GTK_ENTRY (extract_window->destination_path_entry) ));
			if ( strlen ( archive->extraction_path ) > 0 )
			{
				done = TRUE;
				archive->parse_output = 0;
				command = g_strconcat ( flag ? "gzip -dc " : "bzip2 -dc " , archive->escaped_path , NULL );
				SpawnAsyncProcess ( archive , command , 0, 0);
				if ( archive->child_pid == 0 )
				{
					g_free ( command );
					return;
				}
				if (flag)
					text = g_strdup_printf(_("Extracting gzip file to %s"), archive->extraction_path);
				else
					text = g_strdup_printf(_("Extracting bzip2 file to %s"), archive->extraction_path);
				Update_StatusBar ( text );
				g_free (text);

				stream = fopen ( archive->extraction_path , "w" );
				if ( stream == NULL )
				{
					response = ShowGtkMessageDialog (GTK_WINDOW (MainWindow),GTK_DIALOG_MODAL,GTK_MESSAGE_ERROR,GTK_BUTTONS_OK,_("Can't write file:"),g_strerror(errno));
                    done = FALSE;
                    break;					
				}
				GIOChannel *ioc = g_io_channel_unix_new ( output_fd );
				g_io_channel_set_encoding (ioc, NULL , NULL);
				g_io_channel_set_flags ( ioc , G_IO_FLAG_NONBLOCK , NULL );
				g_io_add_watch (ioc, G_IO_IN|G_IO_PRI|G_IO_ERR|G_IO_HUP|G_IO_NVAL, ExtractToDifferentLocation, stream);
			}
			else
				response = ShowGtkMessageDialog (GTK_WINDOW (MainWindow),GTK_DIALOG_MODAL,GTK_MESSAGE_ERROR,GTK_BUTTONS_OK, _("You missed the extraction path!"),("Please type it.") );
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
	else
	{
		archive->status = XA_ARCHIVESTATUS_IDLE;
		gtk_widget_set_sensitive (Stop_button, FALSE);
		gtk_widget_hide ( viewport2 );
		Update_StatusBar ( _("Operation canceled."));
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
		response = ShowGtkMessageDialog (GTK_WINDOW (MainWindow),GTK_DIALOG_MODAL,GTK_MESSAGE_ERROR,GTK_BUTTONS_OK,_("Can't write to /tmp:"),g_strerror(errno) );
		g_free (tmp);
		return NULL;
	}
	if ( temp_path == NULL)
		command = g_strconcat ( dummy ? "gzip -dc " : "bzip2 -dc " , archive->escaped_path , NULL );
	else
		command = g_strconcat ( dummy ? "gzip -dc " : "bzip2 -dc " , temp_path , NULL );
	archive->parse_output = 0;
	SpawnAsyncProcess ( archive , command , 0, 0);
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

void xa_add_delete_tar_bzip2_gzip ( GString *list , XArchive *archive , gboolean dummy , gboolean add )
{
	gchar *command, *msg, *tar,*temp_name;
	gtk_widget_show (viewport2);
	msg = g_strdup_printf(_("Decompressing tar file with %s, please wait...") , dummy ? "gzip" : "bzip2");
	Update_StatusBar ( msg );
	g_free (msg);
	command = g_strconcat (dummy ? "gzip " : "bzip2 ", "-f -d ",archive->escaped_path,NULL);
	result = xa_run_command (command , 0);
	g_free (command);
	if (result == 0)
		return;

	tar = g_find_program_in_path ("gtar");
	if (tar == NULL)
		tar = g_strdup ("tar");
	temp_name = g_strdup (archive->escaped_path);
	l = strlen (temp_name);
	
	if (file_extension_is (archive->escaped_path,".tar.bz2") )
		temp_name[l - 4] = 0;
	else if (file_extension_is (archive->escaped_path,".tbz2") )
	{
		temp_name[l - 3] = 'a';
		temp_name[l - 2] = 'r';
		temp_name[l - 1] = 0;
	}
	else if (file_extension_is (archive->escaped_path,".tar.gz") )
		temp_name[l - 3] = 0;

	else if (file_extension_is (archive->escaped_path,".tgz") || file_extension_is (archive->escaped_path, ".tbz") )
	{
		temp_name[l - 2] = 'a';
		temp_name[l - 1] = 'r';
	}

	if ( add )
		command = g_strconcat (tar, " ",
							archive->add_recurse ? "" : "--no-recursion ",
							archive->remove_files ? "--remove-files " : "",
							archive->update ? "-uvvf " : "-rvvf ",
							temp_name,
							list->str , NULL );
	else
		command = g_strconcat (tar, " --delete -f " , temp_name , list->str , NULL );
	
	result = xa_run_command (command , 0);
	g_free (command);
	g_free (tar);
	if (result == 0)
		return;

	msg = g_strdup_printf(_("Recompressing tar file with %s, please wait...") , dummy ? "gzip" : "bzip2");
	Update_StatusBar ( msg );
	g_free (msg);
	
	command = g_strconcat ( dummy ? "gzip " : "bzip2 ", "-f" , temp_name , NULL );
	result = xa_run_command (command , 1);
	g_free (command);
	g_free (temp_name);
	if (result == 0)
		return;
}

void Bzip2Add ( gchar *filename , XArchive *archive , gboolean flag )
{
	gchar *command = NULL;
	gtk_widget_show ( viewport2 );
	command = g_strconcat ( flag ? "gzip -f " : "bzip2 -zk " , filename , NULL );
	if ( ! cli)
	{
		result = xa_run_command (command , 0);
		g_free (command);
		if ( result == 0 )
			return;
	}
	else
	{
		error_output = SpawnSyncCommand ( command );
		g_free (command);
		if (error_output == FALSE)
			return;
	}

	command = g_strconcat ( "mv -f " , filename , flag ? ".gz" : ".bz2 ", " " , archive->escaped_path , NULL );
	if (! cli)
	{
		result = xa_run_command (command , 1);
		g_free (command);
	}
	else
	{
		error_output = SpawnSyncCommand ( command );
		g_free (command);
	}
}

gboolean ExtractToDifferentLocation (GIOChannel *ioc, GIOCondition cond, gpointer data)
{
	FILE *stream = data;
	gchar buffer[65536];
	gsize bytes_read;
	GIOStatus status;
	GError *error = NULL;

	if (cond & (G_IO_IN | G_IO_PRI) )
	{
		do
	    {
			status = g_io_channel_read_chars (ioc, buffer, sizeof(buffer), &bytes_read, &error);
			if (bytes_read > 0)
			{
				/* Write the content of the bzip/gzip extracted file to the file pointed by the file stream */
				fwrite (buffer, 1, bytes_read, stream);
			}
			else if (error != NULL)
			{
			response = ShowGtkMessageDialog (GTK_WINDOW (MainWindow),GTK_DIALOG_MODAL,GTK_MESSAGE_ERROR,GTK_BUTTONS_OK, _("An error occurred:"),error->message);
			g_error_free (error);
			return FALSE;
			}
		}
		while (status == G_IO_STATUS_NORMAL);

		if (status == G_IO_STATUS_ERROR || status == G_IO_STATUS_EOF)
		goto done;
	}
	else if (cond & (G_IO_ERR | G_IO_HUP | G_IO_NVAL) )
	{
		done:
		fclose ( stream );
		g_io_channel_shutdown ( ioc,TRUE,NULL );
		g_io_channel_unref (ioc);
		return FALSE;
	}
	return TRUE;
}

/* Taken from fileroller */
gboolean file_extension_is (const char *filename, const char *ext)
{
	int filename_l, ext_l;

	filename_l = strlen (filename);
	ext_l = strlen (ext);

    if (filename_l < ext_l)
		return FALSE;
    return strcasecmp (filename + filename_l - ext_l, ext) == 0;
}
/* End code from fileroller */

