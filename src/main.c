/*
 *  Copyright (C) 2006 Giuseppe Torelli - <colossus73@gmail.com>
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
#include "main.h"
#include "interface.h"
#include "support.h"
#include "callbacks.h"

extern gchar *extract_path;

gchar *cli_command = NULL;
gboolean error_output,cli;
GError *cli_error = NULL;

gboolean file_to_open;
gboolean ask_and_extract;
gboolean ask_and_add;

static GOptionEntry entries[] =
{
	{ "extract-to=FOLDER", 'x', 0, G_OPTION_ARG_FILENAME, &extract_path, N_("Extract the archive to the specified folder and quits."), NULL },
	{ "extract", 'e', 0, G_OPTION_ARG_NONE, &ask_and_extract, N_("Extract the archive by asking the destination folder and quits."), NULL },
	{ "add-to=ARCHIVE", 'd', 0, G_OPTION_ARG_FILENAME, &path, N_("Add files to the specified archive and quits."), NULL },
	{ "add", 'a', 0, G_OPTION_ARG_NONE, &ask_and_add, N_("Add files asking the name of the archive and quits."), NULL },
	{ NULL }
};

int main (int argc, char **argv)
{
	#ifdef ENABLE_NLS
	bindtextdomain (GETTEXT_PACKAGE, LOCALEDIR);
	bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
	textdomain (GETTEXT_PACKAGE);
	#endif
	gtk_init_with_args(&argc, &argv, _("[archive name]"), entries, PACKAGE, &cli_error);
	if ( cli_error != NULL )
	{
		g_print (_("xarchiver: %s\nTry xarchiver --help to see a full list of available command line options.\n"),cli_error->message);
		g_error_free (cli_error);
		return 0;
	}
	cli = TRUE;
	if (argc > 1)
	{
		gchar *escaped_path = EscapeBadChars (argv[1]);
		if ( ! g_file_test ( escaped_path , G_FILE_TEST_EXISTS ) )
	    {
		    response = ShowGtkMessageDialog (NULL,GTK_DIALOG_MODAL,GTK_MESSAGE_ERROR,GTK_BUTTONS_OK,_("The file doesn't exist!") );
			argc = 1;
	    }
		else
		{
			archive = xa_init_structure (archive);
			archive->path = g_strdup (argv[1]);
			archive->escaped_path = escaped_path;
			archive->type = DetectArchiveType ( archive );
			if ( g_str_has_suffix ( archive->escaped_path , ".tar.bz2") || g_str_has_suffix ( archive->escaped_path , ".tar.bz") || g_str_has_suffix ( archive->escaped_path , ".tbz") || g_str_has_suffix ( archive->escaped_path , ".tbz2" ) )
				archive->type = XARCHIVETYPE_TAR_BZ2;
			else if ( g_str_has_suffix ( archive->escaped_path , ".tar.gz") || g_str_has_suffix ( archive->escaped_path , ".tgz") )
				archive->type = XARCHIVETYPE_TAR_GZ;
		}
	}

	//Switch -x
	if (extract_path != NULL)
	{
		if (archive->has_passwd)
		{
			response = ShowGtkMessageDialog (NULL,GTK_DIALOG_MODAL,GTK_MESSAGE_ERROR,GTK_BUTTONS_OK,_("This switch can't be used with password protected archives.\n") );
		}
		else
		{
			GString *string = g_string_new ( "" );
			archive->full_path = 1;
			cli_command = xa_extract_single_files ( archive , string, extract_path );
			if ( cli_command != NULL )
			{
				error_output = SpawnSyncCommand ( cli_command );
				g_free (cli_command);
			}
			g_string_free (string , FALSE );
		}
		g_free (archive->path);
		g_free (archive->escaped_path);
		g_free (archive);
		return 0;
	}

	//Switch -e
	else if (ask_and_extract)
	{
		extract_window = xa_create_extract_dialog ( 0 , archive);
		gchar *command = xa_parse_extract_dialog_options ( archive , extract_window, NULL );
		gtk_widget_destroy ( extract_window->dialog1 );
		if ( command != NULL )
		{
			error_output = SpawnSyncCommand ( command );
			g_free (command);
		}
		g_free (extract_window);
		return 0;
	}
	//Switch -d
	else if (path != NULL)
	{
		g_message ("%s\n",path);
		return 0;
	}
	//Switch a
	else if (ask_and_add)
	{
		g_message ("Ask and add");
		return 0;
	}
	else
	{
		GetAvailableCompressors();
		ArchiveSuffix = g_list_reverse (ArchiveSuffix);
		ArchiveType = g_list_reverse (ArchiveType);

		Files_to_Add = NULL;
		MainWindow = create_MainWindow ();
		ShowShellOutput (NULL);
		gtk_window_set_position ( GTK_WINDOW (MainWindow),GTK_WIN_POS_CENTER);
		gtk_window_set_default_size (GTK_WINDOW(MainWindow), 600, 400);
		g_signal_connect (MainWindow, "delete_event", G_CALLBACK (xa_quit_application), NULL);
		xa_set_button_state (1,1,0,0,0);
		Update_StatusBar ( _("Ready."));
		gtk_widget_show (MainWindow);
		cli = FALSE;
		//This to open the archive from the command line
		if ( argc == 2)
			xa_open_archive ( NULL , argv[1] );
		gtk_main ();
		g_list_free ( ArchiveSuffix);
		g_list_free ( ArchiveType);
		return 0;
	}
}

//TODO: Support to load the configuration of Xarchiver when extract and add will allow set own archiver's options

//g_get_home_dir ()

void GetAvailableCompressors()
{
	if ( g_find_program_in_path("arj"))
	{
		ArchiveType = g_list_prepend ( ArchiveType, ".arj");
		ArchiveSuffix = g_list_prepend ( ArchiveSuffix, "*.arj");
	}

    if ( g_find_program_in_path("bzip2"))
	{
		ArchiveType = g_list_prepend ( ArchiveType, ".bz2");
		ArchiveSuffix = g_list_prepend ( ArchiveSuffix, "*.bz2");
	}
	
	if ( g_find_program_in_path("gzip"))
	{
		ArchiveType = g_list_prepend ( ArchiveType, ".gz");
		ArchiveSuffix = g_list_prepend ( ArchiveSuffix, "*.gz");
	}

	if ( g_find_program_in_path("mkisofs"))
	{
		ArchiveType = g_list_prepend ( ArchiveType, ".iso");
		ArchiveSuffix = g_list_prepend ( ArchiveSuffix, "*.iso");
	}

    if ( g_find_program_in_path("rar"))
	{
		ArchiveType = g_list_prepend ( ArchiveType, ".rar");
		ArchiveSuffix = g_list_prepend ( ArchiveSuffix, "*.rar");
	}

    if ( g_find_program_in_path("cpio"))
    {
	    ArchiveSuffix = g_list_prepend ( ArchiveSuffix, "*.rpm");
    }

	if ( g_find_program_in_path("tar"))
	{
		ArchiveType = g_list_prepend ( ArchiveType, ".tar");
		ArchiveSuffix = g_list_prepend ( ArchiveSuffix, "*.tar");
		if ( g_list_find ( ArchiveType , ".bz2") )
		{
			ArchiveType = g_list_prepend ( ArchiveType, ".tar.bz2");
			//The following to avoid double filter when opening
			ArchiveSuffix = g_list_prepend ( ArchiveSuffix, "");
		}
		if ( g_list_find ( ArchiveType , ".gz") )
		{
			ArchiveType = g_list_prepend ( ArchiveType, ".tar.gz");
			ArchiveSuffix = g_list_prepend ( ArchiveSuffix, "*.tgz");
		}
	}

    if ( g_find_program_in_path("7za"))
    {
        ArchiveType = g_list_prepend ( ArchiveType, ".7z");
	    ArchiveSuffix = g_list_prepend ( ArchiveSuffix, "*.7z");
    }

    if ( g_find_program_in_path("zip"))
	{
		ArchiveType = g_list_prepend ( ArchiveType, ".jar");
		ArchiveSuffix = g_list_prepend ( ArchiveSuffix, "*.jar");
		
		ArchiveType = g_list_prepend ( ArchiveType, ".zip");
		ArchiveSuffix = g_list_prepend ( ArchiveSuffix, "*.zip");
	}
}

void xa_set_button_state (gboolean New, gboolean Open,gboolean AddFile,gboolean AddFolder,gboolean Extract)
{
	gtk_widget_set_sensitive ( New_button, New);
    gtk_widget_set_sensitive ( new1, New);
	gtk_widget_set_sensitive ( Open_button, Open);
    gtk_widget_set_sensitive ( open1, Open);
	gtk_widget_set_sensitive ( AddFile_button, AddFile);
	gtk_widget_set_sensitive ( addfile, AddFile);
	gtk_widget_set_sensitive ( addfolder, AddFolder);
	gtk_widget_set_sensitive ( AddFolder_button, AddFolder);
	gtk_widget_set_sensitive ( Extract_button, Extract);
	gtk_widget_set_sensitive ( extract_menu, Extract);
}

gboolean SpawnSyncCommand ( gchar *command )
{
    GError *error = NULL;
    gchar *std_out;
    gchar *std_err;
    gint exit_status;
	gchar **argv;
	int argcp;
    
	g_shell_parse_argv ( command , &argcp , &argv , NULL);
	if ( ! g_spawn_sync (
		NULL,
		argv,
		NULL,
		G_SPAWN_SEARCH_PATH,
		NULL,
		NULL, //user data
		&std_out,
		&std_err,
		&exit_status,
		&error) )
	{
		response = ShowGtkMessageDialog (GTK_WINDOW (MainWindow),GTK_DIALOG_MODAL,GTK_MESSAGE_ERROR,GTK_BUTTONS_OK, error->message);
		g_error_free (error);
		g_strfreev ( argv );
        return FALSE;
	}
    if ( WIFEXITED (exit_status) )
	{
	    if ( WEXITSTATUS (exit_status) )
			response = ShowGtkMessageDialog (NULL,GTK_DIALOG_MODAL,GTK_MESSAGE_ERROR,GTK_BUTTONS_OK,std_err );
	}
	g_strfreev ( argv );
    return TRUE;
}

