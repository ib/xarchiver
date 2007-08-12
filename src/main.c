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
#include "string_utils.h"

#ifdef HAVE_SOCKET
#include "socket.h"
#endif

gint exit_status;
gchar *cli_command = NULL;
gchar *absolute_path = NULL;
gchar *archive_name = NULL;
gchar *_current_dir = NULL;
gchar *extract_path = NULL;
GError *cli_error = NULL;
gboolean error_output, file_to_open, ask_and_extract, ask_and_add;
gboolean cli = FALSE;
gboolean unrar = FALSE;
extern gchar *current_open_directory;
Prefs_dialog_data *prefs_window = NULL;

static GOptionEntry entries[] =
{
	{	"extract-to", 'x', G_OPTION_FLAG_IN_MAIN, G_OPTION_ARG_FILENAME, &extract_path,
		N_("Extract archive to the directory specified by destination_path and quits."),
		N_("destination_path archive")
	},
	{	"extract", 'e', G_OPTION_FLAG_IN_MAIN, G_OPTION_ARG_NONE, &ask_and_extract,
		N_("Extract archive by asking the destination directory and quits."),
		N_("archive")
	},
	{	"add-to", 'd', G_OPTION_FLAG_IN_MAIN, G_OPTION_ARG_FILENAME, &archive_name,
		N_("Add the given files by asking the name of the archive and quits."),
		N_("file1 file2 file3 ... fileN")
	},
	{	"add", 'a', G_OPTION_FLAG_IN_MAIN, G_OPTION_ARG_NONE, &ask_and_add,
		N_("Add files to archive by asking their filenames and quits."),
		N_("archive")
	},
	{ NULL }
};

