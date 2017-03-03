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
#include "rar.h"
#include "main.h"
#include "string_utils.h"
#include "support.h"
#include "window.h"

static int rar_version;
static gboolean header_line, data_line, fname_line, last_line;

int xa_rar_check_version (gchar *path)
{
	gchar *output;

	rar_version = 4;  // default version

	g_spawn_command_line_sync(path, &output, NULL, NULL, NULL);

	if (g_ascii_strncasecmp("\nRAR 5", output, 6) == 0 ||
	    g_ascii_strncasecmp("\nUNRAR 5", output, 8) == 0)
		rar_version = 5;

	g_free(output);

	return rar_version;
}

void xa_rar_ask (XArchive *archive)
{
	archive->can_test = TRUE;
	archive->can_extract = TRUE;
	archive->can_add = archiver[archive->type].is_compressor;
	archive->can_delete = archiver[archive->type].is_compressor;
	archive->can_sfx = archiver[archive->type].is_compressor;
	archive->can_passwd = archiver[archive->type].is_compressor;
	archive->can_overwrite = TRUE;
	archive->can_full_path = TRUE;
	archive->can_touch = TRUE;
	archive->can_freshen = TRUE;
	archive->can_update = TRUE;
	archive->can_solid = archiver[archive->type].is_compressor;
	archive->can_move = archiver[archive->type].is_compressor;
}

static gchar *xa_rar_passwd_str (XArchive *archive)
{
	if (archive->passwd)
		return g_strconcat(" -p", archive->passwd, NULL);
	else
		return g_strdup("");
}

static void xa_rar_parse_output (gchar *line, XArchive *archive)
{
	XEntry *entry;
	gpointer item[9];
	unsigned short int i = 0;
	unsigned int linesize,n,a;
	gboolean encrypted = FALSE, dir = FALSE;
	static gchar *filename;

	if (last_line)
		return;

	if (!data_line)
	{
		if (!header_line)
		{
			if ((strncmp(line, "Solid ", 6) == 0 || strncmp(line, "SFX ", 4) == 0 ||
			     strncmp(line, "Volume ", 7) == 0 || strncmp(line, "Archive ", 8) == 0)
			     && strstr(line, archive->path))
			{
				header_line = TRUE;

				if (archive->comment)
				{
					if (archive->comment->len > 2)
					{
						archive->has_comment = TRUE;
						archive->comment = g_string_truncate(archive->comment, archive->comment->len - 2);
						archive->comment = g_string_erase(archive->comment, 0, 1);
					}
					else
					{
						g_string_free(archive->comment, TRUE);
						archive->comment = NULL;
					}
				}
			}
			else
			{
				if (!archive->comment)
					archive->comment = g_string_new("");

				archive->comment = g_string_append(archive->comment, line);
			}
			return;
		}
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
		if(line[0] == '*')
		{
			archive->has_passwd = TRUE;
			encrypted = TRUE;
		}
		else if (line[0] == '-')
		{
			last_line = TRUE;
			return;
		}
		else if (line[0] != ' ')
			return;
		line[linesize - 1] = '\0';
		filename = g_strdup(line+1);
		fname_line = TRUE;
	}
	else
	{
		linesize = strlen(line);
		archive->nr_of_files++;
		/* Size */
		for(n=0; n < linesize && line[n] == ' '; n++);
		a = n;
		for(; n < linesize && line[n] != ' '; n++);
		line[n]='\0';
		item[i] = line + a;
		archive->files_size += g_ascii_strtoull(item[i],NULL,0);
		i++;
		n++;

		/* Compressed */
		for(; n < linesize && line[n] == ' '; n++);
		a = n;
		for(; n < linesize && line[n] != ' '; n++);
		line[n]='\0';
		item[i] = line + a;
		i++;
		n++;

		/* Ratio */
		for(; n < linesize && line[n] == ' '; n++);
		a = n;
		for(; n < linesize && line[n] != ' '; n++);
		line[n] = '\0';
		item[i] = line + a;
		i++;
		n++;

		/* Date */
		for(; n < linesize && line[n] == ' '; n++);
		a = n;
		for(; n < linesize && line[n] != ' '; n++);
		line[n] = '\0';
		item[i] = line + a;
		i++;
		n++;

		/* Time */
		for(; n < linesize && line[n] == ' '; n++);
		a = n;
		for(; n < linesize && line[n] != ' '; n++);
		line[n] = '\0';
		item[i] = line + a;
		i++;
		n++;

		/* Permissions */
		for(; n < linesize && line[n] == ' '; n++);
		a = n;
		for(; n < linesize && line[n] != ' '; n++);
		line[n] = '\0';
		if ((line+a)[0] == 'd')
			dir = TRUE;
		item[i] = line + a;
		i++;
		n++;

		/* CRC */
		for(; n < linesize && line[n] == ' '; n++);
		a = n;
		for(; n < linesize && line[n] != ' '; n++);
		line[n] = '\0';
		item[i] = line + a;
		i++;
		n++;

		/* Method */
		for(; n < linesize && line[n] == ' '; n++);
		a = n;
		for(; n < linesize && line[n] != ' '; n++);
		line[n] = '\0';
		item[i] = line + a;
		i++;
		n++;

		/* version */
		for(; n < linesize && line[n] == ' '; n++);
		a = n;
		for(; n < linesize && line[n] != ' ' && line[n] != '\n'; n++);
		line[n] = '\0';
		item[i] = line + a;

		/* Work around for rar which doesn't
		 * output / with directories */
		if (dir)
		{
			gchar *filename_with_slash = g_strconcat (filename,"/",NULL);
			g_free (filename);
			filename = filename_with_slash;
		}
		entry = xa_set_archive_entries_for_each_row (archive,filename,item);
		if (entry != NULL)
			entry->is_encrypted = encrypted;
		g_free(filename);
		fname_line = FALSE;
	}
}

