/*
 *  Copyright (c) 2008 Giuseppe Torelli <colossus73@gmail.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License,or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not,write to the Free Software
 *  Foundation,Inc.,59 Temple Place - Suite 330,Boston,MA 02111-1307,USA.
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

extern gboolean unrar,batch_mode;

static gboolean xa_progress_dialog_delete_event (GtkWidget *caller,GdkEvent *event,GPid pid);
static void xa_progress_dialog_stop_action (GtkWidget *widget,GPid pid);

void xa_create_main_window (GtkWidget *xa_main_window,gboolean show_location,gboolean show_output_menu_item,gboolean show_sidebar)
{
	GdkPixbuf *icon;

	xa_create_popup_menu();
	tooltips = gtk_tooltips_new ();
	accel_group = gtk_accel_group_new ();
	xa_set_window_title (xa_main_window,NULL);

	/* icon_theme is initialized in pref_dialog.c:45 */
	//g_signal_connect (G_OBJECT (icon_theme),"changed",G_CALLBACK (xa_icon_theme_changed),NULL);
	icon = gtk_icon_theme_load_icon(icon_theme,"xarchiver",24,0,NULL);
	gtk_window_set_icon (GTK_WINDOW(xa_main_window),icon);
	g_signal_connect (G_OBJECT (xa_main_window),"delete-event",G_CALLBACK (xa_quit_application),NULL);

	vbox1 = gtk_vbox_new (FALSE,2);
	gtk_widget_show (vbox1);
	gtk_container_add (GTK_CONTAINER (xa_main_window),vbox1);

	/* Create the menus */
	menubar1 = gtk_menu_bar_new ();
	gtk_widget_show (menubar1);
	gtk_box_pack_start (GTK_BOX (vbox1),menubar1,FALSE,FALSE,0);

	menuitem1 = gtk_menu_item_new_with_mnemonic (_("_Archive"));
	gtk_widget_show (menuitem1);
	gtk_container_add (GTK_CONTAINER (menubar1),menuitem1);

	menuitem1_menu = gtk_menu_new ();
	gtk_menu_item_set_submenu (GTK_MENU_ITEM (menuitem1),menuitem1_menu);

	new1 = gtk_image_menu_item_new_from_stock ("gtk-new",accel_group);
	gtk_widget_show (new1);
	gtk_container_add (GTK_CONTAINER (menuitem1_menu),new1);

	open1 = gtk_image_menu_item_new_from_stock ("gtk-open",accel_group);
	gtk_widget_show (open1);
	gtk_container_add (GTK_CONTAINER (menuitem1_menu),open1);

	listing = gtk_image_menu_item_new_with_mnemonic(_("_List as"));
	gtk_widget_show (listing);
	gtk_container_add (GTK_CONTAINER (menuitem1_menu),listing);
	tmp_image = gtk_image_new_from_stock ("gtk-copy",GTK_ICON_SIZE_MENU);
	gtk_widget_show (tmp_image);
	gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (listing),tmp_image);
	
	listing_submenu = gtk_menu_new();
	gtk_widget_set_sensitive(listing,FALSE);
	gtk_widget_show (listing_submenu);
	gtk_menu_item_set_submenu (GTK_MENU_ITEM (listing),listing_submenu);
	listing_text = gtk_image_menu_item_new_with_mnemonic (_("_Text file"));
	gtk_widget_show (listing_text);
	gtk_container_add (GTK_CONTAINER (listing_submenu),listing_text);
	tmp_image = gtk_image_new_from_stock ("gtk-edit",GTK_ICON_SIZE_MENU);
	gtk_widget_show (tmp_image);
	gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (listing_text),tmp_image);
	
	listing_html = gtk_image_menu_item_new_with_mnemonic (_("_HTML file"));
	gtk_widget_show (listing_html);
	gtk_container_add (GTK_CONTAINER (listing_submenu),listing_html);
	tmp_image =  xa_main_window_find_image ("xarchiver-html.png",GTK_ICON_SIZE_MENU);
	gtk_widget_show (tmp_image);
	gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (listing_html),tmp_image);
	
	save1 = gtk_image_menu_item_new_from_stock ("gtk-save-as",accel_group);
	gtk_widget_set_sensitive(save1,FALSE);
	gtk_widget_show (save1);
	gtk_container_add (GTK_CONTAINER (menuitem1_menu),save1);

	separatormenuitem1 = gtk_separator_menu_item_new ();
	gtk_widget_show (separatormenuitem1);
	gtk_container_add (GTK_CONTAINER (menuitem1_menu),separatormenuitem1);
	gtk_widget_set_sensitive (separatormenuitem1,FALSE);

	check_menu = gtk_image_menu_item_new_with_mnemonic (_("_Test"));
	gtk_widget_show (check_menu);
	gtk_widget_set_sensitive (check_menu,FALSE);
	gtk_container_add (GTK_CONTAINER (menuitem1_menu),check_menu);
	gtk_widget_add_accelerator (check_menu,"activate",accel_group,GDK_t,GDK_CONTROL_MASK,GTK_ACCEL_VISIBLE);

	tmp_image = gtk_image_new_from_stock ("gtk-index",GTK_ICON_SIZE_MENU);
	gtk_widget_show (tmp_image);
	gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (check_menu),tmp_image);

	properties = gtk_image_menu_item_new_with_mnemonic (_("_Properties"));
	gtk_widget_show (properties);
	gtk_widget_set_sensitive ( properties,FALSE);
	gtk_container_add (GTK_CONTAINER (menuitem1_menu),properties);
	gtk_widget_add_accelerator (properties,"activate",accel_group,GDK_p,GDK_CONTROL_MASK,GTK_ACCEL_VISIBLE);

	tmp_image = gtk_image_new_from_stock ("gtk-properties",GTK_ICON_SIZE_MENU);
	gtk_widget_show (tmp_image);
	gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (properties),tmp_image);

	close1 = gtk_image_menu_item_new_from_stock ("gtk-close",accel_group);
	gtk_widget_set_sensitive (close1,FALSE);
	gtk_widget_show (close1);
	gtk_container_add (GTK_CONTAINER (menuitem1_menu),close1);

	separatormenuitem2 = gtk_separator_menu_item_new ();
	gtk_widget_show (separatormenuitem2);
	gtk_container_add (GTK_CONTAINER (menuitem1_menu),separatormenuitem2);
	gtk_widget_set_sensitive (separatormenuitem2,FALSE);

	quit1 = gtk_image_menu_item_new_from_stock ("gtk-quit",accel_group);
	gtk_widget_show (quit1);
	gtk_container_add (GTK_CONTAINER (menuitem1_menu),quit1);

	menuitem2 = gtk_menu_item_new_with_mnemonic (_("A_ction"));
	gtk_widget_show (menuitem2);
	gtk_container_add (GTK_CONTAINER (menubar1),menuitem2);

	menuitem2_menu = gtk_menu_new ();
	gtk_menu_item_set_submenu (GTK_MENU_ITEM (menuitem2),menuitem2_menu);

	addfile = gtk_image_menu_item_new_with_mnemonic (_("A_dd"));
	gtk_widget_set_sensitive (addfile,FALSE);
	gtk_widget_show (addfile);
	gtk_container_add (GTK_CONTAINER (menuitem2_menu),addfile);
	gtk_widget_add_accelerator (addfile,"activate",accel_group,GDK_d,GDK_CONTROL_MASK,GTK_ACCEL_VISIBLE);

	image2 = xa_main_window_find_image ("xarchiver-add.png",GTK_ICON_SIZE_MENU);
	gtk_widget_show (image2);
	gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (addfile),image2);

	extract_menu = gtk_image_menu_item_new_with_mnemonic (_("_Extract"));
	gtk_widget_set_sensitive (extract_menu,FALSE);
	gtk_widget_show (extract_menu);
	gtk_container_add (GTK_CONTAINER (menuitem2_menu),extract_menu);
	gtk_widget_add_accelerator (extract_menu,"activate",accel_group,GDK_e,GDK_CONTROL_MASK,GTK_ACCEL_VISIBLE);

	image2 =  xa_main_window_find_image ("xarchiver-extract.png",GTK_ICON_SIZE_MENU);
	gtk_widget_show (image2);
	gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (extract_menu),image2);

	delete_menu = gtk_image_menu_item_new_from_stock ("gtk-delete",NULL);
	gtk_widget_set_sensitive (delete_menu,FALSE);
	gtk_widget_show (delete_menu);
	gtk_container_add (GTK_CONTAINER (menuitem2_menu),delete_menu);
	gtk_widget_add_accelerator (delete_menu,"activate",accel_group,GDK_Delete,GDK_MODE_DISABLED,GTK_ACCEL_VISIBLE);

	rename_menu = gtk_image_menu_item_new_with_mnemonic (_("Re_name"));
	gtk_widget_set_sensitive (rename_menu,FALSE);
	gtk_widget_show (rename_menu);
	gtk_container_add (GTK_CONTAINER (menuitem2_menu),rename_menu);

	tmp_image = gtk_image_new_from_stock ("gtk-refresh",GTK_ICON_SIZE_MENU);
	gtk_widget_show (tmp_image);
	gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (rename_menu),tmp_image);

	separatormenuitem3 = gtk_separator_menu_item_new ();
	gtk_widget_show (separatormenuitem3);
	gtk_container_add (GTK_CONTAINER (menuitem2_menu),separatormenuitem3);
	gtk_widget_set_sensitive (separatormenuitem3,FALSE);

	exe_menu = gtk_image_menu_item_new_with_mnemonic (_("Make SF_X"));
	gtk_widget_set_sensitive (exe_menu,FALSE);
	gtk_widget_show (exe_menu);
	gtk_container_add (GTK_CONTAINER (menuitem2_menu),exe_menu);
	gtk_widget_add_accelerator (exe_menu,"activate",accel_group,GDK_x,GDK_CONTROL_MASK,GTK_ACCEL_VISIBLE);

	image2 = gtk_image_new_from_stock ("gtk-execute",GTK_ICON_SIZE_MENU);
	gtk_widget_show (image2);
	gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (exe_menu),image2);

	multi_extract_menu = gtk_menu_item_new_with_mnemonic (_("_Multi-Extract"));
	gtk_widget_show (multi_extract_menu);
	gtk_container_add (GTK_CONTAINER (menuitem2_menu),multi_extract_menu);
	gtk_widget_add_accelerator (multi_extract_menu,"activate",accel_group,GDK_m,GDK_CONTROL_MASK,GTK_ACCEL_VISIBLE);

	comment_menu = gtk_image_menu_item_new_with_mnemonic (_("Archive _comment"));
	gtk_widget_set_sensitive (comment_menu,FALSE);
	gtk_widget_show (comment_menu);
	gtk_container_add (GTK_CONTAINER (menuitem2_menu),comment_menu);
	gtk_widget_add_accelerator (comment_menu,"activate",accel_group,GDK_s,GDK_CONTROL_MASK,GTK_ACCEL_VISIBLE);

	tmp_image = gtk_image_new_from_stock ("gtk-justify-fill",GTK_ICON_SIZE_MENU);
	gtk_widget_show (tmp_image);
	gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (comment_menu),tmp_image);
	separatormenuitem4 = gtk_separator_menu_item_new ();
	gtk_widget_show (separatormenuitem4);
	gtk_container_add (GTK_CONTAINER (menuitem2_menu),separatormenuitem4);
	gtk_widget_set_sensitive (separatormenuitem4,FALSE);

	select_all = gtk_image_menu_item_new_with_mnemonic (_("Select _all"));
	gtk_widget_show (select_all);
	gtk_container_add (GTK_CONTAINER (menuitem2_menu),select_all);
	gtk_widget_set_sensitive (select_all,FALSE);
	gtk_widget_add_accelerator (select_all,"activate",accel_group,GDK_a,GDK_CONTROL_MASK,GTK_ACCEL_VISIBLE);

	tmp_image = gtk_image_new_from_stock ("gtk-select-all",GTK_ICON_SIZE_MENU);
	gtk_widget_show (tmp_image);
	gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (select_all),tmp_image);

	deselect_all = gtk_image_menu_item_new_with_mnemonic (_("Dese_lect all"));
	gtk_widget_show (deselect_all);
	gtk_container_add (GTK_CONTAINER (menuitem2_menu),deselect_all);
	gtk_widget_set_sensitive (deselect_all,FALSE);
	gtk_widget_add_accelerator (deselect_all,"activate",accel_group,GDK_l,GDK_CONTROL_MASK,GTK_ACCEL_VISIBLE);

	select_pattern = gtk_menu_item_new_with_mnemonic (_("Select _by pattern"));
	gtk_widget_show (select_pattern);
	gtk_container_add (GTK_CONTAINER (menuitem2_menu),select_pattern);
	gtk_widget_set_sensitive (select_pattern,FALSE);
	gtk_widget_add_accelerator (select_pattern,"activate",accel_group,GDK_b,GDK_CONTROL_MASK,GTK_ACCEL_VISIBLE);

	separatormenuitem5 = gtk_separator_menu_item_new ();
	gtk_widget_show (separatormenuitem5);
	gtk_container_add (GTK_CONTAINER (menuitem2_menu),separatormenuitem5);
	gtk_widget_set_sensitive (separatormenuitem5,FALSE);

	view_shell_output1 = gtk_image_menu_item_new_with_mnemonic (_("Cmd-line outp_ut"));
	gtk_container_add (GTK_CONTAINER (menuitem2_menu),view_shell_output1);
	gtk_widget_add_accelerator (view_shell_output1,"activate",accel_group,GDK_u,GDK_CONTROL_MASK,GTK_ACCEL_VISIBLE);
	if (show_output_menu_item)
		gtk_widget_set_sensitive(view_shell_output1,TRUE);
	else
		gtk_widget_set_sensitive(view_shell_output1,FALSE);
	gtk_widget_show (view_shell_output1);

	image2 = gtk_image_new_from_stock ("gtk-find-and-replace",GTK_ICON_SIZE_MENU);
	gtk_widget_show (image2);
	gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (view_shell_output1),image2);

	password_entry_menu = gtk_menu_item_new_with_mnemonic (_("Enter passwo_rd"));
	gtk_widget_show (password_entry_menu);
	gtk_widget_set_sensitive (password_entry_menu,FALSE);
	gtk_container_add (GTK_CONTAINER (menuitem2_menu),password_entry_menu);
	gtk_widget_add_accelerator (password_entry_menu,"activate",accel_group,GDK_r,GDK_CONTROL_MASK,GTK_ACCEL_VISIBLE);

	separatormenuitem6 = gtk_separator_menu_item_new ();
	gtk_widget_show (separatormenuitem6);
	gtk_container_add (GTK_CONTAINER (menuitem2_menu),separatormenuitem6);
	gtk_widget_set_sensitive (separatormenuitem6,FALSE);

	prefs_menu = gtk_image_menu_item_new_with_mnemonic (_("_Preferences"));
	gtk_widget_show (prefs_menu);
	gtk_container_add (GTK_CONTAINER (menuitem2_menu),prefs_menu);
	gtk_widget_add_accelerator (prefs_menu,"activate",accel_group,GDK_f,GDK_CONTROL_MASK,GTK_ACCEL_VISIBLE);

	tmp_image = gtk_image_new_from_stock ("gtk-preferences",GTK_ICON_SIZE_MENU);
	gtk_widget_show (tmp_image);
	gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (prefs_menu),tmp_image);

	menuitem4 = gtk_image_menu_item_new_with_mnemonic (_("_Help"));
	gtk_widget_show (menuitem4);
	gtk_container_add (GTK_CONTAINER (menubar1),menuitem4);

	menuitem4_menu = gtk_menu_new ();
	gtk_menu_item_set_submenu (GTK_MENU_ITEM (menuitem4),menuitem4_menu);

	help1 = gtk_image_menu_item_new_with_mnemonic (_("_Contents"));
	gtk_widget_show (help1);
	gtk_container_add (GTK_CONTAINER (menuitem4_menu),help1);
	gtk_widget_add_accelerator (help1,"activate",accel_group,GDK_F1,GDK_MODE_DISABLED,GTK_ACCEL_VISIBLE);

	tmp_image = gtk_image_new_from_stock ("gtk-help",GTK_ICON_SIZE_MENU);
	gtk_widget_show (tmp_image);
	gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (help1),tmp_image);

	donate = gtk_image_menu_item_new_with_mnemonic (_("_Donate"));
	gtk_widget_show (donate);
	gtk_container_add (GTK_CONTAINER (menuitem4_menu),donate);

	about1 = gtk_image_menu_item_new_from_stock ("gtk-about",accel_group);
	gtk_widget_show (about1);
	gtk_container_add (GTK_CONTAINER (menuitem4_menu),about1);

	/* Create the toolbar */
	toolbar1 = gtk_toolbar_new ();
	gtk_widget_show (toolbar1);
	gtk_box_pack_start (GTK_BOX (vbox1),toolbar1,FALSE,FALSE,0);
	gtk_toolbar_set_style (GTK_TOOLBAR (toolbar1),GTK_TOOLBAR_ICONS);
	tmp_toolbar_icon_size = gtk_toolbar_get_icon_size (GTK_TOOLBAR (toolbar1));

	tmp_image = gtk_image_new_from_stock ("gtk-new",tmp_toolbar_icon_size);
	gtk_widget_show (tmp_image);
	New_button = (GtkWidget*) gtk_tool_button_new (tmp_image,"");
	gtk_widget_show (New_button);
	gtk_container_add (GTK_CONTAINER (toolbar1),New_button);
	gtk_tool_item_set_tooltip (GTK_TOOL_ITEM (New_button),tooltips,_("Create a new archive"),NULL);

	tmp_image = gtk_image_new_from_stock ("gtk-open",tmp_toolbar_icon_size);
	gtk_widget_show (tmp_image);
	Open_button = (GtkWidget*) gtk_tool_button_new (tmp_image,"");
	gtk_widget_show (Open_button);
	gtk_container_add (GTK_CONTAINER (toolbar1),Open_button);
	gtk_tool_item_set_tooltip (GTK_TOOL_ITEM (Open_button),tooltips,_("Open an archive"),NULL);

	separatortoolitem1 = (GtkWidget*) gtk_separator_tool_item_new ();
	gtk_widget_show (separatortoolitem1);
	gtk_container_add (GTK_CONTAINER (toolbar1),separatortoolitem1);

	tmp_image = gtk_image_new_from_stock ("gtk-go-back",tmp_toolbar_icon_size);
	gtk_widget_show (tmp_image);
	back_button = (GtkWidget*) gtk_tool_button_new (tmp_image,"");
	gtk_widget_set_sensitive(back_button,FALSE);
	gtk_widget_show (back_button);
	gtk_container_add (GTK_CONTAINER (toolbar1),back_button);
	gtk_tool_item_set_tooltip (GTK_TOOL_ITEM (back_button),tooltips,_("Back"),NULL);

	tmp_image = gtk_image_new_from_stock ("gtk-go-up",tmp_toolbar_icon_size);
	gtk_widget_show (tmp_image);
	up_button = (GtkWidget*) gtk_tool_button_new (tmp_image,"");
	gtk_widget_set_sensitive(up_button,FALSE);
	gtk_widget_show (up_button);
	gtk_container_add (GTK_CONTAINER (toolbar1),up_button);
	gtk_tool_item_set_tooltip (GTK_TOOL_ITEM (up_button),tooltips,_("Up"),NULL);

	tmp_image = gtk_image_new_from_stock ("gtk-go-forward",tmp_toolbar_icon_size);
	gtk_widget_show (tmp_image);
	forward_button = (GtkWidget*) gtk_tool_button_new (tmp_image,"");
	gtk_widget_set_sensitive(forward_button,FALSE);
	gtk_widget_show (forward_button);
	gtk_container_add (GTK_CONTAINER (toolbar1),forward_button);
	gtk_tool_item_set_tooltip (GTK_TOOL_ITEM (forward_button),tooltips,_("Forward"),NULL);

	tmp_image = gtk_image_new_from_stock ("gtk-home",tmp_toolbar_icon_size);
	gtk_widget_show (tmp_image);
	home_button = (GtkWidget*) gtk_tool_button_new (tmp_image,"");
	gtk_widget_set_sensitive(home_button,FALSE);
	gtk_widget_show (home_button);
	gtk_container_add (GTK_CONTAINER (toolbar1),home_button);
	gtk_tool_item_set_tooltip (GTK_TOOL_ITEM (home_button),tooltips,_("Root"),NULL);

	separatortoolitem3 = (GtkWidget*) gtk_separator_tool_item_new ();
	gtk_widget_show (separatortoolitem3);
	gtk_container_add (GTK_CONTAINER (toolbar1),separatortoolitem3);

	tmp_image = xa_main_window_find_image("xarchiver-add.png",GTK_ICON_SIZE_LARGE_TOOLBAR);
	gtk_widget_show (tmp_image);
	AddFile_button = (GtkWidget*) gtk_tool_button_new (tmp_image,"");
	gtk_widget_set_sensitive (AddFile_button,FALSE);
	gtk_widget_show (AddFile_button);
	gtk_container_add (GTK_CONTAINER (toolbar1),AddFile_button);
	gtk_tool_item_set_tooltip (GTK_TOOL_ITEM (AddFile_button),tooltips,_("Add files"),NULL);

	tmp_image = xa_main_window_find_image("xarchiver-extract.png",GTK_ICON_SIZE_LARGE_TOOLBAR);
	gtk_widget_show (tmp_image);
	Extract_button = (GtkWidget*) gtk_tool_button_new (tmp_image,"");
	gtk_widget_set_sensitive (Extract_button,FALSE);
	gtk_widget_show (Extract_button);
	gtk_container_add (GTK_CONTAINER (toolbar1),Extract_button);
	gtk_tool_item_set_tooltip (GTK_TOOL_ITEM (Extract_button),tooltips,_("Extract files"),NULL);

	separatortoolitem2 = (GtkWidget*) gtk_separator_tool_item_new ();
	gtk_widget_show (separatortoolitem2);
	gtk_container_add (GTK_CONTAINER (toolbar1),separatortoolitem2);

	tmp_image = gtk_image_new_from_stock ("gtk-stop",tmp_toolbar_icon_size);
	gtk_widget_show (tmp_image);
	Stop_button = (GtkWidget*) gtk_tool_button_new (tmp_image,"");
	gtk_widget_set_sensitive (Stop_button,FALSE);
	gtk_widget_show (Stop_button);
	gtk_container_add (GTK_CONTAINER (toolbar1),Stop_button);
	gtk_tool_item_set_tooltip (GTK_TOOL_ITEM (Stop_button),tooltips,_("Cancel current operation"),NULL);

	/* Location entry */
	toolbar2 = gtk_toolbar_new ();
	gtk_box_pack_start (GTK_BOX (vbox1),toolbar2,FALSE,FALSE,0);

	toolitem1 = (GtkWidget*)gtk_tool_item_new();
	gtk_tool_item_set_expand(GTK_TOOL_ITEM(toolitem1),TRUE);
  	gtk_container_add (GTK_CONTAINER (toolbar2),toolitem1);

	hbox1 = gtk_hbox_new(FALSE,2);
	gtk_container_add(GTK_CONTAINER (toolitem1),hbox1);

	location_label = gtk_label_new(_("Location:"));
	gtk_box_pack_start (GTK_BOX (hbox1),location_label,FALSE,FALSE,1);

	location_entry = gtk_entry_new();
	gtk_box_pack_start (GTK_BOX (hbox1),location_entry,TRUE,TRUE,0);
	g_signal_connect (G_OBJECT (location_entry),"activate",	G_CALLBACK (xa_location_entry_activated),NULL);
	if (show_location)
		gtk_widget_show_all(toolbar2);

	/* Create the sidepane */
	hpaned1 = gtk_hpaned_new ();
	gtk_widget_show (hpaned1);
  	gtk_box_pack_start (GTK_BOX (vbox1),hpaned1,TRUE,TRUE,0);

  	scrolledwindow2 = gtk_scrolled_window_new (NULL,NULL);
  	if (show_sidebar)
  		gtk_widget_show (scrolledwindow2);
  	else
  		gtk_widget_hide (scrolledwindow2);
  	gtk_paned_pack1 (GTK_PANED (hpaned1),scrolledwindow2,FALSE,TRUE);
	g_object_set (G_OBJECT (scrolledwindow2),"hscrollbar-policy",GTK_POLICY_AUTOMATIC,"shadow-type",GTK_SHADOW_IN,"vscrollbar-policy",GTK_POLICY_AUTOMATIC,NULL);

  	archive_dir_model = gtk_tree_store_new (3,G_TYPE_STRING,G_TYPE_STRING,G_TYPE_POINTER);
	archive_dir_treeview = gtk_tree_view_new_with_model(GTK_TREE_MODEL(archive_dir_model));
	gtk_container_add (GTK_CONTAINER (scrolledwindow2),archive_dir_treeview);
	gtk_widget_show(archive_dir_treeview);
	gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(archive_dir_model),1,GTK_SORT_ASCENDING);
	gtk_tree_view_enable_model_drag_dest(GTK_TREE_VIEW(archive_dir_treeview),drop_targets,1,GDK_ACTION_COPY | GDK_ACTION_MOVE | GDK_ACTION_LINK | GDK_ACTION_ASK);
	g_signal_connect (G_OBJECT (archive_dir_treeview),"row-collapsed",G_CALLBACK(xa_sidepane_row_expanded),archive_dir_model);
	g_signal_connect (G_OBJECT (archive_dir_treeview),"row-expanded",G_CALLBACK(xa_sidepane_row_expanded),archive_dir_model);
	g_signal_connect (G_OBJECT (archive_dir_treeview),"drag-data-received",G_CALLBACK (xa_sidepane_drag_data_received),NULL);
	g_signal_connect (G_OBJECT (archive_dir_treeview),"drag-motion",G_CALLBACK (xa_sidepane_drag_motion),NULL);
	GtkTreeSelection *sel = gtk_tree_view_get_selection(GTK_TREE_VIEW (archive_dir_treeview));
	g_signal_connect (sel,"changed",G_CALLBACK (xa_sidepane_row_selected),NULL);

	column = gtk_tree_view_column_new();
	archive_dir_renderer = gtk_cell_renderer_pixbuf_new();
	gtk_tree_view_column_pack_start(column,archive_dir_renderer,FALSE);
	gtk_tree_view_column_set_attributes(column,archive_dir_renderer,"stock-id",0,NULL);
	gtk_tree_view_column_set_title(column,_("Archive tree"));

	archive_dir_renderer = gtk_cell_renderer_text_new();
	gtk_tree_view_column_pack_start(column,archive_dir_renderer,TRUE);
	gtk_tree_view_column_set_attributes(column,archive_dir_renderer,"text",1,NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW (archive_dir_treeview),column);

	/* Create the notebook widget */
	notebook = GTK_NOTEBOOK(gtk_notebook_new());
	gtk_paned_pack2(GTK_PANED (hpaned1),GTK_WIDGET(notebook),TRUE,TRUE);
	gtk_notebook_set_tab_pos (notebook,GTK_POS_TOP);
	gtk_notebook_set_scrollable (notebook,TRUE);
	gtk_notebook_popup_enable (notebook);
	gtk_widget_show (GTK_WIDGET(notebook));
	g_signal_connect ((gpointer) notebook,"switch-page",G_CALLBACK (xa_page_has_changed),NULL);

	gtk_drag_dest_set (GTK_WIDGET(notebook),GTK_DEST_DEFAULT_ALL,drop_targets,1,GDK_ACTION_COPY | GDK_ACTION_MOVE | GDK_ACTION_LINK | GDK_ACTION_ASK);
	g_signal_connect (G_OBJECT (notebook),"drag-data-received",G_CALLBACK (on_drag_data_received),NULL);

  	hbox_sb = gtk_hbox_new (FALSE,2);
	gtk_widget_show (hbox_sb);
	gtk_box_pack_end (GTK_BOX (vbox1),hbox_sb,FALSE,TRUE,0);

	total_frame = gtk_frame_new (NULL);
	gtk_widget_show (total_frame);
	gtk_box_pack_start (GTK_BOX (hbox_sb),total_frame,TRUE,TRUE,0);
	gtk_frame_set_label_align (GTK_FRAME (total_frame),0,0);
	gtk_frame_set_shadow_type (GTK_FRAME (total_frame),GTK_SHADOW_IN);

	total_label = gtk_label_new (NULL);
	gtk_misc_set_alignment (GTK_MISC(total_label),0.0,0.5);
	gtk_widget_show (total_label);
	gtk_container_add (GTK_CONTAINER (total_frame),total_label);
	
	selected_frame = gtk_frame_new (NULL);
	gtk_box_pack_start (GTK_BOX (hbox_sb),selected_frame,TRUE,TRUE,0);
	gtk_frame_set_label_align (GTK_FRAME (selected_frame),0,0);
	gtk_frame_set_shadow_type (GTK_FRAME (selected_frame),GTK_SHADOW_IN);

	selected_label = gtk_label_new (NULL);
	gtk_misc_set_alignment (GTK_MISC(selected_label),0.0,0.5);
	gtk_widget_show (selected_label);
	gtk_container_add (GTK_CONTAINER (selected_frame),selected_label);
	
	green_led = gtk_image_new_from_icon_name ("gtk-yes",GTK_ICON_SIZE_BUTTON);
	gtk_widget_show (green_led);
	gtk_box_pack_start (GTK_BOX (hbox_sb),green_led,FALSE,FALSE,0);
	gtk_misc_set_alignment (GTK_MISC (green_led),1,1);
	gtk_tooltips_set_tip (tooltips,green_led,_("This is Xarchiver led status. When it's flashing Xarchiver is busy"),NULL);

	red_led = gtk_image_new_from_icon_name ("gtk-no",GTK_ICON_SIZE_BUTTON);
	gtk_box_pack_start (GTK_BOX (hbox_sb),red_led,FALSE,FALSE,0);
	gtk_misc_set_alignment (GTK_MISC (red_led),1,1);

	g_signal_connect ((gpointer) new1,"activate",G_CALLBACK (xa_new_archive),NULL);
	g_signal_connect ((gpointer) open1,"activate",G_CALLBACK (xa_open_archive),NULL);
	g_signal_connect ((gpointer) listing_text,"activate",G_CALLBACK (xa_list_archive),(gpointer) 0);
	g_signal_connect ((gpointer) listing_html,"activate",G_CALLBACK (xa_list_archive),(gpointer) 1);
	g_signal_connect ((gpointer) save1,"activate",G_CALLBACK (xa_save_archive),NULL);
	g_signal_connect ((gpointer) check_menu,"activate",G_CALLBACK (xa_test_archive),NULL);
	g_signal_connect ((gpointer) properties,"activate",G_CALLBACK (xa_archive_properties),NULL);
	g_signal_connect ((gpointer) extract_menu,"activate",G_CALLBACK (xa_extract_archive),NULL);
	g_signal_connect ((gpointer) exe_menu,"activate",G_CALLBACK (xa_convert_sfx),NULL);
	g_signal_connect ((gpointer) addfile,"activate",G_CALLBACK (xa_add_files_archive),NULL);
	g_signal_connect ((gpointer) view_shell_output1,"activate",G_CALLBACK (xa_show_cmd_line_output),NULL);
	g_signal_connect ((gpointer) select_all,"activate",G_CALLBACK (xa_select_all),NULL);
	g_signal_connect ((gpointer) deselect_all,"activate",G_CALLBACK (xa_deselect_all),NULL);
	g_signal_connect ((gpointer) select_pattern,"activate",G_CALLBACK (xa_select_by_pattern_dialog),NULL);
	g_signal_connect ((gpointer) password_entry_menu,"activate",G_CALLBACK (xa_enter_password),NULL);
	g_signal_connect ((gpointer) prefs_menu,"activate",G_CALLBACK (xa_show_prefs_dialog),NULL);
	g_signal_connect ((gpointer) close1,"activate",G_CALLBACK (xa_close_archive),NULL);
	g_signal_connect ((gpointer) quit1,"activate",G_CALLBACK (xa_quit_application),NULL);
	g_signal_connect ((gpointer) delete_menu,"activate",G_CALLBACK (xa_delete_archive),NULL);
	g_signal_connect ((gpointer) rename_menu,"activate",G_CALLBACK (xa_rename_archive),NULL);
	g_signal_connect ((gpointer) comment_menu,"activate",G_CALLBACK (xa_show_archive_comment),NULL);
	g_signal_connect ((gpointer) multi_extract_menu,"activate",G_CALLBACK (xa_show_multi_extract_dialog),NULL);
	g_signal_connect ((gpointer) help1,"activate",G_CALLBACK (xa_show_help),NULL);
	g_signal_connect ((gpointer) about1,"activate",G_CALLBACK (xa_about),NULL);
	g_signal_connect ((gpointer) donate,"activate",G_CALLBACK (xa_show_donate_page_on_the_web),NULL);

	g_signal_connect ((gpointer) New_button,	"clicked",G_CALLBACK (xa_new_archive),NULL);
	g_signal_connect ((gpointer) Open_button,	"clicked",G_CALLBACK (xa_open_archive),NULL);

	g_signal_connect ((gpointer) back_button,	"clicked",G_CALLBACK (xa_handle_navigation_buttons),(gpointer) 1);
	g_signal_connect ((gpointer) up_button,		"clicked",G_CALLBACK (xa_handle_navigation_buttons),(gpointer) 2);
	g_signal_connect ((gpointer) forward_button,"clicked",G_CALLBACK (xa_handle_navigation_buttons),(gpointer) 3);
	g_signal_connect ((gpointer) home_button,	"clicked",G_CALLBACK (xa_handle_navigation_buttons),(gpointer) 0);

	g_signal_connect ((gpointer) AddFile_button,"clicked",G_CALLBACK (xa_add_files_archive),NULL);
    g_signal_connect ((gpointer) Extract_button,"clicked",G_CALLBACK (xa_extract_archive),	NULL);
	g_signal_connect ((gpointer) Stop_button,	"clicked",G_CALLBACK (xa_cancel_archive),	NULL);
	gtk_window_add_accel_group (GTK_WINDOW (xa_main_window),accel_group);
}

