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
#include "rpm.h"
#include "string_utils.h"

FILE *stream;
gchar *cpio_tmp,*gzip_tmp = NULL;
gchar tmp_dir[14] = "";
GIOStatus status;
gboolean result;
GError *error = NULL;
GIOChannel *ioc_cpio;
void xa_get_cpio_line_content (gchar *line, gpointer data);

void xa_open_rpm (XArchive *archive)
{
	unsigned char bytes[8];
	unsigned short int i;
    int dl,il,sigsize,offset;
    gchar *ibs;

    signal (SIGPIPE, SIG_IGN);
    stream = fopen ( archive->path , "r" );
	if (stream == NULL)
    {
        gchar *msg = g_strdup_printf (_("Can't open RPM file %s:") , archive->path);
		response = xa_show_message_dialog (GTK_WINDOW (MainWindow) , GTK_DIALOG_MODAL,GTK_MESSAGE_ERROR,GTK_BUTTONS_OK,
		msg,g_strerror (errno));
		g_free (msg);
		return;
    }
    archive->can_extract = archive->has_properties = TRUE;
    archive->can_add = archive->has_sfx = archive->has_test = FALSE;
    archive->dummy_size = 0;
    archive->nr_of_files = 0;
    archive->nr_of_dirs = 0;
    archive->nc = 8;
	archive->format ="RPM";

	char *names[]= {(_("Points to")),(_("Size")),(_("Permission")),(_("Date")),(_("Hard Link")),(_("Owner")),(_("Group")),NULL};
	GType types[]= {GDK_TYPE_PIXBUF,G_TYPE_STRING,G_TYPE_STRING,G_TYPE_UINT64,G_TYPE_STRING,G_TYPE_STRING,G_TYPE_STRING,G_TYPE_STRING,G_TYPE_STRING,G_TYPE_POINTER};
	archive->column_types = g_malloc0(sizeof(types));
	for (i = 0; i < 10; i++)
		archive->column_types[i] = types[i];

	xa_create_liststore (archive,names);
    if (fseek ( stream, 104 , SEEK_CUR ) )
    {
        fclose (stream);
        response = xa_show_message_dialog (GTK_WINDOW (MainWindow),GTK_DIALOG_MODAL,GTK_MESSAGE_ERROR,GTK_BUTTONS_OK,_("Can't fseek to position 104:"),g_strerror(errno));
        return;
    }
    if ( fread ( bytes, 1, 8, stream ) == 0 )
	{
		fclose ( stream );
		response = xa_show_message_dialog (GTK_WINDOW (MainWindow),GTK_DIALOG_MODAL,GTK_MESSAGE_ERROR,GTK_BUTTONS_OK,_("Can't read data from file:"),g_strerror(errno));
		return;
    }
    il = 256 * ( 256 * ( 256 * bytes[0] + bytes[1]) + bytes[2] ) + bytes[3];
    dl = 256 * ( 256 * ( 256 * bytes[4] + bytes[5]) + bytes[6] ) + bytes[7];
    sigsize = 8 + 16 * il + dl;
    offset = 104 + sigsize + ( 8 - ( sigsize % 8 ) ) % 8 + 8;
    if (fseek ( stream, offset  , SEEK_SET ) )
    {
        fclose (stream);
        response = xa_show_message_dialog (GTK_WINDOW (MainWindow),GTK_DIALOG_MODAL,GTK_MESSAGE_ERROR,GTK_BUTTONS_OK,_("Can't fseek in file:"),g_strerror(errno));
        return;
    }
    if ( fread ( bytes, 1, 8, stream ) == 0 )
	{
		fclose ( stream );
		response = xa_show_message_dialog (GTK_WINDOW (MainWindow),GTK_DIALOG_MODAL,GTK_MESSAGE_ERROR,GTK_BUTTONS_OK,_("Can't read data from file:"),g_strerror(errno));
		return;
    }
    il = 256 * ( 256 * ( 256 * bytes[0] + bytes[1]) + bytes[2] ) + bytes[3];
    dl = 256 * ( 256 * ( 256 * bytes[4] + bytes[5]) + bytes[6] ) + bytes[7];
	sigsize = 8 + 16 * il + dl;
	offset = offset + sigsize;
	fclose (stream);

	/* Create a unique temp dir in /tmp */
	result = xa_create_temp_directory (tmp_dir);
	if (result == 0)
		return;

	archive->tmp = g_strdup(tmp_dir);
	gzip_tmp = g_strconcat (tmp_dir,"/file.gz_bz",NULL);
	ibs = g_strdup_printf ( "%u" , offset );

	//Now I run dd to have the bzip2 / gzip compressed cpio archive in /tmp
	gchar *command = g_strconcat ( "dd if=" , archive->escaped_path, " ibs=" , ibs , " skip=1 of=" , gzip_tmp , NULL );
	g_free (ibs);
	result = xa_run_command (archive,command,0);
	g_free (command);
	if (result == FALSE)
	{	
		fclose (stream);
		g_free (gzip_tmp);
		xa_delete_temp_directory (archive,1);
		return;
	}
	/* Let's decompress the gzip/bzip2 resulting file*/
	xa_open_temp_file ( tmp_dir,gzip_tmp );
}

