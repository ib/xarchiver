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
 * *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */
#ifndef __XARCHIVER_ARCHIVE_H__
#define __XARCHIVER_ARCHIVE_H__

G_BEGIN_DECLS

#define XA_ARCHIVE(obj)         ( \
		G_TYPE_CHECK_INSTANCE_CAST ((obj),    \
			xa_archive_get_type(),      \
			XAArchive))

#define IS_XA_ARCHIVE(obj)      ( \
		G_TYPE_CHECK_INSTANCE_TYPE ((obj),    \
			xa_archive_get_type()))

#define XA_ARCHIVE_CLASS(class) ( \
		G_TYPE_CHECK_CLASS_CAST ((class),     \
			xa_archive_get_type(),      \
			XAArchiveClass))

#define IS_XA_ARCHIVE_CLASS(class) ( \
		G_TYPE_CHECK_CLASS_TYPE ((class),        \
			xa_archive_get_type()))

typedef enum
{
	XARCHIVETYPE_UNKNOWN,
	XARCHIVETYPE_BZIP2,
	XARCHIVETYPE_GZIP,
	XARCHIVETYPE_RAR,
	XARCHIVETYPE_ZIP,
	XARCHIVETYPE_ARJ,
	XARCHIVETYPE_TAR,
	XARCHIVETYPE_RPM,
	XARCHIVETYPE_7ZIP,
	XARCHIVETYPE_ISO
} XAArchiveType;

typedef enum
{
	XA_ARCHIVESTATUS_IDLE,
	XA_ARCHIVESTATUS_EXTRACT,
	XA_ARCHIVESTATUS_ADD,
	XA_ARCHIVESTATUS_REMOVE,
	XA_ARCHIVESTATUS_OPEN,
	XA_ARCHIVESTATUS_RELOAD,
	XA_ARCHIVESTATUS_ERROR,
	XA_ARCHIVESTATUS_USER_BREAK
} XAArchiveStatus;

typedef struct _XAArchive XAArchive;


struct _XAArchive
{
	GObject parent;

	XAArchiveType type;
	XAArchiveStatus status;
	gchar *path;
	gchar *passwd;
	gint nr_of_files;
	gint nr_of_dirs;
	unsigned long long int dummy_size;
	unsigned int row_cnt;
	GSList *error_output;
	GList *row;
	gboolean has_passwd;
};

typedef struct _XAArchiveClass XAArchiveClass;

struct _XAArchiveClass
{
	GObjectClass parent;
}; 

GType xa_archive_get_type(void);
XAArchive *xa_archive_new(gchar *, XAArchiveType);

gboolean xa_catch_errors (GIOChannel *ioc, GIOCondition cond, gpointer data);

G_END_DECLS

#endif
