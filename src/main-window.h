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
 *  GNU Library General Public License for more details.  *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifndef __XARCHIVER_MAIN_WINDOW_H__
#define __XARCHIVER_MAIN_WINDOW_H__

G_BEGIN_DECLS

#define XA_MAIN_WINDOW(obj)             ( \
		G_TYPE_CHECK_INSTANCE_CAST ((obj),    \
			xa_main_window_get_type(),          \
			XAMainWindow))

#define XA_MAIN_WINDOW_CLASS(_class)    ( \
		G_TYPE_CHECK_CLASS_CAST ((_class),    \
			xa_main_window_get_type(),          \
			XAMainWindowClass))

#define IS_XA_MAIN_WINDOW(obj)          ( \
		G_TYPE_CHECK_INSTANCE_TYPE ((obj),    \
			xa_main_window_get_type()))

#define IS_XA_MAIN_WINDOW_CLASS(_class) ( \
		G_TYPE_CHECK_CLASS_TYPE ((_class),    \
			xa_main_window_get_type()))


typedef struct _XAMainWindow      XAMainWindow;
typedef struct _XAMainWindowClass XAMainWindowClass;

struct _XAMainWindow
{
	GtkWindow window;
	GtkWidget *vbox;
	GtkWidget *menubar;
	GtkWidget *toolbar;
	GtkWidget *scrollwindow;
	GtkWidget *contentlist;
	GtkWidget *statusbar;
	GtkWidget *progressbar;
	XAPropertyDialog *propertywindow;
};

struct _XAMainWindowClass
{
	GtkWindowClass parent_class;
};

GType              xa_main_window_get_type              (void);
GtkWidget*         xa_main_window_new                   (void);
void               xa_main_window_set_widget_sensitive  (XAMainWindow *window, gchar *name, gboolean sensitive);
void               xa_main_window_set_widget_visible    (XAMainWindow *window, gchar *name, gboolean visible);
void               xa_main_window_set_list_interface    (XAMainWindow *window, int nc, gchar *column_names[], GType column_types[]);
void               xa_main_window_append_list           (XAMainWindow *window, GList *fields);
void               xa_main_window_clear_list            (XAMainWindow *window);
void               xa_main_window_set_progressbar_value (XAMainWindow *window, gdouble value);
void               xa_main_window_set_statusbar_value   (XAMainWindow *window, gchar *value);
void               xa_main_window_set_property_window   (XAMainWindow *window, XAPropertyDialog *propertywindow);
XAPropertyDialog * xa_main_window_get_property_window   (XAMainWindow *window);

G_END_DECLS

#endif
