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

typedef struct
{
	GtkWidget *dialog1;
	GtkWidget *dialog_vbox1;
	GtkWidget *vbox1;
	GtkWidget *vbox2;
	GtkWidget *hbox3;
	GtkWidget *extract_to_label;
	GtkWidget *destination_path_entry;
	GtkWidget *button1;
	GtkWidget *image1;
	GtkWidget *hbox4;
	GtkWidget *frame1;
	GtkWidget *alignment1;
	GtkWidget *alignment2;
	GtkWidget *alignment3;
	GtkWidget *vbox3;
	GtkWidget *all_files_radio;
	GtkWidget *selected_files_radio;
	GSList *radio_group;
	GtkWidget *files_frame_label;
	GtkWidget *frame2;
	GtkWidget *vbox4;
	GtkWidget *overwrite_check;
	GtkWidget *extract_full;
	GtkWidget *touch;
	GtkWidget *fresh;
	GtkWidget *update;
	GtkWidget *hbox5;
	GtkWidget *hbox6;
	GtkWidget *label_password;
	GtkWidget *password_entry;
	GtkWidget *options_frame_label;
	GtkWidget *dialog_action_area1;
	GtkWidget *cancel_button;
	GtkWidget *extract_button;
	GtkWidget *extract_image;
	GtkWidget *extract_label;
	GtkWidget *extract_hbox;
	GtkTooltips *option_tooltip;
} Extract_dialog_data;

Extract_dialog_data *xa_create_extract_dialog (gint selected ,XArchive *archive);
void fresh_update_toggled_cb (GtkToggleButton *button, Extract_dialog_data *data);
void update_fresh_toggled_cb (GtkToggleButton *button, Extract_dialog_data *data);
gchar *xa_parse_extract_dialog_options ( XArchive *archive , Extract_dialog_data *dialog_data, GtkTreeSelection *selection);
gchar *xa_extract_single_files ( XArchive *archive , GString *files, gchar *path);
gboolean xa_extract_tar_without_directories ( gchar *string, gchar *escaped_path, gboolean overwrite, gboolean tar_touch, gchar *extract_path );
#endif

