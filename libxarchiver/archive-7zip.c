/*
 *  Copyright (c) 2006 Stephan Arts      <stephan.arts@hva.nl>
 *                     Giuseppe Torelli  <colossus73@gmail.com>
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
#include "archive-7zip.h"

gboolean
xarchive_type_7zip_verify(XArchive *archive)
{
	FILE *fp;
	unsigned char magic[6];

	if( (archive->path) && (archive->type == XARCHIVETYPE_UNKNOWN))
	{
		fp = fopen(archive->path, "r");
		if(fp == 0)
			return FALSE;
		fseek ( fp, 0 , SEEK_SET );
		if ( fread ( magic, 1, 6, fp ) )
		{
			if ( memcmp ( magic,"\x37\x7a\xbc\xaf\x27\x1c",6 ) == 0 )
			{
				archive->type = XARCHIVETYPE_7ZIP;
				//TODO: no password detection for 7zip
				//http://sourceforge.net/forum/forum.php?thread_id=1378003&forum_id=383044
			}
		}
		fclose( fp );
	}

	if(archive->type == XARCHIVETYPE_7ZIP)
		return TRUE;
	else
		return FALSE;
}
