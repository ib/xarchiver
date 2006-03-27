/*
 *  Copyright (c) 2006 Stephan Arts <stephan.arts@hva.nl>
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
#include <glib.h>
#include <zlib.h>
#include "internals.h"
#include "libxarchiver.h"

/*
 * xarchive_gzip_support_add(XArchive *archive, GSList *files)
 *
 * can only compress one file, 
 * (will return compressed file in XArchive->path)
 */
gboolean
xarchive_gzip_support_add(XArchive *archive, GSList *files)
{
	gzFile out_file;
	FILE *in_file;
	int n = 0;
	gchar buf[1024];
	if(!files)
		return FALSE;
	if(g_slist_length(files) < 1)
	{
		g_warning("Gzip: no file provided, cannot compress");
		return FALSE;
	}
	if(g_slist_length(files) > 1)
		g_warning("Gzip cannot compress multiple-files");
	if(!archive->path)
	{
		archive->path = g_strconcat((gchar *)files->data, ".gz", NULL);
	}
	in_file = fopen(files->data, "r");
	out_file = gzopen(archive->path, "w");
	while(n = fread(&buf, 1, 1024, in_file))
	{
		gzwrite(out_file, &buf, n);
	}
	gzclose(out_file);
	fclose(in_file);
	return TRUE;
}


/*
 * xarchive_gzip_support_extract(XArchive *archive, GSList *files)
 * Extract archive
 *
 * FIXME: create filename of destination_path/archive-basename
 */
gboolean
xarchive_gzip_support_extract(XArchive *archive, gchar *destination_path, GSList *files)
{
	gchar *in_filename;
	gzFile in_file;
	gchar *out_filename;
	FILE *out_file;
	int n = 0;
	gchar buf[1024];

	if(files)
		g_warning("Gzip cannot extract individual files, there is only one");

	if(!archive->path)
		return FALSE;

	if(destination_path)
	{
		if(g_file_test(destination_path, G_FILE_TEST_EXISTS) && g_file_test(destination_path, G_FILE_TEST_IS_DIR))
		{
			in_filename = g_path_get_basename(archive->path);
			out_filename = g_build_path(destination_path, in_filename, NULL);
			if(!g_str_has_suffix(out_filename, "gz"))
			{
				if(g_strcasecmp(archive->path, out_filename))
					return FALSE;
			} else
			{
				for(n = strlen(out_filename); (out_filename[n] != '.') && (n >= 0); n--);
				if(out_filename[n] == '.')
					out_filename[n] = '\0';
			}
		}
		else if(!g_file_test(destination_path, G_FILE_TEST_EXISTS)) 
		{
			// use it as an absolute filename.
			out_filename = g_strdup(destination_path);
		}
	}

	out_file = fopen(out_filename, "w");
	in_file = gzopen(archive->path, "r");
	while(n = gzread(in_file, &buf, 1024))
	{
		fwrite(&buf, 1, 1024, out_file);
	}
	gzclose(in_file);
	fclose(out_file);
	return TRUE;
}

gboolean
xarchive_gzip_support_verify(XArchive *archive)
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
			archive->passwd = 0;
		}
		fclose(fp);
	}
	if(archive->type == XARCHIVETYPE_GZIP)
		return TRUE;
	else
		return FALSE;
}

XArchiveSupport *
xarchive_gzip_support_new()
{
	XArchiveSupport *support = g_new0(XArchiveSupport, 1);
	support->type = XARCHIVETYPE_GZIP;
	support->verify = xarchive_gzip_support_verify;
	support->add = xarchive_gzip_support_add;
	support->extract = xarchive_gzip_support_extract;
	return support;
}
