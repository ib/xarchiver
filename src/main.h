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

#ifndef MAIN_H
#define MAIN_H

#include <gtk/gtk.h>
#include <glib.h>
#include "interface.h"
#include "new_dialog.h"
#include "archive.h"
#include "window.h"

GList *ArchiveSuffix;
GList *ArchiveType;
const gchar *locale;
gchar *config_file,*tar;
void xa_set_available_archivers();

XArchive *xa_init_structure_from_cmd_line (char *filename);
#endif /* MAIN_H */

