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

#ifndef __XARCHIVER_NEW_DIALOG_H__
#define __XARCHIVER_NEW_DIALOG_H__

G_BEGIN_DECLS

#define XA_NEW_DIALOG_TYPE             (xa_new_dialog_get_type())
#define XA_NEW_DIALOG(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
                                            XA_NEW_DIALOG_TYPE, \
                                            XANewDialog))
#define XA_NEW_DIALOG_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), \
                                            XA_NEW_DIALOG_TYPE, \
                                            XANewDialogClass))
#define XA_IS_NEW_DIALOG(obj)          (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
                                            XA_NEW_DIALOG_TYPE)
#define XA_IS_NEW_DIALOG_CLASS(klass)  (G_TYPE_CHECK_CLASS_CAST ((klass), \
                                            XA_NEW_DIALOG_TYPE)

typedef struct _XANewDialog          XANewDialog;
typedef struct _XANewDialogClass     XANewDialogClass;

struct _XANewDialog
{
	GtkDialog parent;
};

struct _XANewDialogClass
{
	GtkDialogClass parent;
};

GType      xa_new_dialog_get_type(void) G_GNUC_CONST;
GtkWidget *xa_new_dialog_new(GtkWindow *parent);

G_END_DECLS

#endif /* __XARCHIVER_NEW_DIALOG_H__ */
