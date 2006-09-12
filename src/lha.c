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
#include "lha.h"

void OpenLha ( XArchive *archive )
{
	gchar *command;
	gchar *lha;

	command = g_strconcat ("lha l " , archive->escaped_path, NULL);
	archive->dummy_size = 0;
	archive->nr_of_files = 0;
	archive->nr_of_dirs = 0;
	archive->format ="LHA";
	archive->parse_output = LhaOpen;
	SpawnAsyncProcess ( archive , command , 0, 0);
	g_free (command);

	if (archive->child_pid == 0)
		return;

	char *names[]= {(_("Filename")),(_("Permissions")),(_("UID/GID")),(_("Size")),(_("Ratio")),(_("Timestamp"))};
	GType types[]= {G_TYPE_STRING,G_TYPE_STRING,G_TYPE_STRING,G_TYPE_UINT64,G_TYPE_STRING,G_TYPE_STRING};
	xa_create_liststore(6, names, (GType *)types);
}

gboolean LhaOpen (GIOChannel *ioc, GIOCondition cond, gpointer data)
{
	XArchive *archive = data;
	gchar *line	= NULL;
	GValue *filename    = NULL;
	GValue *permissions = NULL;
	GValue *owner       = NULL;
	GValue *size        = NULL;
	GValue *ratio       = NULL;
	GValue *timestamp   = NULL;
	GIOStatus status = G_IO_STATUS_NORMAL;
	gchar *_size		= NULL;
	gchar *temp_filename = NULL;
	unsigned short int a = 0, n = 0, num;

	if (cond & (G_IO_IN | G_IO_PRI) )
	{
    // We don't need the first two lines. No actual data there.
    g_io_channel_read_line(ioc, &line, NULL, NULL, NULL);
    status = g_io_channel_read_line(ioc, &line, NULL, NULL, NULL);
    if(status == G_IO_STATUS_ERROR || status == G_IO_STATUS_EOF)
      goto done;

    do
    {
      status = g_io_channel_read_line(ioc, &line, NULL, NULL, NULL);
      if (line == NULL || (strncmp(line, "---------- -", 12) == 0))
        break;

      filename    = g_new0(GValue, 1);
      permissions = g_new0(GValue, 1);
      owner       = g_new0(GValue, 1);
      size        = g_new0(GValue, 1);
      ratio       = g_new0(GValue, 1);
      timestamp   = g_new0(GValue, 1);

      permissions = g_value_init(permissions, G_TYPE_STRING);
      g_value_set_string(permissions, g_strndup(line, 10));

      owner = g_value_init(owner, G_TYPE_STRING);
      g_value_set_string(owner, g_strndup(&line[11], 11));

      // Parse the size.
      num = strlen(line);
      for(n = 23;n < num;n++)
        if(line[n] != ' ')
          break;

      a = n;
      for(;n < num;n++)
        if(line[n] == ' ')
          break;

      size = g_value_init(size, G_TYPE_UINT64);
      _size = g_strndup(&line[a], n - a);
      g_value_set_uint64 (size, atoll(_size));
      g_free(_size);

      ratio = g_value_init(ratio, G_TYPE_STRING);
      g_value_set_string(ratio, g_strndup(&line[31], 7));

      timestamp = g_value_init(timestamp, G_TYPE_STRING);
      g_value_set_string(timestamp, g_strndup(&line[38], 13));

      filename = g_value_init(filename, G_TYPE_STRING);
      g_value_set_string(filename, g_strndup(&line[51], num - 51 - 1));

      archive->row = g_list_prepend(archive->row, filename);
      archive->row = g_list_prepend(archive->row, permissions);
      archive->row = g_list_prepend(archive->row, owner);
      archive->row = g_list_prepend(archive->row, size);
      archive->row = g_list_prepend(archive->row, ratio);
      archive->row = g_list_prepend(archive->row, timestamp);

      archive->dummy_size += g_value_get_uint64(size);

      if(strstr(g_value_get_string(permissions), "d") == NULL)
      {
        archive->nr_of_files++;
      }
      else
      {
        archive->nr_of_dirs++;
      }
      g_free(line);
      archive->row_cnt++;

      if (archive->row_cnt > 99)
      {
        xa_append_rows(archive, 6);
        archive->row_cnt = 0;
      }
    }
		while (status == G_IO_STATUS_NORMAL);

    if (status == G_IO_STATUS_ERROR || status == G_IO_STATUS_EOF)
      goto done;
	}
	else if (cond & (G_IO_ERR | G_IO_HUP | G_IO_NVAL) )
	{
done:
		g_io_channel_shutdown(ioc, TRUE, NULL);
		g_io_channel_unref(ioc);
		xa_append_rows(archive, 6);
		return FALSE;
	}
	return TRUE;
}