gboolean xa_flash_led_indicator (XArchive *archive)
{
	if (archive->child_pid == 0)
	{
		gtk_widget_show(green_led);
		gtk_widget_hide(red_led);
		archive->pb_source = 0;
		return FALSE;
	}
	if (GTK_WIDGET_VISIBLE(green_led))
	{
		gtk_widget_hide(green_led);
		gtk_widget_show(red_led);
	}
	else
	{
		gtk_widget_show(green_led);
		gtk_widget_hide(red_led);
	}
	return TRUE;
}

void xa_page_has_changed (GtkNotebook *notebook,GtkNotebookPage *page,guint page_num,gpointer user_data)
{
	gint id,selected = 0;
	GtkTreeSelection *selection = NULL;

	id = xa_find_archive_index (page_num);
	if (id == -1)
		return;

	xa_set_window_title (xa_main_window,archive[id]->path);
	xa_restore_navigation(id);
	xa_set_statusbar_message_for_displayed_rows(archive[id]);

	if (selection != NULL)
		xa_row_selected(selection,archive[id]);

	if (archive[id]->type == XARCHIVETYPE_7ZIP || archive[id]->type == XARCHIVETYPE_ZIP || archive[id]->type == XARCHIVETYPE_RAR || archive[id]->type == XARCHIVETYPE_ARJ)
		gtk_widget_set_sensitive (comment_menu,TRUE);
	else
		gtk_widget_set_sensitive (comment_menu,FALSE);

	if (archive[id]->type == XARCHIVETYPE_TAR || is_tar_compressed(archive[id]->type))
			gtk_widget_set_sensitive (password_entry_menu,FALSE);
		else
			gtk_widget_set_sensitive (password_entry_menu,TRUE);

	if (archive[id]->treeview != NULL)
	{
		selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (archive[id]->treeview));
		selected = gtk_tree_selection_count_selected_rows (selection);
		if (selected == 0)
		{
			xa_disable_delete_buttons (FALSE);
			gtk_widget_hide(selected_frame);
		}
		else
		{
			gtk_widget_show(selected_frame);
			gtk_widget_set_sensitive(deselect_all,TRUE);
			if (archive[id]->type == XARCHIVETYPE_RAR && unrar)
			{
				gtk_widget_set_sensitive (delete_menu,FALSE);
				gtk_widget_set_sensitive (rename_menu,FALSE);
			}
			else if ( archive[id]->type != XARCHIVETYPE_RPM && archive[id]->type != XARCHIVETYPE_DEB)
			{
				gtk_widget_set_sensitive (delete_menu,TRUE);
				gtk_widget_set_sensitive (rename_menu,TRUE);
			}
		}
		/* Let's set the location bar */
		if (archive[id]->location_entry_path != NULL)
			gtk_entry_set_text(GTK_ENTRY(location_entry),archive[id]->location_entry_path);
		else
			gtk_entry_set_text(GTK_ENTRY(location_entry),"\0");

		if (GTK_IS_TREE_VIEW(archive[id]->treeview))
			gtk_widget_grab_focus (GTK_WIDGET(archive[id]->treeview));
		xa_fill_dir_sidebar(archive[id],TRUE);
	}
}

