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


#ifndef XARCHIVER_SOCKET_H
#define XARCHIVER_SOCKET_H

struct
{
	gboolean	 ignore_socket;
	gchar		*file_name;
	GIOChannel	*read_ioc;
	gint 		 lock_socket;
	gint 		 lock_socket_tag;
} socket_info;

gint socket_init (gint argc, gchar **argv);
gboolean socket_lock_input_cb(GIOChannel *source, GIOCondition condition, gpointer data);
gint socket_finalize();

#endif
