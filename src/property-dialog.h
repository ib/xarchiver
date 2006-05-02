/*
 *  Copyright (c) 2006 Stephan Arts <psybsd@gmail.com>
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
 */

#ifndef __XARCHIVER_PROPERTY_DIALOG_H__
#define __XARCHIVER_PROPERTY_DIALOG_H__

G_BEGIN_DECLS

#define XA_PROPERTY_DIALOG(obj)             ( \
		G_TYPE_CHECK_INSTANCE_CAST ((obj),        \
			xa_property_dialog_get_type(),          \
			XAPropertyDialog))

#define XA_PROPERTY_DIALOG_CLASS(_class)    ( \
		G_TYPE_CHECK_CLASS_CAST ((_class),        \
			xa_property_dialog_get_type(),          \
			XAPropertyDialogClass))

#define IS_XA_PROPERTY_DIALOG(obj)   ( \
		G_TYPE_CHECK_INSTANCE_TYPE ((obj),        \
			xa_property_dialog_get_type()))

#define IS_XA_PROPERTY_DIALOG_CLASS(_class) ( \
		G_TYPE_CHECK_CLASS_TYPE ((_class),        \
			xa_property_dialog_get_type()))


typedef struct _XAPropertyDialog      XAPropertyDialog;
typedef struct _XAPropertyDialogClass XAPropertyDialogClass;

struct _XAPropertyDialog
{
	GtkDialog dialog;
	GtkWidget *propertylist;
	GtkWidget *table;
	GtkWidget *path_label;
	GtkWidget *modified_label;
	GtkWidget *size_label;
	GtkWidget *content_label;
	GtkWidget *compression_label;
	GtkWidget *number_of_files_label;
	GtkWidget *number_of_dirs_label;
	GtkWidget *name_label;

	GtkWidget *compression_data;
	GtkWidget *number_of_files_data;
	GtkWidget *number_of_dirs_data;
	GtkWidget *content_data;
	GtkWidget *size_data;
	GtkWidget *modified_data;
	GtkWidget *path_data;
	GtkWidget *name_data;
};

struct _XAPropertyDialogClass
{
	GtkDialogClass parent_class;
};

GType          xa_property_dialog_get_type              (void);
GtkWidget*     xa_property_dialog_new                   (GtkWindow *parent);
//void           xa_property_dialog_add_property          (XAPropertyDialog *dialog, gchar *name, gchar *value);
void xa_property_set_label (GtkWidget *label , gchar *text);

G_END_DECLS

#endif
