/*
 *  Copyright (C) 2008 Giuseppe Torelli - <colossus73@gmail.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Street #330, Boston, MA 02111-1307, USA.
 */

#include "config.h"
#include "bzip2.h"
#include "extract_dialog.h"
#include "string_utils.h"

extern delete_func	delete	[XARCHIVETYPE_COUNT];
extern add_func		add	[XARCHIVETYPE_COUNT];
extern extract_func	extract	[XARCHIVETYPE_COUNT];
short int l;

void xa_open_bzip2_lzma (XArchive *archive)
{
	XEntry *entry = NULL;
	gchar *filename = NULL;;
	gchar *_filename;
	gpointer item[2];
	gboolean result;
	gint len = 0;

	if (g_str_has_suffix(archive->escaped_path,".tar.bz2") || g_str_has_suffix (archive->escaped_path,".tar.bz")
    	|| g_str_has_suffix ( archive->escaped_path , ".tbz") || g_str_has_suffix (archive->escaped_path,".tbz2") )
	{
		archive->type = XARCHIVETYPE_TAR_BZ2;
		archive->format = "TAR.BZIP2";
		archive->delete =	delete[archive->type];
		archive->add = 		add[archive->type];
		archive->extract = 	extract[archive->type];
		xa_open_tar_compressed_file(archive);
	}
	else if (g_str_has_suffix(archive->escaped_path,".tar.lzma") || g_str_has_suffix (archive->escaped_path,".tlz"))
	{
		archive->type = XARCHIVETYPE_TAR_LZMA;
		archive->format = "TAR.LZMA";
		archive->delete =	delete[archive->type];
		archive->add = 		add[archive->type];
		archive->extract = 	extract[archive->type];
		xa_open_tar_compressed_file(archive);
	}
	else if (g_str_has_suffix(archive->escaped_path,".tar.lzop") ||
		g_str_has_suffix (archive->escaped_path,".tzo") ||
		g_str_has_suffix(archive->escaped_path,".tar.lzo"))
	{
		archive->type = XARCHIVETYPE_TAR_LZOP;
		archive->format = "TAR.LZOP";
		archive->delete =	delete[archive->type];
		archive->add = 		add[archive->type];
		archive->extract = 	extract[archive->type];
		xa_open_tar_compressed_file(archive);
	}
	else
	{
		struct stat my_stat;
		gchar *compressed = NULL;
		gchar *size = NULL,*command = NULL,*executable = NULL,*dot = NULL;
		unsigned short int i;
		GSList *list = NULL;

		if (archive->type == XARCHIVETYPE_BZIP2)
		{
			archive->format = "BZIP2";
			executable = "bzip2 ";
			len = 4;
		}
		else if (archive->type == XARCHIVETYPE_LZMA)
		{
			archive->format = "LZMA";
			executable = "lzma ";
			len = 5;
		}
		else if (archive->type == XARCHIVETYPE_LZOP)
		{
			archive->format = "LZOP";
			executable = "lzop ";
			len = 4;
		} /* else fail? */
		archive->can_add = archive->has_test = archive->has_sfx = FALSE;
		archive->has_properties = archive->can_extract = TRUE;
		archive->nc = 3;
		archive->nr_of_files = 1;

		GType types[]= {GDK_TYPE_PIXBUF,G_TYPE_STRING,G_TYPE_UINT64,G_TYPE_UINT64,G_TYPE_POINTER};
		archive->column_types = g_malloc0(sizeof(types));
		for (i = 0; i < 5; i++)
			archive->column_types[i] = types[i];

		char *names[]= {(_("Original")),(_("Compressed"))};
		xa_create_liststore (archive,names);
		result = xa_create_temp_directory (archive);
		if (!result)
			return;

		/* Let's copy the bzip2 file in the tmp dir */
		command = g_strconcat("cp -f ",archive->escaped_path," ",archive->tmp,NULL);
		list = g_slist_append(list,command);
		/* Let's get its compressed file size */
		stat (archive->escaped_path,&my_stat);
		compressed = g_strdup_printf("%lld",(unsigned long long int)my_stat.st_size);
		item[1] = compressed;

		/* Let's extract it */
		_filename = g_path_get_basename(archive->escaped_path);
		if (_filename[0] == '.')
			command = g_strconcat(executable,"-f -d ",archive->tmp,"/",archive->escaped_path,NULL);
		else
			command = g_strconcat(executable,"-f -d ",archive->tmp,"/",_filename,NULL);

		list = g_slist_append(list,command);
		xa_run_command (archive,list);

		/* and let's get its uncompressed file size */
		dot = strrchr(_filename,'.');
		if (_filename || G_LIKELY(dot))
		{
			filename = g_strndup(_filename,strlen(_filename) - len);
			command = g_strconcat(archive->tmp,"/",filename,NULL);
		}
		else
		{
			command = g_strconcat(archive->tmp,"/",archive->escaped_path,NULL);
			filename = g_strdup(archive->escaped_path);
		}
		stat (command,&my_stat);
		g_free(command);
		size = g_strdup_printf("%lld",(unsigned long long int)my_stat.st_size);
		archive->dummy_size = my_stat.st_size;
		item[0] = size;

		entry = xa_set_archive_entries_for_each_row (archive,filename,item);
		g_free(compressed);
		g_free(size);
		g_free(filename);

		xa_update_window_with_archive_entries (archive,NULL);
		gtk_tree_view_set_model (GTK_TREE_VIEW(archive->treeview), archive->model);
		g_object_unref (archive->model);
	}
}

