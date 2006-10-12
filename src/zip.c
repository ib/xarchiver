/*
 *  Copyright (C) 2006 Giuseppe Torelli - <colossus73@gmail.com>
 *  Copyright (C) 2006 Benedikt Meurer - <benny@xfce.org>
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
#include "zip.h"
#include "string_utils.h"

static gboolean ZipOpen (GIOChannel *ioc, GIOCondition cond, gpointer data);
void OpenZip ( XArchive *archive )
{
	gchar *command = g_strconcat ( "unzip -vl -qq " , archive->escaped_path, NULL );
	archive->dummy_size = 0;
    archive->nr_of_files = 0;
    archive->nr_of_dirs = 0;
	archive->parse_output = ZipOpen;
	archive->format ="ZIP";
	SpawnAsyncProcess ( archive , command , 0, 0);
	g_free ( command );
	if ( archive->child_pid == 0 )
		return;

	char *names[]= {(_("Filename")),(_("Original")),(_("Method")),(_("Compressed")),(_("Ratio")),(_("Date")),(_("Time")),(_("Checksum"))};
	GType types[]= {G_TYPE_STRING,G_TYPE_UINT64,G_TYPE_STRING,G_TYPE_UINT64,G_TYPE_STRING,G_TYPE_STRING,G_TYPE_STRING,G_TYPE_STRING};
	xa_create_liststore ( 8, names , (GType *)types );
}

static gboolean ZipOpen (GIOChannel *ioc, GIOCondition cond, gpointer data)
{
	XArchive *archive = data;
	gchar **fields = NULL;
	gchar *filename = NULL;
	gchar *line = NULL;
	GtkTreeIter iter;
	GIOStatus status = G_IO_STATUS_NORMAL;

	if (cond & (G_IO_IN | G_IO_PRI) )
	{
		do
		{
			status = g_io_channel_read_line ( ioc, &line, NULL, NULL, NULL );
			if ( line == NULL )
				break;
			fields = split_line (line , 7);
			filename = get_last_field (line , 8);
			if ( g_str_has_suffix(filename , "/") == TRUE)
				archive->nr_of_dirs++;
			else
				archive->nr_of_files++;
			if ( filename != NULL )
			{
				gtk_list_store_append (liststore, &iter);
				for ( x = 0; x < 7; x++)
				{
					if (x == 0 || x == 2)
						gtk_list_store_set (liststore, &iter,x+1, atoll (fields[x]), -1);
					else
						gtk_list_store_set (liststore, &iter,x+1,fields[x], -1);
				}
				archive->dummy_size += atoll (fields[0]);
				gtk_list_store_set (liststore, &iter,0,filename,-1);
			}

			while (gtk_events_pending() )
				gtk_main_iteration();
			g_strfreev ( fields );
			g_free (line);
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
