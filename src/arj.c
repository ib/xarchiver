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
 
#include "arj.h"

void OpenArj ( XArchive *archive )
{
    jump_header = FALSE;
	odd_line = FALSE;
	gchar *command = g_strconcat ( "arj v -he " , archive->escaped_path, NULL );
	archive->dummy_size = 0;
    archive->nr_of_files = 0;
    archive->nr_of_dirs = 0;
	archive->parse_output = ArjOpen;
	archive->format ="ARJ";
	SpawnAsyncProcess ( archive , command , 0, 0);
	g_free ( command );
	if ( archive->child_pid == 0 )
		return;
	    
	char *names[]= {(_("Filename")),(_("Original")),(_("Compressed")),(_("Ratio")),(_("Date")),(_("Time")),(_("Attributes")),(_("GUA")),(_("BPMGS"))};
	GType types[]= {G_TYPE_STRING,G_TYPE_UINT64,G_TYPE_UINT64,G_TYPE_STRING,G_TYPE_STRING,G_TYPE_STRING,G_TYPE_STRING,G_TYPE_STRING,G_TYPE_STRING};
	xa_create_liststore ( 9, names , (GType *)types );
}

gboolean ArjOpen (GIOChannel *ioc, GIOCondition cond, gpointer data)
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
	GValue *attr = NULL;
	GValue *gua = NULL;
	GValue *bpmgs = NULL;	
	
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
			archive->dummy_size += g_value_get_uint64 (original );
			g_free (line);
			if (archive->row_cnt > 99)
			{
				xa_append_rows ( archive , 9 );
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
		xa_append_rows ( archive , 9 );
		return FALSE;
	}
	arj_line++;
	return TRUE;
}

