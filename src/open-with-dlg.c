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

#include <string.h>
#include <gtk/gtk.h>
#include "open-with-dlg.h"
#include "main.h"
#include "mime.h"
#include "pref_dialog.h"
#include "support.h"
#include "window.h"

typedef struct
{
	GtkWidget *dialog1;
	GtkWidget *custom_command_entry;
	gchar *files;
	GSList *icons;
	GSList *names;
	GSList *execs;
} Open_with_data;

static void xa_open_with_dialog_selection_changed (GtkTreeSelection *selection, Open_with_data *data)
{
	gchar *exec;
	GtkTreeIter iter;
	GtkTreeModel *model;

	if (gtk_tree_selection_get_selected(selection,&model,&iter))
	{
		gtk_tree_model_get(model,&iter,2,&exec,-1);
		gtk_entry_set_text(GTK_ENTRY(data->custom_command_entry),exec);
		g_free(exec);
	}
}

static void xa_open_with_dialog_execute_command (GtkButton *button, Open_with_data *data)
{
	const char *application;

	application = gtk_entry_get_text(GTK_ENTRY(data->custom_command_entry));
	xa_launch_external_program(application, data->files);
	gtk_widget_destroy(data->dialog1);
}

static void xa_open_with_dialog_row_selected (GtkTreeView *tree_view, GtkTreePath *path, GtkTreeViewColumn *column, Open_with_data *data)
{
	xa_open_with_dialog_execute_command(NULL, data);
}

static void xa_open_with_dialog_custom_entry_activated (GtkEntry *entry, Open_with_data *data)
{
	xa_open_with_dialog_execute_command(NULL, data);
}

static void xa_destroy_open_with_dialog (GtkObject *object, Open_with_data *data)
{
	g_free(data->files);
	g_slist_foreach(data->icons, (GFunc) g_object_unref, NULL);
	g_slist_free(data->icons);
	g_slist_foreach(data->names, (GFunc) g_free, NULL);
	g_slist_free(data->names);
	g_slist_foreach(data->execs, (GFunc) g_free, NULL);
	g_slist_free(data->execs);
	g_free(data);
}

static void xa_open_with_dialog_browse_custom_command (GtkButton *button, Open_with_data *data)
{
	GtkWidget *file_selector;
	gchar *dest_dir, *dest_dir_utf8;
	gint response;

	file_selector = gtk_file_chooser_dialog_new (_("Select an application"),
							GTK_WINDOW (xa_main_window),
							GTK_FILE_CHOOSER_ACTION_OPEN,
							GTK_STOCK_CANCEL,
							GTK_RESPONSE_CANCEL,
							GTK_STOCK_OPEN,
							GTK_RESPONSE_ACCEPT,
							NULL);

	gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER (file_selector),"/usr/bin");
	response = gtk_dialog_run (GTK_DIALOG(file_selector));
	if (response == GTK_RESPONSE_ACCEPT)
	{
		dest_dir = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER (file_selector));
		dest_dir_utf8 = g_filename_display_name(dest_dir);
		gtk_entry_set_text(GTK_ENTRY(data->custom_command_entry), dest_dir_utf8);
		g_free(dest_dir_utf8);
		g_free(dest_dir);
	}
	gtk_widget_destroy(file_selector);
}

