/*
 *  Copyright (c) 2006 Stephan Arts <stephan.arts@hva.nl>
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
#ifndef __LIBXARCHIVER_H__
#define __LIBXARCHIVER_H__


G_BEGIN_DECLS

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
} XArchiveType;

typedef enum
{
	EXTRACT,
	ADD,
	REMOVE,
	OPEN,
	RELOAD,
	ERROR,
	USER_BREAK
} XArchiveStatus;

typedef struct _XArchive XArchive;

struct _XArchive
{
	XArchiveType type;
	XArchiveStatus status;
	gchar *path;
	gchar *passwd;
	gboolean has_passwd;
	unsigned long long int dummy_size;
	unsigned short int number_of_files;
	unsigned short int number_of_dirs;
	GError *error;
	gint child_pid;
	guint source;
	gint output_fd;
	gint input_fd;
	gint error_fd;
	GSList *output;
	GSList *err;
	GList	*row;
};

typedef struct _XArchiveSupport XArchiveSupport;

struct _XArchiveSupport
{
	XArchiveType type;
	gboolean (*verify)  (XArchive *);
	gboolean (*add)     (XArchive *, GSList *);
	gboolean (*extract) (XArchive *, gchar *, GSList *, gboolean);
	gboolean (*remove)  (XArchive *, GSList *);
	gboolean (*testing) (XArchive *);
	gboolean (*open)    (XArchive *);
	gboolean (*view)    (XArchive *, gchar *);
	gint n_columns;
	gchar **column_names;
	GType *column_types;
};

void xarchiver_init();

int xarchiver_destroy();

XArchive *xarchiver_archive_new(gchar *path, XArchiveType type);
XArchiveSupport *xarchiver_find_archive_support(XArchive *archive);

gint xarchiver_async_process ( XArchive *archive , gchar *command, gboolean input);
gboolean xarchiver_cancel_operation ( XArchive *archive );

gboolean
xarchiver_set_channel ( gint fd, GIOCondition cond, GIOFunc func, gpointer data );
G_END_DECLS

gboolean
xarchiver_error_function (GIOChannel *ioc, GIOCondition cond, gpointer data);

gboolean
xarchiver_output_function (GIOChannel *ioc, GIOCondition cond, gpointer data);

gboolean xarchiver_wait_child ( XArchive *archive, gpointer data );

#endif /* __LIBXARCHIVER_H__ */
