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

#ifndef __XARCHIVER_MIME_H__
#define __XARCHIVER_MIME_H__

#include "xdgmime/xdgmime.h"
#include "support.h"
#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <string.h>

typedef struct _pixbuf_cache pixbuf_cache;

struct _pixbuf_cache
{
	gchar *icon_name;
	GdkPixbuf *pixbuf;
};

const char *xa_get_stock_mime_icon(char *);
GdkPixbuf *xa_get_pixbuf_icon_from_cache(gchar *,gint);
gint xa_icon_name_compare_func(pixbuf_cache *, pixbuf_cache *);
void xa_free_icon_cache();
#endif
