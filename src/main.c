/*
 *  Copyright (c) 2006 Stephan Arts <stephan.arts@hva.nl>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include <glib.h>
#include <libxarchiver/libxarchiver.h>
#include <gtk/gtk.h>

#include "main.h"
#include "property-dialog.h"
#include "main-window.h"


GtkWidget *main_window;
XArchive *archive = NULL;
XArchiveSupport *support = NULL;

void
close_archive(GtkWidget *widget, gpointer data)
{
	xa_main_window_clear_list(XA_MAIN_WINDOW(main_window));
	xarchiver_archive_destroy(archive);
}

void
add_folder(GtkWidget *widget, gpointer data)
{
	GSList *list = g_slist_alloc();
	list->data = (gchar *)data;
	if((archive) && (support))
		support->add(archive, list);
}

void
add_files(GtkWidget *widget, gpointer data)
{
	if((archive) && (support))
		support->add(archive, data);
}


void
open_archive(GtkWidget *widget, gpointer data)
{
	gchar **columns = NULL;
	GType *column_types = NULL;
	unsigned short int nc = 0;
	
	gchar *filename = data;
	
	archive = xarchiver_archive_new(filename, XARCHIVETYPE_UNKNOWN);
	if(archive == NULL)
	{
		//TODO: notify the user with a gtk_dialog error message
		g_warning("Archive %s is not supported\n", filename);
		return;
	}
	else
	{
		if((archive->type == XARCHIVETYPE_BZIP2) || (archive->type == XARCHIVETYPE_GZIP))
		{
			support = xarchiver_find_archive_support(archive);
			support->extract(archive, "/tmp/", NULL, FALSE);
		}
	}
	switch (archive->type)
	{
		case XARCHIVETYPE_RAR:
			nc = 10;
			xarchive_rar_support_open (archive);
			columns = g_new0 ( gchar *,nc);
			column_types = g_new0 ( GType ,nc);
			columns[0] = "Filename";columns[1] = "Original";columns[2] = "Compressed";columns[3] = "Ratio";
			columns[4] = "Date";columns[5] = "Time";columns[6] = "Permissions";columns[7] = "CRC";
			columns[8] = "Method";columns[9] = "Version";
			column_types[0] = G_TYPE_STRING;column_types[1] = G_TYPE_STRING;column_types[2] = G_TYPE_STRING;//G_TYPE_UINT64;
			column_types[3] = G_TYPE_STRING;column_types[4] = G_TYPE_STRING;column_types[5] = G_TYPE_STRING;
			column_types[6] = G_TYPE_STRING;column_types[7] = G_TYPE_STRING;column_types[8] = G_TYPE_STRING;
			column_types[9] = G_TYPE_STRING;
		break;

		case XARCHIVETYPE_TAR:
			nc = 6;
			xarchive_tar_support_open (archive);
			columns = g_new0 ( gchar *,nc);
			column_types = g_new0 ( GType ,nc);
			columns[0] = "Filename";columns[1] = "Permissions";columns[2] = "OwnerGroup";columns[3] = "Size";
			columns[4] = "Date";columns[5] = "Time";
			column_types[0] = G_TYPE_STRING;column_types[1] = G_TYPE_STRING;column_types[2] = G_TYPE_STRING;
			column_types[3] = G_TYPE_STRING;//G_TYPE_UINT64;
			column_types[4] = G_TYPE_STRING;column_types[5] = G_TYPE_STRING;
		break;

		case XARCHIVETYPE_7ZIP:
			nc = 6;
			xarchive_7zip_support_open (archive);
			columns = g_new0 ( gchar *,nc);
			column_types = g_new0 ( GType ,nc);
			columns[0] = "Filename";columns[1] = "Compressed";columns[2] = "Original";columns[3] = "Attr";
			columns[4] = "Time";columns[5] = "Date";
			column_types[0] = G_TYPE_STRING;column_types[1] = G_TYPE_STRING;//G_TYPE_UINT64;
			column_types[2] = G_TYPE_STRING;//G_TYPE_UINT64;
			column_types[3] = G_TYPE_STRING;column_types[4] = G_TYPE_STRING;
			column_types[5] = G_TYPE_STRING;
		break;

		case XARCHIVETYPE_ZIP:
			nc = 8;
			xarchive_zip_support_open (archive);
			columns = g_new0 ( gchar *,nc);
			column_types = g_new0 ( GType ,nc);
			columns[0] = "Filename";columns[1] = "Original";columns[2] = "Method";columns[3] = "Compressed";
			columns[4] = "Ratio";columns[5] = "Date";columns[6] = "Time";columns[7] = "CRC-32";
			column_types[0] = G_TYPE_STRING;column_types[1] = G_TYPE_STRING;//G_TYPE_UINT64;
			column_types[2] = G_TYPE_STRING;column_types[3] = G_TYPE_STRING;//G_TYPE_UINT64
			column_types[4] = G_TYPE_STRING;column_types[5] = G_TYPE_STRING;
			column_types[6] = G_TYPE_STRING;column_types[7] = G_TYPE_STRING;
		break;

		case XARCHIVETYPE_ARJ:
			nc = 9;
			xarchive_arj_support_open (archive);
			columns = g_new0 ( gchar *,nc);
			column_types = g_new0 ( GType ,nc);
			columns[0] = "Filename";columns[1] = "Original";columns[2] = "Compressed";columns[3] = "Ratio";
			columns[4] = "Date";columns[5] = "Time";columns[6] = "Attributes";columns[7] = "GUA";
			columns[8] = "BPMGS";
			column_types[0] = G_TYPE_STRING;column_types[1] = G_TYPE_STRING;//G_TYPE_UINT64;
			column_types[2] = G_TYPE_STRING;//G_TYPE_UINT64;
			column_types[3] = G_TYPE_STRING;column_types[4] = G_TYPE_STRING;column_types[5] = G_TYPE_STRING;
			column_types[6] = G_TYPE_STRING;column_types[7] = G_TYPE_STRING;column_types[8] = G_TYPE_STRING;
		break;

		case XARCHIVETYPE_ISO:
			nc = 5;
			/*
			xarchive_iso_support_open (archive);
			columns = g_new0 ( gchar *,nc);
			column_types = g_new0 ( GType ,nc);
			g_file_name, xname, fstat_buf.st_size, fstat_buf.st_uid, fstat_buf.st_gid)
			columns[0] = "Filename";columns[1] = "xname";columns[2] = "Size";
			columns[3] = "UID";columns[4] = "GID";
			column_types[0] = G_TYPE_STRING;column_types[1] = G_TYPE_STRING;//G_TYPE_UINT64;
			column_types[2] = G_TYPE_STRING;//G_TYPE_UINT64;column_types[3] = G_TYPE_STRING;
			column_types[4] = G_TYPE_STRING;
			*/
		break;
	}
	while (archive->child_pid != 0)
	{
		while (gtk_events_pending())
			gtk_main_iteration();
	}
	archive->row = g_list_reverse ( archive->row );
	xa_main_window_set_list_interface(XA_MAIN_WINDOW(main_window), nc, columns, column_types);
	xa_main_window_append_list(XA_MAIN_WINDOW(main_window), archive->row);
	if(archive->has_passwd)
		xa_main_window_set_widget_visible(XA_MAIN_WINDOW(main_window), "xa-passwd", TRUE);
	else
		xa_main_window_set_widget_visible(XA_MAIN_WINDOW(main_window), "xa-passwd", FALSE);
	//if (archive->has_passwd) gtk_widget_show(viewport);
	g_free (columns);
	g_free (column_types);
	g_print ("Files:%d\nDirs:%d\nArchive Size:%lld\n",archive->number_of_files,archive->number_of_dirs,archive->dummy_size);
	
}

