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
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 *  MA 02110-1301 USA.
 */

#include "config.h"
#include <stdlib.h>
#include <string.h>
#include "main.h"
#include "7zip.h"
#include "add_dialog.h"
#include "ar.h"
#include "arj.h"
#include "cpio.h"
#include "extract_dialog.h"
#include "interface.h"
#include "lha.h"
#include "new_dialog.h"
#include "pref_dialog.h"
#include "rar.h"
#include "rpm.h"
#include "string_utils.h"
#include "support.h"
#include "tar.h"
#include "unar.h"
#include "window.h"
#include "zip.h"

#ifdef HAVE_SOCKET
#include "socket.h"
#endif

#define FUNC(cond1, func1, cond2, func2, cond3, func3) (cond1 ? func1 : (cond2 ? func2 : (cond3 ? func3 : NULL)))

GtkWidget *xa_main_window;

XArchiver archiver[XARCHIVETYPE_TYPES];

gchar *xdg_open;

Add_dialog_data *add_window;
Extract_dialog_data *extract_window;
Multi_extract_data *multi_extract_window;
Prefs_dialog_data *prefs_window;

static gchar *opt_extract_path, *opt_compress;
static gboolean opt_extract, opt_ensure_dir, opt_multi_extract, opt_add, opt_info, opt_version;

static GOptionEntry entries[] =
{
	{	"extract-to", 'x', G_OPTION_FLAG_IN_MAIN, G_OPTION_ARG_FILENAME, &opt_extract_path,
		N_("Extract archive to the destination\n                                     directory and quit"),
		N_("destination")
	},
	{	"extract", 'e', G_OPTION_FLAG_IN_MAIN, G_OPTION_ARG_NONE, &opt_extract,
		N_("Extract archive by asking the extraction\n                                     directory and quit"),
		NULL
	},
	{	"ensure-directory", 'd', G_OPTION_FLAG_IN_MAIN, G_OPTION_ARG_NONE, &opt_ensure_dir,
		N_("Extract archive to a containing directory\n                                     and quit"),
		NULL
	},
	{	"multi-extract", 'm', G_OPTION_FLAG_IN_MAIN, G_OPTION_ARG_NONE, &opt_multi_extract,
		N_("Extract multiple archives by asking the\n                                     extraction directory and quit"),
		NULL
	},
	{	"compress", 'c', G_OPTION_FLAG_IN_MAIN, G_OPTION_ARG_STRING, &opt_compress,
		N_("Add the given files by asking the name of\n                                     the archive and quit"),
		N_("file1 ... fileN")
	},
	{	"add", 'a', G_OPTION_FLAG_IN_MAIN, G_OPTION_ARG_NONE, &opt_add,
		N_("Add to archive by asking which files and\n                                     quit"),
		NULL
	},
	{	"info", 'i', G_OPTION_FLAG_IN_MAIN, G_OPTION_ARG_NONE, &opt_info,
		N_("Show found command line programs to be\n                                     used and exit"),
		NULL
	},
	{	"version", 'v', G_OPTION_FLAG_IN_MAIN, G_OPTION_ARG_NONE, &opt_version,
		N_("Show version and exit\n"), NULL },
	{ NULL }
};

