/*
 *  Copyright (c) 2006 Stephan Arts     <psyBSD@gmail.com>
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
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <glib.h>
#include <glib-object.h>
#include "internals.h"
#include "archive.h"
#include "archive-types.h"
#include "support.h"
#include "support-gnu-tar.h"

void
xa_support_gnu_tar_init(XASupportGnuTar *support);

gint
xa_support_gnu_tar_open(XASupport *support, XAArchive *archive);

gint
xa_support_gnu_tar_add (XASupport *support, XAArchive *archive, GSList *files);

gint
xa_support_gnu_tar_remove (XASupport *support, XAArchive *archive, GSList *files);

gint
xa_support_gnu_tar_extract(XASupport *support, XAArchive *archive, gchar *destination_path, GSList *files, gboolean full_path);

gboolean 
xa_support_gnu_tar_parse_output (GIOChannel *ioc, GIOCondition cond, gpointer data);

GType
xa_support_gnu_tar_get_type ()
{
	static GType xa_support_gnu_tar_type = 0;

 	if (!xa_support_gnu_tar_type)
	{
 		static const GTypeInfo xa_support_gnu_tar_info = 
		{
			sizeof (XASupportGnuTarClass),
			(GBaseInitFunc) NULL,
			(GBaseFinalizeFunc) NULL,
			(GClassInitFunc) NULL,
			(GClassFinalizeFunc) NULL,
			NULL,
			sizeof (XASupportGnuTar),
			0,
			(GInstanceInitFunc) xa_support_gnu_tar_init,
			NULL
		};

		xa_support_gnu_tar_type = g_type_register_static (XA_TYPE_SUPPORT, "XASupportGnuTar", &xa_support_gnu_tar_info, 0);
	}
	return xa_support_gnu_tar_type;
}

void
xa_support_gnu_tar_init(XASupportGnuTar *support)
{
	XASupport *xa_support = XA_SUPPORT(support);
	gint n_columns = 5;
	gchar **column_names  = g_new0(gchar *, n_columns);
	GType *column_types  = g_new0(GType, n_columns);

	column_names[0] = "Filename";
	column_names[1] = "Permissions";
	column_names[2] = "Owner / Group";
	column_names[3] = "Size";
	column_names[4] = "Date";
	column_types[0] = G_TYPE_STRING;
	column_types[1] = G_TYPE_STRING;
	column_types[2] = G_TYPE_STRING;
	column_types[3] = G_TYPE_UINT64;
	column_types[4] = G_TYPE_STRING; /* DATE */

	xa_support_set_columns(xa_support, n_columns, column_names, column_types);
	xa_support->type = XARCHIVETYPE_TAR;
	xa_support->verify = xa_archive_type_tar_verify;
	xa_support->open = xa_support_gnu_tar_open;
	xa_support->add = xa_support_gnu_tar_add;
	xa_support->remove = xa_support_gnu_tar_remove;
	xa_support->extract = xa_support_gnu_tar_extract;
	xa_support->parse_output = xa_support_gnu_tar_parse_output;

	g_free(column_names);
	g_free(column_types);
}

gboolean 
xa_support_gnu_tar_parse_output (GIOChannel *ioc, GIOCondition cond, gpointer data)
{
	XASupport *support = XA_SUPPORT(data);
	gchar *line = NULL;
	GValue *filename    = NULL;
	GValue *permissions = NULL;
	GValue *owner       = NULL;
	GValue *size        = NULL;
	GValue *date        = NULL;
	gchar *_size;

	gint i = 0, a = 0;
	XAArchive *archive = support->exec.archive;
	if (cond & (G_IO_IN | G_IO_PRI) )
	{
		for(i = 0 ; (i < 100) && (g_io_channel_read_line ( ioc, &line, NULL, NULL, NULL ) == G_IO_STATUS_NORMAL); i++)
		{
			filename    = g_new0(GValue, 1);
			permissions = g_new0(GValue, 1);
			owner       = g_new0(GValue, 1);
			size        = g_new0(GValue, 1);
			date        = g_new0(GValue, 1);

			date = g_value_init(date, G_TYPE_STRING);
			g_value_set_string(date, "01-01-1970");

			for(i = 13; i < strlen(line); i++)
				if(line[i] == ' ') break;
			permissions = g_value_init(permissions, G_TYPE_STRING);
			g_value_set_string(permissions, g_strndup(line, 10));
	
			owner = g_value_init(owner , G_TYPE_STRING);
			g_value_set_string(owner, g_strndup(&line[11], i-11));

			for(; i < strlen(line); i++)
				if(line[i] >= '0' && line[i] <= '9') break;
			a = i;
			for(; i < strlen(line); i++)
				if(line[i] == ' ') break;

			size = g_value_init(size, G_TYPE_UINT64);
			_size = g_strndup(&line[a], i-a);
			g_value_set_uint64(size, atoll ( _size ));
			g_free (_size);
			a = i++;
			for(; i < strlen(line); i++) // DATE
				if(line[i] == ' ') break;
			a = i++;
			for(; i < strlen(line); i++) // TIME
				if(line[i] == ' ') break;

			filename = g_value_init(filename, G_TYPE_STRING);
			g_value_set_string(filename, g_strstrip(g_strndup(&line[i], strlen(line)-i-1)));

			archive->row = g_list_prepend(archive->row, filename);
			archive->row = g_list_prepend(archive->row, permissions);
			archive->row = g_list_prepend(archive->row, owner);
			archive->row = g_list_prepend(archive->row, size);
			archive->row = g_list_prepend(archive->row, date);
			
			archive->dummy_size += (unsigned long int)g_list_nth_data ( archive->row , 1);
			if ( strstr ((gchar *)g_list_nth_data ( archive->row , 3) , "d") == NULL )
				archive->nr_of_dirs++;
  	      else
				archive->nr_of_files++;
			g_free(line);
		}
		xa_support_emit_signal(support, XA_SUPPORT_SIGNAL_APPEND_ROWS);
		return TRUE;
	}
	else if (cond & (G_IO_ERR | G_IO_HUP ) )
	{
		g_io_channel_shutdown ( ioc,TRUE,NULL );
		g_io_channel_unref (ioc);
		return FALSE;
	}
	return TRUE;
}

