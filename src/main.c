/*
 *  Xarchiver
 *
 *  Copyright (C) 2005 Giuseppe Torelli - Colossus
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

extern gchar *path;
GError *cl_error = NULL;
gboolean file_to_open;

static GOptionEntry entries[] = {
  { "extract-to=FOLDER", 'x', 0, G_OPTION_ARG_FILENAME, &extract_path, "Extract the archive to the specified folder and quits.", NULL },
  { "extract", 'e', 0, G_OPTION_ARG_NONE, NULL, "Extract the archive by asking the destination folder and quits.", NULL },
  { "add-to=ARCHIVE", 'd', 0, G_OPTION_ARG_FILENAME, &path, "Add files to the specified archive and quits.", NULL },
  { "add", 'a', 0, G_OPTION_ARG_NONE, NULL, "Add files asking the name of the archive and quits.", NULL },
  { NULL }
};

int main (int argc, char *argv[])
{
  #ifdef ENABLE_NLS
  bindtextdomain (GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR);
  bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
  textdomain (GETTEXT_PACKAGE);
  #endif
  gtk_set_locale();
  gtk_init (&argc, &argv);
  add_pixmap_directory (PACKAGE_DATA_DIR "/" PACKAGE "/pixmaps");
  
  context = g_option_context_new ("[archive to open]");
  g_option_context_add_main_entries (context, entries, GETTEXT_PACKAGE);
  g_option_context_add_group (context, gtk_get_option_group (FALSE));
  if ( ! g_option_context_parse (context, &argc, &argv, &cl_error) )
    {
        g_print (_("xarchiver: %s\nTry xarchiver --help for more information.\n"),cl_error->message);
        g_error_free (cl_error);
        return 0;
    }
 
  GetAvailableCompressors();
  g_option_context_free ( context );
  ArchiveSuffix = g_list_reverse (ArchiveSuffix);
  ArchiveType = g_list_reverse (ArchiveType);
  CurrentArchiveType = -1;
  Files_to_Add = NULL;
  MainWindow = create_MainWindow ();
  ShowShellOutput (NULL,FALSE);
  gtk_window_set_position ( GTK_WINDOW (MainWindow),GTK_WIN_POS_CENTER);
  gtk_window_set_default_size( GTK_WINDOW(MainWindow), 600, 400);
  g_signal_connect (MainWindow, "delete_event", G_CALLBACK (on_quit1_activate), NULL);
  gtk_widget_show (MainWindow);
  SetButtonState (1,1,0,0,0);
  Update_StatusBar ( _("Ready."));
  if ( argc == 2)
    {
        path = g_strdup( argv[1] );
        on_open1_activate ( NULL , path);
    }
  else path = NULL; 
  gtk_main ();
  g_list_free ( ArchiveSuffix);
  g_list_free ( ArchiveType);
  return 0;
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

void SetButtonState (gboolean New, gboolean Open,gboolean AddFile,gboolean AddFolder,gboolean Extract)
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

gboolean ParseCommandLine (char *param)
 {
	char option;
 	gint i;
	if ( *param == '-' )
	{
		param++;
		option = toupper ( *param );
		if (option == 'V')
		{
			g_print ("%s\n",VERSION);
			return FALSE;
		}
		else if (option == 'H')
		{
			g_printf ("Xarchiver %s\n", VERSION); 
			g_printf ("By Giuseppe Torelli - Colossus73 <gt67@users.sourceforge.net>\n\n");
			g_printf ("  Usage:  %s %s\n"
			"options:\n"
			"\t-h\t%s\n"
			"\t-v\t%s\n"
			"file:\t\t%s\n",
			"xarchiver [options]", "file\n",
			"Display this help.",
			"show version number.",
			"archive to open.");
			return FALSE;
		}
	}
	
	g_print (_("xarchiver: invalid option %s\nTry xarchiver -h for more information.\n"),param);
	return FALSE;
}





