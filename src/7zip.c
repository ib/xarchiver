/*
 *  Copyright (C) 2006 Giuseppe Torelli - <colossus73@gmail.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Street #330, Boston, MA 02111-1307, USA.
 */
 
#include "7zip.h"
 
void Open7Zip ( XArchive *archive)
{
    jump_header = FALSE;
	gchar *command = g_strconcat ( "7za l " , archive->escaped_path, NULL );
	archive->dummy_size = 0;
    archive->nr_of_files = 0;
    archive->nr_of_dirs = 0;
	archive->parse_output = SevenZipOpen;
	SpawnAsyncProcess ( archive , command , 0, 0);
	g_free ( command );
	if ( archive->child_pid == 0 )
		return;

	char *names[]= {(_("Filename")),(_("Original")),(_("Compressed")),(_("Attr")),(_("Time")),(_("Date"))};
	GType types[]= {G_TYPE_STRING,G_TYPE_UINT64,G_TYPE_UINT64,G_TYPE_STRING,G_TYPE_STRING,G_TYPE_STRING};
	xa_create_liststore ( 6, names , (GType *)types );
}

gboolean SevenZipOpen (GIOChannel *ioc, GIOCondition cond, gpointer data)
{
	XArchive *archive = data;
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
		original = g_value_init(original, G_TYPE_UINT64);
		_original  = g_strndup ( start , end - start);
		g_value_set_uint64 (original , atoll (_original) );
		g_free (_original);

		start = eat_spaces (end);
		end = strchr (start, ' ');
		compressed = g_value_init(compressed, G_TYPE_UINT64);
		/* The following if else to fix archives compressed with ms=on (default on 7za) */
		if (end != NULL)
		{
			_compressed = g_strndup ( start , end - start);
			g_value_set_uint64 ( compressed , atoll (_compressed) );
			g_free (_compressed);

			start = eat_spaces (end);
		}
		else
		{
			unsigned long long int zero = 0;
			g_value_set_uint64 ( compressed , zero );
		}

		end = strchr (start, '\n');
		filename = g_value_init(filename, G_TYPE_STRING);
		g_value_set_string ( filename , g_strndup ( start , end - start) );
		
		archive->row = g_list_prepend(archive->row, filename);
		archive->row = g_list_prepend(archive->row, original);
		archive->row = g_list_prepend(archive->row, compressed);
		archive->row = g_list_prepend(archive->row, attr);
		archive->row = g_list_prepend(archive->row, time);
		archive->row = g_list_prepend(archive->row, date);
			
		if ( g_str_has_prefix(g_value_get_string (attr), "D") == FALSE)
			archive->nr_of_files++;
		else
			archive->nr_of_dirs++;
		archive->dummy_size += g_value_get_uint64 (original);
		g_free (line);
		if (archive->row_cnt > 99)
		{
			xa_append_rows ( archive , 6 );
			archive->row_cnt = 0;
		}
	}
	else if (cond & (G_IO_ERR | G_IO_HUP) )
	{
		g_io_channel_shutdown ( ioc,TRUE,NULL );
		g_io_channel_unref (ioc);
		xa_append_rows ( archive , 6 );
		return FALSE;
	}
	return TRUE;
}


