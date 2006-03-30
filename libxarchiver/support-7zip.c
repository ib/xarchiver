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
	return support;
}

