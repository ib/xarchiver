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
gchar *_current_dir = NULL;
gchar *extract_path = NULL;
GError *cli_error = NULL;
gchar *add_files;
gboolean error_output, file_to_open, ask_and_extract, ask_and_add, multi_extract;
gboolean batch_mode = FALSE;
gboolean unrar = FALSE;
gboolean sevenzr = FALSE, sevenza = FALSE, xdg_open = FALSE;
static gboolean show_version = FALSE;
int response;
extern gchar *current_open_directory;
extern int status;

extern Progress_bar_data *pb;
Prefs_dialog_data   *prefs_window   = NULL;
Extract_dialog_data *extract_window = NULL;
Add_dialog_data     *add_window		= NULL;
Multi_extract_data	*multi_extract_window	= NULL;

delete_func		delete[XARCHIVETYPE_COUNT]	= {NULL};
add_func		add[XARCHIVETYPE_COUNT]		= {NULL};
extract_func	extract[XARCHIVETYPE_COUNT]	= {NULL};
test_func		test[XARCHIVETYPE_COUNT]	= {NULL};
open_func		open_archive[XARCHIVETYPE_COUNT]	= {NULL};

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
	{	"add-to", 'd', G_OPTION_FLAG_IN_MAIN, G_OPTION_ARG_STRING, &add_files,
		N_("Add the given files by asking the name of the archive and quits."),
		N_("file1 file2 file3 ... fileN")
	},
	{	"add", 'a', G_OPTION_FLAG_IN_MAIN, G_OPTION_ARG_NONE, &ask_and_add,
		N_("Add files to archive by asking their filenames and quits."),
		N_("archive")
	},
	{	"version", 'V', G_OPTION_FLAG_IN_MAIN, G_OPTION_ARG_NONE, &show_version,
		N_("Show version and exit"), NULL },
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
    /* print version information */
    if (show_version)
    {
        g_print ("%s %s \n\n", PACKAGE, PACKAGE_VERSION);
        g_print ("%s\n", "Copyright (c) 2005-2008");
        g_print ("\t%s\n\n", "Giuseppe Torelli - Colossus <colossus73@gmail.com>");
        g_print (_("Please report bugs to <%s>."), PACKAGE_BUGREPORT);
        g_print ("\n");

        return EXIT_SUCCESS;
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
			if (xa_detect_encrypted_archive(archive))
			{
				archive->has_passwd = TRUE;
				archive->passwd = xa_create_password_dialog(archive);
				if (archive->passwd == NULL)
					goto done;
			}
			GSList *string = NULL;
			archive->full_path = 1;
			archive->overwrite = 1;
			gchar *escaped_path = xa_escape_bad_chars (extract_path,"$\'`\"\\!?* ()[]&|@#:;");
			archive->extraction_path = escaped_path;
			archive->status = XA_ARCHIVESTATUS_EXTRACT;
			(*archive->extract) (archive,string);
		}
		/* Switch -e */
		else if (ask_and_extract && archive != NULL)
		{
			if (argv[1] == NULL)
			{
				response = xa_show_message_dialog (NULL,GTK_DIALOG_MODAL,GTK_MESSAGE_ERROR,GTK_BUTTONS_OK,_("Can't extract files from the archive:"),_("You missed the archive name!\n"));
				return -1;
			}
			if (xa_detect_encrypted_archive(archive))
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
			archive = xa_new_archive_dialog (add_files,NULL,no_bzip2_gzip);
			if (archive == NULL)
				return -1;

			if (archive->path != NULL)
			{
				xa_create_temp_directory(archive);
				archive->add_recurse = TRUE;
				_current_dir = g_path_get_dirname(add_files);
				chdir (_current_dir);
				g_free(_current_dir);
				GSList *files = NULL;
				_current_dir = g_path_get_basename(add_files);
				files = g_slist_append(files,xa_escape_filename(_current_dir,"$'`\"\\!?* ()[]&|:;<>#"));
				g_free(_current_dir);
				g_free(add_files);
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

		if (pb != NULL)
		{
			gtk_widget_destroy(pb->progress_window);
			g_free(pb);
		}
		if (archive != NULL)
			xa_clean_archive_structure (archive);
		#ifdef HAVE_SOCKET
			socket_finalize();
		#endif
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
		gtk_window_set_transient_for (GTK_WINDOW (multi_extract_window->multi_extract),GTK_WINDOW (xa_main_window));
		if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(prefs_window->check_save_geometry)) && prefs_window->geometry[0] != -1)
		{
			gtk_window_move (GTK_WINDOW(xa_main_window), prefs_window->geometry[0], prefs_window->geometry[1]);
			gtk_window_set_default_size (GTK_WINDOW(xa_main_window), prefs_window->geometry[XARCHIVETYPE_7ZIP], prefs_window->geometry[XARCHIVETYPE_ARJ]);
			gtk_paned_set_position (GTK_PANED (hpaned1),prefs_window->geometry[XARCHIVETYPE_DEB]);
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
	}
	return 0;
}

