/*
 *  Copyright (c) 2006 Giuseppe Torelli <colossus73@gmail.com>
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

typedef enum
{
	XARCHIVETYPE_UNKNOWN,
	XARCHIVETYPE_7ZIP,
	XARCHIVETYPE_ARJ,
	XARCHIVETYPE_BZIP2,
	XARCHIVETYPE_GZIP,
	XARCHIVETYPE_ISO,
	XARCHIVETYPE_RAR,
	XARCHIVETYPE_RPM,
	XARCHIVETYPE_TAR,
	XARCHIVETYPE_TAR_BZ2,
	XARCHIVETYPE_TAR_GZ,
	XARCHIVETYPE_ZIP
} XArchiveType;

typedef enum
{
	XA_ARCHIVESTATUS_IDLE,
	XA_ARCHIVESTATUS_EXTRACT,
	XA_ARCHIVESTATUS_ADD,
	XA_ARCHIVESTATUS_DELETE,
	XA_ARCHIVESTATUS_OPEN,
	XA_ARCHIVESTATUS_RELOAD,
	XA_ARCHIVESTATUS_ERROR,
	XA_ARCHIVESTATUS_USER_BREAK,
	XA_ARCHIVESTATUS_TEST
} XArchiveStatus;

typedef struct _XArchive XArchive;

struct _XArchive
{
	XArchiveType type;
	XArchiveStatus status;
	gchar *path;
	gchar *escaped_path;
	gchar *tmp;
	gboolean has_passwd;
	gboolean add_recurse;
	gboolean overwrite;
	gboolean full_path;
	gboolean freshen;
	gboolean update;
	gboolean tar_touch;
	gboolean solid_archive;
	gboolean remove_files;
	unsigned short int compression_level;
	unsigned short int tar_strip_value;
	gchar *passwd;
	gint nr_of_files;
	gint nr_of_dirs;
	GPid child_pid;
	unsigned long long int dummy_size;
	unsigned int row_cnt;
	GList *row;
	gboolean (*parse_output) (GIOChannel *ioc, GIOCondition cond, gpointer data);
};

unsigned short int x;
gint input_fd, output_fd, error_fd;
void SpawnAsyncProcess (XArchive *archive, gchar *command , gboolean input, gboolean output_flag);
XArchive *xa_init_archive_structure ();
void xa_clean_archive_structure (XArchive *archive);
XArchive *archive;
#endif
