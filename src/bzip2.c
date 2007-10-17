/*
 *  Copyright (C) 2007 Giuseppe Torelli <colossus73@gmail.com>
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

extern void xa_create_liststore ( XArchive *archive, gchar *columns_names[]);
extern gboolean xa_tar_open (GIOChannel *ioc, GIOCondition cond, gpointer data);
extern gboolean cli;

short int l;

void xa_open_bzip2 (XArchive *archive)
{
	XEntry *entry;
	struct stat my_stat;
	gchar tmp_dir[14] = "";
	gchar *compressed = NULL;
	gchar *size = NULL;
	gchar *command = NULL;
	gchar *filename = NULL;;
	gchar *_filename;
	gchar *tar;
	gpointer item[2];
	unsigned short int i;
	gboolean result;

	if (g_str_has_suffix(archive->escaped_path,".tar.bz2") || g_str_has_suffix (archive->escaped_path,".tar.bz")
    	|| g_str_has_suffix ( archive->escaped_path , ".tbz") || g_str_has_suffix (archive->escaped_path,".tbz2") )
	{
		archive->type = XARCHIVETYPE_TAR_BZ2;
		tar = g_find_program_in_path ("gtar");
		if (tar == NULL)
			tar = g_strdup ("tar");

		command = g_strconcat (tar, " tfjv ",archive->escaped_path,NULL);
		archive->has_properties = archive->can_add = archive->can_extract = TRUE;
		archive->has_test = archive->has_sfx = FALSE;
		archive->dummy_size = 0;
		archive->nr_of_files = 0;
		archive->nr_of_dirs = 0;
		archive->format = "TAR.BZIP2";
		archive->nc = 7;
		archive->parse_output = xa_get_tar_line_content;
		xa_spawn_async_process (archive,command,0);

		g_free (command);
		g_free (tar);

		if (archive->child_pid == 0)
			return;

		GType types[]= {GDK_TYPE_PIXBUF,G_TYPE_STRING,G_TYPE_STRING,G_TYPE_STRING,G_TYPE_STRING,G_TYPE_UINT64,G_TYPE_STRING,G_TYPE_STRING,G_TYPE_POINTER};
		archive->column_types = g_malloc0(sizeof(types));
		for (i = 0; i < 9; i++)
			archive->column_types[i] = types[i];

		char *names[]= {(_("Points to")),(_("Permissions")),(_("Owner/Group")),(_("Size")),(_("Date")),(_("Time"),NULL)};
		xa_create_liststore (archive,names);
	}
	else
	{
		archive->can_add = archive->has_test = archive->has_sfx = FALSE;
		archive->has_properties = archive->can_extract = TRUE;
		archive->nc = 2;
		archive->nr_of_files = 1;
		archive->nr_of_dirs = 0;
		archive->format = "BZIP2";

		GType types[]= {G_TYPE_STRING,G_TYPE_STRING,G_TYPE_UINT64,G_TYPE_UINT64};
		archive->column_types = g_malloc0(sizeof(types));
		for (i = 0; i < 4; i++)
			archive->column_types[i] = types[i];

		char *names[]= {(_("Compressed")),(_("Size"))};
		xa_create_liststore (archive,names);
		result = xa_create_temp_directory (tmp_dir);
		if (result == 0)
			return;

		archive->tmp = strdup(tmp_dir);
		/* Let's copy the bzip2 file in the tmp dir */
		command = g_strconcat("cp -f ",archive->escaped_path," ",archive->tmp,NULL);
		result = xa_run_command (archive,command,0);
		g_free (command);
		if (!result)
		{
			xa_delete_temp_directory (archive,0);
			gtk_widget_hide (viewport2);
			Update_StatusBar (_("Operation canceled."));
			return;
		}

		/* Let's get its compressed file size */
		stat (archive->escaped_path,&my_stat);
		compressed = g_strdup_printf("%lld",(unsigned long long int)my_stat.st_size);
		item[0] = compressed;

		/* Let's extract it */
		chdir (archive->tmp);
		_filename = g_strrstr (archive->escaped_path , "/");
		if (_filename)
			command = g_strconcat("bzip2 -f -d ",archive->tmp,_filename,NULL);
		else
			command = g_strconcat("bzip2 -f -d ",archive->tmp,"/",archive->escaped_path,NULL);

		result = xa_run_command (archive,command,1);
		g_free (command);
		if (!result)
		{
			xa_delete_temp_directory (archive,0);
			gtk_widget_hide (viewport2);
			Update_StatusBar (_("Operation canceled."));
			return;
		}

		/* and let's get its uncompressed file size */
		if (_filename)
		{
			_filename++;
			filename = g_strndup(_filename,strlen(_filename)-4);
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
		item[1] = size;

		entry = xa_set_archive_entries_for_each_row (archive,filename,FALSE,item);
		g_free(compressed);
		g_free(size);
		g_free(filename);
		
		xa_update_window_with_archive_entries (archive,NULL);
		gtk_tree_view_set_model (GTK_TREE_VIEW(archive->treeview), archive->model);
		g_object_unref (archive->model);
	}
}

