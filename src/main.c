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
#include <stdlib.h>
#include <string.h>
#include "main.h"
#include "7zip.h"
#include "add_dialog.h"
#include "arj.h"
#include "deb.h"
#include "extract_dialog.h"
#include "gzip_et_al.h"
#include "interface.h"
#include "lha.h"
#include "new_dialog.h"
#include "pref_dialog.h"
#include "rar.h"
#include "rpm.h"
#include "string_utils.h"
#include "support.h"
#include "tar.h"
#include "window.h"
#include "zip.h"

#ifdef HAVE_SOCKET
#include "socket.h"
#endif

XArchiver archiver[XARCHIVETYPE_TYPES];

ask_func ask[XARCHIVETYPE_TYPES];
open_func open[XARCHIVETYPE_TYPES];
test_func test[XARCHIVETYPE_TYPES];
extract_func extract[XARCHIVETYPE_TYPES];
add_func add[XARCHIVETYPE_TYPES];
delete_func delete[XARCHIVETYPE_TYPES];

const gchar *locale;
const gchar *tar;
gboolean batch_mode;
gboolean xdg_open;
gboolean opt_multi_extract;

Add_dialog_data *add_window;
Extract_dialog_data *extract_window;
Multi_extract_data *multi_extract_window;
Prefs_dialog_data *prefs_window;

static gchar *opt_extract_path, *opt_add_files;
static gboolean opt_extract, opt_add, opt_version;
static gboolean zip;

static GOptionEntry entries[] =
{
	{	"extract-to", 'x', G_OPTION_FLAG_IN_MAIN, G_OPTION_ARG_FILENAME, &opt_extract_path,
		N_("Extract archive to the destination directory and quits."),
		N_("destination archive")
	},
	{	"extract", 'e', G_OPTION_FLAG_IN_MAIN, G_OPTION_ARG_NONE, &opt_extract,
		N_("Extract archive by asking the extraction directory and quits."),
		N_("archive")
	},
	{	"multi-extract", 'm', G_OPTION_FLAG_IN_MAIN, G_OPTION_ARG_NONE, &opt_multi_extract,
		N_("Multi-extract archives"),
		N_("filenames")
	},
	{	"add-to", 'd', G_OPTION_FLAG_IN_MAIN, G_OPTION_ARG_STRING, &opt_add_files,
		N_("Add the given files by asking the name of the archive and quits."),
		N_("file1 file2 file3 ... fileN")
	},
	{	"add", 'a', G_OPTION_FLAG_IN_MAIN, G_OPTION_ARG_NONE, &opt_add,
		N_("Add files to archive by asking their filenames and quits."),
		N_("archive")
	},
	{	"version", 'V', G_OPTION_FLAG_IN_MAIN, G_OPTION_ARG_NONE, &opt_version,
		N_("Show version and exit"), NULL },
	{ NULL }
};

