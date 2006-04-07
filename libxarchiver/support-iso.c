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

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <glib.h>
#include "internals.h"
#include "libxarchiver.h"
#include "support-iso.h"

/*
 * xarchive_iso_support_add(XArchive *archive, GSList *files)
 * Add files and folders to the iso image
 *
 */


gboolean
xarchive_iso_support_add (XArchive *archive, GSList *files)
{
	return TRUE;
}

/*
 * xarchive_zip_support_extract(XArchive *archive, GSList *files)
 * Extract files and folders from archive
 * TODO: Salvo, put the code for extraction below; archive->path is the filename, destination_path is the path where to extract the file and files contains the selected names to extract
 * Salvo, don't mind to gboolean type it's just for compatibility with other archive types, ignore it.
 *
 */

gboolean
xarchive_iso_support_extract (XArchive *archive, gchar *destination_path, GSList *files ,gboolean type )
{
	return TRUE;
}


/*
 * xarchive_iso_support_remove(XArchive *archive, GSList *files)
 * Remove files and folders from the iso image
 * TODO: deletion is to be implemented
 */

/*
gboolean
xarchive_iso_support_remove (XArchive *archive, GSList *files )
{
	return TRUE;
}
*/

/*
 * xarchive_iso_support_open(XArchive *archive)
 * Gets the content of the files and folders inside an iso image
 * archive->path is the filename of the iso to open
 */

gboolean
xarchive_iso_support_open (XArchive *archive)
{
	
	return TRUE;
}

/*
 * xarchive_iso_support_open
 * Parse the output from the iso command when opening the archive
 *
 */

gboolean
xarchive_iso_support_verify(XArchive *archive)
{
	char buf[8];
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

XArchiveSupport *
xarchive_iso_support_new()
{
	XArchiveSupport *support = g_new0(XArchiveSupport, 1);
	support->type    = XARCHIVETYPE_ISO;
	support->add     = xarchive_iso_support_add;
	support->verify  = xarchive_iso_support_verify;
	support->extract = xarchive_iso_support_extract;
	//support->remove  = xarchive_iso_support_remove; // to delete the files inside the iso image
	support->open    = xarchive_iso_support_open;
	return support;
}

