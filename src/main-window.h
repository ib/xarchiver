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
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifndef __XARCHIVER_MAIN_WINDOW_H__
#define __XARCHIVER_MAIN_WINDOW_H__


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

G_BEGIN_DECLS

#define XARCHIVER_IS_MAIN_WINDOW(obj)       \
		GTK_CHECK_TYPE(obj,                        \
				xarchiver_main_window_get_type())

#define XARCHIVER_MAIN_WINDOW(obj)          \
    GTK_CHECK_CAST(obj,                        \
        xarchiver_main_window_get_type(),   \
        XArchiverMainWindow)

#define XARCHIVER_MAIN_WINDOW_CLASS(klass)  \
    GTK_CHECK_CLASS_CAST(obj,                  \
        xarchiver_main_window_get_type(),   \
        XArchiverMainWindowClass)

typedef struct _XArchiverMainWindow          XArchiverMainWindow;
typedef struct _XArchiverMainWindowClass     XArchiverMainWindowClass;

struct _XArchiverMainWindow
{
	GtkWindow parent;
};

struct _XArchiverMainWindowClass
{
	GtkWindowClass parent_class;
};

GtkType    xarchiver_main_window_get_type();

GtkWidget *xarchiver_main_window_new();

G_END_DECLS

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __XARCHIVER_MAIN_WINDOW_H__ */
