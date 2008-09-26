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
#include "main.h"
#include "archive.h"
#include "string_utils.h"
#include "mime.h"

#ifdef HAVE_SOCKET
#include "socket.h"
#endif

gchar *absolute_path = NULL;
//gchar *archive_name = NULL;
gchar *_current_dir = NULL;
gchar *extract_path = NULL;
GError *cli_error = NULL;
gboolean error_output, file_to_open, ask_and_extract, add_files,ask_and_add, multi_extract;
gboolean batch_mode = FALSE;
gboolean unrar = FALSE;
gboolean sevenzr = FALSE, sevenza = FALSE, xdg_open = FALSE;
int response;
extern gchar *current_open_directory;
extern int status;
Prefs_dialog_data   *prefs_window   = NULL;
Extract_dialog_data *extract_window = NULL;
Add_dialog_data     *add_window		= NULL;
Multi_extract_data	*multi_extract_window	= NULL;

delete_func		delete[XARCHIVETYPE_COUNT]	= {NULL};
add_func		add[XARCHIVETYPE_COUNT]		= {NULL};
extract_func	extract[XARCHIVETYPE_COUNT]	= {NULL};
test_func		test[XARCHIVETYPE_COUNT]	= {NULL};

static GOptionEntry entries[] =
{
	{	"extract-to", 'x', G_OPTION_FLAG_IN_MAIN, G_OPTION_ARG_FILENAME, &extract_path,
		N_("Extract archive to the destination directory and quits."),
		N_("destination archive")
	},
	{	"extract", 'e', G_OPTION_FLAG_IN_MAIN, G_OPTION_ARG_NONE, &ask_and_extract,
		N_("Extract archive by asking the extraction directory and quits."),
		N_("archive")
	},
	{	"multi-extract", 'm', G_OPTION_FLAG_IN_MAIN, G_OPTION_ARG_NONE, &multi_extract,
		N_("Multi-extract archives"),
		N_("filenames")
	},
	{	"add-to", 'd', G_OPTION_FLAG_IN_MAIN, G_OPTION_ARG_NONE, &add_files,
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
	XArchive *archive = NULL;
	gboolean no_bzip2_gzip;
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
	if (multi_extract || add_files || ask_and_extract || ask_and_add || extract_path != NULL)
		batch_mode = TRUE;

	xa_set_available_archivers();
	prefs_window   = xa_create_prefs_dialog();
	extract_window = xa_create_extract_dialog();
	add_window     = xa_create_add_dialog();
	multi_extract_window = xa_create_multi_extract_dialog();
	xa_prefs_load_options(prefs_window);

	if (batch_mode == TRUE)
	{
		xa_main_window = NULL;
		archive = xa_init_structure_from_cmd_line (argv[1]);
		g_print ("Xarchiver " VERSION " (\xC2\xA9)2005-2008 Giuseppe Torelli\n");

		/* Switch -x */
		if (extract_path != NULL)
		{
			if (argv[1] == NULL)
			{
				response = xa_show_message_dialog (NULL,GTK_DIALOG_MODAL,GTK_MESSAGE_ERROR,GTK_BUTTONS_OK,_("Can't extract files from the archive:"),_("You missed the archive name!\n"));
				return -1;
			}
			if (xa_detect_encrypted_archive (archive))
			{
				archive->has_passwd = TRUE;
				archive->passwd = xa_create_password_dialog(archive);
				if (archive->passwd == NULL)
					goto done;
			}
			for (x = 1; x < argc; x++)
			{
				GSList *string = NULL;
				archive->full_path = 1;
				archive->overwrite = 1;
				gchar *escaped_path = xa_escape_bad_chars (extract_path,"$\'`\"\\!?* ()[]&|@#:;");
				archive->extraction_path = escaped_path;
				(*archive->extract) (archive,string);
			}
		}
		/* Switch -e */
		else if (ask_and_extract && archive != NULL)
		{
			if (argv[1] == NULL)
			{
				response = xa_show_message_dialog (NULL,GTK_DIALOG_MODAL,GTK_MESSAGE_ERROR,GTK_BUTTONS_OK,_("Can't extract files from the archive:"),_("You missed the archive name!\n"));
				return -1;
			}
			if (xa_detect_encrypted_archive (archive))
				archive->has_passwd = TRUE;

			xa_set_extract_dialog_options(extract_window,0,archive);
			xa_parse_extract_dialog_options (archive,extract_window,NULL);
			gtk_widget_destroy (extract_window->dialog1);
			g_free (extract_window);
		}
		/* Switch -m */
		else if (multi_extract)
		{
			Multi_extract_data *multi_extract = NULL;
			multi_extract = xa_create_multi_extract_dialog();
			for (x = 1; x< argc; x++)
				xa_add_files_liststore(argv[x],multi_extract);
			xa_parse_multi_extract_archive(multi_extract);
			gtk_widget_destroy (multi_extract->multi_extract);
			g_free(multi_extract);
		}
		/* Switch -d */
		else if (add_files)
		{
			if (argc > 1 || g_file_test (argv[1],G_FILE_TEST_IS_DIR))
				no_bzip2_gzip = TRUE;
			else
				no_bzip2_gzip = FALSE;
			archive = xa_new_archive_dialog (argv[1],NULL,no_bzip2_gzip);
			if (archive == NULL)
				return -1;

			if (archive->path != NULL)
			{
				xa_create_temp_directory(archive);
				archive->add_recurse = TRUE;
				_current_dir = g_path_get_dirname(argv[1]);
				chdir (_current_dir);
				g_free(_current_dir);
				GSList *files = NULL;
				for (x = 1; x< argc; x++)
				{
					_current_dir = g_path_get_basename(argv[x]);
					files = g_slist_append(files,xa_escape_filename(_current_dir,"$'`\"\\!?* ()[]&|:;<>#"));
					g_free (_current_dir);
				}
				xa_execute_add_commands(archive,files,NULL);
			}
		}
		/* Switch -a */
		else if (ask_and_add)
		{
			if (argv[1] == NULL)
			{
				response = xa_show_message_dialog (NULL,GTK_DIALOG_MODAL,GTK_MESSAGE_ERROR,GTK_BUTTONS_OK,_("Can't add files to the archive:"),_("You missed the archive name!\n"));
				return -1;
			}
			if (archive != NULL)
			{
				xa_set_add_dialog_options(add_window,archive);
				xa_parse_add_dialog_options (archive,add_window);
				gtk_widget_destroy (add_window->dialog1);
				g_free (add_window);
			}
		}
done:	g_list_free (ArchiveSuffix);
		g_list_free (ArchiveType);
		if (archive != NULL)
			xa_clean_archive_structure (archive);
		return WIFEXITED (status);
	}
	else
	{
		xa_main_window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
		xa_create_main_window (xa_main_window,	gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (prefs_window->show_location_bar)),
												gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (prefs_window->store_output)),
												gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (prefs_window->show_sidebar)));

		gtk_window_set_transient_for (GTK_WINDOW (extract_window->dialog1),GTK_WINDOW (xa_main_window));
		gtk_window_set_transient_for (GTK_WINDOW (add_window->dialog1),GTK_WINDOW (xa_main_window));
		gtk_window_set_transient_for (GTK_WINDOW (prefs_window->dialog1),GTK_WINDOW (xa_main_window));
		if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(prefs_window->check_save_geometry)) && prefs_window->geometry[0] != -1)
		{
			gtk_window_move (GTK_WINDOW(xa_main_window), prefs_window->geometry[0], prefs_window->geometry[1]);
			gtk_window_set_default_size (GTK_WINDOW(xa_main_window), prefs_window->geometry[2], prefs_window->geometry[3]);
			gtk_paned_set_position (GTK_PANED (hpaned1),prefs_window->geometry[4]);
		}
		else
		{
			gtk_window_set_position (GTK_WINDOW(xa_main_window),GTK_WIN_POS_CENTER);
			gtk_window_set_default_size (GTK_WINDOW(xa_main_window), 600, 400);
			gtk_paned_set_position (GTK_PANED (hpaned1),200);
		}
		gtk_label_set_text(GTK_LABEL(total_label),_("Select \"New\" to create or \"Open\" to open an archive"));
		gtk_widget_show (xa_main_window);

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
			xa_open_archive (NULL,dummy);
		}
		#ifdef HAVE_SOCKET
		if (! socket_info.ignore_socket && socket_info.lock_socket > 0)
		{
			socket_info.read_ioc = g_io_channel_unix_new(socket_info.lock_socket);
			socket_info.lock_socket_tag = g_io_add_watch(socket_info.read_ioc,	G_IO_IN|G_IO_PRI|G_IO_ERR, socket_lock_input_cb, xa_main_window);
		}
		#endif
		gtk_main ();
		g_list_free (ArchiveSuffix);
		g_list_free (ArchiveType);
		return 0;
	}
}