static void xa_check_available_archivers ()
{
	XArchiveType type;
	gchar *path, *cpio, *lbzip2, *lsar, *pbzip2, *pigz, *plzip, *sevenz, *unar, *xz, *zstd;
	gboolean is7z = TRUE, is7za = TRUE, is7zr = TRUE, standard;

	/* (un)compressors that can handle various types */

	sevenz = g_find_program_in_path("7z");

	if (!sevenz)
	{
		is7z = FALSE;
		sevenz = g_find_program_in_path("7za");

		if (!sevenz)
		{
			is7za = FALSE;
			sevenz = g_find_program_in_path("7zr");

			if (!sevenz)
				is7zr = FALSE;
		}
	}

	cpio = g_find_program_in_path("cpio");
	lbzip2 = g_find_program_in_path("lbzip2");
	lsar = g_find_program_in_path("lsar");
	pbzip2 = g_find_program_in_path("pbzip2");
	pigz = g_find_program_in_path("pigz");
	plzip = g_find_program_in_path("plzip");
	unar = g_find_program_in_path("unar");
	xz = g_find_program_in_path("xz");
	zstd = xa_gzip_et_al_check_zstd("zstd", "unzstd", &archiver[XARCHIVETYPE_ZSTD].is_compressor);

	/* 7-zip */

	type = XARCHIVETYPE_7ZIP;
	path = sevenz;

	standard = (path != NULL);

	if (!standard)
	{
		if (lsar)
			/* alternative ... */
			path = g_strdup(lsar);
		if (unar)
			/* ... uncompressor */
			archiver[type].program[1] = g_strdup(unar);
	}

	if (path)
	{
		archiver[type].program[0] = path;
		archiver[type].is_compressor = standard;
		archiver[type].type = g_slist_append(archiver[type].type, "7zip");
		archiver[type].glob = g_slist_append(archiver[type].glob, "*.7z");
		/* self-extracting Nullsoft Installer */
		archiver[type].tags = g_slist_append(archiver[type].tags, GUINT_TO_POINTER('n'));
		archiver[type].tags = g_slist_append(archiver[type].tags, _("Nullsoft Installer"));

		archiver[type].ask = (standard ? xa_7zip_ask : xa_unar_ask);
		archiver[type].list = (standard ? xa_7zip_list : xa_unar_list);
		archiver[type].test = (standard ? xa_7zip_test : xa_unar_test);
		archiver[type].extract = (standard ? xa_7zip_extract : (unar ? xa_unar_extract : NULL));
		archiver[type].add = (standard ? xa_7zip_add : NULL);
		archiver[type].delete = (standard ? xa_7zip_delete : NULL);
	}

	/* archiver */

	type = XARCHIVETYPE_AR;
	path = g_find_program_in_path("ar");

	standard = (path != NULL);

	if (!standard)
	{
		if (lsar && unar)
		{
			/* alternative ... */
			path = g_strdup(lsar);
			/* ... uncompressor */
			archiver[type].program[1] = g_strdup(unar);
		}
		else if (is7z)
			/* alternative uncompressor */
			path = g_strconcat(sevenz, " -tar", NULL);
	}

	if (path)
	{
		archiver[type].program[0] = path;
		archiver[type].type = g_slist_append(archiver[type].type, "ar");
		archiver[type].glob = g_slist_append(archiver[type].glob, "*.a");
		/* debian package */
		archiver[type].type = g_slist_append(archiver[type].type, "deb");
		archiver[type].glob = g_slist_append(archiver[type].glob, "*.deb");
		archiver[type].tags = g_slist_append(archiver[type].tags, GUINT_TO_POINTER('d'));
		archiver[type].tags = g_slist_append(archiver[type].tags, g_slist_last(archiver[type].type)->data);

		archiver[type].ask = FUNC(standard, xa_ar_ask, lsar && unar, xa_unar_ask, is7z, xa_7zip_ask);
		archiver[type].list = FUNC(standard, xa_ar_list, lsar && unar, xa_unar_list, is7z, xa_7zip_list);
		archiver[type].extract  = FUNC(standard, xa_ar_extract, lsar && unar, xa_unar_extract, is7z, xa_7zip_extract);
	}

	/* ARJ */

	type = XARCHIVETYPE_ARJ;
	path = g_find_program_in_path("arj");

	if (path)
		archiver[type].is_compressor = TRUE;
	else
		path = g_find_program_in_path("unarj");

	standard = (path != NULL);

	if (!standard)
	{
		if (is7z)
			/* alternative uncompressor */
			path = g_strconcat(sevenz, " -tarj", NULL);
		else
		{
			if (lsar)
				/* alternative ... */
				path = g_strdup(lsar);
			if (unar)
				/* ... uncompressor */
				archiver[type].program[1] = g_strdup(unar);
		}
	}

	if (path)
	{
		archiver[type].program[0] = path;
		archiver[type].type = g_slist_append(archiver[type].type, "arj");
		archiver[type].glob = g_slist_append(archiver[type].glob, "*.arj");

		archiver[type].ask = FUNC(standard, xa_arj_ask, is7z, xa_7zip_ask, lsar, xa_unar_ask);
		archiver[type].list = FUNC(standard, xa_arj_list, is7z, xa_7zip_list, lsar, xa_unar_list);
		archiver[type].test = FUNC(standard, xa_arj_test, is7z, xa_7zip_test, lsar, xa_unar_test);
		archiver[type].extract = FUNC(standard, xa_arj_extract, is7z, xa_7zip_extract, unar, xa_unar_extract);
		archiver[type].add = FUNC(standard, xa_arj_add, is7z, NULL, lsar, NULL);
		archiver[type].delete = FUNC(standard, xa_arj_delete, is7z, NULL, lsar, NULL);
	}

	/* bzip2 */

	type = XARCHIVETYPE_BZIP2;
	path = g_find_program_in_path("bzip2");

	if (path)
		archiver[type].is_compressor = TRUE;
	else
		path = g_find_program_in_path("bunzip2");

	standard = (path && archiver[type].is_compressor);

	if (!standard)
	{
		if (lbzip2)
		{
			g_free(path);
			/* alternative compressor */
			path = g_strdup(lbzip2);
			archiver[type].is_compressor = TRUE;
			standard = TRUE;
		}
		else if (pbzip2)
		{
			g_free(path);
			/* alternative compressor */
			path = g_strdup(pbzip2);
			archiver[type].is_compressor = TRUE;
			standard = TRUE;
		}
		else if (is7za)
		{
			g_free(path);
			/* alternative compressor */
			path = g_strconcat(sevenz, " -tbzip2", NULL);
			archiver[type].is_compressor = TRUE;
		}
		else if (!path)
		{
			if (lsar)
				/* alternative ... */
				path = g_strdup(lsar);
			if (unar)
				/* ... uncompressor */
				archiver[type].program[1] = g_strdup(unar);
		}
		else
			standard = TRUE;
	}

	if (path)
	{
		archiver[type].program[0] = path;
		archiver[type].type = g_slist_append(archiver[type].type, "bzip2");
		archiver[type].glob = g_slist_append(archiver[type].glob, "*.bz2");

		archiver[type].ask = FUNC(standard, xa_gzip_et_al_ask, is7za, xa_7zip_ask, lsar, xa_unar_ask);
		archiver[type].list = FUNC(standard, xa_gzip_et_al_list, is7za, xa_7zip_list, lsar, xa_unar_list);
		archiver[type].test = FUNC(standard, xa_gzip_et_al_test, is7za, xa_7zip_test, lsar, xa_unar_test);
		archiver[type].extract = FUNC(standard, xa_gzip_et_al_extract, is7za, xa_7zip_extract, unar, xa_unar_extract);
		archiver[type].add = FUNC(standard, xa_gzip_et_al_add, is7za, xa_7zip_add, lsar, NULL);
	}

	/* compress */

	type = XARCHIVETYPE_COMPRESS;
	path = g_find_program_in_path("compress");

	if (path)
		archiver[type].is_compressor = TRUE;
	else
		path = g_find_program_in_path("uncompress");

	standard = (path != NULL);

	if (!standard)
	{
		if (is7za)
			/* alternative uncompressor */
			path = g_strconcat(sevenz, " -tZ", NULL);
		else
		{
			if (lsar)
				/* alternative ... */
				path = g_strdup(lsar);
			if (unar)
				/* ... uncompressor */
				archiver[type].program[1] = g_strdup(unar);
		}
	}

	if (path)
	{
		archiver[type].program[0] = path;
		archiver[type].type = g_slist_append(archiver[type].type, "compress");
		archiver[type].glob = g_slist_append(archiver[type].glob, "*.Z");

		archiver[type].ask = FUNC(standard, xa_gzip_et_al_ask, is7za, xa_7zip_ask, lsar, xa_unar_ask);
		archiver[type].list = FUNC(standard, xa_gzip_et_al_list, is7za, xa_7zip_list, lsar, xa_unar_list);
		archiver[type].test = FUNC(standard, xa_gzip_et_al_test, is7za, xa_7zip_test, lsar, NULL);
		archiver[type].extract = FUNC(standard, xa_gzip_et_al_extract, is7za, xa_7zip_extract, unar, xa_unar_extract);
		archiver[type].add = FUNC(standard, xa_gzip_et_al_add, is7za, NULL, lsar, NULL);
	}

	/* cpio */

	type = XARCHIVETYPE_CPIO;
	path = cpio;

	standard = (path != NULL);

	if (!standard)
	{
		if (is7z)
			/* alternative uncompressor */
			path = g_strconcat(sevenz, " -tcpio", NULL);
		else
		{
			if (lsar)
				/* alternative ... */
				path = g_strdup(lsar);
			if (unar)
				/* ... uncompressor */
				archiver[type].program[1] = g_strdup(unar);
		}
	}

	if (path)
	{
		archiver[type].program[0] = path;
		archiver[type].is_compressor = standard;
		archiver[type].type = g_slist_append(archiver[type].type, "cpio");
		archiver[type].glob = g_slist_append(archiver[type].glob, "*.cpio");

		archiver[type].ask = FUNC(standard, xa_cpio_ask, is7z, xa_7zip_ask, lsar, xa_unar_ask);
		archiver[type].list = FUNC(standard, xa_cpio_list, is7z, xa_7zip_list, lsar, xa_unar_list);
		archiver[type].test = FUNC(standard, NULL, is7z, xa_7zip_test, lsar, NULL);
		archiver[type].extract = FUNC(standard, xa_cpio_extract, is7z, xa_7zip_extract, unar, xa_unar_extract);
		archiver[type].add = FUNC(standard, xa_cpio_add, is7z, NULL, lsar, NULL);
	}

	/* GNU zip */

	type = XARCHIVETYPE_GZIP;
	path = g_find_program_in_path("gzip");

	if (path)
		archiver[type].is_compressor = TRUE;
	else
		path = g_find_program_in_path("gunzip");

	standard = (path && archiver[type].is_compressor);

	if (!standard)
	{
		if (pigz)
		{
			g_free(path);
			/* alternative compressor */
			path = g_strdup(pigz);
			archiver[type].is_compressor = TRUE;
			standard = TRUE;
		}
		else if (is7za)
		{
			g_free(path);
			/* alternative compressor */
			path = g_strconcat(sevenz, " -tgzip", NULL);
			archiver[type].is_compressor = TRUE;
		}
		else if (!path)
		{
			if (lsar)
				/* alternative ... */
				path = g_strdup(lsar);
			if (unar)
				/* ... uncompressor */
				archiver[type].program[1] = g_strdup(unar);
		}
		else
			standard = TRUE;
	}

	if (path)
	{
		archiver[type].program[0] = path;
		archiver[type].type = g_slist_append(archiver[type].type, "gzip");
		archiver[type].glob = g_slist_append(archiver[type].glob, "*.gz");

		archiver[type].ask = FUNC(standard, xa_gzip_et_al_ask, is7za, xa_7zip_ask, lsar, xa_unar_ask);
		archiver[type].list = FUNC(standard, xa_gzip_et_al_list, is7za, xa_7zip_list, lsar, xa_unar_list);
		archiver[type].test = FUNC(standard, xa_gzip_et_al_test, is7za, xa_7zip_test, lsar, xa_unar_test);
		archiver[type].extract = FUNC(standard, xa_gzip_et_al_extract, is7za, xa_7zip_extract, unar, xa_unar_extract);
		archiver[type].add = FUNC(standard, xa_gzip_et_al_add, is7za, xa_7zip_add, lsar, NULL);
	}

	/* LHA */

	type = XARCHIVETYPE_LHA;
	path = g_find_program_in_path("lha");

	standard = (path != NULL);

	if (!standard)
	{
		if (is7z)
			/* alternative uncompressor */
			path = g_strconcat(sevenz, " -tlzh", NULL);
		else
		{
			if (lsar)
				/* alternative ... */
				path = g_strdup(lsar);
			if (unar)
				/* ... uncompressor */
				archiver[type].program[1] = g_strdup(unar);
		}
	}

	if (path)
	{
		archiver[type].program[0] = path;
		archiver[type].is_compressor = (standard && xa_lha_check_program(path));
		archiver[type].type = g_slist_append(archiver[type].type, "lha");
		archiver[type].glob = g_slist_append(archiver[type].glob, "*.lzh");

		archiver[type].ask = FUNC(standard, xa_lha_ask, is7z, xa_7zip_ask, lsar, xa_unar_ask);
		archiver[type].list = FUNC(standard, xa_lha_list, is7z, xa_7zip_list, lsar, xa_unar_list);
		archiver[type].test = FUNC(standard, xa_lha_test, is7z, xa_7zip_test, lsar, xa_unar_test);
		archiver[type].extract = FUNC(standard, xa_lha_extract, is7z, xa_7zip_extract, unar, xa_unar_extract);
		archiver[type].add = FUNC(standard, xa_lha_add, is7z, NULL, lsar, NULL);
		archiver[type].delete = FUNC(standard, xa_lha_delete, is7z, NULL, lsar, NULL);
	}

	/* lrzip */

	type = XARCHIVETYPE_LRZIP;
	path = g_find_program_in_path("lrzip");

	if (path)
		archiver[type].is_compressor = TRUE;
	else
		path = g_find_program_in_path("lrunzip");

	if (path)
	{
		xa_gzip_et_al_check_lrzip(path);

		archiver[type].program[0] = path;
		archiver[type].type = g_slist_append(archiver[type].type, "lrzip");
		archiver[type].glob = g_slist_append(archiver[type].glob, "*.lrz");

		archiver[type].ask = xa_gzip_et_al_ask;
		archiver[type].list = xa_gzip_et_al_list;
		archiver[type].test = xa_gzip_et_al_test;
		archiver[type].extract = xa_gzip_et_al_extract;
		archiver[type].add = xa_gzip_et_al_add;
	}

	/* lz4 */

	type = XARCHIVETYPE_LZ4;
	path = g_find_program_in_path("lz4");

	if (path)
		archiver[type].is_compressor = TRUE;
	else
		path = g_find_program_in_path("unlz4");

	if (path)
	{
		archiver[type].program[0] = path;
		archiver[type].type = g_slist_append(archiver[type].type, "lz4");
		archiver[type].glob = g_slist_append(archiver[type].glob, "*.lz4");
		/* Mozilla's proprietary LZ4 Frame Formats */
		archiver[type].tags = g_slist_append(archiver[type].tags, GUINT_TO_POINTER('m'));
		archiver[type].tags = g_slist_append(archiver[type].tags, "mozlz4");
		archiver[type].tags = g_slist_append(archiver[type].tags, GUINT_TO_POINTER('m' + 0x100));
		archiver[type].tags = g_slist_append(archiver[type].tags, "mozJSSClz4");

		archiver[type].ask = xa_gzip_et_al_ask;
		archiver[type].list = xa_gzip_et_al_list;
		archiver[type].test = xa_gzip_et_al_test;
		archiver[type].extract = xa_gzip_et_al_extract;
		archiver[type].add = xa_gzip_et_al_add;
	}

	/* lzip */

	type = XARCHIVETYPE_LZIP;
	path = g_find_program_in_path("lzip");

	if (!path && plzip)
		/* alternative compressor */
		path = g_strdup(plzip);

	if (path)
	{
		archiver[type].program[0] = path;
		archiver[type].is_compressor = TRUE;
		archiver[type].type = g_slist_append(archiver[type].type, "lzip");
		archiver[type].glob = g_slist_append(archiver[type].glob, "*.lz");

		archiver[type].ask = xa_gzip_et_al_ask;
		archiver[type].list = xa_gzip_et_al_list;
		archiver[type].test = xa_gzip_et_al_test;
		archiver[type].extract = xa_gzip_et_al_extract;
		archiver[type].add = xa_gzip_et_al_add;
	}

	/* lzma */

	type = XARCHIVETYPE_LZMA;
	path = g_find_program_in_path("lzma");

	if (!path && xz)
		/* alternative compressor */
		path = g_strconcat(xz, " --format=lzma", NULL);

	if (path)
		archiver[type].is_compressor = TRUE;
	else
		path = g_find_program_in_path("unlzma");

	standard = (path != NULL);

	if (!standard)
	{
		if (is7zr)
			/* alternative uncompressor */
			path = g_strconcat(sevenz, " -tlzma", NULL);
		else
		{
			if (lsar)
				/* alternative ... */
				path = g_strdup(lsar);
			if (unar)
				/* ... uncompressor */
				archiver[type].program[1] = g_strdup(unar);
		}
	}

	if (path)
	{
		archiver[type].program[0] = path;
		archiver[type].type = g_slist_append(archiver[type].type, "lzma");
		archiver[type].glob = g_slist_append(archiver[type].glob, "*.lzma");

		archiver[type].ask = FUNC(standard, xa_gzip_et_al_ask, is7zr, xa_7zip_ask, lsar, xa_unar_ask);
		archiver[type].list = FUNC(standard, xa_gzip_et_al_list, is7zr, xa_7zip_list, lsar, xa_unar_list);
		archiver[type].test = FUNC(standard, xa_gzip_et_al_test, is7zr, xa_7zip_test, lsar, NULL);
		archiver[type].extract = FUNC(standard, xa_gzip_et_al_extract, is7zr, xa_7zip_extract, unar, xa_unar_extract);
		archiver[type].add = FUNC(standard, xa_gzip_et_al_add, is7zr, NULL, lsar, NULL);
	}

	/* lzop */

	type = XARCHIVETYPE_LZOP;
	path = g_find_program_in_path("lzop");

	if (path)
		archiver[type].is_compressor = TRUE;
	else
		path = g_find_program_in_path("unlzop");

	if (path)
	{
		archiver[type].program[0] = path;
		archiver[type].type = g_slist_append(archiver[type].type, "lzop");
		archiver[type].glob = g_slist_append(archiver[type].glob, "*.lzo");

		archiver[type].ask = xa_gzip_et_al_ask;
		archiver[type].list = xa_gzip_et_al_list;
		archiver[type].test = xa_gzip_et_al_test;
		archiver[type].extract = xa_gzip_et_al_extract;
		archiver[type].add = xa_gzip_et_al_add;
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
		xa_rar_check_version(path);

		if (!rar_version)
		{
			/* reject incompatible program */
			g_free(path);
			path = NULL;
		}
	}

	standard = (path != NULL);

	if (!standard)
	{
		if (is7z)
		{
			/* alternative uncompressor */
			path = g_strconcat(sevenz, " -trar", NULL);
			archiver[type].program[1] = g_strconcat(sevenz, " -trar5", NULL);
		}
		else
		{
			if (lsar)
				/* alternative ... */
				path = g_strdup(lsar);
			if (unar)
				/* ... uncompressor */
				archiver[type].program[1] = g_strdup(unar);
		}
	}

	if (path)
	{
		archiver[type].program[0] = path;
		archiver[type].type = g_slist_append(archiver[type].type, "rar");
		archiver[type].glob = g_slist_append(archiver[type].glob, "*.rar");

		if (!standard || (rar_version >= 5))
		{
			archiver[type].type = g_slist_append(archiver[type].type, "rar5");
			archiver[type].glob = g_slist_append(archiver[type].glob, " .rar");
			archiver[type].tags = g_slist_append(archiver[type].tags, GUINT_TO_POINTER(5));
			archiver[type].tags = g_slist_append(archiver[type].tags, g_slist_last(archiver[type].type)->data);
		}

		archiver[type].ask = FUNC(standard, xa_rar_ask, is7z, xa_7zip_ask, lsar, xa_unar_ask);
		archiver[type].list = FUNC(standard, xa_rar_list, is7z, xa_7zip_list, lsar, xa_unar_list);
		archiver[type].test = FUNC(standard, xa_rar_test, is7z, xa_7zip_test, lsar, xa_unar_test);
		archiver[type].extract = FUNC(standard, xa_rar_extract, is7z, xa_7zip_extract, unar, xa_unar_extract);
		archiver[type].add = FUNC(standard, xa_rar_add, is7z, NULL, lsar, NULL);
		archiver[type].delete = FUNC(standard, xa_rar_delete, is7z, NULL, lsar, NULL);
	}

	/* RPM package */

	type = XARCHIVETYPE_RPM;
	path = g_strdup(cpio);

	standard = (path != NULL);

	if (!standard)
	{
		if (is7z)
			/* alternative uncompressor */
			path = g_strconcat(sevenz, " -trpm", NULL);
		else
		{
			if (lsar)
				/* alternative ... */
				path = g_strdup(lsar);
			if (unar)
				/* ... uncompressor */
				archiver[type].program[1] = g_strdup(unar);
		}
	}

	if (path)
	{
		archiver[type].program[0] = path;
		archiver[type].type = g_slist_append(archiver[type].type, "rpm");
		archiver[type].glob = g_slist_append(archiver[type].glob, "*.rpm");

		archiver[type].ask = FUNC(standard, xa_rpm_ask, is7z, xa_7zip_ask, lsar, xa_unar_ask);
		archiver[type].list = FUNC(standard, xa_rpm_list, is7z, xa_7zip_list, lsar, xa_unar_list);
		archiver[type].test = FUNC(standard, NULL, is7z, xa_7zip_test, lsar, xa_unar_test);
		archiver[type].extract = FUNC(standard, xa_rpm_extract, is7z, xa_7zip_extract, unar, xa_unar_extract);
	}

	/* tape archive */

	type = XARCHIVETYPE_TAR;
	path = g_find_program_in_path("gtar");

	if (!path)
		path = g_find_program_in_path("tar");

	standard = (path != NULL);

	if (!standard)
	{
		if (is7za)
			/* alternative compressor */
			path = g_strconcat(sevenz, " -ttar", NULL);
		else
		{
			if (lsar)
				/* alternative ... */
				path = g_strdup(lsar);
			if (unar)
				/* ... uncompressor */
				archiver[type].program[1] = g_strdup(unar);
		}
	}

	if (path)
	{
		archiver[type].program[0] = path;
		archiver[type].is_compressor = (standard || is7za);
		archiver[type].type = g_slist_append(archiver[type].type, "tar");
		archiver[type].glob = g_slist_append(archiver[type].glob, "*.tar");

		archiver[type].ask = FUNC(standard, xa_tar_ask, is7za, xa_7zip_ask, lsar, xa_unar_ask);
		archiver[type].list = FUNC(standard, xa_tar_list, is7za, xa_7zip_list, lsar, xa_unar_list);
		archiver[type].test = FUNC(standard, NULL, is7za, xa_7zip_test, lsar, NULL);
		archiver[type].extract = FUNC(standard, xa_tar_extract, is7za, xa_7zip_extract, unar, xa_unar_extract);
		archiver[type].add = FUNC(standard, xa_tar_add, is7za, xa_7zip_add, lsar, NULL);
		archiver[type].delete = FUNC(standard, xa_tar_delete, is7za, xa_7zip_delete, lsar, NULL);
	}

	/* xz */

	type = XARCHIVETYPE_XZ;
	path = xz;

	if (path)
		archiver[type].is_compressor = TRUE;
	else
		path = g_find_program_in_path("unxz");

	standard = (path && archiver[type].is_compressor);

	if (!standard)
	{
		if (is7zr)
		{
			g_free(path);
			/* alternative compressor */
			path = g_strconcat(sevenz, " -txz", NULL);
			archiver[type].is_compressor = TRUE;
		}
		else if (!path)
		{
			if (lsar)
				/* alternative ... */
				path = g_strdup(lsar);
			if (unar)
				/* ... uncompressor */
				archiver[type].program[1] = g_strdup(unar);
		}
		else
			standard = TRUE;
	}

	if (path)
	{
		archiver[type].program[0] = path;
		archiver[type].type = g_slist_append(archiver[type].type, "xz");
		archiver[type].glob = g_slist_append(archiver[type].glob, "*.xz");

		archiver[type].ask = FUNC(standard, xa_gzip_et_al_ask, is7zr, xa_7zip_ask, lsar, xa_unar_ask);
		archiver[type].list = FUNC(standard, xa_gzip_et_al_list, is7zr, xa_7zip_list, lsar, xa_unar_list);
		archiver[type].test = FUNC(standard, xa_gzip_et_al_test, is7zr, xa_7zip_test, lsar, xa_unar_test);
		archiver[type].extract = FUNC(standard, xa_gzip_et_al_extract, is7zr, xa_7zip_extract, unar, xa_unar_extract);
		archiver[type].add = FUNC(standard, xa_gzip_et_al_add, is7zr, xa_7zip_add, lsar, NULL);
	}

	/* zip */

	type = XARCHIVETYPE_ZIP;

	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(prefs_window->prefer_unzip)) ||
	    (!is7za && !lsar))
	{
		path = g_find_program_in_path("unzip");

		if (path)
		{
			gchar *zip = g_find_program_in_path("zip");

			if (zip)
			{
				archiver[type].program[1] = zip;
				archiver[type].is_compressor = TRUE;
			}
		}
	}
	else
		path = NULL;

	standard = (path && archiver[type].is_compressor);

	if (!standard)
	{
		if (is7za)
		{
			g_free(path);
			/* alternative compressor */
			path = g_strconcat(sevenz, " -tzip", NULL);
			archiver[type].is_compressor = TRUE;
		}
		else if (!path)
		{
			if (lsar)
				/* alternative ... */
				path = g_strdup(lsar);
			if (unar)
				/* ... uncompressor */
				archiver[type].program[1] = g_strdup(unar);
		}
		else
			standard = TRUE;
	}

	if (path)
	{
		archiver[type].program[0] = path;
		archiver[type].type = g_slist_append(archiver[type].type, "zip");
		archiver[type].glob = g_slist_append(archiver[type].glob, "*.zip");
		/* Android application package */
		archiver[type].type = g_slist_append(archiver[type].type, "apk");
		archiver[type].glob = g_slist_append(archiver[type].glob, "*.apk");
		archiver[type].tags = g_slist_append(archiver[type].tags, GUINT_TO_POINTER('a'));
		archiver[type].tags = g_slist_append(archiver[type].tags, g_slist_last(archiver[type].type)->data);
		/* comic book */
		archiver[type].type = g_slist_append(archiver[type].type, "cbz");
		archiver[type].glob = g_slist_append(archiver[type].glob, "*.cbz");
		archiver[type].tags = g_slist_append(archiver[type].tags, GUINT_TO_POINTER('c'));
		archiver[type].tags = g_slist_append(archiver[type].tags, g_slist_last(archiver[type].type)->data);
		/* electronic publication */
		archiver[type].type = g_slist_append(archiver[type].type, "epub");
		archiver[type].glob = g_slist_append(archiver[type].glob, "*.epub");
		archiver[type].tags = g_slist_append(archiver[type].tags, GUINT_TO_POINTER('e'));
		archiver[type].tags = g_slist_append(archiver[type].tags, g_slist_last(archiver[type].type)->data);
		/* java archive */
		archiver[type].type = g_slist_append(archiver[type].type, "jar");
		archiver[type].glob = g_slist_append(archiver[type].glob, "*.jar");
		archiver[type].tags = g_slist_append(archiver[type].tags, GUINT_TO_POINTER('j'));
		archiver[type].tags = g_slist_append(archiver[type].tags, g_slist_last(archiver[type].type)->data);
		/* OpenOffice.org extension */
		archiver[type].type = g_slist_append(archiver[type].type, "oxt");
		archiver[type].glob = g_slist_append(archiver[type].glob, "*.oxt");
		archiver[type].tags = g_slist_append(archiver[type].tags, GUINT_TO_POINTER('o'));
		archiver[type].tags = g_slist_append(archiver[type].tags, g_slist_last(archiver[type].type)->data);
		/* XPInstall */
		archiver[type].type = g_slist_append(archiver[type].type, "xpi");
		archiver[type].glob = g_slist_append(archiver[type].glob, "*.xpi");
		archiver[type].tags = g_slist_append(archiver[type].tags, GUINT_TO_POINTER('x'));
		archiver[type].tags = g_slist_append(archiver[type].tags, g_slist_last(archiver[type].type)->data);

		archiver[type].ask = FUNC(standard, xa_zip_ask, is7za, xa_7zip_ask, lsar, xa_unar_ask);
		archiver[type].list = FUNC(standard, xa_zip_list, is7za, xa_7zip_list, lsar, xa_unar_list);
		archiver[type].test = FUNC(standard, xa_zip_test, is7za, xa_7zip_test, lsar, xa_unar_test);
		archiver[type].extract = FUNC(standard, xa_zip_extract, is7za, xa_7zip_extract, unar, xa_unar_extract);
		archiver[type].add = FUNC(standard, xa_zip_add, is7za, xa_7zip_add, lsar, NULL);
		archiver[type].delete = FUNC(standard, xa_zip_delete, is7za, xa_7zip_delete, lsar, NULL);
	}

	/* Zstandard */

	type = XARCHIVETYPE_ZSTD;
	path = zstd;                // either zstd or unzstd

	if (path)
	{
		archiver[type].program[0] = path;
		archiver[type].type = g_slist_append(archiver[type].type, "zstd");
		archiver[type].glob = g_slist_append(archiver[type].glob, "*.zst");

		archiver[type].ask = xa_gzip_et_al_ask;
		archiver[type].list = xa_gzip_et_al_list;
		archiver[type].test = xa_gzip_et_al_test;
		archiver[type].extract = xa_gzip_et_al_extract;
		archiver[type].add = xa_gzip_et_al_add;
	}

	/* compressed tar */

	if (archiver[XARCHIVETYPE_TAR].ask == xa_tar_ask || archiver[XARCHIVETYPE_TAR].ask == xa_unar_ask)
	{
		struct
		{
			XArchiveType compressor;
			gchar *type[2];
			gchar *glob[2];
		} compressed_tar_infos[] =
		{
			{XARCHIVETYPE_BZIP2, {"tar.bzip2", NULL}, {"*.tar.bz2", "*.tbz2"}},
			{XARCHIVETYPE_COMPRESS, {"tar.compress", NULL}, {"*.tar.Z", ""}},
			{XARCHIVETYPE_GZIP, {"tar.gzip", NULL}, {"*.tar.gz", "*.tgz"}},
			{XARCHIVETYPE_LRZIP, {"tar.lrzip", NULL}, {"*.tar.lrz", "*.tlrz"}},
			{XARCHIVETYPE_LZ4, {"tar.lz4", NULL}, {"*.tar.lz4", ""}},
			{XARCHIVETYPE_LZIP, {"tar.lzip", NULL}, {"*.tar.lz", ""}},
			{XARCHIVETYPE_LZMA, {"tar.lzma", NULL}, {"*.tar.lzma", "*.tlz"}},
			{XARCHIVETYPE_LZOP, {"tar.lzop", NULL}, {"*.tar.lzo", "*.tzo"}},
			{XARCHIVETYPE_XZ, {"tar.xz", NULL}, {"*.tar.xz", "*.txz"}},
			{XARCHIVETYPE_ZSTD, {"tar.zstd", NULL}, {"*.tar.zst", "*.tzst"}},
			{XARCHIVETYPE_UNKNOWN, {NULL, NULL}, {NULL, NULL}}
		}, *i;

		for (i = compressed_tar_infos; i->compressor != XARCHIVETYPE_UNKNOWN; i++)
		{
			if (archiver[i->compressor].type)
			{
				type = i->compressor;

				if (!xa_get_compressed_tar_type(&type))
					continue;

				if ((archiver[XARCHIVETYPE_TAR].ask == xa_tar_ask && archiver[i->compressor].ask == xa_gzip_et_al_ask) ||
				    (archiver[XARCHIVETYPE_TAR].ask == xa_unar_ask && archiver[i->compressor].ask == xa_unar_ask))
				{
					archiver[type].is_compressor = archiver[i->compressor].is_compressor;
					archiver[type].type = g_slist_append(archiver[type].type, i->type[0]);
					archiver[type].glob = g_slist_append(archiver[type].glob, i->glob[0]);
					archiver[type].type = g_slist_append(archiver[type].type, i->type[1]);
					archiver[type].glob = g_slist_append(archiver[type].glob, i->glob[1]);

					archiver[type].ask = archiver[XARCHIVETYPE_TAR].ask;
					archiver[type].list = archiver[XARCHIVETYPE_TAR].list;
					archiver[type].extract = archiver[XARCHIVETYPE_TAR].extract;
					archiver[type].add = archiver[XARCHIVETYPE_TAR].add;
					archiver[type].delete = archiver[XARCHIVETYPE_TAR].delete;
				}
			}
		}
	}

	g_free(lbzip2);
	g_free(lsar);
	g_free(pbzip2);
	g_free(pigz);
	g_free(plzip);
	g_free(unar);
}