void xa_add_page (XArchive *archive)
{
	GtkWidget	*page_hbox,*label,*tab_label,*close_button,*image,*align;
	GtkTooltips *close_button_tips = gtk_tooltips_new();
	gchar *filename_only;
	GtkRcStyle *rcstyle;
	GtkRequisition size;

	if (gtk_notebook_get_current_page(notebook) > -1)
		gtk_notebook_set_show_tabs (notebook,TRUE);
	else
		gtk_notebook_set_show_tabs (notebook,FALSE);

	archive->scrollwindow = gtk_scrolled_window_new (NULL,NULL);
	g_object_set (G_OBJECT (archive->scrollwindow),"hscrollbar-policy",GTK_POLICY_AUTOMATIC,"vscrollbar-policy",GTK_POLICY_AUTOMATIC,NULL);
	gtk_widget_show (archive->scrollwindow);

	page_hbox = gtk_hbox_new(FALSE,0);

	filename_only = g_strrstr ( archive->path,"/");
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

	gtk_label_set_max_width_chars(GTK_LABEL(label),50);
	gtk_label_set_ellipsize(GTK_LABEL(label),PANGO_ELLIPSIZE_START);
	gtk_box_pack_start(GTK_BOX(page_hbox),label,FALSE,FALSE,0);

	close_button = gtk_button_new();
	gtk_button_set_focus_on_click(GTK_BUTTON(close_button),FALSE);
	gtk_button_set_relief (GTK_BUTTON(close_button),GTK_RELIEF_NONE);
	gtk_tooltips_set_tip (close_button_tips,close_button,_("Close archive"),NULL);
	g_signal_connect (G_OBJECT(close_button),"clicked",G_CALLBACK(xa_close_page),(gpointer) archive->scrollwindow);

	rcstyle = gtk_rc_style_new();
	rcstyle->xthickness = rcstyle->ythickness = 0;
	gtk_widget_modify_style(close_button,rcstyle);
	gtk_rc_style_unref(rcstyle);

	image = gtk_image_new_from_stock (GTK_STOCK_CLOSE,GTK_ICON_SIZE_MENU);
	gtk_widget_size_request(image,&size);
	gtk_widget_set_size_request(close_button,size.width,size.height);
	gtk_container_add (GTK_CONTAINER(close_button),image);
	align = gtk_alignment_new(1.0,0.0,0.0,0.0);
	gtk_container_add(GTK_CONTAINER(align),close_button);
	gtk_box_pack_start(GTK_BOX(page_hbox),align,TRUE,TRUE,0);
	gtk_widget_show_all(page_hbox);

	gtk_misc_set_alignment(GTK_MISC(tab_label),0.0,0);
	gtk_notebook_append_page_menu (notebook,archive->scrollwindow,page_hbox,tab_label);
	gtk_notebook_set_current_page(notebook,-1);
	gtk_notebook_set_tab_reorderable(notebook,archive->scrollwindow,TRUE);

	archive->treeview = gtk_tree_view_new ();
	gtk_container_add (GTK_CONTAINER (archive->scrollwindow),archive->treeview);
	gtk_widget_show (archive->treeview);
	gtk_tree_view_set_rules_hint ( GTK_TREE_VIEW (archive->treeview),TRUE);
	gtk_tree_view_set_search_equal_func (GTK_TREE_VIEW (archive->treeview),(GtkTreeViewSearchEqualFunc) treeview_select_search,NULL,NULL);
	GtkTreeSelection *sel = gtk_tree_view_get_selection( GTK_TREE_VIEW (archive->treeview));
	gtk_tree_selection_set_mode(sel,GTK_SELECTION_MULTIPLE);
	gtk_tree_view_set_rubber_banding(GTK_TREE_VIEW(archive->treeview),TRUE);

	gtk_drag_source_set (archive->treeview,GDK_BUTTON1_MASK,drag_targets,1,GDK_ACTION_COPY | GDK_ACTION_MOVE | GDK_ACTION_LINK | GDK_ACTION_ASK);
	g_signal_connect ((gpointer) sel,			   "changed",		G_CALLBACK (xa_row_selected),archive);
	g_signal_connect (G_OBJECT (archive->treeview),"drag-begin",	G_CALLBACK (drag_begin),archive);
	g_signal_connect (G_OBJECT (archive->treeview),"drag-data-get", G_CALLBACK (drag_data_get),archive);
	g_signal_connect (G_OBJECT (archive->treeview),"drag-end",		G_CALLBACK (drag_end),NULL);
	g_signal_connect (G_OBJECT (archive->treeview),"row-activated", G_CALLBACK (xa_treeview_row_activated),archive);
	g_signal_connect (G_OBJECT (archive->treeview),"button-press-event",G_CALLBACK (xa_mouse_button_event),archive);
}

