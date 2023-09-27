/*
 *  Copyright (c) 2008 Giuseppe Torelli <colossus73@gmail.com>
 *  Copyright (C) 2016 Ingo Brückl
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 *  MA 02110-1301 USA.
 */

#ifndef XARCHIVER_ARCHIVE_H
#define XARCHIVER_ARCHIVE_H

#include <gtk/gtk.h>

typedef enum
{
	XARCHIVETYPE_FIRST,
	XARCHIVETYPE_7ZIP = XARCHIVETYPE_FIRST,
	XARCHIVETYPE_AR,
	XARCHIVETYPE_ARJ,
	XARCHIVETYPE_BZIP,
	XARCHIVETYPE_BZIP2,
	XARCHIVETYPE_BZIP3,
	XARCHIVETYPE_CAB,
	XARCHIVETYPE_COMPRESS,
	XARCHIVETYPE_CPIO,
	XARCHIVETYPE_GZIP,
	XARCHIVETYPE_ISO,
	XARCHIVETYPE_LHA,
	XARCHIVETYPE_LRZIP,
	XARCHIVETYPE_LZ4,
	XARCHIVETYPE_LZIP,
	XARCHIVETYPE_LZMA,
	XARCHIVETYPE_LZOP,
	XARCHIVETYPE_RAR,
	XARCHIVETYPE_RPM,
	XARCHIVETYPE_RZIP,
	XARCHIVETYPE_SQUASHFS,
	XARCHIVETYPE_TAR,
	XARCHIVETYPE_TAR_BZIP,
	XARCHIVETYPE_TAR_BZIP2,
	XARCHIVETYPE_TAR_BZIP3,
	XARCHIVETYPE_TAR_COMPRESS,
	XARCHIVETYPE_TAR_GZIP,
	XARCHIVETYPE_TAR_LRZIP,
	XARCHIVETYPE_TAR_LZ4,
	XARCHIVETYPE_TAR_LZIP,
	XARCHIVETYPE_TAR_LZMA,
	XARCHIVETYPE_TAR_LZOP,
	XARCHIVETYPE_TAR_RZIP,
	XARCHIVETYPE_TAR_XZ,
	XARCHIVETYPE_TAR_ZSTD,
	XARCHIVETYPE_XZ,
	XARCHIVETYPE_ZIP,
	XARCHIVETYPE_ZPAQ,
	XARCHIVETYPE_ZSTD,
	XARCHIVETYPE_TYPES,
	XARCHIVETYPE_UNKNOWN,
	XARCHIVETYPE_NOT_FOUND
} XArchiveType;

typedef enum
{
	XARCHIVESTATUS_IDLE,
	XARCHIVESTATUS_LIST,
	XARCHIVESTATUS_TEST,
	XARCHIVESTATUS_EXTRACT,
	XARCHIVESTATUS_ADD,
	XARCHIVESTATUS_DELETE,
	XARCHIVESTATUS_SFX,
	XARCHIVESTATUS_RELOAD,
	XARCHIVESTATUS_OTHER,
	XARCHIVESTATUS_ERROR
} XArchiveStatus;

typedef enum
{
	XA_CHILD_EXIT,
	XA_CHILD_STDOUT,
	XA_CHILD_STDERR,
	XA_CHILD_PROCS
} XAChildProcess;

typedef struct _XArchive XArchive;

typedef void (*ask_func)(XArchive *);
typedef void (*list_func)(XArchive *);
typedef void (*parse_output_func)(gchar *, XArchive *);
typedef void (*test_func)(XArchive *);
typedef gboolean (*extract_func)(XArchive *, GSList *);
typedef void (*add_func)(XArchive *, GSList *);
typedef void (*delete_func)(XArchive *, GSList *);

typedef struct
{
	gchar *program[2];
	gboolean is_compressor;
	GSList *type;
	GSList *glob;
	GSList *tags;
	ask_func ask;
	list_func list;
	test_func test;
	extract_func extract;
	add_func add;
	delete_func delete;
} XArchiver;

typedef struct
{
	XArchiveType type;
	gushort tag;
} ArchiveType;

#define TAG(t1,t2) ((t2 << 8) + t1)
#define TAGTYPE(t) (t & 0xff)

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

