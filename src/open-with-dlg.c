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

#include "open-with-dlg.h"
#include "window.h"
#include "mime.h"
#include "support.h"

static void xa_read_desktop_directories(GtkListStore *,const gchar *);
static void xa_parse_desktop_files(GSList **,GSList **,GSList **,gchar *,gchar *);
static void xa_open_with_dialog_selection_changed (GtkTreeSelection *,Open_with_data *);
static void xa_open_with_dialog_browse_custom_command(GtkButton *,Open_with_data *);
static void xa_open_with_dialog_row_selected(GtkTreeView *,GtkTreePath *,GtkTreeViewColumn *,Open_with_data *);
static void xa_destroy_open_with_dialog(GtkWidget *,Open_with_data *);
static void xa_open_with_dialog_execute_command(GtkWidget *,Open_with_data *);
static void xa_open_with_dialog_custom_entry_activated(GtkEditable *,Open_with_data *);
		       
void xa_create_open_with_dialog(gchar *filename,gchar *filenames,int nr)
{
	Open_with_data *data = NULL;
	GtkWidget	*dialog_vbox1,*vbox1,*hbox1,*mime_icon,*open_text,*scrolledwindow1,*apps_treeview,*dialog_action_area1,
				*custom_command_expander,*hbox_expander,*browse,*cancelbutton1,*okbutton1;
	GtkCellRenderer		*renderer;
	GtkTreeViewColumn	*column;
	GtkTreeIter iter;
	GdkPixbuf *pixbuf;
	gchar *text = NULL;
	gchar *title;
	const gchar *icon_name = NULL;	
	const gchar* const *desktop_dirs;
	gint x = 0;

	data = g_new0(Open_with_data,1);
	data->file_list = filenames;
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
	gtk_dialog_set_has_separator (GTK_DIALOG (data->dialog1),FALSE);
	gtk_container_set_border_width (GTK_CONTAINER (data->dialog1),5);
	gtk_widget_set_size_request(data->dialog1,380,380);
	dialog_vbox1 = GTK_DIALOG (data->dialog1)->vbox;

	vbox1 = gtk_vbox_new (FALSE, 5);
	gtk_box_pack_start (GTK_BOX (dialog_vbox1),vbox1,TRUE,TRUE,0);

	hbox1 = gtk_hbox_new (FALSE, 1);
	gtk_box_pack_start (GTK_BOX (vbox1),hbox1,FALSE,FALSE,0);

	if (nr == 1)
	{
		icon_name = xa_get_stock_mime_icon(filename);
		pixbuf = gtk_icon_theme_load_icon(icon_theme,icon_name,40,0,NULL);
		mime_icon = gtk_image_new_from_pixbuf(pixbuf);
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

	data->apps_liststore = gtk_list_store_new (3,GDK_TYPE_PIXBUF,G_TYPE_STRING,G_TYPE_STRING);
	apps_treeview = gtk_tree_view_new_with_model(GTK_TREE_MODEL(data->apps_liststore));
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
	gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(data->apps_liststore),1,GTK_SORT_ASCENDING);
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

	dialog_action_area1 = GTK_DIALOG (data->dialog1)->action_area;
	gtk_button_box_set_layout (GTK_BUTTON_BOX (dialog_action_area1),GTK_BUTTONBOX_END);

	cancelbutton1 = gtk_button_new_from_stock ("gtk-cancel");
	gtk_widget_show (cancelbutton1);
	gtk_dialog_add_action_widget (GTK_DIALOG (data->dialog1),cancelbutton1,GTK_RESPONSE_CANCEL);
	GTK_WIDGET_SET_FLAGS (cancelbutton1, GTK_CAN_DEFAULT);
	g_signal_connect_swapped (G_OBJECT (cancelbutton1),"clicked",G_CALLBACK (gtk_widget_destroy),G_OBJECT(data->dialog1));

	okbutton1 = gtk_button_new_from_stock ("gtk-open");
	gtk_widget_show (okbutton1);
	gtk_dialog_add_action_widget (GTK_DIALOG (data->dialog1),okbutton1,GTK_RESPONSE_OK);
	g_signal_connect (G_OBJECT (okbutton1),"clicked",G_CALLBACK (xa_open_with_dialog_execute_command),data);
	GTK_WIDGET_SET_FLAGS (okbutton1, GTK_CAN_DEFAULT);
	gtk_widget_show_all(data->dialog1);

	/* Let's parse the desktop files in all the system data dirs */
	desktop_dirs = g_get_system_data_dirs();
	while (desktop_dirs[x])
	{
		xa_read_desktop_directories(data->apps_liststore,desktop_dirs[x]);
		x++;
	}
	gtk_tree_model_get_iter_first(GTK_TREE_MODEL(data->apps_liststore),&iter);
	gtk_tree_selection_select_iter(gtk_tree_view_get_selection (GTK_TREE_VIEW (apps_treeview)),&iter);

	g_signal_connect (G_OBJECT (apps_treeview),	"row-activated",G_CALLBACK(xa_open_with_dialog_row_selected),data);
	g_signal_connect (G_OBJECT (data->dialog1),	"destroy",		G_CALLBACK(xa_destroy_open_with_dialog),data);
}

