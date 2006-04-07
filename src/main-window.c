/*
 *  Copyright (c) 2006 Stephan Arts <stephan.arts@hva.nl>
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

#include <stdlib.h>
#include <gtk/gtk.h>
#include <string.h>
#include <config.h>
#include <libintl.h>
#include <libxarchiver/libxarchiver.h>
#include "archive-chooser-dialog.h"
#include "property-dialog.h"
#include "main-window.h"
#define _(String) gettext(String)

static gint
lookup_widget_by_name(gconstpointer widget, gconstpointer name)
{
	gint retval = 1;
	const gchar *widget_name;
	if(widget)
	{
		widget_name = gtk_widget_get_name(GTK_WIDGET(widget));
		if(widget_name)
			if(!strcmp(widget_name, (gchar *)name))
				retval = 0;
	}
	return retval;
}

static void
xa_main_window_class_init (XAMainWindowClass *_class);

static void
xa_main_window_init (XAMainWindow *window);

void
xa_main_window_create_menubar(XAMainWindow *window);

void
xa_main_window_create_toolbar(XAMainWindow *window);

void
xa_main_window_create_contentlist(XAMainWindow *window);

void
xa_main_window_create_statusbar(XAMainWindow *window);

void 
xa_new_archive(GtkWidget *widget, gpointer data);

void 
xa_open_archive(GtkWidget *widget, gpointer data);

void 
xa_add_files(GtkWidget *widget, gpointer data);

void 
xa_add_folders(GtkWidget *widget, gpointer data);

void 
xa_extract_archive(GtkWidget *widget, gpointer data);

void 
xa_test_archive(GtkWidget *widget, gpointer data);

void 
xa_close_archive(GtkWidget *widget, gpointer data);

void 
xa_save_as_archive(GtkWidget *widget, gpointer data);

void 
xa_cancel_operation(GtkWidget *widget, gpointer data);

void 
xa_quit(GtkWidget *widget, gpointer data);

void
xa_show_property_dialog(GtkWidget *widget, gpointer data);

static GtkWidgetClass *xa_main_window_parent_class;

static guint xa_main_window_signals[9];

GSList *xa_main_window_widget_list;

GType
xa_main_window_get_type()
{
	static GType xa_main_window_type = 0;

	if(xa_main_window_type == 0)
	{
		static const GTypeInfo xa_main_window_info = 
		{
			sizeof(XAMainWindowClass),
			(GBaseInitFunc) NULL,
			(GBaseFinalizeFunc) NULL,
			(GClassInitFunc) xa_main_window_class_init,
			(GClassFinalizeFunc) NULL,
			NULL, 
			sizeof(XAMainWindow),
			0,
			(GInstanceInitFunc) xa_main_window_init, 
			NULL
		};
		xa_main_window_type = g_type_register_static(GTK_TYPE_WINDOW, "XAMainWindow", &xa_main_window_info, 0);
	}
	return xa_main_window_type;
}


GtkWidget *
xa_main_window_new()
{
	XAMainWindow *window;

	window = g_object_new(xa_main_window_get_type(), NULL);

	return GTK_WIDGET(window);
}

static void
xa_main_window_destroy (GtkObject *object)
{
	XAMainWindow *window;

	g_return_if_fail (object != NULL);
	g_return_if_fail (IS_XA_MAIN_WINDOW(object));

	window = XA_MAIN_WINDOW(object);

	if (GTK_OBJECT_CLASS (xa_main_window_parent_class)->destroy)
		(* GTK_OBJECT_CLASS (xa_main_window_parent_class)->destroy) (object);
}

static void
xa_main_window_show_all(GtkWidget *widget)
{
	gtk_widget_show(XA_MAIN_WINDOW(widget)->vbox);
	gtk_widget_show(XA_MAIN_WINDOW(widget)->menubar);
	gtk_widget_show(XA_MAIN_WINDOW(widget)->toolbar);
	gtk_widget_show(XA_MAIN_WINDOW(widget)->contentlist);
	gtk_widget_show(XA_MAIN_WINDOW(widget)->statusbar);
	
	gtk_widget_show(widget);
}

static void
xa_main_window_class_init (XAMainWindowClass *_class)
{
	GtkObjectClass *object_class = (GtkObjectClass *) _class;
	GtkWidgetClass *widget_class = (GtkWidgetClass *) _class;

	xa_main_window_parent_class = gtk_type_class(gtk_window_get_type());

	object_class->destroy = xa_main_window_destroy;

	widget_class->show_all = xa_main_window_show_all;

	xa_main_window_signals[0] = g_signal_new("xa_new_archive",
			G_TYPE_FROM_CLASS(_class),
			G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
			0,
			NULL,
			NULL,
			g_cclosure_marshal_VOID__POINTER,
			G_TYPE_NONE,
			1,
			G_TYPE_POINTER,
			NULL);

	xa_main_window_signals[1] = g_signal_new("xa_open_archive",
			G_TYPE_FROM_CLASS(_class),
			G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
			0,
			NULL,
			NULL,
			g_cclosure_marshal_VOID__POINTER,
			G_TYPE_NONE,
			1,
			G_TYPE_POINTER,
			NULL);

	xa_main_window_signals[2] = g_signal_new("xa_add_files",
			G_TYPE_FROM_CLASS(_class),
			G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
			0,
			NULL,
			NULL,
			g_cclosure_marshal_VOID__POINTER,
			G_TYPE_NONE,
			1,
			G_TYPE_POINTER,
			NULL);

	xa_main_window_signals[3] = g_signal_new("xa_add_folders",
			G_TYPE_FROM_CLASS(_class),
			G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
			0,
			NULL,
			NULL,
			g_cclosure_marshal_VOID__POINTER,
			G_TYPE_NONE,
			1,
			G_TYPE_POINTER,
			NULL);

	xa_main_window_signals[4] = g_signal_new("xa_extract_archive",
			G_TYPE_FROM_CLASS(_class),
			G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
			0,
			NULL,
			NULL,
			g_cclosure_marshal_VOID__POINTER,
			G_TYPE_NONE,
			1,
			G_TYPE_POINTER,
			NULL);

	xa_main_window_signals[5] = g_signal_new("xa_test_archive",
			G_TYPE_FROM_CLASS(_class),
			G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
			0,
			NULL,
			NULL,
			g_cclosure_marshal_VOID__VOID,
			G_TYPE_NONE,
			0,
			NULL);

	xa_main_window_signals[6] = g_signal_new("xa_close_archive",
			G_TYPE_FROM_CLASS(_class),
			G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
			0,
			NULL,
			NULL,
			g_cclosure_marshal_VOID__VOID,
			G_TYPE_NONE,
			0,
			NULL);

	xa_main_window_signals[7] = g_signal_new("xa_save_as_archive",
			G_TYPE_FROM_CLASS(_class),
			G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
			0,
			NULL,
			NULL,
			g_cclosure_marshal_VOID__POINTER,
			G_TYPE_NONE,
			1,
			G_TYPE_POINTER,
			NULL);

	xa_main_window_signals[8] = g_signal_new("xa_cancel_operation",
			G_TYPE_FROM_CLASS(_class),
			G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
			0,
			NULL,
			NULL,
			g_cclosure_marshal_VOID__VOID,
			G_TYPE_NONE,
			0,
			NULL);
}

static void
xa_main_window_init (XAMainWindow *window)
{
	window->progressbar = NULL;
	xa_main_window_widget_list = g_slist_alloc();
	xa_main_window_widget_list->data = window;

	gtk_window_set_title(GTK_WINDOW(window), PACKAGE_STRING);

	gtk_container_set_border_width(GTK_CONTAINER(window), 0);
	window->vbox = gtk_vbox_new(FALSE, 0);

	xa_main_window_create_menubar(window);
	xa_main_window_create_toolbar(window);
	xa_main_window_create_contentlist(window);
	xa_main_window_create_statusbar(window);

	gtk_container_add(GTK_CONTAINER(window), window->vbox);

	gtk_box_pack_start(GTK_BOX(window->vbox), window->menubar, FALSE, TRUE, 0); gtk_box_pack_start(GTK_BOX(window->vbox), window->toolbar, FALSE, TRUE, 0); 
	gtk_box_pack_start(GTK_BOX(window->vbox), window->scrollwindow, TRUE, TRUE, 0);
	gtk_box_pack_end(GTK_BOX(window->vbox), window->statusbar, FALSE, TRUE, 1);
}

void
xa_main_window_create_menubar(XAMainWindow *window)
{
	GtkWidget *tmp_image;
	GtkWidget *separator;

	GtkAccelGroup *accel_group = gtk_accel_group_new();
	GtkWidget *menu_bar = gtk_menu_bar_new();

	// Archive Menu
	GtkWidget *archive_item = gtk_menu_item_new_with_mnemonic(_("_Archive"));
	GtkWidget *archive_menu = gtk_menu_new();
	gtk_widget_show(archive_item);
	gtk_widget_show(archive_menu);

	GtkWidget *new      = gtk_image_menu_item_new_from_stock(GTK_STOCK_NEW, accel_group);
	GtkWidget *open     = gtk_image_menu_item_new_from_stock(GTK_STOCK_OPEN, accel_group);
	GtkWidget *save_as  = gtk_image_menu_item_new_from_stock(GTK_STOCK_SAVE_AS, accel_group);
	GtkWidget *close    = gtk_image_menu_item_new_from_stock(GTK_STOCK_CLOSE, accel_group);

	separator = gtk_menu_item_new();

	GtkWidget *test  = gtk_image_menu_item_new_with_mnemonic(_("_Test"));
	GtkWidget *properties = gtk_image_menu_item_new_from_stock(GTK_STOCK_PROPERTIES, accel_group);
	GtkWidget *quit  = gtk_image_menu_item_new_from_stock(GTK_STOCK_QUIT, accel_group);


	gtk_menu_shell_append(GTK_MENU_SHELL(archive_menu), new);
	gtk_menu_shell_append(GTK_MENU_SHELL(archive_menu), open);
	gtk_menu_shell_append(GTK_MENU_SHELL(archive_menu), save_as);
	gtk_menu_shell_append(GTK_MENU_SHELL(archive_menu), close);
	gtk_menu_shell_append(GTK_MENU_SHELL(archive_menu), separator);
	gtk_menu_shell_append(GTK_MENU_SHELL(archive_menu), test);
	gtk_menu_shell_append(GTK_MENU_SHELL(archive_menu), properties);
	gtk_menu_shell_append(GTK_MENU_SHELL(archive_menu), quit);

	tmp_image = gtk_image_new_from_stock(GTK_STOCK_INDEX, GTK_ICON_SIZE_MENU);
	gtk_widget_show(tmp_image);
	gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(test), tmp_image);


	g_signal_connect(G_OBJECT(quit), "activate", G_CALLBACK(xa_quit), window);
	g_signal_connect(G_OBJECT(open), "activate", G_CALLBACK(xa_open_archive), window);
	g_signal_connect(G_OBJECT(test), "activate", G_CALLBACK(xa_test_archive), window);
	g_signal_connect(G_OBJECT(close), "activate", G_CALLBACK(xa_close_archive), window);
	g_signal_connect(G_OBJECT(properties), "activate", G_CALLBACK(xa_show_property_dialog), window);

	gtk_widget_show(new);
	gtk_widget_show(open);
	gtk_widget_show(save_as);
	gtk_widget_show(close);
	gtk_widget_show(quit);
	gtk_widget_show(separator);
	gtk_widget_show(test);
	gtk_widget_show(properties);

	
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(archive_item), archive_menu);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu_bar), archive_item);

	// Action Menu
	GtkWidget *action_item = gtk_menu_item_new_with_mnemonic(_("A_ction"));
	GtkWidget *action_menu = gtk_menu_new();
	gtk_widget_show(action_item);
	gtk_widget_show(action_menu);

	GtkWidget *add_file = gtk_image_menu_item_new_with_mnemonic(_("_Add File(s)"));
	GtkWidget *add_folder = gtk_image_menu_item_new_with_mnemonic(_("Add _Folder"));
	GtkWidget *extract = gtk_image_menu_item_new_with_mnemonic(_("_Extract"));
	separator = gtk_separator_menu_item_new();
	GtkWidget *delete  = gtk_image_menu_item_new_from_stock(GTK_STOCK_DELETE, accel_group);

	g_signal_connect(G_OBJECT(add_file), "activate", G_CALLBACK(xa_add_files), window);
	g_signal_connect(G_OBJECT(add_folder), "activate", G_CALLBACK(xa_add_folders), window);
	g_signal_connect(G_OBJECT(extract), "activate", G_CALLBACK(xa_extract_archive), window);


	gtk_menu_shell_append(GTK_MENU_SHELL(action_menu), add_file);
	gtk_menu_shell_append(GTK_MENU_SHELL(action_menu), add_folder);
	gtk_menu_shell_append(GTK_MENU_SHELL(action_menu), extract);
	gtk_menu_shell_append(GTK_MENU_SHELL(action_menu), separator);
	gtk_menu_shell_append(GTK_MENU_SHELL(action_menu), delete);

	gtk_widget_show(add_file);
	gtk_widget_show(add_folder);
	gtk_widget_show(extract);
	gtk_widget_show(separator);
	gtk_widget_show(delete);
	
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(action_item), action_menu);

	gtk_menu_shell_append(GTK_MENU_SHELL(menu_bar), action_item);

	// Help Menu
	GtkWidget *help_item = gtk_menu_item_new_with_mnemonic(_("_Help"));
	GtkWidget *help_menu = gtk_menu_new();
	gtk_widget_show(help_item);
	gtk_widget_show(help_menu);

	GtkWidget *help = gtk_image_menu_item_new_from_stock(GTK_STOCK_HELP, accel_group);
	GtkWidget *about  = gtk_image_menu_item_new_from_stock(GTK_STOCK_ABOUT, accel_group);

	gtk_menu_shell_append(GTK_MENU_SHELL(help_menu), help);
	gtk_menu_shell_append(GTK_MENU_SHELL(help_menu), about);

	gtk_widget_show(help);
	gtk_widget_show(about);

	gtk_menu_item_set_submenu(GTK_MENU_ITEM(help_item), help_menu);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu_bar), help_item);

	g_slist_append(xa_main_window_widget_list, help);
	g_slist_append(xa_main_window_widget_list, about);
	g_slist_append(xa_main_window_widget_list, add_file);
	g_slist_append(xa_main_window_widget_list, add_folder);
	g_slist_append(xa_main_window_widget_list, extract);
	g_slist_append(xa_main_window_widget_list, delete);
	g_slist_append(xa_main_window_widget_list, help_item);
	g_slist_append(xa_main_window_widget_list, new);
	g_slist_append(xa_main_window_widget_list, open);
	g_slist_append(xa_main_window_widget_list, save_as);
	g_slist_append(xa_main_window_widget_list, close);
	g_slist_append(xa_main_window_widget_list, quit);
	g_slist_append(xa_main_window_widget_list, test);
	g_slist_append(xa_main_window_widget_list, properties);

	gtk_widget_set_name(test,       "xa-button-test");
	gtk_widget_set_name(properties, "xa-button-properties");
	gtk_widget_set_name(quit,       "xa-button-quit");
	gtk_widget_set_name(close,      "xa-button-close");
	gtk_widget_set_name(new,        "xa-button-new");
	gtk_widget_set_name(open,       "xa-button-open");
	gtk_widget_set_name(save_as,    "xa-button-save-as");
	gtk_widget_set_name(add_file,   "xa-button-add-file");
	gtk_widget_set_name(add_folder, "xa-button-add-folder");
	gtk_widget_set_name(delete,     "xa-button-delete");
	gtk_widget_set_name(help,       "xa-button-help");
	gtk_widget_set_name(about,      "xa-button-about");
	gtk_widget_set_name(extract,    "xa-button-extract");
	gtk_widget_set_name(help_item,  "xa-button-help-item");

	window->menubar = menu_bar;
}

void
xa_main_window_create_toolbar(XAMainWindow *window)
{
	GtkWidget *tool_bar = gtk_toolbar_new();
	GtkWidget *tmpimage;
	GtkToolItem *separator;
	
	gtk_toolbar_set_style(GTK_TOOLBAR(tool_bar), GTK_TOOLBAR_BOTH);

	GtkToolItem *new = gtk_tool_button_new_from_stock (GTK_STOCK_NEW);
	GtkToolItem *open = gtk_tool_button_new_from_stock (GTK_STOCK_OPEN);
	GtkToolItem *cancel = gtk_tool_button_new_from_stock (GTK_STOCK_STOP);

	tmpimage = gtk_image_new_from_stock(GTK_STOCK_MISSING_IMAGE, GTK_ICON_SIZE_LARGE_TOOLBAR);
	gtk_widget_show(tmpimage);
	GtkToolItem *add_file = gtk_tool_button_new (tmpimage, _("Add Files"));

	tmpimage = gtk_image_new_from_stock(GTK_STOCK_MISSING_IMAGE, GTK_ICON_SIZE_LARGE_TOOLBAR);
	gtk_widget_show(tmpimage);
	GtkToolItem *add_folder = gtk_tool_button_new (tmpimage, _("Add Folder"));

	tmpimage = gtk_image_new_from_stock(GTK_STOCK_MISSING_IMAGE, GTK_ICON_SIZE_LARGE_TOOLBAR);
	gtk_widget_show(tmpimage);
	GtkToolItem *extract = gtk_tool_button_new (tmpimage, _("Extract"));

	gtk_toolbar_insert(GTK_TOOLBAR(tool_bar), GTK_TOOL_ITEM(new), 0);
	gtk_toolbar_insert(GTK_TOOLBAR(tool_bar), GTK_TOOL_ITEM(open), 1);
	gtk_toolbar_insert(GTK_TOOLBAR(tool_bar), GTK_TOOL_ITEM(add_file), 2);
	gtk_toolbar_insert(GTK_TOOLBAR(tool_bar), GTK_TOOL_ITEM(add_folder), 3);
	gtk_toolbar_insert(GTK_TOOLBAR(tool_bar), GTK_TOOL_ITEM(extract), 4);
	gtk_toolbar_insert(GTK_TOOLBAR(tool_bar), GTK_TOOL_ITEM(cancel), 5);


	gtk_widget_show(GTK_WIDGET(new));
	gtk_widget_show(GTK_WIDGET(open));
	gtk_widget_show(GTK_WIDGET(extract));
	gtk_widget_show(GTK_WIDGET(add_file));
	gtk_widget_show(GTK_WIDGET(add_folder));
	gtk_widget_show(GTK_WIDGET(cancel));

	// insert separators
	separator = gtk_separator_tool_item_new();
	gtk_widget_show(GTK_WIDGET(separator));
	gtk_toolbar_insert(GTK_TOOLBAR(tool_bar), GTK_TOOL_ITEM(separator), 2);
	separator = gtk_separator_tool_item_new();
	gtk_widget_show(GTK_WIDGET(separator));
	gtk_toolbar_insert(GTK_TOOLBAR(tool_bar), GTK_TOOL_ITEM(separator), 6);

	// connect signals
	g_signal_connect(G_OBJECT(open), "clicked", G_CALLBACK(xa_open_archive), window);
	g_signal_connect(G_OBJECT(extract), "clicked", G_CALLBACK(xa_extract_archive), window);
	g_signal_connect(G_OBJECT(add_file), "clicked", G_CALLBACK(xa_add_files), window);
	g_signal_connect(G_OBJECT(add_folder), "clicked", G_CALLBACK(xa_add_folders), window);
	g_signal_connect(G_OBJECT(cancel), "clicked", G_CALLBACK(xa_cancel_operation), window);

	g_slist_append(xa_main_window_widget_list, new);
	g_slist_append(xa_main_window_widget_list, open);
	g_slist_append(xa_main_window_widget_list, extract);
	g_slist_append(xa_main_window_widget_list, add_file);
	g_slist_append(xa_main_window_widget_list, add_folder);
	
	gtk_widget_set_name(GTK_WIDGET(new),        "xa-button-new");
	gtk_widget_set_name(GTK_WIDGET(open),       "xa-button-open");
	gtk_widget_set_name(GTK_WIDGET(add_file),   "xa-button-add-file");
	gtk_widget_set_name(GTK_WIDGET(add_folder), "xa-button-add-folder");
	gtk_widget_set_name(GTK_WIDGET(extract),    "xa-button-extract");

	window->toolbar = tool_bar;
}

void
xa_main_window_create_statusbar(XAMainWindow *window)
{
	GtkWidget *statusbar = gtk_statusbar_new();
	GtkWidget *progressbar = gtk_progress_bar_new();
	GtkWidget *viewport = gtk_viewport_new(NULL, NULL);
	GtkWidget *passwd_image = gtk_image_new_from_stock(GTK_STOCK_DIALOG_AUTHENTICATION, GTK_ICON_SIZE_MENU);

	gtk_container_set_border_width(GTK_CONTAINER(statusbar), 1);
	gtk_progress_configure(GTK_PROGRESS(progressbar), 0, 0, 100);
	gtk_statusbar_set_has_resize_grip(GTK_STATUSBAR(statusbar), FALSE);
	gtk_box_pack_start(GTK_BOX(statusbar), progressbar, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(statusbar), viewport, FALSE, FALSE, 0);

	gtk_container_add(GTK_CONTAINER(viewport), passwd_image);
	gtk_widget_show(statusbar);
	gtk_widget_show(passwd_image);

	g_slist_append(xa_main_window_widget_list, viewport);
	gtk_widget_set_name(viewport,"xa-passwd");

	window->statusbar = statusbar;
	window->progressbar = progressbar;
}

void
xa_main_window_set_widget_sensitive (XAMainWindow *window, gchar *name, gboolean sensitive)
{
	GtkWidget *button;

	GSList *_widget_list = g_slist_find_custom(xa_main_window_widget_list, name, lookup_widget_by_name);
	while(_widget_list)
	{
		button = (_widget_list->data);
		gtk_widget_set_sensitive(button, sensitive);
		_widget_list = _widget_list->next;
		if(_widget_list)
			_widget_list = g_slist_find_custom(_widget_list, name, lookup_widget_by_name);
	}
}

void
xa_main_window_set_widget_visible (XAMainWindow *window, gchar *name, gboolean visible)
{
	GtkWidget *button;

	GSList *_widget_list = g_slist_find_custom(xa_main_window_widget_list, name, lookup_widget_by_name);
	while(_widget_list)
	{
		button = (_widget_list->data);
		if(visible)
			gtk_widget_show(button);
		else
			gtk_widget_hide(button);
		_widget_list = _widget_list->next;
		if(_widget_list)
			_widget_list = g_slist_find_custom(_widget_list, name, lookup_widget_by_name);
	}
}

void
xa_main_window_create_contentlist(XAMainWindow *window)
{
	GtkWidget *scrollwindow = gtk_scrolled_window_new(NULL,NULL);
	GtkWidget *treeview = gtk_tree_view_new();

	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrollwindow), GTK_SHADOW_ETCHED_IN);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrollwindow), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

	gtk_container_add(GTK_CONTAINER(scrollwindow), treeview);

	gtk_widget_show(scrollwindow);
	gtk_widget_show(treeview);

	window->contentlist = treeview;
	window->scrollwindow = scrollwindow;
}

// TODO:
// Check if tree_view already has a model, and free it.
void
xa_main_window_set_list_interface (XAMainWindow *window, int nc, gchar *column_names[], GType column_types[])
{
	int i = 0;
	GtkTreeViewColumn *column;
	GtkCellRenderer *renderer;
	GtkListStore *list_store;

	list_store = gtk_list_store_newv(nc, (GType *)column_types);
	gtk_tree_view_set_model(GTK_TREE_VIEW(window->contentlist), GTK_TREE_MODEL(list_store));
	gtk_tree_view_set_rules_hint(GTK_TREE_VIEW(window->contentlist), TRUE);
	renderer = gtk_cell_renderer_text_new ();
	for(i = 0; i < nc; i++)
	{
		column = gtk_tree_view_column_new_with_attributes(column_names[i], renderer, "text", i, NULL);
		gtk_tree_view_column_set_resizable(column, TRUE);
		gtk_tree_view_column_set_sort_column_id(column, i);
		gtk_tree_view_append_column (GTK_TREE_VIEW (window->contentlist), column);
	}
}

void
xa_main_window_append_list(XAMainWindow *window, GList *fields)
{
	unsigned short int i = 0;
	GtkTreeIter iter;
	GtkTreeModel *list_store = gtk_tree_view_get_model(GTK_TREE_VIEW(window->contentlist));
	g_object_ref(list_store);
	gtk_tree_view_set_model(GTK_TREE_VIEW(window->contentlist), NULL);
	
	gtk_list_store_append (GTK_LIST_STORE(list_store), &iter);
	while ( fields )
	{
		if ( i == gtk_tree_model_get_n_columns(list_store) )
		{
			gtk_list_store_append (GTK_LIST_STORE(list_store), &iter);
			i = 0;
		}
		else
		{
			gtk_list_store_set(GTK_LIST_STORE(list_store), &iter, i, fields->data, -1);
			fields = fields->next;
			i++;
		}
	}
	gtk_tree_view_set_model(GTK_TREE_VIEW(window->contentlist), list_store);
	g_object_unref(list_store);
}

void 
xa_new_archive(GtkWidget *widget, gpointer data)
{
	g_signal_emit(G_OBJECT(data), xa_main_window_signals[0], 0, NULL);
}

void 
xa_open_archive(GtkWidget *widget, gpointer data)
{
	gchar *filename = NULL;

	GtkWidget *dialog = gtk_file_chooser_dialog_new(_("Open Archive"),
			GTK_WINDOW(data),
			GTK_FILE_CHOOSER_ACTION_OPEN,
			GTK_STOCK_CANCEL,
			GTK_RESPONSE_CANCEL,
			GTK_STOCK_OPEN,
			GTK_RESPONSE_OK,
			NULL);

	if(gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_OK)
	{
		gtk_widget_hide(dialog);
		filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
		g_signal_emit(G_OBJECT(data), xa_main_window_signals[1], 0, filename);
	}

	if(filename)
		g_free(filename);
	gtk_widget_destroy(dialog);
}

void 
xa_add_files(GtkWidget *widget, gpointer data)
{
	GSList *files = NULL;

	GtkWidget *dialog = gtk_file_chooser_dialog_new(_("Add Files"),
			GTK_WINDOW(data),
			GTK_FILE_CHOOSER_ACTION_OPEN,
			GTK_STOCK_CANCEL,
			GTK_RESPONSE_CANCEL,
			GTK_STOCK_OPEN,
			GTK_RESPONSE_OK,
			NULL);

	gtk_file_chooser_set_select_multiple(GTK_FILE_CHOOSER(dialog), TRUE);

	if(gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_OK)
	{
		gtk_widget_hide(dialog);
		files = gtk_file_chooser_get_filenames(GTK_FILE_CHOOSER(dialog));
		g_signal_emit(G_OBJECT(data), xa_main_window_signals[2], 0, files);
	}

	if(files)
	{
		g_slist_foreach(files, (GFunc) g_free, NULL);
		g_slist_free(files);
	}
	gtk_widget_destroy(dialog);
}

void 
xa_add_folders(GtkWidget *widget, gpointer data)
{
	gchar *foldername = NULL;

	GtkWidget *dialog = gtk_file_chooser_dialog_new(_("Add Folder"),
			GTK_WINDOW(data),
			GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER,
			GTK_STOCK_CANCEL,
			GTK_RESPONSE_CANCEL,
			GTK_STOCK_OPEN,
			GTK_RESPONSE_OK,
			NULL);

	if(gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_OK)
	{
		gtk_widget_hide(dialog);
		foldername = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
		g_signal_emit(G_OBJECT(data), xa_main_window_signals[3], 0, foldername);
	}

	if(foldername)
		g_free(foldername);
	gtk_widget_destroy(dialog);
} 

void 
xa_extract_archive(GtkWidget *widget, gpointer data)
{

	GtkWidget *dialog = xa_archive_chooser_dialog_new(_("Extract archive"), 
			GTK_WINDOW(data));

	if(gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_OK)
	{
		gtk_widget_hide(dialog);
		g_signal_emit(G_OBJECT(data), xa_main_window_signals[4], 0, NULL);
	}
}

void 
xa_test_archive(GtkWidget *widget, gpointer data)
{
	g_signal_emit(G_OBJECT(data), xa_main_window_signals[5], 0);
}

void 
xa_close_archive(GtkWidget *widget, gpointer data)
{
	g_signal_emit(G_OBJECT(data), xa_main_window_signals[6], 0);
}

void 
xa_save_as_archive(GtkWidget *widget, gpointer data)
{
	g_signal_emit(G_OBJECT(data), xa_main_window_signals[7], 0, NULL);
}

void 
xa_cancel_operation(GtkWidget *widget, gpointer data)
{
	g_signal_emit(G_OBJECT(data), xa_main_window_signals[8], 0);
}

void 
xa_quit(GtkWidget *widget, gpointer data)
{
	gtk_widget_destroy(GTK_WIDGET(data));
}

void
xa_main_window_set_statusbar_value   (XAMainWindow *window, gchar *value)
{
	gtk_statusbar_push(GTK_STATUSBAR(window->statusbar), 0, value);
}

void
xa_main_window_set_progressbar_value (XAMainWindow *window, gdouble value)
{
	if(value >= 0)
	{
		gtk_progress_set_value(GTK_PROGRESS(window->progressbar), value);
		gtk_widget_show(window->progressbar);
	}
	else
		gtk_widget_hide(window->progressbar);
}

void
xa_show_property_dialog(GtkWidget *widget, gpointer data)
{
	XAMainWindow *window = XA_MAIN_WINDOW(data);
	if(window->propertywindow)
		gtk_dialog_run(GTK_DIALOG(window->propertywindow));
}

void
xa_main_window_set_property_window (XAMainWindow *window, XAPropertyDialog *propertywindow)
{
	if(window->propertywindow)
		g_object_unref(propertywindow);

	window->propertywindow = propertywindow;
}

XAPropertyDialog *
xa_main_window_get_property_window (XAMainWindow *window)
{
	return XA_PROPERTY_DIALOG(window->propertywindow);
}