static void xa_parse_desktop_file (const gchar *path, const gchar *name, Open_with_data *data)
{
	gchar *filename, *line, *key;
	gchar *app_name = NULL, *app_exec = NULL, *app_icon = NULL;
	GIOStatus status;
	GIOChannel *file;
	gboolean has_mimetype = FALSE;
	const gchar * const *langs, * const *l;
	gint size;

	filename = g_strconcat(path,"/",name,NULL);
	file = g_io_channel_new_file(filename,"r",NULL);
	g_free(filename);
	if (file == NULL)
		return;
	langs = g_get_language_names();
	g_io_channel_set_encoding(file,NULL,NULL);
	do
	{
		status = g_io_channel_read_line (file, &line, NULL, NULL, NULL);
		if (line != NULL)
		{
			if (g_str_has_prefix(line, "Name["))
			{
				l = langs;

				while (*l)
				{
					key = g_strconcat("Name[", *l, "]=", NULL);

					if (g_str_has_prefix(line, key))
					{
						g_free(app_name);
						app_name = g_strndup(line + strlen(key), strlen(line) - strlen(key) - 1);
						g_free(key);
						break;
					}

					g_free(key);
					l++;
				}
			}
			if (!app_name && g_str_has_prefix(line, "Name="))
			{
				app_name = g_strndup(line + 5,(strlen(line)-6));
				continue;
			}
			if (g_str_has_prefix(line,"Exec="))
			{
				app_exec = strstr(line, " %");
				if (app_exec)
					app_exec = g_strndup(line + 5, app_exec - (line + 5));
				else
					app_exec = g_strndup(line + 5, strlen(line) - 6);
				continue;
			}
			if (g_str_has_prefix(line,"Icon="))
			{
				app_icon = strrchr(line, '.');
				if (app_icon)
					app_icon = g_strndup(line + 5,app_icon - (line+5));
				else
					app_icon = g_strndup(line + 5,(strlen(line)-6));
				continue;
			}
			if (g_str_has_prefix(line,"MimeType="))
				has_mimetype = TRUE;
			g_free(line);
		}
	}
	while (status != G_IO_STATUS_EOF);

	if (has_mimetype)
	{
		data->names = g_slist_prepend(data->names, app_name);
		data->execs = g_slist_prepend(data->execs, app_exec);

		if (!app_icon)
			app_icon = g_strdup("");

		if (gtk_combo_box_get_active(GTK_COMBO_BOX(prefs_window->combo_icon_size)) == 0)
			size = 40;
		else
			size = 24;

		data->icons = g_slist_prepend(data->icons, gtk_icon_theme_load_icon(icon_theme, app_icon, size, GTK_ICON_LOOKUP_FORCE_SIZE, NULL));
	}
	else
	{
		g_free(app_name);
		g_free(app_exec);
	}

	g_free(app_icon);
	g_io_channel_shutdown(file, FALSE, NULL);
}

static void xa_read_desktop_directory (const gchar *dirname, GtkListStore *liststore, Open_with_data *data)
{
	DIR *dir;
	gchar *filename = NULL;
	struct dirent *dirlist;

	filename = g_build_filename(dirname,"applications",NULL);
	dir = opendir(filename);

	if (dir == NULL)
	{
		g_free(filename);
		return;
	}

	while ((dirlist = readdir(dir)))
	{
		if (g_str_has_suffix(dirlist->d_name,".desktop"))
			xa_parse_desktop_file(filename, dirlist->d_name, data);
	}

	closedir(dir);
	g_free(filename);
}

