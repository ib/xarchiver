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
#include "support-arj.h"

#define _(String) gettext(String)

void
xa_support_arj_init(XASupportArj *support);

gint
xa_support_arj_open(XASupport *support, XAArchive *archive);

gint
xa_support_arj_add (XASupport *support, XAArchive *archive, GSList *files);

gint
xa_support_arj_remove (XASupport *support, XAArchive *archive, GSList *files);

gint
xa_support_arj_extract(XASupport *support, XAArchive *archive, gchar *destination_path, GSList *files, gboolean full_path);

gboolean 
xa_support_arj_parse_output (GIOChannel *ioc, GIOCondition cond, gpointer data);

gint
xa_support_arj_testing(XASupport *support, XAArchive *archive);

GType
xa_support_arj_get_type ()
{
	static GType xa_support_arj_type = 0;

 	if (!xa_support_arj_type)
	{
 		static const GTypeInfo xa_support_arj_info = 
		{
			sizeof (XASupportArjClass),
			(GBaseInitFunc) NULL,
			(GBaseFinalizeFunc) NULL,
			(GClassInitFunc) NULL,
			(GClassFinalizeFunc) NULL,
			NULL,
			sizeof (XASupportArj),
			0,
			(GInstanceInitFunc) xa_support_arj_init,
			NULL
		};

		xa_support_arj_type = g_type_register_static (XA_TYPE_SUPPORT, "XASupportArj", &xa_support_arj_info, 0);
	}
	return xa_support_arj_type;
}

void
xa_support_arj_init(XASupportArj *support)
{
	XASupport *xa_support = XA_SUPPORT(support);
	gint n_columns = 9;
	gchar **column_names  = g_new0(gchar *, n_columns);
	GType *column_types  = g_new0(GType, n_columns);
	
	column_names[0] = "Filename";
	column_names[1] = "Original";
	column_names[2] = "Compressed";
	column_names[3] = "Ratio";
	column_names[4] = "Date";
	column_names[5] = "Time";
	column_names[6] = "Attr";
	column_names[7] = "GUA";
	column_names[8] = "BPMGS";
	column_types[0] = G_TYPE_STRING;
	column_types[1] = G_TYPE_UINT64;
	column_types[2] = G_TYPE_UINT64;
	column_types[3] = G_TYPE_STRING;
	column_types[4] = G_TYPE_STRING;
	column_types[5] = G_TYPE_STRING;
	column_types[6] = G_TYPE_STRING;
	column_types[7] = G_TYPE_STRING;
	column_types[8] = G_TYPE_STRING;
	
	xa_support_set_columns(xa_support, n_columns, column_names, column_types);
	xa_support->type    = XARCHIVETYPE_ARJ;
	xa_support->verify  = xa_archive_type_arj_verify;
	xa_support->open    = xa_support_arj_open;
	xa_support->add     = xa_support_arj_add;
	xa_support->remove  = xa_support_arj_remove;
	xa_support->extract = xa_support_arj_extract;
	xa_support->testing = xa_support_arj_testing;
	xa_support->parse_output = xa_support_arj_parse_output;
	
	g_free (column_names);
	g_free(column_types);
}

