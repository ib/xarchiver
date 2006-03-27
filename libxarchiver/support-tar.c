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
 * xarchive_tar_support_add(XArchive *archive, GSList *files)
 * Add files and folders to archive
 */
gboolean
xarchive_tar_support_add(XArchive *archive, GSList *files)
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

		g_shell_parse_argv(command, &argcp, &argvp, NULL);
		g_spawn_async_with_pipes (
				NULL, 
				argvp, 
				NULL, 
				G_SPAWN_SEARCH_PATH | G_SPAWN_DO_NOT_REAP_CHILD,
				NULL,
				NULL,
				NULL,
				NULL, // STDIN
				NULL, // STDOUT
				NULL, // STDERR
				NULL);
		
		g_free(argvp);
		g_free(command);
	}
    g_slist_free (files);
	g_string_free(names, TRUE);
	fchdir(n_cwd);
}

/*
 * xarchive_tar_support_extract(XArchive *archive, GSList *files)
 * Extract files and folders from archive
 */
gboolean
xarchive_tar_support_extract(XArchive *archive, gchar *destination_path, GSList *files)
{
	gchar *command, *dir, *filename;
	gchar **argvp;
	int argcp;

	if(!g_file_test(archive->path, G_FILE_TEST_EXISTS))
		return FALSE;

	// Only extract certain files
	if( (files != NULL) && (g_slist_length(files) != 0))
	{
		chdir(dir);
		g_free(dir);

		filename = g_path_get_basename(files->data);
		
		// Check if the archive already exists or not
		if ((destination_path !=  NULL )) 
			command = g_strconcat("tar xvvf ", archive->path, " -C ", destination_path, " ", filename, NULL);
		else
			command = g_strconcat("tar xvvf ", archive->path, " ", filename, NULL);
	} 
	else
	{
        GSList *_files = files;
        GString *names;

        names = concatenatefilenames ( _files );		
		if ( (destination_path != NULL) ) 
			command = g_strconcat("tar xvvf ", archive->path, " -C ", destination_path, names->str , NULL);
		else
			command = g_strconcat("tar xvvf ", archive->path, NULL);
	}
	g_shell_parse_argv(command, &argcp, &argvp, NULL);
	g_spawn_async_with_pipes (
			NULL, 
			argvp, 
			NULL, 
			G_SPAWN_SEARCH_PATH | G_SPAWN_DO_NOT_REAP_CHILD,
			NULL,
			NULL,
			NULL,
			NULL, // STDIN
			NULL, // STDOUT
			NULL, // STDERR
			NULL);
	
	g_free(command);
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
	return support;
}
