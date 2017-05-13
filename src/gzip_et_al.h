/*
 *  Copyright (C) 2008 Giuseppe Torelli <colossus73@gmail.com>
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

#ifndef GZIP_ET_AL_H
#define GZIP_ET_AL_H

#include <glib.h>
#include "archive.h"

#define SINGLE_FILE_COMPRESSOR(archive) (archive->type == XARCHIVETYPE_BZIP2 || \
                                         archive->type == XARCHIVETYPE_GZIP  || \
                                         archive->type == XARCHIVETYPE_LZ4   || \
                                         archive->type == XARCHIVETYPE_LZIP  || \
                                         archive->type == XARCHIVETYPE_LZMA  || \
                                         archive->type == XARCHIVETYPE_LZOP  || \
                                         archive->type == XARCHIVETYPE_XZ)

void xa_gzip_et_al_add(XArchive *, GSList *, gchar *);
void xa_gzip_et_al_ask(XArchive *);
gboolean xa_gzip_et_al_extract(XArchive *, GSList *);
void xa_gzip_et_al_list(XArchive *);
void xa_gzip_et_al_test(XArchive *);

#endif
