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

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>

#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>

#include "string_utils.h"
#include "window.h"
#include "interface.h"
#include "support.h"

static const GtkTargetEntry drag_targets[] =
{
  { "XdndDirectSave0",0,0 },
};

static const GtkTargetEntry drop_targets[] =
{
  { "text/uri-list",0,0 },
};

extern gboolean unrar;

void xa_create_main_window (GtkWidget *xa_main_window,gboolean show_location)
{
	GdkPixbuf *icon;

	xa_create_popup_menu();
	tooltips = gtk_tooltips_new ();
	accel_group = gtk_accel_group_new ();
	xa_set_window_title (xa_main_window , NULL);

	icon_theme = gtk_icon_theme_get_default();
	icon = gtk_icon_theme_load_icon(icon_theme, "xarchiver", 24, 0, NULL);
	gtk_window_set_icon (GTK_WINDOW(xa_main_window),icon);
	g_signal_connect (G_OBJECT (xa_main_window), "delete-event", G_CALLBACK (xa_quit_application), NULL);

	/* Create the menus */
	vbox1 = gtk_vbox_new (FALSE, 0);
	gtk_widget_show (vbox1);
	gtk_container_add (GTK_CONTAINER (xa_main_window), vbox1);

	menubar1 = gtk_menu_bar_new ();
	gtk_widget_show (menubar1);
	gtk_box_pack_start (GTK_BOX (vbox1), menubar1, FALSE, FALSE, 0);

	menuitem1 = gtk_menu_item_new_with_mnemonic (_("_Archive"));
	gtk_widget_show (menuitem1);
	gtk_container_add (GTK_CONTAINER (menubar1), menuitem1);

	menuitem1_menu = gtk_menu_new ();
	gtk_menu_item_set_submenu (GTK_MENU_ITEM (menuitem1), menuitem1_menu);

	new1 = gtk_image_menu_item_new_from_stock ("gtk-new", accel_group);
	gtk_widget_show (new1);
	gtk_container_add (GTK_CONTAINER (menuitem1_menu), new1);

	open1 = gtk_image_menu_item_new_from_stock ("gtk-open", accel_group);
	gtk_widget_show (open1);
	gtk_container_add (GTK_CONTAINER (menuitem1_menu), open1);

	separatormenuitem1 = gtk_separator_menu_item_new ();
	gtk_widget_show (separatormenuitem1);
	gtk_container_add (GTK_CONTAINER (menuitem1_menu), separatormenuitem1);
	gtk_widget_set_sensitive (separatormenuitem1, FALSE);

	check_menu = gtk_image_menu_item_new_with_mnemonic (_("_Test"));
	gtk_widget_show (check_menu);
	gtk_widget_set_sensitive ( check_menu , FALSE);
	gtk_container_add (GTK_CONTAINER (menuitem1_menu), check_menu);
	gtk_widget_add_accelerator (check_menu, "activate",accel_group,GDK_t, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);

	tmp_image = gtk_image_new_from_stock ("gtk-index", GTK_ICON_SIZE_MENU);
	gtk_widget_show (tmp_image);
	gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (check_menu), tmp_image);

	properties = gtk_image_menu_item_new_with_mnemonic (_("_Properties"));
	gtk_widget_show (properties);
	gtk_widget_set_sensitive ( properties , FALSE);
	gtk_container_add (GTK_CONTAINER (menuitem1_menu), properties);
	gtk_widget_add_accelerator (properties, "activate",accel_group,GDK_p, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);

	tmp_image = gtk_image_new_from_stock ("gtk-properties", GTK_ICON_SIZE_MENU);
	gtk_widget_show (tmp_image);
	gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (properties), tmp_image);

	close1 = gtk_image_menu_item_new_from_stock ("gtk-close", accel_group);
	gtk_widget_set_sensitive (close1,FALSE);
	gtk_widget_show (close1);
	gtk_container_add (GTK_CONTAINER (menuitem1_menu), close1);

	separatormenuitem2 = gtk_separator_menu_item_new ();
	gtk_widget_show (separatormenuitem2);
	gtk_container_add (GTK_CONTAINER (menuitem1_menu), separatormenuitem2);
	gtk_widget_set_sensitive (separatormenuitem2, FALSE);

	quit1 = gtk_image_menu_item_new_from_stock ("gtk-quit", accel_group);
	gtk_widget_show (quit1);
	gtk_container_add (GTK_CONTAINER (menuitem1_menu), quit1);

	menuitem2 = gtk_menu_item_new_with_mnemonic (_("_Action"));
	gtk_widget_show (menuitem2);
	gtk_container_add (GTK_CONTAINER (menubar1), menuitem2);

	menuitem2_menu = gtk_menu_new ();
	gtk_menu_item_set_submenu (GTK_MENU_ITEM (menuitem2), menuitem2_menu);

	addfile = gtk_image_menu_item_new_with_mnemonic (_("Add"));
	gtk_widget_set_sensitive (addfile,FALSE);
	gtk_widget_show (addfile);
	gtk_container_add (GTK_CONTAINER (menuitem2_menu), addfile);
	gtk_widget_add_accelerator (addfile, "activate",accel_group,GDK_c, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);

	image2 = xa_main_window_find_image ("xarchiver-add.png", GTK_ICON_SIZE_MENU);
	gtk_widget_show (image2);
	gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (addfile), image2);

	extract_menu = gtk_image_menu_item_new_with_mnemonic (_("_Extract"));
	gtk_widget_set_sensitive (extract_menu,FALSE);
	gtk_widget_show (extract_menu);
	gtk_container_add (GTK_CONTAINER (menuitem2_menu), extract_menu);
	gtk_widget_add_accelerator (extract_menu, "activate",accel_group,GDK_e, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);

	image2 =  xa_main_window_find_image ("xarchiver-extract.png", GTK_ICON_SIZE_MENU);
	gtk_widget_show (image2);
	gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (extract_menu), image2);

	delete_menu = gtk_image_menu_item_new_from_stock ("gtk-delete", accel_group);
	gtk_widget_set_sensitive (delete_menu,FALSE);
	gtk_widget_show (delete_menu);
	gtk_container_add (GTK_CONTAINER (menuitem2_menu), delete_menu);
	gtk_widget_add_accelerator (delete_menu, "activate",accel_group,GDK_d, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);

	view_menu = gtk_image_menu_item_new_with_mnemonic (_("_View"));
	gtk_widget_set_sensitive (view_menu, FALSE);
	gtk_widget_show (view_menu);
	gtk_container_add (GTK_CONTAINER (menuitem2_menu), view_menu);
	gtk_widget_add_accelerator (view_menu, "activate",accel_group,GDK_v, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);

	image2 = gtk_image_new_from_stock ("gtk-find", GTK_ICON_SIZE_MENU);
	gtk_widget_show (image2);
	gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (view_menu), image2);

	separatormenuitem3 = gtk_separator_menu_item_new ();
	gtk_widget_show (separatormenuitem3);
	gtk_container_add (GTK_CONTAINER (menuitem2_menu), separatormenuitem3);
	gtk_widget_set_sensitive (separatormenuitem3, FALSE);

	exe_menu = gtk_image_menu_item_new_with_mnemonic (_("Make SF_X"));
	gtk_widget_set_sensitive (exe_menu,FALSE);
	gtk_widget_show (exe_menu);
	gtk_container_add (GTK_CONTAINER (menuitem2_menu), exe_menu);
	gtk_widget_add_accelerator (exe_menu, "activate",accel_group,GDK_x, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);

	image2 = gtk_image_new_from_stock ("gtk-execute", GTK_ICON_SIZE_MENU);
	gtk_widget_show (image2);
	gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (exe_menu), image2);

	comment_menu = gtk_image_menu_item_new_with_mnemonic (_("_Show comment"));
	gtk_widget_set_sensitive (comment_menu, FALSE);
	gtk_widget_show (comment_menu);
	gtk_container_add (GTK_CONTAINER (menuitem2_menu), comment_menu);
	gtk_widget_add_accelerator (comment_menu, "activate",accel_group,GDK_s, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);

	tmp_image = gtk_image_new_from_stock ("gtk-justify-fill", GTK_ICON_SIZE_MENU);
	gtk_widget_show (tmp_image);
	gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (comment_menu), tmp_image);

	separatormenuitem4 = gtk_separator_menu_item_new ();
	gtk_widget_show (separatormenuitem4);
	gtk_container_add (GTK_CONTAINER (menuitem2_menu), separatormenuitem4);
	gtk_widget_set_sensitive (separatormenuitem4, FALSE);

	select_all = gtk_image_menu_item_new_with_mnemonic (_("Sele_ct All"));
	gtk_widget_show (select_all);
	gtk_container_add (GTK_CONTAINER (menuitem2_menu), select_all);
	gtk_widget_set_sensitive (select_all, FALSE);
	gtk_widget_add_accelerator (select_all, "activate",accel_group,GDK_a, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
	
	tmp_image = gtk_image_new_from_stock ("gtk-select-all", GTK_ICON_SIZE_MENU);
	gtk_widget_show (tmp_image);
	gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (select_all), tmp_image);

	deselect_all = gtk_image_menu_item_new_with_mnemonic (_("Dese_lect All"));
	gtk_widget_show (deselect_all);
	gtk_container_add (GTK_CONTAINER (menuitem2_menu), deselect_all);
	gtk_widget_set_sensitive (deselect_all, FALSE);
	gtk_widget_add_accelerator (deselect_all, "activate",accel_group,GDK_l, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);

	select_pattern = gtk_image_menu_item_new_with_mnemonic (_("Select _by pattern"));
	gtk_widget_show (select_pattern);
	gtk_container_add (GTK_CONTAINER (menuitem2_menu), select_pattern);
	gtk_widget_add_accelerator (select_pattern, "activate",accel_group,GDK_b, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);

	separatormenuitem5 = gtk_separator_menu_item_new ();
	gtk_widget_show (separatormenuitem5);
	gtk_container_add (GTK_CONTAINER (menuitem2_menu), separatormenuitem5);
	gtk_widget_set_sensitive (separatormenuitem5, FALSE);

	view_shell_output1 = gtk_image_menu_item_new_with_mnemonic (_("C_md-line output"));
	gtk_container_add (GTK_CONTAINER (menuitem2_menu), view_shell_output1);
	gtk_widget_add_accelerator (view_shell_output1, "activate",accel_group,GDK_m, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
	gtk_widget_set_sensitive (view_shell_output1,FALSE);
	gtk_widget_show (view_shell_output1);

	image2 = gtk_image_new_from_stock ("gtk-find-and-replace", GTK_ICON_SIZE_MENU);
	gtk_widget_show (image2);
	gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (view_shell_output1), image2);

	password_entry = gtk_image_menu_item_new_with_mnemonic (_("Reset passwo_rd"));
	gtk_widget_show (password_entry);
	gtk_widget_set_sensitive ( password_entry , FALSE );
	gtk_container_add (GTK_CONTAINER (menuitem2_menu), password_entry);
	gtk_widget_add_accelerator (password_entry, "activate",accel_group,GDK_r, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);

	separatormenuitem6 = gtk_separator_menu_item_new ();
	gtk_widget_show (separatormenuitem6);
	gtk_container_add (GTK_CONTAINER (menuitem2_menu), separatormenuitem6);
	gtk_widget_set_sensitive (separatormenuitem6, FALSE);

	prefs_menu = gtk_image_menu_item_new_with_mnemonic (_("_Preferences"));
	gtk_widget_show (prefs_menu);
	gtk_container_add (GTK_CONTAINER (menuitem2_menu), prefs_menu);
	gtk_widget_add_accelerator (prefs_menu, "activate",accel_group,GDK_f, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);

	tmp_image = gtk_image_new_from_stock ("gtk-preferences", GTK_ICON_SIZE_MENU);
	gtk_widget_show (tmp_image);
	gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (prefs_menu), tmp_image);

	menuitem4 = gtk_menu_item_new_with_mnemonic (_("_Help"));
	gtk_widget_show (menuitem4);
	gtk_container_add (GTK_CONTAINER (menubar1), menuitem4);

	menuitem4_menu = gtk_menu_new ();
	gtk_menu_item_set_submenu (GTK_MENU_ITEM (menuitem4), menuitem4_menu);

	help1 = gtk_image_menu_item_new_from_stock ("gtk-help", accel_group);
	gtk_widget_show (help1);
	gtk_container_add (GTK_CONTAINER (menuitem4_menu), help1);

	about1 = gtk_image_menu_item_new_from_stock ("gtk-about", accel_group);
	gtk_widget_show (about1);
	gtk_container_add (GTK_CONTAINER (menuitem4_menu), about1);

	/* Create the toolbar */
	toolbar1 = gtk_toolbar_new ();
	gtk_widget_show (toolbar1);
	gtk_box_pack_start (GTK_BOX (vbox1), toolbar1, FALSE, FALSE, 0);
	gtk_toolbar_set_style (GTK_TOOLBAR (toolbar1), GTK_TOOLBAR_ICONS);
	tmp_toolbar_icon_size = gtk_toolbar_get_icon_size (GTK_TOOLBAR (toolbar1));

	tmp_image = gtk_image_new_from_stock ("gtk-new", tmp_toolbar_icon_size);
	gtk_widget_show (tmp_image);
	New_button = (GtkWidget*) gtk_tool_button_new (tmp_image, _("New"));
	gtk_widget_show (New_button);
	gtk_tool_item_set_homogeneous (GTK_TOOL_ITEM (New_button), FALSE);
	gtk_container_add (GTK_CONTAINER (toolbar1), New_button);
	gtk_tool_item_set_tooltip (GTK_TOOL_ITEM (New_button), tooltips, _("Create a new archive"), NULL);

	tmp_image = gtk_image_new_from_stock ("gtk-open", tmp_toolbar_icon_size);
	gtk_widget_show (tmp_image);
	Open_button = (GtkWidget*) gtk_tool_button_new (tmp_image, _("Open"));
	gtk_widget_show (Open_button);
	gtk_tool_item_set_homogeneous (GTK_TOOL_ITEM (Open_button), FALSE);
	gtk_container_add (GTK_CONTAINER (toolbar1), Open_button);
	gtk_tool_item_set_tooltip (GTK_TOOL_ITEM (Open_button), tooltips, _("Open an archive"), NULL);

	separatortoolitem1 = (GtkWidget*) gtk_separator_tool_item_new ();
	gtk_widget_show (separatortoolitem1);
	gtk_container_add (GTK_CONTAINER (toolbar1), separatortoolitem1);

	tmp_image = gtk_image_new_from_stock ("gtk-go-back", tmp_toolbar_icon_size);
	gtk_widget_show (tmp_image);
	back_button = (GtkWidget*) gtk_tool_button_new (tmp_image, _("Back"));
	gtk_widget_set_sensitive(back_button,FALSE);
	gtk_widget_show (back_button);
	gtk_tool_item_set_homogeneous (GTK_TOOL_ITEM (back_button), FALSE);
	gtk_container_add (GTK_CONTAINER (toolbar1), back_button);
	gtk_tool_item_set_tooltip (GTK_TOOL_ITEM (back_button), tooltips, _("Back"), NULL);

	tmp_image = gtk_image_new_from_stock ("gtk-go-up", tmp_toolbar_icon_size);
	gtk_widget_show (tmp_image);
	up_button = (GtkWidget*) gtk_tool_button_new (tmp_image, _("Up"));
	gtk_widget_set_sensitive(up_button,FALSE);
	gtk_widget_show (up_button);
	gtk_tool_item_set_homogeneous (GTK_TOOL_ITEM (up_button), FALSE);
	gtk_container_add (GTK_CONTAINER (toolbar1), up_button);
	gtk_tool_item_set_tooltip (GTK_TOOL_ITEM (up_button), tooltips, _("Up"), NULL);

	tmp_image = gtk_image_new_from_stock ("gtk-go-forward", tmp_toolbar_icon_size);
	gtk_widget_show (tmp_image);
	forward_button = (GtkWidget*) gtk_tool_button_new (tmp_image, _("Forward"));
	gtk_widget_set_sensitive(forward_button,FALSE);
	gtk_widget_show (forward_button);
	gtk_tool_item_set_homogeneous (GTK_TOOL_ITEM (forward_button), FALSE);
	gtk_container_add (GTK_CONTAINER (toolbar1), forward_button);
	gtk_tool_item_set_tooltip (GTK_TOOL_ITEM (forward_button), tooltips, _("Forward"), NULL);

	tmp_image = gtk_image_new_from_stock ("gtk-home", tmp_toolbar_icon_size);
	gtk_widget_show (tmp_image);
	home_button = (GtkWidget*) gtk_tool_button_new (tmp_image, _("Home"));
	gtk_widget_set_sensitive(home_button,FALSE);
	gtk_widget_show (home_button);
	gtk_tool_item_set_homogeneous (GTK_TOOL_ITEM (home_button), FALSE);
	gtk_container_add (GTK_CONTAINER (toolbar1), home_button);
	gtk_tool_item_set_tooltip (GTK_TOOL_ITEM (home_button), tooltips, _("Root"), NULL);

	separatortoolitem3 = (GtkWidget*) gtk_separator_tool_item_new ();
	gtk_widget_show (separatortoolitem3);
	gtk_container_add (GTK_CONTAINER (toolbar1), separatortoolitem3);

	tmp_image = xa_main_window_find_image("xarchiver-add.png", GTK_ICON_SIZE_LARGE_TOOLBAR);
	gtk_widget_show (tmp_image);
	AddFile_button = (GtkWidget*) gtk_tool_button_new (tmp_image, _("Add"));
	gtk_widget_set_sensitive (AddFile_button,FALSE);
	gtk_widget_show (AddFile_button);
	gtk_tool_item_set_homogeneous (GTK_TOOL_ITEM (AddFile_button), FALSE);
	gtk_container_add (GTK_CONTAINER (toolbar1), AddFile_button);
	gtk_tool_item_set_tooltip (GTK_TOOL_ITEM (AddFile_button), tooltips, _("Add files"), NULL);

	tmp_image = xa_main_window_find_image("xarchiver-extract.png", GTK_ICON_SIZE_LARGE_TOOLBAR);
	gtk_widget_show (tmp_image);
	Extract_button = (GtkWidget*) gtk_tool_button_new (tmp_image, _("Extract"));
	gtk_widget_set_sensitive (Extract_button,FALSE);
	gtk_widget_show (Extract_button);
	gtk_tool_item_set_homogeneous (GTK_TOOL_ITEM (Extract_button), FALSE);
	gtk_container_add (GTK_CONTAINER (toolbar1), Extract_button);
	gtk_tool_item_set_tooltip (GTK_TOOL_ITEM (Extract_button), tooltips, _("Extract files"), NULL);

	tmp_image = gtk_image_new_from_stock ("gtk-find", tmp_toolbar_icon_size);
	gtk_widget_show (tmp_image);
	View_button = (GtkWidget*) gtk_tool_button_new (tmp_image, _("View"));
	gtk_widget_show (View_button);
	gtk_widget_set_sensitive (View_button,FALSE);
	gtk_tool_item_set_homogeneous (GTK_TOOL_ITEM (View_button), FALSE);
	gtk_container_add (GTK_CONTAINER (toolbar1), View_button);
	gtk_tool_item_set_tooltip (GTK_TOOL_ITEM (View_button), tooltips, _("View file with an external editor/viewer"), NULL);

	separatortoolitem2 = (GtkWidget*) gtk_separator_tool_item_new ();
	gtk_widget_show (separatortoolitem2);
	gtk_container_add (GTK_CONTAINER (toolbar1), separatortoolitem2);

	tmp_image = gtk_image_new_from_stock ("gtk-stop", tmp_toolbar_icon_size);
	gtk_widget_show (tmp_image);
	Stop_button = (GtkWidget*) gtk_tool_button_new (tmp_image, _("Stop"));
	gtk_widget_set_sensitive (Stop_button,FALSE);
	gtk_widget_show (Stop_button);
	gtk_tool_item_set_homogeneous (GTK_TOOL_ITEM ( Stop_button ), FALSE);
	gtk_container_add (GTK_CONTAINER (toolbar1), Stop_button);
	gtk_tool_item_set_tooltip (GTK_TOOL_ITEM (Stop_button), tooltips, _("Cancel current operation"), NULL);

	/* Location entry */
	toolbar2 = gtk_toolbar_new ();
	gtk_box_pack_start (GTK_BOX (vbox1), toolbar2, FALSE, FALSE, 0);

	toolitem1 = (GtkWidget*)gtk_tool_item_new();
	gtk_tool_item_set_expand(GTK_TOOL_ITEM(toolitem1), TRUE);
  	gtk_container_add (GTK_CONTAINER (toolbar2), toolitem1);

	hbox1 = gtk_hbox_new(FALSE,2);
	gtk_container_add(GTK_CONTAINER (toolitem1),hbox1);

	location_label = gtk_label_new(_("Location:"));
	gtk_box_pack_start (GTK_BOX (hbox1), location_label, FALSE, FALSE, 1);

	location_entry = gtk_entry_new();
	gtk_box_pack_start (GTK_BOX (hbox1), location_entry, TRUE, TRUE, 0);
	g_signal_connect (G_OBJECT (location_entry), "activate",	G_CALLBACK (xa_location_entry_activated), NULL);
	if (show_location)
		gtk_widget_show_all(toolbar2);

	/* Create the notebook widget */
	notebook = GTK_NOTEBOOK(gtk_notebook_new() );
	gtk_box_pack_start (GTK_BOX(vbox1), GTK_WIDGET(notebook),TRUE,TRUE,0);
	gtk_notebook_set_tab_pos (notebook, GTK_POS_TOP);
	gtk_notebook_set_scrollable (notebook,TRUE);
	gtk_notebook_popup_enable (notebook);
	gtk_widget_show (GTK_WIDGET(notebook));
	g_signal_connect ((gpointer) notebook, "switch-page",G_CALLBACK (xa_page_has_changed),NULL);

	gtk_drag_dest_set (GTK_WIDGET(notebook),GTK_DEST_DEFAULT_ALL,drop_targets,1,GDK_ACTION_COPY | GDK_ACTION_MOVE | GDK_ACTION_LINK | GDK_ACTION_ASK);
	g_signal_connect (G_OBJECT (notebook), "drag-data-received",G_CALLBACK (on_drag_data_received), NULL);

  	hbox_sb = gtk_hbox_new (FALSE, 0);
	gtk_widget_show (hbox_sb);
	gtk_box_pack_end (GTK_BOX (vbox1), hbox_sb, FALSE, TRUE, 0);

	viewport1 = gtk_viewport_new (NULL, NULL);
	gtk_widget_show (viewport1);
	gtk_box_pack_start (GTK_BOX (hbox_sb), viewport1, TRUE, TRUE, 0);

	info_label = gtk_label_new (NULL);
	gtk_misc_set_alignment (GTK_MISC(info_label), 0.0, 0.5);
	gtk_widget_show (info_label);
	gtk_container_add (GTK_CONTAINER (viewport1), info_label);

	viewport2 = gtk_viewport_new (NULL, NULL);
	gtk_box_pack_start (GTK_BOX (hbox_sb), viewport2, TRUE, TRUE, 0);

	progressbar = gtk_progress_bar_new ();
	gtk_widget_show (progressbar);
	gtk_widget_set_size_request(progressbar, 80, 1);
	gtk_container_add (GTK_CONTAINER (viewport2), progressbar);

	g_signal_connect ((gpointer) new1, "activate", G_CALLBACK (xa_new_archive), NULL);
	g_signal_connect ((gpointer) open1, "activate", G_CALLBACK (xa_open_archive), NULL);
	g_signal_connect ((gpointer) check_menu, "activate", G_CALLBACK (xa_test_archive), NULL);
	g_signal_connect ((gpointer) properties, "activate", G_CALLBACK (xa_archive_properties), NULL);
	g_signal_connect ((gpointer) extract_menu, "activate", G_CALLBACK (xa_extract_archive), NULL);
	g_signal_connect ((gpointer) exe_menu, "activate", G_CALLBACK (xa_convert_sfx), NULL);
	g_signal_connect ((gpointer) addfile, "activate", G_CALLBACK (xa_add_files_archive), NULL);
	g_signal_connect ((gpointer) view_shell_output1, "activate", G_CALLBACK (xa_show_cmd_line_output), NULL);
	g_signal_connect ((gpointer) select_all, "activate", G_CALLBACK (xa_select_all), NULL);
	g_signal_connect ((gpointer) deselect_all, "activate", G_CALLBACK (xa_deselect_all), NULL);
	g_signal_connect ((gpointer) select_pattern, "activate", G_CALLBACK (xa_create_delete_dialog), NULL);
	g_signal_connect ((gpointer) password_entry, "activate", G_CALLBACK (xa_reset_password), NULL);
	g_signal_connect ((gpointer) prefs_menu, "activate", G_CALLBACK (xa_show_prefs_dialog), NULL);
	g_signal_connect ((gpointer) close1, "activate", G_CALLBACK (xa_close_archive), NULL);
	g_signal_connect ((gpointer) quit1, "activate", G_CALLBACK (xa_quit_application), NULL);
	g_signal_connect ((gpointer) delete_menu, "activate", G_CALLBACK (xa_delete_archive), NULL);
	g_signal_connect ((gpointer) view_menu, "activate", G_CALLBACK (xa_view_file_inside_archive), NULL);
	g_signal_connect ((gpointer) comment_menu, "activate", G_CALLBACK (xa_show_archive_comment), NULL);
	g_signal_connect ((gpointer) help1, "activate", G_CALLBACK (xa_show_help), NULL);
	g_signal_connect ((gpointer) about1, "activate", G_CALLBACK (xa_about), NULL);

	g_signal_connect ((gpointer) New_button,	"clicked", G_CALLBACK (xa_new_archive), NULL);
	g_signal_connect ((gpointer) Open_button,	"clicked", G_CALLBACK (xa_open_archive), NULL);

	g_signal_connect ((gpointer) back_button,	"clicked", G_CALLBACK (xa_handle_navigation_buttons), (gpointer) 1 );
	g_signal_connect ((gpointer) up_button,		"clicked", G_CALLBACK (xa_handle_navigation_buttons), (gpointer) 2 );
	g_signal_connect ((gpointer) forward_button,"clicked", G_CALLBACK (xa_handle_navigation_buttons), (gpointer) 3 );
	g_signal_connect ((gpointer) home_button,	"clicked", G_CALLBACK (xa_handle_navigation_buttons), (gpointer) 0 );

	g_signal_connect ((gpointer) AddFile_button,"clicked", G_CALLBACK (xa_add_files_archive), 		NULL);
    g_signal_connect ((gpointer) Extract_button,"clicked", G_CALLBACK (xa_extract_archive), 		NULL);
	g_signal_connect ((gpointer) View_button,	"clicked", G_CALLBACK (xa_view_file_inside_archive),NULL);
	g_signal_connect ((gpointer) Stop_button,	"clicked", G_CALLBACK (xa_cancel_archive),			NULL);
	g_signal_connect (xa_main_window, 		"key-press-event", G_CALLBACK (key_press_function),			NULL);

	gtk_window_add_accel_group (GTK_WINDOW (xa_main_window), accel_group);
}

