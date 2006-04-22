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

#ifndef __XARCHIVER_ARCHIVE_CHOOSER_DIALOG_H__
#define __XARCHIVER_ARCHIVE_CHOOSER_DIALOG_H__

G_BEGIN_DECLS

#define XA_ARCHIVE_CHOOSER_DIALOG_TYPE             (xa_archive_chooser_dialog_get_type())
#define XA_ARCHIVE_CHOOSER_DIALOG(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
                                                   XA_ARCHIVE_CHOOSER_DIALOG_TYPE, \
                                                   XAArchiveChooserDialog))
#define XA_ARCHIVE_CHOOSER_DIALOG_CLASS(_class)    (G_TYPE_CHECK_CLASS_CAST ((klass), \
                                                   XA_ARCHIVE_CHOOSER_DIALOG_TYPE, \
                                                   XAArchiveChooserDialogClass))
#define XA_IS_ARCHIVE_CHOOSER_DIALOG(obj)          (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
                                                   XA_ARCHIVE_CHOOSER_DIALOG_TYPE)
#define XA_IS_ARCHIVE_CHOOSER_DIALOG_CLASS(_class) (G_TYPE_CHECK_CLASS_CAST ((klass), \
                                                   XA_ARCHIVE_CHOOSER_DIALOG_TYPE)

typedef struct _XAArchiveChooserDialog      XAArchiveChooserDialog;
typedef struct _XAArchiveChooserDialogClass XAArchiveChooserDialogClass;

struct _XAArchiveChooserDialog
{
	GtkDialog parent;
	gchar *filename;
};

struct _XAArchiveChooserDialogClass
{
	GtkDialogClass parent;
};

GType      xa_archive_chooser_dialog_get_type(void) G_GNUC_CONST;
GtkWidget *xa_archive_chooser_dialog_new(gchar *title, GtkWindow *parent);
gchar     *xa_archive_chooser_dialog_get_filename(XAArchiveChooserDialog *dialog);


G_END_DECLS

#endif /* __XARCHIVER_ARCHIVE_CHOOSER_DIALOG_H__ */
