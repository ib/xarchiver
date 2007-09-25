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

static GSList *icon_cache = NULL;

const char *xa_get_stock_mime_icon(char *filename)
{
	const char *mime;
	const char *icon_name = "binary";

	mime = xdg_mime_get_mime_type_from_file_name(filename);
	//g_print ("%s\t%s\n",filename,mime);
	if (strstr(filename,".ogg") || strstr(filename,".flac") )
		icon_name = "sound";
	else if (strncmp(mime,"image/",6) == 0)
		icon_name = "image";
	else if (strcmp(mime,"text/html") == 0)
		icon_name = "html";
	else if (strncmp(mime,"text/",5) == 0)
		icon_name = "txt";
	else if (strcmp(mime,"application/rtf") == 0 || strcmp(mime,"application/pdf") == 0 || strcmp(mime,"application/msword") == 0
		|| strcmp (mime,"application/vnd.oasis.opendocument.text") == 0)
		icon_name = "document";
	else if (strcmp(mime,"audio/mpeg") == 0 || strcmp(mime,"audio/midi") == 0 )
		icon_name = "sound";
	else if (strcmp(mime,"application/vnd.ms-excel") == 0 || strcmp(mime,"application/vnd.oasis.opendocument.spreadsheet") == 0)
		icon_name = "gnome-mime-application-vnd.ms-excel";
	else if (strcmp(mime,"application/vnd.ms-powerpoint") == 0 || strcmp (mime,"application/vnd.oasis.opendocument.presentation") == 0)
		icon_name = "gnome-mime-application-vnd.ms-powerpoint";
	else if (strcmp(mime,"application/zip") == 0 || strcmp(mime,"application/x-rar") == 0 || strcmp(mime,"application/x-tar") == 0
		|| strcmp(mime,"application/x-7z-compressed") == 0 || strcmp(mime,"application/x-bzip-compressed-tar") == 0
		|| strcmp (mime,"application/x-compressed-tar") == 0 || strcmp (mime,"application/x-lha") == 0
		|| strcmp (mime,"application/x-rpm") == 0 || strcmp (mime,"application/x-deb") == 0 )
		icon_name = "package";
	else if (strcmp(mime,"application/x-shockwave-flash") == 0 || strcmp(mime,"video/mpeg") == 0 || strcmp(mime,"video/quicktime") == 0
		|| strcmp(mime,"video/x-msvideo") == 0 || strcmp(mime,"application/x-flash-video") == 0)
		icon_name = "video";
	else if (strcmp(mime,"application/x-cd-image") == 0)
		icon_name = "application-x-cd-image";
	else if (strcmp(mime,"application/x-php") == 0)
		icon_name = "gnome-mime-application-x-php";
	else if (strcmp(mime,"application/x-perl") == 0 || strcmp (mime,"application/x-csh") == 0 || strcmp (mime,"application/x-shellscript") == 0)
		icon_name = "gnome-mime-application-x-perl";
	else if (strcmp(mime,"application/x-font-ttf") == 0)
		icon_name = "gnome-mime-application-x-font-ttf";
	return icon_name;		
}

GdkPixbuf *xa_get_pixbuf_icon_from_cache(gchar *filename)
{
	pixbuf_cache *tie;
	const gchar *icon_name;
	GSList *found;
	GdkPixbuf *pixbuf;

	icon_name = xa_get_stock_mime_icon(filename);
	found = g_slist_find_custom(icon_cache,icon_name,(GCompareFunc)xa_icon_name_compare_func);

	if (found)
		return found->data;
	else
	{
		pixbuf = gtk_icon_theme_load_icon(icon_theme,"package",48,GTK_ICON_LOOKUP_FORCE_SVG,NULL);
		if (pixbuf)
		{
			tie = g_new0(pixbuf_cache,1);
			if (tie)
			{
				tie->icon_name = icon_name;
				tie->pixbuf = pixbuf;
				icon_cache = g_slist_prepend(icon_cache,tie);
			}
		}
	}
	return pixbuf;
}

gint xa_icon_name_compare_func(gconstpointer a, gconstpointer b)
{
	struct _pixbuf_cache *_a = (struct _pixbuf_cache *)a;
	struct _pixbuf_cache *_b = (struct _pixbuf_cache *)b;
	return strcmp(_a->icon_name, _b->icon_name);
}

void xa_free_pixbuf_cache()
{
	g_slist_foreach(icon_cache,(GFunc) g_free,NULL);
}