int xa_progressbar_pulse (gpointer data)
{
	if ( ! xa_main_window)
		return FALSE;
	if ( ! GTK_WIDGET_VISIBLE(viewport2) )
		return FALSE;

	gtk_progress_bar_pulse(GTK_PROGRESS_BAR(progressbar) );
	return TRUE;
}


void xa_page_has_changed (GtkNotebook *notebook, GtkNotebookPage *page, guint page_num, gpointer user_data)
{
	gint id;
	GtkTreeSelection *selection;

	id = xa_find_archive_index (page_num);
	if (id == -1)
		return;

	xa_set_window_title (xa_main_window , archive[id]->path);

	if ( GTK_WIDGET_VISIBLE (viewport2) )
	{
		if (archive[id]->status == XA_ARCHIVESTATUS_IDLE)
		{
			gtk_widget_set_sensitive (Stop_button , FALSE);
			goto here;
		}
		xa_set_button_state (0,0,0,0,0,0,0,0);
		gtk_widget_set_sensitive ( Stop_button , TRUE);
		return;
	}
	xa_set_button_state (1,1,GTK_WIDGET_IS_SENSITIVE(close1),archive[id]->can_add,archive[id]->can_extract,archive[id]->has_sfx,archive[id]->has_test,archive[id]->has_properties);

here:
	xa_restore_navigation(id);

	if (archive[id]->has_comment)
		gtk_widget_set_sensitive (comment_menu,TRUE);
	else
		gtk_widget_set_sensitive (comment_menu,FALSE);

	if (archive[id]->status != XA_ARCHIVESTATUS_OPEN && archive[id]->treeview != NULL)
	{
		selection = gtk_tree_view_get_selection ( GTK_TREE_VIEW (archive[id]->treeview) );
		gint selected = gtk_tree_selection_count_selected_rows ( selection );
		if (selected == 0)
			xa_disable_delete_view_buttons (FALSE);
		else
		{
			if (archive[id]->type == XARCHIVETYPE_RAR && unrar)
				gtk_widget_set_sensitive ( delete_menu , FALSE );
			else if ( archive[id]->type != XARCHIVETYPE_RPM && archive[id]->type != XARCHIVETYPE_DEB )
				gtk_widget_set_sensitive ( delete_menu , TRUE );
			if (selected > 1 )
			{
				gtk_widget_set_sensitive ( View_button , FALSE);
				gtk_widget_set_sensitive ( view_menu, FALSE );
			}
			else
			{
				gtk_widget_set_sensitive ( View_button , TRUE );
				gtk_widget_set_sensitive ( view_menu, TRUE );
			}
		}
		/* Let's set the location bar */
		if (archive[id]->location_entry_path != NULL)
			gtk_entry_set_text(GTK_ENTRY(location_entry),archive[id]->location_entry_path);
		else
			gtk_entry_set_text(GTK_ENTRY(location_entry),"\0");

		gtk_widget_grab_focus (GTK_WIDGET(archive[id]->treeview));
	}
}

