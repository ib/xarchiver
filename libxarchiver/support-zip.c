/*
 *  Copyright (c) 2006 Stephan Arts      <psybsd@gmail.com>
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
#include <libintl.h>
#include "internals.h"
#include "archive.h"
#include "archive-types.h"
#include "support.h"
#include "support-zip.h"

#define _(String) gettext(String)

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
	column_types[1] = G_TYPE_UINT64;
	column_types[2] = G_TYPE_STRING;
	column_types[3] = G_TYPE_UINT64;
	column_types[4] = G_TYPE_STRING;
	column_types[5] = G_TYPE_STRING;
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

gint xa_support_zip_parse_output (GIOChannel *ioc, GIOCondition cond, gpointer data)
{
	XASupport *support = XA_SUPPORT(data);
	GIOStatus status = 0;
	GError *error = NULL;
	gchar *line = NULL;
	gchar *start = NULL;
	gchar *end = NULL;
	GValue  *filename = NULL;
	GValue *original = NULL;
	gchar *_original = NULL;
	GValue *method = NULL;
	GValue *compressed = NULL;
	gchar *_compressed = NULL;
	GValue *ratio = NULL;
	GValue *date = NULL;
	GValue *time = NULL;
	GValue *crc32 = NULL;
	
	XAArchive *archive = support->exec.archive;

	if (cond & (G_IO_IN | G_IO_PRI) )
	{
			status =  g_io_channel_read_line ( ioc, &line, NULL, NULL, &error);
			if ((line == NULL) || ( status != G_IO_STATUS_NORMAL))
				return TRUE;

			filename    = g_new0(GValue, 1);
			original    = g_new0(GValue, 1);
			method      = g_new0(GValue, 1);
			compressed  = g_new0(GValue, 1);
			ratio       = g_new0(GValue, 1);
			date        = g_new0(GValue, 1);
			time        = g_new0(GValue, 1);
			crc32       = g_new0(GValue, 1);
			archive->row_cnt++;

			start = eat_spaces (line);
			end = strchr (start, ' ');
			original = g_value_init(original, G_TYPE_UINT64);
			_original = g_strndup ( start , end - start);
			g_value_set_uint64 ( original , atoll (_original) );
			g_free (_original);

			start = eat_spaces (end);
			end = strchr (start, ' ');
			method = g_value_init(method, G_TYPE_STRING);
			g_value_set_string ( method , g_strndup ( start , end - start) );

			start = eat_spaces (end);
			end = strchr (start, ' ');
			compressed = g_value_init(compressed, G_TYPE_UINT64);
			_compressed  = g_strndup ( start , end - start);
			g_value_set_uint64 (compressed , atoll (_compressed) );
			g_free (_compressed);

			start = eat_spaces (end);
			end = strchr (start, ' ');
			ratio = g_value_init(ratio, G_TYPE_STRING);
			g_value_set_string ( ratio , g_strndup ( start , end - start) );

			start = eat_spaces (end);
			end = strchr (start, ' ');
			date = g_value_init(date, G_TYPE_STRING);
			g_value_set_string ( date , g_strndup ( start , end - start) );
			
			start = eat_spaces (end);
			end = strchr (start, ' ');
			time = g_value_init(time, G_TYPE_STRING);
			g_value_set_string ( time , g_strndup ( start , end - start) );
			
			start = eat_spaces (end);
			end = strchr (start, ' ');
			crc32 = g_value_init(crc32, G_TYPE_STRING);
			g_value_set_string ( crc32 , g_strndup ( start , end - start) );
		
			start = eat_spaces (end);
			end = strchr (start, '\n');
			filename = g_value_init(filename, G_TYPE_STRING);
			g_value_set_string ( filename , g_strndup ( start , end - start) );

			archive->row = g_list_prepend(archive->row, filename);
			archive->row = g_list_prepend(archive->row, original);
			archive->row = g_list_prepend(archive->row,method);
			archive->row = g_list_prepend(archive->row, compressed);
			archive->row = g_list_prepend(archive->row, ratio);
			archive->row = g_list_prepend(archive->row, date);
			archive->row = g_list_prepend(archive->row, time);
			archive->row = g_list_prepend(archive->row, crc32);

			if ( g_str_has_suffix (g_value_get_string (filename) , "/") == TRUE)
				archive->nr_of_dirs++;
			else
				archive->nr_of_files++;
			archive->dummy_size += g_value_get_uint64 (compressed);
			g_free(line);

		if(status == G_IO_STATUS_NORMAL && archive->row_cnt > 99)
		{
			xa_support_emit_signal(support, XA_SUPPORT_SIGNAL_APPEND_ROWS);
			archive->row_cnt = 0;
		}
		else if(status == G_IO_STATUS_ERROR)
		{
			g_warning("ERR: %s\n", error->message);
			g_error_free (error);
		}

		if(status == G_IO_STATUS_EOF)
		{
			xa_support_emit_signal(support, XA_SUPPORT_SIGNAL_APPEND_ROWS);
			xa_support_emit_signal (support, XA_SUPPORT_SIGNAL_OPERATION_COMPLETE);
			return FALSE;
		}
		return TRUE;
	}
	else if (cond & (G_IO_ERR | G_IO_HUP) )
	{
		g_io_channel_shutdown ( ioc,TRUE,NULL );
		g_io_channel_unref (ioc);
		xa_support_emit_signal(support, XA_SUPPORT_SIGNAL_APPEND_ROWS);
		xa_support_emit_signal (support, XA_SUPPORT_SIGNAL_OPERATION_COMPLETE);
		return FALSE;
	}
	return TRUE;
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
		support->exec.archive = archive;
		support->exec.signal = 1;
		support->exec.parse_output = 0;
		
		xa_support_execute(support);
		g_free (support->exec.command);
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
	support->exec.archive = archive;
	support->exec.signal = -1;
	support->exec.parse_output = 0;

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
	support->exec.archive = archive;
	support->exec.signal = -1;
	support->exec.parse_output = 0;

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
	support->exec.command = g_strconcat ( "zip -d " , archive->path , names->str , NULL );
	g_print ("%s\n",support->exec.command);
	support->exec.archive = archive;
	support->exec.signal = 1;
	support->exec.parse_output = 0;
	
	xa_support_execute(support);
	g_string_free (names, TRUE);
	g_free (support->exec.command);
	return TRUE;
}

gint
xa_support_zip_open (XASupport *support, XAArchive *archive)
{
	support->exec.command = g_strconcat ("unzip -vl -qq " , archive->path, NULL );
	support->exec.archive = archive;
	support->exec.parse_output = support->parse_output;
	support->exec.signal = -1;
	
	xa_support_emit_signal(support, XA_SUPPORT_SIGNAL_UPDATE_ROWS);

	xa_support_execute(support);
	g_free (support->exec.command);
	archive->dummy_size = 0;
	archive->row_cnt = 0;
	return TRUE;
}

XASupport*
xa_support_zip_new()
{
	XASupport *support;

	support = g_object_new(XA_TYPE_SUPPORT_ZIP, NULL);
	
	return support;
}