void xa_close_page (GtkWidget *widget,gpointer data)
{
	xa_close_archive (NULL,data);
}

gchar *xa_create_password_dialog(XArchive *archive)
{
	GtkWidget *password_dialog,*dialog_vbox1,*vbox1,*hbox2,*image2,*vbox2,*label_pwd_required,*label_filename,*hbox1,*label34,*pw_password_entry;
	gchar *password = NULL;
	gchar *name;
	gboolean done = FALSE;
	int response;

  	password_dialog = gtk_dialog_new_with_buttons ("Xarchiver " VERSION,
									GTK_WINDOW (xa_main_window),GTK_DIALOG_DESTROY_WITH_PARENT,
									GTK_STOCK_CANCEL,GTK_RESPONSE_CANCEL,GTK_STOCK_OK,GTK_RESPONSE_OK,NULL);

  	gtk_container_set_border_width (GTK_CONTAINER (password_dialog),6);
  	gtk_window_set_position (GTK_WINDOW (password_dialog),GTK_WIN_POS_CENTER_ON_PARENT);
  	gtk_window_set_resizable (GTK_WINDOW (password_dialog),FALSE);
  	gtk_window_set_type_hint (GTK_WINDOW (password_dialog),GDK_WINDOW_TYPE_HINT_DIALOG);
  	gtk_dialog_set_has_separator (GTK_DIALOG (password_dialog),FALSE);

  	dialog_vbox1 = GTK_DIALOG (password_dialog)->vbox;
  	gtk_widget_show (dialog_vbox1);

  	vbox1 = gtk_vbox_new (FALSE,12);
  	gtk_widget_show (vbox1);
  	gtk_box_pack_start (GTK_BOX (dialog_vbox1),vbox1,TRUE,TRUE,0);
  	gtk_container_set_border_width (GTK_CONTAINER (vbox1),6);

  	hbox2 = gtk_hbox_new (FALSE,12);
  	gtk_widget_show (hbox2);
  	gtk_box_pack_start (GTK_BOX (vbox1),hbox2,TRUE,TRUE,0);

  	image2 = gtk_image_new_from_stock ("gtk-dialog-authentication",GTK_ICON_SIZE_DIALOG);
  	gtk_widget_show (image2);
  	gtk_box_pack_start (GTK_BOX (hbox2),image2,FALSE,TRUE,0);
  	gtk_misc_set_alignment (GTK_MISC (image2),0,0);

  	vbox2 = gtk_vbox_new (FALSE,0);
  	gtk_widget_show (vbox2);
  	gtk_box_pack_start (GTK_BOX (hbox2),vbox2,TRUE,TRUE,0);

  	if (batch_mode)
  		label_pwd_required = gtk_label_new (_("<span weight='bold' size='larger'>Password required for:</span>"));
  	else
		label_pwd_required = gtk_label_new (_("<span weight='bold' size='larger'>Enter password for:</span>"));  	

  	gtk_widget_show (label_pwd_required);
  	gtk_box_pack_start (GTK_BOX (vbox2),label_pwd_required,FALSE,FALSE,0);
  	gtk_label_set_use_markup (GTK_LABEL (label_pwd_required),TRUE);
  	gtk_misc_set_alignment (GTK_MISC (label_pwd_required),0,0.5);

  	name = xa_remove_path_from_archive_name(archive->path);
  	label_filename = gtk_label_new (name);
	g_free (name);
  	gtk_widget_show (label_filename);
  	gtk_box_pack_start (GTK_BOX (vbox2),label_filename,FALSE,FALSE,12);
  	gtk_misc_set_alignment (GTK_MISC (label_filename),0,0.5);
	
  	hbox1 = gtk_hbox_new (FALSE,5);
  	gtk_widget_show (hbox1);
  	gtk_box_pack_start (GTK_BOX (vbox2),hbox1,TRUE,TRUE,0);

  	label34 = gtk_label_new_with_mnemonic (_("_Password:"));
  	gtk_widget_show (label34);
  	gtk_box_pack_start (GTK_BOX (hbox1),label34,FALSE,FALSE,0);

  	pw_password_entry = gtk_entry_new ();
  	gtk_widget_show (pw_password_entry);
  	gtk_box_pack_start (GTK_BOX (hbox1),pw_password_entry,TRUE,TRUE,0);
  	gtk_entry_set_visibility (GTK_ENTRY (pw_password_entry),FALSE);
  	gtk_entry_set_invisible_char (GTK_ENTRY (pw_password_entry),9679);
  	gtk_entry_set_activates_default (GTK_ENTRY (pw_password_entry),TRUE);
  	gtk_dialog_set_default_response (GTK_DIALOG (password_dialog),GTK_RESPONSE_OK);

	while (! done)
	{
		switch (gtk_dialog_run (GTK_DIALOG(password_dialog)))
		{
			case GTK_RESPONSE_CANCEL:
			case GTK_RESPONSE_DELETE_EVENT:
			done = TRUE;
			password = NULL;
			break;

			case GTK_RESPONSE_OK:
			password = g_strdup (gtk_entry_get_text(GTK_ENTRY(pw_password_entry)));
			if (strlen(password) == 0)
			{
				response = xa_show_message_dialog (GTK_WINDOW (xa_main_window),GTK_DIALOG_MODAL,GTK_MESSAGE_ERROR,GTK_BUTTONS_OK,_("You missed the password!"),_("Please enter it!"));
				break;
			}
			done = TRUE;
			break;
		}
	}
	gtk_widget_destroy (password_dialog);
	return password;
}

