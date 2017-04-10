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
#include "tar.h"
#include "interface.h"
#include "main.h"
#include "string_utils.h"
#include "support.h"
#include "window.h"

#define TMPFILE "xa-tmp.tar"

gboolean isTar (FILE *file)
{
	unsigned char magic[7];

	fseek(file, 0, SEEK_SET);

	if (fseek(file, 257, SEEK_CUR) != 0 ||
	    fread(magic, 1, sizeof(magic), file) != sizeof(magic))
	{
		fseek(file, 0, SEEK_SET);
		return FALSE;
	}

	fseek(file, 0, SEEK_SET);

	return (memcmp(magic, "\x75\x73\x74\x61\x72\x00\x30", sizeof(magic)) == 0 ||
	        memcmp(magic, "\x75\x73\x74\x61\x72\x20\x20", sizeof(magic)) == 0 ||
	        memcmp(magic, "\x0\x0\x0\x0\x0\x0\x0", sizeof(magic)) == 0);
}

static XArchiveType xa_tar_get_compressor_type (XArchive *archive)
{
	switch (archive->type)
	{
		case XARCHIVETYPE_TAR_BZ2:
			return XARCHIVETYPE_BZIP2;

		case XARCHIVETYPE_TAR_GZ:
			return XARCHIVETYPE_GZIP;

		case XARCHIVETYPE_TAR_LZMA:
			return XARCHIVETYPE_LZMA;

		case XARCHIVETYPE_TAR_LZOP:
			return XARCHIVETYPE_LZOP;

		case XARCHIVETYPE_TAR_XZ:
			return XARCHIVETYPE_XZ;

		default:
			return XARCHIVETYPE_TAR;
	}
}

void xa_tar_ask (XArchive *archive)
{
	archive->can_extract = TRUE;
	archive->can_add = archiver[xa_tar_get_compressor_type(archive)].is_compressor;
	archive->can_delete = archiver[xa_tar_get_compressor_type(archive)].is_compressor;
	archive->can_overwrite = TRUE;
	archive->can_full_path = TRUE;
	archive->can_touch = TRUE;
	archive->can_update = TRUE;
	archive->can_recurse = archiver[xa_tar_get_compressor_type(archive)].is_compressor;
	archive->can_move = archiver[xa_tar_get_compressor_type(archive)].is_compressor;
}

static void xa_tar_parse_output (gchar *line, XArchive *archive)
{
	gchar *filename;
	gpointer item[6];
	gint n = 0, a = 0 ,linesize = 0;

	linesize = strlen(line);
	archive->files++;

	/* Permissions */
	line[10] = '\0';
	item[1] = line;

	/* Owner */
	for(n=13; n < linesize; ++n)
		if(line[n] == ' ')
			break;
	line[n] = '\0';
	item[2] = line+11;

	/* Size */
	for(++n; n < linesize; ++n)
		if(line[n] >= '0' && line[n] <= '9')
			break;
	a = n;

	for(; n < linesize; ++n)
		if(line[n] == ' ')
			break;

	line[n] = '\0';
	item[3] = line + a;
	archive->files_size += g_ascii_strtoull(item[3],NULL,0);
	a = ++n;

	/* Date */
	for(; n < linesize; n++)
		if(line[n] == ' ')
			break;

	line[n] = '\0';
	item[4] = line + a;

	/* Time */
	a = ++n;
	for (; n < linesize; n++)
		if (line[n] == ' ')
			break;

	line[n] = '\0';
	item[5] = line + a;
	n++;
	line[linesize-1] = '\0';

	filename = line + n;

	/* Symbolic link */
	gchar *temp = g_strrstr (filename,"->");
	if (temp)
	{
		gint len = strlen(filename) - strlen(temp);
		item[0] = (filename +=3) + len;
		filename[strlen(filename) - strlen(temp)-1] = '\0';
	}
	else
		item[0] = NULL;

	if(line[0] == 'd')
	{
		/* Work around for gtar, which does not output / with directories */
		if(line[linesize-2] != '/')
			filename = g_strconcat(line + n, "/", NULL);
		else
			filename = g_strdup(line + n);
	}
	else
		filename = g_strdup(line + n);
	xa_set_archive_entries_for_each_row (archive,filename,item);
	g_free(filename);
}

