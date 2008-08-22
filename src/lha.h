/*
 *  Copyright (C) 2008 Giuseppe Torelli - <colossus73@gmail.com>
 *  Copyright (C) 2006 Lukasz 'Sil2100' Zemczak - <sil2100@vexillium.org>
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

#ifndef LHA_H
#define LHA_H

#include <gtk/gtk.h>
#include <string.h>
#include <stdlib.h>
#include "support.h"
#include "string_utils.h"
#include "archive.h"

void xa_lha_delete (XArchive *,GSList *);
void xa_lha_add (XArchive *,GString *,gchar *);
gboolean xa_lha_extract(XArchive *,GSList *);
void xa_lha_test (XArchive *);
void xa_get_lha_line_content (gchar *line, gpointer data);
void xa_open_lha (XArchive *archive);
gboolean jump_header, last_line;
gboolean isLha (FILE *ptr);
#endif