gint xa_support_arj_parse_output (GIOChannel *ioc, GIOCondition cond, gpointer data)
{
	XASupport *support = XA_SUPPORT(data);
	gchar *line = NULL;
	gchar *start = NULL;
	gchar *end = NULL;
	GValue *filename = NULL;
	GValue *original = NULL;
	gchar *_original = NULL;
	GValue *compressed = NULL;
	gchar *_compressed = NULL;
	GValue *ratio = NULL;
	GValue *date = NULL;
	GValue *time = NULL;
	GValue *attr = NULL;
	GValue *gua = NULL;
	GValue *bpmgs = NULL;
	XAArchive *archive = support->exec.archive;

	if (cond & (G_IO_IN | G_IO_PRI) )
	{
		/* This to avoid inserting in the liststore arj copyright message */
		if (jump_header == FALSE )
		{
			g_io_channel_read_line ( ioc, &line, NULL, NULL, NULL );
			if (line == NULL)
				return TRUE;
			if  (strncmp (line , "------------" , 12) == 0)
			{
				jump_header = TRUE;
				arj_line = 1;
			}
			return TRUE;
		}
		if (arj_line == 4)
		{
			arj_line = 1;
			return TRUE;
		}
		if (arj_line == 1)
		{
			/* This to avoid reading the last line of arj output */
			g_io_channel_read_line ( ioc, &line, NULL, NULL, NULL );
			if (line == NULL)
				return TRUE;
			if (strncmp (line, "------------", 12) == 0 || strncmp (line, "\x0a",1) == 0)
			{
				g_free (line);
				g_io_channel_read_line ( ioc, &line, NULL, NULL, NULL );
				if (line != NULL) 
					g_free (line);
				return TRUE;
			}
			start = eat_spaces (line);
			end = strchr (start, ' ');
			
			start = eat_spaces (end);
			end = strchr (start, '\n');

			filename = g_new0(GValue, 1);
			filename = g_value_init(filename, G_TYPE_STRING);
			g_value_set_string (filename , g_strndup ( start , end - start));
			archive->row = g_list_prepend (archive->row ,  filename);
			g_free (line);
		}
		else if (arj_line == 2)
		{
			g_io_channel_read_line ( ioc, &line, NULL, NULL, NULL );
			if ( line == NULL)
				return TRUE;
			//g_message (line);
			original    = g_new0(GValue, 1);
			compressed  = g_new0(GValue, 1);
			ratio       = g_new0(GValue, 1);
			date        = g_new0(GValue, 1);
			time        = g_new0(GValue, 1);
			attr        = g_new0(GValue, 1);
			gua         = g_new0(GValue, 1);
			bpmgs       = g_new0(GValue, 1);
			archive->row_cnt++;
			
			/* The following to avoid the first and second field of the second line of arj output */
			start = eat_spaces (line);
			end = strchr (start, ' ');
			start = eat_spaces (end);
			end = strchr (start, ' ');
			
			start = eat_spaces (end);
			end = strchr (start, ' ');
			original = g_new0(GValue, 1);
			original = g_value_init(original, G_TYPE_UINT64);
			_original = g_strndup ( start , end - start);
			g_value_set_uint64 ( original , atoll ( _original ) );
			g_free (_original);
			
			start = eat_spaces (end);
			end = strchr (start, ' ');
			compressed = g_new0 (GValue, 1);
			compressed = g_value_init(compressed, G_TYPE_UINT64);
			_compressed = g_strndup ( start , end - start);
			g_value_set_uint64 ( compressed , atoll ( _compressed ) );
			g_free (_compressed);

			start = eat_spaces (end);
			end = strchr (start, ' ');
			ratio = g_new0(GValue, 1);
			ratio = g_value_init(ratio, G_TYPE_STRING);
			g_value_set_string (ratio , g_strndup ( start , end - start));	
			
			start = eat_spaces (end);
			end = strchr (start, ' ');
			date = g_new0(GValue, 1);
			date = g_value_init(date, G_TYPE_STRING);
			g_value_set_string (date , g_strndup ( start , end - start));

			start = eat_spaces (end);
			end = strchr (start, ' ');
			time = g_new0(GValue, 1);
			time = g_value_init(time, G_TYPE_STRING);
			g_value_set_string (time , g_strndup ( start , end - start));
			
			start = eat_spaces (end);
			end = strchr (start, ' ');
			attr = g_new0(GValue, 1);
			attr = g_value_init(attr, G_TYPE_STRING);
			g_value_set_string (attr , g_strndup ( start , end - start));

			gua = g_new0(GValue, 1);
			gua = g_value_init(gua, G_TYPE_STRING);
			start = eat_spaces (end);
			if (*start == '\n')
			{
				no_attr = TRUE;
				g_value_set_string (gua , g_strdup (" ") );
				
			}
			else
			{
				end = strchr (start, ' ');
				g_value_set_string (gua , g_strndup ( start , end - start));
			}

			start = eat_spaces (end);
			end = strchr (start, '\n');
			bpmgs = g_new0(GValue, 1);
			bpmgs = g_value_init(bpmgs, G_TYPE_STRING);
			if ( ! no_attr)
				g_value_set_string (bpmgs , g_strndup ( start , end - start));
			else
			{
				g_value_set_string (bpmgs , g_value_get_string(attr) );
				g_value_set_string (attr , g_strdup (" ") );
			}
			
			archive->row = g_list_prepend (archive->row , original) ;
			archive->row = g_list_prepend (archive->row , compressed );
			archive->row = g_list_prepend (archive->row , ratio );
			archive->row = g_list_prepend (archive->row , date );
			archive->row = g_list_prepend (archive->row , time );
			archive->row = g_list_prepend (archive->row , attr );
			archive->row = g_list_prepend (archive->row , gua );
			archive->row = g_list_prepend (archive->row , bpmgs );
			no_attr = FALSE;
			if (  g_str_has_suffix (g_value_get_string (attr) , "d") == FALSE)
				archive->nr_of_files++;
			archive->dummy_size += g_value_get_uint64 (compressed );
			g_free (line);
			if (archive->row_cnt > 99)
			{
				xa_support_emit_signal(support, XA_SUPPORT_SIGNAL_APPEND_ROWS);
				archive->row_cnt = 0;
			}
		}
		else if (arj_line == 3)
		{	
			/* Let's discard the third and forth line of arj output */
			g_io_channel_read_line ( ioc, &line, NULL, NULL, NULL );
			g_free (line);
			g_io_channel_read_line ( ioc, &line, NULL, NULL, NULL );
			g_free (line);
			//arj_line = 4;
			//return TRUE;
		}
	}
	else if (cond & (G_IO_ERR | G_IO_HUP | G_IO_NVAL) )
	{
		g_io_channel_shutdown ( ioc,TRUE,NULL );
		g_io_channel_unref (ioc);
		xa_support_emit_signal (support, XA_SUPPORT_SIGNAL_APPEND_ROWS);
		xa_support_emit_signal (support, XA_SUPPORT_SIGNAL_OPERATION_COMPLETE);
		return FALSE;
	}
	arj_line++;
	return TRUE;
}