typedef struct
{
	gboolean can_uncompressed;   // has compression level 0?
	gushort least;               // least compression level
	gushort preset;              // default compression level
	gushort best;                // best compression level
	gushort steps;               // level steps (usually 1)
} compressor_t;

struct _XArchive
{
	/* characteristics */
	XArchiveType type;
	gushort tag;
	XArchiveStatus status;
	/* data */
	XEntry *root_entry;
	XEntry *current_entry;
	/* user interface */
	GtkWidget *page;
	GtkWidget *treeview;
	GtkTreeModel *model;
	GtkListStore *liststore;
	gboolean sorted;
	guint columns;
	guint size_column;
	GType *column_types;
	GtkCellRenderer *pixbuf_renderer;
	GtkCellRenderer *text_renderer;
	GSList *back;
	GSList *forward;
	/* environment */
	gchar *path[4];          // 0: original, 1: escaped, 2: working copy, 3: escaped working copy
	gchar *working_dir;
	gchar *destination_path;
	gchar *extraction_dir;   // (free before use)
	gchar *location_path;
	/* properties */
	guint files;
	guint64 files_size;
	gboolean has_password;
	gchar *password;
	gboolean has_comment;
	GString *comment;
	compressor_t compressor;
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
	gboolean can_recurse;        // can automatically recurse through subdirectories
	gboolean can_compress;       // can compress at different levels
	/* instructions */
	gboolean do_full_path;   // extract_func, (xa_execute_add_commands)
	gboolean do_touch;       // extract_func
	gboolean do_overwrite;   // extract_func
	gboolean do_update;      // extract_func, add_func
	gboolean do_freshen;     // extract_func, add_func
	gboolean do_move;        // add_func
	gboolean do_solid;       // add_func
	/* child process */
	guint timeout;
	gchar *child_dir;   // (free and set NULL after use)
	GPid child_pid;
	guint child_ref;
	gint child_fdout;
	gint child_fderr;
	gchar *command;     // (free before use)
	GSList *output;
	/* (de)compressor interface */
	XArchiver *archiver;
	parse_output_func parse_output;
	guint8 exitstatus_ok;
};

extern XArchive *archive[];

#define can_rename(archive) ((archive)->can_extract && (archive)->can_delete && (archive)->can_add)

typedef enum
{
	XA_CLIPBOARD_EMPTY,
	XA_CLIPBOARD_COPY,
	XA_CLIPBOARD_CUT,
	XA_CLIPBOARD_EDIT
} XAClipboardMode;

typedef struct
{
	XAClipboardMode mode;
	gchar *origin;
	XEntry *target;
	GSList *paths;
	GSList *files;
	struct timespec mtime;
	XArchive *archive;
} XAClipboard;

extern XAClipboard XA_Clipboard;

gchar *xa_build_full_path_name_from_entry(XEntry *);
void xa_clean_archive_structure(XArchive *);
gchar *xa_create_containing_directory(XArchive *, const gchar *);
gboolean xa_create_working_directory(XArchive *);
gchar *xa_create_working_subdirectory(XArchive *);
void xa_detect_encrypted_archive(XArchive *);
void xa_fill_dir_sidebar(XArchive *, gboolean);
void xa_fill_list_with_recursed_entries(XEntry *, GSList **);
gint xa_find_archive_index(gint);
XEntry *xa_find_entry_from_dirpath(XArchive *, const gchar *);
void xa_free_entry(XArchive *, XEntry *);
gboolean xa_get_compressed_tar_type(XArchiveType *);
gint xa_get_new_archive_index();
gboolean xa_has_containing_directory(XArchive *);
XArchive *xa_init_archive_structure(ArchiveType);
gboolean xa_run_command(XArchive *, const gchar *);
XEntry *xa_set_archive_entries_for_each_row(XArchive *, const gchar *, gpointer *);
void xa_dir_sidebar_row_selected(GtkTreeSelection *, gpointer);
void xa_dir_sidebar_select_row(XEntry *);
gint xa_sort_dirs_before_files(GtkTreeModel *, GtkTreeIter *, GtkTreeIter *, XArchive *);
void xa_spawn_async_process(XArchive *, const gchar *);

#endif