void xa_show_donate_page_on_the_web(GtkMenuItem *item,gpointer data)
{
	xa_activate_link (NULL,"http://xarchiver.xfce.org/contribute.html",NULL);
}

void xa_create_popup_menu()
{
	GtkWidget *image6;
	GtkWidget *image7;
	GtkWidget *image8;
	GtkWidget *image1;
	GtkWidget *separator;
	GtkWidget *extract;
	GtkWidget *image9;
	GtkWidget *image10;
	GtkWidget *image11;

	xa_popup_menu = gtk_menu_new();
	open_popupmenu = gtk_image_menu_item_new_with_mnemonic (_("Open With"));
	gtk_widget_show (open_popupmenu);
	gtk_container_add (GTK_CONTAINER (xa_popup_menu),open_popupmenu);

	image9 = gtk_image_new_from_stock ("gtk-open",GTK_ICON_SIZE_MENU);
	gtk_widget_show (image9);
	gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (open_popupmenu),image9);

	extract = gtk_image_menu_item_new_with_mnemonic (_("Extract"));
	gtk_widget_show (extract);
	gtk_container_add (GTK_CONTAINER (xa_popup_menu),extract);

	image9 =  xa_main_window_find_image ("xarchiver-extract.png",GTK_ICON_SIZE_MENU);
	gtk_widget_show (image9);
	gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (extract),image9);

	view = gtk_image_menu_item_new_with_mnemonic (_("View"));
	gtk_widget_show (view);
	gtk_container_add (GTK_CONTAINER (xa_popup_menu),view);

	image1 = gtk_image_new_from_stock ("gtk-find",GTK_ICON_SIZE_MENU);
	gtk_widget_show (image1);
	gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (view),image1);

	separator = gtk_separator_menu_item_new ();
	gtk_widget_show (separator);
	gtk_container_add (GTK_CONTAINER (xa_popup_menu),separator);
	gtk_widget_set_sensitive (separator,FALSE);

	cut = gtk_image_menu_item_new_with_mnemonic (_("Cut"));
	gtk_widget_show (cut);
	gtk_container_add (GTK_CONTAINER (xa_popup_menu),cut);

	image6 = gtk_image_new_from_stock ("gtk-cut",GTK_ICON_SIZE_MENU);
	gtk_widget_show (image6);
	gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (cut),image6);

	copy = gtk_image_menu_item_new_with_mnemonic (_("Copy"));
	gtk_widget_show (copy);
	gtk_container_add (GTK_CONTAINER (xa_popup_menu),copy);

	image7 = gtk_image_new_from_stock ("gtk-copy",GTK_ICON_SIZE_MENU);
	gtk_widget_show (image7);
	gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (copy),image7);

	paste = gtk_image_menu_item_new_with_mnemonic (_("Paste"));
	gtk_widget_set_sensitive(paste,FALSE);
	gtk_widget_show (paste);
	gtk_container_add (GTK_CONTAINER (xa_popup_menu),paste);

	image8 = gtk_image_new_from_stock ("gtk-paste",GTK_ICON_SIZE_MENU);
	gtk_widget_show (image8);
	gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (paste),image8);

	separator = gtk_separator_menu_item_new();
	gtk_widget_show (separator);
	gtk_container_add (GTK_CONTAINER (xa_popup_menu),separator);
	gtk_widget_set_sensitive (separator,FALSE);

	ddelete = gtk_image_menu_item_new_with_mnemonic (_("Delete"));
	gtk_widget_set_sensitive (ddelete,FALSE);
	gtk_widget_show (ddelete);
	gtk_container_add (GTK_CONTAINER (xa_popup_menu),ddelete);

	image10 = gtk_image_new_from_stock ("gtk-delete",GTK_ICON_SIZE_MENU);
	gtk_widget_show (image10);
	gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (ddelete),image10);

	rrename = gtk_image_menu_item_new_with_mnemonic (_("Rename"));
	gtk_widget_show (rrename);
	gtk_container_add (GTK_CONTAINER (xa_popup_menu),rrename);

	image11 = gtk_image_new_from_stock ("gtk-refresh",GTK_ICON_SIZE_MENU);
	gtk_widget_show (image11);
	gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (rrename),image11);

	g_signal_connect ((gpointer) open_popupmenu,"activate",	G_CALLBACK(xa_open_with_from_popupmenu),NULL);
	g_signal_connect ((gpointer) view,	"activate",	G_CALLBACK(xa_view_from_popupmenu),NULL);
	g_signal_connect ((gpointer) cut,	"activate",	G_CALLBACK(xa_clipboard_cut),NULL);
	g_signal_connect ((gpointer) copy,	"activate",	G_CALLBACK(xa_clipboard_copy),NULL);
	g_signal_connect ((gpointer) paste,	"activate",	G_CALLBACK(xa_clipboard_paste),NULL);
	g_signal_connect ((gpointer) extract,"activate",G_CALLBACK(xa_extract_archive),NULL);
	g_signal_connect ((gpointer) ddelete,"activate",G_CALLBACK(xa_delete_archive),NULL);
	g_signal_connect ((gpointer) rrename,"activate",G_CALLBACK(xa_rename_archive),NULL);
}

