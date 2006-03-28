/*
 *  Copyright (c) 2006 Stephan Arts <stephan.arts@hva.nl>
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
 *
 */

#include <stdio.h>
#include <glib.h>
#include "internals.h"
#include "libxarchiver.h"

#include "support-bzip2.h"

/*
 * xarchive_bzip2_support_add(XArchive *archive, GSList *files)
 *
 * can only compress one file, 
 * (will return compressed file in XArchive->path)
 */
gboolean
xarchive_bzip2_support_add(XArchive *archive, GSList *files)
{
	gchar *command, *dir, *filename;
	gchar **argvp;
	int argcp;
	if(files != NULL)
	{
		if(g_slist_length(files) > 1)
			g_warning("bzip2 can compress one file only");
	
		dir = g_path_get_dirname(files->data);
		chdir(dir);
		g_free(dir);

		filename = g_path_get_basename(files->data);
		
		command = g_strconcat("bzip2 -kz ", filename, NULL);
		archive->child_pid = xarchiver_async_process ( archive , command, 0);
		g_free(command);
		if (archive->child_pid == 0)
		{
			g_message (archive->error->message);
			g_error_free (archive->error);
			return FALSE;
		}
		if(archive->path)
			g_free(archive->path);
		archive->path = g_strconcat(filename,".bz2");
	}

	fchdir(n_cwd);
}


/*
 * xarchive_bzip2_support_extract(XArchive *archive, GSList *files)
 * Extract archive
 *
 * FIXME:
 * destination-folder does not work with bare bzip
 */
gboolean
xarchive_bzip2_support_extract(XArchive *archive, gchar *destination_path, GSList *files, gboolean full_path)
{
	gchar *command, *dir, *filename;
	gchar **argvp;
	int argcp;

	if(!g_file_test(archive->path, G_FILE_TEST_EXISTS))
		return FALSE;

	if( (files != NULL) && (g_slist_length(files) != 0))
	{
		g_warning("bzip2 can only extract one file");
	}
	command = g_strconcat("bzip2 -kd ", archive->path, NULL);
	archive->child_pid = xarchiver_async_process ( archive, command, 0);
	if (archive->child_pid == 0)
	{
		g_message (archive->error->message);
		g_error_free (archive->error);
		return FALSE;
	}
	g_free(command);
	fchdir(n_cwd);
}


gboolean
xarchive_bzip2_support_verify(XArchive *archive)
{
	FILE *fp;
	unsigned char magic[3];
	if((archive->path) && (archive->type == XARCHIVETYPE_UNKNOWN))
	{
		fp = fopen(archive->path, "r");
		if(fp == 0)
			return FALSE;
		if(fread( magic, 1, 3, fp) == 0)
		{
			fclose(fp);
			return FALSE;
		}
  	if ( memcmp ( magic,"\x42\x5a\x68",3 ) == 0 )
		{
			archive->type = XARCHIVETYPE_BZIP2;
			archive->has_passwd = FALSE;
			archive->passwd = 0;
		} 
		fclose(fp);
	}
	if(archive->type == XARCHIVETYPE_BZIP2)
		return TRUE;
	else
		return FALSE;
}

XArchiveSupport *
xarchive_bzip2_support_new()
{
	XArchiveSupport *support = g_new0(XArchiveSupport, 1);
	support->type = XARCHIVETYPE_BZIP2;
	support->verify = xarchive_bzip2_support_verify;
	support->add = xarchive_bzip2_support_add;
	support->extract = xarchive_bzip2_support_extract;
	support->remove = NULL;
	support->testing = NULL;
	return support;
}