void xa_tar_open (XArchive *archive)
{
	const GType types[] = {GDK_TYPE_PIXBUF, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_UINT64, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_POINTER};
	const gchar *titles[] = {_("Points to"), _("Permissions"), _("Owner/Group"), _("Size"), _("Date"), _("Time")};
	gchar *command;
	guint i;

	if (!archive->path[2])
		archive->path[2] = g_shell_quote(archive->path[0]);

	archive->files = 0;
	archive->files_size = 0;

	command = g_strconcat(archiver[XARCHIVETYPE_TAR].program[0], " -tvf ", archive->path[2], NULL);
	archive->parse_output = xa_tar_parse_output;
	xa_spawn_async_process(archive, command);
	g_free(command);

	archive->columns = 9;
	archive->column_types = g_malloc0(sizeof(types));

	for (i = 0; i < archive->columns; i++)
		archive->column_types[i] = types[i];

	xa_create_liststore(archive, titles);
}

static gboolean xa_concat_filenames (GtkTreeModel *model,GtkTreePath *path,GtkTreeIter *iter,GSList **list)
{
	XEntry *entry;
	gint current_page,idx;

	current_page = gtk_notebook_get_current_page(notebook);
	idx = xa_find_archive_index (current_page);

	gtk_tree_model_get(model, iter, archive[idx]->columns - 1, &entry, -1);
	if (entry == NULL)
		return TRUE;
	else
		xa_fill_list_with_recursed_entries(entry->child,list);
	return FALSE;
}

static gboolean xa_extract_tar_without_directories (gchar *string, XArchive *archive, gchar *files_to_extract)
{
	GString *files = NULL;
	gchar *command[2];
	GSList *file_list = NULL;
	gboolean result;

	result = xa_create_working_directory(archive);
	if (!result)
		return FALSE;

	if (strlen(files_to_extract) == 0)
	{
		gtk_tree_model_foreach(GTK_TREE_MODEL(archive->liststore),(GtkTreeModelForeachFunc) xa_concat_filenames,&file_list);
		files = xa_quote_filenames(file_list, NULL, TRUE);
		files_to_extract = files->str;
	}

	command[0] = g_strconcat (string, archive->path[1],
										#ifdef __FreeBSD__
											archive->do_overwrite ? " " : " -k",
										#else
											archive->do_overwrite ? " --overwrite" : " --keep-old-files",
											" --no-wildcards ",
										#endif
										archive->do_touch ? " --touch" : "",
										" -C ", archive->working_dir, files_to_extract, NULL);

	if (strstr(files_to_extract,"/") || strcmp(archive->working_dir,archive->extraction_dir) != 0)
	{
		archive->child_dir = g_strdup(archive->working_dir);
		command[1] = g_strconcat ("mv -f ",files_to_extract," ",archive->extraction_dir,NULL);
	}
	else
		command[1] = NULL;

	if (files)
		g_string_free(files, TRUE);

	result = xa_run_command(archive, command[0]);
	g_free(command[0]);

	if (result && command[1])
	{
		result = xa_run_command(archive, command[1]);
		g_free(command[1]);
	}

	return result;
}

/*
 * Note: tar lists '\' as '\\' while it extracts '\', i.e.
 * file names containing this character can't be handled entirely.
 */

