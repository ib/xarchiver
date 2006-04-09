/*
 *  Copyright (c) 2006 Stephan Arts      <stephan.arts@hva.nl>
 *                     Giuseppe Torelli <colossus73@gmail.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include <stdlib.h>
#include <stdio.h>
#include <glib.h>
#include <glib-object.h>
#include "libxarchiver.h"
#include "archive-tar.h"

gboolean
xarchive_type_tar_verify(XArchive *archive)
{
	FILE *fp;
	unsigned char magic[5];

	if( (archive->path) && (archive->type == XARCHIVETYPE_UNKNOWN))
	{
		fp = fopen(archive->path, "r");
		if(fp == 0)
			return FALSE;
		fseek ( fp, 0 , SEEK_SET );
		if ( fseek ( fp , 257, SEEK_CUR ) == 0 ) 
		{
			if ( fread ( magic, 1, 5, fp ) )
			{
				if ( memcmp ( magic,"ustar",5 ) == 0 )
				{
					archive->type = XARCHIVETYPE_TAR;
					archive->has_passwd = 0;
					archive->passwd = 0;
				}
			}
		}
		fclose( fp );
	}

	if(archive->type == XARCHIVETYPE_TAR)
		return TRUE;
	else
		return FALSE;
}