int main (int argc, char **argv)
{
	unsigned short int x;
	#ifdef ENABLE_NLS
	bindtextdomain (GETTEXT_PACKAGE, LOCALEDIR);
	bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
	textdomain (GETTEXT_PACKAGE);
	#endif

#ifdef HAVE_SOCKET
	socket_info.lock_socket = -1;
	socket_info.lock_socket_tag = 0;
	socket_info.lock_socket = socket_init(argc, argv);

	if (socket_info.lock_socket < 0)
	{
		/* Socket exists; filenames were sent to first instance, so quit */
		if (argc > 1)
			return 0;
	}
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
		g_print ("Xarchiver " VERSION " (\xC2\xA9)2005-2007 Giuseppe Torelli (colossus73)\n\n");

		/* Switch -x */
		if (extract_path != NULL)
		{
			if (argv[1] == NULL)
			{
				response = xa_show_message_dialog (NULL,GTK_DIALOG_MODAL,GTK_MESSAGE_ERROR,GTK_BUTTONS_OK,_("Can't extract files from the archive:"),_("You missed the archive name!\n"));
				return 0;
			}
			for ( x = 1; x < argc; x++)
			{
				archive_cmd = xa_init_structure_from_cmd_line ( argv[x] );
				if (archive_cmd != NULL)
				{
					if (archive_cmd->has_passwd)
					{
						archive_cmd->passwd = password_dialog (archive_cmd);
						if (archive_cmd->passwd == NULL)
							goto done;
					}
					GString *string = g_string_new ( "" );
					archive_cmd->full_path = 1;
					archive_cmd->overwrite = 1;
					gchar *escaped_path = EscapeBadChars (extract_path , "$\'`\"\\!?* ()[]&|@#:;");
					archive_cmd->extraction_path = g_strdup (extract_path);
					cli_command = xa_extract_single_files ( archive_cmd , string, escaped_path );
					g_free (escaped_path);
					if ( cli_command != NULL )
						error_output = SpawnSyncCommand ( cli_command );
					g_string_free (string, TRUE);
				}
			}
		}

		/* Switch -e */
		else if (ask_and_extract)
		{
			if (argv[1] == NULL)
			{
				response = xa_show_message_dialog (NULL,GTK_DIALOG_MODAL,GTK_MESSAGE_ERROR,GTK_BUTTONS_OK,_("Can't extract files from the archive:"),_("You missed the archive name!\n"));
				return 0;
			}
			archive_cmd = xa_init_structure_from_cmd_line ( argv[1] );
			if (archive_cmd != NULL)
			{
				extract_window = xa_create_extract_dialog ( 0 , archive_cmd);
				cli_command = xa_parse_extract_dialog_options ( archive_cmd , extract_window, NULL );
				gtk_widget_destroy ( extract_window->dialog1 );
				if ( cli_command != NULL )
					error_output = SpawnSyncCommand ( cli_command );
				g_free (extract_window);
			}
		}
		/* Switch -d */
		else if (archive_name != NULL)
		{
			XArchive *archive_cmd = NULL;
			archive_cmd = xa_new_archive_dialog (archive_name, NULL );
			if (archive_cmd == NULL)
				return 0;

			if (archive_cmd->path != NULL)
			{
				_current_dir = g_path_get_dirname(archive_name);
				chdir (_current_dir);
				g_free (_current_dir);
				GString *string = g_string_new ( "" );

				if ( g_file_test ( archive_name,G_FILE_TEST_EXISTS) )
				{
					_current_dir = g_path_get_basename ( archive_name );
					ConcatenateFileNames2 ( _current_dir, string );
					g_free (_current_dir);
				}

				for (x = 1; x< argc; x++)
				{
					_current_dir = g_path_get_basename ( argv[x] );
					ConcatenateFileNames2 ( _current_dir, string );
					g_free (_current_dir);
				}
				if ( archive_cmd->type == XARCHIVETYPE_7ZIP)
					archive_cmd->add_recurse = FALSE;
				else
					archive_cmd->add_recurse = TRUE;
				cli_command = xa_add_single_files ( archive_cmd , string, NULL);
				if (cli_command != NULL)
					error_output = SpawnSyncCommand ( cli_command );
				g_string_free (string, TRUE);
			}
			if (cli_command != NULL)
				g_free (cli_command);
		}
		/* Switch -a */
		else if (ask_and_add)
		{
			if (argv[1] == NULL)
			{
				response = xa_show_message_dialog (NULL,GTK_DIALOG_MODAL,GTK_MESSAGE_ERROR,GTK_BUTTONS_OK,_("Can't add files to the archive:"),_("You missed the archive name!\n"));
				return 0;
			}
			archive_cmd = xa_init_structure_from_cmd_line ( argv[1] );
			if (archive_cmd != NULL)
			{
				add_window = xa_create_add_dialog (archive_cmd);
				cli_command = xa_parse_add_dialog_options ( archive_cmd, add_window );
				gtk_widget_destroy ( add_window->dialog1 );
				if (cli_command != NULL)
					error_output = SpawnSyncCommand ( cli_command );
				g_free (add_window);
			}
		}
done:	g_list_free ( ArchiveSuffix);
		g_list_free ( ArchiveType);
		if (archive_cmd != NULL)
			xa_clean_archive_structure ( archive_cmd );
		return exit_status;
	}
	else
	{
		GetAvailableCompressors();
		ArchiveSuffix = g_list_reverse (ArchiveSuffix);
		ArchiveType = g_list_reverse (ArchiveType);
		MainWindow = create_MainWindow ();

		prefs_window = xa_create_prefs_dialog();
		xa_prefs_load_options (prefs_window);

		if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(prefs_window->check_save_geometry)) && prefs_window->geometry[0] != -1)
		{
			gtk_window_move (GTK_WINDOW(MainWindow), prefs_window->geometry[0], prefs_window->geometry[1]);
			gtk_window_set_default_size (GTK_WINDOW(MainWindow), prefs_window->geometry[2], prefs_window->geometry[3]);
		}
		else
		{
			gtk_window_set_position (GTK_WINDOW(MainWindow),GTK_WIN_POS_CENTER);
			gtk_window_set_default_size (GTK_WINDOW(MainWindow), 600, 400);
		}
		Update_StatusBar ( _("Ready."));
		gtk_widget_show (MainWindow);

		/* This to open the archive from the command line */
		if ( argc == 2 )
		{
			gchar *dummy = g_strdup(argv[1]);
			current_open_directory = g_path_get_dirname (dummy);
			if (strcmp(current_open_directory,"..") == 0)
			{
				g_free (current_open_directory);
				current_open_directory = g_get_current_dir();
			}
			xa_open_archive ( NULL , dummy );
		}
		#ifdef HAVE_SOCKET
		if (! socket_info.ignore_socket && socket_info.lock_socket > 0)
		{
			socket_info.read_ioc = g_io_channel_unix_new(socket_info.lock_socket);
			socket_info.lock_socket_tag = g_io_add_watch(socket_info.read_ioc,	G_IO_IN|G_IO_PRI|G_IO_ERR, socket_lock_input_cb, MainWindow);
		}
		#endif
		gtk_main ();
		g_list_free ( ArchiveSuffix);
		g_list_free ( ArchiveType);
		return 0;
	}
}

