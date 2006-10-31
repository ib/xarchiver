/*
 *  Copyright (C) 2006 Giuseppe Torelli - <colossus73@gmail.com>
 *  Copyright (C) 2006 Lukasz 'Sil2100' Zemczak - <sil2100@vexillium.org>
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
#include "deb.h"
#include "string_utils.h"

void OpenDeb ( XArchive *archive )
{
	gchar *command = NULL;
	gchar *archive_no_path = NULL;
	gboolean result;

	archive_no_path = g_strrstr (archive->escaped_path,"/");
	archive_no_path++;
	archive->tmp = g_strconcat (" /tmp/",archive_no_path,NULL);

	/* Let's copy the .deb archive to /tmp */
	command = g_strconcat ("cp ",archive->escaped_path,archive->tmp,NULL);
	result = xa_run_command ( command , 0);
	g_free (command);
	if (result == FALSE)
		return;

	/* Ok, let's now extract the .deb archive with ar */
	chdir ("/tmp");
	command = g_strconcat ("ar xv" , archive->tmp, NULL);
	result = xa_run_command ( command , 0);
	g_free (command);
	g_free (archive->tmp);
	archive->tmp = g_strdup ("/tmp/data.tar.gz");

	unlink ("/tmp/control.tar.gz");
	unlink ("/tmp/debian-binary");
	unlink (archive_no_path);

	if (result == FALSE)
		return;

	/* Finally, let's show the content in /tmp/data.tar.gz */
	command = g_strconcat ("tar tfzv /tmp/data.tar.gz", NULL);
	archive->dummy_size = 0;
	archive->nr_of_files = 0;
	archive->nr_of_dirs = 0;
	archive->format ="DEB";
	archive->parse_output = TarOpen;
	SpawnAsyncProcess ( archive , command , 0, 0);
	g_free (command);

	if (archive->child_pid == 0)
		return;

	char *names[]= {(_("Filename")),(_("Permissions")),(_("Symbolic Link")),(_("Owner/Group")),(_("Size")),(_("Date")),(_("Time"))};
	GType types[]= {G_TYPE_STRING,G_TYPE_STRING,G_TYPE_STRING,G_TYPE_STRING,G_TYPE_UINT64,G_TYPE_STRING,G_TYPE_STRING};
	xa_create_liststore ( 7, names , (GType *)types, archive );
}
