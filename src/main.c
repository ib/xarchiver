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

gint exit_status;
gchar *cli_command = NULL;
gchar *archive_name;
gchar *absolute_path = NULL;
gchar *_current_dir = NULL;
gchar *extract_path = NULL;
GError *cli_error = NULL;
gboolean error_output, file_to_open, ask_and_extract, ask_and_add;
gboolean cli = FALSE;

static GOptionEntry entries[] =
{
	{	"extract-to", 'x', G_OPTION_FLAG_IN_MAIN, G_OPTION_ARG_FILENAME, &extract_path,
		N_("Extract the archive to the specified folder and quits."),
		N_("[destination path]")
	},
	{	"extract", 'e', G_OPTION_FLAG_IN_MAIN, G_OPTION_ARG_NONE, &ask_and_extract,
		N_("Extract the archive by asking the destination folder and quits."),
		N_("[archive path]")
	},
	{	"add-to", 'd', G_OPTION_FLAG_IN_MAIN, G_OPTION_ARG_FILENAME, &archive_name,
		N_("Add files to the specified archive and quits."),
		N_("[archive path] [file1] [file2] ... [fileN]")
	},
	{	"add", 'a', G_OPTION_FLAG_IN_MAIN, G_OPTION_ARG_NONE, &ask_and_add,
		N_("Add files to the specified archive by asking their filenames and quits."),
		N_("[archive name]")
	},
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
	g_get_charset (&locale);
	if ( cli_error != NULL )
	{
		g_print (_("xarchiver: %s\nTry xarchiver --help to see a full list of available command line options.\n"),cli_error->message);
		g_error_free (cli_error);
		return 0;
	}
	if (ask_and_extract || ask_and_add || archive_name != NULL || extract_path != NULL)
		cli = TRUE;

	if (cli == TRUE)
	{
		GetAvailableCompressors();
		ArchiveSuffix = g_list_reverse (ArchiveSuffix);
		ArchiveType = g_list_reverse (ArchiveType);
		MainWindow = create_MainWindow ();
		gtk_main_iteration_do (FALSE);
		g_print ("Xarchiver " VERSION " copyright (C)2005-2006 Giuseppe Torelli (colossus73)\n\n");
		/* Switch -x */
		if (extract_path != NULL)
		{
			if (argv[1] == NULL)
			{
				response = ShowGtkMessageDialog (NULL,GTK_DIALOG_MODAL,GTK_MESSAGE_ERROR,GTK_BUTTONS_OK,_("Can't extract files from the archive:"),_("You missed the archive name!\n"));
				return 0;
			}
			for ( x = 1; x < argc; x++)
			{
				archive = xa_init_structure_from_cmd_line ( argv[x] );
				if (archive != NULL)
				{
					if (archive->has_passwd)
					{
						response = ShowGtkMessageDialog (NULL,GTK_DIALOG_MODAL,GTK_MESSAGE_ERROR,GTK_BUTTONS_OK,_("Can't perform this action:"),_("This option can't be used with password protected archives!\n") );
					}
					else
					{
						GString *string = g_string_new ( "" );
						archive->full_path = 1;
						archive->overwrite = 1;
						gchar *escaped_path = EscapeBadChars (extract_path , 1);
						cli_command = xa_extract_single_files ( archive , string, escaped_path );
						g_free (escaped_path);
						if ( cli_command != NULL )
							error_output = SpawnSyncCommand ( cli_command );
						g_string_free (string, TRUE);
					}
				}
			}
		}

		/* Switch -e */
		else if (ask_and_extract)
		{
			if (argv[1] == NULL)
			{
				response = ShowGtkMessageDialog (NULL,GTK_DIALOG_MODAL,GTK_MESSAGE_ERROR,GTK_BUTTONS_OK,_("Can't extract files from the archive:"),_("You missed the archive name!\n"));
				return 0;
			}
			archive = xa_init_structure_from_cmd_line ( argv[1] );
			if (archive != NULL)
			{
				extract_window = xa_create_extract_dialog ( 0 , archive);
				cli_command = xa_parse_extract_dialog_options ( archive , extract_window, NULL );
				gtk_widget_destroy ( extract_window->dialog1 );
				if ( cli_command != NULL )
					error_output = SpawnSyncCommand ( cli_command );
				g_free (extract_window);
			}
		}
		/* Switch -d */
		else if (archive_name != NULL)
		{
			if (argc == 1)
			{
				response = ShowGtkMessageDialog (NULL,GTK_DIALOG_MODAL,GTK_MESSAGE_ERROR,GTK_BUTTONS_OK,_("Can't add files to the archive:"),_("You missed the files to add!\n"));
				return 0;
			}
			if ( g_file_test ( archive_name , G_FILE_TEST_EXISTS ) )
			{
				archive = xa_init_structure_from_cmd_line ( archive_name );
				if (archive != NULL)
				{
					GString *string = g_string_new ( "" );
					for ( x = 1; x < argc; x++)
					{
						_current_dir = g_path_get_basename ( argv[x] );
						ConcatenateFileNames2 ( _current_dir, string );
						g_free (_current_dir);
					}
					cli_command = xa_add_single_files ( archive , string, NULL);
					if (cli_command != NULL)
						error_output = SpawnSyncCommand ( cli_command );
					g_string_free (string, TRUE);
				}
			}
			else
			{
				xa_new_archive ( NULL , archive_name );
				if (archive->path != NULL)
				{
					_current_dir = g_path_get_dirname(argv[1]);
					chdir (_current_dir);
					g_free (_current_dir);
					GString *string = g_string_new ( "" );
					if (argc > 2 && (archive->type == XARCHIVETYPE_BZIP2 || archive->type == XARCHIVETYPE_GZIP) )
					{
						response = ShowGtkMessageDialog (NULL,GTK_DIALOG_MODAL,GTK_MESSAGE_ERROR,GTK_BUTTONS_OK,_("Can't perform the action:"),_("bzip2/gzip can't compress more than one file!\n") );
						if (archive != NULL)
							xa_clean_archive_structure ( archive );
						return 0;
					}
					for ( x = 1; x < argc; x++)
					{
						_current_dir = g_path_get_basename (argv[x]);
						ConcatenateFileNames2 ( _current_dir , string );
						g_free (_current_dir);
					}
				
					archive->add_recurse = TRUE;
					cli_command = xa_add_single_files ( archive , string, NULL);
					if (cli_command != NULL)
						error_output = SpawnSyncCommand ( cli_command );
					g_string_free (string, TRUE);
				}
				if (cli_command != NULL)
					g_free (cli_command);
			}
		}
		/* Switch -a */
		else if (ask_and_add)
		{
			if (argv[1] == NULL)
			{
				response = ShowGtkMessageDialog (NULL,GTK_DIALOG_MODAL,GTK_MESSAGE_ERROR,GTK_BUTTONS_OK,_("Can't add files to the archive:"),_("You missed the archive name!\n"));
				return 0;
			}
			archive = xa_init_structure_from_cmd_line ( argv[1] );
			if (archive != NULL)
			{
				add_window = xa_create_add_dialog (archive);
				cli_command = xa_parse_add_dialog_options ( archive, add_window );
				gtk_widget_destroy ( add_window->dialog1 );
				if (cli_command != NULL)
					error_output = SpawnSyncCommand ( cli_command );
				g_free (add_window);
			}
		}
		g_list_free ( ArchiveSuffix);
		g_list_free ( ArchiveType);
		if (archive != NULL)
			xa_clean_archive_structure ( archive );
		return exit_status;
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
		Update_StatusBar ( _("Ready."));
		gtk_widget_show (MainWindow);
		archive = xa_init_archive_structure(archive);
		/* This to open the archive from the command line */
		if ( argc == 2)
		{
			gchar *dummy = g_strdup(argv[1]);
			xa_open_archive ( NULL , dummy );
		}
		gtk_main ();
		g_list_free ( ArchiveSuffix);
		g_list_free ( ArchiveType);
		return 0;
	}
}