static void xa_rar5_parse_output (gchar *line, XArchive *archive)
{
	XEntry *entry;
	gpointer item[7];
	unsigned short int i = 0;
	unsigned int linesize,n,a,offset;
	gboolean encrypted = FALSE, dir = FALSE;
	static gchar *filename, *end;

	if (last_line)
		return;

	if (!data_line)
	{
		if (!header_line)
		{
			if ((strncmp(line, "Archive: ", 9) == 0) && strstr(line, archive->path))
			{
				header_line = TRUE;

				if (archive->comment)
				{
					if (archive->comment->len > 2)
					{
						archive->has_comment = TRUE;
						archive->comment = g_string_truncate(archive->comment, archive->comment->len - 2);
						archive->comment = g_string_erase(archive->comment, 0, 1);
					}
					else
					{
						g_string_free(archive->comment, TRUE);
						archive->comment = NULL;
					}
				}
			}
			else
			{
				if (!archive->comment)
					archive->comment = g_string_new("");

				archive->comment = g_string_append(archive->comment, line);
			}
			return;
		}
		if (strncmp(line, "Details: RAR 5", 14) == 0)
			archive->version = 5;
		if (line[0] == '-')
		{
			data_line = TRUE;
			return;
		}
		return;
	}

	linesize = strlen(line);
	line[linesize - 1] = '\0';

	if(line[0] == '*')
	{
		archive->has_passwd = TRUE;
		encrypted = TRUE;
	}
	else if (line[0] == '-')
	{
		last_line = TRUE;
		return;
	}
	archive->nr_of_files++;

	/* Permissions */
	for(n=0; n < linesize && line[n] == ' '; n++);
	a = n;
	for(; n < linesize && line[n] != ' '; n++);
	line[n] = '\0';
	if ((line+a)[0] == 'd')
		dir = TRUE;
	item[5] = line + a;
	n++;

	/* Size */
	for(; n < linesize && line[n] == ' '; n++);
	a = n;
	for(; n < linesize && line[n] != ' '; n++);
	line[n]='\0';
	item[i] = line + a;
	archive->files_size += g_ascii_strtoull(item[i],NULL,0);
	i++;
	n++;

	/* Compressed */
	for(; n < linesize && line[n] == ' '; n++);
	a = n;
	for(; n < linesize && line[n] != ' '; n++);
	line[n]='\0';
	item[i] = line + a;
	i++;
	n++;

	/* Ratio */
	for(; n < linesize && line[n] == ' '; n++);
	a = n;
	for(; n < linesize && line[n] != ' '; n++);
	line[n] = '\0';
	item[i] = line + a;
	i++;
	n++;

	/* Date */
	for(; n < linesize && line[n] == ' '; n++);
	a = n;
	for(; n < linesize && line[n] != ' '; n++);
	line[n] = '\0';
	item[i] = line + a;
	i++;
	n++;

	/* Time */
	for(; n < linesize && line[n] == ' '; n++);
	a = n;
	for(; n < linesize && line[n] != ' '; n++);
	line[n] = '\0';
	item[i] = line + a;
	i+=2;
	n++;

	/* CRC */
	for(; n < linesize && line[n] == ' '; n++);
	a = n;
	for(; n < linesize && line[n] != ' '; n++);
	line[n] = '\0';
	item[i] = line + a;

	/* FileName */
	line[linesize - 1] = '\0';
	offset = (strlen(item[3]) == 10 ? 66 : 64);  // date is YYYY-MM-DD since v5.30
	filename = g_strdup(line+offset);            // and was just DD-MM-YY before

	/* Strip trailing whitespace */
	end = filename + strlen(filename) - 1;
	while(end >= filename && *end == ' ') end--;
	*(end + 1) = '\0';

	/* Work around for rar which doesn't
	 * output / with directories */
	if (dir)
	{
		gchar *filename_with_slash = g_strconcat (filename,"/",NULL);
		g_free (filename);
		filename = filename_with_slash;
	}

	entry = xa_set_archive_entries_for_each_row (archive,filename,item);
	if (entry != NULL)
		entry->is_encrypted = encrypted;
	g_free(filename);
}

