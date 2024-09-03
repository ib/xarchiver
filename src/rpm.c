/*
 *  Copyright (C) 2008 Giuseppe Torelli - <colossus73@gmail.com>
 *  Copyright (C) 2016 Ingo Brückl
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

#include <errno.h>
#include "rpm.h"
#include "cpio.h"
#include "main.h"
#include "string_utils.h"
#include "support.h"
#include "window.h"

#define LEAD_LEN 96
#define HDRSIG_MAGIC_LEN 3
#define HDRSIG_VERSION_LEN 1
#define HDRSIG_RESERVED_LEN 4
#define HDRSIG_LEAD_IN_LEN (HDRSIG_MAGIC_LEN + HDRSIG_VERSION_LEN + HDRSIG_RESERVED_LEN)
#define SIGNATURE_START (LEAD_LEN + HDRSIG_LEAD_IN_LEN)
#define HDRSIG_ENTRY_INFO_LEN 8
#define HDRSIG_ENTRY_INDEX_LEN 16

void xa_rpm_ask (XArchive *archive)
{
	archive->can_extract = TRUE;
	archive->can_full_path[0] = TRUE;
	archive->can_touch = TRUE;
	archive->can_overwrite = TRUE;
}

static gchar *xa_rpm2cpio (XArchive *archive)
{
	unsigned char bytes[HDRSIG_ENTRY_INFO_LEN];
	int datalen, entries;
	long offset;
	gchar *cpio_z, *ibs, *command, *executable;
	FILE *stream;
	gboolean success;
	ArchiveType xa;

	signal(SIGPIPE, SIG_IGN);
	stream = fopen(archive->path[0], "r");

	if (stream == NULL)
	{
		gchar *msg, *err;

		msg = g_strdup_printf(_("Can't open RPM file \"%s\":"), archive->path[0]);
		err = g_strconcat(msg, " ", g_strerror(errno), NULL);

		g_free(msg);

		return err;
	}

	/* Signature section */
	if (fseek(stream, SIGNATURE_START, SEEK_CUR) == -1)
	{
		fclose (stream);
		return g_strconcat(_("Can't fseek to position 104:"), " ", g_strerror(errno), NULL);
	}
	if (fread(bytes, sizeof(*bytes), HDRSIG_ENTRY_INFO_LEN, stream) != HDRSIG_ENTRY_INFO_LEN)
	{
		fclose ( stream );
		return g_strconcat(_("Can't read data from file:"), " ", g_strerror(errno), NULL);
	}
	entries = 256 * (256 * (256 * bytes[0] + bytes[1]) + bytes[2]) + bytes[3];
	datalen = 256 * (256 * (256 * bytes[4] + bytes[5]) + bytes[6]) + bytes[7];
	datalen += (16 - (datalen % 16)) % 16;  // header section is aligned
	offset = HDRSIG_ENTRY_INDEX_LEN * entries + datalen;

	/* Header section */
	if (fseek(stream, offset, SEEK_CUR))
	{
		fclose (stream);
		return g_strconcat(_("Can't fseek in file:"), " ", g_strerror(errno), NULL);
	}
	if (fread(bytes, sizeof(*bytes), HDRSIG_ENTRY_INFO_LEN, stream) != HDRSIG_ENTRY_INFO_LEN)
	{
		fclose ( stream );
		return g_strconcat(_("Can't read data from file:"), " ", g_strerror(errno), NULL);
	}
	entries = 256 * (256 * (256 * bytes[0] + bytes[1]) + bytes[2]) + bytes[3];
	datalen = 256 * (256 * (256 * bytes[4] + bytes[5]) + bytes[6]) + bytes[7];
	offset = HDRSIG_ENTRY_INDEX_LEN * entries + datalen;
	offset += ftell(stream);  // offset from top

	fclose(stream);

	/* create a unique temp dir in /tmp */
	if (!xa_create_working_directory(archive))
		return g_strdup("");

	cpio_z = g_strconcat(archive->working_dir, "/xa-tmp.cpio_z", NULL);
	ibs = g_strdup_printf("%lu", offset);

	/* run dd to have the payload (compressed cpio archive) in /tmp */
	command = g_strconcat("dd if=", archive->path[1], " ibs=", ibs, " skip=1 of=", cpio_z, NULL);
	g_free(ibs);

	success = xa_run_command(archive, command);
	g_free(command);

	if (!success)
	{
		g_free(cpio_z);
		return g_strdup("");
	}

	xa = xa_detect_archive_type(cpio_z);

	switch (xa.type)
	{
		case XARCHIVETYPE_GZIP:
			executable = "gzip -dc ";
			break;

		case XARCHIVETYPE_BZIP2:
			executable = "bzip2 -dc ";
			break;

		case XARCHIVETYPE_LZMA:
		case XARCHIVETYPE_XZ:
			executable = "xz -dc ";
			break;

		case XARCHIVETYPE_ZSTD:
			executable = "zstd -dc ";
			break;

		default:
			g_free(cpio_z);
			return g_strdup(_("Unknown compression type!"));
	}

	command = g_strconcat("sh -c \"exec ", executable, cpio_z, " > ", archive->working_dir, "/xa-tmp.cpio\"", NULL);
	g_free(cpio_z);

	archive->status = XARCHIVESTATUS_LIST;        // restore status
	success = xa_run_command(archive, command);
	g_free(command);

	return (success ? NULL : g_strdup(""));
}

void xa_rpm_list (XArchive *archive)
{
	const GType types[] = {GDK_TYPE_PIXBUF, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_UINT64, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_POINTER};
	const gchar *titles[] = {_("Points to"), _("Original Size"), _("Date"), _("Time"), _("Permissions"), _("Owner"), _("Group")};
	gchar *result, *command;
	guint i;

	result = xa_rpm2cpio(archive);

	archive->files_size = 0;
	archive->files = 0;

	archive->columns = 10;
	archive->size_column = 3;
	archive->column_types = g_malloc0(sizeof(types));

	for (i = 0; i < archive->columns; i++)
		archive->column_types[i] = types[i];

	xa_create_liststore(archive, titles);

	if (result != NULL)
	{
		if (*result)
			command = g_strconcat("sh -c \"echo ", result, " >&2; exit 1\"", NULL);
		else
			command = g_strdup("sh -c \"\"");

		g_free(result);
	}
	else
		command = g_strconcat(archiver[archive->type].program[0], " -tv -I ", archive->working_dir, "/xa-tmp.cpio", NULL);

	archive->parse_output = xa_cpio_parse_output;
	xa_spawn_async_process (archive,command);
	g_free(command);
}

gboolean xa_rpm_extract (XArchive *archive, GSList *file_list)
{
	gchar *path;
	gboolean result;

	path = archive->path[1];
	archive->path[1] = g_strconcat(archive->working_dir, "/xa-tmp.cpio", NULL);

	result = xa_cpio_extract(archive, file_list);

	g_free(archive->path[1]);
	archive->path[1] = path;

	return result;
}
