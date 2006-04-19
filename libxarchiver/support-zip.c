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

#include <stdlib.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <glib.h>
#include <glib-object.h>
#include "internals.h"
#include "archive.h"
#include "archive-types.h"
#include "support.h"
#include "support-zip.h"


void
xa_support_zip_init(XASupportZip *support);

gint
xa_support_zip_open(XASupport *support, XAArchive *archive);

gint
xa_support_zip_add (XASupport *support, XAArchive *archive, GSList *files);

gint
xa_support_zip_remove (XASupport *support, XAArchive *archive, GSList *files);

gint
xa_support_zip_extract(XASupport *support, XAArchive *archive, gchar *destination_path, GSList *files, gboolean full_path);

gboolean 
xa_support_zip_parse_output (GIOChannel *ioc, GIOCondition cond, gpointer data);

gint
xa_support_zip_testing(XASupport *support, XAArchive *archive);

GType
xa_support_zip_get_type ()
{
	static GType xa_support_zip_type = 0;

 	if (!xa_support_zip_type)
	{
 		static const GTypeInfo xa_support_zip_info = 
		{
			sizeof (XASupportZipClass),
			(GBaseInitFunc) NULL,
			(GBaseFinalizeFunc) NULL,
			(GClassInitFunc) NULL,
			(GClassFinalizeFunc) NULL,
			NULL,
			sizeof (XASupportZip),
			0,
			(GInstanceInitFunc) xa_support_zip_init,
			NULL
		};

		xa_support_zip_type = g_type_register_static (XA_TYPE_SUPPORT, "XASupportZip", &xa_support_zip_info, 0);
	}
	return xa_support_zip_type;
}

void
xa_support_zip_init(XASupportZip *support)
{
	XASupport *xa_support = XA_SUPPORT(support);
	gint n_columns = 8;
	gchar **column_names  = g_new0(gchar *, n_columns);
	GType *column_types  = g_new0(GType, n_columns);

	column_names[0] = "Filename";
	column_names[1] = "Original";
	column_names[2] = "Method";
	column_names[3] = "Compressed";
	column_names[4] = "Ratio";
	column_names[5] = "Date";
	column_names[6] = "Time";
	column_names[7] = "CRC-32";
	column_types[0] = G_TYPE_STRING;
	column_types[1] = G_TYPE_STRING; /* UINT */
	column_types[2] = G_TYPE_STRING;
	column_types[3] = G_TYPE_STRING; /* UINT */
	column_types[4] = G_TYPE_STRING;
	column_types[5] = G_TYPE_STRING; /* DATE */
	column_types[6] = G_TYPE_STRING;
	column_types[7] = G_TYPE_STRING;

	xa_support_set_columns(xa_support, n_columns, column_names, column_types);
	xa_support->type    = XARCHIVETYPE_ZIP;
	xa_support->verify  = xa_archive_type_zip_verify;
	xa_support->open    = xa_support_zip_open;
	xa_support->add     = xa_support_zip_add;
	xa_support->remove  = xa_support_zip_remove;
	xa_support->extract = xa_support_zip_extract;
	xa_support->testing = xa_support_zip_testing;
	xa_support->parse_output = xa_support_zip_parse_output;
	
	g_free (column_names);
	g_free(column_types);
}

gboolean
xa_support_zip_add (XASupport *support, XAArchive *archive, GSList *files)
{
	gchar *dir;
	GString *names;

	GSList *_files = files;
	if(files != NULL)
	{
		dir = g_path_get_dirname(files->data);
		chdir(dir);
		g_free(dir);
 		names = concatenatefilenames ( _files , TRUE );
		if (archive->has_passwd)
			support->exec.command = g_strconcat ( "zip -P " , archive->passwd , " -r " , archive->path , names->str , NULL );
		else
			support->exec.command = g_strconcat ( "zip -r " , archive->path , names->str , NULL );
		archive->status = ADD;
		//g_free (command);
		/*if (archive->child_pid == 0)
		{
			g_message (archive->error->message);
			g_error_free (archive->error);
			return FALSE;
		}
		if ( ! xarchiver_set_channel ( archive->output_fd, G_IO_IN|G_IO_PRI|G_IO_ERR|G_IO_HUP|G_IO_NVAL, xarchiver_output_function, NULL ) )
			return FALSE;
		if (! xarchiver_set_channel ( archive->error_fd, G_IO_IN|G_IO_PRI|G_IO_ERR|G_IO_HUP|G_IO_NVAL, xarchiver_error_function, NULL ) )
			return FALSE;
			*/
		xa_support_execute(support);
		g_string_free (names, TRUE);
	}
	fchdir(n_cwd);
	return TRUE;
}

