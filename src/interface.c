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

extern gboolean unrar;
extern gint id;

GtkWidget *create_MainWindow (void)
{
	tooltips = gtk_tooltips_new ();
	accel_group = gtk_accel_group_new ();

	MainWindow = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	xa_set_window_title (MainWindow , NULL);
  /* By using GDK_ACTION_MOVE GDK_ACTION_MOVE GDK_ACTION_LINK GDK_ACTION_ASK we should have KDE DnD compatibility. */
	gtk_drag_dest_set (MainWindow,GTK_DEST_DEFAULT_ALL, drop_targets, 1, GDK_ACTION_COPY | GDK_ACTION_MOVE | GDK_ACTION_LINK | GDK_ACTION_ASK);
	gtk_window_set_default_icon_from_file (DATADIR "/pixmaps/xarchiver.png", NULL  );
	g_signal_connect (G_OBJECT (MainWindow), "drag-data-received",	G_CALLBACK (on_drag_data_received), NULL);
	g_signal_connect (G_OBJECT (MainWindow), "delete-event", G_CALLBACK (xa_quit_application), NULL);

	/* Create the menus */
	vbox1 = gtk_vbox_new (FALSE, 0);
	gtk_widget_show (vbox1);
	gtk_container_add (GTK_CONTAINER (MainWindow), vbox1);

	menubar1 = gtk_menu_bar_new ();
	gtk_widget_show (menubar1);
	gtk_box_pack_start (GTK_BOX (vbox1), menubar1, FALSE, FALSE, 0);

	menuitem1 = gtk_menu_item_new_with_mnemonic (_("_File"));
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

  quit1 = gtk_image_menu_item_new_from_stock ("gtk-quit", accel_group);
  gtk_widget_show (quit1);
  gtk_container_add (GTK_CONTAINER (menuitem1_menu), quit1);

  menuitem2 = gtk_menu_item_new_with_mnemonic (_("_Action"));
  gtk_widget_show (menuitem2);
  gtk_container_add (GTK_CONTAINER (menubar1), menuitem2);

  menuitem2_menu = gtk_menu_new ();
  gtk_menu_item_set_submenu (GTK_MENU_ITEM (menuitem2), menuitem2_menu);

  addfile = gtk_image_menu_item_new_with_mnemonic (_("_Add"));
  gtk_widget_set_sensitive (addfile,FALSE);
  gtk_widget_show (addfile);
  gtk_container_add (GTK_CONTAINER (menuitem2_menu), addfile);
  gtk_widget_add_accelerator (addfile, "activate",accel_group,GDK_a, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);

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

  exe_menu = gtk_image_menu_item_new_with_mnemonic (_("Make SF_X"));
  gtk_widget_set_sensitive (exe_menu,FALSE);
  gtk_widget_show (exe_menu);
  gtk_container_add (GTK_CONTAINER (menuitem2_menu), exe_menu);
  gtk_widget_add_accelerator (exe_menu, "activate",accel_group,GDK_x, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);

  image2 = gtk_image_new_from_stock ("gtk-execute", GTK_ICON_SIZE_MENU);
  gtk_widget_show (image2);
  gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (exe_menu), image2);

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
  gtk_widget_set_sensitive (view_menu, FALSE);
  gtk_widget_show (view_menu);
  gtk_container_add (GTK_CONTAINER (menuitem2_menu), view_menu);
  gtk_widget_add_accelerator (view_menu, "activate",accel_group,GDK_v, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);

  comment_menu = gtk_image_menu_item_new_with_mnemonic (_("_Show comment"));
  gtk_widget_set_sensitive (comment_menu, FALSE);
  gtk_widget_show (comment_menu);
  gtk_container_add (GTK_CONTAINER (menuitem2_menu), comment_menu);
  gtk_widget_add_accelerator (comment_menu, "activate",accel_group,GDK_s, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);

  image2 = gtk_image_new_from_stock ("gtk-find", GTK_ICON_SIZE_MENU);
  gtk_widget_show (image2);
  gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (view_menu), image2);

  separatormenuitem3 = gtk_separator_menu_item_new ();
  gtk_widget_show (separatormenuitem3);
  gtk_container_add (GTK_CONTAINER (menuitem2_menu), separatormenuitem3);
  gtk_widget_set_sensitive (separatormenuitem3, FALSE);

  select_all = gtk_image_menu_item_new_with_mnemonic (_("Sele_ct All"));
  gtk_widget_show (select_all);
  gtk_container_add (GTK_CONTAINER (menuitem2_menu), select_all);
  gtk_widget_set_sensitive (select_all, FALSE);
  gtk_widget_add_accelerator (select_all, "activate",accel_group,GDK_c, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);

  deselect_all = gtk_image_menu_item_new_with_mnemonic (_("Dese_lect All"));
  gtk_widget_show (deselect_all);
  gtk_container_add (GTK_CONTAINER (menuitem2_menu), deselect_all);
  gtk_widget_set_sensitive (deselect_all, FALSE);
  gtk_widget_add_accelerator (deselect_all, "activate",accel_group,GDK_l, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);

  separatormenuitem4 = gtk_separator_menu_item_new ();
  gtk_widget_show (separatormenuitem4);
  gtk_container_add (GTK_CONTAINER (menuitem2_menu), separatormenuitem4);
  gtk_widget_set_sensitive (separatormenuitem4, FALSE);

  view_shell_output1 = gtk_image_menu_item_new_with_mnemonic (_("Co_mmand line output"));
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
  gtk_widget_add_accelerator (iso_info, "activate",accel_group,GDK_f, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);

  tmp_image = gtk_image_new_from_stock ("gtk-cdrom", GTK_ICON_SIZE_MENU);
  gtk_widget_show (tmp_image);
  gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (iso_info), tmp_image);

  password_entry = gtk_image_menu_item_new_with_mnemonic (_("Reset passwo_rd"));
  gtk_widget_show (password_entry);
  gtk_widget_set_sensitive ( password_entry , FALSE );
  gtk_container_add (GTK_CONTAINER (menuitem2_menu), password_entry);
  gtk_widget_add_accelerator (password_entry, "activate",accel_group,GDK_r, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);

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

	tmp_image = gtk_image_new_from_stock ("gtk-execute", tmp_toolbar_icon_size);
	gtk_widget_show (tmp_image);
	Exe_button = (GtkWidget*) gtk_tool_button_new (tmp_image, _("SFX"));
	gtk_widget_set_sensitive (Exe_button,FALSE);
	gtk_widget_show (Exe_button);
	gtk_tool_item_set_homogeneous (GTK_TOOL_ITEM ( Exe_button ), FALSE);
	gtk_container_add (GTK_CONTAINER (toolbar1), Exe_button);
	gtk_tool_item_set_tooltip (GTK_TOOL_ITEM (Exe_button), tooltips, _("Make the current archive self-extracting"), NULL);
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

	vbox_body = gtk_vbox_new (FALSE, 2);
	gtk_widget_show (vbox_body);
	gtk_container_set_border_width (GTK_CONTAINER(vbox_body), 2);
	gtk_box_pack_start(GTK_BOX(vbox1), vbox_body, TRUE, TRUE, 0);

	/* Create the notebook widget */
	notebook = GTK_NOTEBOOK(gtk_notebook_new() );
	gtk_box_pack_start (GTK_BOX(vbox_body), GTK_WIDGET(notebook),TRUE,TRUE,0);
	gtk_notebook_set_tab_pos (notebook, GTK_POS_TOP);
	gtk_notebook_set_scrollable (notebook, TRUE);
	gtk_notebook_popup_enable (notebook);
	gtk_widget_show (GTK_WIDGET(notebook));
	g_signal_connect ((gpointer) notebook, "switch-page", G_CALLBACK (xa_page_has_changed), NULL);

  	hbox_sb = gtk_hbox_new (FALSE, 2);
	gtk_widget_show (hbox_sb);
	gtk_box_pack_end (GTK_BOX (vbox_body), hbox_sb, FALSE, TRUE, 0);

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
	gtk_box_pack_start (GTK_BOX (hbox_sb), viewport3, FALSE, TRUE, 0);

	ebox = gtk_event_box_new();
	pad_image = gtk_image_new_from_stock(GTK_STOCK_DIALOG_AUTHENTICATION, GTK_ICON_SIZE_MENU);
	gtk_widget_show (pad_image);
	gtk_container_add (GTK_CONTAINER(ebox), pad_image);
	gtk_widget_show (ebox);
	gtk_container_add (GTK_CONTAINER (viewport3), ebox);
	gtk_widget_set_size_request(ebox, 15, -1);
	pad_tooltip = gtk_tooltips_new ();
	gtk_tooltips_set_tip (pad_tooltip , ebox , _("This archive contains password protected files"), NULL );
	gtk_tooltips_enable ( pad_tooltip );

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
	g_signal_connect ((gpointer) iso_info, "activate", G_CALLBACK (xa_iso_properties), NULL);
	g_signal_connect ((gpointer) password_entry, "activate", G_CALLBACK (xa_reset_password), NULL);
	g_signal_connect ((gpointer) close1, "activate", G_CALLBACK (xa_close_archive), NULL);
	g_signal_connect ((gpointer) quit1, "activate", G_CALLBACK (xa_quit_application), NULL);
	g_signal_connect ((gpointer) delete_menu, "activate", G_CALLBACK (xa_delete_archive), NULL);
	g_signal_connect ((gpointer) view_menu, "activate", G_CALLBACK (xa_view_file_inside_archive), NULL);
	g_signal_connect ((gpointer) comment_menu, "activate", G_CALLBACK (xa_show_archive_comment), NULL);
	g_signal_connect ((gpointer) help1, "activate", G_CALLBACK (xa_show_help), NULL);
	g_signal_connect ((gpointer) about1, "activate", G_CALLBACK (xa_about), NULL);

	g_signal_connect ((gpointer) New_button, "clicked", G_CALLBACK (xa_new_archive), NULL);
	g_signal_connect ((gpointer) Open_button, "clicked", G_CALLBACK (xa_open_archive), NULL);
	g_signal_connect ((gpointer) AddFile_button, "clicked", G_CALLBACK (xa_add_files_archive), NULL);
    g_signal_connect ((gpointer) Extract_button, "clicked", G_CALLBACK (xa_extract_archive), NULL);
    g_signal_connect ((gpointer) Exe_button, "clicked", G_CALLBACK (xa_convert_sfx), NULL);
	g_signal_connect ((gpointer) Delete_button, "clicked", G_CALLBACK (xa_delete_archive), NULL);
	g_signal_connect ((gpointer) View_button, "clicked", G_CALLBACK (xa_view_file_inside_archive), NULL);
	g_signal_connect ((gpointer) Stop_button, "clicked", G_CALLBACK (xa_cancel_archive), NULL);
	g_signal_connect (MainWindow, "key-press-event", G_CALLBACK (key_press_function), NULL);

	gtk_window_add_accel_group (GTK_WINDOW (MainWindow), accel_group);
	return MainWindow;
}