static XArchive *xa_init_structure_from_cmd_line (char *filename)
{
	XArchive *archive;
	ArchiveType xa;
	gchar *path;

	xa = xa_detect_archive_type(filename);

	if (xa.type == XARCHIVETYPE_UNKNOWN || xa.type == XARCHIVETYPE_NOT_FOUND)
		return NULL;

	if (!archiver[xa.type].list)
	{
		xa_show_message_dialog(NULL, GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK, _("Sorry, this archive format is not supported:"), _("The proper archiver is not installed!"));
		return NULL;
	}

	archive = xa_init_archive_structure(xa);

	if (archive == NULL)
	{
		xa_show_message_dialog (NULL,GTK_DIALOG_MODAL,GTK_MESSAGE_ERROR,GTK_BUTTONS_OK,_("Can't allocate memory for the archive structure!"),"" );
		return NULL;
	}

	path = xa_make_full_path(filename);

	archive->path[0] = g_strdup(path);
	archive->path[1] = xa_escape_bad_chars(path, ESCAPES);

	g_free(path);

	return archive;
}

static void xa_cmd_line_error (const char *argv1, const gchar *header)
{
	if (argv1)
		xa_show_message_dialog(NULL, GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK, header, _("Can't open the archive!\n"));
	else
		xa_show_message_dialog(NULL, GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK, header, _("You missed the archive name!\n"));
}