void xa_open_tar_compressed_file(XArchive *archive)
{
	gchar *command = NULL;
	unsigned short int i;

	if (archive->type == XARCHIVETYPE_TAR_BZ2)
		command = g_strconcat(tar," tfjv ",archive->escaped_path,NULL);
	else if (archive->type == XARCHIVETYPE_TAR_LZMA)
		command = g_strconcat(tar," tv --use-compress-program=lzma -f ",archive->escaped_path,NULL);
	else if (archive->type == XARCHIVETYPE_TAR_LZOP)
		command = g_strconcat(tar," tv --use-compress-program=lzop -f ",archive->escaped_path,NULL);
	/* else fail? */

	archive->has_properties = archive->can_add = archive->can_extract = TRUE;
	archive->has_test = archive->has_sfx = FALSE;
	archive->dummy_size = 0;
	archive->nr_of_files = 0;
	archive->nc = 7;
	archive->parse_output = xa_get_tar_line_content;
	xa_spawn_async_process (archive,command);
	g_free (command);

	if (archive->child_pid == 0)
		return;

	GType types[]= {GDK_TYPE_PIXBUF,G_TYPE_STRING,G_TYPE_STRING,G_TYPE_STRING,G_TYPE_STRING,G_TYPE_UINT64,G_TYPE_STRING,G_TYPE_STRING,G_TYPE_POINTER};
	archive->column_types = g_malloc0(sizeof(types));
	for (i = 0; i < 9; i++)
		archive->column_types[i] = types[i];

	char *names[]= {(_("Points to")),(_("Permissions")),(_("Owner/Group")),(_("Size")),(_("Date")),(_("Time")),NULL};
	xa_create_liststore (archive,names);
}

gboolean lzma_bzip2_extract (XArchive *archive,GSList *dummy)
{
	GSList *list = NULL;
	gchar  *command = NULL,*executable = NULL,*filename = NULL, *dot = NULL, *filename_noext = NULL;
	gboolean result = FALSE;
	gint len = 0;

	if (archive->type == XARCHIVETYPE_BZIP2)
	{
		executable = "bzip2 ";
		len = 4;
	}
	else if (archive->type == XARCHIVETYPE_LZMA)
	{
		executable = "lzma ";
		len = 5;
	}
	else if (archive->type == XARCHIVETYPE_LZOP)
	{
		executable = "lzop ";
		len = 5;
	}
	/* else fail? */
	filename = xa_remove_path_from_archive_name(archive->escaped_path);
	dot = strrchr(filename,'.');
	if (G_LIKELY(dot))
	{
		filename_noext = g_strndup(filename,(dot - filename));
		g_free(filename);
	}
	else
		filename_noext = filename;

	command = g_strconcat("sh -c \"",executable, " ",archive->escaped_path," -dc > ",archive->extraction_path,"/",filename_noext,"\"",NULL);
	g_free(filename_noext);
	list = g_slist_append(list,command);
	result = xa_run_command (archive,list);
	return result;
}