void xa_add_page (XArchive *archive)
{
	GtkWidget *page_hbox, *label, *tab_label, *close_button, *image;
	GtkTooltips *close_button_tips = gtk_tooltips_new();
	gchar *filename_only;

	if (gtk_notebook_get_current_page(notebook) > -1)
		gtk_notebook_set_show_tabs (notebook,TRUE);
	else
		gtk_notebook_set_show_tabs (notebook,FALSE);

	archive->scrollwindow = gtk_scrolled_window_new (NULL, NULL);
	g_object_set (G_OBJECT (archive->scrollwindow),"hscrollbar-policy", GTK_POLICY_AUTOMATIC,"vscrollbar-policy", GTK_POLICY_AUTOMATIC, NULL);
	gtk_widget_show (archive->scrollwindow);

	page_hbox = gtk_hbox_new(FALSE, 0);

	filename_only = g_strrstr ( archive->path, "/" );
	if (filename_only != NULL)
	{
		filename_only++;
		label = gtk_label_new (filename_only);
		tab_label = gtk_label_new (filename_only);
	}
	else
	{
		label = gtk_label_new (archive->path);
		tab_label = gtk_label_new (archive->path);
	}

	gtk_label_set_max_width_chars(GTK_LABEL(label), 50);
	gtk_label_set_ellipsize(GTK_LABEL(label), PANGO_ELLIPSIZE_START);
	gtk_box_pack_start(GTK_BOX(page_hbox), label, FALSE, FALSE, 0);

	close_button = gtk_button_new();
	gtk_tooltips_set_tip (close_button_tips, close_button, _("Close archive"), NULL);
	g_signal_connect (G_OBJECT(close_button), "clicked", G_CALLBACK(xa_close_page), (gpointer) archive->scrollwindow);

	image = gtk_image_new_from_stock ("gtk-close", GTK_ICON_SIZE_MENU);
	gtk_container_add (GTK_CONTAINER(close_button), image);
	gtk_widget_set_size_request (close_button, 19, 18);
	gtk_button_set_relief (GTK_BUTTON(close_button), GTK_RELIEF_NONE);
	gtk_box_pack_end (GTK_BOX(page_hbox), close_button, FALSE, FALSE, 0);
	gtk_widget_show_all (page_hbox);

	gtk_misc_set_alignment(GTK_MISC(tab_label), 0.0, 0);
	gtk_notebook_append_page_menu (notebook, archive->scrollwindow,page_hbox,tab_label);
	gtk_notebook_set_current_page(notebook, -1);
	gtk_notebook_set_tab_reorderable(notebook, archive->scrollwindow,TRUE);
	archive->treeview = gtk_tree_view_new ();
	gtk_container_add (GTK_CONTAINER (archive->scrollwindow), archive->treeview);
	gtk_widget_show (archive->treeview);
	gtk_tree_view_set_rules_hint ( GTK_TREE_VIEW (archive->treeview),TRUE);
	gtk_tree_view_set_search_equal_func (GTK_TREE_VIEW (archive->treeview),(GtkTreeViewSearchEqualFunc) treeview_select_search,NULL,NULL);
	GtkTreeSelection *sel = gtk_tree_view_get_selection( GTK_TREE_VIEW (archive->treeview));
	gtk_tree_selection_set_mode(sel, GTK_SELECTION_MULTIPLE);
	gtk_tree_view_set_rubber_banding(GTK_TREE_VIEW(archive->treeview),TRUE);

	gtk_drag_source_set (archive->treeview, GDK_BUTTON1_MASK, drag_targets, 1, GDK_ACTION_COPY | GDK_ACTION_MOVE | GDK_ACTION_LINK | GDK_ACTION_ASK);
	g_signal_connect ((gpointer) sel, 				"changed", 		G_CALLBACK (xa_activate_delete_and_view),archive);
	g_signal_connect (G_OBJECT (archive->treeview), "drag-begin",	G_CALLBACK (drag_begin),archive);
	g_signal_connect (G_OBJECT (archive->treeview), "drag-data-get",G_CALLBACK (drag_data_get),archive);
	g_signal_connect (G_OBJECT (archive->treeview), "drag-end",		G_CALLBACK (drag_end),NULL);
	g_signal_connect (G_OBJECT (archive->treeview), "row-activated",G_CALLBACK (xa_treeview_row_activated),archive);
	g_signal_connect (G_OBJECT (archive->treeview), "button-press-event",G_CALLBACK (xa_mouse_button_event),archive);
}

