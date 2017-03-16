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

#include <string.h>
#include "arj.h"
#include "main.h"
#include "string_utils.h"
#include "support.h"
#include "window.h"

static gboolean data_line, fname_line;

void xa_arj_ask (XArchive *archive)
{
	archive->can_test = TRUE;
	archive->can_extract = TRUE;
	archive->can_add = archiver[archive->type].is_compressor;
	archive->can_delete = archiver[archive->type].is_compressor;
	archive->can_sfx = archiver[archive->type].is_compressor;
	archive->can_password = archiver[archive->type].is_compressor;
	archive->can_overwrite = archiver[archive->type].is_compressor;
	archive->can_full_path = archiver[archive->type].is_compressor;
	archive->can_freshen = archiver[archive->type].is_compressor;
	archive->can_update = archiver[archive->type].is_compressor;
	archive->can_move = archiver[archive->type].is_compressor;
}

static gchar *xa_arj_password_str (XArchive *archive)
{
	if (archive->password && archiver[archive->type].is_compressor)
		return g_strconcat(" -g", archive->password, NULL);
	else
		return g_strdup("");
}

static void xa_arj_parse_output (gchar *line, XArchive *archive)
{
	XEntry *entry;
	gpointer item[7];
	unsigned int linesize,n,a;
	guint64 file_size;
	static gchar *filename;
	gboolean unarj, lfn, dir, encrypted;

	unarj = !archiver[archive->type].is_compressor;

	if (!data_line)
	{
		if (line[0] == '-')
		{
			data_line = TRUE;
			return;
		}
		return;
	}

	if (!fname_line)
	{
		linesize = strlen(line);
		line[linesize - 1] = '\0';

		if (!unarj && (*line == ' '))
			return;

		if (line[0] == '-' && linesize == (unarj ? 59 : 41))
		{
			data_line = FALSE;
			return;
		}

		if (unarj)
		{
			/* simple column separator check */
			lfn = (linesize < 76 || line[34] != ' ' || line[40] != ' ' || line[49] != ' ' || line[58] != ' ' || line[67] != ' ');

			if (lfn)
				filename = g_strdup(line);
			else
			{
				filename = g_strchomp(g_strndup(line, 12));

				if (!*filename)
				{
					g_free(filename);
					filename = g_strdup(" ");   // just a wild guess in order to have an entry
				}
			}
		}
		else
		{
			lfn = TRUE;
			filename = g_strdup(line + 5);
		}

		archive->nr_of_files++;
		fname_line = TRUE;
		if (lfn)
			return;
	}

	if (fname_line)
	{
		linesize = strlen(line);
		/* Size */
		for(n=12; n < linesize && line[n] == ' '; n++);
		a = n;
		for(; n < linesize && line[n] != ' '; n++);
		line[n]='\0';
		item[0] = line + a;
		file_size = g_ascii_strtoull(item[0], NULL, 0);
		n++;

		/* Compressed */
		for(; n < linesize && line[n] == ' '; n++);
		a = n;
		for(; n < linesize && line[n] != ' '; n++);
		line[n]='\0';
		item[1] = line + a;
		n++;

		/* Ratio */
    	line[40] = '\0';
    	item[2] = line + 35;

		/* Date */
		line[49] = '\0';
		item[3] = line + 41;

		/* Time */
		line[58] = '\0';
		item[4] = line + 50;

		/* Permissions */
		line[unarj ? 72 : 69] = '\0';
		item[5] = line + (unarj ? 68 : 59);

		dir = (line[73] == 'D');   // for unarj only

		/* GUA */
		line[73] = '\0';
		item[6] = (unarj ? "" : line + 70);

		/* BPMGS */
		if (unarj)
			encrypted = (line[76] == 'G');
		else
			encrypted = (line[77] == '1');
		if (encrypted)
			archive->has_password = TRUE;

		if (unarj && dir)
		{
			/* skip entry since unarj lacks directory structure information */
			archive->nr_of_files--;
			entry = NULL;
		}
		else
		{
			archive->files_size += file_size;
			entry = xa_set_archive_entries_for_each_row (archive,filename,item);
		}

		if (entry != NULL)
		{
			if (line[59] == 'd')      // for arj only
				entry->is_dir = TRUE;

			entry->is_encrypted	= encrypted;
		}

		g_free(filename);
		fname_line = FALSE;
	}
}

