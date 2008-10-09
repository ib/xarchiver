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

#ifndef __EXTRACT_DIALOG_H
#define __EXTRACT_DIALOG_H

#include "archive.h"
#include <sys/types.h>
#include <dirent.h>

typedef struct
{
	GtkWidget *dialog1,*dialog_vbox1,*destination_path_entry,*overwrite_check,*extract_full,*touch,*fresh,*update,*all_files_radio,*files_radio,
	 *selected_radio,*entry2,*password_entry;
	gchar *string;
} Extract_dialog_data;

typedef struct
{
	GtkWidget *multi_extract,*files_treeview,*hbox2,*extract_to,*entry1,*extract_to_archive_name,*label1,*frame2,*alignment2,*vbox3,*overwrite,*full_path,*label2,*dialog_action_area1,
	*extract_button,*extract_image,*extract_hbox,*extract_label,*image1;
	GtkListStore *files_liststore;
	gint nr;
	gboolean stop_pressed;
	XArchive *archive;
} Multi_extract_data;

GtkWidget *label1,*label2,*label3,*label_password;
GtkWidget *hbox1,*hbox2,*hbox3,*vbox1,*vbox2,*vbox3,*vbox5;
GtkWidget *frame1,*frame2,*alignment1,*alignment2,*alignment3;
GtkWidget *dialog_action_area1,*cancel_button,*okbutton1,*extract_button,*extract_image,*extract_hbox,*extract_label;
GtkTooltips *option_tooltip;
	
Extract_dialog_data *xa_create_extract_dialog();
void xa_activate_entry(GtkToggleButton *,gpointer );
void fresh_update_toggled_cb (GtkToggleButton *, Extract_dialog_data *);
void update_fresh_toggled_cb (GtkToggleButton *, Extract_dialog_data *);
void xa_set_extract_dialog_options(Extract_dialog_data *,gint ,XArchive *);
void xa_parse_extract_dialog_options (XArchive * , Extract_dialog_data *, GtkTreeSelection *);
Multi_extract_data *xa_create_multi_extract_dialog();
void xa_activate_remove_button (GtkTreeModel *, GtkTreePath *, GtkTreeIter *, GtkWidget *);
void xa_add_files_liststore (gchar *, Multi_extract_data *);
void xa_parse_multi_extract_archive(Multi_extract_data *);
#endif