void xa_close_page (GtkWidget *widget, gpointer data)
{
	xa_close_archive ( NULL , data );
}

gchar *password_dialog ()
{
	GtkWidget *passwd;
	GtkWidget *dialog_vbox1;
	GtkWidget *hbox1;
	GtkWidget *label1;
	GtkWidget *password_entry;
	GtkWidget *dialog_action_area1;
	GtkWidget *cancelbutton1;
	GtkWidget *okbutton1;
	gboolean done = FALSE;
	gchar *password = NULL;
	gint current_page;
	gint idx;

	current_page = gtk_notebook_get_current_page (notebook);
	idx = xa_find_archive_index (current_page);

	passwd = gtk_dialog_new ();
	gtk_window_set_title (GTK_WINDOW (passwd),_("Enter Archive Password"));
	gtk_window_set_type_hint (GTK_WINDOW (passwd), GDK_WINDOW_TYPE_HINT_DIALOG);
	gtk_window_set_transient_for ( GTK_WINDOW (passwd) , GTK_WINDOW (xa_main_window) );
	gtk_window_set_default_size(GTK_WINDOW(passwd), 300, 80);

	dialog_vbox1 = GTK_DIALOG (passwd)->vbox;
	gtk_widget_show (dialog_vbox1);

	hbox1 = gtk_hbox_new (FALSE, 0);
	gtk_widget_show (hbox1);
	gtk_box_pack_start (GTK_BOX (dialog_vbox1), hbox1, TRUE, FALSE, 0);

	label1 = gtk_label_new (_("Password:"));
	gtk_widget_show (label1);
	gtk_box_pack_start (GTK_BOX (hbox1), label1, FALSE, FALSE, 0);

	password_entry = gtk_entry_new ();
	gtk_box_pack_start (GTK_BOX (hbox1), password_entry, TRUE, TRUE, 0);
	gtk_entry_set_visibility (GTK_ENTRY (password_entry), FALSE);
	gtk_entry_set_activates_default(GTK_ENTRY(password_entry), TRUE);

	if (current_page > 0 && archive[idx]->passwd != NULL)
		gtk_entry_set_text (GTK_ENTRY(password_entry),archive[idx]->passwd);
	gtk_widget_show (password_entry);

	dialog_action_area1 = GTK_DIALOG (passwd)->action_area;
	gtk_widget_show (dialog_action_area1);
	gtk_button_box_set_layout (GTK_BUTTON_BOX (dialog_action_area1), GTK_BUTTONBOX_END);

	cancelbutton1 = gtk_button_new_from_stock ("gtk-cancel");
	gtk_widget_show (cancelbutton1);
	gtk_dialog_add_action_widget (GTK_DIALOG (passwd), cancelbutton1, GTK_RESPONSE_CANCEL);
	GTK_WIDGET_SET_FLAGS (cancelbutton1, GTK_CAN_DEFAULT);

	okbutton1 = gtk_button_new_from_stock ("gtk-ok");
	gtk_widget_show (okbutton1);
	gtk_dialog_add_action_widget (GTK_DIALOG (passwd), okbutton1, GTK_RESPONSE_OK);
	GTK_WIDGET_SET_FLAGS (okbutton1, GTK_CAN_DEFAULT);
	gtk_dialog_set_default_response (GTK_DIALOG (passwd), GTK_RESPONSE_OK);

	while ( ! done )
	{
		switch (gtk_dialog_run ( GTK_DIALOG (passwd ) ) )
		{
			case GTK_RESPONSE_CANCEL:
			case GTK_RESPONSE_DELETE_EVENT:
			done = TRUE;
			password = NULL;
			break;

			case GTK_RESPONSE_OK:
			password = g_strdup (gtk_entry_get_text ( GTK_ENTRY (password_entry) ));
			if (strlen(password) == 0)
			{
				response = xa_show_message_dialog (GTK_WINDOW (xa_main_window),GTK_DIALOG_MODAL,GTK_MESSAGE_ERROR,GTK_BUTTONS_OK, _("You missed the password!"),_("Please enter it!") );
				break;
			}
			done = TRUE;
			break;
		}
	}
	gtk_widget_destroy (passwd);
	return password;
}