GChildWatchFunc *xa_open_cpio (GPid pid , gint exit_code , gpointer data)
{
	gint current_page;
	gint idx;
	gchar *command;

	current_page = gtk_notebook_get_current_page(notebook);
	idx = xa_find_archive_index (current_page);
	gchar *gzip = data;
	
    if (WIFEXITED( exit_code) )
    {
	    if ( WEXITSTATUS (exit_code) )
    	{
            Update_StatusBar ( _("Operation failed."));
            gtk_widget_hide ( viewport2 );
	    	xa_set_window_title (MainWindow , NULL);
		    response = xa_show_message_dialog (GTK_WINDOW (MainWindow),GTK_DIALOG_MODAL,GTK_MESSAGE_QUESTION,GTK_BUTTONS_YES_NO,_("An error occurred while decompressing the cpio archive."),_("Do you want to view the command line output?") );
			if (response == GTK_RESPONSE_YES)
				xa_show_cmd_line_output (NULL);
			xa_delete_temp_directory (archive[idx],1);
			g_free (cpio_tmp);
			xa_set_button_state (1,1,GTK_WIDGET_IS_SENSITIVE(close1),0,0,0,0,0);
			//TODO:
			//xa_hide_progress_bar_stop_button(archive[idx]);
			return FALSE;
		}
	}

	command = g_strconcat ("cpio -tv --file ",gzip,NULL);
	g_free(gzip);
	archive[idx]->parse_output = xa_get_cpio_line_content;
	xa_spawn_async_process ( archive[idx],command,1);
	g_free(command);
	if ( archive[idx]->child_pid == 0 )
	{
		xa_delete_temp_directory (archive[idx],1);
		g_free ( cpio_tmp );
		return FALSE;
	}

  return NULL;
}

void xa_get_cpio_line_content (gchar *line, gpointer data)
{
	XArchive *archive = data;
	XEntry *entry;
	gchar *filename;
	gpointer item[7];
	gint n = 0, a = 0 ,linesize = 0;
	gboolean dir = FALSE;

	linesize = strlen(line);

	/* Permissions */
	line[10] = '\0';
	item[2] = line;
	a = 11;

	/* Hard Link */
	for(n=a; n < linesize && line[n] == ' '; ++n);
	line[++n] = '\0';
	item[4] = line + a;
	n++;
	a = n;

	/* Owner */
	for(; n < linesize && line[n] != ' '; ++n);
	line[n] = '\0';
	item[5] = line + a;
	n++;

	/* Group */
	for(; n < linesize && line[n] == ' '; ++n);
	a = n;

	for(; n < linesize && line[n] != ' '; ++n);
	line[n] = '\0';
	item[6] = line + a;
	n++;

	/* Size */	
	for(; n < linesize && line[n] == ' '; ++n);
	a = n;

	for(; n < linesize && line[n] != ' '; ++n);
	line[n] = '\0';
	item[1] = line + a;
	archive->dummy_size += strtoll(item[1],NULL,0);
	n++;
	
	/* Date */
	line[54] = '\0';
	item[3] = line + n;
	n = 55;

	line[linesize-1] = '\0';
	filename = line + n;

	/* Symbolic link */
	gchar *temp = g_strrstr (filename,"->"); 
	if (temp) 
	{
		a = 3;
		gint len = strlen(filename) - strlen(temp);
		item[0] = filename + a + len;
		filename[strlen(filename) - strlen(temp)] = '\0';
	}
	else
		item[0] = NULL;

	if(line[0] == 'd')
	{
		dir = TRUE;
		/* Work around for cpio, which does
		 * not output / with directories */

		if(line[linesize-2] != '/')
			filename = g_strconcat(line + n, "/", NULL); 
		else
			filename = g_strdup(line + n); 
	}
	else
	{
		archive->nr_of_files++;
		filename = g_strdup(line + n); 
	}
	
	entry = xa_set_archive_entries_for_each_row (archive,filename,FALSE,item);
	g_free (filename);
}

void xa_open_temp_file (gchar *tmp_dir,gchar *temp_path)
{
	gint current_page;
	gint idx;
	gchar *tmp = NULL;

	current_page = gtk_notebook_get_current_page(notebook);
	idx = xa_find_archive_index (current_page);

	gchar *command = NULL;
	tmp = g_strconcat (tmp_dir,"/file.cpio",NULL);

	stream = fopen (tmp,"w");
	if (stream == NULL)
	{
		response = xa_show_message_dialog (GTK_WINDOW (MainWindow),GTK_DIALOG_MODAL,GTK_MESSAGE_ERROR,GTK_BUTTONS_OK,_("Can't write to /tmp:"),g_strerror(errno) );
		g_free (tmp);
		return;
	}
	if (xa_detect_archive_type (temp_path) == XARCHIVETYPE_GZIP)
		command = g_strconcat ("gzip -dc ",temp_path,NULL);
	else
		command = g_strconcat ("bzip2 -dc ",temp_path,NULL);

	archive[idx]->parse_output = 0;
	xa_spawn_async_process (archive[idx],command,0);
	g_free (command);
	if (archive[idx]->child_pid == 0)
	{
		fclose (stream);
		xa_delete_temp_directory (archive[idx],1);
		g_free (tmp);
		return;
	}
	GIOChannel *ioc = g_io_channel_unix_new (archive[idx]->output_fd);
	g_io_channel_set_encoding (ioc,NULL,NULL);
	g_io_channel_set_flags (ioc,G_IO_FLAG_NONBLOCK,NULL);
	g_io_add_watch (ioc,G_IO_IN|G_IO_PRI|G_IO_ERR|G_IO_HUP|G_IO_NVAL,ExtractToDifferentLocation,stream);

	g_child_watch_add (archive[idx]->child_pid ,(GChildWatchFunc) xa_open_cpio,tmp);
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
			response = xa_show_message_dialog (GTK_WINDOW (MainWindow),GTK_DIALOG_MODAL,GTK_MESSAGE_ERROR,GTK_BUTTONS_OK, _("An error occurred:"),error->message);
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

