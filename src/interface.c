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

#include <string.h>
#include <gdk/gdkkeysyms.h>
#include "interface.h"
#include "add_dialog.h"
#include "main.h"
#include "pref_dialog.h"
#include "string_utils.h"
#include "support.h"
#include "window.h"

GtkWidget *archive_dir_treeview;
GtkWidget *archiver_data;
GtkWidget *back_button;
GtkWidget *comment_data;
GtkWidget *comment_menu;
GtkWidget *compression_data;
GtkWidget *content_data;
GtkWidget *copy;
GtkWidget *cut;
GtkWidget *ddelete;
GtkWidget *delete_menu;
GtkWidget *deselect_all;
GtkWidget *eextract;
GtkWidget *encrypted_data;
GtkWidget *home_button;
GtkWidget *hpaned1;
GtkWidget *listing;
GtkWidget *location_entry;
GtkWidget *modified_data;
GtkWidget *name_data;
GtkWidget *number_of_files_data;
GtkWidget *open_popupmenu;
GtkWidget *password_entry_menu;
GtkWidget *paste;
GtkWidget *path_data;
GtkWidget *rename_menu;
GtkWidget *rrename;
GtkWidget *scrolledwindow2;
GtkWidget *selected_frame;
GtkWidget *selected_label;
GtkWidget *select_all;
GtkWidget *size_data;
GtkWidget *Stop_button;
GtkWidget *toolbar1;
GtkWidget *toolbar2;
GtkWidget *total_label;
GtkWidget *type_data;
GtkWidget *unsort_menu;
GtkWidget *up_button;
GtkWidget *view;
GtkWidget *view_shell_output1;
GtkWidget *xa_popup_menu;
GtkAccelGroup *accel_group;
GtkNotebook *notebook;
GtkTreeStore *archive_dir_model;
Progress *progress;

static GtkWidget *addfile;
static GtkWidget *AddFile_button;
static GtkWidget *check_menu;
static GtkWidget *close1;
static GtkWidget *exe_menu;
static GtkWidget *Extract_button;
static GtkWidget *extract_menu;
static GtkWidget *forward_button;
static GtkWidget *green_led;
static GtkWidget *new1;
static GtkWidget *New_button;
static GtkWidget *open1;
static GtkWidget *Open_button;
static GtkWidget *properties;
static GtkWidget *red_led;
static GtkWidget *save1;
static GtkWidget *select_pattern;
static gulong selchghid;

static const GtkTargetEntry drag_targets[] =
{
  { "XdndDirectSave0",0,0 },
};

static const GtkTargetEntry drop_targets[] =
{
  { "text/uri-list",0,0 },
};

static void xa_create_popup_menu ()
{
	GtkWidget *image6;
	GtkWidget *image7;
	GtkWidget *image8;
	GtkWidget *image1;
	GtkWidget *separator;
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

	eextract = gtk_image_menu_item_new_with_mnemonic(_("Extract"));
	gtk_widget_show(eextract);
	gtk_container_add(GTK_CONTAINER(xa_popup_menu), eextract);

	image9 =  xa_main_window_find_image ("xarchiver-extract.png",GTK_ICON_SIZE_MENU);
	gtk_widget_show (image9);
	gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(eextract), image9);

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

	image11 = gtk_image_new_from_stock("gtk-edit", GTK_ICON_SIZE_MENU);
	gtk_widget_show (image11);
	gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (rrename),image11);

	g_signal_connect(open_popupmenu, "activate", G_CALLBACK(xa_open_with_from_popupmenu), NULL);
	g_signal_connect(view, "activate", G_CALLBACK(xa_view_from_popupmenu), NULL);
	g_signal_connect(cut, "activate", G_CALLBACK(xa_clipboard_cut), NULL);
	g_signal_connect(copy, "activate", G_CALLBACK(xa_clipboard_copy), NULL);
	g_signal_connect(paste, "activate", G_CALLBACK(xa_clipboard_paste), NULL);
	g_signal_connect(eextract, "activate", G_CALLBACK(xa_extract_archive), NULL);
	g_signal_connect(ddelete, "activate", G_CALLBACK(xa_delete_archive), NULL);
	g_signal_connect(rrename, "activate", G_CALLBACK(xa_rename_archive), NULL);
}

static void xa_dir_sidebar_row_expanded (GtkTreeView *tree_view, GtkTreeIter *iter, GtkTreePath *path, gpointer model)
{
	if (gtk_tree_view_row_expanded(tree_view,path))
		gtk_tree_store_set(GTK_TREE_STORE(model),iter,0,"gtk-open",-1);
	else
		gtk_tree_store_set(GTK_TREE_STORE(model),iter,0,"gtk-directory",-1);
}

static void xa_dir_sidebar_drag_data_received (GtkWidget *widget, GdkDragContext *context, int x, int y, GtkSelectionData *data, unsigned int info, unsigned int time, gpointer user_data)
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
	gboolean full_path, dummy_password;

	model = gtk_tree_view_get_model(GTK_TREE_VIEW(widget));
	current_page = gtk_notebook_get_current_page(notebook);
	idx = xa_find_archive_index(current_page);
	if (idx < 0)
	{
		gtk_drag_finish(context,FALSE,FALSE,time);
		return;
	}
	if (!archive[idx]->can_add)
	{
		xa_show_message_dialog(GTK_WINDOW(xa_main_window), GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK, _("Can't perform this action:"), _("You can't add content to this archive type!"));
		gtk_drag_finish(context,FALSE,FALSE,time);
		return;
	}
	array = gtk_selection_data_get_uris(data);
	if (array == NULL || archive[idx]->child_pid)
	{
		xa_show_message_dialog (GTK_WINDOW (xa_main_window),GTK_DIALOG_MODAL,GTK_MESSAGE_ERROR,GTK_BUTTONS_OK,_("Sorry, I could not perform the operation!"),"");
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
	if (archive[idx]->location_path != NULL)
		g_free(archive[idx]->location_path);

	/* This is to have the dragged files stored inside current archive location entry */
	archive[idx]->location_path = g_strdup(full_pathname->str);
	dummy_password = archive[idx]->has_password;
	full_path = archive[idx]->do_full_path;

	archive[idx]->has_password = FALSE;
	archive[idx]->do_full_path = FALSE;

	xa_execute_add_commands(archive[idx], list, TRUE, NULL);

	archive[idx]->has_password = dummy_password;
	archive[idx]->do_full_path = full_path;

	g_string_free(full_pathname,TRUE);
	if (list != NULL)
	{
		g_slist_foreach(list,(GFunc) g_free,NULL);
		g_slist_free(list);
	}
	g_strfreev (array);
	gtk_drag_finish (context,TRUE,FALSE,time);
}

static gboolean xa_dir_sidebar_drag_motion_expand_timeout (gpointer user_data)
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

