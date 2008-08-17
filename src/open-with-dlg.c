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

/*gtk_tree_model_get(archive->liststore,&iter,0,pixbuf);
open_with_dialog = xa_create_open_with_dialog(filename);*/

GtkWidget *xa_create_open_with_dialog(gchar *filename)
{
	GtkWidget	*dialog1,*dialog_vbox1,*vbox1,*hbox1,*mime_icon,*open_text,*scrolledwindow1,*apps_treeview,*app_description,*dialog_action_area1,
				*custom_command_expander,*custom_command_entry,*hbox_expander,*browse,*cancelbutton1,*okbutton1;
	GtkCellRenderer		*renderer;
	GtkTreeViewColumn	*column;
	GdkPixbuf *pixbuf;
	gchar *text = NULL;

	dialog1 = gtk_dialog_new ();
	gtk_window_set_title (GTK_WINDOW (dialog1), _("Open With"));
	gtk_window_set_position (GTK_WINDOW (dialog1), GTK_WIN_POS_CENTER_ON_PARENT);
	gtk_window_set_modal (GTK_WINDOW (dialog1), TRUE);
	gtk_window_set_type_hint (GTK_WINDOW (dialog1), GDK_WINDOW_TYPE_HINT_DIALOG);
	gtk_window_set_transient_for(GTK_WINDOW(dialog1),GTK_WINDOW(xa_main_window));
	gtk_dialog_set_has_separator (GTK_DIALOG (dialog1),FALSE);
	gtk_container_set_border_width (GTK_CONTAINER (dialog1),5);
	gtk_widget_set_size_request(dialog1,355,377);
	dialog_vbox1 = GTK_DIALOG (dialog1)->vbox;

	vbox1 = gtk_vbox_new (FALSE, 5);
	gtk_box_pack_start (GTK_BOX (dialog_vbox1),vbox1,TRUE,TRUE,0);

	hbox1 = gtk_hbox_new (FALSE, 1);
	gtk_box_pack_start (GTK_BOX (vbox1),hbox1,FALSE,FALSE,0);

	pixbuf = xa_get_pixbuf_icon_from_cache(filename);
	mime_icon = gtk_image_new_from_pixbuf(pixbuf);
	gtk_box_pack_start (GTK_BOX (hbox1),mime_icon,FALSE,TRUE,0);
	gtk_misc_set_alignment (GTK_MISC (mime_icon),0,0);

	open_text = gtk_label_new("");
	text = g_strdup_printf(_("Open <i>%s</i> with:"),filename);
	gtk_label_set_use_markup (GTK_LABEL (open_text),TRUE);
	gtk_label_set_markup (GTK_LABEL (open_text),text);
	g_free(text);
	gtk_box_pack_start (GTK_BOX (hbox1),open_text,FALSE,FALSE,10);

	scrolledwindow1 = gtk_scrolled_window_new (NULL,NULL);
	gtk_box_pack_start (GTK_BOX (vbox1),scrolledwindow1,TRUE,TRUE,0);
	g_object_set (G_OBJECT (scrolledwindow1),"hscrollbar-policy",GTK_POLICY_AUTOMATIC,"shadow-type",GTK_SHADOW_IN,"vscrollbar-policy",GTK_POLICY_AUTOMATIC,NULL);

	apps_treeview = gtk_tree_view_new();
	gtk_container_add (GTK_CONTAINER (scrolledwindow1),apps_treeview);
	gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(apps_treeview),FALSE);

	/* First column: icon + text */
	column = gtk_tree_view_column_new();
	renderer = gtk_cell_renderer_pixbuf_new();
	gtk_tree_view_column_pack_start(column,renderer, FALSE);
	gtk_tree_view_column_set_attributes(column,renderer,"pixbuf",0,NULL);

	renderer = gtk_cell_renderer_text_new ();
	gtk_tree_view_column_pack_start(column,renderer, TRUE);
	gtk_tree_view_column_set_attributes( column,renderer,"text",1,NULL);
	gtk_tree_view_column_set_resizable (column, TRUE);
	gtk_tree_view_column_set_sort_column_id (column, 1);
	gtk_tree_view_append_column (GTK_TREE_VIEW (apps_treeview), column);
	gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);

	app_description = gtk_label_new ("This should be the app description");
	gtk_misc_set_alignment(GTK_MISC(app_description),0.05,0.0);
	gtk_box_pack_start (GTK_BOX (vbox1),app_description,FALSE,FALSE,0);

	custom_command_expander = gtk_expander_new_with_mnemonic(_("Use a custom command:"));
	gtk_box_pack_start (GTK_BOX (vbox1),custom_command_expander,FALSE,FALSE,0);

	hbox_expander = gtk_hbox_new(FALSE,5);

	custom_command_entry = gtk_entry_new();
	browse = gtk_button_new_with_label(_("Browse"));

	gtk_box_pack_start (GTK_BOX (hbox_expander),custom_command_entry,TRUE,TRUE,0);
	gtk_box_pack_start (GTK_BOX (hbox_expander),browse,TRUE,TRUE,0);
	gtk_container_add(GTK_CONTAINER(custom_command_expander),hbox_expander);

	dialog_action_area1 = GTK_DIALOG (dialog1)->action_area;
	gtk_button_box_set_layout (GTK_BUTTON_BOX (dialog_action_area1),GTK_BUTTONBOX_END);

	cancelbutton1 = gtk_button_new_from_stock ("gtk-cancel");
	gtk_widget_show (cancelbutton1);
	gtk_dialog_add_action_widget (GTK_DIALOG (dialog1),cancelbutton1,GTK_RESPONSE_CANCEL);
	GTK_WIDGET_SET_FLAGS (cancelbutton1, GTK_CAN_DEFAULT);

	okbutton1 = gtk_button_new_from_stock ("gtk-open");
	gtk_widget_show (okbutton1);
	gtk_dialog_add_action_widget (GTK_DIALOG (dialog1),okbutton1,GTK_RESPONSE_OK);
	GTK_WIDGET_SET_FLAGS (okbutton1, GTK_CAN_DEFAULT);
	gtk_widget_show_all(vbox1);
	return dialog1;
}
