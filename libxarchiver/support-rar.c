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
#include "support-rar.h"

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

	column_names[0] = "Filename";
	column_names[1] = "Original";
	column_names[2] = "Compressed";
	column_names[3] = "Ratio";
	column_names[4] = "Date";
	column_names[5] = "Time";
	column_names[6] = "Permissions";
	column_names[7] = "CRC";
	column_names[8] = "Method";
	column_names[9] = "Version";
	column_types[0] = G_TYPE_STRING;
	column_types[1] = G_TYPE_UINT;
	column_types[2] = G_TYPE_UINT;
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
	gchar *filename = NULL;
	gchar *original = NULL;
	gchar *compressed = NULL;
	gchar *ratio = NULL;
	gchar *date = NULL;
	gchar *time = NULL;
	gchar *permissions = NULL;
	gchar *crc = NULL;
	gchar *method = NULL;
	gchar *version = NULL;
	unsigned long int _original   = 0;
	unsigned long int _compressed = 0;
	XAArchive *archive = support->exec.archive;
	
	if (cond & (G_IO_IN | G_IO_PRI) )
	{
		/* This to avoid inserting in the liststore RAR's copyright message */
		if (jump_header == FALSE )
		{
			g_io_channel_read_line ( ioc, &line, NULL, NULL, NULL );
			if (line == NULL) return TRUE;
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
			/* Let's read the filename */
			g_io_channel_read_line ( ioc, &line, NULL, NULL, NULL );
			if ( line == NULL ) return TRUE;
			/* This to avoid inserting in the liststore the last line of Rar output */
			if (strncmp (line, "--------", 8) == 0 || strncmp (line, "\x0a",1) == 0)
			{
				/* Let's read the last line of the output and discard it */
				g_io_channel_read_line ( ioc, &line, NULL, NULL, NULL );
				g_free (line);
				return TRUE;
			}
			if (line[0] == '*') archive->has_passwd = TRUE;
			/* This to avoid the white space or the * before the first char of the filename */
			line++;
			filename = g_strndup (line , strlen (line) -1 );
			archive->row = g_list_prepend (archive->row , filename);
			/* Restore the pointer before freeing it */
			line--;
			g_free (line);
			odd_line = ! odd_line;
			return TRUE;
		}
		else
		{
			/* Now let's read the rest of the info */
			g_io_channel_read_line ( ioc, &line, NULL, NULL, NULL );
			if ( line == NULL) return TRUE;
			start = eat_spaces (line);
			end = strchr (start, ' ');
			original = g_strndup ( start , end - start);
			//_original = atoll (original);
		//	g_free (original);

			start = eat_spaces (end);
			end = strchr (start, ' ');
			compressed  = g_strndup ( start , end - start);
			//_compressed = atoll (compressed);
		//	g_free (compressed);

			start = eat_spaces (end);
			end = strchr (start, ' ');
			ratio = g_strndup ( start , end - start);

			start = eat_spaces (end);
			end = strchr (start, ' ');
			date = g_strndup ( start , end - start);

			start = eat_spaces (end);
			end = strchr (start, ' ');
			time = g_strndup ( start , end - start);

			start = eat_spaces (end);
			end = strchr (start, ' ');
			permissions = g_strndup ( start , end - start);

			start = eat_spaces (end);
			end = strchr (start, ' ');
			crc = g_strndup ( start , end - start);

			start = eat_spaces (end);
			end = strchr (start, ' ');
			method = g_strndup ( start , end - start);
		
			start = eat_spaces (end);
			end = strchr (start, '\n');
			version = g_strndup ( start , end - start);
			
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
			archive->dummy_size += ( unsigned long int)g_list_nth_data ( archive->row,7);
			odd_line = ! odd_line;
			return TRUE;
		}
	}
	else if (cond & (G_IO_ERR | G_IO_HUP ) )
	{
		g_io_channel_shutdown ( ioc,TRUE,NULL );
		g_io_channel_unref (ioc);
		xa_support_emit_signal(support, 0);
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
	support->exec.command = g_strconcat ( "rar vl -c- " , archive->path, NULL );
	support->exec.archive = archive;
	support->exec.parse_output = support->parse_output;
	support->exec.signal = -1;
	
	xa_support_execute(support);
	g_free (support->exec.command);
	archive->dummy_size = 0;
	return TRUE;
}

XASupport*
xa_support_rar_new()
{
	XASupport *support;

	support = g_object_new(XA_TYPE_SUPPORT_RAR, NULL);
	
	return support;
}