void xa_select_by_pattern_dialog(GtkMenuItem *menuitem,gpointer user_data)
{
	GtkWidget *ddialog1;
	GtkWidget *ddialog_vbox1;
	GtkWidget *dhbox1;
	GtkWidget *pattern_label;
	GtkWidget *pattern_entry;
	GtkWidget *dialog_action_area1;
	GtkWidget *cancelbutton1;
	GtkWidget *okbutton1;
	GtkWidget *tmp_image,*select_hbox,*select_label;
	gchar *string;
  	gboolean done = FALSE;
	gint current_page;
	gint id;

	current_page = gtk_notebook_get_current_page (notebook);
	id = xa_find_archive_index (current_page);

  	GtkTooltips *tooltip = gtk_tooltips_new();
	ddialog1 = gtk_dialog_new ();
	gtk_window_set_title (GTK_WINDOW (ddialog1),_("Select by Pattern"));
	gtk_window_set_modal (GTK_WINDOW (ddialog1),TRUE);
	gtk_window_set_type_hint (GTK_WINDOW (ddialog1),GDK_WINDOW_TYPE_HINT_DIALOG);
	gtk_widget_set_size_request(ddialog1,286,93);
	gtk_window_set_transient_for (GTK_WINDOW (ddialog1),GTK_WINDOW (xa_main_window));
	ddialog_vbox1 = GTK_DIALOG (ddialog1)->vbox;
	gtk_widget_show (ddialog_vbox1);

	dhbox1 = gtk_hbox_new (FALSE,10);
	gtk_widget_show (dhbox1);
	gtk_box_pack_start (GTK_BOX (ddialog_vbox1),dhbox1,TRUE,TRUE,0);
	gtk_container_set_border_width (GTK_CONTAINER (dhbox1),5);

	pattern_label = gtk_label_new (_("Pattern:"));
	gtk_widget_show (pattern_label);
	gtk_box_pack_start (GTK_BOX (dhbox1),pattern_label,FALSE,FALSE,0);

	pattern_entry = gtk_entry_new ();
	gtk_tooltips_set_tip (tooltip,pattern_entry,_("example: *.txt; ac*"),NULL);
	gtk_widget_show (pattern_entry);
	gtk_box_pack_start (GTK_BOX (dhbox1),pattern_entry,TRUE,TRUE,0);
	gtk_entry_set_activates_default(GTK_ENTRY(pattern_entry),TRUE);

	dialog_action_area1 = GTK_DIALOG (ddialog1)->action_area;
	gtk_widget_show (dialog_action_area1);
	gtk_button_box_set_layout (GTK_BUTTON_BOX (dialog_action_area1),GTK_BUTTONBOX_END);

	cancelbutton1 = gtk_button_new_from_stock ("gtk-cancel");
	gtk_widget_show (cancelbutton1);
	gtk_dialog_add_action_widget (GTK_DIALOG (ddialog1),cancelbutton1,GTK_RESPONSE_CANCEL);
	GTK_WIDGET_SET_FLAGS (cancelbutton1,GTK_CAN_DEFAULT);

	okbutton1 = gtk_button_new();
	tmp_image = gtk_image_new_from_stock ("gtk-ok",GTK_ICON_SIZE_BUTTON);
	select_hbox = gtk_hbox_new(FALSE,4);
	select_label = gtk_label_new_with_mnemonic(_("_Select"));

	alignment2 = gtk_alignment_new (0.5,0.5,0,0);
	gtk_container_add (GTK_CONTAINER (alignment2),select_hbox);

	gtk_box_pack_start(GTK_BOX(select_hbox),tmp_image,FALSE,TRUE,0);
	gtk_box_pack_start(GTK_BOX(select_hbox),select_label,FALSE,TRUE,0);
	gtk_container_add(GTK_CONTAINER(okbutton1),alignment2);
	gtk_widget_show_all (okbutton1);
	gtk_dialog_add_action_widget (GTK_DIALOG (ddialog1),okbutton1,GTK_RESPONSE_OK);
	GTK_WIDGET_SET_FLAGS (okbutton1,GTK_CAN_DEFAULT);
	gtk_dialog_set_default_response (GTK_DIALOG (ddialog1),GTK_RESPONSE_OK);

	while (! done)
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
				goto destroy_delete_dialog;
			}
			gtk_widget_set_sensitive(deselect_all,TRUE);
			done = TRUE;
			break;
		}
	}
	gtk_tree_model_foreach(archive[id]->model,(GtkTreeModelForeachFunc)select_matched_rows,string);
	g_free(string);

destroy_delete_dialog:
	gtk_widget_destroy (ddialog1);
}

gboolean select_matched_rows(GtkTreeModel *model,GtkTreePath *path,GtkTreeIter *iter,gpointer data)
{
	gchar *string = data;
	gchar *utf8_name = NULL;
	char **patterns;
	XEntry *entry = NULL;
	gint current_page;
	gint idx;

	current_page = gtk_notebook_get_current_page(notebook);
	idx = xa_find_archive_index (current_page);
	patterns = g_strsplit(string,";",-1);

	gtk_tree_model_get (model,iter,archive[idx]->nc+1,&entry,-1);
	utf8_name = g_filename_to_utf8 (entry->filename,-1,NULL,NULL,NULL);

	if (match_patterns (patterns,utf8_name,0))
		gtk_tree_selection_select_iter(gtk_tree_view_get_selection (GTK_TREE_VIEW (archive[idx]->treeview)),iter);
	else
		gtk_tree_selection_unselect_iter(gtk_tree_view_get_selection (GTK_TREE_VIEW (archive[idx]->treeview)),iter);
	g_free (utf8_name);

	if (patterns != NULL)
		g_strfreev (patterns);

	return FALSE;
}

GtkWidget *xa_create_archive_properties_window()
{
	archive_properties_window = gtk_dialog_new_with_buttons (_("Archive Properties"),
									GTK_WINDOW (xa_main_window),GTK_DIALOG_DESTROY_WITH_PARENT,
									GTK_STOCK_CLOSE,GTK_RESPONSE_CANCEL,NULL);

	g_signal_connect(archive_properties_window,"response",G_CALLBACK(gtk_widget_destroy),NULL);
	g_signal_connect(archive_properties_window,"delete-event",G_CALLBACK(gtk_widget_destroy),NULL);

	gtk_container_set_border_width (GTK_CONTAINER (archive_properties_window),6);
	gtk_window_set_position (GTK_WINDOW (archive_properties_window),GTK_WIN_POS_CENTER_ON_PARENT);
	gtk_window_set_type_hint (GTK_WINDOW (archive_properties_window),GDK_WINDOW_TYPE_HINT_DIALOG);
	gtk_window_set_resizable (GTK_WINDOW (archive_properties_window),FALSE);
	gtk_window_set_modal (GTK_WINDOW (archive_properties_window),TRUE);
	gtk_dialog_set_has_separator(GTK_DIALOG(archive_properties_window),FALSE);

	table1 = gtk_table_new (10,2,FALSE);
	gtk_box_pack_start (GTK_BOX (GTK_DIALOG(archive_properties_window)->vbox),table1,TRUE,TRUE,0);
	gtk_container_set_border_width (GTK_CONTAINER (table1),6);
	gtk_table_set_row_spacings (GTK_TABLE (table1),6);
	gtk_table_set_col_spacings (GTK_TABLE (table1),12);

	name_label = gtk_label_new ("");
	set_label ( name_label,_("Name:"));
	gtk_table_attach (GTK_TABLE (table1),name_label,0,1,0,1,
                     (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0),0,0);
	gtk_misc_set_alignment (GTK_MISC (name_label),1,0.5);

	path_label = gtk_label_new ("");
	set_label ( path_label,_("Path:"));
	gtk_table_attach (GTK_TABLE (table1),path_label,0,1,1,2,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0),0,0);
	gtk_misc_set_alignment (GTK_MISC (path_label),1,0.5);

	type_label = gtk_label_new ("");
	set_label ( type_label,_("Type:"));
	gtk_table_attach (GTK_TABLE (table1),type_label,0,1,2,3,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0),0,0);
	gtk_misc_set_alignment (GTK_MISC (type_label),1,0.5);

	encrypted_label = gtk_label_new ("");
	set_label ( encrypted_label,_("Encrypted:"));
	gtk_table_attach (GTK_TABLE (table1),encrypted_label,0,1,3,4,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0),0,0);
	gtk_misc_set_alignment (GTK_MISC (encrypted_label),0.99,0.5);

	modified_label = gtk_label_new ("");
	set_label ( modified_label,_("Modified on:"));
	gtk_table_attach (GTK_TABLE (table1),modified_label,0,1,4,5,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0),0,0);
	gtk_misc_set_alignment (GTK_MISC (modified_label),1,0.5);

	size_label = gtk_label_new ("");
	set_label ( size_label,_("Compressed size:"));
	gtk_table_attach (GTK_TABLE (table1),size_label,0,1,5,6,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0),0,0);
	gtk_misc_set_alignment (GTK_MISC (size_label),0.99,0.5);

	content_label = gtk_label_new ("");
	set_label ( content_label,_("Uncompressed size:"));
	gtk_table_attach (GTK_TABLE (table1),content_label,0,1,6,7,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0),0,0);
	gtk_misc_set_alignment (GTK_MISC (content_label),0.99,0.5);

	comment_label = gtk_label_new ("");
	set_label ( comment_label,_("Comment:"));
	gtk_table_attach (GTK_TABLE (table1),comment_label,0,1,7,8,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0),0,0);
	gtk_misc_set_alignment (GTK_MISC (comment_label),0.99,0.5);

	number_of_files_label = gtk_label_new ("");
	set_label ( number_of_files_label,_("Number of files:"));
	gtk_table_attach (GTK_TABLE (table1),number_of_files_label,0,1,8,9,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0),0,0);
	gtk_misc_set_alignment (GTK_MISC (number_of_files_label),0.99,0.5);
	
	compression_label = gtk_label_new ("");
	set_label ( compression_label,_("Compression ratio:"));
	gtk_table_attach (GTK_TABLE (table1),compression_label,0,1,9,10,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0),0,0);
	gtk_misc_set_alignment (GTK_MISC (compression_label),0.99,0.5);
	
