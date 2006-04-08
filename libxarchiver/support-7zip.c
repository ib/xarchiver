/*
 *  Copyright (c) 2006 Stephan Arts      <stephan.arts@hva.nl>
 *                     Giuseppe Torelli  <colossus73@gmail.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <glib.h>
#include <glib-object.h>
#include "internals.h"
#include "libxarchiver.h"
#include "support-7zip.h"

/*
 * xarchive_7zip_support_add(XArchive *archive, GSList *files)
 * Add files and folders to archive
 *
 */

gboolean
xarchive_7zip_support_add (XArchive *archive, GSList *files)
{
	gchar *command, *dir;
	GString *names;

	GSList *_files = files;
	if(files != NULL)
	{
		dir = g_path_get_dirname(files->data);
		chdir(dir);
		g_free(dir);
 		names = concatenatefilenames ( _files );
		if (archive->has_passwd)
			command = g_strconcat ( "7za a -ms=off -p" , archive->passwd , " " , archive->path , names->str , NULL );
        else
			command = g_strconcat ( "7za a -ms=off " , archive->path , names->str , NULL );
		archive->status = ADD;
		archive->child_pid = xarchiver_async_process ( archive , command, 0);
		g_free(command);
		if ( ! xarchiver_set_channel ( archive->output_fd, G_IO_IN|G_IO_PRI|G_IO_ERR|G_IO_HUP|G_IO_NVAL, xarchiver_output_function, NULL ) )
			return FALSE;
		if (! xarchiver_set_channel ( archive->error_fd, G_IO_IN|G_IO_PRI|G_IO_ERR|G_IO_HUP|G_IO_NVAL, xarchiver_error_function, NULL ) )
			return FALSE;
		if (archive->child_pid == 0)
		{
			g_message (archive->error->message);
			g_error_free (archive->error);
			return FALSE;
		}
		g_string_free (names, TRUE);
	}
	fchdir(n_cwd);
	return TRUE;
}

/*
 * xarchive_7zip_support_extract(XArchive *archive, GSList *files)
 * Extract files and folders from archive
 *
 */

gboolean
xarchive_7zip_support_extract (XArchive *archive, gchar *destination_path, GSList *files , gboolean full_path)
{
	gchar *command;
	GString *names;

	GSList *_files = files;
	if(!g_file_test(archive->path, G_FILE_TEST_EXISTS))
		return FALSE;

    //This extracts the whole archive
	if( (files == NULL) && (g_slist_length(files) == 0))
	{
		if (archive->has_passwd)
			command = g_strconcat ( "7za x -aoa -bd -p",archive->passwd," ", archive->path , " -o" , destination_path , NULL );
        else
			command = g_strconcat ( "7za x -aoa -bd " , archive->path , " -o" , destination_path , NULL );
	} 
	else
	{
		names = concatenatefilenames ( _files );
		if ( archive->has_passwd)
			command = g_strconcat ("7za " , full_path ? "x" : "e" , " -p",archive->passwd," -aoa -bd " , archive->path , names->str , " -o" , destination_path , NULL );
        else
			command = g_strconcat ( "7za " , full_path ? "x" : "e" ," -aoa -bd " , archive->path , names->str , " -o" , destination_path , NULL );
		g_string_free (names, TRUE);
	}
	archive->child_pid = xarchiver_async_process ( archive , command , 0);
	g_free(command);
	if ( ! xarchiver_set_channel ( archive->output_fd, G_IO_IN|G_IO_PRI|G_IO_ERR|G_IO_HUP|G_IO_NVAL, xarchiver_output_function, NULL ) )
		return FALSE;
	if (! xarchiver_set_channel ( archive->error_fd, G_IO_IN|G_IO_PRI|G_IO_ERR|G_IO_HUP|G_IO_NVAL, xarchiver_error_function, NULL ) )
		return FALSE;
	if (archive->child_pid == 0)
	{
		g_message (archive->error->message);
		g_error_free (archive->error);
	}
	fchdir(n_cwd);
	return TRUE;
}

