/*
 *  Copyright (C) 2008 Giuseppe Torelli <colossus73@gmail.com>
 *  Copyright (C) 2017 Ingo Brückl
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

#ifndef GZIP_ET_AL_H
#define GZIP_ET_AL_H

#include <glib.h>
#include "archive.h"

#define SINGLE_FILE_COMPRESSOR(archive) (archive->type == XARCHIVETYPE_BZIP     || \
                                         archive->type == XARCHIVETYPE_BZIP2    || \
                                         archive->type == XARCHIVETYPE_BZIP3    || \
                                         archive->type == XARCHIVETYPE_COMPRESS || \
                                         archive->type == XARCHIVETYPE_GZIP     || \
                                         archive->type == XARCHIVETYPE_LRZIP    || \
                                         archive->type == XARCHIVETYPE_LZ4      || \
                                         archive->type == XARCHIVETYPE_LZIP     || \
                                         archive->type == XARCHIVETYPE_LZMA     || \
                                         archive->type == XARCHIVETYPE_LZOP     || \
                                         archive->type == XARCHIVETYPE_RZIP     || \
                                         archive->type == XARCHIVETYPE_XZ       || \
                                         archive->type == XARCHIVETYPE_ZSTD)

#define LZ4_MAGIC "\x04\x22\x4d\x18"
#define MOZLZ4_MAGIC "mozLz40\x00"

void xa_gzip_et_al_add(XArchive *, GSList *);
void xa_gzip_et_al_ask(XArchive *);
void xa_gzip_et_al_check_lrzip(const gchar *);
gchar *xa_gzip_et_al_check_zstd(const gchar *, const gchar *, gboolean *);
gboolean xa_gzip_et_al_extract(XArchive *, GSList *);
gchar *xa_gzip_et_al_get_command(const gchar *, gchar *, gchar *, const gchar *, XArchiveType);
void xa_gzip_et_al_list(XArchive *);
void xa_gzip_et_al_test(XArchive *);

#endif
