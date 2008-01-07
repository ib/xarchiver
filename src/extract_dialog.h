/*
 *  Copyright (C) 2006 Giuseppe Torelli - <colossus73@gmail.com>
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

#ifndef __EXTRACT_DIALOG_H
#define __EXTRACT_DIALOG_H

#include "archive.h"
#include <sys/types.h>
#include <dirent.h>

typedef struct
{
	GtkWidget *dialog1;
	GtkWidget *dialog_vbox1;
	GtkWidget *destination_path_entry;
	GtkWidget *overwrite_check;
	GtkWidget *extract_full;
	GtkWidget *touch;
	GtkWidget *fresh;
	GtkWidget *update;
	GtkWidget *all_files_radio;
	GtkWidget *files_radio;
	GtkWidget *entry2;
	GtkWidget *password_entry;
	GtkWidget *create_dir;
	GtkWidget *treeview3;
	GtkCellRenderer *renderer;
} Extract_dialog_data;

GtkWidget *label1,*label2,*label3,*label4,*label_password;
GtkWidget *hbox1,*hbox2,*hbox3,*hbox4,*vbox1,*vbox2,*vbox3,*vbox4,*vbox5;
GtkWidget *frame1,*frame2,*alignment1,*alignment2,*alignment3;
GtkWidget *radiobutton1,*scrolledwindow1;
GtkWidget *dialog_action_area1,*cancel_button,*okbutton1,*extract_button,*extract_image,*extract_hbox,*extract_label;
GtkTreeStore *model;
GtkTreeViewColumn *column;
GtkTooltips *option_tooltip;
const gchar *home_dir;
	
Extract_dialog_data *xa_create_extract_dialog (gint selected ,XArchive *archive);
void xa_create_dir_button_pressed (GtkButton *button, gpointer data);
void xa_cell_edited_canceled(GtkCellRenderer *renderer,gpointer user_data);
void xa_cell_edited (GtkCellRendererText *cell,const gchar *path_string,const gchar *new_text,gpointer data);
void xa_activate_entry(GtkToggleButton *button,gpointer data);
void fresh_update_toggled_cb (GtkToggleButton *button, Extract_dialog_data *data);
void update_fresh_toggled_cb (GtkToggleButton *button, Extract_dialog_data *data);
gchar *xa_parse_extract_dialog_options ( XArchive *archive , Extract_dialog_data *dialog_data, GtkTreeSelection *selection);
gchar *xa_extract_single_files ( XArchive *archive , GString *files, gchar *path);
gboolean xa_extract_tar_without_directories ( gchar *string, XArchive *archive,gchar *extract_path,gboolean cpio_flag);
void xa_browse_dir (GtkTreeStore *model,gchar *dir, GtkTreeIter *iter);
void xa_tree_view_row_selected(GtkTreeSelection *selection, gpointer data);
void xa_row_activated(GtkTreeView *tree_view,GtkTreePath *path,GtkTreeViewColumn *column,gpointer user_data);
void xa_expand_dir(GtkTreeView *tree_view,GtkTreeIter *iter,GtkTreePath *path,gpointer data);
#endif

