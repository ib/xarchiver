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
#include "support-zip.h"
#include "archive-zip.h"

/*
 * xarchive_zip_support_add(XArchive *archive, GSList *files)
 * Add files and folders to archive
 *
 */

gboolean
xarchive_zip_support_add (XArchive *archive, GSList *files)
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
			command = g_strconcat ( "zip -P " , archive->passwd , " -r " , archive->path , names->str , NULL );
		else
			command = g_strconcat ( "zip -r " , archive->path , names->str , NULL );
		archive->status = ADD;
		archive->child_pid = xarchiver_async_process ( archive , command, 0);
		g_free (command);
		if (archive->child_pid == 0)
		{
			g_message (archive->error->message);
			g_error_free (archive->error);
			return FALSE;
		}
		if ( ! xarchiver_set_channel ( archive->output_fd, G_IO_IN|G_IO_PRI|G_IO_ERR|G_IO_HUP|G_IO_NVAL, xarchiver_output_function, NULL ) )
			return FALSE;
		if (! xarchiver_set_channel ( archive->error_fd, G_IO_IN|G_IO_PRI|G_IO_ERR|G_IO_HUP|G_IO_NVAL, xarchiver_error_function, NULL ) )
			return FALSE;
		g_string_free (names, TRUE);
	}
	fchdir(n_cwd);
	return TRUE;
}

/*
 * xarchive_zip_support_extract(XArchive *archive, GSList *files)
 * Extract files and folders from archive
 *
 */

gboolean
xarchive_zip_support_extract (XArchive *archive, gchar *destination_path, GSList *files , gboolean full_path)
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
			command = g_strconcat ( "unzip -o -P " , archive->passwd , " " , archive->path , " -d " , destination_path , NULL );
		else
			command = g_strconcat ( "unzip -o " , archive->path , " -d " , destination_path , NULL );
	} 
	else
	{
		names = concatenatefilenames ( _files );
		if ( archive->has_passwd)
			command = g_strconcat ( "unzip -o -P " , archive->passwd , full_path ? " " : " -j " , archive->path , names->str , " -d " , destination_path , NULL );
		else
			command = g_strconcat ( "unzip -o " , full_path ? "" : "-j " , archive->path , names->str , " -d " , destination_path , NULL );
		g_string_free (names, TRUE);
	}
	archive->child_pid = xarchiver_async_process ( archive , command , 0);
	g_free(command);
	if (archive->child_pid == 0)
	{
		g_message (archive->error->message);
		g_error_free (archive->error);
	}
	if ( ! xarchiver_set_channel ( archive->output_fd, G_IO_IN|G_IO_PRI|G_IO_ERR|G_IO_HUP|G_IO_NVAL, xarchiver_output_function, NULL ) )
		return FALSE;
	if (! xarchiver_set_channel ( archive->error_fd, G_IO_IN|G_IO_PRI|G_IO_ERR|G_IO_HUP|G_IO_NVAL, xarchiver_error_function, NULL ) )
		return FALSE;
	fchdir(n_cwd);
	return TRUE;
}

/*
 * xarchive_zip_support_testing(XArchive *archive, GSList *files)
 * Test the integrity of the files in the archive
 *
 */

gboolean
xarchive_zip_support_testing (XArchive *archive)
{
	gchar *command;
	
	if(!g_file_test(archive->path, G_FILE_TEST_EXISTS))
		return FALSE;
        
	if (archive->has_passwd)
		command = g_strconcat ("unzip -P ", archive->passwd, " -t " , archive->path, NULL);
	else
		command = g_strconcat ("unzip -t " , archive->path, NULL);
	archive->child_pid = xarchiver_async_process ( archive , command , 0);
	g_free (command);
	if (archive->child_pid == 0)
	{
		g_message (archive->error->message);
		g_error_free (archive->error);
	}
	if ( ! xarchiver_set_channel ( archive->output_fd, G_IO_IN|G_IO_PRI|G_IO_ERR|G_IO_HUP|G_IO_NVAL, xarchiver_output_function, NULL ) )
		return FALSE;
	if (! xarchiver_set_channel ( archive->error_fd, G_IO_IN|G_IO_PRI|G_IO_ERR|G_IO_HUP|G_IO_NVAL, xarchiver_error_function, NULL ) )
		return FALSE;
	return TRUE;
}

/*
 * xarchive_zip_support_remove(XArchive *archive, GSList *files)
 * Remove files and folders from the archive
 *
 */

