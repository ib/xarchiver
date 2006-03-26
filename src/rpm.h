/*
 *  Xarchiver
 *
 *  Copyright (C) 2005 Giuseppe Torelli - Colossus
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
 
#ifndef RPM_H
#define RPM_H

 #include <gtk/gtk.h>
 #include "callbacks.h"
 #include "interface.h"
 #include "support.h"
 #include "main.h"

void OpenRPM ( gboolean mode , gchar *path );
GChildWatchFunc *DecompressCPIO (GPid pid , gint status , gpointer data);
GChildWatchFunc *OpenCPIO (GPid pid , gint status , gpointer data);

void SpawnCPIO ( gchar *command , gchar* tmp , gboolean add_flag , gboolean input_flag );
gboolean ReadCPIOOutput (GIOChannel *ioc, GIOCondition cond, gpointer data);
gboolean WriteCPIOInput (GIOChannel *ioc, GIOCondition cond, gpointer data);
void CloseChannels ( GIOChannel *ioc );

#endif
