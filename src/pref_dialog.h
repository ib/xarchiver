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

typedef struct PrefsDialog
{
	GtkWidget *dialog;
	GtkWidget *iconview;
	GtkWidget *notebook;
	GtkWidget *preferred_format;
	GtkWidget *prefer_unzip;
	GtkWidget *confirm_deletion;
	GtkWidget *sort_by_filenames;
	GtkWidget *advanced_isearch;
	GtkWidget *auto_expand;
	GtkWidget *store_output;
	GtkWidget *icon_size;
	GtkWidget *show_comment;
	GtkWidget *show_sidebar;
	GtkWidget *show_location_bar;
	GtkWidget *show_toolbar;
	GtkWidget *preferred_browser;
	GtkWidget *preferred_editor;
	GtkWidget *preferred_viewer;
	GtkWidget *preferred_archiver;
	GtkWidget *preferred_custom_cmd;
	GtkWidget *preferred_temp_dir;
	GtkWidget *preferred_extract_dir;
	GtkWidget *save_geometry;
	GtkWidget *allow_sub_dir;
	GtkListStore *liststore;
	gint main_win_geometry[5];
	gint extract_win_size[2];
	gint add_win_size[2];
	gboolean size_changed[2];
} PrefsDialog;

extern gchar *config_file;
extern GtkIconTheme *icon_theme;

PrefsDialog *xa_create_prefs_dialog();
void xa_prefs_adapt_options(PrefsDialog *);
void xa_prefs_apply_options(PrefsDialog *);
void xa_prefs_iconview_changed(GtkIconView *, PrefsDialog *);
void xa_prefs_load_options(PrefsDialog *);
void xa_prefs_save_options(PrefsDialog *, const char *);

#endif