void xa_arj_open (XArchive *archive)
{
	guint i;

	data_line = FALSE;
	fname_line = FALSE;
	gchar *command = g_strconcat(archiver[archive->type].program[0], archiver[archive->type].is_compressor ? " v " : " l ", archive->path[1], NULL);
	archive->files_size = 0;
	archive->nr_of_files = 0;
	archive->nc = 8;
	archive->parse_output = xa_arj_parse_output;
	xa_spawn_async_process (archive,command);
	g_free (command);

	GType types[]= {GDK_TYPE_PIXBUF,G_TYPE_STRING,G_TYPE_UINT64,G_TYPE_UINT64,G_TYPE_STRING,G_TYPE_STRING,G_TYPE_STRING,G_TYPE_STRING,G_TYPE_STRING,G_TYPE_POINTER};
	archive->column_types = g_malloc0(sizeof(types));
	for (i = 0; i < 10; i++)
		archive->column_types[i] = types[i];

	char *names[]= {(_("Original")),(_("Compressed")),(_("Ratio")),(_("Date")),(_("Time")),(_("Attributes")),("GUA"),NULL};
	xa_create_liststore (archive,names);
}

void xa_arj_test (XArchive *archive)
{
	gchar *password_str, *command;
	GSList *list = NULL;

	password_str = xa_arj_password_str(archive);
	command = g_strconcat(archiver[archive->type].program[0], " t", password_str, archiver[archive->type].is_compressor ?  " -i -y " : " ", archive->path[1], NULL);
	g_free(password_str);

	list = g_slist_append(list,command);
	xa_run_command (archive,list);
}

gboolean xa_arj_extract (XArchive *archive, GSList *file_list)
{
	GString *files;
	gchar *command;
	GSList *list	= NULL;

	files = xa_quote_filenames(file_list, "*?[]", FALSE);

	if (archiver[archive->type].is_compressor)
	{
		gchar *password_str = xa_arj_password_str(archive);
		command = g_strconcat(archiver[archive->type].program[0],
		                      archive->do_full_path ? " x" : " e",
		                      archive->do_overwrite ? "" : " -n",
		                      archive->do_freshen ? " -f" : "",
		                      archive->do_update ? " -u" : "",
		                      password_str, " -i -y ",
		                      archive->path[1], " ",
		                      archive->extraction_dir, files->str, NULL);
		g_free(password_str);
	}
	else
	{
		if (xa_create_working_directory(archive))
		{
			gchar *move;

			if (strcmp(archive->extraction_dir, archive->working_dir) == 0)
				move = g_strdup("");
			else
				move = g_strconcat(" && mv", *files->str ? files->str : " *", " ",
				                   archive->extraction_dir, NULL);

			command = g_strconcat("sh -c \"cd ", archive->working_dir, " && rm -f * && ",
			                      archiver[archive->type].program[0], " e ",
			                      archive->path[1], move, "\"", NULL);
			g_free(move);
		}
		else
			command = g_strdup("sh -c \"\"");
	}

	g_string_free(files,TRUE);
	list = g_slist_append(list,command);

	return xa_run_command(archive, list);
}

void xa_arj_add (XArchive *archive, GSList *file_list, gchar *compression)
{
	GString *files;
	gchar *password_str, *command;
	GSList *list = NULL;

	if (archive->location_path != NULL)
		archive->child_dir = g_strdup(archive->working_dir);

	if (!compression)
		compression = "1";

	files = xa_quote_filenames(file_list, "*?[]", FALSE);
	password_str = xa_arj_password_str(archive);
	command = g_strconcat(archiver[archive->type].program[0],
	                      archive->do_update ? " u" : " a",
	                      archive->do_freshen ? " -f" : "",
	                      archive->do_move ? " -d1" : "",
	                      " -m", compression,
	                      password_str, " -i -y ",
	                      archive->path[1], files->str, NULL);
	g_free(password_str);
	g_string_free(files,TRUE);
	list = g_slist_append(list,command);

	xa_run_command (archive,list);
}

void xa_arj_delete (XArchive *archive, GSList *file_list)
{
	GString *files;
	gchar *command;
	GSList *list = NULL;

	files = xa_quote_filenames(file_list, "*?[]", FALSE);
	command = g_strconcat(archiver[archive->type].program[0], " d -i -y ", archive->path[1], files->str, NULL);
	g_string_free(files,TRUE);
	list = g_slist_append(list,command);

	xa_run_command (archive,list);
}