/*
 * xarchive_7zip_support_testing(XArchive *archive, GSList *files)
 * Test the integrity of the files in the archive
 *
 */

gboolean
xarchive_7zip_support_testing (XArchive *archive)
{
	gchar *command;
	
	if(!g_file_test(archive->path, G_FILE_TEST_EXISTS))
		return FALSE;
        
	if (archive->has_passwd)
		command = g_strconcat ( "7za t -p" , archive->passwd , " " , archive->path, NULL);
	else
		command = g_strconcat ("7za t " , archive->path, NULL);
	archive->child_pid = xarchiver_async_process ( archive , command , 0);
	g_free (command);
	if ( ! xarchiver_set_channel ( archive->output_fd, G_IO_IN|G_IO_PRI|G_IO_ERR|G_IO_HUP|G_IO_NVAL, xarchiver_output_function, NULL ) )
		return FALSE;
	if (! xarchiver_set_channel ( archive->error_fd, G_IO_IN|G_IO_PRI|G_IO_ERR|G_IO_HUP|G_IO_NVAL, xarchiver_error_function, NULL ) )
		return FALSE;
	if (archive->child_pid == 0)
	{
		g_message (archive->error->message);
		g_error_free (archive->error);
	}
	return TRUE;
}

/*
 * xarchive_7zip_support_remove(XArchive *archive, GSList *files)
 * Remove files and folders from the archive
 *
 */

gboolean
xarchive_7zip_support_remove (XArchive *archive, GSList *files )
{
	gchar *command;
	GString *names;

	GSList *_files = files;
	names = concatenatefilenames ( _files );
	archive->status = REMOVE;
	command = g_strconcat ( "7za d " , archive->path , names->str , NULL );
	g_string_free (names, TRUE);
	archive->child_pid = xarchiver_async_process ( archive , command , 0);
	g_free (command);
	if ( ! xarchiver_set_channel ( archive->output_fd, G_IO_IN|G_IO_PRI|G_IO_ERR|G_IO_HUP|G_IO_NVAL, xarchiver_output_function, NULL ) )
		return FALSE;
	if (! xarchiver_set_channel ( archive->error_fd, G_IO_IN|G_IO_PRI|G_IO_ERR|G_IO_HUP|G_IO_NVAL, xarchiver_error_function, NULL ) )
		return FALSE;
	if (archive->child_pid == 0)
	{
		g_message (archive->error->message);
		g_error_free (archive->error);
	}
	return TRUE;
}

/*
 * xarchive_7zip_support_open(XArchive *archive)
 * Open the archive and calls other functions to catch the output
 *
 */

gboolean
xarchive_7zip_support_open (XArchive *archive)
{
	gchar *command;
	jump_header = FALSE;

	command = g_strconcat ( "7za l " , archive->path, NULL );
	archive->child_pid = xarchiver_async_process ( archive , command , 0 );
	g_free (command);
	if (archive->child_pid == 0)
	{
		g_message (archive->error->message);
		g_error_free (archive->error);
		return FALSE;
	}
	if ( ! xarchiver_set_channel ( archive->output_fd, G_IO_IN|G_IO_PRI|G_IO_ERR|G_IO_HUP|G_IO_NVAL, xarchiver_parse_7zip_output, archive ) )
		return FALSE;
	if (! xarchiver_set_channel ( archive->error_fd, G_IO_IN|G_IO_PRI|G_IO_ERR|G_IO_HUP|G_IO_NVAL, xarchiver_error_function, archive ) )
		return FALSE;
	archive->dummy_size = 0;
	return TRUE;
}

/*
 * xarchive_parse_7zip_output
 * Parse the output from the 7za command when opening the archive
 *
 */

