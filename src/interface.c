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

#include "callbacks.h"
#include "interface.h"
#include "support.h"

static const GtkTargetEntry drag_targets[] =
{
  { "XdndDirectSave0", 0, 0 },
};

static const GtkTargetEntry drop_targets[] =
{
  { "text/uri-list", 0, 0 },
};

GtkWidget *create_MainWindow (void)
{
	tooltips = gtk_tooltips_new ();
	accel_group = gtk_accel_group_new ();

	MainWindow = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title (GTK_WINDOW (MainWindow), "Xarchiver " VERSION);
  /* By using GDK_ACTION_MOVE GDK_ACTION_MOVE GDK_ACTION_LINK GDK_ACTION_ASK we should have KDE DnD compatibility. */
	gtk_drag_dest_set (MainWindow,GTK_DEST_DEFAULT_ALL, drop_targets, 1, GDK_ACTION_COPY | GDK_ACTION_MOVE | GDK_ACTION_LINK | GDK_ACTION_ASK);
	gtk_window_set_default_icon_from_file (DATADIR "/pixmaps/xarchiver.png", NULL  );
	g_signal_connect (G_OBJECT (MainWindow), "drag-data-received",	G_CALLBACK (on_drag_data_received), NULL);
	g_signal_connect (G_OBJECT (MainWindow), "delete-event", G_CALLBACK (xa_quit_application), NULL);

  vbox1 = gtk_vbox_new (FALSE, 0);
  gtk_widget_show (vbox1);
  gtk_container_add (GTK_CONTAINER (MainWindow), vbox1);

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

  quit1 = gtk_image_menu_item_new_from_stock ("gtk-quit", accel_group);
  gtk_widget_show (quit1);
  gtk_container_add (GTK_CONTAINER (menuitem1_menu), quit1);

  menuitem2 = gtk_menu_item_new_with_mnemonic (_("A_ction"));
  gtk_widget_show (menuitem2);
  gtk_container_add (GTK_CONTAINER (menubar1), menuitem2);

  menuitem2_menu = gtk_menu_new ();
  gtk_menu_item_set_submenu (GTK_MENU_ITEM (menuitem2), menuitem2_menu);

  addfile = gtk_image_menu_item_new_with_mnemonic (_("Add"));
  gtk_widget_set_sensitive (addfile,FALSE);
  gtk_widget_show (addfile);
  gtk_container_add (GTK_CONTAINER (menuitem2_menu), addfile);
  gtk_widget_add_accelerator (addfile, "activate",accel_group,GDK_f, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);

  image2 = xa_main_window_find_image ("add_button.png", GTK_ICON_SIZE_MENU);
  gtk_widget_show (image2);
  gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (addfile), image2);
 
  extract_menu = gtk_image_menu_item_new_with_mnemonic (_("_Extract"));
  gtk_widget_set_sensitive (extract_menu,FALSE);
  gtk_widget_show (extract_menu);
  gtk_container_add (GTK_CONTAINER (menuitem2_menu), extract_menu);
  gtk_widget_add_accelerator (extract_menu, "activate",accel_group,GDK_e, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);

  image2 =  xa_main_window_find_image ("extract_button.png", GTK_ICON_SIZE_MENU);
  gtk_widget_show (image2);
  gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (extract_menu), image2);

  separatormenuitem2 = gtk_separator_menu_item_new ();
  gtk_widget_show (separatormenuitem2);
  gtk_container_add (GTK_CONTAINER (menuitem2_menu), separatormenuitem2);
  gtk_widget_set_sensitive (separatormenuitem2, FALSE);

  delete_menu = gtk_image_menu_item_new_from_stock ("gtk-delete", accel_group);
  gtk_widget_set_sensitive (delete_menu,FALSE);
  gtk_widget_show (delete_menu);
  gtk_container_add (GTK_CONTAINER (menuitem2_menu), delete_menu);
  gtk_widget_add_accelerator (delete_menu, "activate",accel_group,GDK_d, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);

  view_menu = gtk_image_menu_item_new_with_mnemonic (_("_View"));
  gtk_widget_show (view_menu);
  gtk_container_add (GTK_CONTAINER (menuitem2_menu), view_menu);
  gtk_widget_set_sensitive (view_menu, FALSE);
  gtk_widget_add_accelerator (view_menu, "activate",accel_group,GDK_v, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);

  image2 = gtk_image_new_from_stock ("gtk-find", GTK_ICON_SIZE_MENU);
  gtk_widget_show (image2);
  gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (view_menu), image2);

  separatormenuitem3 = gtk_separator_menu_item_new ();
  gtk_widget_show (separatormenuitem3);
  gtk_container_add (GTK_CONTAINER (menuitem2_menu), separatormenuitem3);
  gtk_widget_set_sensitive (separatormenuitem3, FALSE);

  select_all = gtk_image_menu_item_new_with_mnemonic (_("Select _All"));
  gtk_widget_show (select_all);
  gtk_container_add (GTK_CONTAINER (menuitem2_menu), select_all);
  gtk_widget_set_sensitive (select_all, FALSE);
  gtk_widget_add_accelerator (select_all, "activate",accel_group,GDK_a, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);

  deselect_all = gtk_image_menu_item_new_with_mnemonic (_("Dese_lect All"));
  gtk_widget_show (deselect_all);
  gtk_container_add (GTK_CONTAINER (menuitem2_menu), deselect_all);
  gtk_widget_set_sensitive (deselect_all, FALSE);
  gtk_widget_add_accelerator (deselect_all, "activate",accel_group,GDK_l, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);

  separatormenuitem4 = gtk_separator_menu_item_new ();
  gtk_widget_show (separatormenuitem4);
  gtk_container_add (GTK_CONTAINER (menuitem2_menu), separatormenuitem4);
  gtk_widget_set_sensitive (separatormenuitem4, FALSE);
  
  view_shell_output1 = gtk_image_menu_item_new_with_mnemonic (_("View Error _Messages"));
  gtk_widget_show (view_shell_output1);
  gtk_container_add (GTK_CONTAINER (menuitem2_menu), view_shell_output1);
  gtk_widget_add_accelerator (view_shell_output1, "activate",accel_group,GDK_m, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);

  image2 = gtk_image_new_from_stock ("gtk-find-and-replace", GTK_ICON_SIZE_MENU);
  gtk_widget_show (image2);
  gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (view_shell_output1), image2);
  
  iso_info = gtk_image_menu_item_new_with_mnemonic (_("Show I_SO info"));
  gtk_widget_show (iso_info);
  gtk_widget_set_sensitive ( iso_info , FALSE );
  gtk_container_add (GTK_CONTAINER (menuitem2_menu), iso_info);
  gtk_widget_add_accelerator (iso_info, "activate",accel_group,GDK_s, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);

  tmp_image = gtk_image_new_from_stock ("gtk-cdrom", GTK_ICON_SIZE_MENU);
  gtk_widget_show (tmp_image);
  gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (iso_info), tmp_image);

  menuitem4 = gtk_menu_item_new_with_mnemonic (_("_Help"));
  gtk_widget_show (menuitem4);
  gtk_container_add (GTK_CONTAINER (menubar1), menuitem4);

  menuitem4_menu = gtk_menu_new ();
  gtk_menu_item_set_submenu (GTK_MENU_ITEM (menuitem4), menuitem4_menu);

  about1 = gtk_image_menu_item_new_from_stock ("gtk-about", accel_group);
  gtk_widget_show (about1);
  gtk_container_add (GTK_CONTAINER (menuitem4_menu), about1);

  /* Create the toolbar */
  toolbar1 = gtk_toolbar_new ();
  gtk_widget_show (toolbar1);
  gtk_box_pack_start (GTK_BOX (vbox1), toolbar1, FALSE, FALSE, 0);
  gtk_toolbar_set_style (GTK_TOOLBAR (toolbar1), GTK_TOOLBAR_BOTH);
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

  tmp_image = xa_main_window_find_image("add.png", GTK_ICON_SIZE_LARGE_TOOLBAR);
  gtk_widget_show (tmp_image);
  AddFile_button = (GtkWidget*) gtk_tool_button_new (tmp_image, _("Add"));
  gtk_widget_set_sensitive (AddFile_button,FALSE);
  gtk_widget_show (AddFile_button);
  gtk_tool_item_set_homogeneous (GTK_TOOL_ITEM (AddFile_button), FALSE);
  gtk_container_add (GTK_CONTAINER (toolbar1), AddFile_button);
  gtk_tool_item_set_tooltip (GTK_TOOL_ITEM (AddFile_button), tooltips, _("Add files and directories to the current archive"), NULL);

  tmp_image = xa_main_window_find_image("extract.png", GTK_ICON_SIZE_LARGE_TOOLBAR);
  gtk_widget_show (tmp_image);
  Extract_button = (GtkWidget*) gtk_tool_button_new (tmp_image, _("Extract"));
  gtk_widget_set_sensitive (Extract_button,FALSE);
  gtk_widget_show (Extract_button);
  gtk_tool_item_set_homogeneous (GTK_TOOL_ITEM (Extract_button), FALSE);
  gtk_container_add (GTK_CONTAINER (toolbar1), Extract_button);
  gtk_tool_item_set_tooltip (GTK_TOOL_ITEM (Extract_button), tooltips, _("Extract files from the current archive; use the mouse to select files individually"), NULL);

  separatortoolitem2 = (GtkWidget*) gtk_separator_tool_item_new ();
  gtk_widget_show (separatortoolitem2);
  gtk_container_add (GTK_CONTAINER (toolbar1), separatortoolitem2);

  tmp_image = gtk_image_new_from_stock ("gtk-delete", tmp_toolbar_icon_size);
  gtk_widget_show (tmp_image);
  Delete_button = (GtkWidget*) gtk_tool_button_new (tmp_image, _("Delete"));
  gtk_widget_show (Delete_button);
  gtk_widget_set_sensitive (Delete_button,FALSE);
  gtk_tool_item_set_homogeneous (GTK_TOOL_ITEM (Delete_button), FALSE);
  gtk_container_add (GTK_CONTAINER (toolbar1), Delete_button);
  gtk_tool_item_set_tooltip (GTK_TOOL_ITEM (Delete_button), tooltips, _("Delete files from the current archive"), NULL);

  tmp_image = gtk_image_new_from_stock ("gtk-find", tmp_toolbar_icon_size);
  gtk_widget_show (tmp_image);
  View_button = (GtkWidget*) gtk_tool_button_new (tmp_image, _("View"));
  gtk_widget_show (View_button);
  gtk_widget_set_sensitive (View_button,FALSE);
  gtk_tool_item_set_homogeneous (GTK_TOOL_ITEM (View_button), FALSE);
  gtk_container_add (GTK_CONTAINER (toolbar1), View_button);
  gtk_tool_item_set_tooltip (GTK_TOOL_ITEM (View_button), tooltips, _("View file content in the current archive"), NULL);

  separatortoolitem3 = (GtkWidget*) gtk_separator_tool_item_new ();
  gtk_widget_show (separatortoolitem3);
  gtk_container_add (GTK_CONTAINER (toolbar1), separatortoolitem3);

  tmp_image = gtk_image_new_from_stock ("gtk-stop", tmp_toolbar_icon_size);
  gtk_widget_show (tmp_image);
  Stop_button = (GtkWidget*) gtk_tool_button_new (tmp_image, _("Stop"));
  gtk_widget_set_sensitive (Stop_button,FALSE);
  gtk_widget_show (Stop_button);
  gtk_tool_item_set_homogeneous (GTK_TOOL_ITEM ( Stop_button ), FALSE);
  gtk_container_add (GTK_CONTAINER (toolbar1), Stop_button);
  gtk_tool_item_set_tooltip (GTK_TOOL_ITEM (Stop_button), tooltips, _("Cancel current operation"), NULL);

  scrolledwindow1 = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy ( GTK_SCROLLED_WINDOW (scrolledwindow1) , GTK_POLICY_AUTOMATIC,GTK_POLICY_AUTOMATIC);
  gtk_widget_show (scrolledwindow1);
  
  treeview1 = gtk_tree_view_new ();
  gtk_widget_show (treeview1);
  gtk_container_add (GTK_CONTAINER (scrolledwindow1), treeview1);
  gtk_drag_source_set (treeview1, GDK_BUTTON1_MASK, drag_targets, 1, GDK_ACTION_COPY | GDK_ACTION_MOVE | GDK_ACTION_LINK | GDK_ACTION_ASK);
  
	g_signal_connect (G_OBJECT (treeview1), "drag-begin",			G_CALLBACK (drag_begin), NULL);
	g_signal_connect (G_OBJECT (treeview1), "drag-data-get",		G_CALLBACK (drag_data_get), NULL );
	g_signal_connect (G_OBJECT (treeview1), "drag-end",				G_CALLBACK (drag_end), NULL);

  vbox_body = gtk_vbox_new (FALSE, 2);
  gtk_widget_show (vbox_body);
  gtk_container_set_border_width (GTK_CONTAINER(vbox_body), 2);
  gtk_box_pack_start(GTK_BOX(vbox1), vbox_body, TRUE, TRUE, 0);
 
  gtk_box_pack_start (GTK_BOX (vbox_body), scrolledwindow1, TRUE, TRUE, 0);
  gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolledwindow1), GTK_SHADOW_IN);

  hbox_sb = gtk_hbox_new (FALSE, 2);
  gtk_widget_show (hbox_sb);
  gtk_box_pack_start (GTK_BOX (vbox_body), hbox_sb, FALSE, TRUE, 0);

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

  viewport3 = gtk_viewport_new (NULL, NULL);
  gtk_widget_show (viewport3);
  gtk_box_pack_start (GTK_BOX (hbox_sb), viewport3, FALSE, TRUE, 0);

  ebox = gtk_event_box_new();
  pad_image = gtk_image_new_from_stock(GTK_STOCK_DIALOG_AUTHENTICATION, GTK_ICON_SIZE_MENU);
  gtk_container_add (GTK_CONTAINER(ebox), pad_image);
  gtk_widget_show (ebox);
  gtk_container_add (GTK_CONTAINER (viewport3), ebox);
  gtk_widget_set_size_request(ebox, 15, -1);
  pad_tooltip = gtk_tooltips_new ();
  gtk_tooltips_set_tip (pad_tooltip , ebox , _("This archive contains password protected files"), NULL );
  gtk_tooltips_disable ( pad_tooltip );

	g_signal_connect ((gpointer) new1, "activate", G_CALLBACK (xa_new_archive), NULL);
	g_signal_connect ((gpointer) open1, "activate", G_CALLBACK (xa_open_archive), NULL);
	g_signal_connect ((gpointer) check_menu, "activate", G_CALLBACK (xa_test_archive), NULL);
	g_signal_connect ((gpointer) properties, "activate", G_CALLBACK (xa_archive_properties), NULL);
	g_signal_connect ((gpointer) extract_menu, "activate", G_CALLBACK (xa_extract_archive), NULL);
	g_signal_connect ((gpointer) addfile, "activate", G_CALLBACK (xa_add_files_archive), NULL);
	g_signal_connect ((gpointer) view_shell_output1, "activate", G_CALLBACK (ShowShellOutput), NULL);
	g_signal_connect ((gpointer) select_all, "activate", G_CALLBACK (xa_select_all), NULL);
	g_signal_connect ((gpointer) deselect_all, "activate", G_CALLBACK (xa_deselect_all), NULL);
	g_signal_connect ((gpointer) iso_info, "activate", G_CALLBACK (xa_iso_properties), NULL);
	g_signal_connect ((gpointer) quit1, "activate", G_CALLBACK (xa_quit_application), NULL);
	g_signal_connect ((gpointer) delete_menu, "activate", G_CALLBACK (xa_delete_archive), NULL);
	g_signal_connect ((gpointer) view_menu, "activate", G_CALLBACK (View_File_Window), NULL);
	g_signal_connect ((gpointer) about1, "activate", G_CALLBACK (xa_about), NULL);

	g_signal_connect ((gpointer) New_button, "clicked", G_CALLBACK (xa_new_archive), NULL);
	g_signal_connect ((gpointer) Open_button, "clicked", G_CALLBACK (xa_open_archive), NULL);
	g_signal_connect ((gpointer) AddFile_button, "clicked", G_CALLBACK (xa_add_files_archive), NULL);
    g_signal_connect ((gpointer) Extract_button, "clicked", G_CALLBACK (xa_extract_archive), NULL);
	g_signal_connect ((gpointer) Delete_button, "clicked", G_CALLBACK (xa_delete_archive), NULL);
	g_signal_connect ((gpointer) View_button, "clicked", G_CALLBACK (View_File_Window), NULL);
	g_signal_connect ((gpointer) Stop_button, "clicked", G_CALLBACK (xa_cancel_archive), NULL);
	g_signal_connect (MainWindow, "key-press-event", G_CALLBACK (key_press_function), NULL);

  gtk_window_add_accel_group (GTK_WINDOW (MainWindow), accel_group);
  return MainWindow;
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

	passwd = gtk_dialog_new ();
	gtk_window_set_title (GTK_WINDOW (passwd),_("Enter Archive Password"));
	gtk_window_set_type_hint (GTK_WINDOW (passwd), GDK_WINDOW_TYPE_HINT_DIALOG);
	gtk_window_set_transient_for ( GTK_WINDOW (passwd) , GTK_WINDOW (MainWindow) );
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
				response = ShowGtkMessageDialog (GTK_WINDOW (MainWindow),GTK_DIALOG_MODAL,GTK_MESSAGE_ERROR,GTK_BUTTONS_OK, _("You missed the password!"),_("Please enter it!") );
				break;
			}
			done = TRUE;
			break;
		}
	}
	gtk_widget_destroy (passwd);
	return password;
}