widget_data *xa_create_output_window(gchar *title)
{
	GtkWidget *vbox,*textview,*scrolledwindow;
	widget_data *data;

	data = g_new0(widget_data,1);
	data->dialog1 = gtk_dialog_new_with_buttons (title,
									GTK_WINDOW (xa_main_window), GTK_DIALOG_NO_SEPARATOR,
									GTK_STOCK_CLOSE,GTK_RESPONSE_CLOSE, NULL);
	gtk_dialog_set_default_response (GTK_DIALOG (data->dialog1), GTK_RESPONSE_CLOSE);
	gtk_widget_set_size_request (data->dialog1, 400, 250);
	vbox = GTK_DIALOG (data->dialog1)->vbox;

	scrolledwindow = gtk_scrolled_window_new (NULL, NULL);
	gtk_box_pack_start (GTK_BOX (vbox), scrolledwindow, TRUE, TRUE, 0);
	gtk_container_set_border_width (GTK_CONTAINER (scrolledwindow), 4);
	g_object_set (G_OBJECT (scrolledwindow),"hscrollbar-policy", GTK_POLICY_AUTOMATIC,"shadow-type", GTK_SHADOW_IN,"vscrollbar-policy", GTK_POLICY_AUTOMATIC, NULL);

	data->textbuffer = gtk_text_buffer_new (NULL);
	gtk_text_buffer_create_tag (data->textbuffer, "font","family", "monospace", NULL);
	gtk_text_buffer_get_iter_at_offset (data->textbuffer, &data->iter, 0);

	textview = gtk_text_view_new_with_buffer (data->textbuffer);
	g_object_unref (data->textbuffer);
	gtk_container_add (GTK_CONTAINER (scrolledwindow), textview);
	gtk_text_view_set_editable (GTK_TEXT_VIEW (textview), FALSE);
	gtk_text_view_set_cursor_visible (GTK_TEXT_VIEW (textview), FALSE);

	gtk_widget_show_all (data->dialog1);
	return data;
}

