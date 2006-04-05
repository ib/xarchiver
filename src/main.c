/*
 *  Copyright (c) 2006 Stephan Arts <stephan.arts@hva.nl>
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

#include <glib.h>
#include <libxarchiver/libxarchiver.h>
#include <gtk/gtk.h>

#include "main.h"
#include "main-window.h"


GtkWidget *main_window;

void
open_archive(GtkWidget *widget, gpointer data)
{
	gchar *filename = data;
	XArchive *archive = NULL;
	XArchiveSupport *support = NULL;
	
	
		archive = xarchiver_archive_new(filename, XARCHIVETYPE_UNKNOWN);
		if(archive == NULL)
			//TODO: notify the user with a gtk_dialog error message
			g_warning("Archive %s is not supported\n", filename);
		else
		{
			if((archive->type == XARCHIVETYPE_BZIP2) || (archive->type == XARCHIVETYPE_GZIP))
			{
				support = xarchiver_find_archive_support(archive);
				support->extract(archive, "/tmp/", NULL, FALSE);
			}
		}
		switch (archive->type)
		{
			case XARCHIVETYPE_RAR:
				xarchive_rar_support_open (archive);
			break;
		}
		while (archive->child_pid != 0)
		{
			while (gtk_events_pending())
				gtk_main_iteration();
		}
	//This only to print the content of GList filled in xarchiver_parse_rar_output
	archive->row = g_list_reverse ( archive->row );
	while (archive->row)
	{
		if (archive->row->data == "--")
			g_print ("\n");
		else
			g_print ("%s\t",archive->row->data);
		archive->row = archive->row->next;	
	}
	g_print ("Files:%d\nDirs:%d\nArchive Size:%lld\n",archive->number_of_files,archive->number_of_dirs,archive->dummy_size);
	
}

int main(int argc, char **argv)
{
	gchar *columns[] = {"Filename", "Permissions", "Date", "Time"};
	GType column_types[] = {G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING};
	GSList *fields = g_slist_alloc();
	fields->data = "/xarchiver-test";
	g_slist_append(fields, "drwxrwxr-x");
	g_slist_append(fields, "01-01-1970");
	g_slist_append(fields, "00:00:00");

	xarchiver_init();
	gtk_init(&argc, &argv);

	main_window = xa_main_window_new();

	g_signal_connect(G_OBJECT(main_window), "destroy", gtk_main_quit, NULL);
	g_signal_connect(G_OBJECT(main_window), "xa_open_archive", G_CALLBACK(open_archive), NULL);

	gtk_widget_show_all(main_window);

	xa_main_window_set_widget_sensitive(XA_MAIN_WINDOW(main_window), "xa-button-test", FALSE);
	xa_main_window_set_list_interface(XA_MAIN_WINDOW(main_window), 4, columns, column_types);
	xa_main_window_append_list(XA_MAIN_WINDOW(main_window), fields);

	gtk_main();
	
	xarchiver_destroy();
	return 0;
}
