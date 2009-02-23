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

#ifndef __ADD_DIALOG_H
#define __ADD_DIALOG_H

#include "archive.h"

typedef struct
{
	GtkWidget *dialog1;
	GtkWidget *dialog_vbox1;
	GtkWidget *alignment2;
	GtkWidget *notebook1;
	GtkWidget *filechooserwidget1;
	GtkWidget *frame1;
	GtkWidget *store_path;
	GtkWidget *no_store_path;
	GtkWidget *option_notebook_vbox;
	GtkWidget *remove_files;
	GtkWidget *update;
	GtkWidget *freshen;
	GtkWidget *solid_archive;
	GtkWidget *compression_scale;
	GtkWidget *add_password;
	GtkWidget *add_password_entry;
	GtkWidget *recurse;
	GtkWidget *add_image;
	GtkWidget *add_hbox;
	GtkWidget *add_label;
	GtkWidget *cancel_button;
	GtkWidget *add_button;
	GtkTooltips *add_option_tooltip;
	GtkWidget *add_option_label;
	GtkTooltips *option_tooltip;
	GtkObject *compression_value;
} Add_dialog_data;

Add_dialog_data *xa_create_add_dialog();
void xa_set_add_dialog_options(Add_dialog_data *,XArchive *);
void add_fresh_update_toggled_cb (GtkToggleButton *, Add_dialog_data *);
void add_update_fresh_toggled_cb (GtkToggleButton *, Add_dialog_data *);
void password_toggled_cb ( GtkButton* ,gpointer);
void xa_parse_add_dialog_options ( XArchive *, Add_dialog_data *);
void xa_select_files_to_add ( GtkButton* ,gpointer );
void xa_execute_add_commands (XArchive *archive,GSList *list,gchar *compression_string);
void fix_adjustment_value (GtkAdjustment *,gpointer );
#endif

