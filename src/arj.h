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

#ifndef ARJ_H
#define ARJ_H

#include <gtk/gtk.h>
#include <string.h>
#include <stdlib.h>
#include "string_utils.h"
#include "support.h"
#include "archive.h"
gboolean jump_header, encrypted, last_line;
unsigned short int arj_line;
void xa_arj_delete (XArchive *,GString *);
void xa_arj_add (XArchive *,GString *,gchar *);
void xa_arj_extract(XArchive *,GString *,gchar *extraction_path);
void xa_arj_test (XArchive *);
void xa_get_arj_line_content (gchar *line, gpointer data);
void xa_open_arj (XArchive *archive);
#endif
