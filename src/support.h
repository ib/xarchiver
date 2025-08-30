/*
 *  Copyright (C) 2005 Giuseppe Torelli - <colossus73@gmail.com>
 *  Copyright (C) Ingo Brückl
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 *  MA 02110-1301 USA.
 */

#ifndef XARCHIVER_SUPPORT_H
#define XARCHIVER_SUPPORT_H

#include "config.h"
#include <glib.h>
#include <gtk/gtk.h>

/* Internationalization */

#ifdef ENABLE_NLS
#include <libintl.h>
#define gettext_noop(String) String
#define _(String) dgettext(GETTEXT_PACKAGE, String)
#define N_(String) gettext_noop(String)
#define Q_(String) g_strip_context((String), gettext(String))
#else
#define ngettext(Singular, Plural, Quantity) (Plural)
#define _(String) (String)
#define N_(String) (String)
#define Q_(String) g_strip_context((String), (String))
#endif

/* GLib compatibility */

#if !GLIB_CHECK_VERSION(2,16,0)
static inline int g_strcmp0 (const char *str1, const char *str2) { if (!str1) return -(str1 != str2); if (!str2) return (str1 != str2); return strcmp(str1, str2); }
#endif

#if !GLIB_CHECK_VERSION(2,25,0)
#define GStatBuf struct stat
#endif

#if !GLIB_CHECK_VERSION(2,28,0)
#define g_slist_free_full(list, free_func) do { g_slist_foreach(list, (GFunc) free_func, NULL); g_slist_free(list); } while (0)
#endif

#if !GLIB_CHECK_VERSION(2,30,0)
#include "string_utils.h"
#define g_mkdtemp(tmpl) mkdtemp(tmpl)
#endif

/* GTK+ compatibility */

#if !GTK_CHECK_VERSION(2,12,0)
#define GTK_COMPAT_TOOLTIPS GtkTooltips *tooltips = gtk_tooltips_new()
#define gtk_tool_item_set_tooltip_text(tool_item, text) gtk_tool_item_set_tooltip(tool_item, tooltips, text, NULL)
#define gtk_widget_set_tooltip_text(widget, text) gtk_tooltips_set_tip(tooltips, widget, text, NULL)
#else
#define GTK_COMPAT_TOOLTIPS (void) 0
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
#define gtk_widget_get_window(widget) widget->window
#endif

