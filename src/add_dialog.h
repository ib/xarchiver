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
	GtkWidget *vbox2;
	GtkWidget *frame1;
	GtkWidget *frame2;
	GtkWidget *remove_files_button;
	GtkWidget *add_files_button;
	GtkWidget *files_label;
	GtkWidget *hbuttonbox1;
	GtkWidget *alignment1;
	GtkWidget *alignment2;
	GtkWidget *scrolledwindow1;
	GtkWidget *file_list_treeview;
	GtkWidget *files_frame_label;
	GtkWidget *checkbutton1;
	GtkWidget *checkbutton2;
	GtkWidget *checkbutton3;
	GtkWidget *add_image;
	GtkWidget *add_hbox;
	GtkWidget *add_label;
	GtkWidget *add_button;
	GtkWidget *cancelbutton1;
	GtkTooltips *add_option_tooltip;
	GtkWidget *add_option_label;
	GtkWidget *dialog_action_area1;
} Add_dialog_data;

Add_dialog_data *xa_create_add_dialog (XArchive *archive);
gchar *xa_parse_add_dialog_options ( XArchive *archive ,Add_dialog_data *dialog_data, GtkTreeSelection *selection);
gchar *xa_add_single_files ( XArchive *archive , GString *files, gchar *path);

#endif

