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
#include "support-arj.h"

/*
 * xarchive_arj_support_add(XArchive *archive, GSList *files)
 * Add files and folders to archive
 *
 */

gboolean
xarchive_arj_support_add (XArchive *archive, GSList *files)
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
			command = g_strconcat ( "arj a -i -r -g" , archive->passwd , " " , archive->path , names->str , NULL );
		else
			command = g_strconcat ( "arj a -i -r " , archive->path , names->str , NULL );
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
 * xarchive_arj_support_extract(XArchive *archive, GSList *files)
 * Extract files and folders from archive
 *
 */

gboolean
xarchive_arj_support_extract (XArchive *archive, gchar *destination_path, GSList *files , gboolean full_path)
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
			command = g_strconcat ( "arj x -g",archive->passwd," -i -y " , archive->path , " " , destination_path , NULL );
		else
			command = g_strconcat ( "arj x -i -y " , archive->path , " " , destination_path , NULL );
	} 
	else
	{
		names = concatenatefilenames ( _files );
		if ( archive->has_passwd)
			command = g_strconcat ( "arj x -g",archive->passwd," -i -y " , archive->path , " " , destination_path , names->str , NULL );
		else
			command = g_strconcat ( "arj ",full_path ? "x" : "e"," -i -y " , archive->path , " " , destination_path , names->str, NULL );
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
 * xarchive_arj_support_testing(XArchive *archive, GSList *files)
 * Test the integrity of the files in the archive
 *
 */

gboolean
xarchive_arj_support_testing (XArchive *archive)
{
	gchar *command;
	
	if(!g_file_test(archive->path, G_FILE_TEST_EXISTS))
		return FALSE;
        
	if (archive->has_passwd)
		command = g_strconcat ("arj t -g" , archive->passwd , " -i " , archive->path, NULL);
	else
		command = g_strconcat ("arj t -i " , archive->path, NULL);
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
 * xarchive_arj_support_remove(XArchive *archive, GSList *files)
 * Remove files and folders from the archive
 *
 */

gboolean
xarchive_arj_support_remove (XArchive *archive, GSList *files )
{
	gchar *command;
	GString *names;

	GSList *_files = files;
	names = concatenatefilenames ( _files );
	archive->status = REMOVE;
	command = g_strconcat ( "arj d " , archive->path , names->str, NULL);
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

gboolean
xarchive_arj_support_open (XArchive *archive)
{
	jump_header = FALSE;
    gchar *command = g_strconcat ( "arj v -y " , archive->path, NULL );
	archive->child_pid = xarchiver_async_process ( archive , command , 0 );
	g_free (command);
	if (archive->child_pid == 0)
	{
		g_message (archive->error->message);
		g_error_free (archive->error);
		return FALSE;
	}
	if ( ! xarchiver_set_channel ( archive->output_fd, G_IO_IN|G_IO_PRI|G_IO_ERR|G_IO_HUP|G_IO_NVAL, xarchiver_parse_arj_output, archive ) )
		return FALSE;
	if (! xarchiver_set_channel ( archive->error_fd, G_IO_IN|G_IO_PRI|G_IO_ERR|G_IO_HUP|G_IO_NVAL, xarchiver_error_function, archive ) )
		return FALSE;
	archive->dummy_size = 0;
	return TRUE;
}

/*
 * xarchiver_parse_arj_output
 * Parse the output from the arj command when opening the archive
 *
 */

gboolean xarchiver_parse_arj_output (GIOChannel *ioc, GIOCondition cond, gpointer data)
{
	gchar *line = NULL;
	XArchive *archive = data;

	if (cond & (G_IO_IN | G_IO_PRI) )
	{
		//This to avoid inserting in the liststore arj copyright message
		if (jump_header == FALSE )
		{
			g_io_channel_read_line ( ioc, &line, NULL, NULL, NULL );
			if (line == NULL) return TRUE;
			if (! archive->status == RELOAD) archive->output = g_slist_prepend ( archive->output , line );
			if  (strncmp (line , "------------" , 12) == 0)
			{
				jump_header = TRUE;
				arj_line = 1;
			}
			return TRUE;
		}
		if (arj_line == 4)
		{
			arj_line = 1;
			return TRUE;
		}
		if (arj_line == 1)
		{
			g_io_channel_read_line ( ioc, &line, NULL, NULL, NULL );
			if (line == NULL) return TRUE;
			if (strncmp (line, "------------", 12) == 0 || strncmp (line, "\x0a",1) == 0)
			{
				g_free (line);
				g_io_channel_read_line ( ioc, &line, NULL, NULL, NULL );
				if ( ! archive->status == RELOAD )
					archive->output = g_slist_prepend ( archive->output , line );
				return TRUE;
			}
			if ( ! archive->status == RELOAD )
				archive->output = g_slist_prepend ( archive->output , line );
			archive->row = get_last_field ( archive->row , line , 2 );
			arj_line++;
			return TRUE;
		}
		else if (arj_line == 2)
		{
			g_io_channel_read_line ( ioc, &line, NULL, NULL, NULL );
			if ( line == NULL) return TRUE;
			if ( ! archive->status == RELOAD )
				archive->output = g_slist_prepend ( archive->output , line );
			archive->row = split_line ( archive->row , line , 10 );
			//The following to remove the first and second fields of the second line of arj output
			archive->row = g_list_remove ( archive->row , (gconstpointer *)g_list_nth_data ( archive->row , 9) );
			archive->row = g_list_remove ( archive->row , (gconstpointer *)g_list_nth_data ( archive->row , 8) );
			if (  g_str_has_suffix (g_list_nth_data ( archive->row , 7) , "d") == FALSE)
				archive->number_of_files++;
			archive->dummy_size += atoll ( (gchar*)g_list_nth_data ( archive->row , 2) );
		}
		//Let's discard the third and forth line of arj output
		g_io_channel_read_line ( ioc, &line, NULL, NULL, NULL );
		g_free (line);
		g_io_channel_read_line ( ioc, &line, NULL, NULL, NULL );
		g_free (line);
		arj_line = 4;
		return TRUE;
	}
	else if (cond & (G_IO_ERR | G_IO_HUP | G_IO_NVAL) )
	{
		g_io_channel_shutdown ( ioc,TRUE,NULL );
		g_io_channel_unref (ioc);
		return FALSE;
	}
	return TRUE;
}

gboolean
xarchive_arj_support_verify(XArchive *archive)
{
	FILE *fp;
	unsigned char magic[2];
	unsigned short int basic_header_size;
	unsigned short int extended_header_size;
	unsigned int basic_header_CRC;
	unsigned int extended_header_CRC;
	unsigned int compressed_size;
	unsigned char arj_flag;

	if( (archive->path) && (archive->type == XARCHIVETYPE_UNKNOWN))
	{
		fp = fopen(archive->path, "r");
		if(fp == 0)
			return FALSE;
		fseek ( fp, 0 , SEEK_SET );
		if ( fread ( magic, 1, 2, fp ) )
		{
			if ( memcmp ( magic,"\x60\xea",2 ) == 0 )
			{
				archive->type = XARCHIVETYPE_ARJ;
				archive->has_passwd = FALSE;
				//Let's check for the password flag
				fread (&magic,1,2,fp);
				fseek (fp , magic[0]+magic[1] , SEEK_CUR);
				fread (&extended_header_size,1,2,fp);
				if (extended_header_size != 0)
					fread (&extended_header_CRC,1,4,fp);
				fread (&magic,1,2,fp);
				while ( memcmp (magic,"\x60\xea",2) == 0)
				{
					fread ( &basic_header_size , 1 , 2 , fp );
					if ( basic_header_size == 0 ) 
						break;
					fseek ( fp , 4 , SEEK_CUR);
					fread (&arj_flag,1,1,fp);
					if ((arj_flag & ( 1<<0) ) > 0)
					{
						archive->has_passwd = TRUE;
						return TRUE;
					}
					fseek ( fp , 7 , SEEK_CUR);
					fread (&compressed_size,1,4,fp);
					fseek ( fp , basic_header_size - 16 , SEEK_CUR);
					fread (&basic_header_CRC,1,4,fp);
					fread (&extended_header_size,1,2,fp);
					if (extended_header_size != 0) 
						fread (&extended_header_CRC,1,4,fp);
					fseek ( fp , compressed_size , SEEK_CUR);
					fread (&magic,1,2,fp);
				}
			}
		}
		fclose( fp );
	}

	if(archive->type == XARCHIVETYPE_ARJ)
		return TRUE;
	else
		return FALSE;
}

XArchiveSupport *
xarchive_arj_support_new()
{
	XArchiveSupport *support = g_new0(XArchiveSupport, 1);
	support->type    = XARCHIVETYPE_ARJ;
	support->add     = xarchive_arj_support_add;
	support->verify  = xarchive_arj_support_verify;
	support->extract = xarchive_arj_support_extract;
	support->testing = xarchive_arj_support_testing;
	support->remove  = xarchive_arj_support_remove;
	support->open    = xarchive_arj_support_open;
	support->n_columns = 9;
	support->column_names  = g_new0(gchar *, support->n_columns);
	support->column_types  = g_new0(GType, support->n_columns);
	support->column_names[0] = "Filename";
	support->column_names[1] = "Original";
	support->column_names[2] = "Compressed";
	support->column_names[3] = "Ratio";
	support->column_names[4] = "Date";
	support->column_names[5] = "Time";
	support->column_names[6] = "Attr";
	support->column_names[7] = "GUA";
	support->column_names[8] = "BPMGS";
	support->column_types[0] = G_TYPE_STRING;
	support->column_types[1] = G_TYPE_STRING;
	support->column_types[2] = G_TYPE_STRING;
	support->column_types[3] = G_TYPE_STRING;
	support->column_types[4] = G_TYPE_STRING;
	support->column_types[5] = G_TYPE_STRING;
	support->column_types[6] = G_TYPE_STRING;
	support->column_types[7] = G_TYPE_STRING;
	support->column_types[8] = G_TYPE_STRING;
	return support;
}