void xa_page_has_changed (GtkNotebook *notebook, GtkNotebookPage *page, guint page_num, gpointer user_data)
{
	gboolean new	= FALSE;
	gboolean open	= FALSE;
	gboolean add	= FALSE;
	gboolean extract= FALSE;
	gboolean exe	= FALSE;
	gboolean select	= FALSE;
	gboolean check	= FALSE;
	gboolean info	= FALSE;
	gint id;

	id = xa_find_archive_index ( page_num );
	if (id == -1)
		return;

	xa_set_window_title (MainWindow , archive[id]->path);
	if ( archive[id]->type == XARCHIVETYPE_BZIP2 || archive[id]->type == XARCHIVETYPE_GZIP )
	{
		new = open = TRUE;
		info = exe = FALSE;
	}
	else if (archive[id]->type == XARCHIVETYPE_RPM || archive[id]->type == XARCHIVETYPE_DEB)
	{
		new = open = extract = select = info = TRUE;
		exe = FALSE;
	}
	else if (archive[id]->type == XARCHIVETYPE_TAR_BZ2 || archive[id]->type == XARCHIVETYPE_TAR_GZ || archive[id]->type == XARCHIVETYPE_TAR )
	{
		new = open = add = extract = select = info = TRUE;
		check = exe = FALSE;
	}
	else if (archive[id]->type == XARCHIVETYPE_LHA)
	{
		new = open = add = extract = select = info = TRUE;
		check = TRUE;
		exe = FALSE;
	}
	else if (archive[id]->type == XARCHIVETYPE_RAR && unrar)
	{
		check = TRUE;
		add = exe = FALSE;
		new = open = extract = select = info = TRUE;
	}
	else
	{
		check = TRUE;
		new = open = add = extract = exe = select = info = TRUE;
	}
	gtk_widget_set_sensitive ( check_menu , check);
	gtk_widget_set_sensitive ( properties , info);
	xa_set_button_state (new,open,add,extract,exe,select);
	if (archive[id]->has_passwd)
		gtk_widget_show (viewport3);
	else
		gtk_widget_hide (viewport3);
}

