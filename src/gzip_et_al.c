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

#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include "gzip_et_al.h"
#include "main.h"
#include "string_utils.h"
#include "support.h"
#include "tar.h"
#include "window.h"

void xa_gzip_et_al_ask (XArchive *archive)
{
	switch (archive->type)
	{
		case XARCHIVETYPE_BZIP2:
		case XARCHIVETYPE_GZIP:
			archive->can_test = TRUE;
			archive->can_extract = TRUE;
			break;

		case XARCHIVETYPE_TAR_BZ2:
		case XARCHIVETYPE_TAR_GZ:
		case XARCHIVETYPE_TAR_LZMA:
		case XARCHIVETYPE_TAR_LZOP:
		case XARCHIVETYPE_TAR_XZ:
			archive->can_test = TRUE;
			archive->can_extract = TRUE;
			archive->can_add = TRUE;
			archive->can_overwrite = TRUE;
			archive->can_full_path = TRUE;
			archive->can_touch = TRUE;
			archive->can_update = TRUE;
			archive->can_recurse = TRUE;
			archive->can_move = TRUE;
			break;

		default:
			break;
	}
}

static void xa_open_tar_compressed_file (XArchive *archive)
{
	gchar *command = NULL;
	guint i;

	if (archive->type == XARCHIVETYPE_TAR_BZ2)
		command = g_strconcat(tar," tfjv ",archive->path[1],NULL);
	else if (archive->type == XARCHIVETYPE_TAR_GZ)
		command = g_strconcat(tar, " tvzf ", archive->path[1], NULL);
	else if (archive->type == XARCHIVETYPE_TAR_LZMA)
		command = g_strconcat(tar," tv --use-compress-program=lzma -f ",archive->path[1],NULL);
	else if (archive->type == XARCHIVETYPE_TAR_XZ)
		command = g_strconcat(tar," tv --use-compress-program=xz -f ",archive->path[1],NULL);
	else if (archive->type == XARCHIVETYPE_TAR_LZOP)
		command = g_strconcat(tar," tv --use-compress-program=lzop -f ",archive->path[1],NULL);
	/* else fail? */

	archive->files_size = 0;
	archive->nr_of_files = 0;
	archive->nc = 7;
	archive->parse_output = xa_tar_parse_output;
	xa_spawn_async_process (archive,command);
	g_free (command);

	GType types[]= {GDK_TYPE_PIXBUF,G_TYPE_STRING,G_TYPE_STRING,G_TYPE_STRING,G_TYPE_STRING,G_TYPE_UINT64,G_TYPE_STRING,G_TYPE_STRING,G_TYPE_POINTER};
	archive->column_types = g_malloc0(sizeof(types));
	for (i = 0; i < 9; i++)
		archive->column_types[i] = types[i];

	char *names[]= {(_("Points to")),(_("Permissions")),(_("Owner/Group")),(_("Size")),(_("Date")),(_("Time")),NULL};
	xa_create_liststore (archive,names);
}

static void xa_gzip_parse_output (gchar *line, XArchive *archive)
{
	gchar *filename;
	gchar *basename;
	gpointer item[3];
	gint n = 0, a = 0 ,linesize = 0;

	linesize = strlen(line);
	if (line[9] == 'c')
		return;

	/* Size */
	for(n=0; n < linesize && line[n] == ' '; n++);
	a = n;
	for(; n < linesize && line[n] != ' '; n++);
	line[n]='\0';
	item[0] = line + a;
	n++;

	/* Compressed */
	for(; n < linesize && line[n] == ' '; n++);
	a = n;
	for(; n < linesize && line[n] != ' '; n++);
	line[n]='\0';
	item[1] = line + a;
	archive->files_size += g_ascii_strtoull(item[1],NULL,0);
	n++;

	/* Ratio */
	for(; n < linesize && line[n] == ' '; n++);
	a = n;
	for(; n < linesize && line[n] != ' '; n++);
	line[n] = '\0';
	item[2] = line + a;
	n++;

	line[linesize-1] = '\0';
	filename = line+n;

	basename = g_path_get_basename(filename);
	if (basename == NULL)
		basename = g_strdup(filename);

	xa_set_archive_entries_for_each_row (archive,basename,item);
	g_free(basename);
}

