/*
 *  Copyright (C) 2005 Giuseppe Torelli - <colossus73@gmail.com>
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

#ifndef TAR_H
#define TAR_H

#include <glib.h>
#include <stdio.h>
#include "archive.h"

gboolean isTar(FILE *);
gboolean is_tar_compressed(gint);
void xa_tar_add(XArchive *, GSList *, gchar *);
void xa_tar_ask(XArchive *);
void xa_tar_delete(XArchive *, GSList *);
gboolean xa_tar_extract(XArchive *, GSList *);
void xa_tar_open(XArchive *);
void xa_tar_parse_output(gchar *, XArchive *);
void xa_tar_test(XArchive *);

#endif