#if !GTK_CHECK_VERSION(2,16,0)
#include "sexy-icon-entry.h"
#define GTK_COMPAT_ENTRY_ICON_TYPE gint
#define GTK_COMPAT_ENTRY_ICON_NEW sexy_icon_entry_new
static inline void GTK_COMPAT_ENTRY_ICON (GtkWidget *entry, gpointer callback, gpointer user_data)
{
	sexy_icon_entry_add_clear_button(SEXY_ICON_ENTRY(entry), user_data, callback);
}
static inline void gtk_menu_item_set_label (GtkMenuItem *menu_item, const gchar *label)
{
	if (GTK_IS_LABEL(GTK_BIN(menu_item)->child))
		gtk_label_set_label(GTK_LABEL(GTK_BIN(menu_item)->child), label ? label : "");
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

#if !GTK_CHECK_VERSION(2,18,0)
#define gtk_widget_get_sensitive(widget) GTK_WIDGET_SENSITIVE(widget)
#define gtk_widget_get_visible(widget) GTK_WIDGET_VISIBLE(widget)
#define gtk_widget_is_sensitive(widget) GTK_WIDGET_IS_SENSITIVE(widget)
static inline void gtk_widget_set_can_default (GtkWidget *widget, gboolean can_default)
{
	if (can_default) GTK_WIDGET_SET_FLAGS(widget, GTK_CAN_DEFAULT);
	else GTK_WIDGET_UNSET_FLAGS(widget, GTK_CAN_DEFAULT);
}
static inline void gtk_widget_set_can_focus (GtkWidget *widget, gboolean can_focus)
{
	if (can_focus) GTK_WIDGET_SET_FLAGS(widget, GTK_CAN_FOCUS);
	else GTK_WIDGET_UNSET_FLAGS(widget, GTK_CAN_FOCUS);
}
#endif

#if !GTK_CHECK_VERSION(2,22,0)
#define GDK_KEY_a GDK_a
#define GDK_KEY_b GDK_b
#define GDK_KEY_d GDK_d
#define GDK_KEY_e GDK_e
#define GDK_KEY_f GDK_f
#define GDK_KEY_l GDK_l
#define GDK_KEY_m GDK_m
#define GDK_KEY_p GDK_p
#define GDK_KEY_r GDK_r
#define GDK_KEY_s GDK_s
#define GDK_KEY_t GDK_t
#define GDK_KEY_u GDK_u
#define GDK_KEY_x GDK_x
#define GDK_KEY_Delete GDK_Delete
#define GDK_KEY_F1 GDK_F1
#define GDK_KEY_Home GDK_Home
#define GDK_KEY_Left GDK_Left
#define GDK_KEY_Right GDK_Right
#define GDK_KEY_Up GDK_Up
#define GTK_COMPAT_SWITCH_PAGE_TYPE GtkNotebookPage *
#define gdk_drag_context_get_source_window(context) context->source_window
#define gdk_drag_context_get_suggested_action(context) context->suggested_action
static inline void gtk_icon_view_set_item_orientation (GtkIconView *icon_view, GtkOrientation orientation)
{
	gtk_icon_view_set_orientation(icon_view, orientation);
}
#else
#define GTK_COMPAT_SWITCH_PAGE_TYPE gpointer
#endif

#if !GTK_CHECK_VERSION(2,24,0)
#define GDK_COMPAT_SPAWN(p1, p2, p3, p4, p5, p6, p7, p8) gdk_spawn_on_screen(screen, p1, p2, p3, p4, NULL, NULL, p7, p8)
#define GTK_COMPAT_ABOUT_DIALOG_URI(about, func) \
	gtk_about_dialog_set_email_hook(func, NULL, NULL); \
	gtk_about_dialog_set_url_hook(func, NULL, NULL)
#define GtkComboBoxText GtkWidget
#define GTK_COMBO_BOX_TEXT
#define gtk_combo_box_text_new gtk_combo_box_new_text
static inline void gtk_combo_box_text_append_text (GtkWidget *combo_box, const gchar *text)
{
	gtk_combo_box_append_text(GTK_COMBO_BOX(combo_box), text);
}
static inline gchar *gtk_combo_box_text_get_active_text (GtkWidget *combo_box)
{
	return gtk_combo_box_get_active_text(GTK_COMBO_BOX(combo_box));
}
static inline void gtk_combo_box_text_insert_text (GtkWidget *combo_box, gint position, const gchar *text)
{
	gtk_combo_box_insert_text(GTK_COMBO_BOX(combo_box), position, text);
}
static inline void gtk_combo_box_text_prepend_text (GtkWidget *combo_box, const gchar *text)
{
	gtk_combo_box_prepend_text(GTK_COMBO_BOX(combo_box), text);
}
static inline void gtk_combo_box_text_remove (GtkWidget *combo_box, gint position)
{
	gtk_combo_box_remove_text(GTK_COMBO_BOX(combo_box), position);
}
#else
#define GDK_COMPAT_SPAWN(p1, p2, p3, p4, p5, p6, p7, p8) g_spawn_async(p1, p2, p3, p4, p5, p6, p7, p8)
#define GTK_COMPAT_ABOUT_DIALOG_URI(about, func) g_signal_connect(about, "activate-link", G_CALLBACK(func), NULL)
#endif

#if !GTK_CHECK_VERSION(2,24,32)
#include <locale.h>
#define GTK_COMPAT_PARAMETER_STRING setlocale(LC_ALL, "")
#else
#define GTK_COMPAT_PARAMETER_STRING (void) 0
#endif

#if GTK_CHECK_VERSION(3,0,0)
#define GTK_COMPAT_ADJUSTMENT_TYPE GtkAdjustment *
#define GTK_COMPAT_DESTROY_TYPE GtkWidget *
#undef GTK_COMPAT_SWITCH_PAGE_TYPE
#define GTK_COMPAT_SWITCH_PAGE_TYPE GtkWidget *
#else
#define GTK_COMPAT_ADJUSTMENT_TYPE GtkObject *
#define GTK_COMPAT_DESTROY_TYPE GtkObject *
#endif

#endif
