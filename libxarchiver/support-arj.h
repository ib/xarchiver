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

#ifndef __XARCHIVER_SUPPORT_ARJ_H__
#define __XARCHIVER_SUPPORT_ARJ_H__

G_BEGIN_DECLS

#define XA_TYPE_SUPPORT_ARJ xa_support_arj_get_type()

#define XA_SUPPORT_ARJ(obj)         ( \
		G_TYPE_CHECK_INSTANCE_CAST ((obj),    \
			XA_TYPE_SUPPORT_ARJ,      \
			XASupportArj))

#define IS_XA_SUPPORT_ARJ(obj)      ( \
		G_TYPE_CHECK_INSTANCE_TYPE ((obj),    \
		XA_TYPE_SUPPORT_ARJ))

#define XA_SUPPORT_ARJ_CLASS(klass) ( \
		G_TYPE_CHECK_CLASS_CAST ((klass),     \
			XA_TYPE_SUPPORT_ARJ,      \
			XASupportArjClass))

#define IS_XA_SUPPORT_ARJ_CLASS(klass) ( \
		G_TYPE_CHECK_CLASS_TYPE ((klass),        \
			XA_TYPE_SUPPORT_ARJ))

typedef struct _XASupportArj XASupportArj;

struct _XASupportArj
{
	XASupport parent;
};

typedef struct _XASupportArjClass XASupportArjClass;

struct _XASupportArjClass
{
	XASupportClass parent;
}; 

GType       xa_support_arj_get_type(void);
XASupport * xa_support_arj_new();

gboolean jump_header;
gboolean no_attr;
unsigned short int arj_line;

G_END_DECLS

#endif /* __XARCHIVER_SUPPORT_ARJ_H__*/


