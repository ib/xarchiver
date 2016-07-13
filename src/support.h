/*
 *  Copyright (C) 2005 Giuseppe Torelli - <colossus73@gmail.com>
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

#ifndef __XARCHIVER_SUPPORT_H
#define __XARCHIVER_SUPPORT_H

#include "config.h"
#include <gtk/gtk.h>

/* Internationalization */

#ifdef ENABLE_NLS
#include <libintl.h>
#define gettext_noop(String) String
#define _(String) dgettext(PACKAGE, String)
#define N_(String) gettext_noop(String)
#define Q_(String) g_strip_context((String), gettext(String))
#else
#define ngettext(Singular, Plural, Quantity) (Plural)
#define _(String) (String)
#define N_(String) (String)
#define Q_(String) g_strip_context((String), (String))
#endif

/* GTK+ compatibility */

#if !GTK_CHECK_VERSION(2,12,0)
#define GTK_COMPAT_TOOLTIPS GtkTooltips *tooltips = gtk_tooltips_new();
#define gtk_tool_item_set_tooltip_text(tool_item, text) gtk_tool_item_set_tooltip(tool_item, tooltips, text, NULL)
#define gtk_widget_set_tooltip_text(widget, text) gtk_tooltips_set_tip(tooltips, widget, text, NULL)
#else
#define GTK_COMPAT_TOOLTIPS
#endif

#if !GTK_CHECK_VERSION(2,14,0)
#define GTK_ICON_LOOKUP_FORCE_SIZE 0
static inline GtkWidget *gtk_dialog_get_action_area (GtkDialog *dialog)
{
	return dialog->action_area;
}
static inline GtkWidget *gtk_dialog_get_content_area (GtkDialog *dialog)
{
	return dialog->vbox;
}
#define gtk_selection_data_get_data(selection_data) selection_data->data
#define gtk_selection_data_get_target(selection_data) selection_data->target
#endif

#if !GTK_CHECK_VERSION(2,16,0)
#include "sexy-icon-entry.h"
#define GTK_COMPAT_ENTRY_ICON_TYPE gint
#define GTK_COMPAT_ENTRY_ICON_NEW sexy_icon_entry_new
static inline void GTK_COMPAT_ENTRY_ICON (GtkWidget *entry, gpointer callback, gpointer user_data)
{
	sexy_icon_entry_add_clear_button(SEXY_ICON_ENTRY(entry), user_data, callback);
}
#else
#define GTK_COMPAT_ENTRY_ICON_TYPE GdkEvent *
#define GTK_COMPAT_ENTRY_ICON_NEW gtk_entry_new
static inline void GTK_COMPAT_ENTRY_ICON (GtkWidget *entry, gpointer callback, gpointer user_data)
{
	gtk_entry_set_icon_from_stock(GTK_ENTRY(entry), GTK_ENTRY_ICON_SECONDARY, GTK_STOCK_OPEN);
	g_signal_connect(entry, "icon-release", G_CALLBACK(callback), user_data);
}
#endif

#endif