gboolean xa_tar_extract (XArchive *archive, GSList *file_list)
{
	GString *files;
	gchar *command;
	gboolean result = FALSE;

	files = xa_quote_filenames(file_list, NULL, TRUE);

	switch (archive->type)
	{
		case XARCHIVETYPE_TAR:
		if (archive->do_full_path)
		{
			command = g_strconcat(archiver[XARCHIVETYPE_TAR].program[0], " -xvf ", archive->path[1],
						#ifdef __FreeBSD__
								archive->do_overwrite ? " " : " -k",
						#else
								archive->do_overwrite ? " --overwrite" : " --keep-old-files",
						#endif
								archive->do_touch ? " --touch" : "",
								" -C ", archive->extraction_dir, files->str, NULL);
		}
		else
		{
			result = xa_extract_tar_without_directories ( "tar -xvf ",archive,files->str);
			command = NULL;
		}
		break;

		case XARCHIVETYPE_TAR_BZ2:
		if (archive->do_full_path)
		{
			command = g_strconcat(archiver[XARCHIVETYPE_TAR].program[0], " -xjvf ", archive->path[1],
						#ifdef __FreeBSD__
								archive->do_overwrite ? " " : " -k",
						#else
								archive->do_overwrite ? " --overwrite" : " --keep-old-files",
						#endif
								archive->do_touch ? " --touch" : "",
								" -C ", archive->extraction_dir, files->str, NULL);
		}
		else
		{
			result = xa_extract_tar_without_directories ( "tar -xjvf ",archive,files->str);
			command = NULL;
		}
		break;

		case XARCHIVETYPE_TAR_GZ:
		if (archive->do_full_path)
		{
			command = g_strconcat(archiver[XARCHIVETYPE_TAR].program[0], " -xzvf ", archive->path[1],
						#ifdef __FreeBSD__
								archive->do_overwrite ? " " : " -k",
						#else
								archive->do_overwrite ? " --overwrite" : " --keep-old-files",
						#endif
								archive->do_touch ? " --touch" : "",
								" -C ", archive->extraction_dir, files->str, NULL);
		}
		else
		{
			result = xa_extract_tar_without_directories ( "tar -xzvf ",archive,files->str);
			command = NULL;
		}
		break;

		case XARCHIVETYPE_TAR_LZMA:
		if (archive->do_full_path)
		{
			command = g_strconcat(archiver[XARCHIVETYPE_TAR].program[0], " --use-compress-program=lzma -xvf ", archive->path[1],
						#ifdef __FreeBSD__
								archive->do_overwrite ? " " : " -k",
						#else
								archive->do_overwrite ? " --overwrite" : " --keep-old-files",
						#endif
								archive->do_touch ? " --touch" : "",
								" -C ", archive->extraction_dir, files->str, NULL);
		}
		else
		{
			result = xa_extract_tar_without_directories ( "tar --use-compress-program=lzma -xvf ",archive,files->str);
			command = NULL;
		}
		break;

		case XARCHIVETYPE_TAR_LZOP:
		if (archive->do_full_path)
		{
			command = g_strconcat(archiver[XARCHIVETYPE_TAR].program[0], " --use-compress-program=lzop -xvf ", archive->path[1],
						#ifdef __FreeBSD__
								archive->do_overwrite ? " " : " -k",
						#else
								archive->do_overwrite ? " --overwrite" : " --keep-old-files",
						#endif
								archive->do_touch ? " --touch" : "",
								" -C ", archive->extraction_dir, files->str, NULL);
		}
		else
		{
			result = xa_extract_tar_without_directories ( "tar --use-compress-program=lzop -xvf ",archive,files->str);
			command = NULL;
		}
		break;

		case XARCHIVETYPE_TAR_XZ:
		if (archive->do_full_path)
		{
			command = g_strconcat(archiver[XARCHIVETYPE_TAR].program[0], " --use-compress-program=xz -xvf ", archive->path[1],
						#ifdef __FreeBSD__
								archive->do_overwrite ? " " : " -k",
						#else
								archive->do_overwrite ? " --overwrite" : " --keep-old-files",
						#endif
								archive->do_touch ? " --touch" : "",
								" -C ", archive->extraction_dir, files->str, NULL);
		}
		else
		{
			result = xa_extract_tar_without_directories ( "tar --use-compress-program=xz -xvf ",archive,files->str);
			command = NULL;
		}
		break;

		default:
		command = NULL;
	}

	if (command != NULL)
	{
		result = xa_run_command(archive, command);
		g_free(command);
	}

	g_string_free(files, TRUE);

	return result;
}

