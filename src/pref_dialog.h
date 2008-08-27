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

#ifndef __PREF_DIALOG_H
#define __PREF_DIALOG_H
#include "extract_dialog.h"
#include "add_dialog.h"

typedef struct
{
	GtkWidget *dialog1,*dialog_vbox1,*combo_prefered_format;
	GtkWidget *confirm_deletion, *store_output,*combo_archive_view,*combo_icon_size;
	GtkWidget *check_show_comment, *check_sort_filename_column,*show_location_bar,*show_sidebar,*combo_prefered_viewer;
	GtkWidget *combo_prefered_web_browser, *combo_prefered_editor, *combo_prefered_temp_dir, *allow_sub_dir,*check_save_geometry,*prefs_notebook;
	GtkListStore *prefs_liststore;
	GtkTooltips *tooltips;
	gint geometry[5];
	gint extract_dialog[2];
	gint add_coords[2];
} Prefs_dialog_data;

Prefs_dialog_data *xa_create_prefs_dialog ();
void xa_prefs_iconview_changed (GtkIconView *, gpointer );
void xa_prefs_dialog_set_default_options (Prefs_dialog_data *);
void xa_prefs_save_options (Prefs_dialog_data *,const char *);
void xa_prefs_load_options(Prefs_dialog_data *);
void xa_prefs_combo_changed (GtkComboBox *,gpointer );
void xa_apply_prefs_option(Prefs_dialog_data *);
gchar *xa_prefs_choose_program(gboolean );
#endif

