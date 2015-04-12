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

#include "config.h"
#include "rpm.h"
#include "string_utils.h"
extern gboolean batch_mode;

void xa_open_rpm (XArchive *archive)
{
	unsigned short int i;
    int response;
	GSList *list = NULL;
	FILE *stream;
	gboolean result;

    signal (SIGPIPE, SIG_IGN);
    stream = fopen ( archive->path , "r" );
	if (stream == NULL)
    {
        gchar *msg = g_strdup_printf (_("Can't open RPM file %s:") , archive->path);
		response = xa_show_message_dialog (GTK_WINDOW (xa_main_window) , GTK_DIALOG_MODAL,GTK_MESSAGE_ERROR,GTK_BUTTONS_OK,
		msg,g_strerror(errno));
		g_free (msg);
		return;
    }
    archive->can_extract = archive->has_properties = TRUE;
    archive->can_add = archive->has_sfx = archive->has_test = FALSE;
    archive->dummy_size = 0;
    archive->nr_of_files = 0;
    archive->nc = 8;
	archive->format ="RPM";

	char *names[]= {(_("Points to")),(_("Size")),(_("Permission")),(_("Date")),(_("Hard Link")),(_("Owner")),(_("Group")),NULL};
	GType types[]= {GDK_TYPE_PIXBUF,G_TYPE_STRING,G_TYPE_STRING,G_TYPE_UINT64,G_TYPE_STRING,G_TYPE_STRING,G_TYPE_STRING,G_TYPE_STRING,G_TYPE_STRING,G_TYPE_POINTER};
	archive->column_types = g_malloc0(sizeof(types));
	for (i = 0; i < 10; i++)
		archive->column_types[i] = types[i];

	xa_create_liststore (archive,names);

	/* Create a unique temp dir in /tmp */
	result = xa_create_temp_directory (archive);
	if (!result)
		return;

	/* Now I run dd to have the bzip2 / gzip compressed cpio archive in /tmp */
	gchar *command = g_strconcat ( "sh -c \"rpm2cpio ",archive->escaped_path," > ",archive->tmp, "/file.cpio\"",NULL);
	list = NULL;
	list = g_slist_append(list,command);
	result = xa_run_command (archive,list);
	if (result == FALSE)
	{
		gtk_widget_set_sensitive(Stop_button,FALSE);
		xa_set_button_state (1,1,1,1,archive->can_add,archive->can_extract,0,archive->has_test,archive->has_properties,archive->has_passwd,0);
		gtk_label_set_text(GTK_LABEL(total_label),"");
		return;
	}
	/* And finally cpio to receive the content */
	command = g_strconcat ("sh -c \"cpio -tv < ",archive->tmp,"/file.cpio\"",NULL);
	archive->parse_output = xa_get_cpio_line_content;
	xa_spawn_async_process (archive,command);
	g_free(command);
}

void xa_get_cpio_line_content (gchar *line, gpointer data)
{
	XArchive *archive = data;
	XEntry *entry;
	gchar *filename;
	gpointer item[7];
	gint n = 0, a = 0 ,linesize = 0;
	gboolean dir = FALSE;

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
		dir = TRUE;
		/* Work around for cpio, which does
		 * not output / with directories */

		if(line[linesize-2] != '/')
			filename = g_strconcat(line + n, "/", NULL); 
		else
			filename = g_strdup(line + n); 
	}
	else
		filename = g_strdup(line + n); 
	
	entry = xa_set_archive_entries_for_each_row (archive,filename,item);
	g_free (filename);
}

gboolean xa_rpm_extract(XArchive *archive,GSList *files)
{
	gchar *command = NULL,*e_filename = NULL;
	GSList *list = NULL,*_files = NULL;
	GString *names = g_string_new("");
	gboolean result = FALSE;

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
	command = g_strconcat ( "sh -c \"cpio -id" , names->str," < ",archive->tmp,"/file.cpio\"",NULL);

	g_string_free(names,TRUE);
	list = g_slist_append(list,command);
	result = xa_run_command (archive,list);
	return result;
}
