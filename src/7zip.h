/*
 *  Copyright (C) 2005 Giuseppe Torelli - <colossus73@gmail.com>
 *  Copyright (C) 2016 Ingo Brückl
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

#ifndef SEVENZIP_H
#define SEVENZIP_H

#include <glib.h>
#include "archive.h"

gboolean is_7zip_mhe(const gchar *);
void xa_7zip_add(XArchive *, GSList *);
void xa_7zip_ask(XArchive *);
void xa_7zip_delete(XArchive *, GSList *);
gboolean xa_7zip_extract(XArchive *, GSList *);
void xa_7zip_list(XArchive *);
void xa_7zip_test(XArchive *);

#endif
