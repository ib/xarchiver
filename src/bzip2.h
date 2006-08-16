/*
 *  Copyright (C) 2006 Giuseppe Torelli <colossus73@gmail.com>
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
 
#ifndef BZIP2_H
#define BZIP2_H

#include <gtk/gtk.h>
#include "callbacks.h"
#include "interface.h"
#include "support.h"
#include "main.h"
#include "archive.h"

void OpenBzip2 ( XArchive *archive );
void Bzip2Extract ( XArchive *archive , gboolean flag );
gboolean ExtractToDifferentLocation (GIOChannel *ioc, GIOCondition cond , gpointer data);
void xa_add_delete_tar_bzip2_gzip ( GString *list , XArchive *archive , gboolean dummy , gboolean add );
GChildWatchFunc *AddToTar (GPid pid,gint status , gpointer data);
void Bzip2Add ( gchar *filename , XArchive *archive , gboolean flag );
gchar *OpenTempFile ( gboolean dummy , gchar *temp_path );
gboolean file_extension_is (const char *filename, const char *ext);
#endif
