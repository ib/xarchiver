/*
 *  Copyright (C) 2008 Giuseppe Torelli - <colossus73@gmail.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Street #330, Boston, MA 02111-1307, USA.
 */

#ifndef __XARCHIVER_WINDOW_H
#define __XARCHIVER_WINDOW_H

#define _GNU_SOURCE
#include <string.h>

#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <glib/gstdio.h>
#include <errno.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <time.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>

#include "arj.h"
#include "deb.h"
#include "lha.h"
#include "zip.h"
#include "rar.h"
#include "tar.h"
#include "7zip.h"
#include "bzip2.h"
#include "rpm.h"
#include "gzip.h"
#include "archive.h"
#include "new_dialog.h"
#include "extract_dialog.h"
#include "add_dialog.h"
#include "pref_dialog.h"

short int response;
double content_size;
unsigned long long int file_size, file_offset;
gboolean done,full_path,overwrite,add_recurse;
Extract_dialog_data *extract_window;
Add_dialog_data *add_window;
GtkWidget *dialog, *scrollwin, *view_window, *archive_properties_win;

void xa_new_archive (GtkMenuItem *menuitem, gpointer user_data);
void xa_open_archive (GtkMenuItem *menuitem, gpointer user_data );
void xa_test_archive (GtkMenuItem *menuitem, gpointer user_data);
void xa_close_archive (GtkMenuItem *menuitem, gpointer user_data);
void xa_quit_application (GtkMenuItem *menuitem, gpointer user_data);
void xa_delete_archive (GtkMenuItem *menuitem, gpointer user_data);
gboolean xa_launch_external_program(gchar *program,gchar *arg);
void xa_show_help (GtkMenuItem *menuitem, gpointer user_data);
void xa_reset_password (GtkMenuItem *menuitem, gpointer user_data );
void xa_about (GtkMenuItem *menuitem, gpointer user_data);
void on_options1_activate (GtkMenuItem *menuitem, gpointer user_data);
void xa_extract_archive ( GtkMenuItem *menuitem, gpointer user_data);
void xa_convert_sfx ( GtkMenuItem *menuitem, gpointer user_data);
void xa_select_all ( GtkMenuItem *menuitem, gpointer user_data);
void xa_deselect_all ( GtkMenuItem *menuitem, gpointer user_data);
void xa_show_archive_comment ( GtkMenuItem *menuitem, gpointer user_data);
void xa_show_cmd_line_output( GtkMenuItem *menuitem );
void xa_archive_properties ( GtkMenuItem *menuitem , gpointer user_data );
void xa_view_file_inside_archive ( GtkMenuItem *menuitem , gpointer user_data );
void xa_cancel_archive ( GtkMenuItem *menuitem , gpointer user_data);
void xa_add_files_archive ( GtkMenuItem *menuitem, gpointer user_data );
void xa_show_prefs_dialog ( GtkMenuItem *menuitem , gpointer user_data );
void xa_activate_delete_and_view (GtkTreeSelection *treeselection,gpointer data);
void on_drag_data_received (GtkWidget *widget,GdkDragContext *context, int x,int y,GtkSelectionData *data, unsigned int info, unsigned int time, gpointer user_data);
void drag_begin (GtkWidget *treeview1,GdkDragContext *context, gpointer data);
void drag_end (GtkWidget *treeview1, GdkDragContext *context, gpointer data);
void drag_data_get (GtkWidget *widget, GdkDragContext *dc, GtkSelectionData *selection_data, guint info, guint t, gpointer data);

int xa_show_message_dialog ( GtkWindow *window, int mode,int type,int button, const gchar *message1,const gchar *message2);
int xa_detect_archive_type ( gchar *filename );
gboolean xa_detect_archive_comment ( int type, gchar *filename, XArchive *archive );
gboolean key_press_function ( GtkWidget* widget, GdkEventKey* event,gpointer data);
gboolean treeview_select_search (GtkTreeModel *model,gint column,const gchar *key,GtkTreeIter *iter,gpointer search_data);
gboolean xa_check_child_for_error_on_exit(XArchive *archive,gint status);
void xa_archive_operation_finished(XArchive *archive,gboolean error);
void xa_reload_archive_content(XArchive *archive);
void xa_watch_child ( GPid pid, gint status, gpointer data);
void xa_remove_columns();
void xa_create_liststore ( XArchive *archive, gchar *columns_names[]);
void xa_concat_filenames (GtkTreeModel *model, GtkTreePath *treepath, GtkTreeIter *iter, GString *data);
void xa_shell_quote_filename (gchar *filename,GString *data,XArchive *archive);
void xa_cat_filenames (XArchive *archive,GSList *list,GString *data);
void xa_cat_filenames_basename (XArchive *archive,GSList *list,GString *data);
void xa_disable_delete_view_buttons (gboolean value);
void Update_StatusBar (gchar *msg);

gchar *xa_open_file_dialog ();
gchar *xa_open_sfx_file_selector ();
void xa_activate_link (GtkAboutDialog *about,const gchar *link,gpointer data);
void xa_location_entry_activated (GtkEntry *entry,gpointer user_data);
void xa_treeview_row_activated(GtkTreeView *tree_view,GtkTreePath *path,GtkTreeViewColumn *column,gpointer user_data);
int xa_mouse_button_event(GtkWidget *widget,GdkEventButton *event,gpointer data);
gchar *name;
gchar *permissions;
#endif

