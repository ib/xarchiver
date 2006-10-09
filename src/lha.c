/*
 *  Copyright (C) 2006 Giuseppe Torelli - <colossus73@gmail.com>
 *  Copyright (C) 2006 Lukasz 'Sil2100' Zemczak - <sil2100@vexillium.org>
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
#include "lha.h"

static gboolean LhaOpen (GIOChannel *ioc, GIOCondition cond, gpointer data);
void OpenLha ( XArchive *archive )
{
	gchar *command;

	command = g_strconcat ("lha l " , archive->escaped_path, NULL);
	archive->dummy_size = 0;
	archive->nr_of_files = 0;
	archive->nr_of_dirs = 0;
	archive->format ="LHA";
	archive->parse_output = LhaOpen;
	SpawnAsyncProcess ( archive , command , 0, 0);
	g_free (command);

	if (archive->child_pid == 0)
		return;

	char *names[]= {(_("Filename")),(_("Permissions")),(_("UID/GID")),(_("Size")),(_("Ratio")),(_("Timestamp"))};
	GType types[]= {G_TYPE_STRING,G_TYPE_STRING,G_TYPE_STRING,G_TYPE_UINT64,G_TYPE_STRING,G_TYPE_STRING};
	xa_create_liststore(6, names, (GType *)types);
}

static gboolean LhaOpen (GIOChannel *ioc, GIOCondition cond, gpointer data)
{
	XArchive *archive = data;
	GtkTreeIter iter;
	gchar *permissions = NULL;
	gchar *owner = NULL;
	gchar *ratio = NULL;
	gchar *timestamp = NULL;
	gchar *size = NULL;
	gchar *line = NULL;
	gchar *filename = NULL;
	GIOStatus status = G_IO_STATUS_NORMAL;
	unsigned short int a = 0, n = 0, num;

	if (cond & (G_IO_IN | G_IO_PRI) )
	{
		// We don't need the first two lines. No actual data there.
		g_io_channel_read_line(ioc, &line, NULL, NULL, NULL);
		if (line != NULL)
			g_free (line);

		g_io_channel_read_line(ioc, &line, NULL, NULL, NULL);
		if (line != NULL)
			g_free (line);
		do
		{
			status = g_io_channel_read_line(ioc, &line, NULL, NULL, NULL);
			if (line == NULL || (strncmp(line, "---------- -", 12) == 0))
				break;
			gtk_list_store_append (liststore, &iter);

			permissions = g_strndup(line, 10);
			gtk_list_store_set (liststore, &iter,1,permissions,-1);
			if (strstr(permissions, "d") == NULL)
				archive->nr_of_files++;
			else
				archive->nr_of_dirs++;
			g_free (permissions);

			owner = g_strndup(&line[11], 11);
			gtk_list_store_set (liststore, &iter,2,owner,-1);
			g_free (owner);

			num = strlen(line);
			for(n = 23;n < num;n++)
			if(line[n] != ' ')
				break;

			a = n;
			for(;n < num;n++)
			if(line[n] == ' ')
				break;

			size = g_strndup(&line[a], n - a);
			gtk_list_store_set (liststore, &iter,3,atoll(size),-1);
			archive->dummy_size += atoll(size);
			g_free(size);

			ratio = g_strndup(&line[31], 7);
			gtk_list_store_set (liststore, &iter,4,ratio,-1);
			g_free (ratio);

			timestamp = g_strndup(&line[38], 13);
			gtk_list_store_set (liststore, &iter,5,timestamp,-1);
			g_free (timestamp);

			filename = g_strndup(&line[51], num - 51 - 1);
			gtk_list_store_set (liststore, &iter,0,filename,-1);
			g_free (filename);

			g_free(line);
		}
		while (status == G_IO_STATUS_NORMAL);

		if (status == G_IO_STATUS_ERROR || status == G_IO_STATUS_EOF)
		goto done;
	}
	else if (cond & (G_IO_ERR | G_IO_HUP | G_IO_NVAL) )
	{
done:
		g_io_channel_shutdown(ioc, TRUE, NULL);
		g_io_channel_unref(ioc);
		gtk_tree_view_set_model (GTK_TREE_VIEW(treeview1), model);
		g_object_unref (model);
		return FALSE;
	}
	return TRUE;
}
