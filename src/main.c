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
	GSList *files = data;
	XArchive *archive = NULL;
	
	while(files != NULL)
	{
		archive = xarchiver_archive_new((gchar *)files->data, XARCHIVETYPE_UNKNOWN);
		if(archive == NULL)
			g_warning("Archive %s is not supported\n", files->data);
		else
		{
			xa_main_window_add_tab(XA_MAIN_WINDOW(main_window), archive, g_path_get_basename(archive->path));
		}

		files = files->next;
	}
}

int main(int argc, char **argv)
{
	xarchiver_init();
	gtk_init(&argc, &argv);

	main_window = xa_main_window_new();

	g_signal_connect(G_OBJECT(main_window), "destroy", gtk_main_quit, NULL);
	g_signal_connect(G_OBJECT(main_window), "xa_open_archive", G_CALLBACK(open_archive), NULL);

	gtk_widget_show_all(main_window);

	gtk_main();
	
	xarchiver_destroy();
	return 0;
}
