/*
 *  Copyright (C) 2022 Ingo Brückl
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 *  MA 02110-1301 USA.
 */

#include <string.h>
#include "zpaq.h"
#include "main.h"
#include "parser.h"
#include "string_utils.h"
#include "support.h"
#include "window.h"

static gboolean data_line, last_line;

void xa_zpaq_ask (XArchive *archive)
{
	compressor_t zpaq_compressor = {TRUE, 1, 1, 5, 1};

	archive->can_test = TRUE;
	archive->can_extract = TRUE;
	archive->can_add = TRUE;
	archive->can_delete = TRUE;
	archive->can_full_path[0] = TRUE;
	archive->can_full_path[1] = TRUE;   // n.b.: adds leading slash
	archive->can_overwrite = TRUE;
	archive->can_recurse = TRUE;
	archive->can_compress = TRUE;
	archive->compressor = zpaq_compressor;
	archive->compression = archive->compressor.preset;
}

static void xa_zpaq_parse_output (gchar *line, XArchive *archive)
{
	XEntry *entry;
	gpointer item[4];
	gchar *filename;

	USE_PARSER;

	if (last_line)
		return;

	if (!data_line)
	{
		if (line[0] == '\n')
		{
			data_line = TRUE;
			return;
		}

		return;
	}

	if (line[0] == '\n')
	{
		last_line = TRUE;
		return;
	}

	/* comparison result */
	SKIP_ITEM;   // always: -

	/* date */
	NEXT_ITEM(item[1]);

	/* time */
	NEXT_ITEM(item[2]);

	/* size */
	NEXT_ITEM(item[0]);

	/* permissions */
	NEXT_ITEM(item[3]);

	if (*(char *) item[3] == 'd')
		item[3]++;

	/* name */
	LAST_ITEM(filename);

	entry = xa_set_archive_entries_for_each_row(archive, filename, item);

	if (entry)
	{
		if (!entry->is_dir)
			archive->files++;

		archive->files_size += g_ascii_strtoull(item[0], NULL, 0);
	}
}

void xa_zpaq_list (XArchive *archive)
{
	const GType types[] = {GDK_TYPE_PIXBUF, G_TYPE_STRING, G_TYPE_UINT64, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_POINTER};
	const gchar *titles[] = {_("Original Size"), _("Date"), _("Time"), _("Permissions")};
	gchar *command;
	guint i;

	data_line = FALSE;
	last_line = FALSE;

	archive->files = 0;
	archive->files_size = 0;

	command = g_strconcat(archiver[archive->type].program[0], " l ", archive->path[1], NULL);
	archive->parse_output = xa_zpaq_parse_output;
	xa_spawn_async_process(archive, command);
	g_free(command);

	archive->columns = 7;
	archive->size_column = 2;
	archive->column_types = g_malloc0(sizeof(types));

	for (i = 0; i < archive->columns; i++)
		archive->column_types[i] = types[i];

	xa_create_liststore(archive, titles);
}

void xa_zpaq_test (XArchive *archive)
{
	gchar *command;

	command = g_strconcat(archiver[archive->type].program[0], " x ", archive->path[1], " -test", NULL);

	xa_run_command(archive, command);
	g_free(command);
}

/*
 * Note: zpaq's wildcard handling is weird. A file with wildcard character(s)
 * is actually extracted, added or deleted, but all other files matching the
 * pattern will be extracted, deleted (when adding) or deleted (when deleting)
 * at the same time, regardless of whether the wildcard is escaped or quoted!
 */

static gboolean xa_zpaq_wildcards (XArchive *archive, GSList *file_list)
{
	GSList *list;
	gchar *command;

	list = file_list;

	while (list)
	{
		if (strpbrk(list->data, "*?"))
		{
			command = g_strconcat("sh -c \"echo ", _("Will not handle files with wildcard characters!"), " >&2; exit 1\"", NULL);
			xa_run_command(archive, command);
			g_free(command);
			return TRUE;
		}

		list = list->next;
	}

	return FALSE;
}

