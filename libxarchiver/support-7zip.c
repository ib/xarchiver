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
#include "support-7zip.h"

#define _(String) gettext(String)

void
xa_support_7zip_init(XASupport7zip *support);

gint
xa_support_7zip_open(XASupport *support, XAArchive *archive);

gint
xa_support_7zip_add (XASupport *support, XAArchive *archive, GSList *files);

gint
xa_support_7zip_remove (XASupport *support, XAArchive *archive, GSList *files);

gint
xa_support_7zip_extract(XASupport *support, XAArchive *archive, gchar *destination_path, GSList *files, gboolean full_path);

gboolean 
xa_support_7zip_parse_output (GIOChannel *ioc, GIOCondition cond, gpointer data);

gint
xa_support_7zip_testing(XASupport *support, XAArchive *archive);

GType
xa_support_7zip_get_type ()
{
	static GType xa_support_7zip_type = 0;

 	if (!xa_support_7zip_type)
	{
 		static const GTypeInfo xa_support_7zip_info = 
		{
			sizeof (XASupport7zipClass),
			(GBaseInitFunc) NULL,
			(GBaseFinalizeFunc) NULL,
			(GClassInitFunc) NULL,
			(GClassFinalizeFunc) NULL,
			NULL,
			sizeof (XASupport7zip),
			0,
			(GInstanceInitFunc) xa_support_7zip_init,
			NULL
		};

		xa_support_7zip_type = g_type_register_static (XA_TYPE_SUPPORT, "XASupport7zip", &xa_support_7zip_info, 0);
	}
	return xa_support_7zip_type;
}

void
xa_support_7zip_init(XASupport7zip *support)
{
	XASupport *xa_support = XA_SUPPORT(support);
	gint n_columns = 6;
	gchar **column_names  = g_new0(gchar *, n_columns);
	GType *column_types  = g_new0(GType, n_columns);
		
	column_names[0] = "Filename";
	column_names[1] = "Compressed";
	column_names[2] = "Original";
	column_names[3] = "Attr";
	column_names[4] = "Time";
	column_names[5] = "Date";
	column_types[0] = G_TYPE_STRING;
	column_types[1] = G_TYPE_UINT64;
	column_types[2] = G_TYPE_UINT64;
	column_types[3] = G_TYPE_STRING;
	column_types[4] = G_TYPE_STRING;
	column_types[5] = G_TYPE_STRING;

	xa_support_set_columns(xa_support, n_columns, column_names, column_types);
	xa_support->type    = XARCHIVETYPE_7ZIP;
	xa_support->verify  = xa_archive_type_7zip_verify;
	xa_support->add     = xa_support_7zip_add;
	xa_support->extract = xa_support_7zip_extract;
	xa_support->testing = xa_support_7zip_testing;
	xa_support->remove  = xa_support_7zip_remove;
	xa_support->open    = xa_support_7zip_open;
	xa_support->parse_output = xa_support_7zip_parse_output;

	g_free (column_names);
	g_free(column_types);
}