void xa_gzip_et_al_open (XArchive *archive)
{
	gchar *filename = NULL;;
	gchar *_filename;
	gpointer item[2];
	gboolean result;
	gint len = 0;

	archive->delete =	delete[archive->type];
	archive->add = 		add[archive->type];
	archive->extract = 	extract[archive->type];

	if (g_str_has_suffix(archive->path[1],".tar.bz2") || g_str_has_suffix (archive->path[1],".tar.bz")
    	|| g_str_has_suffix ( archive->path[1] , ".tbz") || g_str_has_suffix (archive->path[1],".tbz2") )
	{
		archive->type = XARCHIVETYPE_TAR_BZ2;
		xa_open_tar_compressed_file(archive);
	}
	else if (g_str_has_suffix(archive->path[1],".tar.gz") || g_str_has_suffix (archive->path[1],".tgz"))
	{
		archive->type = XARCHIVETYPE_TAR_GZ;
		xa_open_tar_compressed_file(archive);
	}
	else if (g_str_has_suffix(archive->path[1],".tar.lzma") || g_str_has_suffix (archive->path[1],".tlz"))
	{
		archive->type = XARCHIVETYPE_TAR_LZMA;
		xa_open_tar_compressed_file(archive);
	}
	else if (g_str_has_suffix(archive->path[1],".tar.xz") || g_str_has_suffix (archive->path[1],".txz"))
	{
		archive->type = XARCHIVETYPE_TAR_XZ;
		xa_open_tar_compressed_file(archive);
	}
	else if (g_str_has_suffix(archive->path[1],".tar.lzop") ||
		g_str_has_suffix (archive->path[1],".tzo") ||
		g_str_has_suffix(archive->path[1],".tar.lzo"))
	{
		archive->type = XARCHIVETYPE_TAR_LZOP;
		xa_open_tar_compressed_file(archive);
	}
	else if (archive->type == XARCHIVETYPE_GZIP)
	{
		gchar *command;
		guint i;

		archive->nc = 4;
		archive->parse_output = xa_gzip_parse_output;
		archive->nr_of_files = 1;

		GType types[]= {GDK_TYPE_PIXBUF,G_TYPE_STRING,G_TYPE_UINT64,G_TYPE_UINT64,G_TYPE_STRING,G_TYPE_POINTER};
		archive->column_types = g_malloc0(sizeof(types));
		for (i = 0; i < 6; i++)
			archive->column_types[i] = types[i];

		char *names[]= {(_("Compressed")),(_("Size")),(_("Ratio"))};
		xa_create_liststore (archive,names);

		command = g_strconcat ("gzip -l ",archive->path[1],NULL);
		xa_spawn_async_process (archive,command);
		g_free (command);
	}
	else
	{
		struct stat my_stat;
		gchar *compressed = NULL;
		gchar *size = NULL,*command = NULL,*executable = NULL,*dot = NULL;
		guint i;
		GSList *list = NULL;

		archive->nc = 3;
		archive->nr_of_files = 1;

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
		else if (archive->type == XARCHIVETYPE_XZ)
		{
			executable = "xz ";
			len = 3;
		}
		else if (archive->type == XARCHIVETYPE_LZOP)
		{
			executable = "lzop ";
			len = 4;
		} /* else fail? */


		GType types[]= {GDK_TYPE_PIXBUF,G_TYPE_STRING,G_TYPE_UINT64,G_TYPE_UINT64,G_TYPE_POINTER};
		archive->column_types = g_malloc0(sizeof(types));
		for (i = 0; i < 5; i++)
			archive->column_types[i] = types[i];

		char *names[]= {(_("Original")),(_("Compressed"))};
		xa_create_liststore (archive,names);
		result = xa_create_working_directory(archive);
		if (!result)
			return;

		/* Let's copy the bzip2 file in the tmp dir */
		command = g_strconcat("cp -f ",archive->path[1]," ",archive->working_dir,NULL);
		list = g_slist_append(list,command);
		/* Let's get its compressed file size */
		stat (archive->path[1],&my_stat);
		compressed = g_strdup_printf("%" G_GUINT64_FORMAT, (guint64) my_stat.st_size);
		item[1] = compressed;

		/* Let's extract it */
		_filename = g_path_get_basename(archive->path[1]);
		if (_filename[0] == '.')
			command = g_strconcat(executable,"-f -d ",archive->working_dir,"/",archive->path[1],NULL);
		else
			command = g_strconcat(executable,"-f -d ",archive->working_dir,"/",_filename,NULL);

		list = g_slist_append(list,command);
		xa_run_command (archive,list);

		/* and let's get its uncompressed file size */
		dot = strrchr(_filename,'.');
		if (_filename || G_LIKELY(dot))
		{
			filename = g_strndup(_filename,strlen(_filename) - len);
			command = g_strconcat(archive->working_dir,"/",filename,NULL);
		}
		else
		{
			command = g_strconcat(archive->working_dir,"/",archive->path[1],NULL);
			filename = g_strdup(archive->path[1]);
		}
		stat (command,&my_stat);
		g_free(command);
		size = g_strdup_printf("%" G_GUINT64_FORMAT, (guint64) my_stat.st_size);
		archive->files_size = my_stat.st_size;
		item[0] = size;

		xa_set_archive_entries_for_each_row (archive,filename,item);
		g_free(compressed);
		g_free(size);
		g_free(filename);

		xa_update_window_with_archive_entries (archive,NULL);
		gtk_tree_view_set_model (GTK_TREE_VIEW(archive->treeview), archive->model);
		g_object_unref (archive->model);
	}
}