void xa_set_available_archivers()
{
	open_archive[0]  = 0;
	open_archive[XARCHIVETYPE_7ZIP]  = &xa_open_7zip;
	open_archive[XARCHIVETYPE_ARJ]  = &xa_open_arj;
	open_archive[XARCHIVETYPE_DEB]  = &xa_open_deb;
	open_archive[XARCHIVETYPE_BZIP2]  = &xa_open_bzip2_lzma;
	open_archive[XARCHIVETYPE_GZIP]  = &xa_open_gzip;
	open_archive[XARCHIVETYPE_LZMA]  = &xa_open_bzip2_lzma;
	open_archive[XARCHIVETYPE_RAR]  = &xa_open_rar;
	open_archive[XARCHIVETYPE_RPM]  = &xa_open_rpm;
	open_archive[XARCHIVETYPE_TAR]  = open_archive[XARCHIVETYPE_TAR_BZ2] = open_archive[XARCHIVETYPE_TAR_GZ] = open_archive[XARCHIVETYPE_TAR_LZMA] = open_archive[XARCHIVETYPE_TAR_LZOP] = &xa_open_tar;
	open_archive[XARCHIVETYPE_ZIP] = &xa_open_zip;
	open_archive[XARCHIVETYPE_LHA] = &xa_open_lha;
	open_archive[XARCHIVETYPE_LZOP] = &xa_open_bzip2_lzma;
	
	delete[0]  = 0;
	delete[XARCHIVETYPE_7ZIP]  = &xa_7zip_delete;
	delete[XARCHIVETYPE_ARJ]  = &xa_arj_delete;
	delete[XARCHIVETYPE_DEB]  = 0;
	delete[XARCHIVETYPE_BZIP2]  = 0;
	delete[XARCHIVETYPE_GZIP]  = 0;
	delete[XARCHIVETYPE_LZMA]  = 0;
	delete[XARCHIVETYPE_RAR]  = &xa_rar_delete;
	delete[XARCHIVETYPE_RPM]  = 0;
	delete[XARCHIVETYPE_TAR]  = delete[XARCHIVETYPE_TAR_BZ2] = delete[XARCHIVETYPE_TAR_GZ] = delete[XARCHIVETYPE_TAR_LZMA] = delete[XARCHIVETYPE_TAR_LZOP] = &xa_tar_delete;
	delete[XARCHIVETYPE_ZIP] = &xa_zip_delete;
	delete[XARCHIVETYPE_LHA] = &xa_lha_delete;
	delete[XARCHIVETYPE_LZOP] = 0;
	
	
	add[0]  = 0;
	add[XARCHIVETYPE_7ZIP]  = &xa_7zip_add;
	add[XARCHIVETYPE_ARJ]  = &xa_arj_add;
	add[XARCHIVETYPE_DEB]  = 0;
	add[XARCHIVETYPE_BZIP2]  = add[XARCHIVETYPE_GZIP] = add[XARCHIVETYPE_LZMA] = &xa_tar_add;
	add[XARCHIVETYPE_RAR]  = &xa_rar_add;
	add[XARCHIVETYPE_RPM]  = 0;
	add[XARCHIVETYPE_TAR]  = add[XARCHIVETYPE_TAR_BZ2] = add[XARCHIVETYPE_TAR_GZ] = add[XARCHIVETYPE_TAR_LZMA] = add[XARCHIVETYPE_TAR_LZOP] = &xa_tar_add;
	add[XARCHIVETYPE_ZIP] = &xa_zip_add;
	add[XARCHIVETYPE_LHA] = &xa_lha_add;
	add[XARCHIVETYPE_LZOP] = &xa_tar_add;
	
	extract[0]  = 0;
	extract[XARCHIVETYPE_7ZIP]  = &xa_7zip_extract;
	extract[XARCHIVETYPE_ARJ]  = &xa_arj_extract;
	extract[XARCHIVETYPE_DEB]  = &xa_deb_extract;;
	extract[XARCHIVETYPE_BZIP2]  = extract[XARCHIVETYPE_GZIP] = extract[XARCHIVETYPE_LZMA] = &xa_tar_extract;
	extract[XARCHIVETYPE_RAR]  = &xa_rar_extract;
	extract[XARCHIVETYPE_RPM]  = &xa_rpm_extract;
	extract[XARCHIVETYPE_TAR]  = extract[XARCHIVETYPE_TAR_BZ2] = extract[XARCHIVETYPE_TAR_GZ] = extract[XARCHIVETYPE_TAR_LZMA] = extract[XARCHIVETYPE_TAR_LZOP] = &xa_tar_extract;
	extract[XARCHIVETYPE_ZIP] = &xa_zip_extract;
	extract[XARCHIVETYPE_LHA] = &xa_lha_extract;
	extract[XARCHIVETYPE_LZOP] = &xa_tar_extract;
	
	test[0]  = 0;
	test[XARCHIVETYPE_7ZIP]  = &xa_7zip_test;
	test[XARCHIVETYPE_ARJ]  = &xa_arj_test;
	test[XARCHIVETYPE_DEB]  = test[XARCHIVETYPE_BZIP2] = test[XARCHIVETYPE_GZIP] = test[XARCHIVETYPE_LZMA] = 0;
	test[XARCHIVETYPE_RAR]  = &xa_rar_test;
	test[XARCHIVETYPE_RPM]  = 0;
	test[XARCHIVETYPE_TAR]  = test[XARCHIVETYPE_TAR_BZ2] = test[XARCHIVETYPE_TAR_GZ] = test[XARCHIVETYPE_TAR_LZMA] = test[XARCHIVETYPE_TAR_LZOP] = 0;
	test[XARCHIVETYPE_ZIP] = &xa_zip_test;
	test[XARCHIVETYPE_LHA] = &xa_lha_test;
	test[XARCHIVETYPE_LZOP] = 0;

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

	absolute_path = g_find_program_in_path("lzop");
	if ( absolute_path )
	{
		ArchiveType = g_list_append(ArchiveType, "lzo");
		ArchiveSuffix = g_list_append(ArchiveSuffix, "*.lzo");
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
		if ( g_list_find ( ArchiveType , "lzo") )
		{
			ArchiveType = g_list_append(ArchiveType, "tar.lzo");
			ArchiveSuffix = g_list_append(ArchiveSuffix, "*.tzo");
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
	if (type == XARCHIVETYPE_UNKNOWN || type == XARCHIVETYPE_NOT_FOUND)
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
	else if ( g_str_has_suffix ( archive->escaped_path , ".tar.lzo") ||
		g_str_has_suffix ( archive->escaped_path , ".tzo") ||
		g_str_has_suffix ( archive->escaped_path , ".tar.lzop"))
		archive->type = XARCHIVETYPE_TAR_LZOP;
	return (archive);
}

