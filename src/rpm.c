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

#include <errno.h>
#include <string.h>
#include "rpm.h"
#include "interface.h"
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

static int xa_rpm2cpio (XArchive *archive)
{
	unsigned char bytes[HDRSIG_ENTRY_INFO_LEN];
	int datalen, entries;
	long offset;
	gchar *cpio_z, *ibs, *command, *executable;
	GSList *list;
	FILE *stream;

	signal(SIGPIPE, SIG_IGN);
	stream = fopen(archive->path, "r");

	if (stream == NULL)
	{
		gchar *msg = g_strdup_printf(_("Can't open RPM file %s:"), archive->path);
		xa_show_message_dialog(GTK_WINDOW(xa_main_window), GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK, msg, g_strerror(errno));
		g_free(msg);
		return -1;
	}

	/* Signature section */
	if (fseek(stream, SIGNATURE_START, SEEK_CUR) == -1)
	{
		fclose (stream);
		xa_show_message_dialog (GTK_WINDOW (xa_main_window),GTK_DIALOG_MODAL,GTK_MESSAGE_ERROR,GTK_BUTTONS_OK,_("Can't fseek to position 104:"),g_strerror(errno));
		return -1;
	}
	if (fread(bytes, 1, HDRSIG_ENTRY_INFO_LEN, stream) != HDRSIG_ENTRY_INFO_LEN)
	{
		fclose ( stream );
		xa_show_message_dialog (GTK_WINDOW (xa_main_window),GTK_DIALOG_MODAL,GTK_MESSAGE_ERROR,GTK_BUTTONS_OK,_("Can't read data from file:"),g_strerror(errno));
		return -1;
	}
	entries = 256 * (256 * (256 * bytes[0] + bytes[1]) + bytes[2]) + bytes[3];
	datalen = 256 * (256 * (256 * bytes[4] + bytes[5]) + bytes[6]) + bytes[7];
	datalen += (16 - (datalen % 16)) % 16;  // header section is aligned
	offset = HDRSIG_ENTRY_INDEX_LEN * entries + datalen;

	/* Header section */
	if (fseek(stream, offset, SEEK_CUR))
	{
		fclose (stream);
		xa_show_message_dialog (GTK_WINDOW (xa_main_window),GTK_DIALOG_MODAL,GTK_MESSAGE_ERROR,GTK_BUTTONS_OK,_("Can't fseek in file:"),g_strerror(errno));
		return -1;
	}
	if (fread(bytes, 1, HDRSIG_ENTRY_INFO_LEN, stream) != HDRSIG_ENTRY_INFO_LEN)
	{
		fclose ( stream );
		xa_show_message_dialog (GTK_WINDOW (xa_main_window),GTK_DIALOG_MODAL,GTK_MESSAGE_ERROR,GTK_BUTTONS_OK,_("Can't read data from file:"),g_strerror(errno));
		return -1;
	}
	entries = 256 * (256 * (256 * bytes[0] + bytes[1]) + bytes[2]) + bytes[3];
	datalen = 256 * (256 * (256 * bytes[4] + bytes[5]) + bytes[6]) + bytes[7];
	offset = HDRSIG_ENTRY_INDEX_LEN * entries + datalen;
	offset += ftell(stream);  // offset from top

	fclose(stream);

	/* create a unique temp dir in /tmp */
	if (!xa_create_temp_directory(archive))
		return -1;

	cpio_z = g_strconcat(archive->tmp, "/xa-tmp.cpio_z", NULL);
	ibs = g_strdup_printf("%lu", offset);

	/* run dd to have the payload (compressed cpio archive) in /tmp */
	command = g_strconcat("dd if=", archive->escaped_path, " ibs=", ibs, " skip=1 of=", cpio_z, NULL);
	list = g_slist_append(NULL, command);
	g_free(ibs);

	if (!xa_run_command(archive, list))
	{
		g_free(cpio_z);
		return -1;
	}

	switch (xa_detect_archive_type(cpio_z))
	{
		case XARCHIVETYPE_GZIP:
			executable = "gzip -dc ";
			break;

		case XARCHIVETYPE_BZIP2:
			executable = "bzip2 -dc ";
			break;

		default:
			executable = "xz -dc ";
			break;
	}

	command = g_strconcat("sh -c \"", executable, cpio_z, " > ", archive->tmp, "/xa-tmp.cpio\"", NULL);
	list = g_slist_append(NULL, command);
	g_free(cpio_z);

	if (!xa_run_command(archive, list))
		return 0;

	return 1;
}