gboolean
xa_support_arj_add (XASupport *support, XAArchive *archive, GSList *files)
{
	gchar *dir;
	GString *names;

	GSList *_files = files;
	if(files != NULL)
	{
		dir = g_path_get_dirname(files->data);
		chdir(dir);
		g_free(dir);
 		names = concatenatefilenames ( _files, TRUE );
		if (archive->has_passwd)
			support->exec.command = g_strconcat ( "arj a -i -r -g" , archive->passwd , " " , archive->path , names->str , NULL );
		else
			support->exec.command = g_strconcat ( "arj a -i -r " , archive->path , names->str , NULL );
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
xa_support_arj_extract(XASupport *support, XAArchive *archive, gchar *destination_path, GSList *files, gboolean full_path)
{
	GString *names;

	GSList *_files = files;
	if(!g_file_test(archive->path, G_FILE_TEST_EXISTS))
		return FALSE;

    //This extracts the whole archive
	if( (files == NULL) && (g_slist_length(files) == 0))
	{
		if (archive->has_passwd)
			support->exec.command = g_strconcat ( "arj x -g",archive->passwd," -i -y " , archive->path , " " , destination_path , NULL );
		else
			support->exec.command = g_strconcat ( "arj x -i -y " , archive->path , " " , destination_path , NULL );
	} 
	else
	{
		names = concatenatefilenames ( _files, TRUE );
		if ( archive->has_passwd)
			support->exec.command = g_strconcat ( "arj x -g",archive->passwd," -i -y " , archive->path , " " , destination_path , names->str , NULL );
		else
			support->exec.command = g_strconcat ( "arj ",full_path ? "x" : "e"," -i -y " , archive->path , " " , destination_path , names->str, NULL );
		g_string_free (names, TRUE);
	}
	support->exec.archive = archive;
	support->exec.signal = -1;
	support->exec.parse_output = 0;

	xa_support_execute(support);
	g_free (support->exec.command);
	fchdir(n_cwd);
	return TRUE;
}

gint
xa_support_arj_testing (XASupport *support, XAArchive *archive)
{
	if(!g_file_test(archive->path, G_FILE_TEST_EXISTS))
		return FALSE;
        
	if (archive->has_passwd)
		support->exec.command = g_strconcat ("arj t -g" , archive->passwd , " -i " , archive->path, NULL);
	else
		support->exec.command = g_strconcat ("arj t -i " , archive->path, NULL);
	support->exec.archive = archive;
	support->exec.signal = -1;
	support->exec.parse_output = 0;
	
	xa_support_execute(support);
	g_free (support->exec.command);
	return TRUE;
}

gint
xa_support_arj_remove (XASupport *support, XAArchive *archive, GSList *files)
{
	GString *names;

	GSList *_files = files;
	names = concatenatefilenames ( _files, FALSE );
	support->exec.command = g_strconcat ( "arj d " , archive->path , names->str, NULL);
	support->exec.archive = archive;
	support->exec.signal = XA_SUPPORT_SIGNAL_ARCHIVE_MODIFIED;
	support->exec.parse_output = 0;

	xa_support_execute(support);
	g_string_free (names, TRUE);
	g_free (support->exec.command);
	return TRUE;
}

gint
xa_support_arj_open (XASupport *support, XAArchive *archive)
{
	jump_header = FALSE;
    support->exec.command = g_strconcat ( "arj v -he " , archive->path, NULL );
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
xa_support_arj_new()
{
	XASupport *support;

	support = g_object_new(XA_TYPE_SUPPORT_ARJ, NULL);
	
	return support;
}