static gboolean xa_dir_sidebar_drag_motion (GtkWidget *widget, GdkDragContext *context, gint x, gint y, guint time, gpointer user_data)
{
	GtkTreePath *path;

	gtk_tree_view_get_model(GTK_TREE_VIEW(widget));
	gtk_tree_view_get_dest_row_at_pos (GTK_TREE_VIEW (widget),x,y,&path,NULL);
	if (path)
	{
		g_timeout_add_full(G_PRIORITY_LOW, 1000, xa_dir_sidebar_drag_motion_expand_timeout, NULL, NULL);
		g_object_set_data(G_OBJECT(context),"current_path",path);
	}
	/* This to set the focus on the dropped row */
	gtk_tree_view_set_drag_dest_row(GTK_TREE_VIEW(widget),path,GTK_TREE_VIEW_DROP_INTO_OR_BEFORE);
	gdk_drag_status(context, gdk_drag_context_get_suggested_action(context), time);
	return TRUE;
}

static void xa_restore_navigation (int idx)
{
	gboolean back = FALSE,up = FALSE,forward = FALSE,home=FALSE;

	/*If the pointers exist,we should show the icon*/
	if(archive[idx]->forward !=NULL)
		forward = TRUE;

	if(archive[idx]->back !=NULL)
		back = TRUE;

	if (archive[idx]->location_path)
	{
		/* If there's a slash on the path,we should allow UP and HOME operations */
		if (strstr(archive[idx]->location_path, "/"))
			home = up = TRUE;
	}
	gtk_widget_set_sensitive(back_button,back);
	gtk_widget_set_sensitive(forward_button,forward);
	gtk_widget_set_sensitive(up_button,up);
	gtk_widget_set_sensitive(home_button,home);
}

static void xa_page_has_changed (GtkNotebook *notebook, GTK_COMPAT_SWITCH_PAGE_TYPE page, guint page_num, gpointer user_data)
{
	gint id;

	id = xa_find_archive_index (page_num);
	if (id == -1)
		return;

	xa_set_window_title(xa_main_window,archive[id]->path[0]);
	xa_restore_navigation(id);
	xa_set_statusbar_message_for_displayed_rows(archive[id]);

	if (archive[id]->treeview != NULL)
	{
		GtkTreeSelection *selection;
		gint selected;

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
			gtk_widget_set_sensitive(delete_menu, archive[id]->can_delete);
			gtk_widget_set_sensitive(rename_menu, can_rename(archive[id]));
		}
		/* Let's set the location bar */
		if (archive[id]->location_path)
		{
			gchar *entry_utf8 = g_filename_display_name(archive[id]->location_path);
			gtk_entry_set_text(GTK_ENTRY(location_entry), entry_utf8);
			g_free(entry_utf8);
		}
		else
			gtk_entry_set_text(GTK_ENTRY(location_entry),"\0");

		if (GTK_IS_TREE_VIEW(archive[id]->treeview))
			gtk_widget_grab_focus (GTK_WIDGET(archive[id]->treeview));

		selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(archive_dir_treeview));
		g_signal_handler_block(selection, selchghid);

		xa_fill_dir_sidebar(archive[id],TRUE);

		if (archive[id]->location_path)
			xa_dir_sidebar_select_row(xa_find_entry_from_dirpath(archive[id], archive[id]->location_path));

		g_signal_handler_unblock(selection, selchghid);
	}
	xa_set_button_state(1, 1, 1, 1, archive[id]->can_test, 1, archive[id]->can_add, archive[id]->can_extract, archive[id]->can_sfx, archive[id]->has_comment, archive[id]->output, archive[id]->has_password);
}

static void xa_select_by_pattern_dialog (GtkMenuItem *menuitem, gpointer user_data)
{
	GTK_COMPAT_TOOLTIPS;
	GtkWidget *ddialog1;
	GtkWidget *ddialog_vbox1;
	GtkWidget *dhbox1;
	GtkWidget *pattern_label;
	GtkWidget *pattern_entry;
	GtkWidget *dialog_action_area1;
	GtkWidget *cancelbutton1;
	GtkWidget *okbutton1;
	GtkWidget *tmp_image,*select_hbox,*select_label;
	GtkWidget *alignment2;
	gchar *string;
	gboolean done = FALSE;
	gint current_page;
	gint id;

	current_page = gtk_notebook_get_current_page (notebook);
	id = xa_find_archive_index (current_page);

	ddialog1 = gtk_dialog_new ();
	gtk_window_set_title (GTK_WINDOW (ddialog1),_("Select by Pattern"));
	gtk_window_set_modal (GTK_WINDOW (ddialog1),TRUE);
	gtk_window_set_type_hint (GTK_WINDOW (ddialog1),GDK_WINDOW_TYPE_HINT_DIALOG);
	gtk_widget_set_size_request(ddialog1,286,93);
	gtk_window_set_transient_for (GTK_WINDOW (ddialog1),GTK_WINDOW (xa_main_window));
	ddialog_vbox1 = gtk_dialog_get_content_area(GTK_DIALOG(ddialog1));
	gtk_widget_show (ddialog_vbox1);

	dhbox1 = gtk_hbox_new (FALSE,10);
	gtk_widget_show (dhbox1);
	gtk_box_pack_start (GTK_BOX (ddialog_vbox1),dhbox1,TRUE,TRUE,0);
	gtk_container_set_border_width (GTK_CONTAINER (dhbox1),5);

	pattern_label = gtk_label_new (_("Pattern:"));
	gtk_widget_show (pattern_label);
	gtk_box_pack_start (GTK_BOX (dhbox1),pattern_label,FALSE,FALSE,0);

	pattern_entry = gtk_entry_new ();
	gtk_widget_set_tooltip_text(pattern_entry, _("example: *.txt; ac*"));
	gtk_widget_show (pattern_entry);
	gtk_box_pack_start (GTK_BOX (dhbox1),pattern_entry,TRUE,TRUE,0);
	gtk_entry_set_activates_default(GTK_ENTRY(pattern_entry),TRUE);

	dialog_action_area1 = gtk_dialog_get_action_area(GTK_DIALOG(ddialog1));
	gtk_widget_show (dialog_action_area1);
	gtk_button_box_set_layout (GTK_BUTTON_BOX (dialog_action_area1),GTK_BUTTONBOX_END);

	cancelbutton1 = gtk_button_new_from_stock ("gtk-cancel");
	gtk_widget_show (cancelbutton1);
	gtk_dialog_add_action_widget (GTK_DIALOG (ddialog1),cancelbutton1,GTK_RESPONSE_CANCEL);
	gtk_widget_set_can_default(cancelbutton1, TRUE);

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
	gtk_widget_set_can_default(okbutton1, TRUE);
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
	gtk_tree_model_foreach(archive[id]->model, select_matched_rows, string);
	g_free(string);

destroy_delete_dialog:
	gtk_widget_destroy (ddialog1);
}

