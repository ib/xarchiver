/*
 *  Copyright (C) 2006 Giuseppe Torelli - <colossus73@gmail.com>
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

#include "config.h"
#include <glib.h>
#include <gtk/gtk.h>
#include "pref_dialog.h"
#include "interface.h"
#include "new_dialog.h"
#include "support.h"

Prefs_dialog_data *xa_create_prefs_dialog()
{
	Prefs_dialog_data *prefs_data;
	
	prefs_data = g_new0 (Prefs_dialog_data,1);
	prefs_data->dialog1 = gtk_dialog_new_with_buttons (_("Preferences Window"),
									GTK_WINDOW (MainWindow), GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
									GTK_STOCK_CANCEL,GTK_RESPONSE_CANCEL,GTK_STOCK_OK,GTK_RESPONSE_OK, NULL);
	prefs_data->prefs_notebook = GTK_NOTEBOOK(gtk_notebook_new() );
	gtk_box_pack_start (GTK_BOX(vbox_body), GTK_WIDGET(prefs_data->prefs_notebook),TRUE,TRUE,0);
	gtk_notebook_set_tab_pos (prefs_data->prefs_notebook, GTK_POS_TOP);
	//gtk_notebook_set_scrollable (prefs_data->prefs_notebook, TRUE);
	gtk_notebook_popup_disable (prefs_data->prefs_notebook);
	gtk_widget_show (GTK_WIDGET(prefs_data->prefs_notebook));
	//g_signal_connect ((gpointer) notebook, "switch-page", G_CALLBACK (xa_page_has_changed), NULL);
	gtk_dialog_set_default_response (GTK_DIALOG (prefs_data->dialog1), GTK_RESPONSE_OK);
	return prefs_data;
}