gint
xa_support_gnu_tar_open(XASupport *support, XAArchive *archive)
{
	gint child_pid;

	support->exec.command = g_strconcat ( "tar tfv " , archive->path, NULL );
	support->exec.archive = archive;
	support->exec.parse_output = support->parse_output;
	support->exec.signal = -1;

	xa_support_emit_signal(support, XA_SUPPORT_SIGNAL_UPDATE_ROWS);

	xa_support_execute(support);
	g_free (support->exec.command);
	archive->dummy_size = 0;
}

gint
xa_support_gnu_tar_remove (XASupport *support, XAArchive *archive, GSList *files)
{
	GString *names;
	GSList *_files = files;
	if(files != NULL)
	{
		names = concatenatefilenames ( _files , FALSE);
		support->exec.command = g_strconcat ( "tar --delete -vf " , archive->path ," ", names->str , NULL );
		support->exec.archive = archive;
		support->exec.signal = XA_SUPPORT_SIGNAL_ARCHIVE_MODIFIED;
		support->exec.parse_output = 0;
		
		xa_support_execute( support );
		g_free (support->exec.command);
		g_string_free (names, TRUE);
	}
	return 0;
}

gint
xa_support_gnu_tar_add (XASupport *support, XAArchive *archive, GSList *files)
{
	gchar *dir;
	GString *names;
	
	GSList *_files = files;
	if(files != NULL)
	{
		dir = g_path_get_dirname(_files->data);
		chdir(dir);
		g_free(dir);

		names = concatenatefilenames ( _files , TRUE);		
		// Check if the archive already exists or not
		if(g_file_test(archive->path, G_FILE_TEST_EXISTS))
			support->exec.command = g_strconcat("tar rvvf ", archive->path, " ", names->str, NULL);
		else
			support->exec.command = g_strconcat("tar cvvf ", archive->path, " ", names->str, NULL);
		support->exec.archive = archive;
		support->exec.signal = XA_SUPPORT_SIGNAL_ARCHIVE_MODIFIED;
		support->exec.parse_output = 0;

		xa_support_execute(support);
		g_free (support->exec.command);
		g_string_free(names, TRUE);
	}
	fchdir(n_cwd);
	return TRUE;
}

gint
xa_support_gnu_tar_extract(XASupport *support, XAArchive *archive, gchar *destination_path, GSList *files, gboolean full_path)
{
	gchar *dir, *filename;
	unsigned short int levels;
	char digit[2];
	gchar *strip = NULL;
	gboolean to_free = FALSE;
    
	if(!g_file_test(archive->path, G_FILE_TEST_EXISTS))
		return FALSE;
    
	// Only extract certain files
	if( (files == NULL) || (g_slist_length(files) == 0))
	{
		support->exec.command = g_strconcat("tar xf ", archive->path, " -C ", destination_path, NULL);
	} 
	else
	{
		GSList *_files = files;
		GString *names;
		names = concatenatefilenames ( _files , TRUE);
		if ( full_path == 0 )
		{
			to_free = TRUE;
			levels = countcharacters ( names->str , '/');
			sprintf ( digit , "%d" , levels );
			strip = g_strconcat ( "--strip-components=" , digit , " " , NULL );
		}
		support->exec.command = g_strconcat("tar " , full_path ? "" : strip , "-xvf ", archive->path, " -C ", destination_path, names->str , NULL);
		g_string_free (names,TRUE);
	}
	support->exec.archive = archive;
	support->exec.signal = -1;
	support->exec.parse_output = 0;

	xa_support_execute(support);
	if (to_free)
		g_free (strip);
	g_free (support->exec.command);
	fchdir(n_cwd);
	return TRUE;
}

XASupport*
xa_support_gnu_tar_new()
{
	XASupport *support;

	support = g_object_new(XA_TYPE_SUPPORT_GNU_TAR, NULL);
	
	return support;
}
