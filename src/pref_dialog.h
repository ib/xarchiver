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

#ifndef XARCHIVER_PREF_DIALOG_H
#define XARCHIVER_PREF_DIALOG_H

#include <gtk/gtk.h>

typedef struct Prefs_dialog_data
{
	GtkWidget *dialog, *preferred_format, *prefer_unzip;
	GtkWidget *confirm_deletion, *store_output, *icon_size;
	GtkWidget *show_comment, *sort_by_filenames, *advanced_isearch, *auto_expand, *show_location_bar, *show_sidebar, *show_toolbar, *preferred_viewer, *preferred_archiver, *preferred_custom_cmd;
	GtkWidget *preferred_browser, *preferred_editor, *preferred_temp_dir, *preferred_extract_dir, *allow_sub_dir, *save_geometry, *notebook;
	GtkListStore *liststore;
	GtkWidget *iconview;
	gint geometry[5];
	gint extract_dialog[2];
	gint add_coords[2];
	gboolean size_changed[2];
} Prefs_dialog_data;

extern gchar *config_file;
extern GtkIconTheme *icon_theme;

Prefs_dialog_data *xa_create_prefs_dialog();
void xa_prefs_adapt_options(Prefs_dialog_data *);
void xa_prefs_apply_options(Prefs_dialog_data *);
void xa_prefs_iconview_changed(GtkIconView *, Prefs_dialog_data *);
void xa_prefs_load_options(Prefs_dialog_data *);
void xa_prefs_save_options(Prefs_dialog_data *, const char *);

#endif