void xa_create_open_with_dialog (const gchar *filename, gchar *filenames, gint nr)
{
	Open_with_data *data = NULL;
	GtkListStore *apps_liststore;
	GtkWidget	*dialog_vbox1,*vbox1,*hbox1,*mime_icon,*open_text,*scrolledwindow1,*apps_treeview,*dialog_action_area1,
				*custom_command_expander,*hbox_expander,*browse,*cancelbutton1,*okbutton1;
	GtkCellRenderer		*renderer;
	GtkTreeViewColumn	*column;
	GtkTreeIter iter;
	GdkPixbuf *pixbuf;
	GSList *icon, *name, *exec;
	gchar *text = NULL;
	gchar *title;
	const gchar *icon_name = NULL;
	const gchar* const *desktop_dirs;
	gint x = 0;

	data = g_new0(Open_with_data,1);
	data->files = filenames;
	data->dialog1 = gtk_dialog_new ();
	if (nr == 1)
		title = _("Open With");
	else
		title = _("Open the selected files with");

	gtk_window_set_title (GTK_WINDOW (data->dialog1),title);
	gtk_window_set_position (GTK_WINDOW (data->dialog1), GTK_WIN_POS_CENTER_ON_PARENT);
	gtk_window_set_modal (GTK_WINDOW (data->dialog1), TRUE);
	gtk_window_set_type_hint (GTK_WINDOW (data->dialog1), GDK_WINDOW_TYPE_HINT_DIALOG);
	gtk_window_set_transient_for(GTK_WINDOW(data->dialog1),GTK_WINDOW(xa_main_window));
	gtk_container_set_border_width (GTK_CONTAINER (data->dialog1),5);
	gtk_widget_set_size_request(data->dialog1,380,380);
	dialog_vbox1 = gtk_dialog_get_content_area(GTK_DIALOG(data->dialog1));

	vbox1 = gtk_vbox_new (FALSE, 5);
	gtk_box_pack_start (GTK_BOX (dialog_vbox1),vbox1,TRUE,TRUE,0);

	hbox1 = gtk_hbox_new (FALSE, 1);
	gtk_box_pack_start (GTK_BOX (vbox1),hbox1,FALSE,FALSE,0);

	if (nr == 1)
	{
		icon_name = xa_get_stock_mime_icon(filename);
		pixbuf = gtk_icon_theme_load_icon(icon_theme,icon_name,40,0,NULL);
		mime_icon = gtk_image_new_from_pixbuf(pixbuf);
		if (pixbuf)
			g_object_unref(pixbuf);
		gtk_box_pack_start (GTK_BOX (hbox1),mime_icon,FALSE,TRUE,0);
		gtk_misc_set_alignment (GTK_MISC (mime_icon),0,0);

		open_text = gtk_label_new("");
		gtk_box_pack_start (GTK_BOX (hbox1),open_text,FALSE,FALSE,10);
		text = g_strdup_printf(_("Open <i>%s</i> with:"),filename);
		gtk_label_set_use_markup (GTK_LABEL (open_text),TRUE);
		gtk_label_set_markup (GTK_LABEL (open_text),text);
		g_free(text);
	}
	scrolledwindow1 = gtk_scrolled_window_new (NULL,NULL);
	gtk_box_pack_start (GTK_BOX (vbox1),scrolledwindow1,TRUE,TRUE,0);
	g_object_set (G_OBJECT (scrolledwindow1),"hscrollbar-policy",GTK_POLICY_AUTOMATIC,"shadow-type",GTK_SHADOW_IN,"vscrollbar-policy",GTK_POLICY_AUTOMATIC,NULL);

	apps_liststore = gtk_list_store_new(3, GDK_TYPE_PIXBUF, G_TYPE_STRING, G_TYPE_STRING);
	apps_treeview = gtk_tree_view_new_with_model(GTK_TREE_MODEL(apps_liststore));
	gtk_container_add (GTK_CONTAINER (scrolledwindow1),apps_treeview);
	gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(apps_treeview),FALSE);
	GtkTreeSelection *sel = gtk_tree_view_get_selection( GTK_TREE_VIEW (apps_treeview));
	g_signal_connect ((gpointer) sel,"changed",G_CALLBACK (xa_open_with_dialog_selection_changed),data);

	/* First column: icon + text */
	column = gtk_tree_view_column_new();
	renderer = gtk_cell_renderer_pixbuf_new();
	gtk_tree_view_column_pack_start(column,renderer, FALSE);
	gtk_tree_view_column_set_attributes(column,renderer,"pixbuf",0,NULL);

	renderer = gtk_cell_renderer_text_new ();
	gtk_tree_view_column_pack_start(column,renderer, TRUE);
	gtk_tree_view_column_set_attributes( column,renderer,"text",1,NULL);
	gtk_tree_view_column_set_resizable (column, TRUE);
	gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(apps_liststore), 1, GTK_SORT_ASCENDING);
	gtk_tree_view_append_column (GTK_TREE_VIEW (apps_treeview), column);
	gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);

	/* Hidden column with the application executable name */
	column = gtk_tree_view_column_new();
	gtk_tree_view_column_set_visible(column,FALSE);
	gtk_tree_view_append_column (GTK_TREE_VIEW (apps_treeview), column);

	custom_command_expander = gtk_expander_new_with_mnemonic(_("Use a custom command:"));
	gtk_box_pack_start (GTK_BOX (vbox1),custom_command_expander,FALSE,FALSE,0);

	hbox_expander = gtk_hbox_new(FALSE,5);

	data->custom_command_entry = gtk_entry_new();
	g_signal_connect (G_OBJECT (data->custom_command_entry),"activate",G_CALLBACK (xa_open_with_dialog_custom_entry_activated),data);

	browse = gtk_button_new_with_label(_("Browse"));
	g_signal_connect (G_OBJECT (browse),"clicked",G_CALLBACK (xa_open_with_dialog_browse_custom_command),data);

	gtk_box_pack_start (GTK_BOX (hbox_expander),data->custom_command_entry,TRUE,TRUE,0);
	gtk_box_pack_start (GTK_BOX (hbox_expander),browse,FALSE,TRUE,0);
	gtk_container_add(GTK_CONTAINER(custom_command_expander),hbox_expander);

	dialog_action_area1 = gtk_dialog_get_action_area(GTK_DIALOG(data->dialog1));
	gtk_button_box_set_layout (GTK_BUTTON_BOX (dialog_action_area1),GTK_BUTTONBOX_END);

	cancelbutton1 = gtk_button_new_from_stock ("gtk-cancel");
	gtk_widget_show (cancelbutton1);
	gtk_dialog_add_action_widget (GTK_DIALOG (data->dialog1),cancelbutton1,GTK_RESPONSE_CANCEL);
	gtk_widget_set_can_default(cancelbutton1, TRUE);
	g_signal_connect_swapped (G_OBJECT (cancelbutton1),"clicked",G_CALLBACK (gtk_widget_destroy),G_OBJECT(data->dialog1));

	okbutton1 = gtk_button_new_from_stock ("gtk-open");
	gtk_widget_show (okbutton1);
	gtk_dialog_add_action_widget (GTK_DIALOG (data->dialog1),okbutton1,GTK_RESPONSE_OK);
	g_signal_connect (G_OBJECT (okbutton1),"clicked",G_CALLBACK (xa_open_with_dialog_execute_command),data);
	gtk_widget_set_can_default(okbutton1, TRUE);
	gtk_widget_show_all(data->dialog1);

	/* let's parse the desktop files in all the system data dirs */
	desktop_dirs = g_get_system_data_dirs();

	while (desktop_dirs[x])
	{
		xa_read_desktop_directory(desktop_dirs[x], apps_liststore, data);
		x++;
	}

	icon = data->icons;
	name = data->names;
	exec = data->execs;

	while (exec)
	{
		gtk_list_store_append(apps_liststore, &iter);
		gtk_list_store_set(apps_liststore, &iter, 0, icon->data, 1,name->data, 2, exec->data, -1);

		icon = icon->next;
		name = name->next;
		exec = exec->next;
	}

	gtk_tree_model_get_iter_first(GTK_TREE_MODEL(apps_liststore), &iter);
	gtk_tree_selection_select_iter(gtk_tree_view_get_selection (GTK_TREE_VIEW (apps_treeview)),&iter);

	g_signal_connect (G_OBJECT (apps_treeview),	"row-activated",G_CALLBACK(xa_open_with_dialog_row_selected),data);
	g_signal_connect (G_OBJECT (data->dialog1),	"destroy",		G_CALLBACK(xa_destroy_open_with_dialog),data);
}
