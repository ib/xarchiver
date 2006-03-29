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
#include <glib.h>
#include "internals.h"
#include "libxarchiver.h"

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
		if ( ! xarchiver_set_channel ( archive->output_fd, G_IO_IN|G_IO_PRI|G_IO_ERR|G_IO_HUP|G_IO_NVAL, xarchiver_output_function, NULL ) )
			return FALSE;
		if (! xarchiver_set_channel ( archive->error_fd, G_IO_IN|G_IO_PRI|G_IO_ERR|G_IO_HUP|G_IO_NVAL, xarchiver_error_function, NULL ) )
			return FALSE;
		g_free(command);
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
	if ( ! xarchiver_set_channel ( archive->output_fd, G_IO_IN|G_IO_PRI|G_IO_ERR|G_IO_HUP|G_IO_NVAL, xarchiver_output_function, NULL ) )
		return FALSE;
	if (! xarchiver_set_channel ( archive->error_fd, G_IO_IN|G_IO_PRI|G_IO_ERR|G_IO_HUP|G_IO_NVAL, xarchiver_error_function, NULL ) )
		return FALSE;
	if (archive->child_pid == 0)
	{
		g_message (archive->error->message);
		g_error_free (archive->error);
	}
	g_free(command);
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
	if ( ! xarchiver_set_channel ( archive->output_fd, G_IO_IN|G_IO_PRI|G_IO_ERR|G_IO_HUP|G_IO_NVAL, xarchiver_output_function, NULL ) )
		return FALSE;
	if (! xarchiver_set_channel ( archive->error_fd, G_IO_IN|G_IO_PRI|G_IO_ERR|G_IO_HUP|G_IO_NVAL, xarchiver_error_function, NULL ) )
		return FALSE;
	if (archive->child_pid == 0)
	{
		g_message (archive->error->message);
		g_error_free (archive->error);
	}
	g_free (command);
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
	if ( ! xarchiver_set_channel ( archive->output_fd, G_IO_IN|G_IO_PRI|G_IO_ERR|G_IO_HUP|G_IO_NVAL, xarchiver_output_function, NULL ) )
		return FALSE;
	if (! xarchiver_set_channel ( archive->error_fd, G_IO_IN|G_IO_PRI|G_IO_ERR|G_IO_HUP|G_IO_NVAL, xarchiver_error_function, NULL ) )
		return FALSE;
	if (archive->child_pid == 0)
	{
		g_message (archive->error->message);
		g_error_free (archive->error);
	}
	g_free (command);
	return TRUE;
}

// FIXME: dont know if password-check is correct
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
				//Let's check for the password flag
				fread (&magic,1,2,fp);
				fseek (fp , magic[0]+magic[1] , SEEK_CUR);
				fseek (fp , 2 , SEEK_CUR);
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
						archive->has_passwd = TRUE;
					else
						archive->has_passwd = FALSE;
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
	return support;
}