void GetAvailableCompressors()
{
	absolute_path = g_find_program_in_path("bzip2");
    if ( absolute_path )
	{
		ArchiveType = g_list_prepend ( ArchiveType, ".bz2");
		ArchiveSuffix = g_list_prepend ( ArchiveSuffix, "*.bz2");
		g_free (absolute_path);
	}

	absolute_path = g_find_program_in_path("gzip");
	if ( absolute_path )
	{
		ArchiveType = g_list_prepend ( ArchiveType, ".gz");
		ArchiveSuffix = g_list_prepend ( ArchiveSuffix, "*.gz");
		g_free (absolute_path);
	}

	/* In future releases of xarchiver we'll use mkisofs to allow creation of iso images
	if ( g_find_program_in_path("mkisofs"))
		Allow creation of ISO images
	*/

	ArchiveType = g_list_prepend ( ArchiveType, ".iso");
	ArchiveSuffix = g_list_prepend ( ArchiveSuffix, "*.iso");

	absolute_path = g_find_program_in_path("arj");
	if ( absolute_path )
	{
		ArchiveType = g_list_prepend ( ArchiveType, ".arj");
		ArchiveSuffix = g_list_prepend ( ArchiveSuffix, "*.arj");
		g_free (absolute_path);
	}

	absolute_path = g_find_program_in_path("cpio");
    if ( absolute_path )
	{
	    ArchiveSuffix = g_list_prepend ( ArchiveSuffix, "*.rpm");
		g_free (absolute_path);
	}

	absolute_path = g_find_program_in_path("tar");
	if ( absolute_path )
	{
		ArchiveType = g_list_prepend ( ArchiveType, ".tar");
		ArchiveSuffix = g_list_prepend ( ArchiveSuffix, "*.tar");
		g_free (absolute_path);
		if ( g_list_find ( ArchiveType , ".bz2") )
		{
			ArchiveType = g_list_prepend ( ArchiveType, ".tar.bz2");
			/* The following to avoid double filter when opening */
			ArchiveSuffix = g_list_prepend ( ArchiveSuffix, "");
		}
		if ( g_list_find ( ArchiveType , ".gz") )
		{
			ArchiveType = g_list_prepend ( ArchiveType, ".tar.gz");
			ArchiveSuffix = g_list_prepend ( ArchiveSuffix, "*.tgz");
		}
	}

	absolute_path = g_find_program_in_path("rar");
    if ( absolute_path )
	{
		ArchiveType = g_list_prepend ( ArchiveType, ".rar");
		ArchiveSuffix = g_list_prepend ( ArchiveSuffix, "*.rar");
		g_free (absolute_path);
	}

	absolute_path = g_find_program_in_path("zip");
    if ( absolute_path )
	{
		ArchiveType = g_list_prepend ( ArchiveType, ".jar");
		ArchiveSuffix = g_list_prepend ( ArchiveSuffix, "*.jar");
		g_free (absolute_path);
		
		ArchiveType = g_list_prepend ( ArchiveType, ".zip");
		ArchiveSuffix = g_list_prepend ( ArchiveSuffix, "*.zip");
	}

	absolute_path = g_find_program_in_path("7za");
    if ( absolute_path )
    {
        ArchiveType = g_list_prepend ( ArchiveType, ".7z");
	    ArchiveSuffix = g_list_prepend ( ArchiveSuffix, "*.7z");
		g_free (absolute_path);
    }
}

