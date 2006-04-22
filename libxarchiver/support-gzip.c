/*
 *  Copyright (c) 2006 Stephan Arts <psybsd@gmail.com>
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
#include <glib.h>
#include <glib-object.h>
#include <zlib.h>
#include <libintl.h>
#include "archive.h"
#include "archive-types.h"
#include "support.h"
#include "support-gzip.h"

#define _(String) gettext(String)

void
xa_support_gzip_init (XASupportGzip *support);

gint
xarchive_support_gzip_add (XASupport *support, XAArchive *archive, GSList *files);

gint
xarchive_support_gzip_extract (XASupport *support, XAArchive *archive, gchar *destination_path, GSList *files, gboolean full_path);

GType
xa_support_gzip_get_type ()
{
	static GType xa_support_gzip_type = 0;

 	if (!xa_support_gzip_type)
	{
 		static const GTypeInfo xa_support_gzip_info = 
		{
			sizeof (XASupportGzipClass),
			(GBaseInitFunc) NULL,
			(GBaseFinalizeFunc) NULL,
			(GClassInitFunc) NULL,
			(GClassFinalizeFunc) NULL,
			NULL,
			sizeof (XASupportGzip),
			0,
			(GInstanceInitFunc) xa_support_gzip_init,
			NULL
		};

		xa_support_gzip_type = g_type_register_static (XA_TYPE_SUPPORT, "XASupportGzip", &xa_support_gzip_info, 0);
	}
	return xa_support_gzip_type;
}

void
xa_support_gzip_init (XASupportGzip *support)
{
	XASupport *xa_support = XA_SUPPORT(support);

	xa_support->type      = XARCHIVETYPE_GZIP;
	xa_support->verify    = xa_archive_type_gzip_verify;
	xa_support->add       = xarchive_support_gzip_add;
	xa_support->extract   = xarchive_support_gzip_extract;
}

/*
 * gint
 * xarchive_support_gzip_add (XArchive *archive, GSList *files)
 *
 * can only compress one file, 
 * (will return compressed file in XArchive->path)
 */
gint
xarchive_support_gzip_add (XASupport *support, XAArchive *archive, GSList *files)
{
	gzFile out_file;
	FILE *in_file;
	int n = 0;
	gchar buf[1024];
	if(!files)
		return 1;
	if(g_slist_length(files) < 1)
	{
		g_warning(_("XASupportGzip: no file provided, cannot compress"));
		return 1;
	}
	if(g_slist_length(files) > 1)
		g_warning(_("XASupportGzip: cannot compress multiple-files to single archive"));
	if(!archive->path)
	{
		archive->path = g_strconcat((gchar *)files->data, ".gz", NULL);
	}
	in_file = fopen(files->data, "r");
	out_file = gzopen(archive->path, "w");
	while((n = fread(&buf, 1, 1024, in_file)) > 0)
	{
		gzwrite(out_file, &buf, n);
	}
	gzclose(out_file);
	fclose(in_file);
	return 0;
}

/*
 * gint
 * xarchive_support_gzip_extract (XArchive *archive, GSList *files)
 * Extract archive
 *
 */
gint
xarchive_support_gzip_extract (XASupport *support, XAArchive *archive, gchar *destination_path, GSList *files, gboolean full_path)
{
	gchar *in_filename;
	gzFile in_file;
	gchar *out_filename;
	gchar *tmp_filename;
	FILE *out_file;
	int n = 0;
	gchar buf[1024];
	if(files)
		g_warning(_("XASupportGzip: cannot extract individual files, there is only one"));
	if(!archive->path)
		return 1;
	if(destination_path)
	{
		if(g_file_test(destination_path, G_FILE_TEST_IS_DIR))
		{
			in_filename = g_path_get_basename(archive->path);
			out_filename = g_build_path("/",destination_path, in_filename, NULL);
		}
		else /* use it as an absolute filename. */
			out_filename = g_strdup(destination_path);
	}
	else
	{
		out_filename = g_strdup(archive->path);
	}
	if(!g_str_has_suffix(out_filename, "gz"))
	{
		if(!g_strcasecmp(archive->path, out_filename))
			return 1;
	} 
	else
	{
		if(g_str_has_suffix(out_filename, ".gz"))
		{
			for(n = strlen(out_filename)-1; (out_filename[n] != '.') && (n >= 0); n--)
				if(out_filename[n] == '.')
					out_filename[n] = '\0';
		}
		else
		{
			if(g_str_has_suffix(out_filename, ".tgz"))
			{
				n = strlen(out_filename);
				out_filename[n-1] = 'r';
				out_filename[n-2] = 'a';
			}
			else
			{
				tmp_filename = out_filename;
				out_filename = g_strconcat(tmp_filename, ".out", NULL);
				g_free(tmp_filename);
			}
		}
	}
	out_file = fopen(out_filename, "w");
	in_file = gzopen(archive->path, "r");
	while((n = gzread(in_file, &buf, 1024)) > 0)
	{
		fwrite(&buf, 1, n, out_file);
	}
	gzclose(in_file);
	fclose(out_file);
	return 0;
}

XASupport*
xa_support_gzip_new ()
{
	XASupport *support;

	support = g_object_new(XA_TYPE_SUPPORT_GZIP, NULL);
	
	return support;
}