static void xa_handle_navigation_buttons (GtkMenuItem *menuitem, gpointer user_data)
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
			if (archive[idx]->location_path != NULL)
			{
				g_free(archive[idx]->location_path);
				archive[idx]->location_path = NULL;
			}
			/* Let's unselect the row in the dir_sidebar */
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
				xa_dir_sidebar_select_row(archive[idx]->back->data);
			}

			archive[idx]->back = archive[idx]->back->next;
			xa_restore_navigation(idx);
		break;
		/* Up */
		case 2:
			if (archive[idx]->back)
				archive[idx]->forward = g_slist_prepend(archive[idx]->forward,archive[idx]->current_entry);

			/* Let's unselect the row in the dir_sidebar */
			selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (archive_dir_treeview));
			if (selection != NULL)
			{
				gtk_tree_selection_get_selected (selection,&model,&iter);
				gtk_tree_selection_unselect_iter(selection,&iter);
			}
			new_entry = xa_find_entry_from_dirpath(archive[idx], archive[idx]->location_path);
			xa_update_window_with_archive_entries(archive[idx],new_entry->prev);
			xa_dir_sidebar_select_row(new_entry->prev);

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
				xa_dir_sidebar_select_row(archive[idx]->forward->data);
				archive[idx]->forward = archive[idx]->forward->next;
			}
			xa_restore_navigation(idx);
		break;
	}
}

static void set_label (GtkWidget *label, gchar *text)
{
    gchar *tmp_markup = g_strdup_printf ("<b>%s</b>",text);
    gtk_label_set_markup ( GTK_LABEL (label),tmp_markup);
    g_free (tmp_markup);
}

static void xa_cancel_progress_bar (GtkButton *button, GPid *pid)
{
	if (pid != NULL && *pid != 0)
		kill(*pid, SIGINT);
}

static gboolean xa_close_progress_bar (GtkWidget *caller, GdkEvent *event, GPid *pid)
{
	if (pid != NULL && *pid != 0)
		kill(*pid, SIGINT);
	return TRUE;
}

static gint xa_slist_strcmp (gconstpointer a, gconstpointer b)
{
	return strcasecmp((const char *) a, (const char *) b);
}

/* TODO:
static void xa_icon_theme_changed (GtkIconTheme *icon_theme, gpointer data)
{
 * Here we should reload all the icons currently displayed according to the
 * new icon_theme. xa_get_pixbuf_icon_from_cache() is to be called as many
 * time as the filenames currently displayed. What of the other tabs then?
}*/

