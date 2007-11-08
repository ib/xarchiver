/*
 *  Copyright (C) 2007 Giuseppe Torelli - <colossus73@gmail.com>
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

extern void xa_create_liststore ( XArchive *archive, gchar *columns_names[]);

void xa_open_deb ( XArchive *archive )
{
	gchar *command = NULL;
	gchar *archive_no_path = NULL;
	gboolean result;
	unsigned short int i;
	gchar tmp_dir[14] = "";

	/* Create a unique tmp dir in /tmp */
	result = xa_create_temp_directory (tmp_dir);
	if (result == 0)
		return;

	archive_no_path = g_strrstr (archive->escaped_path,"/");
	if (archive_no_path == NULL)
		archive->tmp = g_strconcat (" ",tmp_dir,"/",archive->escaped_path,NULL);
	else
	{
		archive_no_path++;
		archive->tmp = g_strconcat (" ",tmp_dir,"/",archive_no_path,NULL);
	}

	/* Copy the .deb archive to the unique dir */
	command = g_strconcat ("cp ",archive->escaped_path,archive->tmp,NULL);
	result = xa_run_command (archive,command,0);
	g_free (command);
	if (result == FALSE)
		return;

	/* Ok, let's now extract the .deb archive with ar */
	chdir (tmp_dir);
	command = g_strconcat ("ar xv" , archive->tmp, NULL);
	result = xa_run_command (archive,command,0);
	g_free (command);
	g_free (archive->tmp);

	archive->tmp = g_strdup(tmp_dir);
	chdir (tmp_dir);
	unlink ("control.tar.gz");
	unlink ("debian-binary");
	/* Delete the .deb archive copied to the unique dir */
	if (archive_no_path != NULL)
		unlink (archive_no_path);
	else
		unlink (archive->escaped_path);

	if (result == FALSE)
		return;

	/* Finally, let's show the content of data.tar.gz in the unique dir */
	command = g_strconcat ("tar tfzv ",tmp_dir,"/data.tar.gz", NULL);
	archive->has_properties = archive->can_extract = TRUE;
	archive->can_add = archive->has_test = archive->has_sfx = FALSE;
	archive->dummy_size = 0;
	archive->nr_of_files = 0;
	archive->nr_of_dirs = 0;
	archive->nc = 7;
	archive->format = "DEB";
	archive->parse_output = xa_get_tar_line_content;
	xa_spawn_async_process (archive,command);
	g_free (command);

	if (archive->child_pid == 0)
		return;

	char *names[]= {(_("Points to")),(_("Permissions")),(_("Owner/Group")),(_("Size")),(_("Date")),(_("Time")),NULL};
	GType types[]= {GDK_TYPE_PIXBUF,G_TYPE_STRING,G_TYPE_STRING,G_TYPE_STRING,G_TYPE_STRING,G_TYPE_UINT64,G_TYPE_STRING,G_TYPE_STRING,G_TYPE_POINTER};
	archive->column_types = g_malloc0(sizeof(types));
	for (i = 0; i < 9; i++)
		archive->column_types[i] = types[i];
	xa_create_liststore (archive,names);
}
