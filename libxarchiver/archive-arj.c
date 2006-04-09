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
#include <stdio.h>
#include <stdlib.h>
#include <glib.h>
#include <glib-object.h>
#include "libxarchiver.h"
#include "archive-arj.h"

gboolean
xarchive_type_arj_verify(XArchive *archive)
{
	FILE *fp;
	unsigned char magic[2];
	unsigned short int basic_header_size;
	unsigned short int extended_header_size;
	unsigned int basic_header_CRC;
	unsigned int extended_header_CRC;
	unsigned int compressed_size;
	unsigned char arj_flag;

	if( (archive->path) && (archive->type == XARCHIVETYPE_UNKNOWN))
	{
		fp = fopen(archive->path, "r");
		if(fp == 0)
			return FALSE;
		fseek ( fp, 0 , SEEK_SET );
		if ( fread ( magic, 1, 2, fp ) )
		{
			if ( memcmp ( magic,"\x60\xea",2 ) == 0 )
			{
				archive->type = XARCHIVETYPE_ARJ;
				archive->has_passwd = FALSE;
				//Let's check for the password flag
				fread (&magic,1,2,fp);
				fseek (fp , magic[0]+magic[1] , SEEK_CUR);
				fread (&extended_header_size,1,2,fp);
				if (extended_header_size != 0)
					fread (&extended_header_CRC,1,4,fp);
				fread (&magic,1,2,fp);
				while ( memcmp (magic,"\x60\xea",2) == 0)
				{
					fread ( &basic_header_size , 1 , 2 , fp );
					if ( basic_header_size == 0 ) 
						break;
					fseek ( fp , 4 , SEEK_CUR);
					fread (&arj_flag,1,1,fp);
					if ((arj_flag & ( 1<<0) ) > 0)
					{
						archive->has_passwd = TRUE;
						return TRUE;
					}
					fseek ( fp , 7 , SEEK_CUR);
					fread (&compressed_size,1,4,fp);
					fseek ( fp , basic_header_size - 16 , SEEK_CUR);
					fread (&basic_header_CRC,1,4,fp);
					fread (&extended_header_size,1,2,fp);
					if (extended_header_size != 0) 
						fread (&extended_header_CRC,1,4,fp);
					fseek ( fp , compressed_size , SEEK_CUR);
					fread (&magic,1,2,fp);
				}
			}
		}
		fclose( fp );
	}

	if(archive->type == XARCHIVETYPE_ARJ)
		return TRUE;
	else
		return FALSE;
}