void xa_create_main_window (GtkWidget *xa_main_window, gboolean show_location, gboolean show_sidebar, gboolean show_toolbar)
{
	GTK_COMPAT_TOOLTIPS;
	GdkPixbuf *icon;
	GtkIconSize tmp_toolbar_icon_size;
	GtkTreeViewColumn *column;
	GtkCellRenderer *archive_dir_renderer;
	GtkWidget *vbox1, *hbox1, *hbox_sb, *menubar1, *menuitem1, *menuitem1_menu;
	GtkWidget *listing_submenu, *listing_text, *listing_html, *total_frame;
	GtkWidget *separatormenuitem1, *separatormenuitem2, *separatormenuitem3;
	GtkWidget *separatormenuitem4, *separatormenuitem5, *separatormenuitem6;
	GtkWidget *quit1, *menuitem2, *menuitem2_menu, *multi_extract_menu;
	GtkWidget *menuitem4, *menuitem4_menu, *about1, *help1, *prefs_menu;
	GtkWidget *separatortoolitem1, *separatortoolitem2, *separatortoolitem3;
	GtkWidget *image2, *tmp_image, *toolitem1, *location_label;

	xa_create_popup_menu();
	accel_group = gtk_accel_group_new ();
	xa_set_window_title (xa_main_window,NULL);

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
	tmp_image = gtk_image_new_from_stock ("gtk-justify-fill",GTK_ICON_SIZE_MENU);
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
	gtk_widget_add_accelerator(check_menu, "activate", accel_group, GDK_KEY_t, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);

	tmp_image = gtk_image_new_from_stock ("gtk-index",GTK_ICON_SIZE_MENU);
	gtk_widget_show (tmp_image);
	gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (check_menu),tmp_image);

	properties = gtk_image_menu_item_new_with_mnemonic (_("_Properties"));
	gtk_widget_show (properties);
	gtk_widget_set_sensitive ( properties,FALSE);
	gtk_container_add (GTK_CONTAINER (menuitem1_menu),properties);
	gtk_widget_add_accelerator(properties, "activate", accel_group, GDK_KEY_p, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);

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
	gtk_widget_add_accelerator(addfile, "activate", accel_group, GDK_KEY_d, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);

	image2 = xa_main_window_find_image ("xarchiver-add.png",GTK_ICON_SIZE_MENU);
	gtk_widget_show (image2);
	gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (addfile),image2);

	extract_menu = gtk_image_menu_item_new_with_mnemonic (_("_Extract"));
	gtk_widget_set_sensitive (extract_menu,FALSE);
	gtk_widget_show (extract_menu);
	gtk_container_add (GTK_CONTAINER (menuitem2_menu),extract_menu);
	gtk_widget_add_accelerator(extract_menu, "activate", accel_group, GDK_KEY_e, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);

	image2 =  xa_main_window_find_image ("xarchiver-extract.png",GTK_ICON_SIZE_MENU);
	gtk_widget_show (image2);
	gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (extract_menu),image2);

	delete_menu = gtk_image_menu_item_new_from_stock ("gtk-delete",NULL);
	gtk_widget_set_sensitive (delete_menu,FALSE);
	gtk_widget_show (delete_menu);
	gtk_container_add (GTK_CONTAINER (menuitem2_menu),delete_menu);
	gtk_widget_add_accelerator(delete_menu, "activate", accel_group, GDK_KEY_Delete, 0, GTK_ACCEL_VISIBLE);

	rename_menu = gtk_image_menu_item_new_with_mnemonic (_("Re_name"));
	gtk_widget_set_sensitive (rename_menu,FALSE);
	gtk_widget_show (rename_menu);
	gtk_container_add (GTK_CONTAINER (menuitem2_menu),rename_menu);

	tmp_image = gtk_image_new_from_stock("gtk-edit", GTK_ICON_SIZE_MENU);
	gtk_widget_show (tmp_image);
	gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (rename_menu),tmp_image);

	unsort_menu = gtk_image_menu_item_new_with_mnemonic(_("Uns_ort"));
	gtk_widget_set_sensitive(unsort_menu, FALSE);
	gtk_widget_show(unsort_menu);
	gtk_container_add(GTK_CONTAINER(menuitem2_menu), unsort_menu);

	tmp_image = gtk_image_new_from_stock("gtk-undo", GTK_ICON_SIZE_MENU);
	gtk_widget_show(tmp_image);
	gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(unsort_menu), tmp_image);

	separatormenuitem3 = gtk_separator_menu_item_new ();
	gtk_widget_show (separatormenuitem3);
	gtk_container_add (GTK_CONTAINER (menuitem2_menu),separatormenuitem3);
	gtk_widget_set_sensitive (separatormenuitem3,FALSE);

	exe_menu = gtk_image_menu_item_new_with_mnemonic (_("Make SF_X"));
	gtk_widget_set_sensitive (exe_menu,FALSE);
	gtk_widget_show (exe_menu);
	gtk_container_add (GTK_CONTAINER (menuitem2_menu),exe_menu);
	gtk_widget_add_accelerator(exe_menu, "activate", accel_group, GDK_KEY_x, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);

	image2 = gtk_image_new_from_stock ("gtk-execute",GTK_ICON_SIZE_MENU);
	gtk_widget_show (image2);
	gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (exe_menu),image2);

	multi_extract_menu = gtk_menu_item_new_with_mnemonic (_("_Multi-Extract"));
	gtk_widget_show (multi_extract_menu);
	gtk_container_add (GTK_CONTAINER (menuitem2_menu),multi_extract_menu);
	gtk_widget_add_accelerator(multi_extract_menu, "activate", accel_group, GDK_KEY_m, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);

	comment_menu = gtk_image_menu_item_new_with_mnemonic (_("Archive _comment"));
	gtk_widget_set_sensitive (comment_menu,FALSE);
	gtk_widget_show (comment_menu);
	gtk_container_add (GTK_CONTAINER (menuitem2_menu),comment_menu);
	gtk_widget_add_accelerator(comment_menu, "activate", accel_group, GDK_KEY_s, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);

	tmp_image = gtk_image_new_from_stock ("gtk-justify-left",GTK_ICON_SIZE_MENU);
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
	gtk_widget_add_accelerator(select_all, "activate", accel_group, GDK_KEY_a, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);

	tmp_image = gtk_image_new_from_stock ("gtk-select-all",GTK_ICON_SIZE_MENU);
	gtk_widget_show (tmp_image);
	gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (select_all),tmp_image);

	deselect_all = gtk_image_menu_item_new_with_mnemonic (_("Dese_lect all"));
	gtk_widget_show (deselect_all);
	gtk_container_add (GTK_CONTAINER (menuitem2_menu),deselect_all);
	gtk_widget_set_sensitive (deselect_all,FALSE);
	gtk_widget_add_accelerator(deselect_all, "activate", accel_group, GDK_KEY_l, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);

	select_pattern = gtk_menu_item_new_with_mnemonic (_("Select _by pattern"));
	gtk_widget_show (select_pattern);
	gtk_container_add (GTK_CONTAINER (menuitem2_menu),select_pattern);
	gtk_widget_set_sensitive (select_pattern,FALSE);
	gtk_widget_add_accelerator(select_pattern, "activate", accel_group, GDK_KEY_b, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);

	separatormenuitem5 = gtk_separator_menu_item_new ();
	gtk_widget_show (separatormenuitem5);
	gtk_container_add (GTK_CONTAINER (menuitem2_menu),separatormenuitem5);
	gtk_widget_set_sensitive (separatormenuitem5,FALSE);

	view_shell_output1 = gtk_image_menu_item_new_with_mnemonic (_("Cmd-line outp_ut"));
	gtk_container_add (GTK_CONTAINER (menuitem2_menu),view_shell_output1);
	gtk_widget_add_accelerator(view_shell_output1, "activate", accel_group, GDK_KEY_u, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
	gtk_widget_set_sensitive(view_shell_output1, FALSE);
	gtk_widget_show (view_shell_output1);

	image2 = gtk_image_new_from_stock ("gtk-find-and-replace",GTK_ICON_SIZE_MENU);
	gtk_widget_show (image2);
	gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (view_shell_output1),image2);

	password_entry_menu = gtk_image_menu_item_new_with_mnemonic(_("Enter passwo_rd"));
	gtk_widget_show (password_entry_menu);
	gtk_widget_set_sensitive (password_entry_menu,FALSE);
	gtk_container_add (GTK_CONTAINER (menuitem2_menu),password_entry_menu);
	gtk_widget_add_accelerator(password_entry_menu, "activate", accel_group, GDK_KEY_r, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);

	tmp_image = gtk_image_new_from_stock("gtk-dialog-authentication", GTK_ICON_SIZE_MENU);
	gtk_widget_show(tmp_image);
	gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(password_entry_menu), tmp_image);

	separatormenuitem6 = gtk_separator_menu_item_new ();
	gtk_widget_show (separatormenuitem6);
	gtk_container_add (GTK_CONTAINER (menuitem2_menu),separatormenuitem6);
	gtk_widget_set_sensitive (separatormenuitem6,FALSE);

	prefs_menu = gtk_image_menu_item_new_with_mnemonic (_("_Preferences"));
	gtk_widget_show (prefs_menu);
	gtk_container_add (GTK_CONTAINER (menuitem2_menu),prefs_menu);
	gtk_widget_add_accelerator(prefs_menu, "activate", accel_group, GDK_KEY_f, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);

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
	gtk_widget_add_accelerator(help1, "activate", accel_group, GDK_KEY_F1, 0, GTK_ACCEL_VISIBLE);

	tmp_image = gtk_image_new_from_stock ("gtk-help",GTK_ICON_SIZE_MENU);
	gtk_widget_show (tmp_image);
	gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (help1),tmp_image);

	about1 = gtk_image_menu_item_new_from_stock ("gtk-about",accel_group);
	gtk_widget_show (about1);
	gtk_container_add (GTK_CONTAINER (menuitem4_menu),about1);

	/* Create the toolbar */
	toolbar1 = gtk_toolbar_new ();
	if (show_toolbar)
		gtk_widget_show(toolbar1);
	gtk_box_pack_start (GTK_BOX (vbox1),toolbar1,FALSE,FALSE,0);
	gtk_toolbar_set_style (GTK_TOOLBAR (toolbar1),GTK_TOOLBAR_ICONS);
	tmp_toolbar_icon_size = gtk_toolbar_get_icon_size (GTK_TOOLBAR (toolbar1));

	tmp_image = gtk_image_new_from_stock ("gtk-new",tmp_toolbar_icon_size);
	gtk_widget_show (tmp_image);
	New_button = (GtkWidget*) gtk_tool_button_new (tmp_image,_("New"));
	gtk_widget_show (New_button);
	gtk_tool_item_set_homogeneous (GTK_TOOL_ITEM (New_button),FALSE);
	gtk_container_add (GTK_CONTAINER (toolbar1),New_button);
	gtk_tool_item_set_tooltip_text(GTK_TOOL_ITEM(New_button), _("Create a new archive"));

	tmp_image = gtk_image_new_from_stock ("gtk-open",tmp_toolbar_icon_size);
	gtk_widget_show (tmp_image);
	Open_button = (GtkWidget*) gtk_tool_button_new (tmp_image,_("Open"));
	gtk_widget_show (Open_button);
	gtk_tool_item_set_homogeneous (GTK_TOOL_ITEM (Open_button),FALSE);
	gtk_container_add (GTK_CONTAINER (toolbar1),Open_button);
	gtk_tool_item_set_tooltip_text(GTK_TOOL_ITEM(Open_button), _("Open an archive"));

	separatortoolitem1 = (GtkWidget*) gtk_separator_tool_item_new ();
	gtk_widget_show (separatortoolitem1);
	gtk_container_add (GTK_CONTAINER (toolbar1),separatortoolitem1);

	tmp_image = gtk_image_new_from_stock ("gtk-go-back",tmp_toolbar_icon_size);
	gtk_widget_show (tmp_image);
	back_button = (GtkWidget*) gtk_tool_button_new (tmp_image,_("Back"));
	gtk_widget_set_sensitive(back_button,FALSE);
	gtk_widget_show (back_button);
	gtk_tool_item_set_homogeneous (GTK_TOOL_ITEM (back_button),FALSE);
	gtk_container_add (GTK_CONTAINER (toolbar1),back_button);
	gtk_tool_item_set_tooltip_text(GTK_TOOL_ITEM(back_button), _("Back"));

	tmp_image = gtk_image_new_from_stock ("gtk-go-up",tmp_toolbar_icon_size);
	gtk_widget_show (tmp_image);
	up_button = (GtkWidget*) gtk_tool_button_new (tmp_image,_("Up"));
	gtk_widget_set_sensitive(up_button,FALSE);
	gtk_widget_show (up_button);
	gtk_tool_item_set_homogeneous (GTK_TOOL_ITEM (up_button),FALSE);
	gtk_container_add (GTK_CONTAINER (toolbar1),up_button);
	gtk_tool_item_set_tooltip_text(GTK_TOOL_ITEM(up_button), _("Up"));

	tmp_image = gtk_image_new_from_stock ("gtk-go-forward",tmp_toolbar_icon_size);
	gtk_widget_show (tmp_image);
	forward_button = (GtkWidget*) gtk_tool_button_new (tmp_image,_("Forward"));
	gtk_widget_set_sensitive(forward_button,FALSE);
	gtk_widget_show (forward_button);
	gtk_tool_item_set_homogeneous (GTK_TOOL_ITEM (forward_button),FALSE);
	gtk_container_add (GTK_CONTAINER (toolbar1),forward_button);
	gtk_tool_item_set_tooltip_text(GTK_TOOL_ITEM(forward_button), _("Forward"));

	tmp_image = gtk_image_new_from_stock ("gtk-goto-top",tmp_toolbar_icon_size);
	gtk_widget_show (tmp_image);
	home_button = (GtkWidget*) gtk_tool_button_new (tmp_image,_("Root"));
	gtk_widget_set_sensitive(home_button,FALSE);
	gtk_widget_show (home_button);
	gtk_tool_item_set_homogeneous (GTK_TOOL_ITEM (home_button),FALSE);
	gtk_container_add (GTK_CONTAINER (toolbar1),home_button);
	gtk_tool_item_set_tooltip_text(GTK_TOOL_ITEM(home_button), _("Root"));

	separatortoolitem3 = (GtkWidget*) gtk_separator_tool_item_new ();
	gtk_widget_show (separatortoolitem3);
	gtk_container_add (GTK_CONTAINER (toolbar1),separatortoolitem3);

	tmp_image = xa_main_window_find_image("xarchiver-add.png",GTK_ICON_SIZE_LARGE_TOOLBAR);
	gtk_widget_show (tmp_image);
	AddFile_button = (GtkWidget*) gtk_tool_button_new (tmp_image,_("Add"));
	gtk_widget_set_sensitive (AddFile_button,FALSE);
	gtk_widget_show (AddFile_button);
	gtk_tool_item_set_homogeneous (GTK_TOOL_ITEM (AddFile_button),FALSE);
	gtk_container_add (GTK_CONTAINER (toolbar1),AddFile_button);
	gtk_tool_item_set_tooltip_text(GTK_TOOL_ITEM(AddFile_button), _("Add files"));

	tmp_image = xa_main_window_find_image("xarchiver-extract.png",GTK_ICON_SIZE_LARGE_TOOLBAR);
	gtk_widget_show (tmp_image);
	Extract_button = (GtkWidget*) gtk_tool_button_new (tmp_image,_("Extract"));
	gtk_widget_set_sensitive (Extract_button,FALSE);
	gtk_widget_show (Extract_button);
	gtk_tool_item_set_homogeneous (GTK_TOOL_ITEM (Extract_button),FALSE);
	gtk_container_add (GTK_CONTAINER (toolbar1),Extract_button);
	gtk_tool_item_set_tooltip_text(GTK_TOOL_ITEM(Extract_button), _("Extract files"));

	separatortoolitem2 = (GtkWidget*) gtk_separator_tool_item_new ();
	gtk_widget_show (separatortoolitem2);
	gtk_container_add (GTK_CONTAINER (toolbar1),separatortoolitem2);

	tmp_image = gtk_image_new_from_stock ("gtk-stop",tmp_toolbar_icon_size);
	gtk_widget_show (tmp_image);
	Stop_button = (GtkWidget*) gtk_tool_button_new (tmp_image,_("Stop"));
	gtk_widget_set_sensitive (Stop_button,FALSE);
	gtk_widget_show (Stop_button);
	gtk_tool_item_set_homogeneous (GTK_TOOL_ITEM ( Stop_button),FALSE);
	gtk_container_add (GTK_CONTAINER (toolbar1),Stop_button);
	gtk_tool_item_set_tooltip_text(GTK_TOOL_ITEM(Stop_button), _("Cancel current operation"));

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

	/* Create the dir_sidebar */
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
	g_signal_connect (G_OBJECT (archive_dir_treeview),"row-collapsed",G_CALLBACK(xa_dir_sidebar_row_expanded),archive_dir_model);
	g_signal_connect(G_OBJECT(archive_dir_treeview), "row-expanded", G_CALLBACK(xa_dir_sidebar_row_expanded), archive_dir_model);
	g_signal_connect(G_OBJECT(archive_dir_treeview), "drag-data-received", G_CALLBACK(xa_dir_sidebar_drag_data_received), NULL);
	g_signal_connect(G_OBJECT(archive_dir_treeview), "drag-motion", G_CALLBACK(xa_dir_sidebar_drag_motion), NULL);
	GtkTreeSelection *sel = gtk_tree_view_get_selection(GTK_TREE_VIEW (archive_dir_treeview));
	selchghid = g_signal_connect(G_OBJECT(sel), "changed", G_CALLBACK(xa_dir_sidebar_row_selected), NULL);

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
	g_signal_connect(notebook, "switch-page", G_CALLBACK(xa_page_has_changed), NULL);

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
	gtk_widget_set_tooltip_text(green_led, _("This is Xarchiver's LED status. When it's flashing Xarchiver is busy."));

	red_led = gtk_image_new_from_icon_name ("gtk-no",GTK_ICON_SIZE_BUTTON);
	gtk_box_pack_start (GTK_BOX (hbox_sb),red_led,FALSE,FALSE,0);
	gtk_misc_set_alignment (GTK_MISC (red_led),1,1);

	g_signal_connect(new1, "activate", G_CALLBACK(xa_new_archive), NULL);
	g_signal_connect(open1, "activate", G_CALLBACK(xa_open_archive), NULL);
	g_signal_connect(listing_text, "activate", G_CALLBACK(xa_list_archive), GUINT_TO_POINTER(0));
	g_signal_connect(listing_html, "activate", G_CALLBACK(xa_list_archive), GUINT_TO_POINTER(1));
	g_signal_connect(save1, "activate", G_CALLBACK(xa_save_archive), NULL);
	g_signal_connect(check_menu, "activate", G_CALLBACK(xa_test_archive), NULL);
	g_signal_connect(properties, "activate", G_CALLBACK(xa_archive_properties), NULL);
	g_signal_connect(extract_menu, "activate", G_CALLBACK(xa_extract_archive), NULL);
	g_signal_connect(exe_menu, "activate", G_CALLBACK(xa_convert_sfx), NULL);
	g_signal_connect(addfile, "activate", G_CALLBACK(xa_add_files_archive), NULL);
	g_signal_connect(view_shell_output1, "activate", G_CALLBACK(xa_show_archive_output), NULL);
	g_signal_connect(select_all, "activate", G_CALLBACK(xa_select_all), NULL);
	g_signal_connect(deselect_all, "activate", G_CALLBACK(xa_deselect_all), NULL);
	g_signal_connect(select_pattern, "activate", G_CALLBACK(xa_select_by_pattern_dialog), NULL);
	g_signal_connect(password_entry_menu, "activate", G_CALLBACK(xa_enter_password), NULL);
	g_signal_connect(prefs_menu, "activate", G_CALLBACK(xa_show_prefs_dialog), NULL);
	g_signal_connect(close1, "activate", G_CALLBACK(xa_close_archive), NULL);
	g_signal_connect(quit1, "activate", G_CALLBACK(xa_quit_application), NULL);
	g_signal_connect(delete_menu, "activate", G_CALLBACK(xa_delete_archive), NULL);
	g_signal_connect(rename_menu, "activate", G_CALLBACK(xa_rename_archive), NULL);
	g_signal_connect(comment_menu, "activate", G_CALLBACK(xa_show_archive_comment), NULL);
	g_signal_connect(multi_extract_menu, "activate", G_CALLBACK(xa_show_multi_extract_dialog), NULL);
	g_signal_connect(help1, "activate", G_CALLBACK(xa_show_help), NULL);
	g_signal_connect(about1, "activate", G_CALLBACK(xa_about), NULL);
	g_signal_connect(unsort_menu, "activate", G_CALLBACK(xa_unsort), NULL);

	g_signal_connect(New_button, "clicked", G_CALLBACK(xa_new_archive), NULL);
	g_signal_connect(Open_button, "clicked", G_CALLBACK(xa_open_archive), NULL);

	g_signal_connect(back_button, "clicked", G_CALLBACK(xa_handle_navigation_buttons), GUINT_TO_POINTER(1));
	g_signal_connect(up_button, "clicked", G_CALLBACK(xa_handle_navigation_buttons), GUINT_TO_POINTER(2));
	g_signal_connect(forward_button, "clicked", G_CALLBACK(xa_handle_navigation_buttons), GUINT_TO_POINTER(3));
	g_signal_connect(home_button, "clicked", G_CALLBACK(xa_handle_navigation_buttons), GUINT_TO_POINTER(0));

	g_signal_connect(AddFile_button, "clicked", G_CALLBACK(xa_add_files_archive), NULL);
	g_signal_connect(Extract_button, "clicked", G_CALLBACK(xa_extract_archive), NULL);
	g_signal_connect(Stop_button, "clicked", G_CALLBACK(xa_cancel_archive), NULL);
	gtk_window_add_accel_group (GTK_WINDOW (xa_main_window),accel_group);
}

