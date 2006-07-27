/*
 *  Copyright (C) 2006 Giuseppe Torelli - <colossus73@gmail.com>
 *  Copyright (C) 2006 Stephan Arts - <psybsd@gmail.com>
 *  Copyright (C) 2006 Benedikt Meurer - <benny@xfce.org>
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
#include "tar.h"
 
void OpenTar ( XArchive *archive )
{
	gchar *command;
	gchar *tar;
  
	tar = g_find_program_in_path ("gtar");
	if (tar == NULL)
		tar = g_strdup ("tar");

	command = g_strconcat (tar, " tfv " , archive->escaped_path, NULL);
	archive->dummy_size = 0;
	archive->nr_of_files = 0;
	archive->nr_of_dirs = 0;
	archive->format ="TAR";
	archive->parse_output = TarOpen;

	SpawnAsyncProcess ( archive , command , 0, 0);

	g_free (command);
	g_free (tar);

	if (archive->child_pid == 0)
		return;

	char *names[]= {(_("Filename")),(_("Permissions")),(_("Owner/Group")),(_("Size")),(_("Date")),(_("Time"))};
	GType types[]= {G_TYPE_STRING,G_TYPE_STRING,G_TYPE_STRING,G_TYPE_UINT64,G_TYPE_STRING,G_TYPE_STRING};
	xa_create_liststore ( 6, names , (GType *)types );
}

gboolean TarOpen (GIOChannel *ioc, GIOCondition cond, gpointer data)
{
	XArchive *archive = data;
	gchar *line	= NULL;
	GValue *filename    = NULL;
	GValue *permissions = NULL;
	GValue *owner       = NULL;
	GValue *size        = NULL;
	GValue *date        = NULL;
	GValue *time        = NULL;
  GIOStatus status = G_IO_STATUS_NORMAL;
	gchar *_size		= NULL;
	unsigned short int a = 0, n = 0;
	
	if (cond & (G_IO_IN | G_IO_PRI) )
	{
    do
    {
      status = g_io_channel_read_line(ioc, &line, NULL,NULL,NULL);
      if (line == NULL)
        break;

      filename    = g_new0(GValue, 1);
      permissions = g_new0(GValue, 1);
      owner       = g_new0(GValue, 1);
      size        = g_new0(GValue, 1);
      date        = g_new0(GValue, 1);
      time        = g_new0(GValue, 1);

      for(n = 13; n < strlen(line); n++)
        if(line[n] == ' ') break;
      permissions = g_value_init(permissions, G_TYPE_STRING);
      g_value_set_string(permissions, g_strndup(line, 10));
    
      owner = g_value_init(owner , G_TYPE_STRING);
      g_value_set_string(owner, g_strndup(&line[11], n-11));

      for(; n < strlen(line); n++)
        if(line[n] >= '0' && line[n] <= '9')
          break;
        a = n;
        for(; n < strlen(line); n++)
          if(line[n] == ' ')
            break;

      size = g_value_init(size, G_TYPE_UINT64);
      _size = g_strndup(&line[a], n-a);
      g_value_set_uint64(size, atoll ( _size ));
      g_free (_size);
      a = n++;
      for(; n < strlen(line); n++) // DATE
      if(line[n] == ' ')
        break;
      date = g_value_init(date, G_TYPE_STRING);
      g_value_set_string ( date, g_strndup (&line[n-10], 10) );

      a = n++;
      for(; n < strlen(line); n++) // TIME
      if(line[n] == ' ') break;
      time = g_value_init(time, G_TYPE_STRING);
      g_value_set_string ( time, g_strndup (&line[n-8], 8) );

      filename = g_value_init(filename, G_TYPE_STRING);
      g_value_set_string(filename, g_strstrip(g_strndup(&line[n], strlen(line)-n-1)));

      archive->row = g_list_prepend(archive->row, filename);
      archive->row = g_list_prepend(archive->row, permissions);
      archive->row = g_list_prepend(archive->row, owner);
      archive->row = g_list_prepend(archive->row, size);
      archive->row = g_list_prepend(archive->row, date);
      archive->row = g_list_prepend(archive->row, time);

      archive->dummy_size += g_value_get_uint64 (size);
      if ( strstr (g_value_get_string (permissions) , "d") == NULL )
        archive->nr_of_files++;
      else
        archive->nr_of_dirs++;
      g_free(line);
      archive->row_cnt++;
      if (archive->row_cnt > 99)
      {
        xa_append_rows ( archive , 6 );
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
		g_io_channel_shutdown ( ioc,TRUE,NULL );
		g_io_channel_unref (ioc);
		xa_append_rows ( archive , 6 );
		return FALSE;
	}
	return TRUE;
}
