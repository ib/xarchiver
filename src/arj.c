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

#include "config.h"
#include "arj.h"
#include "string_utils.h"

GtkTreeIter iter;
gboolean jump_header = FALSE;
unsigned short int arj_line = 0;

static gboolean ArjOpen (GIOChannel *ioc, GIOCondition cond, gpointer data);

void OpenArj ( XArchive *archive )
{
    jump_header = FALSE;
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

static gboolean ArjOpen (GIOChannel *ioc, GIOCondition cond, gpointer data)
{
	XArchive *archive = data;
	gchar **fields = NULL;
	gchar *line = NULL;
    gchar *filename = NULL;
	GIOStatus status = G_IO_STATUS_NORMAL;

	if (cond & (G_IO_IN | G_IO_PRI) )
	{
		do
		{
			/* This to avoid inserting in the liststore arj copyright message */
			if (jump_header == FALSE )
			{
				status = g_io_channel_read_line ( ioc, &line, NULL, NULL, NULL );
				if (line == NULL)
					break;
				if (strncmp (line , "----------" , 10) == 0)
				{
					jump_header = TRUE;
					arj_line = 1;
				}
				g_free (line);
				break;
			}
			if (arj_line == 4)
			{
				arj_line = 1;
				break;
			}
			else if (arj_line == 1)
			{
				/* This to avoid reading the last line of arj output */
				status = g_io_channel_read_line ( ioc, &line, NULL, NULL, NULL );
				if (line == NULL)
					break;
				if (strncmp (line, "----------", 10) == 0 || strncmp (line, "\x0a",1) == 0)
				{
					g_free (line);
					status = g_io_channel_read_line ( ioc, &line, NULL, NULL, NULL );
					if (line != NULL)
						g_free (line);
					break;
				}
				filename = get_last_field ( line , 2 );
				gtk_list_store_append (liststore, &iter);
				gtk_list_store_set (liststore, &iter,0,filename,-1);
				g_free (line);
				break;
			}
			else if (arj_line == 2)
			{
				status = g_io_channel_read_line ( ioc, &line, NULL, NULL, NULL );
				if ( line == NULL)
					break;
				fields = split_line ( line , 10 );
				if ( g_str_has_prefix(fields[7] , "d") == FALSE)
					archive->nr_of_files++;

				for ( x = 2; x < 10; x++)
				{
					if ( x == 2 || x == 3)
						gtk_list_store_set (liststore, &iter,x-1,atoll(fields[x]),-1);
					else
						gtk_list_store_set (liststore, &iter,x-1,fields[x],-1);
				}
				archive->dummy_size += atoll(fields[2]);
				g_free (line);
				status = g_io_channel_read_line ( ioc, &line, NULL, NULL, NULL );
				g_free (line);
				status = g_io_channel_read_line ( ioc, &line, NULL, NULL, NULL );
				g_free (line);
				g_strfreev ( fields );
			}
			else if (arj_line == 3)
			{
				status = g_io_channel_read_line ( ioc, &line, NULL, NULL, NULL );
				if (line != NULL)
					g_free (line);
				status = g_io_channel_read_line ( ioc, &line, NULL, NULL, NULL );
				if (line != NULL)
				{
					arj_line++;
					g_free (line);
				}
			}
		}
		while (status == G_IO_STATUS_NORMAL);
		if (status == G_IO_STATUS_ERROR || status == G_IO_STATUS_EOF)
			goto done;
	}
	else if (cond & (G_IO_ERR | G_IO_HUP | G_IO_NVAL) )
	{
done:	g_io_channel_shutdown ( ioc,TRUE,NULL );
		g_io_channel_unref (ioc);
		gtk_tree_view_set_model (GTK_TREE_VIEW(treeview1), model);
		g_object_unref (model);
		return FALSE;
	}
	return TRUE;
}