gboolean xa_flash_led_indicator (XArchive *archive)
{
	if (archive->child_ref == 0)
	{
		gtk_widget_show(green_led);
		gtk_widget_hide(red_led);
		return FALSE;
	}
	if (gtk_widget_get_visible(green_led))
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

void xa_add_page (XArchive *archive)
{
	GTK_COMPAT_TOOLTIPS;
	GtkWidget	*page_hbox,*label,*tab_label,*close_button,*image,*align;
	gchar *filename_only, *label_utf8;
	GtkRcStyle *rcstyle;
	GtkRequisition size;

	if (gtk_notebook_get_current_page(notebook) > -1)
		gtk_notebook_set_show_tabs (notebook,TRUE);
	else
		gtk_notebook_set_show_tabs (notebook,FALSE);

	archive->page = gtk_scrolled_window_new(NULL, NULL);
	g_object_set(G_OBJECT(archive->page), "hscrollbar-policy", GTK_POLICY_AUTOMATIC, "vscrollbar-policy", GTK_POLICY_AUTOMATIC, NULL);
	gtk_widget_show(archive->page);

	page_hbox = gtk_hbox_new(FALSE,0);

	filename_only = g_strrstr(archive->path[0], "/");
	if (filename_only != NULL)
	{
		filename_only++;
		label_utf8 = g_filename_display_name(filename_only);
		label = gtk_label_new(label_utf8);
		tab_label = gtk_label_new(label_utf8);
	}
	else
	{
		label_utf8 = g_filename_display_name(archive->path[0]);
		label = gtk_label_new(label_utf8);
		tab_label = gtk_label_new(label_utf8);
	}
	g_free(label_utf8);

	gtk_label_set_max_width_chars(GTK_LABEL(label),50);
	gtk_label_set_ellipsize(GTK_LABEL(label),PANGO_ELLIPSIZE_START);
	gtk_box_pack_start(GTK_BOX(page_hbox),label,FALSE,FALSE,0);

	close_button = gtk_button_new();
	gtk_button_set_focus_on_click(GTK_BUTTON(close_button),FALSE);
	gtk_button_set_relief (GTK_BUTTON(close_button),GTK_RELIEF_NONE);
	gtk_widget_set_tooltip_text(close_button, _("Close archive"));
	g_signal_connect(G_OBJECT(close_button), "clicked", G_CALLBACK(xa_close_archive), archive->page);

	rcstyle = gtk_rc_style_new();
	rcstyle->xthickness = rcstyle->ythickness = 0;
	gtk_widget_modify_style(close_button,rcstyle);
	g_object_unref(rcstyle);

	image = gtk_image_new_from_stock (GTK_STOCK_CLOSE,GTK_ICON_SIZE_MENU);
	gtk_widget_size_request(image,&size);
	gtk_widget_set_size_request(close_button,size.width,size.height);
	gtk_container_add (GTK_CONTAINER(close_button),image);
	align = gtk_alignment_new(1.0,0.0,0.0,0.0);
	gtk_container_add(GTK_CONTAINER(align),close_button);
	gtk_box_pack_start(GTK_BOX(page_hbox),align,TRUE,TRUE,0);
	gtk_widget_show_all(page_hbox);

	gtk_misc_set_alignment(GTK_MISC(tab_label),0.0,0);
	gtk_notebook_append_page_menu(notebook, archive->page, page_hbox, tab_label);
	gtk_notebook_set_current_page(notebook,-1);
	gtk_notebook_set_tab_reorderable(notebook, archive->page, TRUE);

	archive->treeview = gtk_tree_view_new ();
	gtk_container_add(GTK_CONTAINER(archive->page), archive->treeview);
	gtk_widget_show (archive->treeview);
	gtk_tree_view_set_rules_hint ( GTK_TREE_VIEW (archive->treeview),TRUE);
	gtk_tree_view_set_search_equal_func(GTK_TREE_VIEW(archive->treeview), treeview_select_search, NULL, NULL);
	GtkTreeSelection *sel = gtk_tree_view_get_selection( GTK_TREE_VIEW (archive->treeview));
	gtk_tree_selection_set_mode(sel,GTK_SELECTION_MULTIPLE);
	gtk_tree_view_set_rubber_banding(GTK_TREE_VIEW(archive->treeview),TRUE);

	gtk_drag_source_set (archive->treeview,GDK_BUTTON1_MASK,drag_targets,1,GDK_ACTION_COPY | GDK_ACTION_MOVE | GDK_ACTION_LINK | GDK_ACTION_ASK);
	g_signal_connect(sel, "changed", G_CALLBACK(xa_row_selected), archive);
	g_signal_connect (G_OBJECT (archive->treeview),"drag-begin",	G_CALLBACK (drag_begin),archive);
	g_signal_connect (G_OBJECT (archive->treeview),"drag-data-get", G_CALLBACK (drag_data_get),archive);
	g_signal_connect (G_OBJECT (archive->treeview),"drag-end",		G_CALLBACK (drag_end),NULL);
	g_signal_connect (G_OBJECT (archive->treeview),"row-activated", G_CALLBACK (xa_treeview_row_activated),archive);
	g_signal_connect (G_OBJECT (archive->treeview),"button-press-event",G_CALLBACK (xa_mouse_button_event),archive);
}

gboolean xa_check_password (XArchive *archive)
{
	GtkWidget *password_dialog,*dialog_vbox1,*vbox1,*hbox2,*image2,*vbox2,*label_pwd_required,*label_filename,*hbox1,*label34,*pw_password_entry;
	gchar *name, *name_utf8;
	gboolean done = FALSE;

	if (archive->password)
		return TRUE;

  	password_dialog = gtk_dialog_new_with_buttons(PACKAGE_NAME " " VERSION,
									GTK_WINDOW (xa_main_window),GTK_DIALOG_DESTROY_WITH_PARENT,
									GTK_STOCK_CANCEL,GTK_RESPONSE_CANCEL,GTK_STOCK_OK,GTK_RESPONSE_OK,NULL);

  	gtk_container_set_border_width (GTK_CONTAINER (password_dialog),6);
  	gtk_window_set_position (GTK_WINDOW (password_dialog),GTK_WIN_POS_CENTER_ON_PARENT);
  	gtk_window_set_resizable (GTK_WINDOW (password_dialog),FALSE);
  	gtk_window_set_type_hint (GTK_WINDOW (password_dialog),GDK_WINDOW_TYPE_HINT_DIALOG);

  	dialog_vbox1 = gtk_dialog_get_content_area(GTK_DIALOG(password_dialog));
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

	if (xa_main_window)
		label_pwd_required = gtk_label_new(_("<span weight='bold' size='larger'>Enter password for:</span>"));
	else
		label_pwd_required = gtk_label_new(_("<span weight='bold' size='larger'>Password required for:</span>"));

  	gtk_widget_show (label_pwd_required);
  	gtk_box_pack_start (GTK_BOX (vbox2),label_pwd_required,FALSE,FALSE,0);
  	gtk_label_set_use_markup (GTK_LABEL (label_pwd_required),TRUE);
  	gtk_misc_set_alignment (GTK_MISC (label_pwd_required),0,0.5);

  	name = g_path_get_basename(archive->path[0]);
  	name_utf8 = g_filename_display_name(name);
  	label_filename = gtk_label_new(name_utf8);
  	g_free(name_utf8);
  	g_free(name);
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
			break;

			case GTK_RESPONSE_OK:
			archive->password = g_strdup(gtk_entry_get_text(GTK_ENTRY(pw_password_entry)));
			if (*archive->password == 0)
			{
				xa_show_message_dialog (GTK_WINDOW (xa_main_window),GTK_DIALOG_MODAL,GTK_MESSAGE_ERROR,GTK_BUTTONS_OK,_("You missed the password!"),_("Please enter it!"));
				g_free(archive->password);
				archive->password = NULL;
				break;
			}
			done = TRUE;
			break;
		}
	}
	gtk_widget_destroy (password_dialog);

	return (archive->password != NULL);
}