void gzip_bzip2_extract (XArchive *archive , gboolean flag )
{
    gchar *text = NULL;
	gchar *filename_only = NULL;
	gchar *command = NULL;
	gboolean result = FALSE;
	gboolean ext;

	if ( ! cli )
		archive->extraction_path = g_strdup (gtk_entry_get_text ( GTK_ENTRY (extract_window->destination_path_entry) ));

	if ( strlen ( archive->extraction_path ) > 0 )
	{
		if (! cli)
		{
			if (flag)
				text = g_strdup_printf(_("Extracting gzip file to %s"), archive->extraction_path);
			else
				text = g_strdup_printf(_("Extracting bzip2 file to %s"), archive->extraction_path);
			Update_StatusBar ( text );
			g_free (text);
		}

		filename_only = g_strrstr (archive->escaped_path , "/");
		if (file_extension_is (filename_only,".gz") || file_extension_is (filename_only,".bz2") )
			ext = TRUE;
		else
			ext = FALSE;

		if (ext)
			command = g_strconcat ("cp -f ", archive->escaped_path, " /tmp", NULL);
		else
			command = g_strconcat ("cp -f ", archive->escaped_path, " /tmp" , filename_only, flag ? ".gz" : ".bz2", NULL);

		result = xa_run_command (archive,command , 0);
		g_free (command);
		if (result == 0)
			return ;
		if ( ext  )
			command = g_strconcat (flag ? "gzip -f -d -n " : "bzip2 -f -d ", "/tmp",filename_only, NULL);
		else
			command = g_strconcat (flag ? "gzip -f -d -n " : "bzip2 -f -d ","/tmp",filename_only, flag ? ".gz" : ".bz2", NULL);

		result = xa_run_command (archive,command , 0);
		g_free (command);
		if (result == 0)
			return;

		if (ext)
		{
			if (flag)
				filename_only[strlen(filename_only) - 3] = '\0';
			else
				filename_only[strlen(filename_only) - 4] = '\0';
				command = g_strconcat ("mv -f /tmp",filename_only, " ", archive->extraction_path,NULL);
		}
		else
		{
			if ( g_file_test (archive->extraction_path, G_FILE_TEST_IS_DIR) )
				command = g_strconcat ("mv -f /tmp",filename_only, " ", archive->extraction_path,filename_only,NULL);
			else
				command = g_strconcat ("mv -f /tmp",filename_only, " ", archive->extraction_path,NULL);
		}

		result = xa_run_command (archive,command , 0);
		g_free (command);
		if (result == 0)
			return;
	}
	if (result == 0)
	{
		xa_set_button_state (1,1,GTK_WIDGET_IS_SENSITIVE(close1),0,0,0,0,0);
		archive->status = XA_ARCHIVESTATUS_IDLE;
		gtk_widget_set_sensitive (Stop_button, FALSE);
		gtk_widget_hide ( viewport2 );
		Update_StatusBar ( _("Operation canceled."));
	}
	//TODO:
	/*else
		xa_watch_child (archive->child_pid, 0, archive);*/
}

