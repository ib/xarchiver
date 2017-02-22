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

#ifndef XARCHIVER_ARCHIVE_H
#define XARCHIVER_ARCHIVE_H

#include <gtk/gtk.h>

typedef enum
{
	XARCHIVETYPE_UNKNOWN,
	XARCHIVETYPE_NOT_FOUND,
	XARCHIVETYPE_7ZIP,
	XARCHIVETYPE_ARJ,
	XARCHIVETYPE_BZIP2,
	XARCHIVETYPE_DEB,
	XARCHIVETYPE_GZIP,
	XARCHIVETYPE_LHA,
	XARCHIVETYPE_LZMA,
	XARCHIVETYPE_LZOP,
	XARCHIVETYPE_RAR,
	XARCHIVETYPE_RPM,
	XARCHIVETYPE_TAR,
	XARCHIVETYPE_TAR_BZ2,
	XARCHIVETYPE_TAR_GZ,
	XARCHIVETYPE_TAR_LZMA,
	XARCHIVETYPE_TAR_LZOP,
	XARCHIVETYPE_TAR_XZ,
	XARCHIVETYPE_XZ,
	XARCHIVETYPE_ZIP,
	XARCHIVETYPE_COUNT
} XArchiveType;

typedef enum
{
	XA_ARCHIVESTATUS_IDLE,
	XA_ARCHIVESTATUS_OPEN,
	XA_ARCHIVESTATUS_TEST,
	XA_ARCHIVESTATUS_EXTRACT,
	XA_ARCHIVESTATUS_ADD,
	XA_ARCHIVESTATUS_DELETE,
	XA_ARCHIVESTATUS_RENAME,
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

typedef void (*ask_func)(XArchive *);
typedef void (*open_func)(XArchive *);
typedef void (*parse_output_func)(gchar *, gpointer);
typedef void (*test_func)(XArchive *);
typedef gboolean (*extract_func)(XArchive *, GSList *);
typedef void (*add_func)(XArchive *, GString *, gchar *);
typedef void (*delete_func)(XArchive *, GSList *);

struct _XArchive
{
	XArchiveType type;
	int version;
	XArchiveStatus status;
	XEntry *root_entry;
	XEntry *current_entry;
	XAClipboard *clipboard_data;
	GSList *back;
	GSList *forward;
	gchar *path;
	gchar *escaped_path;
	gchar *tmp;
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
	gboolean can_test;
	gboolean can_sfx;
	gboolean can_passwd;
	gboolean can_overwrite;
	gboolean can_full_path;
	gboolean can_touch;
	gboolean can_freshen;
	gboolean can_update;
	gboolean can_recurse;
	gboolean can_solid;
	gboolean can_move;
	gboolean can_add;
	gboolean can_delete;
	gboolean can_extract;
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
	gboolean touch;
	gboolean add_solid;
	gboolean add_move;
	unsigned short int compression_level;
	unsigned short int nc;
	gint nr_of_files;
	gint output_fd;
	gint error_fd;
	guint pb_source;
	GPid child_pid;
	unsigned long long int files_size;
	ask_func ask;
	parse_output_func parse_output;
	delete_func delete;
	add_func add;
	extract_func extract;
	test_func test;
	open_func open;
};

extern XArchive *archive[];

#define can_rename(archive) ((archive)->can_extract && (archive)->can_delete && (archive)->can_add)

#define XA_CLIPBOARD (gdk_atom_intern_static_string("XARCHIVER_OWN_CLIPBOARD"))
#define XA_INFO_LIST (gdk_atom_intern_static_string("application/xarchiver-info-list"))

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

gchar *xa_build_full_path_name_from_entry(XEntry *, XArchive *);
void xa_clean_archive_structure(XArchive *);
gboolean xa_create_temp_directory(XArchive *);
gboolean xa_detect_encrypted_archive(XArchive *);
void xa_fill_dir_sidebar(XArchive *, gboolean);
void xa_fill_list_with_recursed_entries(XEntry *, GSList **);
gint xa_find_archive_index(gint);
XEntry *xa_find_entry_from_path(XEntry *, const gchar *);
void xa_free_entry(XArchive *, XEntry *);
const gchar *xa_get_archive_format(XArchive *);
gint xa_get_new_archive_idx();
XArchive *xa_init_archive_structure(gint);
gboolean xa_run_command(XArchive *, GSList *);
XEntry *xa_set_archive_entries_for_each_row(XArchive *, gchar *, gpointer *);
void xa_sidepane_row_selected(GtkTreeSelection *, gpointer);
void xa_sidepane_select_row(XEntry *);
gint xa_sort_dirs_before_files(GtkTreeModel *, GtkTreeIter *, GtkTreeIter *, gpointer);
void xa_spawn_async_process(XArchive *, gchar *);

#endif
