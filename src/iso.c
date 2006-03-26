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
 *  GNU Gener al Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Street #330, Boston, MA 02111-1307, USA.
 */

#include "iso.h"

void OpenISO ( gboolean mode , gchar *path)
{
    //Let's read the iso information first:
    iso_stream = fopen ( path ,"r" );
    if (iso_stream == NULL)
    {
        gchar *msg = g_strdup_printf (_("Can't open ISO image %s:\n%s") , path , strerror (errno) );
		response = ShowGtkMessageDialog (GTK_WINDOW (MainWindow) , GTK_DIALOG_MODAL,GTK_MESSAGE_ERROR,GTK_BUTTONS_OK, msg);
		g_free (msg);
        return;
    }
    if ( stat ( path , &my_stat ) < 0 )
    {
        response = ShowGtkMessageDialog (GTK_WINDOW (MainWindow) , GTK_DIALOG_MODAL,GTK_MESSAGE_ERROR,GTK_BUTTONS_OK, strerror (errno));
		return;
    }

    if ( fseek (iso_stream , 32768, SEEK_SET ) < 0)
    {
        response = ShowGtkMessageDialog (GTK_WINDOW (MainWindow) , GTK_DIALOG_MODAL,GTK_MESSAGE_ERROR,GTK_BUTTONS_OK, "This ISO image is malformed !");
		return;
    }
    fread ( &ipd, 1, sizeof(ipd), iso_stream );
    ipd.system_id[31] = '\000';
    ipd.volume_id[31] = '\000';
    ipd.volume_set_id[127] = '\000';
    ipd.publisher_id[127] = '\000';
    ipd.preparer_id[127] = '\000';
    ipd.application_id[127] = '\000';
    ipd.copyright_file_id[36] = '\000';
    ipd.abstract_file_id[36] = '\000';
    ipd.bibliographic_file_id[36] = '\000';

    gchar *iso_info = g_strdup_printf (
                _("\n\nFilename        : %s\n"
                "Size               : %lld bytes\n"
                "\nSystem ID      : %s\n"
                "Volume ID      : %s\n"
                "Application      : %s\n"
                "Publisher        : %s\n"
                "Preparer        : %s\n"
                "Volume Set ID   : %s\n"
                "Bibliographic ID: %s\n"
                "Copyright ID    : %s\n"
                "Abstract ID     : %s\n\n"
                "Creation Date   : %4.4s %2.2s %2.2s %2.2s:%2.2s:%2.2s.%2.2s\n"
                "Modified Date   : %4.4s %2.2s %2.2s %2.2s:%2.2s:%2.2s.%2.2s\n"
                "Expiration Date : %4.4s %2.2s %2.2s %2.2s:%2.2s:%2.2s.%2.2s\n"
                "Effective Date  : %4.4s %2.2s %2.2s %2.2s:%2.2s:%2.2s.%2.2s\n\n"
                "Volume Set Size    : %d\n"
                "Volume Sequence n. : %d\n"
                "Volume Space Size  : %d\n"
                "Logical Block Size : %d\n"
                "Path Table Size    : %d\n"
                "Root Dir. Record   : %d\n"),
                path,
                my_stat.st_size,
                ipd.system_id,
                ipd.volume_id,
                ipd.application_id,
                ipd.publisher_id,
                ipd.preparer_id,
                ipd.volume_set_id,
                ipd.bibliographic_file_id,
                ipd.copyright_file_id,
                ipd.abstract_file_id,
                &ipd.creation_date[0],
                &ipd.creation_date[4],
                &ipd.creation_date[6],
                &ipd.creation_date[8],
                &ipd.creation_date[10],
                &ipd.creation_date[12],
                &ipd.creation_date[14],
                &ipd.modification_date[0],
                &ipd.modification_date[4],
                &ipd.modification_date[6],
                &ipd.modification_date[8],
                &ipd.modification_date[10],
                &ipd.modification_date[12],
                &ipd.modification_date[14],
                &ipd.expiration_date[0],
                &ipd.expiration_date[4],
                &ipd.expiration_date[6],
                &ipd.expiration_date[8],
                &ipd.expiration_date[10],
                &ipd.expiration_date[12],
                &ipd.expiration_date[14],
                &ipd.effective_date[0],
                &ipd.effective_date[4],
                &ipd.effective_date[6],
                &ipd.effective_date[8],
                &ipd.effective_date[10],
                &ipd.effective_date[12],
                &ipd.effective_date[14],
                iso_723 (ipd.volume_set_size),
                iso_723 (ipd.volume_sequence_number),
                iso_731 (ipd.volume_space_size),
                iso_723 (ipd.logical_block_size),
                iso_723 (ipd.path_table_size) ,
                iso_731 (ipd.root_directory_record));
    gtk_text_buffer_insert (textbuf, &enditer, iso_info, strlen (iso_info));
    ShowShellOutput ( NULL , TRUE );
    g_free (iso_info);
	char *names[]= {(_("Filename")),(_("Original")),(_("Method")),(_("Compressed")),(_("Ratio")),(_("Date")),(_("Time")),(_("CRC-32"))};
	GType types[]= {G_TYPE_STRING,G_TYPE_UINT,G_TYPE_STRING,G_TYPE_UINT,G_TYPE_STRING,G_TYPE_STRING,G_TYPE_STRING,G_TYPE_STRING};
	CreateListStore ( 8, names , (GType *)types );
}

//The following two functions are taken from isodump.c - By Eric Youngdale (1993)
int iso_723 ( unsigned char *p)
{
    return (( p[0] & 0xff) | ((p[1] & 0xff) << 8));
}

int iso_731 ( unsigned char *p)
{
    return ((p[0] & 0xff) | ((p[1] & 0xff) << 8) | ((p[2] & 0xff) << 16) | ((p[3] & 0xff) << 24));
}