void xa_add_delete_tar_bzip2_gzip ( GString *list , XArchive *archive , gboolean dummy , gboolean add )
{
	gchar *command, *msg, *tar,*temp_name,*file_ext;
	gboolean result;

	if ( ! cli )
	{
		gtk_widget_show (viewport2);
		msg = g_strdup_printf(_("Decompressing tar file with %s, please wait...") , dummy ? "gzip" : "bzip2");
		Update_StatusBar ( msg );
		g_free (msg);
	}

	/* Let's copy the archive to /tmp first */
	
	//TODO: replace /tmp with the user chosen dir in the pref dialog
	temp_name = g_strconcat ( " /tmp", g_strrstr (archive->escaped_path , "/"), NULL);
	command = g_strconcat ("cp -ar " ,archive->escaped_path,temp_name,NULL);
	if ( ! cli)
		result = xa_run_command (archive,command , 0);
	else
		result = SpawnSyncCommand ( command );
	g_free (command);
	if (result == 0)
	{
		g_free (temp_name);
		return;
	}
	command = g_strconcat (dummy ? "gzip -f " : "bzip2 ", "-f -d ",temp_name,NULL);
	if ( ! cli )
		result = xa_run_command (archive,command , 0);
	else
		result = SpawnSyncCommand (command);
	g_free (command);
	if (result == 0)
	{
		g_free (temp_name);
		return;
	}

	tar = g_find_program_in_path ("gtar");
	if (tar == NULL)
		tar = g_strdup ("tar");
	l = strlen (temp_name);

	if (file_extension_is (archive->escaped_path,".tar.bz2") )
		temp_name[l - 4] = 0;
	else if (file_extension_is (archive->escaped_path,".tbz2") )
	{
		temp_name[l - 3] = 'a';
		temp_name[l - 2] = 'r';
		temp_name[l - 1] = 0;
	}
	else if (file_extension_is (archive->escaped_path,".tar.gz") )
		temp_name[l - 3] = 0;

	else if (file_extension_is (archive->escaped_path,".tgz") || file_extension_is (archive->escaped_path, ".tbz") )
	{
		temp_name[l - 2] = 'a';
		temp_name[l - 1] = 'r';
	}

	if ( add )
		command = g_strconcat (tar, " ",
							archive->add_recurse ? "" : "--no-recursion ",
							archive->remove_files ? "--remove-files " : "",
							archive->update ? "-uvvf " : "-rvvf ",
							temp_name,
							list->str , NULL );
	else
		command = g_strconcat (tar, " --delete -f " , temp_name , list->str , NULL );
	if ( ! cli)
		result = xa_run_command (archive,command,0);
	else
		result = SpawnSyncCommand ( command );
	g_free (command);
	g_free (tar);
	if (result == 0)
	{
		g_free (temp_name);
		return;
	}

	if ( ! cli )
	{
		msg = g_strdup_printf(_("Recompressing tar file with %s, please wait...") , dummy ? "gzip" : "bzip2");
		Update_StatusBar ( msg );
		g_free (msg);
	}

	command = g_strconcat ( dummy ? "gzip -f " : "bzip2 ", "-f " , temp_name , NULL );
	if ( ! cli )
		result = xa_run_command (archive,command , 0);
	else
		result = SpawnSyncCommand (command);
	g_free (command);

	if (result == 0)
	{
		g_free (temp_name);
		return;
	}
	if (dummy)
		file_ext = ".gz";
	else
		file_ext = ".bz2";
	/* Let's move the modified archive from /tmp to the original archive location */
	command = g_strconcat ( "mv " , temp_name , file_ext, " " ,archive->escaped_path, NULL );
	if ( ! cli )
		result = xa_run_command (archive,command,1);
	else
		result = SpawnSyncCommand (command);
	g_free (command);
	g_free (temp_name);

	if (result == 0)
		return;
}