void xa_rar_open (XArchive *archive)
{
	GType types4[]= {GDK_TYPE_PIXBUF,G_TYPE_STRING,G_TYPE_UINT64,G_TYPE_UINT64,G_TYPE_STRING,G_TYPE_STRING,G_TYPE_STRING,G_TYPE_STRING,G_TYPE_STRING,G_TYPE_STRING,G_TYPE_STRING,G_TYPE_POINTER};
	char *names4[]= {(_("Original")),(_("Compressed")),(_("Ratio")),(_("Date")),(_("Time")),(_("Permissions")),(_("CRC")),(_("Method")),(_("Version")),NULL};
	GType types5[]= {GDK_TYPE_PIXBUF,G_TYPE_STRING,G_TYPE_UINT64,G_TYPE_UINT64,G_TYPE_STRING,G_TYPE_STRING,G_TYPE_STRING,G_TYPE_STRING,G_TYPE_STRING,G_TYPE_POINTER};
	char *names5[]= {(_("Original")),(_("Compressed")),(_("Ratio")),(_("Date")),(_("Time")),(_("Permissions")),(_("CRC")),NULL};

	unsigned short int i;
	gchar *command = NULL;

	header_line = FALSE;
	data_line = FALSE;
	fname_line = FALSE;
	last_line = FALSE;

	command = g_strconcat(archiver[archive->type].program[0], " v -idc ", archive->escaped_path, NULL);
	archive->files_size = 0;
    archive->nr_of_files = 0;


	if (rar_version == 5)
	{
		archive->nc = 8;
		archive->parse_output = xa_rar5_parse_output;
		xa_spawn_async_process (archive,command);
		g_free ( command );
		if ( archive->child_pid == 0 )
			return;
		archive->column_types = g_malloc0(sizeof(types5));
		for (i = 0; i < archive->nc+2; i++)
			archive->column_types[i] = types5[i];

		xa_create_liststore (archive, names5);
	}
	else
	{
		archive->nc = 10;
		archive->parse_output = xa_rar_parse_output;
		xa_spawn_async_process (archive,command);
		g_free ( command );
		if ( archive->child_pid == 0 )
			return;
		archive->column_types = g_malloc0(sizeof(types4));
		for (i = 0; i < archive->nc+2; i++)
			archive->column_types[i] = types4[i];

		xa_create_liststore (archive, names4);
	}
}