static void xa_add_delete_bzip2_gzip_lzma_compressed_tar (GString *files, XArchive *archive, gboolean add)
{
	gchar *command[5], *executable = NULL, *filename = NULL;
	gboolean result;

	switch (archive->type)
	{
		case XARCHIVETYPE_TAR_BZ2:
			executable = "bzip2 -f ";
			filename = TMPFILE ".bz2";
		break;
		case XARCHIVETYPE_TAR_GZ:
			executable = "gzip -f ";
			filename = TMPFILE ".gz";
		break;
		case XARCHIVETYPE_TAR_LZMA:
			executable = "lzma -f ";
			filename = TMPFILE ".lzma";
		break;
		case XARCHIVETYPE_TAR_XZ:
			executable = "xz -f ";
			filename = TMPFILE ".xz";
		break;
		case XARCHIVETYPE_TAR_LZOP:
			executable = "lzop -f ";
			filename = TMPFILE ".lzo";
		break;

		default:
		break;
	}
	/* Let's copy the archive to /tmp first */
	result = xa_create_working_directory(archive);
	if (!result)
		return;

	/* Let's copy the archive to /tmp first */
	command[0] = g_strconcat ("cp -a ",archive->path[1]," ",archive->working_dir,"/",filename,NULL);

	command[1] = g_strconcat (executable,"-d ",archive->working_dir,"/",filename,NULL);

	if (add)
		command[2] = g_strconcat(archiver[XARCHIVETYPE_TAR].program[0], " ",
							archive->do_recurse ? "" : "--no-recursion ",
							archive->do_move ? "--remove-files " : "",
							archive->do_update ? "-uvvf " : "-rvvf ",
							archive->working_dir,"/" TMPFILE,
							files->str , NULL );
	else
		command[2] = g_strconcat(archiver[XARCHIVETYPE_TAR].program[0], " --no-wildcards --delete -f ", archive->working_dir, "/" TMPFILE, files->str, NULL);

	command[3] = g_strconcat (executable,archive->working_dir,"/" TMPFILE,NULL);

	/* Let's move the modified archive from /tmp to the original archive location */
	command[4] = g_strconcat ("mv ",archive->working_dir,"/",filename," ",archive->path[1],NULL);

	xa_run_command(archive, command[0]);
	g_free(command[0]);

	xa_run_command(archive, command[1]);
	g_free(command[1]);

	xa_run_command(archive, command[2]);
	g_free(command[2]);

	xa_run_command(archive, command[3]);
	g_free(command[3]);

	xa_run_command(archive, command[4]);
	g_free(command[4]);
}

