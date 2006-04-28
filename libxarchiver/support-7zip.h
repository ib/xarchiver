/*
 *  Copyright (c) 2006 Stephan Arts <psyBSD@gmail.com>
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

#ifndef __XARCHIVER_SUPPORT_7ZIP_H__
#define __XARCHIVER_SUPPORT_7ZIP_H__

G_BEGIN_DECLS

#define XA_TYPE_SUPPORT_7ZIP xa_support_7zip_get_type()

#define XA_SUPPORT_7ZIP(obj)         ( \
		G_TYPE_CHECK_INSTANCE_CAST ((obj),    \
			XA_TYPE_SUPPORT_7ZIP,      \
			XASupport7zip))

#define IS_XA_SUPPORT_7ZIP(obj)      ( \
		G_TYPE_CHECK_INSTANCE_TYPE ((obj),    \
		XA_TYPE_SUPPORT_7ZIP))

#define XA_SUPPORT_7ZIP_CLASS(klass) ( \
		G_TYPE_CHECK_CLASS_CAST ((klass),     \
			XA_TYPE_SUPPORT_7ZIP,      \
			XASupport7zipClass))

#define IS_XA_SUPPORT_7ZIP_CLASS(klass) ( \
		G_TYPE_CHECK_CLASS_TYPE ((klass),        \
			XA_TYPE_SUPPORT_7ZIP))

typedef struct _XASupport7zip XASupport7zip;

struct _XASupport7zip
{
	XASupport parent;
};

typedef struct _XASupport7zipClass XASupport7zipClass;

struct _XASupport7zipClass
{
	XASupportClass parent;
}; 

GType       xa_support_7zip_get_type(void);
XASupport * xa_support_7zip_new();

gboolean jump_header;

G_END_DECLS

#endif /* __XARCHIVER_SUPPORT_7ZIP_H__*/


