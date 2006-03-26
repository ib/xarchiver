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
 
#include "gzip.h"
#include "bzip2.h"

FILE *fd;

extern int output_fd,error_fd,child_pid;

void OpenGzip ( gboolean mode , gchar *path)
{
	if ( g_str_has_suffix ( path , ".tar.gz") || g_str_has_suffix ( path , ".tgz") )
	{
        gchar *command = g_strconcat ("tar tfzv " , path, NULL );
		compressor_pid = SpawnAsyncProcess ( command , 1 , 0 );
		g_free ( command );
		if ( compressor_pid == 0 ) return;
		char *names[]= {(_("Filename")),(_("Permissions")),(_("Owner/Group")),(_("Size")),(_("Date")),(_("Time"))};
		GType types[]= {G_TYPE_STRING,G_TYPE_STRING,G_TYPE_STRING,G_TYPE_STRING,G_TYPE_STRING,G_TYPE_STRING};
		CreateListStore ( 6, names , (GType *)types );
        SetIOChannel (output_fd, G_IO_IN|G_IO_PRI|G_IO_ERR|G_IO_HUP|G_IO_NVAL,Bzip2Output, (gpointer) mode );
		SetIOChannel (error_fd, G_IO_IN|G_IO_PRI|G_IO_ERR|G_IO_HUP|G_IO_NVAL,GenError, NULL );
        CurrentArchiveType = 5;
	}
		
	else 
	{
		response = ShowGtkMessageDialog (GTK_WINDOW (MainWindow),GTK_DIALOG_MODAL,GTK_MESSAGE_QUESTION,GTK_BUTTONS_YES_NO,_("You selected a gunzip compressed file.\nDo you want to extract it now ?") );
		if (response == GTK_RESPONSE_YES)
        {
            bz_gz = TRUE;
            Bzip2Extract ( 1 );
        }
	}
}


