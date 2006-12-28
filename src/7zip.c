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
#include "7zip.h"

static gboolean SevenZipOpen (GIOChannel *ioc, GIOCondition cond, gpointer data);
void Open7Zip ( XArchive *archive)
{
    jump_header = FALSE;
	gchar *command = g_strconcat ( "7za l " , archive->escaped_path, NULL );
	archive->has_sfx = archive->has_properties = archive->can_add = archive->can_extract = archive->has_test = TRUE;
	archive->dummy_size = 0;
    archive->nr_of_files = 0;
    archive->nr_of_dirs = 0;
	archive->format ="7-ZIP";
	archive->parse_output = SevenZipOpen;
	xa_spawn_async_process (archive,command,0);
	g_free ( command );
	if ( archive->child_pid == 0 )
		return;

	char *names[]= {(_("Filename")),(_("Original")),(_("Compressed")),(_("Attr")),(_("Time")),(_("Date"))};
	GType types[]= {G_TYPE_STRING,G_TYPE_UINT64,G_TYPE_UINT64,G_TYPE_STRING,G_TYPE_STRING,G_TYPE_STRING};
	xa_create_liststore ( 6, names , (GType *)types, archive );
}

static gboolean SevenZipOpen (GIOChannel *ioc, GIOCondition cond, gpointer data)
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
			/* This to avoid inserting in the liststore 7zip's message */
			if (jump_header == FALSE )
			{
				for ( x = 0; x <= 7; x++)
				{
					status = g_io_channel_read_line ( ioc, &line, NULL, NULL, NULL );
					g_free (line);
				}
				jump_header = TRUE;
			}
			status = g_io_channel_read_line ( ioc, &line, NULL, NULL, NULL );
			if ( line == NULL )
				break;

			/* This to avoid inserting the last line of output */
			if (strncmp (line, "-----------------", 17) == 0 || strncmp (line, "\x0a",1) == 0)
			{
				g_free (line);
				status = g_io_channel_read_line ( ioc, &line, NULL, NULL, NULL );
				g_free (line);
				break;
			}

			fields = split_line ( line , 5 );
			filename = get_last_field ( line , 6);
			gtk_list_store_append (archive->liststore, &iter);
			if ( g_str_has_prefix(fields[2] , "D") == FALSE)
				archive->nr_of_files++;
			else
				archive->nr_of_dirs++;
			for ( x = 0; x < 5; x++)
			{
				if (x == 3)
					gtk_list_store_set (archive->liststore, &iter,1,strtoll(fields[3],NULL,0),-1);
				else if (x == 4)
					gtk_list_store_set (archive->liststore, &iter,2,strtoll(fields[4],NULL,0),-1);
				else
					gtk_list_store_set (archive->liststore, &iter,(5-x),fields[x],-1);
			}
			archive->dummy_size += strtoll(fields[3],NULL,0);
			if ( filename == NULL )
				gtk_list_store_set (archive->liststore, &iter,0,fields[4],-1);
			else
				gtk_list_store_set (archive->liststore, &iter,0,filename,-1);
			g_strfreev ( fields );

			while (gtk_events_pending() )
				gtk_main_iteration();
			g_free (line);
		}
		while (status == G_IO_STATUS_NORMAL);
		if (status == G_IO_STATUS_ERROR || status == G_IO_STATUS_EOF)
			goto done;
	}
	else if (cond & (G_IO_ERR | G_IO_HUP) )
	{
done:	g_io_channel_shutdown ( ioc,TRUE,NULL );
		g_io_channel_unref (ioc);
		gtk_tree_view_set_model (GTK_TREE_VIEW(archive->treeview), archive->model);
		g_object_unref (archive->model);
		return FALSE;
	}
	return TRUE;
}