void xa_set_available_archivers()
{
	delete[0]  = 0;
	delete[1]  = &xa_7zip_delete;
	delete[2]  = &xa_arj_delete;
	delete[3]  = 0;
	delete[4]  = 0;
	delete[5]  = 0;
	delete[6]  = 0;
	delete[7]  = &xa_rar_delete;
	delete[8]  = 0;
	delete[9]  = delete[10] = delete[11] = delete[12] = &xa_tar_delete;
	delete[13] = &xa_zip_delete;
	delete[14] = &xa_lha_delete;
	
	add[0]  = 0;
	add[1]  = &xa_7zip_add;
	add[2]  = &xa_arj_add;
	add[3]  = 0;
	add[4]  = add[5] = add[6] = &xa_tar_add;
	add[7]  = &xa_rar_add;
	add[8]  = 0;
	add[9]  = add[10] = add[11] = add[12] = &xa_tar_add;
	add[13] = &xa_zip_add;
	add[14] = &xa_lha_add;
	
	extract[0]  = 0;
	extract[1]  = &xa_7zip_extract;
	extract[2]  = &xa_arj_extract;
	extract[3]  = &xa_deb_extract;;
	extract[4]  = extract[6] = &lzma_bzip2_extract;
	extract[5]  = &gzip_extract;
	extract[7]  = &xa_rar_extract;
	extract[8]  = &xa_rpm_extract;
	extract[9]  = extract[10] = extract[11] = extract[12] = &xa_tar_extract;
	extract[13] = &xa_zip_extract;
	extract[14] = &xa_lha_extract;
	
	test[0]  = 0;
	test[1]  = &xa_7zip_test;
	test[2]  = &xa_arj_test;
	test[3]  = test[4] = test[5] = test[6] = 0;
	test[7]  = &xa_rar_test;
	test[8]  = 0;
	test[9]  = test[10] = test[11] = test[12] = 0;
	test[13] = &xa_zip_test;
	test[14] = &xa_lha_test;

	absolute_path = g_find_program_in_path("arj");
	if ( absolute_path )
	{
		ArchiveType = g_list_append(ArchiveType, "arj");
		ArchiveSuffix = g_list_append(ArchiveSuffix, "*.arj");
		g_free (absolute_path);
	}

	absolute_path = g_find_program_in_path("bzip2");
    if ( absolute_path )
	{
		ArchiveType = g_list_append (ArchiveType, "bz2");
		ArchiveSuffix = g_list_append (ArchiveSuffix, "*.bz2");
		g_free (absolute_path);
	}

	absolute_path = g_find_program_in_path("ar");
    if ( absolute_path )
	{
	    ArchiveSuffix = g_list_append (ArchiveSuffix, "*.deb");
		g_free (absolute_path);
	}

	absolute_path = g_find_program_in_path("gzip");
	if ( absolute_path )
	{
		ArchiveType = g_list_append(ArchiveType, "gz");
		ArchiveSuffix = g_list_append(ArchiveSuffix, "*.gz");
		g_free (absolute_path);
	}

	absolute_path = g_find_program_in_path("lzma");
	if ( absolute_path )
	{
		ArchiveType = g_list_append(ArchiveType, "lzma");
		ArchiveSuffix = g_list_append(ArchiveSuffix, "*.lzma");
		g_free (absolute_path);
	}

	absolute_path = g_find_program_in_path("lha");
	if (absolute_path)
	{
		ArchiveType = g_list_append(ArchiveType, "lzh");
		ArchiveSuffix = g_list_append(ArchiveSuffix, "*.lzh");
		g_free (absolute_path);
		//ArchiveType = g_list_append(ArchiveType, ".lha");
		//ArchiveSuffix = g_list_append(ArchiveSuffix, "");
	}

	absolute_path = g_find_program_in_path ("rar");
    if ( absolute_path )
	{
		ArchiveType = g_list_append(ArchiveType, "rar");
		ArchiveSuffix = g_list_append(ArchiveSuffix, "*.rar");
		g_free (absolute_path);
	}
	else
	{
		absolute_path = g_find_program_in_path("unrar");
		if ( absolute_path )
		{
			unrar = TRUE;
			ArchiveType = g_list_append(ArchiveType, "rar");
			ArchiveSuffix = g_list_append(ArchiveSuffix, "*.rar");
			g_free (absolute_path);
		}
	}

	absolute_path = g_find_program_in_path("cpio");
    if ( absolute_path )
	{
	    ArchiveSuffix = g_list_append(ArchiveSuffix, "*.rpm");
		g_free (absolute_path);
	}

	absolute_path = g_find_program_in_path("gtar");
	
	if (absolute_path == NULL)
	{
		tar = "tar";
		ArchiveType = g_list_append(ArchiveType, "tar");
		ArchiveSuffix = g_list_append(ArchiveSuffix, "*.tar");
		g_free (absolute_path);
		if ( g_list_find ( ArchiveType , "bz2") )
		{
			ArchiveType = g_list_append(ArchiveType, "tar.bz2");
			/* The following to avoid double filter when opening */
			ArchiveSuffix = g_list_append(ArchiveSuffix, "");
		}
		if ( g_list_find ( ArchiveType , "gz") )
		{
			ArchiveType = g_list_append(ArchiveType, "tar.gz");
			ArchiveSuffix = g_list_append(ArchiveSuffix, "*.tgz");
		}
		if ( g_list_find ( ArchiveType , "lzma") )
		{
			ArchiveType = g_list_append(ArchiveType, "tar.lzma");
			ArchiveSuffix = g_list_append(ArchiveSuffix, "*.tlz");
		}
	}
	else
		tar = "gtar";

	absolute_path = g_find_program_in_path ("zip");
    if (absolute_path)
	{
		g_free (absolute_path);
		absolute_path = g_find_program_in_path ("zipinfo");
    	if (absolute_path)
		{
			g_free (absolute_path);
			ArchiveType = g_list_append(ArchiveType, "jar");
			ArchiveSuffix = g_list_append(ArchiveSuffix, "*.jar");

			ArchiveType = g_list_append(ArchiveType, "zip");
			ArchiveSuffix = g_list_append(ArchiveSuffix, "*.zip");
		}
	}
	absolute_path = g_find_program_in_path("7za");
    if (absolute_path != NULL)
    	sevenza = TRUE;
    else
    	absolute_path = g_find_program_in_path("7zr");
    if (absolute_path != NULL)
    {
    	sevenzr = TRUE;
        ArchiveType = g_list_append(ArchiveType, "7z");
	    ArchiveSuffix = g_list_append(ArchiveSuffix, "*.7z");
		g_free (absolute_path);
    }
    absolute_path = g_find_program_in_path("xdg-open");
    if (absolute_path != NULL)
    {
    	xdg_open = TRUE;
    	g_free (absolute_path);
    }
}

