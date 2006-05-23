/*
 *  Copyright (C) 2006 Giuseppe Torelli - <colossus73@gmail.com>
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

GtkWidget *MainWindow;
GList *ArchiveSuffix;
GList *ArchiveType;
GOptionContext *context;

void GetAvailableCompressors();
void SetButtonState ( gboolean, gboolean,gboolean,gboolean,gboolean );
gboolean SpawnSyncCommand ( gchar *command );
#endif /* MAIN_H */