gint xa_support_7zip_parse_output (GIOChannel *ioc, GIOCondition cond, gpointer data)
{
    XASupport *support = XA_SUPPORT(data);
	gchar *line = NULL;
	gchar *start = NULL;
	gchar *end = NULL;
	GValue  *filename = NULL;
	GValue *original = NULL;
	gchar *_original = NULL;
	GValue *attr = NULL;
	GValue *compressed = NULL;
	gchar *_compressed = NULL;
	GValue *time = NULL;
	GValue *date = NULL;
	unsigned short int x;

	XAArchive *archive = support->exec.archive;
	if (cond & (G_IO_IN | G_IO_PRI) )
	{
		//This to avoid inserting in the liststore 7zip's message
		if (jump_header == FALSE )
		{
			for ( x = 0; x <= 7; x++)
			{
				g_io_channel_read_line ( ioc, &line, NULL, NULL, NULL );
				if (line != NULL)
					g_free (line);
			}
			jump_header = TRUE;
		}
		g_io_channel_read_line ( ioc, &line, NULL, NULL, NULL );
		if ( line == NULL )
			return TRUE;
		//This to avoid inserting the last line of output
		if (strncmp (line, "-------------------", 19) == 0 || strncmp (line, "\x0a",1) == 0)
		{
			g_free (line);
			g_io_channel_read_line ( ioc, &line, NULL, NULL, NULL );
			if (line != NULL)
				g_free (line);
			return TRUE;
		}
		filename    = g_new0(GValue, 1);
		original    = g_new0(GValue, 1);
		compressed  = g_new0(GValue, 1);
		attr        = g_new0(GValue, 1);
		date        = g_new0(GValue, 1);
		time        = g_new0(GValue, 1);
		archive->row_cnt++;
		
		start = eat_spaces (line);
		end = strchr (start, ' ');
		date = g_value_init(date, G_TYPE_STRING);
		g_value_set_string ( date , g_strndup ( start , end - start) );

		start = eat_spaces (end);
		end = strchr (start, ' ');
		time = g_value_init(time, G_TYPE_STRING);
		g_value_set_string ( time , g_strndup ( start , end - start) );
		
		start = eat_spaces (end);
		end = strchr (start, ' ');
		attr = g_value_init(attr, G_TYPE_STRING);
		g_value_set_string ( attr , g_strndup ( start , end - start) );

		start = eat_spaces (end);
		end = strchr (start, ' ');
		compressed = g_value_init(compressed, G_TYPE_UINT64);
		_compressed  = g_strndup ( start , end - start);
		g_value_set_uint64 (compressed , atoll (_compressed) );
		g_free (_compressed);

		start = eat_spaces (end);
		end = strchr (start, ' ');
		original = g_value_init(original, G_TYPE_UINT64);
		_original = g_strndup ( start , end - start);
		g_value_set_uint64 ( original , atoll (_original) );
		g_free (_original);
		
		start = eat_spaces (end);
		end = strchr (start, '\n');
		filename = g_value_init(filename, G_TYPE_STRING);
		g_value_set_string ( filename , g_strndup ( start , end - start) );

		archive->row = g_list_prepend(archive->row, filename);
		archive->row = g_list_prepend(archive->row, compressed);
		archive->row = g_list_prepend(archive->row, original);
		archive->row = g_list_prepend(archive->row, attr);
		archive->row = g_list_prepend(archive->row, time);
		archive->row = g_list_prepend(archive->row, date);
			
		if ( g_str_has_prefix(g_value_get_string (attr), "D") == FALSE)
			archive->nr_of_files++;
		else
			archive->nr_of_dirs++;
		archive->dummy_size += g_value_get_uint64 (compressed);
		g_free (line);
		if (archive->row_cnt > 99)
		{
			xa_support_emit_signal(support, XA_SUPPORT_SIGNAL_APPEND_ROWS);
			archive->row_cnt = 0;
		}
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
xa_support_7zip_add (XASupport *support, XAArchive *archive, GSList *files)
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
			support->exec.command = g_strconcat ( "7za a -ms=off -p" , archive->passwd , " " , archive->path , names->str , NULL );
        else
			support->exec.command = g_strconcat ( "7za a -ms=off " , archive->path , names->str , NULL );
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

gint xa_support_7zip_extract (XASupport *support, XAArchive *archive, gchar *destination_path, GSList *files, gboolean full_path)
{
	GString *names;

	GSList *_files = files;
	if(!g_file_test(archive->path, G_FILE_TEST_EXISTS))
		return FALSE;

    //This extracts the whole archive
	if( (files == NULL) && (g_slist_length(files) == 0))
	{
		if (archive->has_passwd)
				support->exec.command = g_strconcat ( "7za x -aoa -bd -p",archive->passwd," ", archive->path , " -o" , destination_path , NULL );
        else
			support->exec.command = g_strconcat ( "7za x -aoa -bd " , archive->path , " -o" , destination_path , NULL );
	} 
	else
	{
		names = concatenatefilenames ( _files , TRUE );
		if ( archive->has_passwd)
			support->exec.command = g_strconcat ("7za " , full_path ? "x" : "e" , " -p",archive->passwd," -aoa -bd " , archive->path , names->str , " -o" , destination_path , NULL );
        else
			support->exec.command = g_strconcat ( "7za " , full_path ? "x" : "e" ," -aoa -bd " , archive->path , names->str , " -o" , destination_path , NULL );
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
xa_support_7zip_testing (XASupport *support, XAArchive *archive)
{
	if(!g_file_test(archive->path, G_FILE_TEST_EXISTS))
		return FALSE;
        
	if (archive->has_passwd)
		support->exec.command = g_strconcat ( "7za t -p" , archive->passwd , " " , archive->path, NULL);
	else
		support->exec.command = g_strconcat ("7za t " , archive->path, NULL);
	support->exec.archive = archive;
	support->exec.signal = -1;
	support->exec.parse_output = 0;

	xa_support_execute(support);
	g_free (support->exec.command);
	return TRUE;
}

gint
xa_support_7zip_remove (XASupport *support, XAArchive *archive, GSList *files)
{
	GString *names;

	GSList *_files = files;
	names = concatenatefilenames ( _files , FALSE );
	support->exec.command = g_strconcat ( "7za d " , archive->path , names->str , NULL );
	support->exec.archive = archive;
	support->exec.signal = XA_SUPPORT_SIGNAL_ARCHIVE_MODIFIED;
	support->exec.parse_output = 0;

	xa_support_execute(support);
	g_string_free (names, TRUE);
	g_free (support->exec.command);
	return TRUE;
}

gint
xa_support_7zip_open (XASupport *support, XAArchive *archive)
{
	jump_header = FALSE;

	support->exec.command = g_strconcat ( "7za l " , archive->path, NULL );
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
xa_support_7zip_new()
{
	XASupport *support;

	support = g_object_new(XA_TYPE_SUPPORT_7ZIP, NULL);
	
	return support;
}