static void xa_check_available_archivers ()
{
	XArchiveType type;
	gchar *path;

	ask[XARCHIVETYPE_BZIP2]  = &xa_gzip_et_al_ask;
	ask[XARCHIVETYPE_GZIP]  = &xa_gzip_et_al_ask;
	ask[XARCHIVETYPE_LZMA]  = &xa_gzip_et_al_ask;
	ask[XARCHIVETYPE_XZ]  = &xa_gzip_et_al_ask;
	ask[XARCHIVETYPE_RPM]  = &xa_rpm_ask;
	ask[XARCHIVETYPE_TAR]  = ask[XARCHIVETYPE_TAR_BZ2] = ask[XARCHIVETYPE_TAR_GZ] = ask[XARCHIVETYPE_TAR_LZMA] = ask[XARCHIVETYPE_TAR_XZ] = ask[XARCHIVETYPE_TAR_LZOP] = &xa_tar_ask;
	ask[XARCHIVETYPE_ZIP] = &xa_zip_ask;
	ask[XARCHIVETYPE_LZOP] = &xa_gzip_et_al_ask;

	open[XARCHIVETYPE_BZIP2]  = &xa_gzip_et_al_open;
	open[XARCHIVETYPE_GZIP]  = &xa_gzip_et_al_open;
	open[XARCHIVETYPE_LZMA]  = &xa_gzip_et_al_open;
	open[XARCHIVETYPE_XZ]  = &xa_gzip_et_al_open;
	open[XARCHIVETYPE_RPM]  = &xa_rpm_open;
	open[XARCHIVETYPE_TAR]  = open[XARCHIVETYPE_TAR_BZ2] = open[XARCHIVETYPE_TAR_GZ] = open[XARCHIVETYPE_TAR_LZMA] = open[XARCHIVETYPE_TAR_XZ] = open[XARCHIVETYPE_TAR_LZOP] = &xa_tar_open;
	open[XARCHIVETYPE_ZIP] = &xa_zip_open;
	open[XARCHIVETYPE_LZOP] = &xa_gzip_et_al_open;

	delete[XARCHIVETYPE_BZIP2]  = delete[XARCHIVETYPE_GZIP] = delete[XARCHIVETYPE_LZMA] = delete[XARCHIVETYPE_XZ] = delete[XARCHIVETYPE_LZOP] = &xa_tar_delete;
	delete[XARCHIVETYPE_RPM]  = 0;
	delete[XARCHIVETYPE_TAR]  = delete[XARCHIVETYPE_TAR_BZ2] = delete[XARCHIVETYPE_TAR_GZ] = delete[XARCHIVETYPE_TAR_LZMA] = delete[XARCHIVETYPE_TAR_XZ] = delete[XARCHIVETYPE_TAR_LZOP] = &xa_tar_delete;
	delete[XARCHIVETYPE_ZIP] = &xa_zip_delete;


	add[XARCHIVETYPE_BZIP2]  = add[XARCHIVETYPE_GZIP] = add[XARCHIVETYPE_LZMA] = add[XARCHIVETYPE_XZ] = add[XARCHIVETYPE_LZOP] = &xa_tar_add;
	add[XARCHIVETYPE_RPM]  = 0;
	add[XARCHIVETYPE_TAR]  = add[XARCHIVETYPE_TAR_BZ2] = add[XARCHIVETYPE_TAR_GZ] = add[XARCHIVETYPE_TAR_LZMA] = add[XARCHIVETYPE_TAR_XZ] = add[XARCHIVETYPE_TAR_LZOP] = &xa_tar_add;
	add[XARCHIVETYPE_ZIP] = &xa_zip_add;

	extract[XARCHIVETYPE_BZIP2]  = extract[XARCHIVETYPE_GZIP] = extract[XARCHIVETYPE_LZMA] = extract[XARCHIVETYPE_XZ] = extract[XARCHIVETYPE_LZOP] = &xa_tar_extract;
	extract[XARCHIVETYPE_RPM]  = &xa_rpm_extract;
	extract[XARCHIVETYPE_TAR]  = extract[XARCHIVETYPE_TAR_BZ2] = extract[XARCHIVETYPE_TAR_GZ] = extract[XARCHIVETYPE_TAR_LZMA] = extract[XARCHIVETYPE_TAR_XZ] = extract[XARCHIVETYPE_TAR_LZOP] = &xa_tar_extract;
	extract[XARCHIVETYPE_ZIP] = &xa_zip_extract;

	test[XARCHIVETYPE_BZIP2] = test[XARCHIVETYPE_GZIP] = test[XARCHIVETYPE_LZMA] = test[XARCHIVETYPE_XZ] = test[XARCHIVETYPE_LZOP] = &xa_tar_test;
	test[XARCHIVETYPE_RPM]  = 0;
	test[XARCHIVETYPE_TAR]  = test[XARCHIVETYPE_TAR_BZ2] = test[XARCHIVETYPE_TAR_GZ] = test[XARCHIVETYPE_TAR_LZMA] = test[XARCHIVETYPE_TAR_XZ] = test[XARCHIVETYPE_TAR_LZOP] =  &xa_tar_test;
	test[XARCHIVETYPE_ZIP] = &xa_zip_test;

	/* 7-zip */

	type = XARCHIVETYPE_7ZIP;
	path = g_find_program_in_path("7z");

	if (!path)
		path = g_find_program_in_path("7za");

	if (!path)
		path = g_find_program_in_path("7zr");

	if (path)
	{
		archiver[type].program[0] = path;
		archiver[type].is_compressor = TRUE;
		archiver[type].type = g_slist_append(archiver[type].type, "7zip");
		archiver[type].glob = g_slist_append(archiver[type].glob, "*.7z");

		ask[type] = xa_7zip_ask;
		open[type] = xa_7zip_open;
		test[type] = xa_7zip_test;
		extract[type] = xa_7zip_extract;
		add[type] = xa_7zip_add;
		delete[type] = xa_7zip_delete;
	}

	/* ARJ */

	type = XARCHIVETYPE_ARJ;
	path = g_find_program_in_path("arj");

	if (path)
		archiver[type].is_compressor = TRUE;
	else
		path = g_find_program_in_path("unarj");

	if (path)
	{
		archiver[type].program[0] = path;
		archiver[type].type = g_slist_append(archiver[type].type, "arj");
		archiver[type].glob = g_slist_append(archiver[type].glob, "*.arj");

		ask[type] = xa_arj_ask;
		open[type] = xa_arj_open;
		test[type] = xa_arj_test;
		extract[type] = xa_arj_extract;
		add[type] = xa_arj_add;
		delete[type] = xa_arj_delete;
	}

	/* bzip2 */

	type = XARCHIVETYPE_BZIP2;

	path = g_find_program_in_path("bzip2");
    if ( path )
	{
		archiver[type].type = g_slist_append(archiver[type].type, "bzip2");
		archiver[type].glob = g_slist_append(archiver[type].glob, "*.bz2");
		g_free (path);
	}

	/* debian package */

	type = XARCHIVETYPE_DEB;
	path = g_find_program_in_path("ar");

	if (path)
	{
		archiver[type].program[0] = path;
		archiver[type].type = g_slist_append(archiver[type].type, "deb");
		archiver[type].glob = g_slist_append(archiver[type].glob, "*.deb");

		ask[type] = xa_deb_ask;
		open[type] = xa_deb_open;
		extract[type]  = xa_deb_extract;
	}

	/* GNU zip */

	type = XARCHIVETYPE_GZIP;

	path = g_find_program_in_path("gzip");
	if ( path )
	{
		archiver[type].type = g_slist_append(archiver[type].type, "gzip");
		archiver[type].glob = g_slist_append(archiver[type].glob, "*.gz");
		g_free (path);
	}

	/* java archive */

	path = g_find_program_in_path("zip");
	if (path)
	{
		g_free (path);
		path = g_find_program_in_path("zipinfo");
		if (path)
		{
			zip = TRUE;
			archiver[type].type = g_slist_append(archiver[type].type, "jar");
			archiver[type].glob = g_slist_append(archiver[type].glob, "*.jar");
			g_free(path);
		}
	}

	/* LHA */

	type = XARCHIVETYPE_LHA;
	path = g_find_program_in_path("lha");

	if (path)
	{
		archiver[type].program[0] = path;
		archiver[type].is_compressor = TRUE;
		archiver[type].type = g_slist_append(archiver[type].type, "lha");
		archiver[type].glob = g_slist_append(archiver[type].glob, "*.lzh");

		ask[type] = xa_lha_ask;
		open[type] = xa_lha_open;
		test[type] = xa_lha_test;
		extract[type] = xa_lha_extract;
		add[type] = xa_lha_add;
		delete[type] = xa_lha_delete;
	}

	/* lzma */

	type = XARCHIVETYPE_LZMA;

	path = g_find_program_in_path("lzma");
	if ( path )
	{
		archiver[type].type = g_slist_append(archiver[type].type, "lzma");
		archiver[type].glob = g_slist_append(archiver[type].glob, "*.lzma");
		g_free (path);
	}

	/* lzop */

	type = XARCHIVETYPE_LZOP;

	path = g_find_program_in_path("lzop");
	if (path)
	{
		archiver[type].type = g_slist_append(archiver[type].type, "lzop");
		archiver[type].glob = g_slist_append(archiver[type].glob, "*.lzo");
		g_free (path);
	}

	/* RAR */

	type = XARCHIVETYPE_RAR;
	path = g_find_program_in_path("rar");

	if (path)
		archiver[type].is_compressor = TRUE;
	else
		path = g_find_program_in_path("unrar");

	if (path)
	{
		archiver[type].program[0] = path;
		archiver[type].type = g_slist_append(archiver[type].type, "rar");
		archiver[type].glob = g_slist_append(archiver[type].glob, "*.rar");

		if (xa_rar_check_version(path) == 5)
		{
			archiver[type].type = g_slist_append(archiver[type].type, "rar5");
			archiver[type].glob = g_slist_append(archiver[type].glob, " .rar");
		}

		ask[type] = xa_rar_ask;
		open[type] = xa_rar_open;
		test[type] = xa_rar_test;
		extract[type] = xa_rar_extract;
		add[type] = xa_rar_add;
		delete[type] = xa_rar_delete;
	}

	/* RPM package */

	type = XARCHIVETYPE_RPM;

	path = g_find_program_in_path("cpio");
    if ( path )
	{
	    archiver[type].type = g_slist_append(archiver[type].type, "rpm");
	    archiver[type].glob = g_slist_append(archiver[type].glob, "*.rpm");
		g_free (path);
	}

	/* tape archive */

	type = XARCHIVETYPE_TAR;

	path = g_find_program_in_path("gtar");
	if (path)
	{
		tar = "gtar";
		g_free (path);
	}
	else
	{
		path = g_find_program_in_path("tar");
		if (path)
		{
			tar = "tar";
			g_free (path);
		}
		else
			tar = NULL;
	}
	if (tar)
	{
		archiver[type].type = g_slist_append(archiver[type].type, "tar");
		archiver[type].glob = g_slist_append(archiver[type].glob, "*.tar");
	}

	/* xz */

	type = XARCHIVETYPE_XZ;

	path = g_find_program_in_path("xz");
	if (path)
	{
		archiver[type].type = g_slist_append(archiver[type].type, "xz");
		archiver[type].glob = g_slist_append(archiver[type].glob, "*.xz");
		g_free(path);
	}

	/* zip */

	type = XARCHIVETYPE_ZIP;

	if (zip)
	{
		archiver[type].type = g_slist_append(archiver[type].type, "zip");
		archiver[type].glob = g_slist_append(archiver[type].glob, "*.zip");
	}

	/* compressed tar */

	if (tar)
	{
		if (archiver[XARCHIVETYPE_BZIP2].type)
		{
			type = XARCHIVETYPE_TAR_BZ2;
			archiver[type].type = g_slist_append(archiver[type].type, "tar.bzip2");
			archiver[type].glob = g_slist_append(archiver[type].glob, "*.tar.bz2");
			archiver[type].type = g_slist_append(archiver[type].type, NULL);
			archiver[type].glob = g_slist_append(archiver[type].glob, "*.tbz2");
		}
		if (archiver[XARCHIVETYPE_GZIP].type)
		{
			type = XARCHIVETYPE_TAR_GZ;
			archiver[type].type = g_slist_append(archiver[type].type, "tar.gzip");
			archiver[type].glob = g_slist_append(archiver[type].glob, "*.tar.gz");
			archiver[type].type = g_slist_append(archiver[type].type, NULL);
			archiver[type].glob = g_slist_append(archiver[type].glob, "*.tgz");
		}
		if (archiver[XARCHIVETYPE_LZMA].type)
		{
			type = XARCHIVETYPE_TAR_LZMA;
			archiver[type].type = g_slist_append(archiver[type].type, "tar.lzma");
			archiver[type].glob = g_slist_append(archiver[type].glob, "*.tar.lzma");
			archiver[type].type = g_slist_append(archiver[type].type, NULL);
			archiver[type].glob = g_slist_append(archiver[type].glob, "*.tlz");
		}
		if (archiver[XARCHIVETYPE_LZOP].type)
		{
			type = XARCHIVETYPE_TAR_LZOP;
			archiver[type].type = g_slist_append(archiver[type].type, "tar.lzop");
			archiver[type].glob = g_slist_append(archiver[type].glob, "*.tar.lzo");
			archiver[type].type = g_slist_append(archiver[type].type, NULL);
			archiver[type].glob = g_slist_append(archiver[type].glob, "*.tzo");
		}
		if (archiver[XARCHIVETYPE_XZ].type)
		{
			type = XARCHIVETYPE_TAR_XZ;
			archiver[type].type = g_slist_append(archiver[type].type, "tar.xz");
			archiver[type].glob = g_slist_append(archiver[type].glob, "*.tar.xz");
			archiver[type].type = g_slist_append(archiver[type].type, NULL);
			archiver[type].glob = g_slist_append(archiver[type].glob, "*.txz");
		}
	}
}

