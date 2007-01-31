/*
 * Copyright (C) 2006 Giuseppe Torelli - <colossus73@gmail.com>
 * Copyright (C) 2007 Thomas Dy - <dysprosium66@gmail.com>
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

//this file is modified from bzip2 module

#include "config.h"
#include "lzma.h"
#include "bzip2.h"
#include "extract_dialog.h"
#include "string_utils.h"

FILE *fd;

extern gboolean TarOpen (GIOChannel *ioc, GIOCondition cond, gpointer data);
extern int output_fd;
extern gboolean cli;

short int l;

void OpenLzma ( XArchive *archive )
{
	gchar *command;

	if ( g_str_has_suffix ( archive->escaped_path , ".tar.lzma") || g_str_has_suffix ( archive->escaped_path , ".tlz") )
	{
    gchar *tar;

    tar = g_find_program_in_path ("gtar");
    if (tar == NULL)
      tar = g_strdup ("tar");

		command = g_strconcat (tar, " tv --use-compress-program=lzma -f " , archive->escaped_path, NULL );
		archive->dummy_size = 0;
		archive->nr_of_files = 0;
		archive->nr_of_dirs = 0;
		archive->format ="TAR.LZMA";
		archive->parse_output = TarOpen;

		xa_spawn_async_process (archive,command,0);

		g_free (command);
		g_free (tar);

		if ( archive->child_pid == 0 )
			return;

		char *names[]= {(_("Filename")),(_("Permissions")),(_("Symbolic Link")),(_("Owner/Group")),(_("Size")),(_("Date")),(_("Time"))};
		GType types[]= {G_TYPE_STRING,G_TYPE_STRING,G_TYPE_STRING,G_TYPE_STRING,G_TYPE_UINT64,G_TYPE_STRING,G_TYPE_STRING};
//		xa_create_liststore ( 7, names , (GType *)types, archive );
        archive->type = XARCHIVETYPE_TAR_LZMA;
	}
	else
	{
		extract_window = xa_create_extract_dialog ( 0 , archive);
		command = xa_parse_extract_dialog_options ( archive , extract_window, NULL );
		gtk_widget_destroy ( extract_window->dialog1 );
		g_free (extract_window);
	}
}


void lzma_extract ( XArchive *archive )
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
			text = g_strdup_printf(_("Extracting lzma file to %s"), archive->extraction_path);
			Update_StatusBar ( text );
			g_free (text);
		}

		filename_only = g_strrstr (archive->escaped_path , "/");
		if (file_extension_is (filename_only,".lzma") )
			ext = TRUE;
		else
			ext = FALSE;

		if (ext)
			command = g_strconcat ("cp -f ", archive->escaped_path, " /tmp", NULL);
		else
			command = g_strconcat ("cp -f ", archive->escaped_path, " /tmp" , filename_only, ".lzma", NULL);

		result = xa_run_command (command , 0);
		g_free (command);
		if (result == 0)
			return ;
		if ( ext  )
			command = g_strconcat ("lzma -f -d ", "/tmp",filename_only, NULL);
		else
			command = g_strconcat ("lzma -f -d ","/tmp",filename_only, ".lzma", NULL);

		result = xa_run_command (command , 0);
		g_free (command);
		if (result == 0)
			return;

		if (ext)
		{
			filename_only[strlen(filename_only) - 5] = '\0';
			command = g_strconcat ("mv -f /tmp",filename_only, " ", archive->extraction_path,NULL);
		}
		else
		{
			command = g_strconcat ("mv -f /tmp",filename_only, " ", archive->extraction_path,NULL);
		}

		result = xa_run_command (command , 0);
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

void xa_add_delete_tar_lzma ( GString *list , XArchive *archive , gboolean add )
{
	gchar *command, *msg, *tar,*temp_name,*file_ext;
	gboolean result;

	if ( ! cli )
	{
		gtk_widget_show (viewport2);
		msg = g_strdup_printf(_("Decompressing tar file with %s, please wait...") , "lzma");
		Update_StatusBar ( msg );
		g_free (msg);
	}

	/* Let's copy the archive to /tmp first */
	temp_name = g_strconcat ( " /tmp", g_strrstr (archive->escaped_path , "/"), NULL);
	command = g_strconcat ("cp -ar " ,archive->escaped_path,temp_name,NULL);
	if ( ! cli)
		result = xa_run_command (command , 0);
	else
		result = SpawnSyncCommand ( command );
	g_free (command);
	if (result == 0)
	{
		g_free (temp_name);
		return;
	}
	command = g_strconcat ("lzma ", "-f -d ",temp_name,NULL);
	if ( ! cli )
		result = xa_run_command (command , 0);
	else
		result = SpawnSyncCommand ( command );
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

	if (file_extension_is (archive->escaped_path,".tar.lzma") )
		temp_name[l - 5] = 0;
	else if (file_extension_is (archive->escaped_path,".tlz") )
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
		result = xa_run_command (command , 0);
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
		msg = g_strdup_printf(_("Recompressing tar file with %s, please wait...") , "lzma");
		Update_StatusBar ( msg );
		g_free (msg);
	}

	command = g_strconcat ( "lzma ", "-f " , temp_name , NULL );
	if ( ! cli )
		result = xa_run_command (command , 0);
	else
		result = SpawnSyncCommand ( command );
	g_free (command);

	if (result == 0)
	{
		g_free (temp_name);
		return;
	}
	file_ext = ".lzma";

	/* Let's move the modified archive from /tmp to the original archive location */
	command = g_strconcat ( "mv " , temp_name , file_ext, " " ,archive->escaped_path, NULL );
	if ( ! cli )
		result = xa_run_command (command , 1);
	else
		result = SpawnSyncCommand ( command );
	g_free (command);
	g_free (temp_name);

	if (result == 0)
		return;
}