GtkWidget *view_win ()
{
	GtkWidget *view_window;
	GtkWidget *scrolledwindow2;
	GtkWidget *textview1;

	view_window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title (GTK_WINDOW (view_window), _("View File Window"));
	gtk_window_set_destroy_with_parent (GTK_WINDOW (view_window), TRUE);
	gtk_window_set_type_hint (GTK_WINDOW (view_window), GDK_WINDOW_TYPE_HINT_UTILITY);
	gtk_window_set_position (GTK_WINDOW (view_window), GTK_WIN_POS_CENTER);
	gtk_window_set_default_size(GTK_WINDOW (view_window), 450, 300);
	gtk_window_set_modal ( GTK_WINDOW (view_window),TRUE);
	gtk_window_set_transient_for ( GTK_WINDOW (view_window) , GTK_WINDOW (MainWindow) );
	scrolledwindow2 = gtk_scrolled_window_new (NULL, NULL);
	gtk_widget_show (scrolledwindow2);
	gtk_container_add (GTK_CONTAINER (view_window), scrolledwindow2);
	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolledwindow2), GTK_SHADOW_IN);
	gtk_scrolled_window_set_policy ( GTK_SCROLLED_WINDOW (scrolledwindow2) , GTK_POLICY_AUTOMATIC,GTK_POLICY_AUTOMATIC);

	textview1 = gtk_text_view_new ();
	gtk_widget_show (textview1);
	gtk_container_add (GTK_CONTAINER (scrolledwindow2), textview1);
	gtk_container_set_border_width (GTK_CONTAINER (textview1), 5);
	gtk_text_view_set_editable (GTK_TEXT_VIEW (textview1), FALSE);
	gtk_text_view_set_cursor_visible (GTK_TEXT_VIEW (textview1), FALSE);
	viewtextbuf = gtk_text_view_get_buffer ( GTK_TEXT_VIEW (textview1) );
	gtk_text_buffer_get_start_iter (viewtextbuf, &viewenditer);
	return view_window;
}

