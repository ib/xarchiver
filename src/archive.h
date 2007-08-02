/*
 *  Copyright (c) 2007 Giuseppe Torelli <colossus73@gmail.com>
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
	XARCHIVETYPE_UNKNOWN = 0,
	XARCHIVETYPE_7ZIP,
	XARCHIVETYPE_ARJ,
	XARCHIVETYPE_DEB,
	XARCHIVETYPE_BZIP2,
	XARCHIVETYPE_GZIP,
	XARCHIVETYPE_ISO,
	XARCHIVETYPE_LZMA,
	XARCHIVETYPE_RAR,
	XARCHIVETYPE_RPM,
	XARCHIVETYPE_TAR,
	XARCHIVETYPE_TAR_BZ2,
	XARCHIVETYPE_TAR_GZ,
	XARCHIVETYPE_TAR_LZMA,
	XARCHIVETYPE_ZIP,
	XARCHIVETYPE_LHA
} XArchiveType;

typedef enum
{
	XA_ARCHIVESTATUS_IDLE = 0,
	XA_ARCHIVESTATUS_EXTRACT,
	XA_ARCHIVESTATUS_ADD,
	XA_ARCHIVESTATUS_DELETE,
	XA_ARCHIVESTATUS_OPEN,
	XA_ARCHIVESTATUS_TEST,
	XA_ARCHIVESTATUS_SFX
} XArchiveStatus;

typedef struct _XEntry XEntry;

struct _XEntry
{
	gchar *filename;
	gchar *mime_type;
	gpointer columns;
	gboolean is_dir;
	gboolean is_encrypted;
	XEntry *child;
	XEntry *next;
};

typedef struct _XArchive XArchive;

struct _XArchive
{
	XArchiveType type;
	XArchiveStatus status;
	XEntry *entry;
	gchar *path;
	gchar *escaped_path;
	gchar *tmp;
	gchar *format;
	gchar *extraction_path;
	gchar *passwd;
	gchar *location_entry_path;
	GtkTreeModel *model;
	GtkListStore *liststore;
	GtkWidget *treeview;
	GtkWidget *scrollwindow;
	gboolean has_passwd;
	gboolean has_comment;
	gboolean has_test;
	gboolean has_sfx;
	gboolean can_add;
	gboolean can_extract;
	gboolean has_properties;
	GString *comment;
	//TODO: remove this once you fix the various arj,rar,etc
	GList *cmd_line_output;
	GSList *error_output;
	GSList *entries;
	GType *column_types;
	gboolean add_recurse;
	gboolean overwrite;
	gboolean full_path;
	gboolean freshen;
	gboolean update;
	gboolean tar_touch;
	gboolean solid_archive;
	gboolean remove_files;
	unsigned short int compression_level;
	unsigned short int nc;
	gint nr_of_files;
	gint nr_of_dirs;
	gint input_fd;
	gint output_fd;
	gint error_fd;
	guint pb_source;
	GPid child_pid;
	unsigned long long int dummy_size;
	void (*parse_output) (gchar *line, gpointer data);
};

GHashTable *filename_paths_buffer;
gchar *system_id,*volume_id,*publisher_id,*preparer_id,*application_id,*creation_date,*modified_date,*expiration_date,*effective_date;
void xa_spawn_async_process (XArchive *archive, gchar *command , gboolean input);
XArchive *xa_init_archive_structure ();
void xa_clean_archive_structure ( XArchive *archive);
gboolean xa_dump_child_error_messages (GIOChannel *ioc, GIOCondition cond, gpointer data);
gint xa_find_archive_index (gint page_num);
gint xa_get_new_archive_idx();
XEntry *xa_alloc_memory_for_each_row ( guint nc,GType column_types[]);
void xa_free_entry (XArchive *archive,XEntry *entry);
XEntry *xa_find_archive_entry(XEntry *entry, gchar *string);
XEntry *xa_set_archive_entries_for_each_row (XArchive *archive,gchar *filename,gboolean encrypted,gpointer *items);
gpointer *xa_fill_archive_entry_columns_for_each_row (XArchive *archive,XEntry *entry,gpointer *items);
void xa_update_window_with_archive_entries (XArchive *archive,gchar *path);
XArchive *archive[100];
XArchive *archive_cmd;
#endif