gboolean select_matched_rows (GtkTreeModel *model, GtkTreePath *path, GtkTreeIter *iter, gpointer string)
{
	char **patterns;
	XEntry *entry = NULL;
	gint current_page;
	gint idx;

	current_page = gtk_notebook_get_current_page(notebook);
	idx = xa_find_archive_index (current_page);
	patterns = g_strsplit(string,";",-1);

	gtk_tree_model_get(model, iter, archive[idx]->columns - 1, &entry, -1);

	if (match_patterns(patterns, entry->filename, 0))
		gtk_tree_selection_select_iter(gtk_tree_view_get_selection (GTK_TREE_VIEW (archive[idx]->treeview)),iter);
	else
		gtk_tree_selection_unselect_iter(gtk_tree_view_get_selection (GTK_TREE_VIEW (archive[idx]->treeview)),iter);

	if (patterns != NULL)
		g_strfreev (patterns);

	return FALSE;
}

GtkWidget *xa_create_archive_properties_window()
{
	GtkWidget *archive_properties_window, *table1, *path_label, *modified_label;
	GtkWidget *size_label, *content_label, *comment_label, *compression_label;
	GtkWidget *number_of_files_label, *name_label, *type_label, *encrypted_label;
	GtkWidget *archiver_label;

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

	table1 = gtk_table_new(11, 2, FALSE);
	gtk_box_pack_start(GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(archive_properties_window))), table1, TRUE, TRUE, 0);
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

	archiver_label = gtk_label_new("");
	set_label(archiver_label, _("Archiver executable:"));
	gtk_table_attach(GTK_TABLE(table1), archiver_label, 0, 1, 10, 11, GTK_FILL, 0, 0, 0);
	gtk_misc_set_alignment(GTK_MISC(archiver_label), 0.99, 0.5);

