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

#ifndef RPM_H
#define RPM_H

#include <gtk/gtk.h>
#include "window.h"
#include "interface.h"
#include "archive.h"

void xa_open_rpm ( XArchive *);
void xa_open_temp_file (gchar *,gchar *);
gboolean xa_rpm_extract(XArchive *,GSList *);
void xa_get_cpio_line_content (gchar *, gpointer );
#endif