gchar *get_argv_filename(const gchar *filename)
{
	gchar *result;

	if (g_path_is_absolute(filename))
		result = g_strdup(filename);
	else
	{
		//use current dir
		gchar *cur_dir = g_get_current_dir();
		result = g_strjoin(
			G_DIR_SEPARATOR_S, cur_dir, filename, NULL);
		g_free(cur_dir);
	}
	return result;
}

void GetAvailableCompressors()
{
	absolute_path = g_find_program_in_path("arj");
	if ( absolute_path )
	{
		ArchiveType = g_list_prepend ( ArchiveType, "arj");
		ArchiveSuffix = g_list_prepend ( ArchiveSuffix, "*.arj");
		g_free (absolute_path);
	}

	absolute_path = g_find_program_in_path("bzip2");
    if ( absolute_path )
	{
		ArchiveType = g_list_prepend ( ArchiveType, "bz2");
		ArchiveSuffix = g_list_prepend ( ArchiveSuffix, "*.bz2");
		g_free (absolute_path);
	}

	absolute_path = g_find_program_in_path("ar");
    if ( absolute_path )
	{
	    ArchiveSuffix = g_list_prepend ( ArchiveSuffix, "*.deb");
		g_free (absolute_path);
	}

	absolute_path = g_find_program_in_path("gzip");
	if ( absolute_path )
	{
		ArchiveType = g_list_prepend ( ArchiveType, "gz");
		ArchiveSuffix = g_list_prepend ( ArchiveSuffix, "*.gz");
		g_free (absolute_path);
	}

	absolute_path = g_find_program_in_path("lzma");
	if ( absolute_path )
	{
		ArchiveType = g_list_prepend ( ArchiveType, "lzma");
		ArchiveSuffix = g_list_prepend ( ArchiveSuffix, "*.lzma");
		g_free (absolute_path);
	}

	/* In future releases of xarchiver we'll use bkisofs library to allow creation of iso images */
	ArchiveType = g_list_prepend ( ArchiveType, "iso");
	ArchiveSuffix = g_list_prepend ( ArchiveSuffix, "*.iso");

	absolute_path = g_find_program_in_path("lha");
	if (absolute_path)
	{
		ArchiveType = g_list_prepend(ArchiveType, "lzh");
		ArchiveSuffix = g_list_prepend(ArchiveSuffix, "*.lzh");
		g_free (absolute_path);
		//ArchiveType = g_list_prepend(ArchiveType, ".lha");
		//ArchiveSuffix = g_list_prepend(ArchiveSuffix, "");
	}

	absolute_path = g_find_program_in_path ("rar");
    if ( absolute_path )
	{
		ArchiveType = g_list_prepend ( ArchiveType, "rar");
		ArchiveSuffix = g_list_prepend ( ArchiveSuffix, "*.rar");
		g_free (absolute_path);
	}
	else
	{
		absolute_path = g_find_program_in_path ("unrar");
		if ( absolute_path )
		{
			unrar = TRUE;
			ArchiveType = g_list_prepend ( ArchiveType, "rar");
			ArchiveSuffix = g_list_prepend ( ArchiveSuffix, "*.rar");
			g_free (absolute_path);
		}
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
		ArchiveType = g_list_prepend ( ArchiveType, "tar");
		ArchiveSuffix = g_list_prepend ( ArchiveSuffix, "*.tar");
		g_free (absolute_path);
		if ( g_list_find ( ArchiveType , "bz2") )
		{
			ArchiveType = g_list_prepend ( ArchiveType, "tar.bz2");
			/* The following to avoid double filter when opening */
			ArchiveSuffix = g_list_prepend ( ArchiveSuffix, "");
		}
		if ( g_list_find ( ArchiveType , "gz") )
		{
			ArchiveType = g_list_prepend ( ArchiveType, "tar.gz");
			ArchiveSuffix = g_list_prepend ( ArchiveSuffix, "*.tgz");
		}
		if ( g_list_find ( ArchiveType , "lzma") )
		{
			ArchiveType = g_list_prepend ( ArchiveType, "tar.lzma");
			ArchiveSuffix = g_list_prepend ( ArchiveSuffix, "*.tlz");
		}
	}

	absolute_path = g_find_program_in_path ("zip");
    if (absolute_path)
	{
		g_free (absolute_path);
		absolute_path = g_find_program_in_path ("zipinfo");
    	if (absolute_path)
		{
			g_free (absolute_path);
			ArchiveType = g_list_prepend ( ArchiveType, "jar");
			ArchiveSuffix = g_list_prepend ( ArchiveSuffix, "*.jar");

			ArchiveType = g_list_prepend ( ArchiveType, "zip");
			ArchiveSuffix = g_list_prepend ( ArchiveSuffix, "*.zip");
		}
	}
	absolute_path = g_find_program_in_path("7za");
    if (absolute_path == NULL)
    	absolute_path = g_find_program_in_path("7zr");
    if (absolute_path)
    {
        ArchiveType = g_list_prepend ( ArchiveType, "7z");
	    ArchiveSuffix = g_list_prepend ( ArchiveSuffix, "*.7z");
		g_free (absolute_path);
    }
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
		response = xa_show_message_dialog (GTK_WINDOW (MainWindow),GTK_DIALOG_MODAL,GTK_MESSAGE_ERROR,GTK_BUTTONS_OK, _("Can't spawn the command:"),error->message);
		g_error_free (error);
		g_strfreev ( argv );
        return FALSE;
	}
    if ( WIFEXITED (exit_status) )
	{
	    if ( WEXITSTATUS (exit_status) )
		{
			response = xa_show_message_dialog (NULL,GTK_DIALOG_MODAL,GTK_MESSAGE_ERROR,GTK_BUTTONS_OK,_("An error occurred!"),std_err );
			return FALSE;
		}
	}
	g_strfreev ( argv );
    return TRUE;
}

