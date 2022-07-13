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
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 *  MA 02110-1301 USA.
 */

#include <string.h>
#include "mime.h"
#include "pref_dialog.h"
#include "xdgmime/xdgmime.h"

typedef struct
{
	gchar *icon_name;
	GdkPixbuf *pixbuf;
} pixbuf_cache;

static GSList *icon_cache;

static gint xa_icon_name_compare_func (const pixbuf_cache *a, const pixbuf_cache *b)
{
	return strcmp(a->icon_name, b->icon_name);
}

const char *xa_get_stock_mime_icon(const char *filename)
{
	const char *mime;
	static char icon_name[80];
	char *p;

	mime = xdg_mime_get_mime_type_from_file_name(filename);

	strncpy(icon_name,mime,sizeof(icon_name));
	icon_name[sizeof(icon_name)-1] = 0;

	p = strchr(icon_name,'/');
	if (p) *p = '-';

	if (strcmp(mime,"text/html") == 0)
		return icon_name;
	else if (strncmp(mime,"text/",5) == 0)
		return "text-x-generic";
	else if (strncmp(mime,"audio/",6) == 0)
		return "audio-x-generic";
	else if (strncmp(mime,"image/",6) == 0)
		return "image-x-generic";
	else if (strncmp(mime,"video/",6) == 0 ||
	         strcmp(mime,"application/vnd.adobe.flash.movie") == 0)
		return "video-x-generic";
	else if (strcmp(mime,"application/msword") == 0 ||
	         strcmp(mime,"application/pdf") == 0 ||
	         strcmp(mime,"application/rtf") == 0 ||
	         strcmp(mime,"application/vnd.oasis.opendocument.text") == 0)
		return "x-office-document";
	else if (strcmp(mime,"application/vnd.ms-excel") == 0 ||
	         strcmp(mime,"application/vnd.oasis.opendocument.spreadsheet") == 0)
		return "x-office-spreadsheet";
	else if (strcmp(mime,"application/vnd.ms-powerpoint") == 0 ||
	         strcmp(mime,"application/vnd.oasis.opendocument.presentation") == 0)
		return "x-office-presentation";
	else if (strcmp(mime,"application/gzip") == 0 ||
	         strcmp(mime,"application/vnd.android.package-archive") == 0 ||
	         strcmp(mime,"application/vnd.debian.binary-package") == 0 ||
	         strcmp(mime,"application/vnd.ms-cab-compressed") == 0 ||
	         strcmp(mime,"application/vnd.openofficeorg.extension") == 0 ||
	         strcmp(mime,"application/vnd.rar") == 0 || /* legacy */ strcmp(mime,"application/x-rar") == 0 ||
	         strcmp(mime,"application/x-7z-compressed") == 0 ||
	         strcmp(mime,"application/x-archive") == 0 ||
	         strcmp(mime,"application/x-arj") == 0 ||
	         strcmp(mime,"application/x-bzip") == 0 ||
	         strcmp(mime,"application/x-bzip-compressed-tar") == 0 ||
	         strcmp(mime,"application/x-compress") == 0 ||
	         strcmp(mime,"application/x-compressed-tar") == 0 ||
	         strcmp(mime,"application/x-cpio") == 0 ||
	         strcmp(mime,"application/x-cpio-compressed") == 0 ||
	         strcmp(mime,"application/x-java-archive") == 0 ||
	         strcmp(mime,"application/x-lha") == 0 ||
	         strcmp(mime,"application/x-lrzip") == 0 ||
	         strcmp(mime,"application/x-lrzip-compressed-tar") == 0 ||
	         strcmp(mime,"application/x-lz4") == 0 ||
	         strcmp(mime,"application/x-lz4-compressed-tar") == 0 ||
	         strcmp(mime,"application/x-lzip") == 0 ||
	         strcmp(mime,"application/x-lzip-compressed-tar") == 0 ||
	         strcmp(mime,"application/x-lzma") == 0 ||
	         strcmp(mime,"application/x-lzma-compressed-tar") == 0 ||
	         strcmp(mime,"application/x-lzop") == 0 ||
	         strcmp(mime,"application/x-rpm") == 0 ||
	         strcmp(mime,"application/x-source-rpm") == 0 ||
	         strcmp(mime,"application/x-tar") == 0 ||
	         strcmp(mime,"application/x-tarz") == 0 ||
	         strcmp(mime,"application/x-tzo") == 0 ||
	         strcmp(mime,"application/x-xpinstall") == 0 ||
	         strcmp(mime,"application/x-xz") == 0 ||
	         strcmp(mime,"application/x-xz-compressed-tar") == 0 ||
	         strcmp(mime,"application/x-zstd-compressed-tar") == 0 ||
	         strcmp(mime,"application/zip") == 0 ||
	         strcmp(mime,"application/zstd") == 0)
		return "package-x-generic";
	else if (strcmp(mime,"application/x-cd-image") == 0)
		return "media-optical";
	else if (strcmp(mime,"application/x-csh") == 0 ||
	         strcmp(mime,"application/x-perl") == 0 ||
	         strcmp(mime,"application/x-php") == 0 ||
	         strcmp(mime,"application/x-shellscript") == 0)
		return "text-x-script";
	else if (strncmp(mime,"font/",5) == 0 ||
	         strncmp(mime,"application/x-font",18) == 0)
		return "font-x-generic";

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
		icon_name = "dialog-password";
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
			pixbuf = gtk_icon_theme_load_icon(icon_theme, icon_name, size, (GtkIconLookupFlags) 0, NULL);
			if (pixbuf)
			{
				tie->pixbuf = pixbuf;
				icon_cache = g_slist_prepend(icon_cache,tie);
			}
		}
	}
	return pixbuf;
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