XArchive *xa_init_structure_from_cmd_line (char *filename)
{
	XArchive *archive;
	XArchiveType type;

	type = xa_detect_archive_type (filename);
	if (type == -1 || type == -2)
		return NULL;

	archive = xa_init_archive_structure (type);
	if (archive == NULL)
	{
		response = xa_show_message_dialog (NULL,GTK_DIALOG_MODAL,GTK_MESSAGE_ERROR,GTK_BUTTONS_OK,_("Can't allocate memory for the archive structure!"),"" );
		return NULL;
	}
	archive->path = g_strdup (filename);
	archive->escaped_path = xa_escape_bad_chars(filename , "$\'`\"\\!?* ()&|@#:;");
	archive->type = type;
	if ( g_str_has_suffix ( archive->escaped_path , ".tar.bz2") || g_str_has_suffix ( archive->escaped_path , ".tar.bz") || g_str_has_suffix ( archive->escaped_path , ".tbz") || g_str_has_suffix ( archive->escaped_path , ".tbz2" ) )
		archive->type = XARCHIVETYPE_TAR_BZ2;
	else if ( g_str_has_suffix ( archive->escaped_path , ".tar.gz") || g_str_has_suffix ( archive->escaped_path , ".tgz") )
		archive->type = XARCHIVETYPE_TAR_GZ;
	else if ( g_str_has_suffix ( archive->escaped_path , ".tar.lzma") || g_str_has_suffix ( archive->escaped_path , ".tlz") )
		archive->type = XARCHIVETYPE_TAR_LZMA;
	return (archive);
}

