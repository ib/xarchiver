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

#include "mime.h"

GSList *icon_cache = NULL;
extern GtkIconTheme *icon_theme;
const char *xa_get_stock_mime_icon(char *filename)
{
	const char *mime;
	const char *icon_name = "binary";

	mime = xdg_mime_get_mime_type_from_file_name(filename);
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
	else if (strcmp(mime,"audio/mpeg") == 0 || strcmp(mime,"audio/midi") == 0 || strcmp (mime,"audio/mp2") == 0)
		icon_name = "sound";
	else if (strcmp(mime,"application/vnd.ms-excel") == 0 || strcmp(mime,"application/vnd.oasis.opendocument.spreadsheet") == 0)
		icon_name = "gnome-mime-application-vnd.ms-excel";
	else if (strcmp(mime,"application/vnd.ms-powerpoint") == 0 || strcmp (mime,"application/vnd.oasis.opendocument.presentation") == 0)
		icon_name = "gnome-mime-application-vnd.ms-powerpoint";
	else if (strcmp(mime,"application/zip") == 0 || strcmp(mime,"application/x-rar") == 0 || strcmp(mime,"application/x-tar") == 0
		|| strcmp(mime,"application/x-7z-compressed") == 0 || strcmp(mime,"application/x-bzip-compressed-tar") == 0
		|| strcmp (mime,"application/x-compressed-tar") == 0 || strcmp (mime,"application/x-lha") == 0
		|| strcmp (mime,"application/x-rpm") == 0 || strcmp (mime,"application/x-deb") == 0
		|| strcmp (mime,"application/x-bzip") == 0  || strcmp (mime,"application/x-gzip") == 0)
		icon_name = "package";
	else if (strcmp(mime,"application/x-shockwave-flash") == 0 || strcmp(mime,"video/mpeg") == 0 || strcmp(mime,"video/quicktime") == 0
		|| strcmp(mime,"video/x-msvideo") == 0 || strcmp (mime,"video/mp4") == 0 || strcmp(mime,"application/x-flash-video") == 0
		|| strcmp(mime,"video/dv") == 0)
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

GdkPixbuf *xa_get_pixbuf_icon_from_cache(gchar *filename,gint size)
{
	pixbuf_cache *tie = NULL;
	const gchar *icon_name;
	GSList *found = NULL;
	GdkPixbuf *pixbuf = NULL;

	if (strcmp(filename,"folder") == 0)
		icon_name = filename;
	else if (strcmp(filename,"lock") == 0)
		icon_name = "gtk-dialog-authentication";
	else
		icon_name = xa_get_stock_mime_icon(filename);

	tie = g_new0(pixbuf_cache,1);
	if (tie)
	{
		tie->icon_name = g_strdup(icon_name);
		found = g_slist_find_custom(icon_cache,tie,(GCompareFunc)xa_icon_name_compare_func);
		if (found)
		{
			g_free (tie->icon_name);
			g_free (tie);
			return ((pixbuf_cache *)found->data)->pixbuf;
		}
		else
		{
			pixbuf = gtk_icon_theme_load_icon(icon_theme,icon_name,size,GTK_ICON_LOOKUP_FORCE_SVG,NULL);
			if (pixbuf)
			{
				tie->pixbuf = pixbuf;
				icon_cache = g_slist_prepend(icon_cache,tie);
			}
		}
	}
	return pixbuf;
}

gint xa_icon_name_compare_func(pixbuf_cache *a, pixbuf_cache *b)
{
	return strcmp(a->icon_name, b->icon_name);
}

void xa_free_icon_cache()
{
	GSList *x = icon_cache;

	while (x)
	{
		pixbuf_cache *tie = x->data;
		g_free (tie->icon_name);
		g_object_unref (tie->pixbuf);
		g_free(tie);
		x = x->next;
	}
	g_slist_free(icon_cache);
}
