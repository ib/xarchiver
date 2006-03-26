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
 
#ifndef ISO_H
#define ISO_H

 #include <gtk/gtk.h>
 #include <sys/types.h>
 #include <sys/stat.h>

 #include "callbacks.h"
 #include "interface.h"
 #include "support.h"
 #include "main.h"

struct iso_primary_descriptor
{
    unsigned char type[1];
    unsigned char id[5];
    unsigned char version[1];
    unsigned char unused1[1];
    unsigned char system_id[32];
    unsigned char volume_id[32];
    unsigned char unused2[8];
    unsigned char volume_space_size[8];
    unsigned char unused3[32];
    unsigned char volume_set_size[4];
    unsigned char volume_sequence_number[4];
    unsigned char logical_block_size[4];
    unsigned char path_table_size[8];
    unsigned char type_l_path_table[4];
    unsigned char opt_type_l_path_table[4];
    unsigned char type_m_path_table[4];
    unsigned char opt_type_m_path_table[4];
    unsigned char root_directory_record[34];
    unsigned char volume_set_id[128];
    unsigned char publisher_id[128];
    unsigned char preparer_id[128];
    unsigned char application_id[128];
    unsigned char copyright_file_id[37];
    unsigned char abstract_file_id[37];
    unsigned char bibliographic_file_id[37];
    unsigned char creation_date[17];
    unsigned char modification_date[17];
    unsigned char expiration_date[17];
    unsigned char effective_date[17];
    unsigned char file_structure_version[1];
    unsigned char unused4[1];
    unsigned char application_data[512];
    unsigned char unused5[653];
};

struct iso_directory_record
{
    unsigned char length[1];
    unsigned char ext_attr_length[1];
    unsigned char extent[8];
    unsigned char size[8];
    unsigned char date[7];
    unsigned char flags[1];
    unsigned char file_unit_size[1];
    unsigned char interleave_gap[1];
    unsigned char volume_seq_number[4];
    unsigned char name_len[1];
    unsigned char file_name[1];
};

FILE *iso_stream;
struct iso_primary_descriptor ipd;
struct stat my_stat;

int iso_723 ( unsigned char *p);
int iso_731 ( unsigned char *p);
void OpenISO ( gboolean mode , gchar *path );
#endif

