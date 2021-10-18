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
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 *  MA 02110-1301 USA.
 */

#ifndef RAR_H
#define RAR_H

#include <glib.h>
#include "archive.h"

extern int rar_version;

void xa_rar_add(XArchive *, GSList *);
void xa_rar_ask(XArchive *);
void xa_rar_check_version(gchar *);
void xa_rar_delete(XArchive *, GSList *);
gboolean xa_rar_extract(XArchive *, GSList *);
void xa_rar_list(XArchive *);
void xa_rar_test(XArchive *);

#endif
