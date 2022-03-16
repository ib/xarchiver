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
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 *  MA 02110-1301 USA.
 */

#ifndef XARCHIVER_WINDOW_H
#define XARCHIVER_WINDOW_H

#include <gtk/gtk.h>
#include "archive.h"

extern gchar *current_open_directory;

void drag_begin(GtkWidget *, GdkDragContext *, XArchive *);
void drag_data_get(GtkWidget *, GdkDragContext *, GtkSelectionData *, guint, guint, XArchive *);
void xa_page_drag_data_received(GtkWidget *, GdkDragContext *, int, int, GtkSelectionData *, unsigned int, unsigned int, gpointer);
gboolean treeview_select_search(GtkTreeModel *, gint, const gchar *, GtkTreeIter *, gpointer);
void xa_about(GtkMenuItem *, gpointer);
void xa_add_files_archive(GtkMenuItem *, gpointer);
void xa_archive_properties(GtkMenuItem *, gpointer);
void xa_cancel_archive(GtkMenuItem *, gpointer);
void xa_child_processed(XAChildProcess, guint8, XArchive *);
void xa_clipboard_clear(GtkClipboard *, XArchive *);
void xa_clipboard_copy(GtkMenuItem *, gpointer);
void xa_clipboard_cut(GtkMenuItem *, gpointer);
void xa_clipboard_paste(GtkMenuItem *, gpointer);
void xa_close_archive(GtkWidget *, gpointer);
void xa_concat_selected_filenames(GtkTreeModel *, GtkTreePath *, GtkTreeIter *, GSList **);
void xa_convert_sfx(GtkMenuItem *, gpointer);
void xa_create_liststore(XArchive *, const gchar *[]);
void xa_delete_archive(GtkMenuItem *, gpointer);
void xa_deselect_all(GtkMenuItem *, gpointer);
ArchiveType xa_detect_archive_type(const gchar *);
void xa_enter_password(GtkMenuItem *, gpointer);
void xa_extract_archive(GtkMenuItem *, gpointer);
gboolean xa_launch_external_program(const gchar *, const gchar *);
void xa_list_archive(GtkMenuItem *, gpointer);
void xa_location_entry_activated(GtkEntry *, gpointer);
GtkWidget *xa_main_window_find_image(gchar *, GtkIconSize);
gboolean xa_mouse_button_event(GtkWidget *, GdkEventButton *, XArchive *);
void xa_new_archive(GtkMenuItem *, gpointer);
void xa_open_archive(GtkWidget *, gchar *);
void xa_open_with_from_popupmenu(GtkMenuItem *, gpointer);
void xa_quit_application(GtkWidget *, GdkEvent *, gpointer);
void xa_reload_archive_content(XArchive *);
void xa_rename_archive(GtkMenuItem *, gpointer);
void xa_row_selected(GtkTreeSelection *, XArchive *);
void xa_save_archive(GtkMenuItem *, gpointer);
void xa_select_all(GtkMenuItem *, gpointer);
void xa_set_statusbar_message_for_displayed_rows(XArchive *);
void xa_set_xarchiver_icon(GtkWindow *);
void xa_show_archive_comment(GtkMenuItem *, gpointer);
void xa_show_archive_output(GtkMenuItem *, XArchive *);
void xa_show_help(GtkMenuItem *, gpointer);
int xa_show_message_dialog(GtkWindow *, int, int, int, const gchar *, const gchar *);
void xa_show_multi_extract_dialog(GtkMenuItem *, gpointer);
void xa_show_prefs_dialog(GtkMenuItem *, gpointer);
void xa_test_archive(GtkMenuItem *, gpointer);
void xa_treeview_row_activated(GtkTreeView *, GtkTreePath *, GtkTreeViewColumn *, XArchive *);
void xa_unsort(GtkMenuItem *, gpointer);
void xa_update_window_with_archive_entries(XArchive *, XEntry *);
void xa_view_from_popupmenu(GtkMenuItem *, gpointer);

#endif
