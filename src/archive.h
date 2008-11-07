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
 * *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifndef __XARCHIVER_ARCHIVE_H__
#define __XARCHIVER_ARCHIVE_H__

int status;

typedef enum
{
	XARCHIVETYPE_UNKNOWN = 0,
	XARCHIVETYPE_NOT_FOUND,
	XARCHIVETYPE_7ZIP,
	XARCHIVETYPE_ARJ,
	XARCHIVETYPE_DEB,
	XARCHIVETYPE_BZIP2,
	XARCHIVETYPE_GZIP,
	XARCHIVETYPE_LZMA,
	XARCHIVETYPE_LZOP,
	XARCHIVETYPE_RAR,
	XARCHIVETYPE_RPM,
	XARCHIVETYPE_TAR,
	XARCHIVETYPE_TAR_BZ2,
	XARCHIVETYPE_TAR_GZ,
	XARCHIVETYPE_TAR_LZMA,
	XARCHIVETYPE_TAR_LZOP,
	XARCHIVETYPE_ZIP,
	XARCHIVETYPE_LHA,
	XARCHIVETYPE_COUNT
} XArchiveType;

typedef enum
{
	XA_ARCHIVESTATUS_IDLE = 0,
	XA_ARCHIVESTATUS_EXTRACT,
	XA_ARCHIVESTATUS_ADD,
	XA_ARCHIVESTATUS_DELETE,
	XA_ARCHIVESTATUS_RENAME,
	XA_ARCHIVESTATUS_OPEN,
	XA_ARCHIVESTATUS_TEST,
	XA_ARCHIVESTATUS_SFX,
	XA_ARCHIVESTATUS_ERROR
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
	XEntry *prev;
	XEntry *next;
};

typedef struct _XArchive XArchive;
typedef struct _XAClipboard XAClipboard;

typedef void (*parse_output_func)	(gchar *line, gpointer);
typedef void (*delete_func)			(XArchive *,GSList *);
typedef void (*add_func)			(XArchive *,GString *,gchar *);
typedef gboolean (*extract_func)	(XArchive *,GSList *);
typedef void (*test_func)			(XArchive *);
typedef void (*open_func)			(XArchive *);

struct _XArchive
{
	XArchiveType type;
	XArchiveStatus status;
	XEntry *root_entry;
	XEntry *current_entry;
	XAClipboard *clipboard_data;
	GSList *back;
	GSList *forward;
	gchar *path;
	gchar *escaped_path;
	gchar *tmp;
	gchar *format;
	gchar *extraction_path;
	gchar *passwd;
	gchar *location_entry_path;
	gchar *working_dir;
	GtkTreeModel *model;
	GtkCellRenderer *renderer;
	GtkCellRenderer *renderer_text;
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
	gboolean cut_copy_string;
	gboolean create_image;
	GString *comment;
	GSList *error_output;
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
	gint output_fd;
	gint error_fd;
	guint pb_source;
	GPid child_pid;
	unsigned long long int dummy_size;
	parse_output_func parse_output;
	delete_func delete;
	add_func add;
	extract_func extract;
	test_func test;
	open_func open_archive;
};

#define XA_CLIPBOARD (gdk_atom_intern_static_string ("XARCHIVER_OWN_CLIPBOARD")) 
#define XA_INFO_LIST (gdk_atom_intern_static_string ("application/xarchiver-info-list"))

typedef enum
{
	XA_CLIPBOARD_CUT,
	XA_CLIPBOARD_COPY
} XAClipboardMode;

struct _XAClipboard
{
	gchar *filename;
	XAClipboardMode mode;
	XArchive *cut_copy_archive;
	GSList *files;
};

void xa_spawn_async_process (XArchive *, gchar *);
//gchar *xa_split_command_line(XArchive *archive,GSList *list);
XArchive *xa_init_archive_structure(gint);
void xa_clean_archive_structure (XArchive *);
gboolean xa_dump_child_error_messages (GIOChannel *, GIOCondition , gpointer );
gboolean xa_create_temp_directory(XArchive *);
void xa_delete_temp_directory(XArchive *);
gboolean xa_run_command (XArchive *,GSList *);
gint xa_find_archive_index (gint );
gint xa_get_new_archive_idx();
XEntry *xa_alloc_memory_for_each_row ( guint ,GType column_types[]);
void xa_free_entry(XArchive *archive,XEntry *);
XEntry *xa_find_child_entry(XEntry *, gchar *);
XEntry *xa_set_archive_entries_for_each_row(XArchive *,gchar *,gpointer *);
gpointer *xa_fill_archive_entry_columns_for_each_row (XArchive *,XEntry *,gpointer *);
XEntry* xa_find_entry_from_path(XEntry *root_entry,const gchar *);
gchar *xa_build_full_path_name_from_entry(XEntry *, XArchive *);
void xa_fill_list_with_recursed_entries(XEntry *,GSList **);
gboolean xa_detect_encrypted_archive (XArchive *);
void xa_browse_dir_sidebar (XEntry *, GtkTreeStore *,gchar *,GtkTreeIter *);
void xa_fill_dir_sidebar(XArchive *,gboolean);
void xa_sidepane_row_selected(GtkTreeSelection *, gpointer );
void xa_sidepane_select_row(XEntry *);
gboolean _xa_sidepane_select_row(GtkTreeModel *,GtkTreePath *,GtkTreeIter *,gpointer );
gint xa_sort_dirs_before_files(GtkTreeModel *,GtkTreeIter *,GtkTreeIter *,gpointer );
XArchive *archive[100];
#endif
