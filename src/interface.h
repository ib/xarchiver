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
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 */

#ifndef __XARCHIVER_INTERFACE_H__
#define __XARCHIVER_INTERFACE_H__

#include "archive.h"
GtkWidget *xa_popup_menu;
GtkWidget *xa_main_window;
GtkWidget *vbox1;
GtkNotebook *notebook;
GtkWidget *hbox_sb;
GtkWidget *menubar1;
GtkWidget *menuitem1;
GtkWidget *menuitem1_menu;
GtkWidget *new1;
GtkWidget *open1;
GtkWidget *entry1;
GtkWidget *progressbar;
GtkWidget *info_label;
GtkWidget *viewport1,*viewport2;
GtkWidget *archive_properties_window;
GtkWidget *table1;
GtkWidget *path_label;
GtkWidget *modified_label;
GtkWidget *size_label;
GtkWidget *content_label;
GtkWidget *comment_label;
GtkWidget *compression_label;
GtkWidget *number_of_files_label;
GtkWidget *number_of_dirs_label;
GtkWidget *name_label;
GtkWidget *type_label;
GtkWidget *compression_data;
GtkWidget *number_of_files_data;
GtkWidget *number_of_dirs_data;
GtkWidget *content_data;
GtkWidget *comment_data;
GtkWidget *size_data;
GtkWidget *modified_data;
GtkWidget *path_data;
GtkWidget *type_data;
GtkWidget *name_data;
GtkWidget *separatormenuitem1;
GtkWidget *separatormenuitem2;
GtkWidget *separatormenuitem3;
GtkWidget *separatormenuitem4;
GtkWidget *separatormenuitem5;
GtkWidget *separatormenuitem6;
GtkWidget *quit1;
GtkWidget *close1;
GtkWidget *check_menu;
GtkWidget *properties;
GtkWidget *menuitem2;
GtkWidget *menuitem2_menu;
GtkWidget *addfile;
GtkWidget *extract_menu;
GtkWidget *delete_menu;
GtkWidget *comment_menu;
GtkWidget *view_shell_output1;
GtkWidget *prefs_menu;
GtkWidget *password_entry_menu;
GtkWidget *image1;
GtkWidget *image2;
GtkWidget *menuitem4;
GtkWidget *select_all,*deselect_all,*select_pattern;
GtkWidget *exe_menu;
GtkWidget *menuitem4_menu;
GtkWidget *about1;
GtkWidget *help1;
GtkWidget *toolbar1;
GtkWidget *toolbar2;
GtkWidget *hbox1;
GtkIconSize tmp_toolbar_icon_size;
GtkWidget *tmp_image;
GtkWidget *pad_image;
GtkWidget *New_button;
GtkWidget *Open_button;
GtkWidget *back_button;
GtkWidget *home_button;
GtkWidget *forward_button;
GtkWidget *up_button;
GtkWidget *separatortoolitem1;
GtkWidget *separatortoolitem2;
GtkWidget *separatortoolitem3;
GtkWidget *AddFile_button;
GtkWidget *Extract_button;
GtkWidget *Stop_button;
GtkWidget *toolitem1;
GtkWidget *location_label;
GtkWidget *location_entry;
GtkWidget *hpaned1;
GtkWidget *archive_dir_treeview,*scrolledwindow2;
GtkTreeStore *archive_dir_model;
GtkTreeViewColumn *column;
GtkCellRenderer *archive_dir_renderer;
GtkWidget *ebox;
GtkAccelGroup *accel_group;
GtkTooltips *tooltips;


typedef struct
{
	GtkWidget *dialog1;
	GtkTextBuffer *textbuffer;
	GtkTextIter iter;
} widget_data;

gchar *xa_create_password_dialog(gchar *);
void set_label (GtkWidget *label,gchar *);
int xa_progressbar_pulse (gpointer );
void xa_create_popup_menu();
widget_data *xa_create_output_window(gchar *);
void xa_create_main_window (GtkWidget *,gboolean,gboolean,gboolean);
GtkWidget *create_archive_properties_window();
gboolean select_matched_rows(GtkTreeModel *,GtkTreePath *,GtkTreeIter *,gpointer );
void xa_create_delete_dialog(GtkMenuItem *, gpointer );
void xa_handle_navigation_buttons (GtkMenuItem *, gpointer );
void xa_add_page (XArchive *);
void xa_page_has_changed (GtkNotebook *, GtkNotebookPage *, guint , gpointer );
void xa_close_page (GtkWidget *, gpointer );
void xa_set_button_state (gboolean,gboolean,gboolean,gboolean,gboolean, gboolean, gboolean,gboolean);
void xa_restore_navigation(int idx);
void xa_disable_delete_buttons (gboolean );
#endif

