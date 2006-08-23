/*
 *  Copyright (C) 2006 Giuseppe Torelli - <colossus73@gmail.com>
 *  Copyright (C) 2006 Benedikt Meurer - <benny@xfce.org>
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
 
#include "config.h"
#include "gzip.h"
#include "bzip2.h"

FILE *fd;
extern int output_fd;

void OpenGzip ( XArchive *archive )
{
	if ( g_str_has_suffix ( archive->escaped_path , ".tar.gz") || g_str_has_suffix ( archive->escaped_path , ".tgz") )
	{
    gchar *command;
    gchar *tar;

    tar = g_find_program_in_path ("gtar");
    if (tar == NULL)
      tar = g_strdup ("tar");
    
		command = g_strconcat (tar, " tzvf " , archive->escaped_path, NULL );
		archive->dummy_size = 0;
		archive->nr_of_files = 0;
		archive->nr_of_dirs = 0;
		archive->format ="TAR.GZIP";
		archive->parse_output = TarOpen;

		SpawnAsyncProcess ( archive , command , 0, 0);

		g_free (command);
		g_free (tar);

		if ( archive->child_pid == 0 )
			return;

		char *names[]= {(_("Filename")),(_("Permissions")),(_("Symbolic Link")),(_("Owner/Group")),(_("Size")),(_("Date")),(_("Time"))};
		GType types[]= {G_TYPE_STRING,G_TYPE_STRING,G_TYPE_STRING,G_TYPE_STRING,G_TYPE_UINT64,G_TYPE_STRING,G_TYPE_STRING};
		xa_create_liststore ( 7, names , (GType *)types );
        archive->type = XARCHIVETYPE_TAR_GZ;
	}
	else
	{
		archive->format ="GZIP";
		gzip_bzip2_extract ( archive , 1 );
	}
}