gint
xa_support_zip_extract(XASupport *support, XAArchive *archive, gchar *destination_path, GSList *files, gboolean full_path)
{
	GString *names;

	GSList *_files = files;
	if(!g_file_test(archive->path, G_FILE_TEST_EXISTS))
		return FALSE;

    //This extracts the whole archive
	if( (files == NULL) && (g_slist_length(files) == 0))
	{
		if (archive->has_passwd)
			support->exec.command = g_strconcat ( "unzip -o -P " , archive->passwd , " " , archive->path , " -d " , destination_path , NULL );
		else
			support->exec.command = g_strconcat ( "unzip -o " , archive->path , " -d " , destination_path , NULL );
	} 
	else
	{
		names = concatenatefilenames ( _files , TRUE );
		if ( archive->has_passwd)
			support->exec.command = g_strconcat ( "unzip -o -P " , archive->passwd , full_path ? " " : " -j " , archive->path , names->str , " -d " , destination_path , NULL );
		else
			support->exec.command = g_strconcat ( "unzip -o " , full_path ? "" : "-j " , archive->path , names->str , " -d " , destination_path , NULL );
		g_string_free (names, TRUE);
	}
	xa_support_execute(support);
	g_free(support->exec.command);
	fchdir(n_cwd);
	return TRUE;
}

gint
xa_support_zip_testing (XASupport *support, XAArchive *archive)
{
	if(!g_file_test(archive->path, G_FILE_TEST_EXISTS))
		return FALSE;
        
	if (archive->has_passwd)
		support->exec.command = g_strconcat ("unzip -P ", archive->passwd, " -t " , archive->path, NULL);
	else
		support->exec.command = g_strconcat ("unzip -t " , archive->path, NULL);

	xa_support_execute(support);
	g_free (support->exec.command);
	return TRUE;
}

gint
xa_support_zip_remove (XASupport *support, XAArchive *archive, GSList *files)
{
	GString *names;

	GSList *_files = files;
	names = concatenatefilenames ( _files , TRUE );
	archive->status = REMOVE;
	support->exec.command = g_strconcat ( "zip -d " , archive->path , names->str , NULL );
	xa_support_execute(support);
	g_string_free (names, TRUE);
	g_free (support->exec.command);
	return TRUE;
}

/*
 * xarchive_zip_support_open(XArchive *archive)
 * Open the archive and calls other functions to catch the output in the archive->output g_slist
 *
 */

gint
xa_support_zip_open (XASupport *support, XAArchive *archive)
{
	support->exec.command = g_strconcat ("unzip -vl -qq " , archive->path, NULL );
	xa_support_execute(support);
	g_free (support->exec.command);
	archive->dummy_size = 0;
	return TRUE;
}

/*
 * xarchiver_parse_zip_output
 * Parse the output from the zip command when opening the archive
 *
 */

gint xa_support_zip_parse_output (GIOChannel *ioc, GIOCondition cond, gpointer data)
{
	XASupport *support = XA_SUPPORT(data);
	gchar *line = NULL;
	XAArchive *archive = support->exec.archive;
	
	if (cond & (G_IO_IN | G_IO_PRI) )
	{
		g_io_channel_read_line ( ioc, &line, NULL, NULL, NULL );
		if ( line == NULL ) return TRUE;
		/*if ( ! archive->status == RELOAD ) archive->output = g_slist_prepend (archive->output , line);*/
		archive->row = get_last_field ( archive->row , line , 8);
		archive->row = split_line ( archive->row , line , 7 );
		if ( g_str_has_suffix (g_list_nth_data ( archive->row , 7) , "/") == TRUE)
			archive->nr_of_dirs++;
		else
			archive->nr_of_files++;
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

XASupport*
xa_support_zip_new()
{
	XASupport *support;

	support = g_object_new(XA_TYPE_SUPPORT_ZIP, NULL);
	
	return support;
}