gboolean
xarchive_zip_support_remove (XArchive *archive, GSList *files )
{
	gchar *command;
	GString *names;

	GSList *_files = files;
	names = concatenatefilenames ( _files );
	archive->status = REMOVE;
	command = g_strconcat ( "zip -d " , archive->path , names->str , NULL );
	g_string_free (names, TRUE);
	archive->child_pid = xarchiver_async_process ( archive , command , 0);
	g_free (command);
	if (archive->child_pid == 0)
	{
		g_message (archive->error->message);
		g_error_free (archive->error);
	}
	if ( ! xarchiver_set_channel ( archive->output_fd, G_IO_IN|G_IO_PRI|G_IO_ERR|G_IO_HUP|G_IO_NVAL, xarchiver_output_function, NULL ) )
		return FALSE;
	if (! xarchiver_set_channel ( archive->error_fd, G_IO_IN|G_IO_PRI|G_IO_ERR|G_IO_HUP|G_IO_NVAL, xarchiver_error_function, NULL ) )
		return FALSE;
	return TRUE;
}

/*
 * xarchive_zip_support_open(XArchive *archive)
 * Open the archive and calls other functions to catch the output in the archive->output g_slist
 *
 */

gboolean
xarchive_zip_support_open (XArchive *archive)
{
	gchar *command = g_strconcat ("unzip -vl -qq " , archive->path, NULL );
	archive->child_pid = xarchiver_async_process ( archive , command , 0 );
	g_free (command);
	if (archive->child_pid == 0)
	{
		g_message (archive->error->message);
		g_error_free (archive->error);
		return FALSE;
	}
	if ( ! xarchiver_set_channel ( archive->output_fd, G_IO_IN|G_IO_PRI|G_IO_ERR|G_IO_HUP|G_IO_NVAL, xarchiver_parse_zip_output, archive ) )
		return FALSE;
	if (! xarchiver_set_channel ( archive->error_fd, G_IO_IN|G_IO_PRI|G_IO_ERR|G_IO_HUP|G_IO_NVAL, xarchiver_error_function, archive ) )
		return FALSE;
	archive->dummy_size = 0;
	return TRUE;
}

/*
 * xarchiver_parse_zip_output
 * Parse the output from the zip command when opening the archive
 *
 */

gboolean xarchiver_parse_zip_output (GIOChannel *ioc, GIOCondition cond, gpointer data)
{
	gchar *line = NULL;
	XArchive *archive = data;

	if (cond & (G_IO_IN | G_IO_PRI) )
	{
		g_io_channel_read_line ( ioc, &line, NULL, NULL, NULL );
		if ( line == NULL ) return TRUE;
		if ( ! archive->status == RELOAD ) archive->output = g_slist_prepend (archive->output , line);
		archive->row = get_last_field ( archive->row , line , 8);
		archive->row = split_line ( archive->row , line , 7 );
		if ( g_str_has_suffix (g_list_nth_data ( archive->row , 7) , "/") == TRUE)
			archive->number_of_dirs++;
		else
			archive->number_of_files++;
		archive->dummy_size += atoll ( (gchar*)g_list_nth_data ( archive->row , 4) );
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

XArchiveSupport *
xarchive_zip_support_new()
{
	XArchiveSupport *support = g_new0(XArchiveSupport, 1);
	support->type    = XARCHIVETYPE_ZIP;
	support->add     = xarchive_zip_support_add;
	support->verify  = xarchive_type_zip_verify;
	support->extract = xarchive_zip_support_extract;
	support->testing = xarchive_zip_support_testing;
	support->remove  = xarchive_zip_support_remove;
	support->open    = xarchive_zip_support_open;
	support->n_columns = 8;
	support->column_names  = g_new0(gchar *, support->n_columns);
	support->column_types  = g_new0(GType, support->n_columns);
	support->column_names[0] = "Filename";
	support->column_names[1] = "Original";
	support->column_names[2] = "Method";
	support->column_names[3] = "Compressed";
	support->column_names[4] = "Ratio";
	support->column_names[5] = "Date";
	support->column_names[6] = "Time";
	support->column_names[7] = "CRC-32";
	support->column_types[0] = G_TYPE_STRING;
	support->column_types[1] = G_TYPE_STRING;
	support->column_types[2] = G_TYPE_STRING;
	support->column_types[3] = G_TYPE_STRING;
	support->column_types[4] = G_TYPE_STRING;
	support->column_types[5] = G_TYPE_STRING;
	support->column_types[6] = G_TYPE_STRING;
	support->column_types[7] = G_TYPE_STRING;
	return support;
}

