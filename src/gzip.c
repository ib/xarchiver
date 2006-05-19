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
 
#include "gzip.h"
#include "bzip2.h"

FILE *fd;
extern int output_fd,error_fd;

void OpenGzip ( XArchive *archive )
{
	if ( g_str_has_suffix ( archive->escaped_path , ".tar.gz") || g_str_has_suffix ( archive->escaped_path , ".tgz") )
	{
        gchar *command = g_strconcat ("tar tfzv " , archive->escaped_path, NULL );
	   	archive->dummy_size = 0;
		archive->nr_of_files = 0;
		archive->nr_of_dirs = 0;
		archive->parse_output = TarOpen;
		SpawnAsyncProcess ( archive , command , 0);
		g_free ( command );
		if ( archive->child_pid == 0 )
			return;

		char *names[]= {(_("Filename")),(_("Permissions")),(_("Owner/Group")),(_("Size")),(_("Date")),(_("Time"))};
		GType types[]= {G_TYPE_STRING,G_TYPE_STRING,G_TYPE_STRING,G_TYPE_UINT64,G_TYPE_STRING,G_TYPE_STRING};
		xa_create_liststore ( 6, names , (GType *)types );
        archive->type = XARCHIVETYPE_TAR_GZ;
	}
	else 
	{
        bz_gz = TRUE;
        Bzip2Extract ( archive , 1 );
    }
}


