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
#include "support-rar.h"

#define _(String) gettext(String)

void
xa_support_rar_init(XASupportRar *support);

gint
xa_support_rar_open(XASupport *support, XAArchive *archive);

gint
xa_support_rar_add (XASupport *support, XAArchive *archive, GSList *files);

gint
xa_support_rar_remove (XASupport *support, XAArchive *archive, GSList *files);

gint
xa_support_rar_extract(XASupport *support, XAArchive *archive, gchar *destination_path, GSList *files, gboolean full_path);

gboolean 
xa_support_rar_parse_output (GIOChannel *ioc, GIOCondition cond, gpointer data);

gint
xa_support_rar_testing(XASupport *support, XAArchive *archive);

GType
xa_support_rar_get_type ()
{
	static GType xa_support_rar_type = 0;

 	if (!xa_support_rar_type)
	{
 		static const GTypeInfo xa_support_rar_info = 
		{
			sizeof (XASupportRarClass),
			(GBaseInitFunc) NULL,
			(GBaseFinalizeFunc) NULL,
			(GClassInitFunc) NULL,
			(GClassFinalizeFunc) NULL,
			NULL,
			sizeof (XASupportRar),
			0,
			(GInstanceInitFunc) xa_support_rar_init,
			NULL
		};

		xa_support_rar_type = g_type_register_static (XA_TYPE_SUPPORT, "XASupportRar", &xa_support_rar_info, 0);
	}
	return xa_support_rar_type;
}

void
xa_support_rar_init(XASupportRar *support)
{
	XASupport *xa_support = XA_SUPPORT(support);
	gint n_columns = 10;
	gchar **column_names  = g_new0(gchar *, n_columns);
	GType *column_types  = g_new0(GType, n_columns);

	column_names[0] = _("Filename");
	column_names[1] = _("Original");
	column_names[2] = _("Compressed");
	column_names[3] = _("Ratio");
	column_names[4] = _("Date");
	column_names[5] = _("Time");
	column_names[6] = _("Permissions");
	column_names[7] = _("CRC");
	column_names[8] = _("Method");
	column_names[9] = _("Version");
	column_types[0] = G_TYPE_STRING;
	column_types[1] = G_TYPE_UINT64;
	column_types[2] = G_TYPE_UINT64;
	column_types[3] = G_TYPE_STRING;
	column_types[4] = G_TYPE_STRING;
	column_types[5] = G_TYPE_STRING;
	column_types[6] = G_TYPE_STRING;
	column_types[7] = G_TYPE_STRING;
	column_types[8] = G_TYPE_STRING;
	column_types[9] = G_TYPE_STRING;

	xa_support_set_columns(xa_support, n_columns, column_names, column_types);
	xa_support->type    = XARCHIVETYPE_RAR;
	xa_support->verify  = xa_archive_type_rar_verify;
	xa_support->open    = xa_support_rar_open;
	xa_support->add     = xa_support_rar_add;
	xa_support->remove  = xa_support_rar_remove;
	xa_support->extract = xa_support_rar_extract;
	xa_support->testing = xa_support_rar_testing;
	xa_support->parse_output = xa_support_rar_parse_output;
	
	g_free (column_names);
	g_free(column_types);
}