int main (int argc, char **argv)
{
	GError *cli_error = NULL;
	XArchive *archive;
	gchar *current_dir;
	int i, result = -1;

#ifdef ENABLE_NLS
	GTK_COMPAT_PARAMETER_STRING;
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
	gtk_init_with_args(&argc, &argv, _("[ARCHIVE]"), entries, GETTEXT_PACKAGE, &cli_error);

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
        g_print (_("Maintained by "));
        g_print (MAINTAINER ".\n\n");
        g_print ("%s\n", "Copyright (c) " COPYRIGHT_YEAR);
        g_print ("\t%s\n\n", COPYRIGHT_HOLDER " - Colossus <colossus73@gmail.com>");
        g_print (_("Please report bugs to <%s>."), PACKAGE_BUGREPORT);
        g_print ("\n");

        return EXIT_SUCCESS;
    }

	xdg_open = g_find_program_in_path("xdg-open");

	prefs_window   = xa_create_prefs_dialog();
	extract_window = xa_create_extract_dialog();
	add_window     = xa_create_add_dialog();
	multi_extract_window = xa_create_multi_extract_dialog();

	xa_prefs_load_options(prefs_window);
	xa_check_available_archivers();
	xa_prefs_adapt_options(prefs_window);

	if (opt_extract || opt_extract_path || opt_ensure_dir || opt_multi_extract || opt_add || opt_compress || opt_info)
	{
		archive = xa_init_structure_from_cmd_line (argv[1]);
		g_print(PACKAGE_NAME " 0.5.4 \xC2\xA9  " COPYRIGHT_YEAR " " COPYRIGHT_HOLDER "\n");
		g_print(PACKAGE_NAME " %-8s " MAINTAINER "\n", VERSION);

		/* Switches -d and -x */
		if (opt_ensure_dir || opt_extract_path)
		{
			gchar *extraction_dir = NULL;

			if (!archive)
			{
				xa_cmd_line_error(argv[1], _("Can't extract files from the archive:"));
				goto leave;
			}

			if (!archive->can_extract)
			{
				xa_show_message_dialog(NULL, GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK, _("Can't extract files from the archive:"), argv[1]);
				goto leave;
			}

			xa_detect_encrypted_archive(archive);

			if (archive->status == XARCHIVESTATUS_ERROR)
				goto leave;

			if (archive->has_password)
			{
				if (!xa_check_password(archive))
					goto leave;
			}

			if (opt_ensure_dir)
			{
				current_dir = g_get_current_dir();

				if (xa_has_containing_directory(archive))
					extraction_dir = g_strdup(current_dir);
				else
				{
					extraction_dir = xa_create_containing_directory(archive, current_dir);

					if (!extraction_dir)
					{
						xa_show_message_dialog(NULL, GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK, _("Can't create directory!"), "");
						g_free(current_dir);
						goto leave;
					}
				}

				opt_extract_path = extraction_dir;
				g_free(current_dir);
			}

			archive->do_full_path = TRUE;
			archive->do_overwrite = TRUE;
			archive->extraction_dir = xa_escape_bad_chars(opt_extract_path, ESCAPES);
			g_free(extraction_dir);

			archive->status = XARCHIVESTATUS_EXTRACT;
			(*archive->archiver->extract)(archive, NULL);
		}
		/* Switch -e */
		else if (opt_extract)
		{
			if (!archive)
			{
				xa_cmd_line_error(argv[1], _("Can't extract files from the archive:"));
				goto leave;
			}

			if (!archive->can_extract)
			{
				xa_show_message_dialog(NULL, GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK, _("Can't extract files from the archive:"), argv[1]);
				goto leave;
			}

			xa_detect_encrypted_archive(archive);

			if (archive->status == XARCHIVESTATUS_ERROR)
				goto leave;

			if (archive->has_password)
			{
				if (!xa_check_password(archive))
					goto leave;
			}

			xa_set_extract_dialog_options(extract_window,0,archive);
			xa_parse_extract_dialog_options (archive,extract_window,NULL);
			gtk_widget_destroy (extract_window->dialog1);
			g_free (extract_window);
		}
		/* Switch -m */
		else if (opt_multi_extract)
		{
			Multi_extract_data *multi_extract;
			gchar *arg;

			multi_extract = xa_create_multi_extract_dialog();

			for (i = 1; i < argc; i++)
			{
				arg = xa_make_full_path(argv[i]);

				if (!g_file_test(arg, G_FILE_TEST_IS_DIR))
					xa_multi_extract_dialog_add_file(arg, multi_extract);

				g_free(arg);
			}

			xa_multi_extract_dialog(multi_extract);
			gtk_widget_destroy (multi_extract->multi_extract);
			g_free(multi_extract);
		}
		/* Switch -c */
		else if (opt_compress)
		{
			if (*opt_compress == 0)
			{
				xa_show_message_dialog(NULL, GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK, _("Can't perform this action:"), _("You haven't selected any files to add!"));
				g_free(opt_compress);
				goto leave;
			}

			archive = xa_new_archive_dialog(opt_compress, NULL);

			if (archive == NULL)
			{
				g_free(opt_compress);
				goto leave;
			}

			if (archive->path[0] != NULL)
			{
				gchar *fname;
				gboolean is_dir;
				GSList *files = NULL;

				fname = xa_make_full_path(opt_compress);
				is_dir = g_file_test(fname, G_FILE_TEST_IS_DIR);
				g_free(fname);

				if (SINGLE_FILE_COMPRESSOR(archive) && (is_dir || (argc > 1)))
				{
					xa_show_message_dialog(NULL, GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK, _("An error occurred!"), _("The archiver doesn't support compression of directories or multiple files!"));
					g_free(opt_compress);
					goto leave;
				}

				xa_create_working_directory(archive);

				archive->child_dir = g_path_get_dirname(opt_compress);

				fname = g_path_get_basename(opt_compress);
				files = g_slist_append(files, fname);
				g_free(opt_compress);

				for (i = 1; i < argc; i++)
				{
					fname = g_path_get_basename(argv[i]);
					files = g_slist_append(files, fname);
				}

				xa_execute_add_commands(archive, files, TRUE);
			}
		}
		/* Switch -a */
		else if (opt_add)
		{
			if (!archive)
			{
				xa_cmd_line_error(argv[1], _("Can't add files to the archive:"));
				goto leave;
			}
			if (!archive->can_add || SINGLE_FILE_COMPRESSOR(archive))
			{
				xa_show_message_dialog(NULL, GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK, _("Can't add files to the archive:"), argv[1]);
				goto leave;
			}

			xa_detect_encrypted_archive(archive);

			if (archive->status == XARCHIVESTATUS_ERROR)
				goto leave;

			if (archive->has_password)
			{
				if (!xa_check_password(archive))
					goto leave;
			}

			xa_set_add_dialog_options(add_window, archive);
			xa_parse_add_dialog_options(archive, add_window);
			gtk_widget_destroy(add_window->dialog1);
			g_free(add_window);
		}
		/* Switch -i */
		else if (opt_info)
		{
			GSList *list;

			for (i = XARCHIVETYPE_FIRST; i < XARCHIVETYPE_TYPES; i++)
			{
				list = archiver[i].type;

				while (list)
				{
					if (list->data)
					{
						if (strncmp(list->data, "tar.", 4) == 0)
							break;

						g_print("%s", (char *) list->data);
					}

					list = list->next;

					if (list && list->data)
						g_print(", ");
					else
					{
						g_print(":\n  %s\n", archiver[i].program[0]);

						if (archiver[i].program[1])
							g_print("  %s\n", archiver[i].program[1]);
					}
				}
			}
		}

		result = EXIT_SUCCESS;

leave:

		if (progress)
		{
			gtk_widget_destroy(progress->window);
			g_free(progress);
			progress = NULL;
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
			socket_info.lock_socket_tag = g_io_add_watch(socket_info.read_ioc, G_IO_IN | G_IO_ERR, socket_lock_input_cb, xa_main_window);
		}
		#endif
		gtk_main ();

		result = EXIT_SUCCESS;
	}

	for (i = XARCHIVETYPE_FIRST; i < XARCHIVETYPE_TYPES; i++)
	{
		g_free(archiver[i].program[0]);
		g_free(archiver[i].program[1]);
		g_slist_free(archiver[i].type);
		g_slist_free(archiver[i].glob);
		g_slist_free(archiver[i].tags);
	}

	g_free(xdg_open);

	return result;
}