void xa_rar_test (XArchive *archive)
{
	gchar *passwd_str, *command;
	GSList *list = NULL;

	passwd_str = xa_rar_passwd_str(archive);
	command = g_strconcat(archiver[archive->type].program[0], " t", passwd_str, " -idp -y ", archive->escaped_path, NULL);
	g_free(passwd_str);

	list = g_slist_append(list,command);
	xa_run_command (archive,list);
}

/*
 * Note: rar does not seem to be able to handle wildcards in file names.
 */

gboolean xa_rar_extract (XArchive *archive, GSList *file_list)
{
	GString *files;
	gchar *passwd_str, *command;
	GSList *list = NULL;
	gboolean result = FALSE;

	files = xa_quote_filenames(file_list, NULL);
	passwd_str = xa_rar_passwd_str(archive);
	command = g_strconcat(archiver[archive->type].program[0],
	                      archive->full_path ? " x" : " e",
	                      archive->overwrite ? " -o+" : " -o-",
	                      archive->touch ? " -tsm-" : "",
	                      archive->freshen ? " -f" : "",
	                      archive->update ? " -u" : "",
	                      passwd_str, " -idp -y ",
	                      archive->escaped_path, files->str,
	                      " ", archive->extraction_path, NULL);
	g_free(passwd_str);
	g_string_free(files,TRUE);
	list = g_slist_append(list,command);

	result = xa_run_command (archive,list);
	return result;
}

void xa_rar_add (XArchive *archive, GSList *file_list, gchar *compression)
{
	GString *files;
	gchar *passwd_str, *command, *version_switch;
	GSList *list = NULL;


	if (archive->location_entry_path != NULL)
		archive->working_dir = g_strdup(archive->tmp);

	if (rar_version == 5)
	{
		if (archive->version == 5)
			version_switch = " -ma5";
		else
			version_switch = " -ma4";
	}
	else
		version_switch = "";

	if (!compression)
		compression = "3";

	files = xa_quote_filenames(file_list, NULL);
	passwd_str = xa_rar_passwd_str(archive);
	command = g_strconcat(archiver[archive->type].program[0],
	                      archive->update ? " u" : " a", version_switch,
	                      archive->freshen ? " -f" : "",
	                      archive->add_solid ? " -s" : "",
	                      archive->add_move ? " -df" : "",
	                      " -m", compression,
	                      passwd_str, " -idp -y ",
	                      archive->escaped_path, files->str, NULL);
	g_free(passwd_str);
	g_string_free(files,TRUE);
	list = g_slist_append(list,command);

	xa_run_command (archive,list);
	xa_reload_archive_content(archive);
}

void xa_rar_delete (XArchive *archive, GSList *file_list)
{
	GString *files;
	gchar *command;
	GSList *list = NULL;

	files = xa_quote_filenames(file_list, NULL);
	command = g_strconcat(archiver[archive->type].program[0], " d -idp -y ", archive->escaped_path, files->str, NULL);
	g_string_free(files,TRUE);
	list = g_slist_append(list,command);

	xa_run_command (archive,list);
	if (archive->status == XA_ARCHIVESTATUS_DELETE)
		xa_reload_archive_content(archive);
}
