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
#include "archive-zip.h"

gboolean
xarchive_type_zip_verify(XArchive *archive)
{
	FILE *fp;
	unsigned char magic[4];
	unsigned int fseek_offset;
	unsigned short int password_flag;
	unsigned int compressed_size;
	unsigned int uncompressed_size;
	unsigned short int file_length;
	unsigned short int extra_length;

	if( (archive->path) && (archive->type == XARCHIVETYPE_UNKNOWN))
	{
		fp = fopen(archive->path, "r");
		if(fp == 0)
			return FALSE;
		fseek ( fp, 0 , SEEK_SET );
		if ( fread ( magic, 1, 4, fp ) )
		{
			fseek ( fp, 2 , SEEK_CUR );
			if ( memcmp ( magic,"\x50\x4b\x03\x04",4 ) == 0 || memcmp ( magic,"\x50\x4b\x05\x06",4 ) == 0 )
			{
				archive->type = XARCHIVETYPE_ZIP;
				//Let's check for the password flag
				while ( memcmp ( magic,"\x50\x4b\x03\x04",4 ) == 0  || memcmp ( magic,"\x50\x4b\x05\x06",4 ) == 0 )
				{
					fread ( &password_flag, 1, 2, fp );
					if (( password_flag & ( 1<<0) ) > 0)
					{
						archive->has_passwd = TRUE;
						return TRUE;
					}
					else
						archive->has_passwd = FALSE;
					fseek (fp,10,SEEK_CUR);
					fread (&compressed_size,1,4,fp);
					fread (&uncompressed_size,1,4,fp);
					fread (&file_length,1,2,fp);
					//If the zip archive is empty (no files) it should return here
					if (fread (&extra_length,1,2,fp) < 2 )
					{
						archive->has_passwd = FALSE;
						return TRUE;
					}
					fseek_offset = compressed_size + file_length + extra_length;
					fseek (fp , fseek_offset , SEEK_CUR);
					fread (magic , 1 , 4 , fp);
					fseek ( fp , 2 , SEEK_CUR);
				}
			}
		}
		fclose( fp );
	}

	if(archive->type == XARCHIVETYPE_ZIP)
		return TRUE;
	else
		return FALSE;
}
