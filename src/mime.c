/*
 *  Copyright (c) 2007 Giuseppe Torelli <colossus73@gmail.com>
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

#include "mime.h"

//
//gboolean gtk_icon_theme_has_icon

GdkPixbuf *xa_get_stock_mime_icon(gchar *filename)
{
	const char *mime;
	GdkPixbuf *pixbuf = NULL;
	const char *icon_name = "GTK_STOCK_FILE";

	mime = xdg_mime_get_mime_type_from_file_name(filename);
	g_print ("%s\n",mime);
	if (strncmp(mime,"image/",6) == 0)
		icon_name = "image";
	else if (strcmp(mime,"text/html") == 0)
		icon_name = "html";
	else if (strcmp(mime,"application/octet-stream") == 0)
		icon_name = "folder";
	
	pixbuf = gtk_icon_theme_load_icon (gtk_icon_theme_get_default(),icon_name, GTK_ICON_SIZE_SMALL_TOOLBAR, 0, NULL);
	return pixbuf;
}