void xa_gzip_et_al_test (XArchive *archive)
{
	gchar  *command = NULL,*executable = NULL,*filename = NULL, *dot = NULL, *filename_noext = NULL;
	GSList *list = NULL;

	if (archive->type == XARCHIVETYPE_GZIP)
		executable = "gzip ";
	if (archive->type == XARCHIVETYPE_BZIP2)
		executable = "bzip2 ";
	else if (archive->type == XARCHIVETYPE_LZMA)
		executable = "lzma ";
	else if (archive->type == XARCHIVETYPE_XZ)
		executable = "xz ";
	else if (archive->type == XARCHIVETYPE_LZOP)
		executable = "lzop ";
	/* else fail? */
	filename = xa_remove_path_from_archive_name(archive->path[1]);
	dot = strrchr(filename,'.');
	if (G_LIKELY(dot))
	{
		filename_noext = g_strndup(filename,(dot - filename));
		g_free(filename);
	}
	else
		filename_noext = filename;

	command = g_strconcat("sh -c \"",executable, " ",archive->path[1]," -tv ","\"",NULL);
	g_free(filename_noext);
	list = g_slist_append(list,command);
	xa_run_command (archive,list);
}

gboolean xa_gzip_et_al_extract (XArchive *archive, GSList *file_list)
{
	GSList *list = NULL;
	gchar  *command = NULL,*executable = NULL,*filename = NULL, *dot = NULL, *filename_noext = NULL;

	if (archive->type == XARCHIVETYPE_BZIP2)
		executable = "bzip2 ";
	else if (archive->type == XARCHIVETYPE_GZIP)
		executable = "gzip ";
	else if (archive->type == XARCHIVETYPE_LZMA)
		executable = "lzma ";
	else if (archive->type == XARCHIVETYPE_XZ)
		executable = "xz ";
	else if (archive->type == XARCHIVETYPE_LZOP)
		executable = "lzop ";
	/* else fail? */
	filename = xa_remove_path_from_archive_name(archive->path[1]);
	dot = strrchr(filename,'.');
	if (G_LIKELY(dot))
	{
		filename_noext = g_strndup(filename,(dot - filename));
		g_free(filename);
	}
	else
		filename_noext = filename;

	command = g_strconcat("sh -c \"",executable, " ",archive->path[1]," -dc > ",archive->extraction_dir,"/",filename_noext,"\"",NULL);
	g_free(filename_noext);
	list = g_slist_append(list,command);

	return xa_run_command(archive, list);
}
