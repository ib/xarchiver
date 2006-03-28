/*
 *  Copyright (c) 2006 Stephan Arts      <stephan.arts@hva.nl>
 *                     Giuseppe Torelli <colossus73@gmail.com>
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

/* TODO:
 * Currently this implementation only checks for USTAR magic-header
 */

#include <stdio.h>
#include <string.h>
#include <glib.h>
#include "internals.h"
#include "libxarchiver.h"

/*
 * xarchive_tar_support_remove(XArchive *archive, GSList *files)
 * Remove files and folders from archive
 */
gboolean
xarchive_tar_support_remove (XArchive *archive, GSList *files)
{
	gchar *command, *dir;
	GString *names;
	gchar **argvp;
	
	GSList *_files = files;
	int argcp;
	if(files != NULL)
	{
		names = concatenatefilenames ( _files );
		command = g_strconcat ( "tar --delete -vf " , archive->path , names->str , NULL );
		archive->child_pid = xarchiver_async_process ( archive , command, 0);
		archive->status = DELETE;
		//TODO: to reload the archive to show the changes in the liststore
		g_free(command);
		g_string_free (names, TRUE);
	}
	fchdir(n_cwd);
}

/*
 * xarchive_tar_support_add(XArchive *archive, GSList *files)
 * Add files and folders to archive
 */
gboolean
xarchive_tar_support_add (XArchive *archive, GSList *files)
{
	gchar *command, *dir;
	GString *names;
	gchar **argvp;
	
	GSList *_files = files;
	int argcp;
	if(files != NULL)
	{
		dir = g_path_get_dirname(_files->data);
		chdir(dir);
		g_free(dir);

		names = concatenatefilenames ( _files );		
		// Check if the archive already exists or not
		if(g_file_test(archive->path, G_FILE_TEST_EXISTS))
			command = g_strconcat("tar rvvf ", archive->path, " ", names->str, NULL);
		else
			command = g_strconcat("tar cvvf ", archive->path, " ", names->str, NULL);

		archive->status = ADD;
		archive->child_pid = xarchiver_async_process ( archive , command, 0);
		g_free(command);
		if (archive->child_pid == 0)
		{
			g_message (archive->error->message);
			g_error_free (archive->error);
			return FALSE;
		}
		g_string_free(names, TRUE);
	}
	fchdir(n_cwd);
}

/*
 * xarchive_tar_support_extract(XArchive *archive, GSList *files)
 * Extract files and folders from archive
 */
gboolean
xarchive_tar_support_extract(XArchive *archive, gchar *destination_path, GSList *files, gboolean full_path)
{
	gchar *command, *dir, *filename;
	unsigned short int levels;
	char digit[2];
	gchar *strip = NULL;
    
	if(!g_file_test(archive->path, G_FILE_TEST_EXISTS))
		return FALSE;
    
	// Only extract certain files
	if( (files == NULL) && (g_slist_length(files) == 0))
	{
		chdir(dir);
		g_free(dir);
		filename = g_path_get_basename(files->data);
		command = g_strconcat("tar xvvf ", archive->path, " -C ", destination_path, " ", filename, NULL);
	} 
	else
	{
		GSList *_files = files;
		GString *names;
		names = concatenatefilenames ( _files );
		if ( full_path == 0 )
		{
			levels = countcharacters ( names->str , '/');
			sprintf ( digit , "%d" , levels );
			strip = g_strconcat ( "--strip-components=" , digit , " " , NULL );
		}
		command = g_strconcat("tar " , full_path ? "" : strip , "-xvf ", archive->path, " -C ", destination_path, names->str , NULL);
		g_string_free (names,TRUE);
	}
	archive->child_pid = xarchiver_async_process ( archive , command,0);
	g_free(command);
	if ( strip != NULL)
		g_free ( strip );
	if (archive->child_pid == 0)
	{
		g_message (archive->error->message);
		g_error_free (archive->error);
		return FALSE;
	}
	if ( ! xarchiver_set_channel ( archive->output_fd, G_IO_IN|G_IO_PRI|G_IO_ERR|G_IO_HUP|G_IO_NVAL, xarchiver_output_function, NULL ) ) return FALSE;
	if (! xarchiver_set_channel ( archive->error_fd, G_IO_IN|G_IO_PRI|G_IO_ERR|G_IO_HUP|G_IO_NVAL, xarchiver_error_function, NULL ) ) return FALSE;
	fchdir(n_cwd);
}

gboolean
xarchive_tar_support_verify(XArchive *archive)
{
	FILE *fp;
	unsigned char magic[6];

	if( (archive->path) && (archive->type == XARCHIVETYPE_UNKNOWN))
	{
		fp = fopen(archive->path, "r");
		if(fp == 0)
			return FALSE;
		fseek ( fp, 0 , SEEK_SET );
		if ( fseek ( fp , 257, SEEK_CUR ) == 0 ) 
		{
			if ( fread ( magic, 1, 5, fp ) )
			{
				if ( memcmp ( magic,"ustar",5 ) == 0 )
				{
					archive->type = XARCHIVETYPE_TAR;
					archive->has_passwd = 0;
					archive->passwd = 0;
				}
			}
		}
		fclose( fp );
	}

	if(archive->type == XARCHIVETYPE_TAR)
		return TRUE;
	else
		return FALSE;
}

XArchiveSupport *
xarchive_tar_support_new()
{
	XArchiveSupport *support = g_new0(XArchiveSupport, 1);
	support->type    = XARCHIVETYPE_TAR;
	support->add     = xarchive_tar_support_add;
	support->verify  = xarchive_tar_support_verify;
	support->extract = xarchive_tar_support_extract;
	support->remove  = xarchive_tar_support_remove;
	return support;
}