gboolean xa_zpaq_extract (XArchive *archive, GSList *file_list)
{
	GSList *list;
	GString *to, *files;
	gchar *extract_to, *quoted, *new, *command;
	gboolean result;

	/* don't confuse the user with the weird wildcard handling */
	if (xa_zpaq_wildcards(archive, file_list))
		return FALSE;

	/* extract to a new temporary directory because archive might contain absolute paths */
	extract_to = xa_create_working_subdirectory(archive);

	if (!extract_to)
		return FALSE;

	/* create the renaming list */

	list = file_list;

	if (list)
		to = g_string_new("");
	else
		to = g_string_new(" .");

	while (list)
	{
		quoted = g_shell_quote(list->data);

		if (*(char *) list->data == '/')
			new = g_strdup_printf(" .%s", quoted);
		else
			new = g_strdup_printf(" ./%s", quoted);

		g_string_prepend(to, new);

		g_free(new);
		g_free(quoted);

		list = list->next;
	}

	files = xa_quote_filenames(file_list, NULL, TRUE);
	archive->child_dir = extract_to;
	command = g_strconcat(archiver[archive->type].program[0], " x ",
	/* do_overwrite can be ignored in new temporary directory */
	                      archive->path[1], files->str,
	                      " -to", to->str, NULL);

	result = xa_run_command(archive, command);
	g_free(command);

	/* get all files that have been extracted to move them to the destination */
	if (result)
	{
		GString *all_files;

		if (archive->do_full_path)
			all_files = xa_quote_dir_contents(extract_to);
		else
			all_files = xa_collect_files_in_dir(extract_to);

		command = g_strconcat("mv",
		                      archive->do_overwrite ? " -f" : " -n",
		                      " --", all_files->str, " ", archive->extraction_dir, NULL);
		g_string_free(all_files, TRUE);

		archive->status = XARCHIVESTATUS_EXTRACT;   // restore status
		result = xa_run_command(archive, command);
		g_free(command);
	}

	archive->child_dir = NULL;
	g_free(extract_to);
	g_string_free(files, TRUE);

	return result;
}

void xa_zpaq_add (XArchive *archive, GSList *file_list)
{
	GString *files;
	gchar *compression, *command;

	/* don't confuse the user with the weird wildcard handling */
	if (xa_zpaq_wildcards(archive, file_list))
		return;

	compression = g_strdup_printf("%hu", archive->compression);

	files = xa_quote_filenames(file_list, "-", TRUE);
	command = g_strconcat(archiver[archive->type].program[0], " a ",
	                      archive->path[1], files->str,
	                      " -m", compression, NULL);
	g_string_free(files, TRUE);
	g_free(compression);

	xa_run_command(archive, command);
	g_free(command);
}

void xa_zpaq_delete (XArchive *archive, GSList *file_list)
{
	gchar *workdir, *command = NULL;
	GSList *list;
	GString *files;

	/* don't confuse the user with the weird wildcard handling */
	if (xa_zpaq_wildcards(archive, file_list))
		return;

	workdir = xa_create_working_subdirectory(archive);

	if (!workdir)
		return;

	/* do not accidentally delete existing files */

	list = file_list;

	while (list)
	{
		if (*(char *) list->data == '/')
		{
			command = g_strconcat("sh -c \"echo ", _("Files with absolute pathnames cannot be deleted!"), " >&2; exit 1\"", NULL);
			break;
		}

		list = list->next;
	}

	if (!command)
	{
		files = xa_quote_filenames(file_list, NULL, TRUE);
		archive->child_dir = workdir;
		command = g_strconcat(archiver[archive->type].program[0], " a ", archive->path[1], files->str, NULL);
		g_string_free(files, TRUE);
	}

	xa_run_command(archive, command);
	g_free(command);

	archive->child_dir = NULL;
	g_free(workdir);
}
