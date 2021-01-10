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
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 *  MA 02110-1301 USA.
 */

#ifndef XARCHIVER_STRING_UTILS_H
#define XARCHIVER_STRING_UTILS_H

#include "config.h"
#include <gtk/gtk.h>
#include "archive.h"

#define ESCAPES " \"#'\\"

#ifndef HAVE_MKDTEMP
gchar *mkdtemp(gchar *);
#else
#include <stdlib.h>
#endif

#ifndef HAVE_STRCASESTR
const char *strcasestr(const char *haystack, const char *needle);
#else
#include <string.h>
#endif

gboolean match_patterns(char **, const char *, int);
gchar *xa_escape_bad_chars(const gchar *, const gchar *);
GSList *xa_collect_filenames(XArchive *, GSList *);
GString *xa_collect_files_in_dir(const gchar *);
GString *xa_quote_filenames(GSList *, const gchar *, gboolean);
gchar *xa_quote_shell_command(const gchar *, gboolean);
void xa_recurse_local_directory(gchar *, GSList **, gboolean, gboolean);
gchar *xa_remove_level_from_path(const gchar *);
gchar *xa_set_max_width_chars_ellipsize(const gchar *, gint, PangoEllipsizeMode);
void xa_set_window_title(GtkWidget *, gchar *);
GSList *xa_slist_copy(GSList *);

#endif
