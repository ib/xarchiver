/*
 *  Copyright (c) 2006 Stephan Arts <stephan.arts@hva.nl>
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
 *
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
#include "support-bzip2.h"

#define _(String) gettext(String)


void
xa_support_bzip2_init (XASupportBzip2 *support);

gint
xa_support_bzip2_add (XASupport *support, XAArchive *archive, GSList *files);

gint
xa_support_bzip2_extract (XASupport *support, XAArchive *archive, gchar *destination_path, GSList *files, gboolean full_path);

gboolean 
xa_support_bzip2_parse_output (GIOChannel *ioc, GIOCondition cond, gpointer data);

GType
xa_support_bzip2_get_type ()
{
	static GType xa_support_bzip2_type = 0;

 	if (!xa_support_bzip2_type)
	{
 		static const GTypeInfo xa_support_bzip2_info = 
		{
			sizeof (XASupportBzip2Class),
			(GBaseInitFunc) NULL,
			(GBaseFinalizeFunc) NULL,
			(GClassInitFunc) NULL,
			(GClassFinalizeFunc) NULL,
			NULL,
			sizeof (XASupportBzip2),
			0,
			(GInstanceInitFunc) xa_support_bzip2_init,
			NULL
		};

		xa_support_bzip2_type = g_type_register_static (XA_TYPE_SUPPORT, "XASupportBzip2", &xa_support_bzip2_info, 0);
	}
	return xa_support_bzip2_type;
}

void
xa_support_bzip2_init (XASupportBzip2 *support)
{
	XASupport *xa_support = XA_SUPPORT(support);

	xa_support->type      = XARCHIVETYPE_BZIP2;
	xa_support->verify    = xa_archive_type_bzip2_verify;
	xa_support->extract   = xa_support_bzip2_extract;
}

gint
xa_support_bzip2_add(XASupport *support, XAArchive *archive, GSList *files)
{
	return -1;
}

gint
xa_support_bzip2_extract(XASupport *support, XAArchive *archive, gchar *destination_path, GSList *files, gboolean full_path)
{
	gchar *in_filename;
	gchar *out_filename;
	gchar *tmp_filename;
	gchar *command;
	gint n;

	if(!g_file_test(archive->path, G_FILE_TEST_EXISTS))
		return 1;

	if( (files != NULL) && (g_slist_length(files) != 0))
	{
		g_warning("bzip2 can only extract one file");
	}
	support->exec.command = g_strconcat("bzip2 -kdc ", archive->path, NULL);
	support->exec.archive = archive;
	support->exec.parse_output = support->parse_output;

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
	if(!g_str_has_suffix(out_filename, "bz") && !g_str_has_suffix(out_filename, "bz2"))
	{
		if(!g_strcasecmp(archive->path, out_filename))
			return 1;
	} 
	else
	{
		if(g_str_has_suffix(out_filename, ".bz"))
		{
			for(n = strlen(out_filename)-1; (out_filename[n] != '.') && (n >= 0); n--)
				if(out_filename[n] == '.')
					out_filename[n] = '\0';
		}
		else
		{
			if(g_str_has_suffix(out_filename, ".bz2"))
			{
			for(n = strlen(out_filename)-1; (out_filename[n] != '.') && (n >= 0); n--)
				if(out_filename[n] == '.')
					out_filename[n] = '\0';
			}
			else
			{
				if(g_str_has_suffix(out_filename, ".tbz"))
				{
					n = strlen(out_filename);
					out_filename[n-1] = 'r';
					out_filename[n-2] = 'a';
				}
				else
				{
					if(g_str_has_suffix(out_filename, ".tbz2"))
					{
						n = strlen(out_filename);
						out_filename[n-1] = '\0';
						out_filename[n-2] = 'r';
						out_filename[n-3] = 'a';
					}
					else
					{
						tmp_filename = out_filename;
						out_filename = g_strconcat(tmp_filename, ".out", NULL);
						g_free(tmp_filename);
					}
				}
			}
		}
	}
	support->exec.signal = -1;
	
	xa_support_execute(support);
	return 0;
}


gboolean 
xa_support_bzip2_parse_output (GIOChannel *ioc, GIOCondition cond, gpointer data)
{
	XASupportBzip2 *support = XA_SUPPORT_BZIP2(data);
	FILE *out_file;
	gchar *buf = g_new0(gchar, 1024);
	guint read = 1024;

	out_file = fopen(support->out_filename, "w");
	if (cond & (G_IO_IN | G_IO_PRI) )
	{
		while(read)
		{
			g_io_channel_read(ioc, buf, 1024, &read);
			fwrite(buf, 1, read, out_file);
		}
	}
	g_free(buf);
	fclose(out_file);
}

/*
 * xarchive_bzip2_support_add(XArchive *archive, GSList *files)
 *
 * can only compress one file, 
 * (will return compressed file in XArchive->path)
 */
/*
gint
xa_support_bzip2_add(XAArchive *archive, GSList *files)
{
	gchar *command, *dir, *filename;
	if(files != NULL)
	{
		if(g_slist_length(files) > 1)
			g_warning("bzip2 can compress one file only");
	
		dir = g_path_get_dirname(files->data);
		chdir(dir);
		g_free(dir);

		filename = g_path_get_basename(files->data);
		
		command = g_strconcat("bzip2 -kz ", filename, NULL);
		archive->child_pid = xarchiver_async_process ( archive , command, 0);
		g_free(command);
		if (archive->child_pid == 0)
		{
			g_message (archive->error->message);
			g_error_free (archive->error);
			return 1;
		}
		if(archive->path)
			g_free(archive->path);
		archive->path = g_strconcat(filename,".bz2");
	}

	fchdir(n_cwd);
	return 0;
}
*/


/*
 * xarchive_bzip2_support_extract(XArchive *archive, GSList *files)
 * Extract archive
 *
 * FIXME:
 * destination-folder does not work with bare bzip
 */

XASupport*
xa_support_bzip2_new ()
{
	XASupport *support;

	support = g_object_new(XA_TYPE_SUPPORT_BZIP2, NULL);
	
	return support;
}
