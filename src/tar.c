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
#include <string.h>
#include "tar.h"

extern void xa_create_liststore ( XArchive *archive, gchar *columns_names[]);
extern gchar *tar;

void xa_open_tar (XArchive *archive)
{
	gchar *command;
	unsigned short int i;

	command = g_strconcat (tar, " tfv " , archive->escaped_path, NULL);
	archive->has_properties = archive->can_add = archive->can_extract = TRUE;
	archive->has_test = archive->has_sfx = FALSE;
	archive->dummy_size = 0;
	archive->nr_of_files = 0;
	archive->nc = 7;
	archive->parse_output = xa_get_tar_line_content;
	archive->format ="TAR";
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

void xa_get_tar_line_content (gchar *line, gpointer data)
{
	XArchive *archive = data;
	XEntry *entry;
	gchar *filename;
	gpointer item[6];
	gint n = 0, a = 0 ,linesize = 0;
	gboolean dir = FALSE;

	linesize = strlen(line);
	archive->nr_of_files++;

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
	archive->dummy_size += strtoll(item[3],NULL,0);
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
		dir = TRUE;
		/* Work around for gtar, which does not output / with directories */
		if(line[linesize-2] != '/')
			filename = g_strconcat(line + n, "/", NULL); 
		else
			filename = g_strdup(line + n); 
	}
	else
		filename = g_strdup(line + n);
	entry = xa_set_archive_entries_for_each_row (archive,filename,item);
	g_free(filename);
}

gboolean isTar (FILE *ptr)
{
	unsigned char magic[7];
	fseek ( ptr, 0 , SEEK_SET );
    if ( fseek ( ptr , 257 , SEEK_CUR) < 0 )
		return FALSE;
    if ( fread ( magic, 1, 7, ptr ) == 0 )
		return FALSE;
    if ( memcmp ( magic,"\x75\x73\x74\x61\x72\x00\x30",7 ) == 0 || memcmp (magic,"\x75\x73\x74\x61\x72\x20\x20",7 ) == 0)
		return TRUE;
    else
		return FALSE;
}

void xa_tar_delete (XArchive *archive,GSList *files)
{
	gchar *e_filename,*command = NULL;
	GSList *_files,*list = NULL;
	GString *names = g_string_new("");
	
	_files = files;
	while (_files)
	{
		e_filename  = xa_escape_filename((gchar*)_files->data,"$'`\"\\!?* ()[]&|:;<>#");
		g_string_prepend (names,e_filename);
		g_string_prepend_c (names,' ');
		_files = _files->next;
	}
	g_slist_foreach(files,(GFunc)g_free,NULL);
	g_slist_free(files);

	if (is_tar_compressed(archive->type))
		xa_add_delete_bzip2_gzip_lzma_compressed_tar(names,archive,0);
	else
	{
		command = g_strconcat (tar, " --delete -vf ",archive->escaped_path," ",names->str,NULL);
		list = g_slist_append(list,command);
		xa_run_command (archive,list);
	}
}

void xa_tar_add (XArchive *archive,GSList *names,gchar *compression_string)
{
	GSList *list = NULL;
	gchar *command = NULL;
 	GString *files = g_string_new("");
 
 	xa_cat_filenames(archive,names,files);
	if (archive->location_entry_path != NULL)
		chdir (archive->tmp);

	switch (archive->type)
	{
		case XARCHIVETYPE_TAR:
		if ( g_file_test ( archive->escaped_path , G_FILE_TEST_EXISTS ) )
			command = g_strconcat (tar, " ",
									archive->add_recurse ? "" : "--no-recursion ",
									archive->remove_files ? "--remove-files " : "",
									archive->update ? "-uvvf " : "-rvvf ",
									archive->escaped_path,
									files->str , NULL );
		else
			command = g_strconcat (tar, " ",
									archive->add_recurse ? "" : "--no-recursion ",
									archive->remove_files ? "--remove-files " : "",
									"-cvvf ",archive->escaped_path,
									files->str , NULL );
		break;

		case XARCHIVETYPE_TAR_BZ2:
		if ( g_file_test ( archive->escaped_path , G_FILE_TEST_EXISTS ) )
			xa_add_delete_bzip2_gzip_lzma_compressed_tar (files,archive,1);
		else
			command = g_strconcat (tar, " ",
									archive->add_recurse ? "" : "--no-recursion ",
									archive->remove_files ? "--remove-files " : "",
									"-cvvjf ",archive->escaped_path,
									files->str , NULL );
		break;

		case XARCHIVETYPE_TAR_GZ:
		if ( g_file_test ( archive->escaped_path , G_FILE_TEST_EXISTS ) )
			xa_add_delete_bzip2_gzip_lzma_compressed_tar (files,archive,1);
		else
			command = g_strconcat (tar, " ",
									archive->add_recurse ? "" : "--no-recursion ",
									archive->remove_files ? "--remove-files " : "",
									"-cvvzf ",archive->escaped_path,
									files->str , NULL );
		break;
		
		case XARCHIVETYPE_TAR_LZMA:
		if ( g_file_test ( archive->escaped_path , G_FILE_TEST_EXISTS ) )
			xa_add_delete_bzip2_gzip_lzma_compressed_tar (files,archive,1);
		else
			command = g_strconcat (tar, " ",
									archive->add_recurse ? "" : "--no-recursion ",
									archive->remove_files ? "--remove-files " : "",
									"--use-compress-program=lzma -cvvf ",archive->escaped_path,
									files->str , NULL );
		break;

		default:
		command = NULL;
	}

	if (command != NULL)
	{
		g_string_free(files,TRUE);
		list = g_slist_append(list,command);
		xa_run_command (archive,list);
	}
}

