/*
 *  Copyright (c) 2006 Salvatore Santagati  <salvatore.santagati@gmail.com>
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
#include "archive-iso.h"

gboolean
xarchive_type_iso_verify(XArchive *archive)
{
	gchar buf[8];
	FILE *iso;
	
	if( (archive->path) && (archive->type == XARCHIVETYPE_UNKNOWN) )
	{
		iso = fopen ( archive->path , "r");
		if (iso == NULL) 
			return FALSE;
		fseek (iso, 0L, SEEK_SET);
		fseek (iso, 32768, SEEK_CUR);
		fread (buf, sizeof (char), 8, iso);
		if ( memcmp ("\x01\x43\x44\x30\x30\x31\x01\x00", buf, 8) == 0)
			archive->type = XARCHIVETYPE_ISO;

		fclose (iso);
	}
	if(archive->type == XARCHIVETYPE_ISO)
		return TRUE;
	else
		return FALSE;
}
