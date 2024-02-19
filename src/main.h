/*
 *  Copyright (C) 2008 Giuseppe Torelli - <colossus73@gmail.com>
 *  Copyright (C) 2016 Ingo Brückl
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

#ifndef XARCHIVER_MAIN_H
#define XARCHIVER_MAIN_H

#include <glib.h>
#include "archive.h"
#include "gzip_et_al.h"

#define COPYRIGHT_YEAR "2005-2014"
#define COPYRIGHT_HOLDER "Giuseppe Torelli"
#define MAINTAINER "Ingo Brückl"
#define MAINTAINER_YEAR "2015-2024"

extern GtkWidget *xa_main_window;

extern XArchiver archiver[];
extern gchar *xdg_open;

extern struct AddDialog *add_window;
extern struct ExtractDialog *extract_window;
extern struct MultiExtractDialog *multi_extract_window;
extern struct PrefsDialog *prefs_window;

#endif
