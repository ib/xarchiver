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

#include <gtk/gtk.h>
#include <stdlib.h>
#include <unistd.h>
#include "string_utils.h"
#include "support.h"
#include "archive.h"
#include "gzip.h"

void xa_open_tar ( XArchive * );
void xa_tar_delete (XArchive *,GSList *);
void xa_tar_add (XArchive *,GString *,gchar *);
gboolean xa_tar_extract(XArchive *,GSList *);
void xa_get_tar_line_content (gchar *, gpointer );
gboolean isTar (FILE *);
gboolean is_tar_compressed (gint );
void xa_add_delete_bzip2_gzip_lzma_compressed_tar (GString *,XArchive *,gboolean );
gboolean xa_extract_tar_without_directories ( gchar *, XArchive *,gchar *);
#endif