void xa_tar_extract(XArchive *archive,GSList *files)
{
	gchar *command,*e_filename = NULL;
	GSList *list = NULL,*_files = NULL;
	GString *names = g_string_new("");
	
	_files = files;
	while (_files)
	{
		e_filename = xa_escape_filename((gchar*)_files->data,"$'`\"\\!?* ()[]&|:;<>#");
		g_string_prepend (names,e_filename);
		g_string_prepend_c (names,' ');
		_files = _files->next;
	}
	command = xa_split_command_line(archive,files);
	g_slist_foreach(files,(GFunc)g_free,NULL);
	g_slist_free(files);

	switch (archive->type)
	{
		case XARCHIVETYPE_TAR:
		if (archive->full_path)
		{
			command = g_strconcat (tar, " -xvf " , archive->escaped_path,
								archive->overwrite ? " --overwrite" : " --keep-old-files",
								archive->tar_touch ? " --touch" : "",
								" -C ",archive->extraction_path,names->str,NULL);
		}
		else
		{
			xa_extract_tar_without_directories ( "tar -xvf ",archive,archive->extraction_path,FALSE );
			command = NULL;
		}
		break;

		case XARCHIVETYPE_TAR_BZ2:
		if (archive->full_path)
		{
			command = g_strconcat (tar, " -xjvf " , archive->escaped_path,
								archive->overwrite ? " --overwrite" : " --keep-old-files",
								archive->tar_touch ? " --touch" : "",
								" -C ",archive->extraction_path,names->str,NULL);
		}
		else
		{
			xa_extract_tar_without_directories ( "tar -xjvf ",archive,archive->extraction_path,FALSE);
			command = NULL;
		}
		break;

		case XARCHIVETYPE_TAR_GZ:
		if (archive->full_path == 1)
		{
			command = g_strconcat (tar, " -xzvf " , archive->escaped_path,
								archive->overwrite ? " --overwrite" : " --keep-old-files",
								archive->tar_touch ? " --touch" : "",
								" -C ",archive->extraction_path,names->str,NULL);
		}
		else
		{
			xa_extract_tar_without_directories ( "tar -xzvf ",archive,archive->extraction_path,FALSE);
			command = NULL;
		}
		break;

		case XARCHIVETYPE_TAR_LZMA:
		if (archive->full_path == 1)
		{
			command = g_strconcat (tar, " --use-compress-program=lzma -xvf " , archive->escaped_path,
								archive->overwrite ? " --overwrite" : " --keep-old-files",
								archive->tar_touch ? " --touch" : "",
								" -C ",archive->extraction_path,names->str,NULL);
		}
		else
		{
			xa_extract_tar_without_directories ( "tar --use-compress-program=lzma -xvf ",archive,archive->extraction_path,FALSE);
			command = NULL;
		}
		break;

		default:
		command = NULL;
	}
	if (command != NULL)
	{
		g_string_free(names,TRUE);
		list = g_slist_append(list,command);
		xa_run_command (archive,list);
	}
}