void xa_create_popup_menu()
{
	GtkWidget *cut;
	GtkWidget *image6;
	GtkWidget *copy;
	GtkWidget *image7;
	GtkWidget *paste;
	GtkWidget *image8;
	GtkWidget *separator;
	GtkWidget *view;
	GtkWidget *extract;
	GtkWidget *image9;
	GtkWidget *ddelete;
	GtkWidget *image10;
	GtkWidget *rename;
	GtkWidget *image11;

	xa_popup_menu = gtk_menu_new();

	view = gtk_image_menu_item_new_with_mnemonic (_("View"));
	gtk_widget_show (view);
	gtk_container_add (GTK_CONTAINER (xa_popup_menu),view);

	image9 = gtk_image_new_from_stock ("gtk-find", GTK_ICON_SIZE_MENU);
	gtk_widget_show (image9);
	gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (view), image9);

	extract = gtk_image_menu_item_new_with_mnemonic (_("Extract..."));
	gtk_widget_show (extract);
	gtk_container_add (GTK_CONTAINER (xa_popup_menu),extract);
	
	image9 =  xa_main_window_find_image ("xarchiver-extract.png", GTK_ICON_SIZE_MENU);
	gtk_widget_show (image9);
	gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (extract), image9);

	separator = gtk_separator_menu_item_new ();
	gtk_widget_show (separator);
	gtk_container_add (GTK_CONTAINER (xa_popup_menu), separator);
	gtk_widget_set_sensitive (separator,FALSE);
	
	cut = gtk_image_menu_item_new_with_mnemonic (_("Cut"));
	gtk_widget_show (cut);
	gtk_container_add (GTK_CONTAINER (xa_popup_menu), cut);

	image6 = gtk_image_new_from_stock ("gtk-cut", GTK_ICON_SIZE_MENU);
	gtk_widget_show (image6);
	gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (cut), image6);

	copy = gtk_image_menu_item_new_with_mnemonic (_("Copy"));
	gtk_widget_show (copy);
	gtk_container_add (GTK_CONTAINER (xa_popup_menu), copy);

	image7 = gtk_image_new_from_stock ("gtk-copy", GTK_ICON_SIZE_MENU);
	gtk_widget_show (image7);
	gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (copy), image7);

	paste = gtk_image_menu_item_new_with_mnemonic (_("Paste"));
	gtk_widget_show (paste);
	gtk_container_add (GTK_CONTAINER (xa_popup_menu), paste);

	image8 = gtk_image_new_from_stock ("gtk-paste", GTK_ICON_SIZE_MENU);
	gtk_widget_show (image8);
	gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (paste), image8);

	separator = gtk_separator_menu_item_new();
	gtk_widget_show (separator);
	gtk_container_add (GTK_CONTAINER (xa_popup_menu), separator);
	gtk_widget_set_sensitive (separator,FALSE);

	ddelete = gtk_image_menu_item_new_with_mnemonic (_("Delete"));
	gtk_widget_show (ddelete);
	gtk_container_add (GTK_CONTAINER (xa_popup_menu), ddelete);

	image10 = gtk_image_new_from_stock ("gtk-delete", GTK_ICON_SIZE_MENU);
	gtk_widget_show (image10);
	gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (ddelete), image10);

	rename = gtk_image_menu_item_new_with_mnemonic (_("Rename..."));
	gtk_widget_show (rename);
	gtk_container_add (GTK_CONTAINER (xa_popup_menu), rename);

	image11 = gtk_image_new_from_stock ("gtk-refresh", GTK_ICON_SIZE_MENU);
	gtk_widget_show (image11);
	gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (rename), image11);

	/*g_signal_connect ((gpointer) cut, "activate",G_CALLBACK (on_xa_cut_activate),NULL);
	g_signal_connect ((gpointer) copy, "activate",G_CALLBACK (on_xa_copy_activate),NULL);
	g_signal_connect ((gpointer) paste, "activate",G_CALLBACK (on_xa_paste_activate),NULL);
	g_signal_connect ((gpointer) open, "activate",G_CALLBACK (on_xa_open_activate),NULL);
	g_signal_connect ((gpointer) ddelete, "activate",G_CALLBACK (on_xa_delete_activate),NULL);
	g_signal_connect ((gpointer) rename, "activate",G_CALLBACK (on_xa_rename_activate),NULL);*/
}

