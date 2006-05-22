/*
 *  Copyright (c) 2006 Giuseppe Torelli <colossus73@gmail.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 */

#ifndef __XARCHIVER_INTERFACE_H__
#define __XARCHIVER_INTERFACE_H__

GtkWidget *prefs (gint);
GtkWidget *passwd_win ( void );
GtkWidget *view_win ( void );
GtkWidget *create_MainWindow (void);
GtkWidget *create_archive_properties_window (void);
GtkWidget *MainWindow;
GtkWidget *vbox1;
GtkWidget *vbox_body;
GtkWidget *hbox_sb;
GtkWidget *menubar1;
GtkWidget *menuitem1;
GtkWidget *menuitem1_menu;
GtkWidget *new1;
GtkWidget *open1;
GtkWidget *entry1;
GtkWidget *progressbar;
GtkWidget *info_label;
GtkWidget *viewport1,*viewport2,*viewport3;

GtkWidget *compression_data;
GtkWidget *number_of_files_data;
GtkWidget *number_of_dirs_data;
GtkWidget *content_data;
GtkWidget *size_data;
GtkWidget *modified_data;
GtkWidget *path_data;
GtkWidget *name_data;
GtkWidget *password_entry;
GtkWidget *repeat_password;
GtkWidget *separatormenuitem1;
GtkWidget *separatormenuitem2;
GtkWidget *separatormenuitem3;
GtkWidget *quit1;
GtkWidget *check_menu;
GtkWidget *properties;
GtkWidget *menuitem2;
GtkWidget *menuitem2_menu;
GtkWidget *cut1;
GtkWidget *addfile;
GtkWidget *addfolder;
GtkWidget *extract_menu;
GtkWidget *copy1;
GtkWidget *delete_menu;
GtkWidget *view_menu;
GtkWidget *view_shell_output1;
GtkWidget *add_pwd;
GtkWidget *iso_info;
GtkWidget *image1;
GtkWidget *image2;
GtkWidget *menuitem4;
GtkWidget *menuitem4_menu;
GtkWidget *about1;
GtkWidget *toolbar1;
GtkIconSize tmp_toolbar_icon_size;
GtkWidget *tmp_image;
GtkWidget *pad_image;
GtkWidget *New_button;
GtkWidget *Open_button;
GtkWidget *separatortoolitem1;
GtkWidget *AddFile_button;
GtkWidget *AddFolder_button;
GtkWidget *separatortoolitem2;
GtkWidget *Extract_button;
GtkWidget *Delete_button;
GtkWidget *View_button;
GtkWidget *Stop_button;
GtkWidget *separatortoolitem3;
GtkWidget *scrolledwindow1;
GtkWidget *treeview1;
GtkAccelGroup *accel_group;
GtkTooltips *tooltips;
GtkTooltips *pad_tooltip;
GtkWidget *ebox;

void set_label (GtkWidget *label,gchar *text);
int xa_progressbar_pulse (gpointer data);

#endif

