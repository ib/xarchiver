/*
 *  Copyright (c) 2008 Giuseppe Torelli <colossus73@gmail.com>
 *
 *  This program is free software, you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY, without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program, if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 */

#ifndef XARCHIVER_INTERFACE_H
#define XARCHIVER_INTERFACE_H

#include <gtk/gtk.h>
#include "archive.h"

typedef struct
{
	GtkWidget *window;
	GtkWidget *label;
	GtkWidget *bar;
	gboolean multi_extract;
} Progress;

extern GtkWidget *archive_dir_treeview;
extern GtkWidget *back_button;
extern GtkWidget *comment_data;
extern GtkWidget *comment_menu;
extern GtkWidget *compression_data;
extern GtkWidget *content_data;
extern GtkWidget *copy;
extern GtkWidget *cut;
extern GtkWidget *ddelete;
extern GtkWidget *delete_menu;
extern GtkWidget *deselect_all;
extern GtkWidget *eextract;
extern GtkWidget *encrypted_data;
extern GtkWidget *home_button;
extern GtkWidget *hpaned1;
extern GtkWidget *listing;
extern GtkWidget *location_entry;
extern GtkWidget *modified_data;
extern GtkWidget *name_data;
extern GtkWidget *number_of_files_data;
extern GtkWidget *open_popupmenu;
extern GtkWidget *password_entry_menu;
extern GtkWidget *paste;
extern GtkWidget *path_data;
extern GtkWidget *rename_menu;
extern GtkWidget *rrename;
extern GtkWidget *scrolledwindow2;
extern GtkWidget *selected_frame;
extern GtkWidget *selected_label;
extern GtkWidget *select_all;
extern GtkWidget *size_data;
extern GtkWidget *Stop_button;
extern GtkWidget *toolbar1;
extern GtkWidget *toolbar2;
extern GtkWidget *total_label;
extern GtkWidget *type_data;
extern GtkWidget *unsort_menu;
extern GtkWidget *up_button;
extern GtkWidget *view;
extern GtkWidget *view_shell_output1;
extern GtkWidget *xa_popup_menu;
extern GtkAccelGroup *accel_group;
extern GtkNotebook *notebook;
extern GtkTreeStore *archive_dir_model;
extern Progress *progress;

gboolean select_matched_rows(GtkTreeModel *, GtkTreePath *, GtkTreeIter *, gpointer);
void xa_add_page(XArchive *);
void xa_combo_box_text_append_compressor_types(GtkComboBoxText *);
gboolean xa_check_password(XArchive *);
GtkWidget *xa_create_archive_properties_window();
void xa_create_main_window(GtkWidget *, gboolean, gboolean, gboolean);
void xa_disable_delete_buttons(gboolean);
GSList *xa_file_filter_add_archiver_pattern_sort(GtkFileFilter *);
gboolean xa_flash_led_indicator(XArchive *);
void xa_increase_progress_bar(Progress *, gchar *, double);
gboolean xa_pulse_progress_bar(gpointer);
void xa_set_button_state(gboolean, gboolean, gboolean, gboolean, gboolean, gboolean, gboolean, gboolean, gboolean, gboolean, GSList *, gboolean);
void xa_show_progress_bar(XArchive *);

#endif