void xa_create_delete_dialog(GtkMenuItem *menuitem, gpointer user_data)
{
	GtkWidget *ddialog1;
	GtkWidget *ddialog_vbox1;
	GtkWidget *dhbox1;
	GtkWidget *pattern_label;
	GtkWidget *pattern_entry;
	GtkWidget *dialog_action_area1;
	GtkWidget *cancelbutton1;
	GtkWidget *okbutton1;
  	gchar *string = NULL;
  	gboolean done = FALSE;
  
	ddialog1 = gtk_dialog_new ();
	gtk_window_set_title (GTK_WINDOW (ddialog1), _("Select by Pattern"));
	gtk_window_set_modal (GTK_WINDOW (ddialog1), TRUE);
	gtk_window_set_type_hint (GTK_WINDOW (ddialog1), GDK_WINDOW_TYPE_HINT_DIALOG);
	gtk_widget_set_size_request(ddialog1,286,93);
	gtk_window_set_transient_for (GTK_WINDOW (ddialog1),GTK_WINDOW (xa_main_window));
	ddialog_vbox1 = GTK_DIALOG (ddialog1)->vbox;
	gtk_widget_show (ddialog_vbox1);

	dhbox1 = gtk_hbox_new (FALSE, 10);
	gtk_widget_show (dhbox1);
	gtk_box_pack_start (GTK_BOX (ddialog_vbox1), dhbox1, TRUE, TRUE, 0);
	gtk_container_set_border_width (GTK_CONTAINER (dhbox1), 5);

	pattern_label = gtk_label_new (_("Pattern:"));
	gtk_widget_show (pattern_label);
	gtk_box_pack_start (GTK_BOX (dhbox1), pattern_label, FALSE, FALSE, 0);

	pattern_entry = gtk_entry_new ();
	gtk_widget_show (pattern_entry);
	gtk_box_pack_start (GTK_BOX (dhbox1), pattern_entry, TRUE, TRUE, 0);
	gtk_entry_set_activates_default(GTK_ENTRY(pattern_entry), TRUE);

	dialog_action_area1 = GTK_DIALOG (ddialog1)->action_area;
	gtk_widget_show (dialog_action_area1);
	gtk_button_box_set_layout (GTK_BUTTON_BOX (dialog_action_area1),GTK_BUTTONBOX_END);

	cancelbutton1 = gtk_button_new_from_stock ("gtk-cancel");
	gtk_widget_show (cancelbutton1);
	gtk_dialog_add_action_widget (GTK_DIALOG (ddialog1), cancelbutton1,GTK_RESPONSE_CANCEL);
	GTK_WIDGET_SET_FLAGS (cancelbutton1, GTK_CAN_DEFAULT);

	okbutton1 = gtk_button_new_with_mnemonic (_("Select"));
	gtk_widget_show (okbutton1);
	gtk_dialog_add_action_widget (GTK_DIALOG (ddialog1), okbutton1,GTK_RESPONSE_OK);
	GTK_WIDGET_SET_FLAGS (okbutton1, GTK_CAN_DEFAULT);
	gtk_dialog_set_default_response (GTK_DIALOG (ddialog1), GTK_RESPONSE_OK);
	
	while ( ! done )
	{
		switch (gtk_dialog_run (GTK_DIALOG(ddialog1)))
		{
			case GTK_RESPONSE_CANCEL:
			case GTK_RESPONSE_DELETE_EVENT:
			done = TRUE;
			goto destroy_delete_dialog;
			break;

			case GTK_RESPONSE_OK:
			string = g_strdup (gtk_entry_get_text(GTK_ENTRY(pattern_entry)));
			if (strlen(string) == 0)
			{
				xa_deselect_all(NULL,NULL);
				done = TRUE;
				break;
			}
			done = TRUE;
			break;
		}
	}
	//check and select the rows matching the pattern here
	
	g_free(string);

destroy_delete_dialog:
	gtk_widget_destroy (ddialog1);
 }

