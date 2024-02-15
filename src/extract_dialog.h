/*
 *  Copyright (C) 2008 Giuseppe Torelli - <colossus73@gmail.com>
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

#ifndef XARCHIVER_EXTRACT_DIALOG_H
#define XARCHIVER_EXTRACT_DIALOG_H

#include <gtk/gtk.h>
#include "archive.h"

typedef struct Extract_dialog_data
{
	GtkWidget *dialog, *destination_path_entry, *ensure_directory, *overwrite, *touch, *freshen, *update, *all_files, *specified_files, *selected_files, *specified_files_entry, *full_path, *relative_path, *without_path, *password_entry;
	XArchive *archive;
} Extract_dialog_data;

typedef struct Multi_extract_data
{
	GtkWidget *multi_extract, *files_treeview, *hbox2, *extract_to, *entry1, *extract_to_archive_name, *label1, *frame2, *alignment2, *vbox3, *overwrite, *full_path, *label2,
	*extract_button,*extract_image,*extract_hbox,*extract_label,*image1;
	GtkListStore *files_liststore;
	gint nr;
	gboolean stop_pressed;
	XArchive *archive;
} Multi_extract_data;

Extract_dialog_data *xa_create_extract_dialog();
Multi_extract_data *xa_create_multi_extract_dialog();
void xa_execute_extract_commands(XArchive *, GSList *, gboolean);
void xa_multi_extract_dialog(Multi_extract_data *);
void xa_multi_extract_dialog_add_file(gchar *, Multi_extract_data *);
void xa_parse_extract_dialog_options(XArchive * , Extract_dialog_data *, GtkTreeSelection *);
void xa_set_extract_dialog_options(Extract_dialog_data *, gint, XArchive *);

#endif
