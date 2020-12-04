/*
 *  Copyright (C) 2020 Ingo Brückl
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

#ifndef DATE_UTILS_H
#define DATE_UTILS_H

#include <glib.h>

gchar *date_DD_MM_YY(const gchar *);
gchar *date_MMM_dD_HourYear(const gchar *);
gchar *date_YY_MM_DD(const gchar *);
gchar *date_YY_MMM_DD(const gchar *);

#endif