/* */
	compression_data = gtk_label_new("");
	gtk_misc_set_alignment (GTK_MISC (compression_data),0,0.5);
	gtk_table_attach (GTK_TABLE (table1),compression_data,1,2,9,10,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0),0,0);

	number_of_files_data = gtk_label_new("");
	gtk_misc_set_alignment (GTK_MISC (number_of_files_data),0,0.5);
	gtk_table_attach (GTK_TABLE (table1),number_of_files_data,1,2,8,9,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0),0,0);

	comment_data = gtk_label_new("");
	gtk_misc_set_alignment (GTK_MISC (comment_data),0,0.5);
	gtk_table_attach (GTK_TABLE (table1),comment_data,1,2,7,8,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0),0,0);

	content_data = gtk_label_new("");
	gtk_misc_set_alignment (GTK_MISC (content_data),0,0.5);
	gtk_table_attach (GTK_TABLE (table1),content_data,1,2,6,7,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0),0,0);

	size_data = gtk_label_new("");
	gtk_misc_set_alignment (GTK_MISC (size_data),0,0.5);
	gtk_table_attach (GTK_TABLE (table1),size_data,1,2,5,6,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0),0,0);

	modified_data = gtk_label_new("");
	gtk_misc_set_alignment (GTK_MISC (modified_data),0,0.5);
	gtk_table_attach (GTK_TABLE (table1),modified_data,1,2,4,5,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0),0,0);

	encrypted_data = gtk_label_new("");
	gtk_misc_set_alignment (GTK_MISC (encrypted_data),0,0.5);
	gtk_table_attach (GTK_TABLE (table1),encrypted_data,1,2,3,4,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0),0,0);

	type_data = gtk_label_new("");
	gtk_misc_set_alignment (GTK_MISC (type_data),0,0.5);
	gtk_table_attach (GTK_TABLE (table1),type_data,1,2,2,3,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0),0,0);

	path_data = gtk_label_new("");
	gtk_misc_set_alignment (GTK_MISC (path_data),0,0.5);
	gtk_table_attach (GTK_TABLE (table1),path_data,1,2,1,2,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0),0,0);

	name_data = gtk_label_new("");
	gtk_misc_set_alignment (GTK_MISC (name_data),0,0.5);
	gtk_table_attach (GTK_TABLE (table1),name_data,1,2,0,1,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0),0,0);
	return archive_properties_window;
}

void set_label (GtkWidget *label,gchar *text)
{
    gchar *tmp_markup = g_strdup_printf ("<b>%s</b>",text);
    gtk_label_set_markup ( GTK_LABEL (label),tmp_markup);
    g_free (tmp_markup);
}

void xa_set_button_state (gboolean New,gboolean Open,gboolean save,gboolean Close,gboolean add,gboolean extract,gboolean sfx,gboolean test,gboolean info,gboolean has_password,gboolean can_list)
{
	gtk_widget_set_sensitive (New_button,New);
    gtk_widget_set_sensitive (new1,New);
	gtk_widget_set_sensitive (Open_button,Open);
    gtk_widget_set_sensitive (open1,Open);
    gtk_widget_set_sensitive (save1,save);
    gtk_widget_set_sensitive (close1,Close);
	gtk_widget_set_sensitive (AddFile_button,add);
	gtk_widget_set_sensitive (addfile,add);
	gtk_widget_set_sensitive (Extract_button,extract);
	gtk_widget_set_sensitive (extract_menu,extract);
	gtk_widget_set_sensitive (exe_menu,sfx);
	gtk_widget_set_sensitive (check_menu,test);
	gtk_widget_set_sensitive (properties,info);
	gtk_widget_set_sensitive (select_all,Close);
	gtk_widget_set_sensitive (select_pattern,Close);
	gtk_widget_set_sensitive (password_entry_menu,has_password);
	gtk_widget_set_sensitive (listing,can_list);
}

void xa_handle_navigation_buttons (GtkMenuItem *menuitem,gpointer user_data)
{
	unsigned short int bp = GPOINTER_TO_UINT(user_data);
	gint current_page;
	gint idx;
	XEntry *new_entry = NULL;
	GtkTreeIter iter;
	GtkTreeSelection *selection;
	GtkTreeModel *model;

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
			/* Let's unselect the row in the sidepane */
			selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (archive_dir_treeview));
			if (selection != NULL)
			{
				gtk_tree_selection_get_selected (selection,&model,&iter);
				gtk_tree_selection_unselect_iter(selection,&iter);
			}
			xa_update_window_with_archive_entries(archive[idx],NULL);
			xa_restore_navigation(idx);
		break;
		/* Back */
		case 1:
			if (archive[idx]->back)
			{
				if (g_slist_find(archive[idx]->forward,archive[idx]->current_entry) == NULL)
					archive[idx]->forward = g_slist_prepend(archive[idx]->forward,archive[idx]->current_entry);

				xa_update_window_with_archive_entries(archive[idx],archive[idx]->back->data);
				xa_sidepane_select_row(archive[idx]->back->data);
			}

			archive[idx]->back = archive[idx]->back->next;
			xa_restore_navigation(idx);
		break;
		/* Up */
		case 2:
			if (archive[idx]->back)
				archive[idx]->forward = g_slist_prepend(archive[idx]->forward,archive[idx]->current_entry);

			/* Let's unselect the row in the sidepane */
			selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (archive_dir_treeview));
			if (selection != NULL)
			{
				gtk_tree_selection_get_selected (selection,&model,&iter);
				gtk_tree_selection_unselect_iter(selection,&iter);
			}
			new_entry = xa_find_entry_from_path(archive[idx]->root_entry,archive[idx]->location_entry_path);
			xa_update_window_with_archive_entries(archive[idx],new_entry->prev);
			xa_sidepane_select_row(new_entry->prev);

			if (archive[idx]->back)
				archive[idx]->back = archive[idx]->back->next;
			xa_restore_navigation(idx);
		break;
		/* Forward */
		case 3:
			if (archive[idx]->forward)
			{
				if (g_slist_find(archive[idx]->back,archive[idx]->current_entry) == NULL)
					archive[idx]->back = g_slist_prepend(archive[idx]->back,archive[idx]->current_entry);

				xa_update_window_with_archive_entries(archive[idx],archive[idx]->forward->data);
				xa_sidepane_select_row(archive[idx]->forward->data);
				archive[idx]->forward = archive[idx]->forward->next;
			}
			xa_restore_navigation(idx);
		break;
	}
}

void xa_restore_navigation(int idx)
{
	gboolean back = FALSE,up = FALSE,forward = FALSE,home=FALSE;

	/*If the pointers exist,we should show the icon*/
	if(archive[idx]->forward !=NULL)
		forward = TRUE;

	if(archive[idx]->back !=NULL)
		back = TRUE;

	if(archive[idx]->location_entry_path!=NULL)
	{
		/* If there's a slash on the path,we should allow UP and HOME operations */
		if(strstr(archive[idx]->location_entry_path,"/")!=NULL)
			home = up = TRUE;
	}
	gtk_widget_set_sensitive(back_button,back);
	gtk_widget_set_sensitive(forward_button,forward);
	gtk_widget_set_sensitive(up_button,up);
	gtk_widget_set_sensitive(home_button,home);
}

void xa_disable_delete_buttons (gboolean value)
{
    gtk_widget_set_sensitive (delete_menu,value);
    gtk_widget_set_sensitive (rename_menu,value);
}

void xa_sidepane_row_expanded(GtkTreeView *tree_view,GtkTreeIter *iter,GtkTreePath *path,gpointer data)
{
	GtkTreeModel *model = data;

	if (gtk_tree_view_row_expanded(tree_view,path))
		gtk_tree_store_set(GTK_TREE_STORE(model),iter,0,"gtk-open",-1);
	else
		gtk_tree_store_set(GTK_TREE_STORE(model),iter,0,"gtk-directory",-1);
}

void xa_sidepane_drag_data_received (GtkWidget *widget,GdkDragContext *context,int x,int y,GtkSelectionData *data,unsigned int info,unsigned int time,gpointer user_data)
{
	gchar **array = NULL;
	gchar *filename = NULL;
	gchar *name = NULL;
	unsigned int len = 0;
	gint current_page;
	gint idx;
	GSList *list = NULL;
	GtkTreeModel *model;
	GtkTreePath *path;
	GtkTreeIter iter;
	GtkTreeIter parent;
	GString *full_pathname = g_string_new("");
	gboolean full_path,add_recurse,dummy_password;
	int response;

	model = gtk_tree_view_get_model(GTK_TREE_VIEW(widget));
	current_page = gtk_notebook_get_current_page(notebook);
	idx = xa_find_archive_index(current_page);
	if (idx < 0)
	{
		gtk_drag_finish(context,FALSE,FALSE,time);
		return;
	}
	if (archive[idx]->type == XARCHIVETYPE_DEB || archive[idx]->type == XARCHIVETYPE_RPM)
	{
		gchar *msg;
		if (archive[idx]->type == XARCHIVETYPE_DEB)
			msg = _("You can't add content to deb packages!");
		else
			msg = _("You can't add content to rpm packages!");
		response = xa_show_message_dialog (GTK_WINDOW (xa_main_window),GTK_DIALOG_MODAL,GTK_MESSAGE_ERROR,GTK_BUTTONS_OK,_("Can't perform this action:"),msg);
		gtk_drag_finish(context,FALSE,FALSE,time);
		return;
	}
	array = gtk_selection_data_get_uris(data);
	if (array == NULL || archive[idx]->child_pid)
	{
		response = xa_show_message_dialog (GTK_WINDOW (xa_main_window),GTK_DIALOG_MODAL,GTK_MESSAGE_ERROR,GTK_BUTTONS_OK,_("Sorry,I could not perform the operation!"),"");
		gtk_drag_finish(context,FALSE,FALSE,time);
		return;
	}
	while (array[len])
	{
		filename = g_filename_from_uri (array[len],NULL,NULL);
		list = g_slist_append(list,filename);
		len++;
	}
	const char *home_dir = g_get_home_dir();
	chdir (home_dir);

	/* Let's get the full pathname so to add dropped files there */
	path = g_object_get_data(G_OBJECT(context),"current_path");
	if (path == NULL)
	{
		gtk_drag_finish (context,TRUE,FALSE,time);
		return;
	}
	gtk_tree_model_get_iter(model,&iter,path);
	gtk_tree_model_get(model,&iter,1,&name,-1);
	g_string_prepend_c(full_pathname,'/');
	g_string_prepend(full_pathname,name);
	gtk_tree_path_free(path);

	while (gtk_tree_model_iter_parent(model,&parent,&iter))
	{
		gtk_tree_model_get(model,&parent,1,&name,-1);
		g_string_prepend_c(full_pathname,'/');
		g_string_prepend(full_pathname,name);
		iter = parent;
	}
	if (archive[idx]->location_entry_path != NULL)
		g_free(archive[idx]->location_entry_path);

	/* This is to have the dragged files stored inside current archive location entry */
	archive[idx]->location_entry_path = g_strdup(full_pathname->str);
	dummy_password = archive[idx]->has_passwd;
	full_path = archive[idx]->full_path;
	add_recurse = archive[idx]->add_recurse;

	archive[idx]->has_passwd = 0;
	archive[idx]->full_path = 0;
	archive[idx]->add_recurse = 1;
	xa_execute_add_commands(archive[idx],list,NULL);

	archive[idx]->has_passwd = dummy_password;
	archive[idx]->full_path = full_path;
	archive[idx]->add_recurse = add_recurse;
	
	g_string_free(full_pathname,TRUE);
	if (list != NULL)
	{
		g_slist_foreach(list,(GFunc) g_free,NULL);
		g_slist_free(list);
	}
	g_strfreev (array);
	gtk_drag_finish (context,TRUE,FALSE,time);
}

