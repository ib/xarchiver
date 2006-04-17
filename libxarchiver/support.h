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
#ifndef __XARCHIVER_SUPPORT_H__
#define __XARCHIVER_SUPPORT_H__

G_BEGIN_DECLS


#define XA_TYPE_SUPPORT xa_support_get_type()

#define XA_SUPPORT(obj)         ( \
		G_TYPE_CHECK_INSTANCE_CAST ((obj),    \
			XA_TYPE_SUPPORT,      \
			XASupport))

#define IS_XA_SUPPORT(obj)      ( \
		G_TYPE_CHECK_INSTANCE_TYPE ((obj),    \
			XA_TYPE_SUPPORT))

#define XA_SUPPORT_CLASS(klass) ( \
		G_TYPE_CHECK_CLASS_CAST ((klass),     \
			XA_TYPE_SUPPORT,      \
			XASupportClass))

#define IS_XA_SUPPORT_CLASS(klass) ( \
		G_TYPE_CHECK_CLASS_TYPE ((klass),        \
			XA_TYPE_SUPPORT))

typedef struct _XASupport XASupport;

struct _XASupport
{
	GObject parent;
	
	XAArchiveType type;
	gint     (*verify)       (XAArchive *);
	gint     (*add)          (XASupport *, XAArchive *, GSList *);
	gint     (*extract)      (XASupport *, XAArchive *, gchar *, GSList *, gboolean);
	gint     (*remove)       (XASupport *, XAArchive *, GSList *);
	gint     (*testing)      (XASupport *, XAArchive *);
	gint     (*open)         (XASupport *, XAArchive *);
	gint     (*view)         (XASupport *, XAArchive *, gchar *);
	gboolean (*parse_output) (GIOChannel *ioc, GIOCondition cond, gpointer data);

	struct
	{
		guint source;
		GIOChannel *out_ioc;
		XAArchive *archive;
		GPid child_pid;
		gchar *command;
		gboolean (*parse_output) (GIOChannel *ioc, GIOCondition cond, gpointer data);
		guint signal;
	} exec;

	gint n_columns;
	gchar **column_names;
	GType *column_types;
};

typedef struct _XASupportClass XASupportClass;

struct _XASupportClass
{
	GObjectClass parent;
}; 

GType        xa_support_get_type(void);
XASupport *  xa_support_new();
gpointer     xa_support_execute(gpointer data);
gint         xa_support_cancel(XASupport *support);
void         xa_support_watch_child (GPid pid, gint status, XASupport *support);
void         xa_support_get_columns(XASupport *support, gint *n_columns, gchar ***column_names, GType **column_types);
void         xa_support_set_columns(XASupport *support, gint n_columns, gchar **column_names, GType *column_types);

G_END_DECLS

#endif
