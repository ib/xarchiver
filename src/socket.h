/*
 *  Copyright (C) 2006 Giuseppe Torelli - <colossus73@gmail.com>
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


#ifndef XARCHIVER_SOCKET_H
#define XARCHIVER_SOCKET_H

#include <glib.h>

typedef struct
{
	gboolean	 ignore_socket;
	gchar		*file_name;
	GIOChannel	*read_ioc;
	gint 		 lock_socket;
	gint 		 lock_socket_tag;
} socket_info_t;

extern socket_info_t socket_info;

gint socket_finalize();
gint socket_init(gint, gchar **);
gboolean socket_lock_input_cb(GIOChannel *, GIOCondition, gpointer);

#endif