void xa_add_delete_bzip2_gzip_lzma_compressed_tar (GString *files,XArchive *archive,gboolean add)
{
	gchar *command,*executable = NULL,*filename = NULL;
	gboolean result;
	GSList *list = NULL;
	
	switch (archive->type)
	{
		case XARCHIVETYPE_TAR_BZ2:
			executable = "bzip2 -f ";
			filename = "dummy.bz2";
		break;
		case XARCHIVETYPE_TAR_GZ:
			executable = "gzip -f ";
			filename = "dummy.gz";
		break;
		case XARCHIVETYPE_TAR_LZMA:
			executable = "lzma -f ";
			filename = "dummy.lzma";
		break;
		
		default:
		break;
	}
	/* Let's copy the archive to /tmp first */
	result = xa_create_temp_directory(archive);
	if (!result)
		return;

	/* Let's copy the archive to /tmp first */
	command = g_strconcat ("cp -a ",archive->escaped_path," ",archive->tmp,"/",filename,NULL);
	list = g_slist_append(list,command);

	command = g_strconcat (executable,"-d ",archive->tmp,"/",filename,NULL);
	list = g_slist_append(list,command);

	if (add)
		command = g_strconcat (tar, " ",
							archive->add_recurse ? "" : "--no-recursion ",
							archive->remove_files ? "--remove-files " : "",
							archive->update ? "-uvvf " : "-rvvf ",
							archive->tmp,"/dummy",
							files->str , NULL );
	else
		command = g_strconcat (tar," --no-wildcards --delete -f ",archive->tmp,"/dummy ",files->str,NULL);
	list = g_slist_append(list,command);

	command = g_strconcat (executable,archive->tmp,"/dummy",NULL);
	list = g_slist_append(list,command);

	/* Let's move the modified archive from /tmp to the original archive location */
	command = g_strconcat ("mv ",archive->tmp,"/",filename," ",archive->escaped_path,NULL);
	list = g_slist_append(list,command);
	xa_run_command (archive,list);
}

gboolean is_tar_compressed (gint type)
{
	return (type == XARCHIVETYPE_TAR_BZ2 || type == XARCHIVETYPE_TAR_GZ || type == XARCHIVETYPE_TAR_LZMA);
}

void xa_extract_tar_without_directories (gchar *string,XArchive *archive,gchar *extract_path,gboolean cpio_flag)
{
	XEntry *entry;
	gchar *command = NULL;
	GtkTreeSelection *selection;
	GtkTreeIter iter;
	GList *row_list;
	GSList *list = NULL,*files = NULL;
	gboolean result;
	GString *names = g_string_new("");

	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(archive->treeview));
	row_list = gtk_tree_selection_get_selected_rows(selection, &archive->model);

	if (row_list != NULL)
	{
		/* Here we need to fill a GSList with only the selected entries in the archive */
		while (row_list)
		{
			gtk_tree_model_get_iter(archive->model, &iter,row_list->data);
			gtk_tree_model_get (archive->model,&iter,archive->nc+1,&entry,-1);
			gtk_tree_path_free (row_list->data);
			if (entry->is_dir)
				xa_fill_list_with_recursed_entries(entry, &files,"");
			else
				files = g_slist_prepend(files,xa_build_full_path_name_from_entry(entry));
			row_list = row_list->next;
		}
		g_list_free (row_list);
	}
	else
	{
		/* Here we need to fill a GSList with all the entries in the archive */
		XEntry *entry = archive->root_entry;
		while(entry)
		{
			xa_entries_to_filelist(entry, &files,"");
			entry = entry->next;
		}
	}
	result = xa_create_temp_directory (archive);
	if (!result)
	{
		g_slist_foreach(files,(GFunc)g_free,NULL);
		g_slist_free(files);
		return;
	}
	xa_cat_filenames(archive,files,names);
	if (cpio_flag)
	{
		chdir (archive->tmp);
		command = g_strconcat ("cpio --make-directories -F ",archive->tmp," -i",NULL);
	}
	else
		command = g_strconcat (string, archive->escaped_path,
										archive->overwrite ? " --overwrite" : " --keep-old-files",
										archive->tar_touch ? " --touch" : "",
										" --no-wildcards -C ",
										archive->tmp," ",names->str,NULL);
	list = g_slist_append(list,command);

	if (extract_path == NULL)
		extract_path = archive->tmp;

	chdir (archive->tmp);
	command = g_strconcat ("mv -f ",names->str," ",extract_path,NULL);
	g_string_free(names,TRUE);

	list = g_slist_append(list,command);
	xa_run_command (archive,list);
}