gboolean xarchiver_parse_7zip_output (GIOChannel *ioc, GIOCondition cond, gpointer data)
{
    gchar *line = NULL;
	unsigned short int x;

	XArchive *archive = data;
	if (cond & (G_IO_IN | G_IO_PRI) )
	{
		//This to avoid inserting in the liststore 7zip's message
		if (jump_header == FALSE )
		{
			for ( x = 0; x <= 7; x++)
			{
				g_io_channel_read_line ( ioc, &line, NULL, NULL, NULL );
				if (line == NULL) return TRUE;
				if ( ! archive->status == RELOAD )
					archive->output = g_slist_prepend (archive->output , line );
			}
			jump_header = TRUE;
		}
		g_io_channel_read_line ( ioc, &line, NULL, NULL, NULL );
		if ( line == NULL ) return TRUE;
		//This to avoid inserting the last line of output
		if (strncmp (line, "-------------------", 19) == 0 || strncmp (line, "\x0a",1) == 0)
		{
			g_free (line);
			g_io_channel_read_line ( ioc, &line, NULL, NULL, NULL );
			if ( ! archive->status == RELOAD ) archive->output = g_slist_prepend (archive->output , line );
			return TRUE;
		}
		if ( ! archive->status == RELOAD ) archive->output = g_slist_prepend (archive->output , line );
		archive->row = split_line (archive->row , line , 5);
		archive->row = get_last_field ( archive->row , line , 6);
		//g_print ("%s\n",(gchar*)g_list_nth_data ( archive->row , 3) );
		if ( g_str_has_prefix(g_list_nth_data ( archive->row , 3) , "D") == FALSE)
			archive->number_of_files++;
		else
			archive->number_of_dirs++;
		archive->dummy_size += atoll ( (gchar*)g_list_nth_data ( archive->row , 1) );
		return TRUE;
	}
	else if (cond & (G_IO_ERR | G_IO_HUP | G_IO_NVAL) )
	{
		g_io_channel_shutdown ( ioc,TRUE,NULL );
		g_io_channel_unref (ioc);
		archive->row = g_list_reverse (archive->row);
		return FALSE;
	}
	return TRUE;
}

gboolean
xarchive_7zip_support_verify(XArchive *archive)
{
	FILE *fp;
	unsigned char magic[6];

	if( (archive->path) && (archive->type == XARCHIVETYPE_UNKNOWN))
	{
		fp = fopen(archive->path, "r");
		if(fp == 0)
			return FALSE;
		fseek ( fp, 0 , SEEK_SET );
		if ( fread ( magic, 1, 6, fp ) )
		{
			if ( memcmp ( magic,"\x37\x7a\xbc\xaf\x27\x1c",6 ) == 0 )
			{
				archive->type = XARCHIVETYPE_7ZIP;
				//TODO: no password detection for 7zip
				//http://sourceforge.net/forum/forum.php?thread_id=1378003&forum_id=383044
			}
		}
		fclose( fp );
	}

	if(archive->type == XARCHIVETYPE_7ZIP)
		return TRUE;
	else
		return FALSE;
}

XArchiveSupport *
xarchive_7zip_support_new()
{
	XArchiveSupport *support = g_new0(XArchiveSupport, 1);
	support->type    = XARCHIVETYPE_7ZIP;
	support->add     = xarchive_7zip_support_add;
	support->verify  = xarchive_7zip_support_verify;
	support->extract = xarchive_7zip_support_extract;
	support->testing = xarchive_7zip_support_testing;
	support->remove  = xarchive_7zip_support_remove;
	support->open    = xarchive_7zip_support_open;

	support->n_columns = 6;
	support->column_names  = g_new0(gchar *, support->n_columns);
	support->column_types  = g_new0(GType, support->n_columns);
	support->column_names[0] = "Filename";
	support->column_names[1] = "Compressed";
	support->column_names[2] = "Original";
	support->column_names[3] = "Attr";
	support->column_names[4] = "Time";
	support->column_names[5] = "Date";
	support->column_types[0] = G_TYPE_STRING;
	support->column_types[1] = G_TYPE_STRING;
	support->column_types[2] = G_TYPE_STRING;
	support->column_types[3] = G_TYPE_STRING;
	support->column_types[4] = G_TYPE_STRING;
	support->column_types[5] = G_TYPE_STRING;
	return support;
}