static void xa_destroy_open_with_dialog(GtkWidget *widget,Open_with_data *data)
{
	g_free(data);
}
		       
static void xa_read_desktop_directories(GtkListStore *liststore,const gchar *dirname)
{
	DIR *dir;
	gchar *filename = NULL;
	struct dirent *dirlist;
	GSList *app_icon = NULL;
	GSList *app_name  = NULL;
	GSList *app_exe = NULL;
	GtkTreeIter iter;

	filename = g_build_path ("/",dirname,"applications",NULL);
	dir = opendir(filename);

	if (dir == NULL)
	{
		g_free(filename);
		return;
	}
	while ((dirlist = readdir(dir)))
	{
		if (g_str_has_suffix(dirlist->d_name,".desktop"))
			xa_parse_desktop_files(&app_name,&app_exe,&app_icon,filename,dirlist->d_name);
	}
	while (app_name)
	{
		gtk_list_store_append(liststore,&iter);
		gtk_list_store_set(liststore,&iter,0,app_icon->data,1,app_name->data,2,app_exe->data,-1);
		if (app_icon->data != NULL)
			g_object_unref(app_icon->data);
		app_name = app_name->next;
		app_icon = app_icon->next;
		app_exe  = app_exe->next;
	}
	g_free(filename);
	closedir(dir);

	g_slist_foreach(app_name,(GFunc)g_free,NULL);
	g_slist_foreach(app_exe,(GFunc)g_free,NULL);
	g_slist_free(app_name);
	g_slist_free(app_exe);
}

static void xa_parse_desktop_files(GSList **app_name_list,GSList **app_exe_list,GSList **app_icon_list,gchar *path,gchar *name)
{
	gchar *filename,*line;
	gchar *app_name = NULL, *app_exe = NULL, *app_icon = NULL;
	GIOStatus status;
	GIOChannel *file;
	gboolean has_mimetype = FALSE;

	filename = g_strconcat(path,"/",name,NULL);
	file = g_io_channel_new_file(filename,"r",NULL);
	g_free(filename);
	if (file == NULL)
		return;
	g_io_channel_set_encoding(file,NULL,NULL);
	do
	{
		status = g_io_channel_read_line (file, &line, NULL, NULL, NULL);
		if (line != NULL)
		{
			if (g_str_has_prefix(line,"Name="))
			{
				app_name = g_strndup(line + 5,(strlen(line)-6));
				continue;
			}
			if (g_str_has_prefix(line,"Exec="))
			{
				app_exe = strstr(line," ");
				if (app_exe)
					app_exe = g_strndup(line + 5,app_exe - (line+5));
				else
					app_exe = g_strndup(line + 5,(strlen(line)-6));
				continue;
			}
			if (g_str_has_prefix(line,"Icon="))
			{
				app_icon = strstr(line,".");
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
		*app_name_list	= g_slist_prepend(*app_name_list,app_name);
		*app_exe_list	= g_slist_prepend(*app_exe_list ,app_exe);
		if (app_icon == NULL)
			app_icon = "";
		*app_icon_list = g_slist_prepend(*app_icon_list,gtk_icon_theme_load_icon(icon_theme,app_icon,40,0,NULL));
		g_io_channel_close(file);	
		return;
	}
	if (app_name != NULL)
	{
		g_free(app_name);
		app_name = NULL;
	}
	if (app_exe != NULL)
	{
		g_free(app_exe);
		app_exe = NULL;
	}
	if (app_icon != NULL)
	{
		g_free(app_icon);
		app_icon = NULL;
	}
	g_io_channel_close(file);	
}

static void xa_open_with_dialog_selection_changed (GtkTreeSelection *selection,Open_with_data *data)
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

static void xa_open_with_dialog_custom_entry_activated(GtkEditable *entry,Open_with_data *data)
{
	xa_open_with_dialog_execute_command(NULL,data);
}

static void xa_open_with_dialog_browse_custom_command(GtkButton *button,Open_with_data *data)
{
	GtkWidget *file_selector;
	gchar *dest_dir;
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
		gtk_entry_set_text(GTK_ENTRY(data->custom_command_entry),dest_dir);
		g_free(dest_dir);
	}
	gtk_widget_destroy(file_selector);
}

static void xa_open_with_dialog_row_selected(GtkTreeView *tree_view,GtkTreePath *path,GtkTreeViewColumn *column,Open_with_data *data)
{
	xa_open_with_dialog_execute_command(NULL,data);
}

static void xa_open_with_dialog_execute_command(GtkWidget *widget, Open_with_data *data)
{
	const char *application;	

	application = gtk_entry_get_text(GTK_ENTRY(data->custom_command_entry));
	xa_launch_external_program((gchar*)application,data->file_list);
	gtk_widget_destroy(data->dialog1);
}
