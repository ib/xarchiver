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

#ifndef __ADD_DIALOG_H
#define __ADD_DIALOG_H

#include "archive.h"

typedef struct
{
	GtkWidget *dialog1;
	GtkWidget *dialog_vbox1;
	GtkWidget *vbox1;
	GtkWidget *vbox6;
	GtkWidget *vbox7;
	GtkWidget *vbox8;
	GtkWidget *hbox1;
	GtkWidget *frame5;
	GtkWidget *frame4;
	GtkWidget *remove_button;
	GtkWidget *add_files_button;
	GtkWidget *label2;
	GtkWidget *label3;
	GtkWidget *files_radio;
	GtkWidget *directories_radio;
	GSList *file_dir_radio_group;
	GtkWidget *hbuttonbox2;
	GtkWidget *alignment4;
	GtkWidget *alignment5;
	GtkWidget *scrolledwindow3;
	GtkWidget *file_list_treeview;
	GtkListStore *file_liststore;
	GtkCellRenderer *renderer;
	GtkTreeSelection *selection;
	GtkTreeViewColumn *column;
	GtkWidget *checkbutton1;
	GtkWidget *checkbutton2;
	GtkWidget *checkbutton3;
	GtkWidget *add_image;
	GtkWidget *add_hbox;
	GtkWidget *add_label;
	GtkWidget *cancel_button;
	GtkWidget *add_button;
	GtkTooltips *add_option_tooltip;
	GtkWidget *add_option_label;
	GtkWidget *dialog_action_area2;
} Add_dialog_data;

Add_dialog_data *xa_create_add_dialog (XArchive *archive);
gchar *xa_parse_add_dialog_options ( XArchive *archive ,Add_dialog_data *dialog_data, GtkTreeSelection *selection);
void xa_select_files_to_add ( GtkButton* button , gpointer _add_dialog );
void add_files_liststore (gchar *file_path, GtkListStore *liststore);
void remove_files_liststore (GtkWidget *widget, gpointer data);
void remove_foreach_func (GtkTreeModel *model, GtkTreePath *path, GtkTreeIter *iter, GList **rowref_list);

#endif

