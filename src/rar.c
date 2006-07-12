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
 
#include "rar.h"

void OpenRar ( XArchive *archive )
{
	jump_header = FALSE;
    gchar *command = g_strconcat ( "rar vl -c- " , archive->escaped_path, NULL );
	archive->dummy_size = 0;
    archive->nr_of_files = 0;
    archive->nr_of_dirs = 0;
	archive->parse_output = RarOpen;
	archive->format ="RAR";
	SpawnAsyncProcess ( archive , command , 0, 0);
	g_free ( command );
	if ( archive->child_pid == 0 )
		return;
	
	char *names[]	= {(_("Filename")),(_("Original")),(_("Compressed")),(_("Ratio")),(_("Date")),(_("Time")),(_("Permissions")),(_("CRC")),(_("Method")),(_("Version"))};
	GType types[]= {G_TYPE_STRING,G_TYPE_UINT64,G_TYPE_UINT64,G_TYPE_STRING,G_TYPE_STRING,G_TYPE_STRING,G_TYPE_STRING,G_TYPE_STRING,G_TYPE_STRING,G_TYPE_STRING};
    archive->has_passwd = FALSE;
	xa_create_liststore ( 10, names , (GType *)types );
}

gboolean RarOpen (GIOChannel *ioc, GIOCondition cond, gpointer data)
{
	XArchive *archive = data;
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

	if (cond & (G_IO_IN | G_IO_PRI) )
	{
        /* This to avoid inserting in the list RAR's copyright message */
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
        if ( jump_header && odd_line )
		{
			/* Now read the filename */
			g_io_channel_read_line ( ioc, &line, NULL, NULL, NULL );
		    if ( line == NULL )
				return TRUE;
			{
            /* This to avoid inserting in the liststore the last line of Rar output */
            if (strncmp (line, "--------", 8) == 0 || strncmp (line, "\x0a",1) == 0)
			{
                g_free (line);
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

			if ( strstr (g_value_get_string ( permissions) , "d") == NULL && strstr (g_value_get_string ( permissions) , "D") == NULL )
				archive->nr_of_files++;
			else
				archive->nr_of_dirs++;

			archive->dummy_size += g_value_get_uint64 (original);
			odd_line = ! odd_line;
			g_free (line);
			if (archive->row_cnt > 99)
			{
				xa_append_rows (archive , 10 );
				archive->row_cnt = 0;
			}
		}
	}
	else if (cond & (G_IO_ERR | G_IO_HUP | G_IO_NVAL) )
	{
		g_io_channel_shutdown ( ioc,TRUE,NULL );
		g_io_channel_unref (ioc);
		xa_append_rows (archive , 10 );
    	return FALSE;
	}
	return TRUE;
}