gboolean xa_sidepane_drag_motion_expand_timeout (gpointer data)
{
	GtkTreePath *path;

	gtk_tree_view_get_drag_dest_row (GTK_TREE_VIEW(archive_dir_treeview), &path, NULL);
	if (G_LIKELY (path != NULL))
	{
		gtk_tree_view_expand_row (GTK_TREE_VIEW(archive_dir_treeview), path, FALSE);
		gtk_tree_path_free (path);
		return FALSE;
	}
	else
		return TRUE;
}

gboolean xa_sidepane_drag_motion (GtkWidget *widget,GdkDragContext *context,gint x,gint y,guint time,gpointer user_data)
{
	GtkTreeModel *model;
	GtkTreePath *path;
	
	model = gtk_tree_view_get_model(GTK_TREE_VIEW(widget));
	gtk_tree_view_get_dest_row_at_pos (GTK_TREE_VIEW (widget),x,y,&path,NULL);
	if (path)
	{
		g_timeout_add_full (G_PRIORITY_LOW, 1000,(GSourceFunc) xa_sidepane_drag_motion_expand_timeout,NULL,NULL);
		g_object_set_data(G_OBJECT(context),"current_path",path);
	}
	/* This to set the focus on the dropped row */
	gtk_tree_view_set_drag_dest_row(GTK_TREE_VIEW(widget),path,GTK_TREE_VIEW_DROP_INTO_OR_BEFORE);
	gdk_drag_status (context,context->suggested_action,time);
	return TRUE;
}

Progress_bar_data *xa_create_progress_bar(gboolean flag,XArchive *archive)
{
	GtkWidget *vbox1,*vbox2,*message = NULL,*hbox1,*icon_pixbuf,*total_label,*action_area;
	GdkPixbuf *pixbuf;
	PangoAttrList *italic_attr;
	static Progress_bar_data *pb = NULL;
	gchar *text = NULL,*markup;

	if (pb)
		return pb;
		
	pb = g_new0(Progress_bar_data,1);
	pb->progress_window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title (GTK_WINDOW (pb->progress_window),_("Xarchiver"));
	gtk_window_set_position (GTK_WINDOW (pb->progress_window),GTK_WIN_POS_CENTER_ALWAYS);
	gtk_widget_set_size_request(pb->progress_window,400,-1);
	gtk_window_set_resizable(GTK_WINDOW (pb->progress_window),FALSE);
	gtk_container_set_border_width (GTK_CONTAINER (pb->progress_window),6);
	gtk_window_set_transient_for (GTK_WINDOW (pb->progress_window),GTK_WINDOW (xa_main_window));	

	vbox1 = gtk_vbox_new (FALSE,12);
  	gtk_container_add (GTK_CONTAINER (pb->progress_window),vbox1);
  	gtk_container_set_border_width (GTK_CONTAINER (vbox1),6);

	hbox1 = gtk_hbox_new (FALSE,12);
	gtk_box_pack_start (GTK_BOX (vbox1),hbox1,TRUE,TRUE,0);

	pixbuf = gtk_icon_theme_load_icon(icon_theme,"gnome-mime-application-zip",40,0,NULL);
	icon_pixbuf = gtk_image_new_from_pixbuf(pixbuf);
	g_object_unref(pixbuf);

	gtk_box_pack_start (GTK_BOX (hbox1),icon_pixbuf,FALSE,FALSE,0);
	gtk_misc_set_alignment (GTK_MISC (icon_pixbuf),0.0,0.0);

	vbox2 = gtk_vbox_new (FALSE,0);
	gtk_box_pack_start (GTK_BOX (hbox1),vbox2,TRUE,TRUE,0);

	if (archive)
	{
		if (archive->status == XA_ARCHIVESTATUS_EXTRACT)
			text = _("Extracting from archive:");
		else
			text = _("Adding to archive:");

		message = gtk_label_new("");
		markup = g_markup_printf_escaped("<b>%s</b>", text);
		gtk_label_set_markup(GTK_LABEL(message),markup);
		g_free (markup);
		gtk_box_pack_start (GTK_BOX (vbox2),message,FALSE,FALSE,0);
		gtk_misc_set_alignment (GTK_MISC (message),0,0.5);
	}

	pb->archive_label = gtk_label_new("");
	gtk_label_set_ellipsize(GTK_LABEL(pb->archive_label),PANGO_ELLIPSIZE_END);
	gtk_misc_set_alignment (GTK_MISC (pb->archive_label),0,0.5);
	gtk_box_pack_start (GTK_BOX (vbox2),pb->archive_label,FALSE,FALSE,12);
	if (archive)
		gtk_label_set_text(GTK_LABEL(pb->archive_label),archive->path);

	if (flag == FALSE)
	{
		pb->multi_extract = TRUE;
		total_label = gtk_label_new (_("Total Progress:"));
		gtk_box_pack_start (GTK_BOX (vbox2),total_label,FALSE,FALSE,0);
		gtk_misc_set_alignment (GTK_MISC (total_label),0,0);
	}
	pb->progressbar1 = gtk_progress_bar_new ();
	gtk_box_pack_start (GTK_BOX (vbox2),pb->progressbar1,FALSE,FALSE,0);
	pb->file_label = gtk_label_new("");
	gtk_misc_set_alignment (GTK_MISC (pb->file_label),0,0.5);
	gtk_box_pack_start (GTK_BOX (vbox2),pb->file_label,FALSE,FALSE,12);
	if (flag)
	{
		gtk_progress_bar_set_pulse_step(GTK_PROGRESS_BAR(pb->progressbar1),0.033);
		
		italic_attr = pango_attr_list_new ();
		pango_attr_list_insert (italic_attr, pango_attr_style_new (PANGO_STYLE_ITALIC));
		gtk_label_set_attributes (GTK_LABEL (pb->file_label),italic_attr);
		pango_attr_list_unref (italic_attr);
		gtk_label_set_ellipsize(GTK_LABEL(pb->file_label),PANGO_ELLIPSIZE_END);
		
		action_area = gtk_hbutton_box_new ();
		gtk_button_box_set_layout (GTK_BUTTON_BOX (action_area),GTK_BUTTONBOX_END);  
  		gtk_box_pack_end (GTK_BOX (vbox2),action_area,FALSE, TRUE, 0);

		cancel_button = gtk_button_new_from_stock ("gtk-cancel");
		gtk_box_pack_end (GTK_BOX (action_area),cancel_button,TRUE,TRUE,12);

		g_signal_connect (G_OBJECT (cancel_button),		 "clicked",		G_CALLBACK (xa_progress_dialog_stop_action), GINT_TO_POINTER (archive->child_pid));
		g_signal_connect (G_OBJECT (pb->progress_window),"delete_event",G_CALLBACK (xa_progress_dialog_delete_event),GINT_TO_POINTER (archive->child_pid));
	}
	gtk_widget_show_all(pb->progress_window);
	return pb;
}

void xa_increase_progress_bar(Progress_bar_data *pb,gchar *filename,double percent)
{
	gchar *message = NULL, *basename, *markup;

	if (pb->multi_extract)
	{
		gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (pb->progressbar1),percent);
		message = g_strdup_printf("%.0f%%",(percent*100));
		gtk_progress_bar_set_text (GTK_PROGRESS_BAR(pb->progressbar1),message);
		g_free(message);
		
		basename = g_path_get_basename(filename);
		markup = g_markup_printf_escaped("<b>%s</b>", basename);
		g_free(basename);
		gtk_label_set_markup(GTK_LABEL(pb->archive_label),markup);
		g_free (markup);
	}
	else
		gtk_label_set_text(GTK_LABEL(pb->file_label),filename);

	while (gtk_events_pending())
		gtk_main_iteration();
}

/* TODO:
void xa_icon_theme_changed (GtkIconTheme *icon_theme,gpointer data)
{
 	 * Here we should reload all the icons currently displayed according to the
 	 * new icon_theme. xa_get_pixbuf_icon_from_cache() is to be called as many
 	 * time as the filenames currently displayed. What of the other tabs then?
}*/

static gboolean xa_progress_dialog_delete_event (GtkWidget *caller,GdkEvent *event,GPid pid)
{
	kill (pid,SIGINT);
	return TRUE;
}

static void xa_progress_dialog_stop_action (GtkWidget *widget,GPid pid)
{
	kill (pid,SIGINT);
}

gboolean xa_pulse_progress_bar_window (Progress_bar_data *pb)
{
	if (GTK_WIDGET_VISIBLE(pb->progress_window))
	{
		gtk_progress_bar_pulse(GTK_PROGRESS_BAR(pb->progressbar1));
		return TRUE;
	}
	else
		return FALSE;
}