GtkWidget *create_archive_properties_window ()
{
	archive_properties_window = gtk_dialog_new_with_buttons (_("Archive Properties Window"),
									GTK_WINDOW (MainWindow), GTK_DIALOG_DESTROY_WITH_PARENT,
									GTK_STOCK_CLOSE, GTK_RESPONSE_NONE, NULL);

	g_signal_connect(archive_properties_window, "response", G_CALLBACK(gtk_widget_destroy), NULL);
	g_signal_connect(archive_properties_window, "delete-event", G_CALLBACK(gtk_widget_destroy), NULL);

	gtk_window_set_position (GTK_WINDOW (archive_properties_window), GTK_WIN_POS_CENTER);
	gtk_window_set_resizable (GTK_WINDOW (archive_properties_window), FALSE);
	gtk_window_set_modal (GTK_WINDOW (archive_properties_window), TRUE);
	gtk_window_set_type_hint (GTK_WINDOW (archive_properties_window), GDK_WINDOW_TYPE_HINT_UTILITY);

	table1 = gtk_table_new (9, 2, TRUE);
	gtk_widget_show (table1);
	gtk_container_add (GTK_CONTAINER (GTK_DIALOG (archive_properties_window)->vbox), table1);
	gtk_table_set_row_spacings (GTK_TABLE (table1), 6);
	gtk_table_set_col_spacings (GTK_TABLE (table1), 6);

	name_label = gtk_label_new ("");
	set_label ( name_label , _("Name:"));
	gtk_widget_show (name_label);
	gtk_table_attach (GTK_TABLE (table1), name_label, 0, 1, 0, 1,
                     (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (GTK_EXPAND | GTK_SHRINK | GTK_FILL), 0, 0);
	gtk_misc_set_alignment (GTK_MISC (name_label), 0.99, 0.5);

	path_label = gtk_label_new ("");
	set_label ( path_label , _("Path:"));
	gtk_widget_show (path_label);
	gtk_table_attach (GTK_TABLE (table1), path_label, 0, 1, 1, 2,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (GTK_EXPAND | GTK_SHRINK | GTK_FILL), 0, 0);
	gtk_misc_set_alignment (GTK_MISC (path_label), 0.99, 0.5);

	type_label = gtk_label_new ("");
	set_label ( type_label , _("Type:"));
	gtk_widget_show (type_label);
	gtk_table_attach (GTK_TABLE (table1), type_label, 0, 1, 2, 3,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (GTK_EXPAND | GTK_SHRINK | GTK_FILL), 0, 0);
	gtk_misc_set_alignment (GTK_MISC (type_label), 0.99, 0.5);

	modified_label = gtk_label_new ("");
	set_label ( modified_label , _("Modified on:"));
	gtk_widget_show (modified_label);
	gtk_table_attach (GTK_TABLE (table1), modified_label, 0, 1, 3, 4,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (GTK_EXPAND | GTK_SHRINK | GTK_FILL), 0, 0);
	gtk_misc_set_alignment (GTK_MISC (modified_label), 0.99, 0.5);
  
	size_label = gtk_label_new ("");
	set_label ( size_label , _("Archive size:"));
	gtk_widget_show (size_label);
	gtk_table_attach (GTK_TABLE (table1), size_label, 0, 1, 4, 5,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (GTK_EXPAND | GTK_SHRINK | GTK_FILL), 0, 0);
	gtk_misc_set_alignment (GTK_MISC (size_label), 0.99, 0.5);

	content_label = gtk_label_new ("");
	set_label ( content_label , _("Content size:"));
	gtk_widget_show (content_label);
	gtk_table_attach (GTK_TABLE (table1), content_label, 0, 1, 5, 6,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (GTK_EXPAND | GTK_SHRINK | GTK_FILL), 0, 0);
	gtk_misc_set_alignment (GTK_MISC (content_label), 0.99, 0.5);
  
	compression_label = gtk_label_new ("");
	set_label ( compression_label , _("Compression ratio:"));
	gtk_widget_show (compression_label);
	gtk_table_attach (GTK_TABLE (table1), compression_label, 0, 1, 8, 9,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (GTK_EXPAND | GTK_SHRINK | GTK_FILL), 0, 0);
	gtk_misc_set_alignment (GTK_MISC (compression_label), 0.99, 0.5);
  
	number_of_files_label = gtk_label_new ("");
	set_label ( number_of_files_label , _("Number of files:"));
	gtk_widget_show (number_of_files_label);
	gtk_table_attach (GTK_TABLE (table1), number_of_files_label, 0, 1, 6, 7,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (GTK_EXPAND | GTK_SHRINK | GTK_FILL), 0, 0);
	gtk_misc_set_alignment (GTK_MISC (number_of_files_label), 0.99, 0.5);

	number_of_dirs_label = gtk_label_new ("");
	set_label ( number_of_dirs_label , _("Number of dirs:"));
	gtk_widget_show (number_of_dirs_label);
	gtk_table_attach (GTK_TABLE (table1), number_of_dirs_label, 0, 1, 7, 8,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (GTK_EXPAND | GTK_SHRINK | GTK_FILL), 0, 0);
	gtk_misc_set_alignment (GTK_MISC (number_of_dirs_label), 0.99, 0.5);

	compression_data = gtk_entry_new ();
	gtk_editable_set_editable (GTK_EDITABLE (compression_data), FALSE);
	gtk_entry_set_has_frame (GTK_ENTRY (compression_data), FALSE);
	gtk_widget_show (compression_data);
	gtk_table_attach (GTK_TABLE (table1), compression_data, 1, 2, 8, 9,
                    (GtkAttachOptions) (GTK_FILL | GTK_EXPAND),
                    (GtkAttachOptions) (0), 0, 0);

	number_of_dirs_data = gtk_entry_new ();
	gtk_editable_set_editable (GTK_EDITABLE (number_of_dirs_data), FALSE);
	gtk_entry_set_has_frame (GTK_ENTRY (number_of_dirs_data), FALSE);
	gtk_widget_show (number_of_dirs_data);
	gtk_table_attach (GTK_TABLE (table1), number_of_dirs_data, 1, 2, 7, 8,
                    (GtkAttachOptions) (GTK_FILL | GTK_EXPAND),
                    (GtkAttachOptions) (0), 0, 0);

	number_of_files_data = gtk_entry_new ();
	gtk_editable_set_editable (GTK_EDITABLE (number_of_files_data), FALSE);
	gtk_entry_set_has_frame (GTK_ENTRY (number_of_files_data), FALSE);
	gtk_widget_show (number_of_files_data);
	gtk_table_attach (GTK_TABLE (table1), number_of_files_data, 1, 2, 6, 7,
                    (GtkAttachOptions) (GTK_FILL | GTK_EXPAND),
                    (GtkAttachOptions) (0), 0, 0);

	content_data = gtk_entry_new ();
	gtk_editable_set_editable (GTK_EDITABLE (content_data), FALSE);
	gtk_entry_set_has_frame (GTK_ENTRY (content_data), FALSE);
	gtk_widget_show (content_data);
	gtk_table_attach (GTK_TABLE (table1), content_data, 1, 2, 5, 6,
                    (GtkAttachOptions) (GTK_FILL | GTK_EXPAND),
                    (GtkAttachOptions) (0), 0, 0);

	size_data = gtk_entry_new ();
	gtk_editable_set_editable (GTK_EDITABLE (size_data), FALSE);
	gtk_entry_set_has_frame (GTK_ENTRY (size_data), FALSE);
	gtk_widget_show (size_data);
	gtk_table_attach (GTK_TABLE (table1), size_data, 1, 2, 4, 5,
                    (GtkAttachOptions) (GTK_FILL | GTK_EXPAND),
                    (GtkAttachOptions) (0), 0, 0);

	modified_data = gtk_entry_new ();
	gtk_widget_show (modified_data);
	gtk_table_attach (GTK_TABLE (table1), modified_data, 1, 2, 3, 4,
                    (GtkAttachOptions) (GTK_FILL | GTK_EXPAND),
                    (GtkAttachOptions) (0), 0, 0);
	gtk_editable_set_editable (GTK_EDITABLE (modified_data), FALSE);
	gtk_entry_set_has_frame (GTK_ENTRY (modified_data), FALSE);

	type_data = gtk_entry_new ();
	gtk_editable_set_editable (GTK_EDITABLE (type_data), FALSE);
	gtk_entry_set_has_frame (GTK_ENTRY (type_data), FALSE);
	gtk_widget_show (type_data);
	gtk_table_attach (GTK_TABLE (table1), type_data, 1, 2, 2, 3,
                    (GtkAttachOptions) (GTK_FILL | GTK_EXPAND),
                    (GtkAttachOptions) (0), 0, 0);

	path_data = gtk_entry_new ();
	gtk_editable_set_editable (GTK_EDITABLE (path_data), FALSE);
	gtk_entry_set_has_frame (GTK_ENTRY (path_data), FALSE);
	gtk_widget_show (path_data);
	gtk_table_attach (GTK_TABLE (table1), path_data, 1, 2, 1, 2,
                    (GtkAttachOptions) (GTK_FILL | GTK_EXPAND),
                    (GtkAttachOptions) (0), 0, 0);

	name_data = gtk_entry_new ();
	gtk_editable_set_editable (GTK_EDITABLE (name_data), FALSE);
	gtk_entry_set_has_frame (GTK_ENTRY (name_data), FALSE);
	gtk_widget_show (name_data);
	gtk_table_attach (GTK_TABLE (table1), name_data, 1, 2, 0, 1,
                    (GtkAttachOptions) (GTK_FILL | GTK_EXPAND),
                    (GtkAttachOptions) (0), 0, 0);
	return archive_properties_window;
}

void set_label (GtkWidget *label , gchar *text)
{
    gchar *tmp_markup = g_strdup_printf ("<b>%s</b>",text );
    gtk_label_set_markup ( GTK_LABEL (label) , tmp_markup);
    g_free (tmp_markup);
}

int xa_progressbar_pulse (gpointer data)
{
	gtk_progress_bar_pulse(GTK_PROGRESS_BAR(progressbar) );
	if (GTK_WIDGET_VISIBLE(viewport2))
		return TRUE;
	else
		return FALSE;
}