XArchive *xa_init_structure_from_cmd_line (char *filename)
{
	XArchive *archive_cmd;
	XArchiveType type;

	type = xa_detect_archive_type ( filename );
	if (type == -2)
		return NULL;

	archive_cmd = xa_init_archive_structure ();
	if (archive_cmd == NULL)
	{
		response = xa_show_message_dialog (NULL,GTK_DIALOG_MODAL,GTK_MESSAGE_ERROR,GTK_BUTTONS_OK,_("Can't allocate memory for the archive structure!"),"" );
		return NULL;
	}
	archive_cmd->path = g_strdup (filename);
	archive_cmd->escaped_path = EscapeBadChars(filename , "$\'`\"\\!?* ()&|@#:;");
	archive_cmd->type = type;

	if ( g_str_has_suffix ( archive_cmd->escaped_path , ".tar.bz2") || g_str_has_suffix ( archive_cmd->escaped_path , ".tar.bz") || g_str_has_suffix ( archive_cmd->escaped_path , ".tbz") || g_str_has_suffix ( archive_cmd->escaped_path , ".tbz2" ) )
		archive_cmd->type = XARCHIVETYPE_TAR_BZ2;
	else if ( g_str_has_suffix ( archive_cmd->escaped_path , ".tar.gz") || g_str_has_suffix ( archive_cmd->escaped_path , ".tgz") )
		archive_cmd->type = XARCHIVETYPE_TAR_GZ;
	else if ( g_str_has_suffix ( archive_cmd->escaped_path , ".tar.lzma") || g_str_has_suffix ( archive_cmd->escaped_path , ".tlz") )
		archive_cmd->type = XARCHIVETYPE_TAR_LZMA;

	return (archive_cmd);
}