void xa_open_rpm (XArchive *archive)
{
	int result, i;
	gchar *command;

	char *names[]= {(_("Points to")),(_("Size")),(_("Permission")),(_("Date")),(_("Hard Link")),(_("Owner")),(_("Group")),NULL};
	GType types[]= {GDK_TYPE_PIXBUF,G_TYPE_STRING,G_TYPE_STRING,G_TYPE_UINT64,G_TYPE_STRING,G_TYPE_STRING,G_TYPE_STRING,G_TYPE_STRING,G_TYPE_STRING,G_TYPE_POINTER};

	result = xa_rpm2cpio(archive);

	if (result < 0)
		return;

	archive->can_extract = archive->has_properties = TRUE;
	archive->can_add = archive->has_sfx = archive->has_test = FALSE;
	archive->dummy_size = 0;
	archive->nr_of_files = 0;
	archive->nc = 8;
	archive->format ="RPM";

	archive->column_types = g_malloc0(sizeof(types));
	for (i = 0; i < 10; i++)
		archive->column_types[i] = types[i];

	xa_create_liststore(archive, names);

	if (result == 0)
	{
		gtk_widget_set_sensitive(Stop_button,FALSE);
		xa_set_button_state (1,1,1,1,archive->can_add,archive->can_extract,0,archive->has_test,archive->has_properties,archive->has_passwd,0);
		gtk_label_set_text(GTK_LABEL(total_label),"");
		return;
	}
	/* list the content */
	command = g_strconcat ("sh -c \"cpio -tv < ",archive->tmp,"/xa-tmp.cpio\"",NULL);
	archive->parse_output = xa_get_cpio_line_content;
	xa_spawn_async_process (archive,command);
	g_free(command);
}

void xa_get_cpio_line_content (gchar *line, gpointer data)
{
	XArchive *archive = data;
	gchar *filename;
	gpointer item[7];
	gint n = 0, a = 0 ,linesize = 0;

	linesize = strlen(line);
	archive->nr_of_files++;

	/* Permissions */
	line[10] = '\0';
	item[2] = line;
	a = 11;

	/* Hard Link */
	for(n=a; n < linesize && line[n] == ' '; ++n);
	line[++n] = '\0';
	item[4] = line + a;
	n++;
	a = n;

	/* Owner */
	for(; n < linesize && line[n] != ' '; ++n);
	line[n] = '\0';
	item[5] = line + a;
	n++;

	/* Group */
	for(; n < linesize && line[n] == ' '; ++n);
	a = n;

	for(; n < linesize && line[n] != ' '; ++n);
	line[n] = '\0';
	item[6] = line + a;
	n++;

	/* Size */
	for(; n < linesize && line[n] == ' '; ++n);
	a = n;

	for(; n < linesize && line[n] != ' '; ++n);
	line[n] = '\0';
	item[1] = line + a;
	archive->dummy_size += g_ascii_strtoull(item[1],NULL,0);
	n++;

	/* Date */
	line[54] = '\0';
	item[3] = line + n;
	n = 55;

	line[linesize-1] = '\0';
	filename = line + n;

	/* Symbolic link */
	gchar *temp = g_strrstr (filename,"->");
	if (temp)
	{
		a = 3;
		gint len = strlen(filename) - strlen(temp);
		item[0] = filename + a + len;
		filename[strlen(filename) - strlen(temp)] = '\0';
	}
	else
		item[0] = NULL;

	if(line[0] == 'd')
	{
		/* Work around for cpio, which does
		 * not output / with directories */

		if(line[linesize-2] != '/')
			filename = g_strconcat(line + n, "/", NULL);
		else
			filename = g_strdup(line + n);
	}
	else
		filename = g_strdup(line + n);

	xa_set_archive_entries_for_each_row (archive,filename,item);
	g_free (filename);
}

gboolean xa_rpm_extract(XArchive *archive,GSList *files)
{
	gchar *command = NULL,*e_filename = NULL;
	GSList *list = NULL,*_files = NULL;
	GString *names = g_string_new("");
	gboolean result = FALSE;

	if (archive->tmp == NULL)
		if (xa_rpm2cpio(archive) <= 0)
			return FALSE;

	_files = files;
	while (_files)
	{
		e_filename  = xa_escape_filename((gchar*)_files->data,"$'`\"\\!?* ()[]&|:;<>#");
		g_string_prepend (names,e_filename);
		g_string_prepend_c (names,' ');
		_files = _files->next;
	}
	g_slist_foreach(files,(GFunc)g_free,NULL);
	g_slist_free(files);

	chdir (archive->extraction_path);
	command = g_strconcat ( "sh -c \"cpio -id" , names->str," < ",archive->tmp,"/xa-tmp.cpio\"",NULL);

	g_string_free(names,TRUE);
	list = g_slist_append(list,command);
	result = xa_run_command (archive,list);
	return result;
}
