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
#include "rar.h"

extern gboolean unrar;
static gboolean RarOpen (GIOChannel *ioc, GIOCondition cond, gpointer data);
GtkTreeIter iter;

void OpenRar ( XArchive *archive )
{
	jump_header = FALSE;
	gchar *command = NULL;
	gchar *rar = NULL;

	if (unrar)
	{
		rar = "unrar";
		archive->can_add = archive->has_sfx = FALSE;
	}
	else
	{
		rar = "rar";
		archive->can_add = archive->has_sfx = TRUE;
	}

	command = g_strconcat ( rar," vl -c- " , archive->escaped_path, NULL );
	archive->can_extract = archive->has_test = archive->has_properties = TRUE;
	archive->dummy_size = 0;
    archive->nr_of_files = 0;
    archive->nr_of_dirs = 0;
	archive->parse_output = RarOpen;
	archive->format ="RAR";
	SpawnAsyncProcess ( archive , command , 0, 0);
	g_free ( command );
	if ( archive->child_pid == 0 )
		return;

	char *names[]= {(_("Filename")),(_("Original")),(_("Compressed")),(_("Ratio")),(_("Date")),(_("Time")),(_("Permissions")),(_("Checksum")),(_("Method")),(_("Version"))};
	GType types[]= {G_TYPE_STRING,G_TYPE_UINT64,G_TYPE_UINT64,G_TYPE_STRING,G_TYPE_STRING,G_TYPE_STRING,G_TYPE_STRING,G_TYPE_STRING,G_TYPE_STRING,G_TYPE_STRING};
    archive->has_passwd = FALSE;
	xa_create_liststore ( 10, names , (GType *)types, archive );
}

static gboolean RarOpen (GIOChannel *ioc, GIOCondition cond, gpointer data)
{
	XArchive *archive = data;
	gchar **fields = NULL;
	gchar *line = NULL;
	GIOStatus status = G_IO_STATUS_NORMAL;

	if (cond & (G_IO_IN | G_IO_PRI) )
	{
		do
		{
			/* This to avoid inserting in the list RAR's copyright message */
			if (jump_header == FALSE )
			{
				status = g_io_channel_read_line ( ioc, &line, NULL, NULL, NULL );
				if (line == NULL)
					break;
				if  (strncmp (line , "--------" , 8) == 0)
				{
					jump_header = TRUE;
					odd_line = TRUE;
				}
				g_free (line);
				break;
			}
			if ( jump_header && odd_line )
			{
				/* Now read the filename */
				status = g_io_channel_read_line ( ioc, &line, NULL, NULL, NULL );
				if ( line == NULL )
					break;
				/* This to avoid inserting in the liststore the last line of Rar output */
				if (strncmp (line, "--------", 8) == 0 || strncmp (line, "\x0a",1) == 0)
				{
					g_free (line);
					status = g_io_channel_read_line ( ioc, &line, NULL, NULL, NULL );
					g_free (line);
					break;
				}
				gtk_list_store_append (archive->liststore, &iter);
				line[ strlen(line) - 1 ] = '\000';
				if (line[0] == '*')
					archive->has_passwd = TRUE;
				/* This to avoid the white space or the * before the first char of the filename */
				line++;
				gtk_list_store_set (archive->liststore, &iter,0,line,-1);
				/* Restore the pointer before freeing it */
				line--;
				g_free (line);
				odd_line = ! odd_line;
				break;
			}
			else
			{
				/* Now read the rest of the data */
				status = g_io_channel_read_line ( ioc, &line, NULL, NULL, NULL );
				if ( line == NULL)
					break;
				fields = split_line (line,9);
				if (fields[5] == NULL)
					break;
				if ( strstr (fields[5] , "d") == NULL && strstr (fields[5] , "D") == NULL )
					archive->nr_of_files++;
				else
					archive->nr_of_dirs++;
				for (x = 0; x < 9; x++)
				{
					if (x == 0 || x == 1)
						gtk_list_store_set (archive->liststore, &iter,x+1,strtoll(fields[x],NULL,0),-1);
					else
						gtk_list_store_set (archive->liststore, &iter,x+1,fields[x],-1);
				}
				while ( gtk_events_pending() )
					gtk_main_iteration();
				archive->dummy_size += strtoll(fields[0],NULL,0);
				g_strfreev ( fields );
				g_free (line);
				odd_line = ! odd_line;
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
		gtk_tree_view_set_model (GTK_TREE_VIEW(archive->treeview), archive->model);
		g_object_unref (archive->model);
		return FALSE;
	}
	return TRUE;
}