void xa_add_page (XArchive *archive)
{
	GtkWidget *page_hbox, *label, *close_button, *image;
	GtkTooltips *close_button_tips = gtk_tooltips_new();
	gchar *filename_only;

	if (gtk_notebook_get_current_page(notebook) > -1)
		gtk_notebook_set_show_tabs (notebook,TRUE);
	else
		gtk_notebook_set_show_tabs (notebook,FALSE);

	archive->scrollwindow = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_policy ( GTK_SCROLLED_WINDOW (archive->scrollwindow) , GTK_POLICY_AUTOMATIC,GTK_POLICY_AUTOMATIC);
	gtk_widget_show (archive->scrollwindow);

	page_hbox = gtk_hbox_new(FALSE, 2);

	filename_only = g_strrstr ( archive->path, "/" );
    if (filename_only != NULL)
    {
        filename_only++;
		label = gtk_label_new (filename_only);
    }
	else
		label = gtk_label_new (archive->path);

	gtk_label_set_max_width_chars(GTK_LABEL(label), 50);
	gtk_label_set_ellipsize(GTK_LABEL(label), PANGO_ELLIPSIZE_START);
	gtk_box_pack_start(GTK_BOX(page_hbox), label, FALSE, FALSE, 0);

	close_button = gtk_button_new();
	gtk_tooltips_set_tip (close_button_tips, close_button, _("Close Archive"), NULL);
	g_signal_connect (G_OBJECT(close_button), "clicked", G_CALLBACK(xa_close_page), (gpointer) archive->scrollwindow);

    image = xa_main_window_find_image ("close.png", GTK_ICON_SIZE_MENU);
	gtk_container_add (GTK_CONTAINER(close_button), image);
	gtk_button_set_relief (GTK_BUTTON(close_button), GTK_RELIEF_NONE);
	gtk_box_pack_end (GTK_BOX(page_hbox), close_button, FALSE, FALSE, 0);
	gtk_widget_show_all (page_hbox);

	gtk_notebook_append_page (notebook, archive->scrollwindow, page_hbox);
	gtk_notebook_set_current_page(notebook, -1);
	archive->treeview = gtk_tree_view_new ();
	gtk_container_add (GTK_CONTAINER (archive->scrollwindow), archive->treeview);
	gtk_widget_show (archive->treeview);
	gtk_tree_view_set_rules_hint ( GTK_TREE_VIEW (archive->treeview) , TRUE );
	gtk_tree_view_set_search_equal_func (GTK_TREE_VIEW (archive->treeview),(GtkTreeViewSearchEqualFunc) treeview_select_search, NULL, NULL);
	GtkTreeSelection *sel = gtk_tree_view_get_selection( GTK_TREE_VIEW (archive->treeview) );
	gtk_tree_selection_set_mode(sel, GTK_SELECTION_MULTIPLE);

	gtk_drag_source_set (archive->treeview, GDK_BUTTON1_MASK, drag_targets, 1, GDK_ACTION_COPY | GDK_ACTION_MOVE | GDK_ACTION_LINK | GDK_ACTION_ASK);

	g_signal_connect ((gpointer) sel, "changed", G_CALLBACK (xa_activate_delete_and_view), NULL);
	g_signal_connect (G_OBJECT (archive->treeview), "drag-begin",			G_CALLBACK (drag_begin), NULL);
	g_signal_connect (G_OBJECT (archive->treeview), "drag-data-get",		G_CALLBACK (drag_data_get), NULL );
	g_signal_connect (G_OBJECT (archive->treeview), "drag-end",				G_CALLBACK (drag_end), NULL);
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

	current_page = gtk_notebook_get_current_page (notebook);

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

	if (current_page > 0 && archive[current_page]->passwd != NULL)
		gtk_entry_set_text (GTK_ENTRY(password_entry),archive[current_page]->passwd);
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


GtkWidget *view_win ( gchar *title)
{
	GtkWidget *view_window;
	GtkWidget *scrolledwindow2;
	GtkWidget *textview1;

	view_window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title (GTK_WINDOW (view_window), title);
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
	set_label ( comment_label , _("Has comment:"));
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

void set_label (GtkWidget *label , gchar *text)
{
    gchar *tmp_markup = g_strdup_printf ("<b>%s</b>",text );
    gtk_label_set_markup ( GTK_LABEL (label) , tmp_markup);
    g_free (tmp_markup);
}

int xa_progressbar_pulse (gpointer data)
{
	gtk_progress_bar_pulse(GTK_PROGRESS_BAR(progressbar) );
	return TRUE;
}

gint xa_find_archive_index ( gint page_num )
{
	GtkWidget *scrollwindow;
	gint i;

	scrollwindow = gtk_notebook_get_nth_page(notebook, page_num);
	for (i = 0; i < 1024; i++)
	{
		if (archive[i] != NULL && archive[i]->scrollwindow == scrollwindow)
			return i;
	}
	return -1;
}

gint xa_get_new_archive_idx()
{
	gint i;

	for(i = 0; i < 1024; i++)
	{
		if (archive[i] == NULL)
			return i;
	}
	return -1;
}