void xa_tar_add (XArchive *archive, GSList *file_list, gchar *compression)
{
	GString *files;
	gchar *command = NULL;

	if (archive->location_path != NULL)
		archive->child_dir = g_strdup(archive->working_dir);

	files = xa_quote_filenames(file_list, NULL, TRUE);

	switch (archive->type)
	{
		case XARCHIVETYPE_TAR:
		if ( g_file_test (archive->path[1],G_FILE_TEST_EXISTS))
			command = g_strconcat(archiver[XARCHIVETYPE_TAR].program[0], " ",
									archive->do_recurse ? "" : "--no-recursion ",
									archive->do_move ? "--remove-files " : "",
									archive->do_update ? "-uvvf " : "-rvvf ",
									archive->path[1],
									files->str , NULL );
		else
			command = g_strconcat(archiver[XARCHIVETYPE_TAR].program[0], " ",
									archive->do_recurse ? "" : "--no-recursion ",
									archive->do_move ? "--remove-files " : "",
									"-cvvf ",archive->path[1],
									files->str , NULL );
		break;

		case XARCHIVETYPE_TAR_BZ2:
		if ( g_file_test ( archive->path[1] , G_FILE_TEST_EXISTS ) )
			xa_add_delete_bzip2_gzip_lzma_compressed_tar (files,archive,1);
		else
			command = g_strconcat(archiver[XARCHIVETYPE_TAR].program[0], " ",
									archive->do_recurse ? "" : "--no-recursion ",
									archive->do_move ? "--remove-files " : "",
									"-cvvjf ",archive->path[1],
									files->str , NULL );
		break;

		case XARCHIVETYPE_TAR_GZ:
		if ( g_file_test ( archive->path[1] , G_FILE_TEST_EXISTS ) )
			xa_add_delete_bzip2_gzip_lzma_compressed_tar (files,archive,1);
		else
			command = g_strconcat(archiver[XARCHIVETYPE_TAR].program[0], " ",
									archive->do_recurse ? "" : "--no-recursion ",
									archive->do_move ? "--remove-files " : "",
									"-cvvzf ",archive->path[1],
									files->str , NULL );
		break;

		case XARCHIVETYPE_TAR_LZMA:
		if ( g_file_test ( archive->path[1] , G_FILE_TEST_EXISTS ) )
			xa_add_delete_bzip2_gzip_lzma_compressed_tar (files,archive,1);
		else
			command = g_strconcat(archiver[XARCHIVETYPE_TAR].program[0], " ",
									archive->do_recurse ? "" : "--no-recursion ",
									archive->do_move ? "--remove-files " : "",
									"--use-compress-program=lzma -cvvf ",archive->path[1],
									files->str , NULL );
		break;

		case XARCHIVETYPE_TAR_XZ:
		if ( g_file_test ( archive->path[1] , G_FILE_TEST_EXISTS ) )
			xa_add_delete_bzip2_gzip_lzma_compressed_tar (files,archive,1);
		else
			command = g_strconcat(archiver[XARCHIVETYPE_TAR].program[0], " ",
									archive->do_recurse ? "" : "--no-recursion ",
									archive->do_move ? "--remove-files " : "",
									"--use-compress-program=xz -cvvf ",archive->path[1],
									files->str , NULL );
		break;

		case XARCHIVETYPE_TAR_LZOP:
		if ( g_file_test ( archive->path[1] , G_FILE_TEST_EXISTS ) )
			xa_add_delete_bzip2_gzip_lzma_compressed_tar (files,archive,1);
		else
			command = g_strconcat(archiver[XARCHIVETYPE_TAR].program[0], " ",
									archive->do_recurse ? "" : "--no-recursion ",
									archive->do_move ? "--remove-files " : "",
									"--use-compress-program=lzop -cvvf ",archive->path[1],
									files->str , NULL );
		break;

		default:
		command = NULL;
	}

	if (command != NULL)
	{
		g_string_free(files,TRUE);
		xa_run_command(archive, command);
		g_free(command);
	}
}

void xa_tar_delete (XArchive *archive, GSList *file_list)
{
	GString *files;
	gchar *command;

	files = xa_quote_filenames(file_list, NULL, TRUE);

	if (archive->type == XARCHIVETYPE_TAR_BZ2 || archive->type == XARCHIVETYPE_TAR_GZ || archive->type == XARCHIVETYPE_TAR_LZMA || archive->type == XARCHIVETYPE_TAR_LZOP || archive->type == XARCHIVETYPE_TAR_XZ)
		xa_add_delete_bzip2_gzip_lzma_compressed_tar(files,archive,0);
	else
	{
		command = g_strconcat(archiver[XARCHIVETYPE_TAR].program[0], " --delete --no-recursion -vf ", archive->path[1], files->str, NULL);
		xa_run_command(archive, command);
		g_free(command);
	}
}