gint xa_support_rar_parse_output (GIOChannel *ioc, GIOCondition cond, gpointer data)
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
	GValue *permissions = NULL;
	GValue *crc = NULL;
	GValue *method = NULL;
	GValue *version = NULL;
	XAArchive *archive = support->exec.archive;
	
	if (cond & (G_IO_IN | G_IO_PRI) )
	{
		/* This to avoid inserting in the liststore RAR's copyright message */
		if (jump_header == FALSE )
		{
			g_io_channel_read_line ( ioc, &line, NULL, NULL, NULL );
			if (line == NULL)
				return TRUE;
			if  (strncmp (line , "--------" , 8) == 0)
			{
				jump_header = TRUE;
				odd_line = TRUE;
			}
			g_free (line);
			return TRUE;
		}
		if ( odd_line )
		{
			g_io_channel_read_line ( ioc, &line, NULL, NULL, NULL );
			/* Let's parse the filename */
			if (line == NULL)
				return TRUE;
			{
				/* This to avoid inserting in the liststore the last line of Rar output */
				if (strncmp (line, "--------", 8) == 0 || strncmp (line, "\x0a",1) == 0)
				{
					/* Let's discard the last line of the output */
					g_io_channel_read_line ( ioc, &line, NULL, NULL, NULL );
					g_free (line);
					return TRUE;
				}
				if (line[0] == '*') archive->has_passwd = TRUE;
				/* This to avoid the white space or the * before the first char of the filename */
				line++;
				filename = g_new0(GValue, 1);
				filename = g_value_init (filename, G_TYPE_STRING);
				g_value_set_string (filename, g_strndup (line , strlen (line) -1 ) );
				archive->row = g_list_prepend (archive->row , filename);
				/* Restore the pointer before freeing it */
				line--;
				g_free (line);
				odd_line = ! odd_line;
			}
		}
		else
		{
			/* Now let's parse the rest of the info */
			g_io_channel_read_line ( ioc, &line, NULL, NULL, NULL );
			if (line == NULL)
				return TRUE;
			archive->row_cnt++;
			start = eat_spaces (line);
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
			permissions = g_new0(GValue, 1);
			permissions = g_value_init(permissions, G_TYPE_STRING);
			g_value_set_string (permissions , g_strndup ( start , end - start));

			start = eat_spaces (end);
			end = strchr (start, ' ');
			crc = g_new0(GValue, 1);
			crc = g_value_init(crc, G_TYPE_STRING);
			g_value_set_string (crc , g_strndup ( start , end - start));

			start = eat_spaces (end);
			end = strchr (start, ' ');
			method = g_new0(GValue, 1);
			method = g_value_init(method, G_TYPE_STRING);
			g_value_set_string (method , g_strndup ( start , end - start));	
		
			start = eat_spaces (end);
			end = strchr (start, '\n');
			version = g_new0(GValue, 1);
			version = g_value_init(version, G_TYPE_STRING);
			g_value_set_string (version , g_strndup ( start , end - start));	
		
			archive->row = g_list_prepend (archive->row , original) ;
			archive->row = g_list_prepend (archive->row , compressed );
			archive->row = g_list_prepend (archive->row , ratio );
			archive->row = g_list_prepend (archive->row , date );
			archive->row = g_list_prepend (archive->row , time );
			archive->row = g_list_prepend (archive->row , permissions );
			archive->row = g_list_prepend (archive->row , crc );
			archive->row = g_list_prepend (archive->row , method );
			archive->row = g_list_prepend (archive->row , version );

			if ( strstr ((gchar *)g_list_nth_data ( archive->row,3) , "d") == NULL && strstr ((gchar *)g_list_nth_data ( archive->row,3) , "D") == NULL )
				archive->nr_of_files++;
			else
				archive->nr_of_dirs++;

			archive->dummy_size += g_value_get_uint64 (compressed );
			odd_line = ! odd_line;
			g_free (line);
			if (archive->row_cnt > 99)
			{
				xa_support_emit_signal(support, XA_SUPPORT_SIGNAL_APPEND_ROWS);
				archive->row_cnt = 0;
			}
		}
		return TRUE;
	}
	else if (cond & (G_IO_ERR | G_IO_HUP ) )
	{
		g_io_channel_shutdown ( ioc,TRUE,NULL );
		g_io_channel_unref (ioc);
		xa_support_emit_signal (support, XA_SUPPORT_SIGNAL_APPEND_ROWS);
		xa_support_emit_signal (support, XA_SUPPORT_SIGNAL_OPERATION_COMPLETE);
		return FALSE;
	}
	return TRUE;
}

gboolean
xa_support_rar_add (XASupport *support, XAArchive *archive, GSList *files)
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
			support->exec.command = g_strconcat ( "rar a -p" , archive->passwd, " -o+ -ep1 -idp " , archive->path , names->str , NULL );
		else
			support->exec.command = g_strconcat ( "rar a -o+ -ep1 -idp " , archive->path , names->str , NULL );
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
xa_support_rar_extract(XASupport *support, XAArchive *archive, gchar *destination_path, GSList *files, gboolean full_path)
{
	GString *names;

	GSList *_files = files;
	if(!g_file_test(archive->path, G_FILE_TEST_EXISTS))
		return FALSE;

    //This extracts the whole archive
	if( (files == NULL) && (g_slist_length(files) == 0))
	{
		if (archive->has_passwd)
			support->exec.command = g_strconcat ( "rar x -p",archive->passwd," -o+ -idp " , archive->path , " " , destination_path , NULL );
		else
			support->exec.command = g_strconcat ( "rar x -o+ -idp " , archive->passwd , " " , destination_path , NULL );
	} 
	else
	{
		names = concatenatefilenames ( _files , TRUE );
		if ( archive->has_passwd)
			support->exec.command = g_strconcat ( "rar " , full_path ? "x" : "e" , " -p",archive->passwd, " -o+ -idp " , archive->path , " " , names->str , " " , destination_path , NULL );
		else
			support->exec.command = g_strconcat ( "rar ", full_path ? "x" : "e" , " -o+ -idp " , archive->path , " " , names->str , " ", destination_path ,NULL);
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
xa_support_rar_testing (XASupport *support, XAArchive *archive)
{
	if(!g_file_test(archive->path, G_FILE_TEST_EXISTS))
		return FALSE;
	
	if (archive->has_passwd)
		support->exec.command = g_strconcat ("rar t -idp -p" , archive->passwd ," " , archive->path, NULL);
	else
		support->exec.command = g_strconcat ("rar t -idp " , archive->path, NULL);
	support->exec.archive = archive;
	support->exec.signal = -1;
	support->exec.parse_output = 0;
	
	xa_support_execute(support);
	g_free (support->exec.command);
	return TRUE;
}

gint
xa_support_rar_remove (XASupport *support, XAArchive *archive, GSList *files)
{
	GString *names;

	GSList *_files = files;
	names = concatenatefilenames ( _files , TRUE );
	support->exec.command = g_strconcat ( "rar d " , archive->path , names->str , NULL );
	support->exec.archive = archive;
	support->exec.signal = 1;
	support->exec.parse_output = 0;

	xa_support_execute(support);
	g_string_free (names, TRUE);
	g_free (support->exec.command);
	return TRUE;
}

gint
xa_support_rar_open (XASupport *support, XAArchive *archive)
{
	jump_header = FALSE;
	odd_line = FALSE;
	support->exec.command = g_strconcat ( "rar vl -c- " , archive->path, NULL );
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
xa_support_rar_new()
{
	XASupport *support;

	support = g_object_new(XA_TYPE_SUPPORT_RAR, NULL);
	
	return support;
}