void xa_set_button_state (gboolean New, gboolean Open,gboolean AddFile,gboolean Extract, gboolean select)
{
	gtk_widget_set_sensitive ( New_button, New);
    gtk_widget_set_sensitive ( new1, New);
	gtk_widget_set_sensitive ( Open_button, Open);
    gtk_widget_set_sensitive ( open1, Open);
	gtk_widget_set_sensitive ( AddFile_button, AddFile);
	gtk_widget_set_sensitive ( addfile, AddFile);
	gtk_widget_set_sensitive ( Extract_button, Extract);
	gtk_widget_set_sensitive ( extract_menu, Extract);
	gtk_widget_set_sensitive ( select_all, select);
}

gboolean SpawnSyncCommand ( gchar *command )
{
    GError *error = NULL;
    gchar *std_out;
    gchar *std_err;
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
		response = ShowGtkMessageDialog (GTK_WINDOW (MainWindow),GTK_DIALOG_MODAL,GTK_MESSAGE_ERROR,GTK_BUTTONS_OK, _("Can't spawn the command:"),error->message);
		g_error_free (error);
		g_strfreev ( argv );
        return FALSE;
	}
    if ( WIFEXITED (exit_status) )
	{
	    if ( WEXITSTATUS (exit_status) )
		{
			response = ShowGtkMessageDialog (NULL,GTK_DIALOG_MODAL,GTK_MESSAGE_ERROR,GTK_BUTTONS_OK,_("An error occurred!"),std_err );
			return FALSE;
		}
	}
	g_strfreev ( argv );
    return TRUE;
}

XArchive *xa_init_structure_from_cmd_line (char *filename)
{
	archive = xa_init_archive_structure (archive);
	archive->path = g_strdup (filename);
	archive->escaped_path = EscapeBadChars(filename , 0);
	archive->type = DetectArchiveType ( archive->path );
	if (archive->type == -2)
		return NULL;
	if ( g_str_has_suffix ( archive->escaped_path , ".tar.bz2") || g_str_has_suffix ( archive->escaped_path , ".tar.bz") || g_str_has_suffix ( archive->escaped_path , ".tbz") || g_str_has_suffix ( archive->escaped_path , ".tbz2" ) )
		archive->type = XARCHIVETYPE_TAR_BZ2;
	else if ( g_str_has_suffix ( archive->escaped_path , ".tar.gz") || g_str_has_suffix ( archive->escaped_path , ".tgz") )
		archive->type = XARCHIVETYPE_TAR_GZ;
	return (archive);
}