GtkWidget *create_archive_properties_window()
{
	archive_properties_window = gtk_dialog_new_with_buttons (_("Archive Properties Window"),
									GTK_WINDOW (xa_main_window), GTK_DIALOG_DESTROY_WITH_PARENT,
									GTK_STOCK_CLOSE, GTK_RESPONSE_CANCEL, NULL);

	g_signal_connect(archive_properties_window, "response", G_CALLBACK(gtk_widget_destroy), NULL);
	g_signal_connect(archive_properties_window, "delete-event", G_CALLBACK(gtk_widget_destroy), NULL);

	gtk_window_set_position (GTK_WINDOW (archive_properties_window), GTK_WIN_POS_CENTER);
	gtk_window_set_resizable (GTK_WINDOW (archive_properties_window), FALSE);
	gtk_window_set_modal (GTK_WINDOW (archive_properties_window), TRUE);
	gtk_window_set_type_hint (GTK_WINDOW (archive_properties_window), GDK_WINDOW_TYPE_HINT_UTILITY);

	table1 = gtk_table_new (10, 2, TRUE);
	gtk_container_add (GTK_CONTAINER (GTK_DIALOG (archive_properties_window)->vbox), table1);
	gtk_table_set_row_spacings (GTK_TABLE (table1), 6);
	gtk_table_set_col_spacings (GTK_TABLE (table1), 6);

	name_label = gtk_label_new ("");
	set_label ( name_label , _("Name:"));
	gtk_table_attach (GTK_TABLE (table1), name_label, 0, 1, 0, 1,
                     (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (GTK_EXPAND | GTK_SHRINK | GTK_FILL), 0, 0);
	gtk_misc_set_alignment (GTK_MISC (name_label), 0.99, 0.5);

	path_label = gtk_label_new ("");
	set_label ( path_label , _("Path:"));
	gtk_table_attach (GTK_TABLE (table1), path_label, 0, 1, 1, 2,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (GTK_EXPAND | GTK_SHRINK | GTK_FILL), 0, 0);
	gtk_misc_set_alignment (GTK_MISC (path_label), 0.99, 0.5);

	type_label = gtk_label_new ("");
	set_label ( type_label , _("Type:"));
	gtk_table_attach (GTK_TABLE (table1), type_label, 0, 1, 2, 3,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (GTK_EXPAND | GTK_SHRINK | GTK_FILL), 0, 0);
	gtk_misc_set_alignment (GTK_MISC (type_label), 0.99, 0.5);

	modified_label = gtk_label_new ("");
	set_label ( modified_label , _("Modified on:"));
	gtk_table_attach (GTK_TABLE (table1), modified_label, 0, 1, 3, 4,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (GTK_EXPAND | GTK_SHRINK | GTK_FILL), 0, 0);
	gtk_misc_set_alignment (GTK_MISC (modified_label), 0.99, 0.5);

	size_label = gtk_label_new ("");
	set_label ( size_label , _("Archive size:"));
	gtk_table_attach (GTK_TABLE (table1), size_label, 0, 1, 4, 5,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (GTK_EXPAND | GTK_SHRINK | GTK_FILL), 0, 0);
	gtk_misc_set_alignment (GTK_MISC (size_label), 0.99, 0.5);

	content_label = gtk_label_new ("");
	set_label ( content_label , _("Content size:"));
	gtk_table_attach (GTK_TABLE (table1), content_label, 0, 1, 5, 6,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (GTK_EXPAND | GTK_SHRINK | GTK_FILL), 0, 0);
	gtk_misc_set_alignment (GTK_MISC (content_label), 0.99, 0.5);

	comment_label = gtk_label_new ("");
	set_label ( comment_label , _("Comment:"));
	gtk_table_attach (GTK_TABLE (table1), comment_label, 0, 1, 6, 7,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (GTK_EXPAND | GTK_SHRINK | GTK_FILL), 0, 0);
	gtk_misc_set_alignment (GTK_MISC (comment_label), 0.99, 0.5);

	compression_label = gtk_label_new ("");
	set_label ( compression_label , _("Compression ratio:"));
	gtk_table_attach (GTK_TABLE (table1), compression_label, 0, 1, 9, 10,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (GTK_EXPAND | GTK_SHRINK | GTK_FILL), 0, 0);
	gtk_misc_set_alignment (GTK_MISC (compression_label), 0.99, 0.5);

	number_of_files_label = gtk_label_new ("");
	set_label ( number_of_files_label , _("Number of files:"));
	gtk_table_attach (GTK_TABLE (table1), number_of_files_label, 0, 1, 7, 8,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (GTK_EXPAND | GTK_SHRINK | GTK_FILL), 0, 0);
	gtk_misc_set_alignment (GTK_MISC (number_of_files_label), 0.99, 0.5);

	number_of_dirs_label = gtk_label_new ("");
	set_label ( number_of_dirs_label , _("Number of dirs:"));
	gtk_table_attach (GTK_TABLE (table1), number_of_dirs_label, 0, 1, 8, 9,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (GTK_EXPAND | GTK_SHRINK | GTK_FILL), 0, 0);
	gtk_misc_set_alignment (GTK_MISC (number_of_dirs_label), 0.99, 0.5);

	compression_data = gtk_entry_new ();
	gtk_editable_set_editable (GTK_EDITABLE (compression_data), FALSE);
	gtk_entry_set_has_frame (GTK_ENTRY (compression_data), FALSE);
	gtk_table_attach (GTK_TABLE (table1), compression_data, 1, 2, 9, 10,
                    (GtkAttachOptions) (GTK_FILL | GTK_EXPAND),
                    (GtkAttachOptions) (0), 0, 0);

	number_of_dirs_data = gtk_entry_new ();
	gtk_editable_set_editable (GTK_EDITABLE (number_of_dirs_data), FALSE);
	gtk_entry_set_has_frame (GTK_ENTRY (number_of_dirs_data), FALSE);
	gtk_table_attach (GTK_TABLE (table1), number_of_dirs_data, 1, 2, 8, 9,
                    (GtkAttachOptions) (GTK_FILL | GTK_EXPAND),
                    (GtkAttachOptions) (0), 0, 0);

	number_of_files_data = gtk_entry_new ();
	gtk_editable_set_editable (GTK_EDITABLE (number_of_files_data), FALSE);
	gtk_entry_set_has_frame (GTK_ENTRY (number_of_files_data), FALSE);
	gtk_table_attach (GTK_TABLE (table1), number_of_files_data, 1, 2, 7, 8,
                    (GtkAttachOptions) (GTK_FILL | GTK_EXPAND),
                    (GtkAttachOptions) (0), 0, 0);

	comment_data = gtk_entry_new ();
	gtk_editable_set_editable (GTK_EDITABLE (comment_data), FALSE);
	gtk_entry_set_has_frame (GTK_ENTRY (comment_data), FALSE);
	gtk_table_attach (GTK_TABLE (table1), comment_data, 1, 2, 6, 7,
                    (GtkAttachOptions) (GTK_FILL | GTK_EXPAND),
                    (GtkAttachOptions) (0), 0, 0);

	content_data = gtk_entry_new ();
	gtk_editable_set_editable (GTK_EDITABLE (content_data), FALSE);
	gtk_entry_set_has_frame (GTK_ENTRY (content_data), FALSE);
	gtk_table_attach (GTK_TABLE (table1), content_data, 1, 2, 5, 6,
                    (GtkAttachOptions) (GTK_FILL | GTK_EXPAND),
                    (GtkAttachOptions) (0), 0, 0);

	size_data = gtk_entry_new ();
	gtk_editable_set_editable (GTK_EDITABLE (size_data), FALSE);
	gtk_entry_set_has_frame (GTK_ENTRY (size_data), FALSE);
	gtk_table_attach (GTK_TABLE (table1), size_data, 1, 2, 4, 5,
                    (GtkAttachOptions) (GTK_FILL | GTK_EXPAND),
                    (GtkAttachOptions) (0), 0, 0);

	modified_data = gtk_entry_new ();
	gtk_table_attach (GTK_TABLE (table1), modified_data, 1, 2, 3, 4,
                    (GtkAttachOptions) (GTK_FILL | GTK_EXPAND),
                    (GtkAttachOptions) (0), 0, 0);
	gtk_editable_set_editable (GTK_EDITABLE (modified_data), FALSE);
	gtk_entry_set_has_frame (GTK_ENTRY (modified_data), FALSE);

	type_data = gtk_entry_new ();
	gtk_editable_set_editable (GTK_EDITABLE (type_data), FALSE);
	gtk_entry_set_has_frame (GTK_ENTRY (type_data), FALSE);
	gtk_table_attach (GTK_TABLE (table1), type_data, 1, 2, 2, 3,
                    (GtkAttachOptions) (GTK_FILL | GTK_EXPAND),
                    (GtkAttachOptions) (0), 0, 0);

	path_data = gtk_entry_new ();
	gtk_editable_set_editable (GTK_EDITABLE (path_data), FALSE);
	gtk_entry_set_has_frame (GTK_ENTRY (path_data), FALSE);
	gtk_table_attach (GTK_TABLE (table1), path_data, 1, 2, 1, 2,
                    (GtkAttachOptions) (GTK_FILL | GTK_EXPAND),
                    (GtkAttachOptions) (0), 0, 0);

	name_data = gtk_entry_new ();
	gtk_editable_set_editable (GTK_EDITABLE (name_data), FALSE);
	gtk_entry_set_has_frame (GTK_ENTRY (name_data), FALSE);
	gtk_table_attach (GTK_TABLE (table1), name_data, 1, 2, 0, 1,
                    (GtkAttachOptions) (GTK_FILL | GTK_EXPAND),
                    (GtkAttachOptions) (0), 0, 0);
	return archive_properties_window;
}

void set_label (GtkWidget *label,gchar *text)
{
    gchar *tmp_markup = g_strdup_printf ("<b>%s</b>",text );
    gtk_label_set_markup ( GTK_LABEL (label) , tmp_markup);
    g_free (tmp_markup);
}

void xa_set_button_state (gboolean New, gboolean Open,gboolean Close, gboolean add,gboolean extract, gboolean sfx, gboolean test, gboolean info)
{
	gtk_widget_set_sensitive (New_button, New);
    gtk_widget_set_sensitive (new1, New);
	gtk_widget_set_sensitive (Open_button, Open);
    gtk_widget_set_sensitive (open1, Open);
    gtk_widget_set_sensitive (close1, Close);
	gtk_widget_set_sensitive (AddFile_button, add);
	gtk_widget_set_sensitive (addfile, add);
	gtk_widget_set_sensitive (Extract_button, extract);
	gtk_widget_set_sensitive (extract_menu, extract);
	gtk_widget_set_sensitive (exe_menu, sfx);
	gtk_widget_set_sensitive (check_menu, test);
	gtk_widget_set_sensitive (properties, info);
	gtk_widget_set_sensitive (select_all, Close);
	gtk_widget_set_sensitive (select_pattern, Close);
}

void xa_handle_navigation_buttons (GtkMenuItem *menuitem, gpointer user_data)
{
	unsigned short int bp = GPOINTER_TO_UINT(user_data);
	gint current_page;
	gint idx;
	XEntry *new_entry = NULL;

	current_page = gtk_notebook_get_current_page (notebook);
	idx = xa_find_archive_index (current_page);

	switch (bp)
	{
		/* Root */
		case 0:
			if (archive[idx]->location_entry_path != NULL)
			{
				g_free(archive[idx]->location_entry_path);
				archive[idx]->location_entry_path = NULL;
			}

			xa_update_window_with_archive_entries(archive[idx],NULL);

			xa_restore_navigation(idx);
		break;

		/* Back */
		case 1:
			if (g_slist_find(archive[idx]->forward,archive[idx]->current_entry) == NULL)
				archive[idx]->forward = g_slist_prepend(archive[idx]->forward,archive[idx]->current_entry);
			xa_update_window_with_archive_entries(archive[idx],archive[idx]->back->data);
			archive[idx]->back = archive[idx]->back->next;

			xa_restore_navigation(idx);
		break;

		/* Up */
		case 2:
			archive[idx]->forward = g_slist_prepend(archive[idx]->forward,archive[idx]->current_entry);
			new_entry = archive[idx]->current_entry;

			if (new_entry->prev->prev == NULL)
				xa_update_window_with_archive_entries(archive[idx],NULL);
			else
				xa_update_window_with_archive_entries(archive[idx],new_entry->prev);

			xa_restore_navigation(idx);
		break;

		/* Forward */
		case 3:
			if (g_slist_find(archive[idx]->back,archive[idx]->current_entry) == NULL)
				archive[idx]->back = g_slist_prepend(archive[idx]->back,archive[idx]->current_entry);

			xa_update_window_with_archive_entries(archive[idx],archive[idx]->forward->data);
			archive[idx]->forward = archive[idx]->forward->next;

			xa_restore_navigation(idx);
		break;
	}
}

void xa_restore_navigation(int idx)
{
	gboolean back = FALSE,up = FALSE,forward = FALSE, home=FALSE;

	/*If the pointers exist, we should show the icon*/
	if(archive[idx]->forward!=NULL) forward=TRUE;
	if(archive[idx]->back!=NULL) back=TRUE;

	if(archive[idx]->location_entry_path!=NULL)
	{
		/* If there's a slash on the path, we should allow UP and HOME operations */
		if(strstr(archive[idx]->location_entry_path,"/")!=NULL)
		{
			home=TRUE;
			up=TRUE;
		}
	}

	gtk_widget_set_sensitive(back_button,back);
	gtk_widget_set_sensitive(forward_button,forward);
	gtk_widget_set_sensitive(up_button,up);
	gtk_widget_set_sensitive(home_button,home);
}

