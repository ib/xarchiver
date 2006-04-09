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
#include "archive-gzip.h"

gboolean
xarchive_type_gzip_verify(XArchive *archive)
{
	FILE *fp;
	unsigned char magic[3];
	if( (archive->path) && (archive->type == XARCHIVETYPE_UNKNOWN))
	{
		fp = fopen(archive->path, "r");
		if(fp == 0)
			return FALSE;
		if(fread( magic, 1, 3, fp) == 0)
		{
			fclose(fp);
			return FALSE;
		}
  	if ( memcmp ( magic,"\x1f\x8b\x08",3 ) == 0 )
		{
			archive->type = XARCHIVETYPE_GZIP;
			archive->has_passwd = FALSE;
			archive->passwd = 0;
		}
		fclose(fp);
	}
	if(archive->type == XARCHIVETYPE_GZIP)
		return TRUE;
	else
		return FALSE;
}