static XArchive *xa_init_structure_from_cmd_line (char *filename)
{
	XArchive *archive;
	XArchiveType type;

	type = xa_detect_archive_type (filename);
	if (type == XARCHIVETYPE_UNKNOWN || type == XARCHIVETYPE_NOT_FOUND)
		return NULL;

	archive = xa_init_archive_structure (type);
	if (archive == NULL)
	{
		xa_show_message_dialog (NULL,GTK_DIALOG_MODAL,GTK_MESSAGE_ERROR,GTK_BUTTONS_OK,_("Can't allocate memory for the archive structure!"),"" );
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
	else if ( g_str_has_suffix ( archive->escaped_path , ".tar.xz") || g_str_has_suffix ( archive->escaped_path , ".txz") )
		archive->type = XARCHIVETYPE_TAR_XZ;
	else if ( g_str_has_suffix ( archive->escaped_path , ".tar.lzo") ||
		g_str_has_suffix ( archive->escaped_path , ".tzo") ||
		g_str_has_suffix ( archive->escaped_path , ".tar.lzop"))
		archive->type = XARCHIVETYPE_TAR_LZOP;
	archive->extract = 	extract[archive->type];
	return (archive);
}

int main (int argc, char **argv)
{
	GError *cli_error = NULL;
	XArchive *archive = NULL;
	gboolean no_bzip2_gzip;
	unsigned short int x;
	gchar *path, *_current_dir;

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
    if (opt_version)
    {
        g_print("%s %s \n\n", PACKAGE_NAME, PACKAGE_VERSION);
        g_print ("%s\n", "Copyright (c) 2005-2008");
        g_print ("\t%s\n\n", "Giuseppe Torelli - Colossus <colossus73@gmail.com>");
        g_print (_("Maintained by "));
        g_print ("Ingo Brückl.\n");
        g_print (_("Please report bugs to <%s>."), PACKAGE_BUGREPORT);
        g_print ("\n");

        return EXIT_SUCCESS;
    }

	if (opt_extract || opt_extract_path || opt_multi_extract || opt_add || opt_add_files)
		batch_mode = TRUE;

	path = g_find_program_in_path("xdg-open");

	if (path)
	{
		xdg_open = TRUE;
		g_free(path);
	}

	xa_check_available_archivers();
	prefs_window   = xa_create_prefs_dialog();
	extract_window = xa_create_extract_dialog();
	add_window     = xa_create_add_dialog();
	multi_extract_window = xa_create_multi_extract_dialog();
	xa_prefs_load_options(prefs_window);

	if (batch_mode)
	{
		xa_main_window = NULL;
		archive = xa_init_structure_from_cmd_line (argv[1]);
		g_print(PACKAGE_NAME " " VERSION " (\xC2\xA9)2005-2008 Giuseppe Torelli\n");

		/* Switch -x */
		if (opt_extract_path)
		{
			if (argv[1] == NULL || archive == NULL)
			{
				xa_show_message_dialog (NULL,GTK_DIALOG_MODAL,GTK_MESSAGE_ERROR,GTK_BUTTONS_OK,_("Can't extract files from the archive:"),_("You missed the archive name!\n"));
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
			archive->full_path = archive->can_full_path;
			archive->overwrite = archive->can_overwrite;
			gchar *escaped_path = xa_escape_bad_chars(opt_extract_path, "$\'`\"\\!?* ()[]&|@#:;");
			archive->extraction_path = escaped_path;
			archive->status = XA_ARCHIVESTATUS_EXTRACT;
			(*archive->extract) (archive,string);
		}
		/* Switch -e */
		else if (opt_extract && archive)
		{
			if (argv[1] == NULL)
			{
				xa_show_message_dialog (NULL,GTK_DIALOG_MODAL,GTK_MESSAGE_ERROR,GTK_BUTTONS_OK,_("Can't extract files from the archive:"),_("You missed the archive name!\n"));
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
		else if (opt_multi_extract)
		{
			Multi_extract_data *multi_extract = NULL;
			multi_extract = xa_create_multi_extract_dialog();
			for (x = 1; x< argc; x++)
				if (! g_file_test(argv[x], G_FILE_TEST_IS_DIR))
					xa_add_files_liststore(argv[x],multi_extract);

			xa_parse_multi_extract_archive(multi_extract);
			gtk_widget_destroy (multi_extract->multi_extract);
			g_free(multi_extract);
		}
		/* Switch -d */
		else if (opt_add_files)
		{
			if (argc > 1 || g_file_test (argv[1],G_FILE_TEST_IS_DIR))
				no_bzip2_gzip = TRUE;
			else
				no_bzip2_gzip = FALSE;
			archive = xa_new_archive_dialog(opt_add_files, NULL, no_bzip2_gzip);
			if (archive == NULL)
				return -1;

			if (archive->path != NULL)
			{
				xa_create_temp_directory(archive);
				archive->add_recurse = archive->can_recurse;
				_current_dir = g_path_get_dirname(opt_add_files);
				chdir (_current_dir);
				g_free(_current_dir);
				GSList *files = NULL;
				_current_dir = g_path_get_basename(opt_add_files);
				files = g_slist_append(files,g_strdup(_current_dir));
				g_free(_current_dir);
				g_free(opt_add_files);
				for (x = 1; x< argc; x++)
				{
					_current_dir = g_path_get_basename(argv[x]);
					files = g_slist_append(files,g_strdup(_current_dir));
					g_free (_current_dir);
				}
				xa_execute_add_commands(archive,files,NULL);
			}
		}
		/* Switch -a */
		else if (opt_add)
		{
			if (argv[1] == NULL)
			{
				xa_show_message_dialog (NULL,GTK_DIALOG_MODAL,GTK_MESSAGE_ERROR,GTK_BUTTONS_OK,_("Can't add files to the archive:"),_("You missed the archive name!\n"));
				return -1;
			}
			if (archive != NULL)
			{
				if (!archive->add)
				{
					xa_show_message_dialog(NULL, GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK, _("Can't add files to the archive:"), argv[1]);
					return -1;
				}
				xa_set_add_dialog_options(add_window,archive);
				xa_parse_add_dialog_options (archive,add_window);
				gtk_widget_destroy (add_window->dialog1);
				g_free (add_window);
			}
		}
done:
		for (x = XARCHIVETYPE_FIRST; x < XARCHIVETYPE_TYPES; x++)
		{
			g_free(archiver[x].program[0]);
			g_free(archiver[x].program[1]);
			g_slist_free(archiver[x].type);
			g_slist_free(archiver[x].glob);
		}

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
												gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (prefs_window->show_sidebar)),
												gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (prefs_window->show_toolbar)));

		gtk_window_set_transient_for (GTK_WINDOW (extract_window->dialog1),GTK_WINDOW (xa_main_window));
		gtk_window_set_transient_for (GTK_WINDOW (add_window->dialog1),GTK_WINDOW (xa_main_window));
		gtk_window_set_transient_for (GTK_WINDOW (prefs_window->dialog1),GTK_WINDOW (xa_main_window));
		gtk_window_set_transient_for (GTK_WINDOW (multi_extract_window->multi_extract),GTK_WINDOW (xa_main_window));
		if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(prefs_window->check_save_geometry)) && prefs_window->geometry[0] != -1)
		{
			gtk_window_move (GTK_WINDOW(xa_main_window), prefs_window->geometry[0], prefs_window->geometry[1]);
			gtk_window_set_default_size(GTK_WINDOW(xa_main_window), prefs_window->geometry[2], prefs_window->geometry[3]);
			gtk_paned_set_position(GTK_PANED(hpaned1), prefs_window->geometry[4]);
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
			gchar *dummy;
			if (g_str_has_prefix(argv[1], "file://") == TRUE)
				dummy = g_strdup(argv[1]+6);
			else
				dummy = g_strdup(argv[1]);

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

		for (x = XARCHIVETYPE_FIRST; x < XARCHIVETYPE_TYPES; x++)
		{
			g_free(archiver[x].program[0]);
			g_free(archiver[x].program[1]);
			g_slist_free(archiver[x].type);
			g_slist_free(archiver[x].glob);
		}
	}
	return 0;
}