/* */

	archiver_data = gtk_label_new("");
	gtk_misc_set_alignment(GTK_MISC(archiver_data), 0, 0.5);
	gtk_table_attach(GTK_TABLE(table1), archiver_data, 1, 2, 10, 11, GTK_FILL, 0, 0, 0);

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

void xa_set_button_state (gboolean new, gboolean open, gboolean list, gboolean save, gboolean test, gboolean close, gboolean add, gboolean extract, gboolean sfx, gboolean comment, GSList *output, gboolean password)
{
	gtk_widget_set_sensitive(new1, new);
	gtk_widget_set_sensitive(New_button, new);
	gtk_widget_set_sensitive(open1, open);
	gtk_widget_set_sensitive(Open_button, open);
	gtk_widget_set_sensitive(listing, list);
	gtk_widget_set_sensitive(save1, save);
	gtk_widget_set_sensitive(check_menu, test);
	gtk_widget_set_sensitive(properties, close);
	gtk_widget_set_sensitive(close1, close);
	gtk_widget_set_sensitive(addfile, add);
	gtk_widget_set_sensitive(AddFile_button, add);
	gtk_widget_set_sensitive(extract_menu, extract);
	gtk_widget_set_sensitive(Extract_button, extract);
	gtk_widget_set_sensitive(unsort_menu, gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(prefs_window->check_sort_filename_column)) && close);
	gtk_widget_set_sensitive(exe_menu, sfx);
	gtk_widget_set_sensitive(comment_menu, comment);
	gtk_widget_set_sensitive(select_all, close);
	gtk_widget_set_sensitive(select_pattern, close);
	gtk_widget_set_sensitive(view_shell_output1, output != NULL);
	gtk_widget_set_sensitive(password_entry_menu, password);
}

