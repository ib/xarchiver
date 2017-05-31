/*
 *  Copyright (C) 2017 Ingo Brückl
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

#ifndef CPIO_H
#define CPIO_H

#include <glib.h>
#include "archive.h"

void xa_cpio_add(XArchive *, GSList *, gchar *);
void xa_cpio_ask(XArchive *);
gboolean xa_cpio_extract(XArchive *, GSList *);
void xa_cpio_list(XArchive *);

#endif
