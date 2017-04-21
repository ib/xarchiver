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
	XARCHIVETYPE_FIRST,
	XARCHIVETYPE_7ZIP = XARCHIVETYPE_FIRST,
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
	XARCHIVETYPE_TYPES,
	XARCHIVETYPE_UNKNOWN,
	XARCHIVETYPE_NOT_FOUND
} XArchiveType;

typedef enum
{
	XARCHIVESTATUS_IDLE,
	XARCHIVESTATUS_OPEN,
	XARCHIVESTATUS_TEST,
	XARCHIVESTATUS_EXTRACT,
	XARCHIVESTATUS_ADD,
	XARCHIVESTATUS_DELETE,
	XARCHIVESTATUS_SFX,
	XARCHIVESTATUS_RELOAD,
	XARCHIVESTATUS_ERROR
} XArchiveStatus;

typedef enum
{
	XA_CHILD_EXIT,
	XA_CHILD_STDOUT,
	XA_CHILD_STDERR,
	XA_CHILD_PROCS
} XAChildProcess;

typedef struct
{
	gchar *program[2];
	gboolean is_compressor;
	GSList *type;
	GSList *glob;
} XArchiver;

typedef struct _XEntry XEntry;

struct _XEntry
{
	gchar *filename;
	gboolean is_dir;
	gboolean is_encrypted;
	gpointer columns;
	XEntry *child;
	XEntry *prev;
	XEntry *next;
};

typedef struct _XArchive XArchive;

typedef void (*ask_func)(XArchive *);
typedef void (*open_func)(XArchive *);
typedef void (*parse_output_func)(gchar *, XArchive *);
typedef void (*test_func)(XArchive *);
typedef gboolean (*extract_func)(XArchive *, GSList *);
typedef void (*add_func)(XArchive *, GSList *, gchar *);
typedef void (*delete_func)(XArchive *, GSList *);

typedef struct _XAClipboard XAClipboard;

struct _XArchive
{
	/* characteristics */
	XArchiveType type;
	gushort version;
	XArchiveStatus status;
	/* data */
	XEntry *root_entry;
	XEntry *current_entry;
	/* user interface */
	GtkWidget *page;
	GtkWidget *treeview;
	GtkTreeModel *model;
	GtkListStore *liststore;
	guint columns;
	GType *column_types;
	GtkCellRenderer *pixbuf_renderer;
	GtkCellRenderer *text_renderer;
	GSList *back;
	GSList *forward;
	XAClipboard *clipboard;
	/* environment */
	gchar *path[3];           // 0: original, 1: escaped, 2: working copy
	gchar *working_dir;
	gchar *extraction_dir;
	gchar *location_path;
	/* properties */
	guint files;
	guint64 files_size;
	gboolean has_password;
	gchar *password;
	gboolean has_comment;
	GString *comment;
	gushort compression;
	/* capabilities */           // 0: extract, 1: add
	gboolean can_test;           // can test an archive for integrity
	gboolean can_extract;        // can extract files from an archive
	gboolean can_add;            // can add files to an archive
	gboolean can_delete;         // can delete files from an archive
	//       can_rename             see macro below: can_extract AND can_delete AND can_add
	gboolean can_sfx;            // can create a self-extracting archive
	gboolean can_password;       // can password protect an archive
	gboolean can_full_path[2];   // can carry out activity with or without full path
	gboolean can_touch;          // can modify timestamp of files on extraction
	gboolean can_overwrite;      // can overwrite files on extraction
	gboolean can_update[2];      // can limit activity to only changed or new files
	gboolean can_freshen[2];     // can limit activity to only changed files
	gboolean can_move;           // can delete files after adding
	gboolean can_solid;          // can create a solid archive
	/* instructions */
	gboolean do_full_path;
	gboolean do_touch;
	gboolean do_overwrite;
	gboolean do_update;
	gboolean do_freshen;
	gboolean do_recurse;
	gboolean do_move;
	gboolean do_solid;
	/* child process */
	gchar *child_dir;     // (free and set NULL after use)
	GPid child_pid;
	guint child_ref;
	gint child_fdout;
	gint child_fderr;
	GSList *output;
	/* (un)compressor interface */
	ask_func ask;
	open_func open;
	parse_output_func parse_output;
	test_func test;
	extract_func extract;
	add_func add;
	delete_func delete;
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
	XAClipboardMode mode;
	GSList *files;
	XArchive *target;
};

gchar *xa_build_full_path_name_from_entry(XEntry *);
void xa_clean_archive_structure(XArchive *);
gboolean xa_create_working_directory(XArchive *);
void xa_detect_encrypted_archive(XArchive *);
void xa_fill_dir_sidebar(XArchive *, gboolean);
void xa_fill_list_with_recursed_entries(XEntry *, GSList **);
gint xa_find_archive_index(gint);
XEntry *xa_find_entry_from_path(XEntry *, const gchar *);
void xa_free_entry(XArchive *, XEntry *);
gint xa_get_new_archive_idx();
XArchive *xa_init_archive_structure(gint);
gboolean xa_run_command(XArchive *, const gchar *);
XEntry *xa_set_archive_entries_for_each_row(XArchive *, gchar *, gpointer *);
void xa_dir_sidebar_row_selected(GtkTreeSelection *, gpointer);
void xa_dir_sidebar_select_row(XEntry *);
gint xa_sort_dirs_before_files(GtkTreeModel *, GtkTreeIter *, GtkTreeIter *, gpointer);
void xa_spawn_async_process(XArchive *, const gchar *);

#endif