int main(int argc, char **argv)
{
	xarchiver_init();
	gtk_init(&argc, &argv);

	main_window = xa_main_window_new();
	GtkWidget *prop_dialog = xa_property_dialog_new(GTK_WINDOW(main_window));
	xa_property_dialog_add_property(XA_PROPERTY_DIALOG(prop_dialog), "filename", "/etc/passwd");

	g_signal_connect(G_OBJECT(main_window), "destroy", gtk_main_quit, NULL);
	//g_signal_connect(G_OBJECT(main_window), "xa_new_archive", G_CALLBACK(open_archive), NULL);
	g_signal_connect(G_OBJECT(main_window), "xa_open_archive", G_CALLBACK(open_archive), NULL);
	g_signal_connect(G_OBJECT(main_window), "xa_test_archive", G_CALLBACK(open_archive), NULL);
	g_signal_connect(G_OBJECT(main_window), "xa_close_archive", G_CALLBACK(close_archive), NULL);
	g_signal_connect(G_OBJECT(main_window), "xa_extract_archive", G_CALLBACK(close_archive), NULL);
	g_signal_connect(G_OBJECT(main_window), "xa_add_folders", G_CALLBACK(add_folder), NULL);
	g_signal_connect(G_OBJECT(main_window), "xa_add_files", G_CALLBACK(add_files), NULL);
	//g_signal_connect(G_OBJECT(main_window), "xa_save_as_archive", G_CALLBACK(close_archive), NULL);
	//g_signal_connect(G_OBJECT(main_window), "xa_cancel_operation", G_CALLBACK(close_archive), NULL);

	gtk_widget_show_all(main_window);

	xa_main_window_set_widget_sensitive(XA_MAIN_WINDOW(main_window), "xa-button-test", FALSE);
	xa_main_window_set_statusbar_value(XA_MAIN_WINDOW(main_window), "Xarchiver 0.3.9 svn");
	xa_main_window_set_property_window(XA_MAIN_WINDOW(main_window), XA_PROPERTY_DIALOG(prop_dialog));
	
	gtk_main();
	
	xarchiver_destroy();
	return 0;
}
