/*
 *  Copyright (c) 2008 Giuseppe Torelli <colossus73@gmail.com>
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
 *
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <gtk/gtk.h>
#include "support.h"

GtkWidget *xa_main_window_find_image(gchar *filename, GtkIconSize size)
{
    GtkWidget *file_image;
	gchar *path;
	GdkPixbuf *file_pixbuf = NULL;
	path = g_strconcat(PIXMAPSDIR, "/", filename, NULL);
	file_pixbuf = gdk_pixbuf_new_from_file (path, NULL);
	g_free (path);

	if (file_pixbuf == NULL)
		file_image = gtk_image_new_from_stock(GTK_STOCK_MISSING_IMAGE, size);
    else
	{
		file_image = gtk_image_new_from_pixbuf (file_pixbuf);
	    g_object_unref (file_pixbuf);
	}

    return file_image;
}

