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

#ifndef __XARCHIVER_INTERFACE_H__
#define __XARCHIVER_INTERFACE_H__
#include "archive.h"
#include "pref_dialog.h"

typedef struct _Progress_bar_data Progress_bar_data;

struct _Progress_bar_data
{
	GtkWidget *progress_window;
	GtkWidget *progressbar1;
	GtkWidget *archive_label;
	GtkWidget *file_label;
	gboolean multi_extract;
};

GtkNotebook *notebook;
GtkIconSize tmp_toolbar_icon_size;
GtkTreeStore *archive_dir_model;
GtkTreeViewColumn *column;
GtkCellRenderer *archive_dir_renderer;
GtkAccelGroup *accel_group;
GtkTooltips *tooltips;
GtkIconTheme *icon_theme;

GtkWidget *xa_popup_menu, *xa_main_window, *vbox1, *hbox_sb, *menubar1, *menuitem1, *menuitem1_menu, *new1, *open1, *listing, *listing_submenu,
*listing_text, *listing_html, *save1, *entry1, *green_led, *red_led, *progressbar, *total_label,*selected_label, *total_frame,*selected_frame,
*archive_properties_window, *table1, *path_label, *modified_label, *size_label, *content_label, *comment_label, *compression_label, *number_of_files_label,
*name_label, *type_label, *compression_data, *encrypted_data, *encrypted_label,*number_of_files_data, *content_data, *comment_data, *size_data, *modified_data, *path_data, *type_data,
*name_data, *separatormenuitem1, *separatormenuitem2, *separatormenuitem3, *separatormenuitem4, *separatormenuitem5, *separatormenuitem6, *quit1,
*close1, *check_menu, *properties, *menuitem2, *menuitem2_menu, *addfile, *extract_menu, *delete_menu, *comment_menu, *multi_extract_menu,*view_shell_output1,
*prefs_menu,*password_entry_menu, *image1, *image2, *menuitem4, *donate,*select_all,*deselect_all,*select_pattern, *exe_menu, *menuitem4_menu,
*about1, *help1,*toolbar1, *toolbar2, *hbox1, *tmp_image, *pad_image, *New_button, *Open_button, *back_button, *home_button, *forward_button,
*up_button,*separatortoolitem1,*separatortoolitem2,*separatortoolitem3,*AddFile_button,*Extract_button,*Stop_button,*toolitem1,*location_label,
*location_entry,*hpaned1,*archive_dir_treeview,*scrolledwindow2,*ddelete,*rename_menu,*rrename,*cut,*copy,*paste,*view,*open_popupmenu;

gchar *xa_create_password_dialog(XArchive *);
void set_label (GtkWidget *label,gchar *);
gboolean xa_flash_led_indicator (XArchive *);
gboolean xa_pulse_progress_bar_window (Progress_bar_data *);
void xa_show_donate_page_on_the_web(GtkMenuItem *,gpointer );
void xa_create_popup_menu();
void xa_create_main_window (GtkWidget *,gboolean,gboolean,gboolean);
GtkWidget *xa_create_archive_properties_window();
gboolean select_matched_rows(GtkTreeModel *,GtkTreePath *,GtkTreeIter *,gpointer );
void xa_select_by_pattern_dialog(GtkMenuItem *, gpointer );
void xa_handle_navigation_buttons (GtkMenuItem *, gpointer );
void xa_add_page (XArchive *);
void xa_page_has_changed (GtkNotebook *, GtkNotebookPage *, guint ,gpointer );
void xa_close_page ( GtkWidget*, gpointer );
void xa_set_button_state (gboolean,gboolean,gboolean,gboolean,gboolean,gboolean, gboolean, gboolean,gboolean,gboolean,gboolean);
void xa_restore_navigation(int idx);
void xa_disable_delete_buttons (gboolean);
void xa_sidepane_row_expanded(GtkTreeView *,GtkTreeIter *,GtkTreePath *,gpointer );
void xa_sidepane_drag_data_received ( GtkWidget*,GdkDragContext *, int x, int y, GtkSelectionData *, unsigned int, unsigned int, gpointer );
gboolean xa_sidepane_drag_motion_expand_timeout (gpointer );
gboolean xa_sidepane_drag_motion ( GtkWidget*,GdkDragContext *,gint x,gint y,guint ,gpointer );
Progress_bar_data *xa_create_progress_bar(gboolean,XArchive *);
void xa_increase_progress_bar(Progress_bar_data *,gchar *,double);
//void xa_icon_theme_changed (GtkIconTheme *,gpointer );
#endif
