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

#ifndef XARCHIVER_MAIN_H
#define XARCHIVER_MAIN_H

#include <glib.h>
#include "archive.h"

extern GList *ArchiveSuffix;
extern GList *ArchiveType;
extern const gchar *locale;
extern const gchar *sevenz;
extern const gchar *tar;

extern add_func add[];
extern ask_func ask[];
extern delete_func delete[];
extern extract_func extract[];
extern open_func open[];
extern test_func test[];

extern struct Add_dialog_data *add_window;
extern struct Extract_dialog_data *extract_window;
extern struct Prefs_dialog_data *prefs_window;

#endif
