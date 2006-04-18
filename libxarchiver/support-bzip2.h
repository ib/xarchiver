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

#ifndef __XARCHIVER_SUPPORT_BZIP2_H__
#define __XARCHIVER_SUPPORT_BZIP2_H__

G_BEGIN_DECLS

#define XA_TYPE_SUPPORT_BZIP2 xa_support_bzip2_get_type()

#define XA_SUPPORT_BZIP2(obj)         ( \
		G_TYPE_CHECK_INSTANCE_CAST ((obj),    \
			XA_TYPE_SUPPORT_BZIP2,      \
			XASupportBzip2))

#define IS_XA_SUPPORT_BZIP2(obj)      ( \
		G_TYPE_CHECK_INSTANCE_TYPE ((obj),    \
			XA_TYPE_SUPPORT_BZIP2))

#define XA_SUPPORT_BZIP2_CLASS(klass) ( \
		G_TYPE_CHECK_CLASS_CAST ((klass),     \
			XA_TYPE_SUPPORT_BZIP2,      \
			XASupportBzip2Class))

#define IS_XA_SUPPORT_BZIP2_CLASS(klass) ( \
		G_TYPE_CHECK_CLASS_TYPE ((klass),        \
			XA_TYPE_SUPPORT_BZIP2))

typedef struct _XASupportBzip2 XASupportBzip2;

struct _XASupportBzip2
{
	XASupport parent;
	gchar *out_filename;
};

typedef struct _XASupportBzip2Class XASupportBzip2Class;

struct _XASupportBzip2Class
{
	XASupportClass parent;
}; 

GType       xa_support_bzip2_get_type(void);
XASupport*  xa_support_bzip2_new();

G_END_DECLS

#endif /* __XARCHIVER_SUPPORT_BZIP2_H__*/