void xa_disable_delete_buttons (gboolean value)
{
    gtk_widget_set_sensitive (delete_menu,value);
    gtk_widget_set_sensitive (rename_menu,value);
}

void xa_show_progress_bar (XArchive *archive)
{
	GtkWidget *vbox1, *vbox2, *message, *hbox1, *icon_pixbuf, *total_label, *action_area, *cancel_button;
	GdkPixbuf *pixbuf;
	gchar *text = NULL,*markup;

	if (progress)
	{
		gtk_widget_show(progress->window);
		return;
	}

	progress = g_new0(Progress, 1);
	progress->window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title(GTK_WINDOW(progress->window), PACKAGE_NAME);
	gtk_window_set_position(GTK_WINDOW(progress->window), GTK_WIN_POS_CENTER_ALWAYS);
	gtk_widget_set_size_request(progress->window, 400, -1);
	gtk_window_set_resizable(GTK_WINDOW(progress->window), FALSE);
	gtk_container_set_border_width(GTK_CONTAINER(progress->window), 6);
	gtk_window_set_transient_for(GTK_WINDOW(progress->window), GTK_WINDOW(xa_main_window));

	pixbuf = gtk_icon_theme_load_icon(icon_theme,"xarchiver",24,0,NULL);
	gtk_window_set_icon(GTK_WINDOW(progress->window), pixbuf);
	g_object_unref(pixbuf);

	vbox1 = gtk_vbox_new (FALSE,12);
  	gtk_container_add(GTK_CONTAINER(progress->window), vbox1);
  	gtk_container_set_border_width (GTK_CONTAINER (vbox1),6);

	hbox1 = gtk_hbox_new (FALSE,12);
	gtk_box_pack_start (GTK_BOX (vbox1),hbox1,TRUE,TRUE,0);

	pixbuf = gtk_icon_theme_load_icon(icon_theme,"gnome-mime-application-zip",40,0,NULL);
	if (!pixbuf) pixbuf = gtk_icon_theme_load_icon(icon_theme,"package-x-generic",40,GTK_ICON_LOOKUP_FORCE_SIZE,NULL);
	icon_pixbuf = gtk_image_new_from_pixbuf(pixbuf);
	g_object_unref(pixbuf);

	gtk_box_pack_start (GTK_BOX (hbox1),icon_pixbuf,FALSE,FALSE,0);
	gtk_misc_set_alignment (GTK_MISC (icon_pixbuf),0.0,0.0);

	vbox2 = gtk_vbox_new (FALSE,0);
	gtk_box_pack_start (GTK_BOX (hbox1),vbox2,TRUE,TRUE,0);

	if (archive)
	{
		if (archive->status == XARCHIVESTATUS_EXTRACT)
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

	progress->label = gtk_label_new("");
	gtk_label_set_ellipsize(GTK_LABEL(progress->label), PANGO_ELLIPSIZE_END);
	gtk_misc_set_alignment(GTK_MISC(progress->label), 0, 0.5);
	gtk_box_pack_start(GTK_BOX(vbox2), progress->label, FALSE, FALSE, 12);
	if (archive)
	{
		text = g_filename_display_name(archive->path[0]);
		gtk_label_set_text(GTK_LABEL(progress->label), text);
		g_free(text);
	}
	else
	{
		progress->multi_extract = TRUE;
		total_label = gtk_label_new (_("Total Progress:"));
		gtk_box_pack_start(GTK_BOX(vbox2), total_label, FALSE, FALSE, 6);
		gtk_misc_set_alignment (GTK_MISC (total_label),0,0);
	}
	progress->bar = gtk_progress_bar_new();
	gtk_box_pack_start(GTK_BOX(vbox2), progress->bar, FALSE, FALSE, 0);
	if (archive)
	{
		gtk_progress_bar_set_pulse_step(GTK_PROGRESS_BAR(progress->bar), 0.033);

		action_area = gtk_hbutton_box_new ();
		gtk_button_box_set_layout (GTK_BUTTON_BOX (action_area),GTK_BUTTONBOX_END);
  		gtk_box_pack_end (GTK_BOX (vbox2),action_area,FALSE, TRUE, 0);

		cancel_button = gtk_button_new_from_stock ("gtk-cancel");
		gtk_box_pack_end (GTK_BOX (action_area),cancel_button,TRUE,TRUE,12);

		g_signal_connect(G_OBJECT(cancel_button), "clicked", G_CALLBACK(xa_cancel_progress_bar), &archive->child_pid);
		g_signal_connect(G_OBJECT(progress->window), "delete-event", G_CALLBACK(xa_close_progress_bar), &archive->child_pid);
	}
	gtk_widget_show_all(progress->window);
}

void xa_increase_progress_bar (Progress *progress, gchar *filename, double percent)
{
	gchar *message = NULL, *basename, *markup;

		gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(progress->bar), percent);
		message = g_strdup_printf("%.0f%%",(percent*100));
		gtk_progress_bar_set_text(GTK_PROGRESS_BAR(progress->bar), message);
		g_free(message);

		basename = g_path_get_basename(filename);
		markup = g_markup_printf_escaped("<b>%s</b>", basename);
		g_free(basename);
		gtk_label_set_markup(GTK_LABEL(progress->label), markup);
		g_free (markup);
}

gboolean xa_pulse_progress_bar (gpointer user_data)
{
	if (progress && gtk_widget_get_visible(progress->window))
	{
		gtk_progress_bar_pulse(GTK_PROGRESS_BAR(progress->bar));
		return TRUE;
	}
	else
		return FALSE;
}

void xa_combo_box_text_append_compressor_types (GtkComboBoxText *combo_box_text)
{
	int i;
	GSList *sorted = NULL;

	for (i = XARCHIVETYPE_FIRST; i < XARCHIVETYPE_TYPES; i++)
	{
		if (archiver[i].is_compressor)
		{
			GSList *list = archiver[i].type;

			while (list)
			{
				if (list->data)
					sorted = g_slist_append(sorted, list->data);

				list = list->next;
			}
		}
	}

	sorted = g_slist_sort(sorted, xa_slist_strcmp);

	while (sorted)
	{
		gtk_combo_box_text_append_text(combo_box_text, sorted->data);
		sorted = sorted->next;
	}

	g_slist_free(sorted);
}

GSList *xa_file_filter_add_archiver_pattern_sort (GtkFileFilter *filter)
{
	int i;
	GSList *sorted = NULL;

	for (i = XARCHIVETYPE_FIRST; i < XARCHIVETYPE_TYPES; i++)
	{
		GSList *list = archiver[i].glob;

		while (list)
		{
			if (*(char *) list->data == '*')
			{
				sorted = g_slist_insert_sorted(sorted, list->data, xa_slist_strcmp);
				gtk_file_filter_add_pattern(filter, list->data);
			}

			list = list->next;
		}
	}

	return sorted;
}
