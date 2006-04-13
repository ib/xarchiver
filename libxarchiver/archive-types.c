/*
 *  Copyright (c) 2006 Stephan Arts <psyBSD@gmail.com>
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
#include "archive.h"
#include "archive-types.h"

gboolean
xa_archive_type_7zip_verify(XAArchive *archive)
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

gboolean
xa_archive_type_gzip_verify(XAArchive *archive)
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

gboolean
xa_archive_type_tar_verify(XAArchive *archive)
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

gboolean
xa_archive_type_rar_verify(XAArchive *archive)
{
	FILE *fp;
	unsigned char magic[4];

	if( (archive->path) && (archive->type == XARCHIVETYPE_UNKNOWN))
	{
		fp = fopen(archive->path, "r");
		if(fp == 0)
			return FALSE;
		fseek ( fp, 0 , SEEK_SET );
		if ( fread ( magic, 1, 4, fp ) )
		{
			if ( memcmp ( magic,"\x52\x61\x72\x21",4 ) == 0 )
			{
				archive->type = XARCHIVETYPE_RAR;
			}
		}
		fclose( fp );
	}

	if(archive->type == XARCHIVETYPE_RAR)
		return TRUE;
	else
		return FALSE;
}

gboolean
xa_archive_type_arj_verify(XAArchive *archive)
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

gboolean
xa_archive_type_bzip2_verify(XAArchive *archive)
{
	FILE *fp;
	unsigned char magic[3];
	if((archive->path) && (archive->type == XARCHIVETYPE_UNKNOWN))
	{
		fp = fopen(archive->path, "r");
		if(fp == 0)
			return FALSE;
		if(fread( magic, 1, 3, fp) == 0)
		{
			fclose(fp);
			return FALSE;
		}
  	if ( memcmp ( magic,"\x42\x5a\x68",3 ) == 0 )
		{
			archive->type = XARCHIVETYPE_BZIP2;
			archive->has_passwd = FALSE;
			archive->passwd = 0;
		} 
		fclose(fp);
	}
	if(archive->type == XARCHIVETYPE_BZIP2)
		return TRUE;
	else
		return FALSE;
}

gboolean
xa_archive_type_iso_verify(XAArchive *archive)
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

gboolean
xa_archive_type_zip_verify(XAArchive *archive)
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
