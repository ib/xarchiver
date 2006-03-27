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

#include <stdio.h>
#include <glib.h>
#include "internals.h"
#include "libxarchiver.h"

/*
 * xarchive_rar_support_add(XArchive *archive, GSList *files)
 * Add files and folders to archive
 *
 */

gboolean
xarchive_rar_support_add(XArchive *archive, GSList *files)
{
	gchar *command, *dir, *filename;
	gchar **argvp;
	int argcp;
	if(files != NULL)
	{
		dir = g_path_get_dirname(files->data);
		chdir(dir);
		g_free(dir);


		filename = g_path_get_basename(files->data);
	
		// Check if the archive already exists or not
		if(g_file_test(archive->path, G_FILE_TEST_EXISTS))
			command = g_strconcat("tar rvvf ", archive->path, " ", filename, NULL);
		else
			command = g_strconcat("tar cvvf ", archive->path, " ", filename, NULL);

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
	}

	fchdir(n_cwd);
}

/*
 * xarchive_rar_support_extract(XArchive *archive, GSList *files)
 * Extract files and folders from archive
 *
 */

gboolean
xarchive_rar_support_extract(XArchive *archive, gchar *destination_path, GSList *files)
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
		if((destination_path !=  NULL )) 
			command = g_strconcat("rar xvvf ", archive->path, " -C ", destination_path, " ", filename, NULL);
		else
			command = g_strconcat("rar xvvf ", archive->path, " ", filename, NULL);
	} 
	else
	{
		if((destination_path != NULL)) 
			command = g_strconcat("tar xvvf ", archive->path, " -C ", destination_path, NULL);
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
xarchive_rar_support_verify(XArchive *archive)
{
	FILE *fp;
	unsigned char magic[4];

	if( (archive->path) && (archive->type == XARCHIVETYPE_UNKNOWN))
	{
		fp = fopen(archive->path, "r");
		if(fp == 0)
			return FALSE;
		fseek ( fp, 0 , SEEK_SET );
		if ( fread ( magic, 1, 4, fp ) )
		{
			if ( memcmp ( magic,"\x52\x61\x72\x21",4 ) == 0 )
				archive->type = XARCHIVETYPE_RAR;
		}
		fclose( fp );
	}

	if(archive->type == XARCHIVETYPE_RAR)
		return TRUE;
	else
		return FALSE;
}

gboolean
xarchive_rar_support_testing (XArchive *archive, gboolean has_passwd)
{
	gchar *command;
	gchar **argvp;
	int argcp;

	if(!g_file_test(archive->path, G_FILE_TEST_EXISTS))
		return FALSE;
        
	if (has_passwd)
	{
		if (archive->passwd == NULL)
			return FALSE;
		command = g_strconcat ("rar t -idp -p" , archive->passwd ," " , archive->path, NULL);
	}
	else
		command = g_strconcat ("rar t -idp " , archive->path, NULL);
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
	g_free (command);
}

XArchiveSupport *
xarchive_rar_support_new()
{
	XArchiveSupport *support = g_new0(XArchiveSupport, 1);
	support->type    = XARCHIVETYPE_RAR;
	support->add     = xarchive_rar_support_add;
	support->verify  = xarchive_rar_support_verify;
	support->extract = xarchive_rar_support_extract;
	support->testing = xarchive_rar_support_testing;
	return support;
}
