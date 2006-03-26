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
 
#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include "main.h"
#include "interface.h"
#include "support.h"

extern gchar *path;

GList *ArchiveSuffix = NULL;
GList *ArchiveName = NULL;
gboolean file_to_open;

int main (int argc, char *argv[])
{
  gtk_init (&argc, &argv);
  add_pixmap_directory (PACKAGE_DATA_DIR "/" PACKAGE "/pixmaps");
  int param = 1;
  while (param < argc)
  {
	if ( ! ParseCommandLine (argv[param++]) ) return 0;
  }
  GetAvailableCompressors();
  ArchiveSuffix = g_list_reverse (ArchiveSuffix);
  ArchiveName = g_list_reverse (ArchiveName);
  MainWindow = create_MainWindow ();
  ShowShellOutput();
  gtk_window_set_position ( GTK_WINDOW (MainWindow),GTK_WIN_POS_CENTER);
  gtk_window_set_default_size( GTK_WINDOW(MainWindow), 600, 400);
  g_signal_connect (MainWindow, "destroy", G_CALLBACK (gtk_main_quit), NULL);
  gtk_widget_show (MainWindow);
  SetButtonState (1,1,0,0,0);
  if (file_to_open) on_open1_activate ( NULL,NULL);
  gtk_main ();
  g_list_free ( ArchiveSuffix);
  g_list_free ( ArchiveName);
  return 0;
}

//TODO add support to load the conf of Xarchiver

//g_get_home_dir ()

void GetAvailableCompressors()
{
	if ( g_find_program_in_path("bzip2"))
	{
		ArchiveName = g_list_prepend ( ArchiveName, ".bz2");
		ArchiveSuffix = g_list_prepend ( ArchiveSuffix, "*.bz2");
	}
	
	if ( g_find_program_in_path("gzip"))
	{
		ArchiveName = g_list_prepend ( ArchiveName, ".gz");
		ArchiveSuffix = g_list_prepend ( ArchiveSuffix, "*.gz");
	}

	if ( g_find_program_in_path("rar"))
	{
		ArchiveName = g_list_prepend ( ArchiveName, ".rar");
		ArchiveSuffix = g_list_prepend ( ArchiveSuffix, "*.rar");
	}
		
	if ( g_find_program_in_path("tar"))
	{
		ArchiveName = g_list_prepend ( ArchiveName, ".tar");
		ArchiveSuffix = g_list_prepend ( ArchiveSuffix, "*.tar");
		if ( g_list_find ( ArchiveName , ".bz2") )
		{
			ArchiveName = g_list_prepend ( ArchiveName, ".tar.bz2");
			//The following to avoid double filter when opening
			ArchiveSuffix = g_list_prepend ( ArchiveSuffix, "");
		}
		if ( g_list_find ( ArchiveName , ".gz") )
		{
			ArchiveName = g_list_prepend ( ArchiveName, ".tar.gz");
			//The following to avoid double filter when opening
			ArchiveSuffix = g_list_prepend ( ArchiveSuffix, "");
			ArchiveName = g_list_prepend ( ArchiveName, ".tgz");
			ArchiveSuffix = g_list_prepend ( ArchiveSuffix, "*.tgz");
		}
		
	}

	if ( g_find_program_in_path("zip"))
	{
		ArchiveName = g_list_prepend ( ArchiveName, ".jar");
		ArchiveSuffix = g_list_prepend ( ArchiveSuffix, "*.jar");
		
		ArchiveName = g_list_prepend ( ArchiveName, ".zip");
		ArchiveSuffix = g_list_prepend ( ArchiveSuffix, "*.zip");
	}
}

void SetButtonState (gboolean New, gboolean Open,gboolean AddFile,gboolean AddFolder,gboolean Extract)
{
	gtk_widget_set_sensitive ( New_button, New);
	gtk_widget_set_sensitive ( Open_button, Open);
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
	else
	{
		file_to_open = TRUE;
		path = param;
		return TRUE;
	}
	g_print ("xarchiver: invalid option %s\nTry xarchiver -h for more information.\n",param);
	return FALSE;
}





