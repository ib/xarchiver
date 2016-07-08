/*
 *  Copyright (C) 2005 Giuseppe Torelli - <colossus73@gmail.com>
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

#ifndef __XARCHIVER_SUPPORT_H
#define __XARCHIVER_SUPPORT_H

#include "config.h"
#include <gtk/gtk.h>

/* Internationalization */

#ifdef ENABLE_NLS
#include <libintl.h>
#define gettext_noop(String) String
#define _(String) dgettext(PACKAGE, String)
#define N_(String) gettext_noop(String)
#define Q_(String) g_strip_context((String), gettext(String))
#else
#define ngettext(Singular, Plural, Quantity) (Plural)
#define _(String) (String)
#define N_(String) (String)
#define Q_(String) g_strip_context((String), (String))
#endif

/* GTK+ compatibility */

#if !GTK_CHECK_VERSION(2,14,0)
#define GTK_ICON_LOOKUP_FORCE_SIZE 0
#endif

#endif
