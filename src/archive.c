/*
 *  Copyright (c) 2008 Giuseppe Torelli <colossus73@gmail.com>
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
#include <gtk/gtk.h>
#include <sys/param.h>
#include "config.h"
#include "archive.h"
#include "support.h"
#include "window.h"

#ifndef NCARGS
#define NCARGS _POSIX_ARG_MAX
#endif

#define MAX_CMD_LEN (NCARGS * 2 / 3)

extern open_func	open_archive[XARCHIVETYPE_COUNT];
extern delete_func	delete	[XARCHIVETYPE_COUNT];
extern add_func		add	[XARCHIVETYPE_COUNT];
extern extract_func extract	[XARCHIVETYPE_COUNT];
extern test_func	test	[XARCHIVETYPE_COUNT];
extern Prefs_dialog_data *prefs_window;
extern gboolean batch_mode;

Progress_bar_data *pb = NULL;

static gboolean xa_process_output (GIOChannel *ioc, GIOCondition cond, gpointer data);
static gboolean xa_process_output_from_command_line (GIOChannel *ioc,GIOCondition cond,gpointer data);

XArchive *xa_init_archive_structure(gint type)
{
	XEntry *entry = NULL;
	XArchive *archive = NULL;

	archive = g_new0(XArchive,1);
	if (archive == NULL)
		return NULL;
	entry = g_new0(XEntry,1);
	entry->filename = "";
	archive->root_entry = entry;
	archive->open_archive =	open_archive[type];
	archive->delete =	delete[type];
	archive->add =		add[type];
	archive->extract = 	extract[type];
	archive->test = 	test[type];
	archive->type = type;
	return archive;
}

void xa_spawn_async_process (XArchive *archive, gchar *command)
{
	GIOChannel *ioc,*err_ioc;
	gchar **argv;
	gint argcp, response;
	GError *error = NULL;

	g_shell_parse_argv (command,&argcp,&argv,NULL);
	if ( ! g_spawn_async_with_pipes (
		archive->working_dir,
		argv,
		NULL,
		(G_SPAWN_LEAVE_DESCRIPTORS_OPEN | G_SPAWN_SEARCH_PATH | G_SPAWN_DO_NOT_REAP_CHILD),
		NULL,
		NULL,
		&archive->child_pid,
		NULL,
		&archive->output_fd,
		&archive->error_fd,
		&error))
	{
		response = xa_show_message_dialog (NULL,GTK_DIALOG_MODAL,GTK_MESSAGE_ERROR,GTK_BUTTONS_OK, _("Can't run the archiver executable:"),error->message);
		g_error_free (error);
		g_strfreev (argv);
		archive->child_pid = 0;
		xa_set_button_state (1,1,1,1,archive->can_add,archive->can_extract,archive->has_sfx,archive->has_test,archive->has_properties,archive->has_passwd,1);
		return;
	}
	g_strfreev (argv);

	if (archive->status == XA_ARCHIVESTATUS_OPEN)
		archive->pb_source = g_timeout_add (350,(GSourceFunc)xa_flash_led_indicator,archive);

	if (archive->error_output != NULL)
	{
		g_slist_foreach (archive->error_output,(GFunc)g_free,NULL);
		g_slist_free (archive->error_output);
		archive->error_output = NULL;
	}

	ioc = g_io_channel_unix_new (archive->output_fd);
	g_io_channel_set_encoding (ioc,NULL,NULL);
	g_io_channel_set_flags (ioc,G_IO_FLAG_NONBLOCK,NULL);
	
	if (xa_main_window)
		g_io_add_watch (ioc, G_IO_IN|G_IO_PRI|G_IO_ERR|G_IO_HUP|G_IO_NVAL,xa_process_output,archive);
	else
		g_io_add_watch (ioc, G_IO_IN|G_IO_PRI|G_IO_ERR|G_IO_HUP|G_IO_NVAL,xa_process_output_from_command_line,archive);

	if (archive->parse_output)
		g_child_watch_add_full (G_PRIORITY_LOW,archive->child_pid, (GChildWatchFunc)xa_watch_child,archive,NULL);

	err_ioc = g_io_channel_unix_new (archive->error_fd);
	g_io_channel_set_encoding (err_ioc,locale,NULL);
	g_io_channel_set_flags (err_ioc,G_IO_FLAG_NONBLOCK,NULL);
	g_io_add_watch (err_ioc,G_IO_IN|G_IO_PRI|G_IO_ERR|G_IO_HUP|G_IO_NVAL,xa_dump_child_error_messages,archive);
}

/*	TODO: workaround for bug #3235
 * 
 * gchar *xa_split_command_line(XArchive *archive,GSList *list)
{
	gchar *command = NULL;
	GSList *chunks = NULL;
	GSList *scan = NULL;
	int length;

	for (scan = list; scan != NULL;)
	{
		length = 0;
		while ((scan != NULL) && (length < 5000)) //MAX_CMD_LEN
		{
			length += strlen (scan->data);
			chunks = g_slist_prepend(chunks,scan->data);
			scan = scan->next;
		}
		chunks = g_slist_prepend(chunks,"****** ");
	}
	chunks = g_slist_reverse(chunks);
	return command;
}
*/

static gboolean xa_process_output_from_command_line (GIOChannel *ioc,GIOCondition cond,gpointer data)
{
	XArchive *archive = data;
	GIOStatus status;
	gchar *line = NULL;

	if (cond & (G_IO_IN | G_IO_PRI))
	{
		do
		{
			status = g_io_channel_read_line (ioc, &line, NULL, NULL, NULL);
			if (line != NULL)
			{
				if (pb->multi_extract == FALSE)
					xa_increase_progress_bar(pb,line,0.0);
				g_free(line);
			}
		}
		while (status == G_IO_STATUS_NORMAL);
		if (status == G_IO_STATUS_ERROR || status == G_IO_STATUS_EOF)
			goto done;
	}
	else if (cond & (G_IO_ERR | G_IO_HUP | G_IO_NVAL))
	{
	done:
		if (archive->error_output != NULL)
			archive->error_output = g_slist_reverse (archive->error_output);
		g_io_channel_shutdown (ioc,TRUE,NULL);
		g_io_channel_unref (ioc);
		return FALSE;
	}
	return TRUE;
}

static gboolean xa_process_output (GIOChannel *ioc,GIOCondition cond,gpointer data)
{
	XArchive *archive = data;
	GIOStatus status;
	gchar *line = NULL;

	if (cond & (G_IO_IN | G_IO_PRI))
	{
		do
		{
			status = g_io_channel_read_line (ioc, &line, NULL, NULL, NULL);
			if (line != NULL)
			{
				if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(prefs_window->store_output)))
					archive->error_output = g_slist_prepend (archive->error_output,g_strdup(line));

				if (archive->parse_output)
					(*archive->parse_output) (line,archive);
				g_free(line);
			}
			while (gtk_events_pending())
				gtk_main_iteration();
		}
		while (status == G_IO_STATUS_NORMAL);
		if (status == G_IO_STATUS_ERROR || status == G_IO_STATUS_EOF)
			goto done;
	}
	else if (cond & (G_IO_ERR | G_IO_HUP | G_IO_NVAL))
	{
	done:
		if (archive->error_output != NULL)
			archive->error_output = g_slist_reverse (archive->error_output);
		g_io_channel_shutdown (ioc,TRUE,NULL);
		g_io_channel_unref (ioc);

		if (archive->parse_output)
		{
			if (archive->has_comment && archive->status == XA_ARCHIVESTATUS_OPEN && gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(prefs_window->check_show_comment)))
				xa_show_archive_comment (NULL,NULL);

			xa_update_window_with_archive_entries (archive,NULL);
			gtk_tree_view_set_model (GTK_TREE_VIEW(archive->treeview),archive->model);
			g_object_unref (archive->model);
			
			if (archive->type == XARCHIVETYPE_ZIP || archive->type == XARCHIVETYPE_RAR || archive->type == XARCHIVETYPE_ARJ)
				gtk_widget_set_sensitive (comment_menu,TRUE);
			else
				gtk_widget_set_sensitive (comment_menu,FALSE);

			if (archive->type == XARCHIVETYPE_TAR || is_tar_compressed(archive->type))
				gtk_widget_set_sensitive (password_entry_menu,FALSE);
			else
				gtk_widget_set_sensitive (password_entry_menu,TRUE);

			gtk_widget_set_sensitive(listing,TRUE);

			if (GTK_IS_TREE_VIEW(archive->treeview))
				gtk_widget_grab_focus (GTK_WIDGET(archive->treeview));

			xa_set_statusbar_message_for_displayed_rows(archive);

			if (archive->status == XA_ARCHIVESTATUS_TEST)
			{
				archive->create_image = FALSE;
				xa_show_cmd_line_output (NULL,archive);
			}
			if (archive->status == XA_ARCHIVESTATUS_OPEN)
				xa_set_button_state (1,1,1,1,archive->can_add,archive->can_extract,archive->has_sfx,archive->has_test,archive->has_properties,archive->has_passwd,1);
		}
		return FALSE;
	}
	return TRUE;
}

gboolean xa_dump_child_error_messages (GIOChannel *ioc,GIOCondition cond,gpointer data)
{
	XArchive *archive = data;
	GIOStatus status;
	gchar *line = NULL;

	if (cond & (G_IO_IN | G_IO_PRI))
	{
		do
		{
			status = g_io_channel_read_line (ioc, &line, NULL, NULL, NULL);
			if (line != NULL)
			{
				if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(prefs_window->store_output)))
					archive->error_output = g_slist_prepend (archive->error_output,g_strdup(line));
				g_free(line);
			}
		}
		while (status == G_IO_STATUS_NORMAL);
		if (status == G_IO_STATUS_ERROR || status == G_IO_STATUS_EOF)
			goto done;
	}
	else if (cond & (G_IO_ERR | G_IO_HUP | G_IO_NVAL))
	{
	done:
		g_io_channel_shutdown (ioc, TRUE, NULL);
		g_io_channel_unref (ioc);
		return FALSE;
	}
	return TRUE;
}

void xa_clean_archive_structure (XArchive *archive)
{
	XEntry *entry;

	if (archive == NULL)
		return;

	entry = archive->root_entry;
	xa_free_entry (archive,entry);
	
	if (archive->column_types != NULL)
		g_free(archive->column_types);

	if (archive->error_output != NULL)
	{
		g_slist_foreach (archive->error_output, (GFunc) g_free, NULL);
		g_slist_free (archive->error_output);
		archive->error_output = NULL;
	}
	
	if (archive->path != NULL)
		g_free(archive->path);

	if (archive->escaped_path != NULL)
		g_free(archive->escaped_path);

	if (archive->tmp != NULL)
	{
		xa_delete_temp_directory (archive);
		g_free (archive->tmp);
	}

	if (archive->passwd != NULL)
		g_free (archive->passwd);
	
	if (archive->working_dir != NULL)
		g_free (archive->working_dir);

	if (archive->extraction_path != NULL)
		g_free (archive->extraction_path);

	if (archive->has_comment)
	{
		if (archive->comment != NULL)
			g_string_free (archive->comment,TRUE);
	}
	if (archive->clipboard_data)
		xa_clipboard_clear(NULL,archive);
	g_free (archive);
}

void xa_delete_temp_directory (XArchive *archive)
{
	if (xa_main_window)
		xa_launch_external_program("rm -rf",archive->tmp);
	else
	{
		char *argv[4];
		argv[0] = "rm";
		argv[1] = "-rf";
		argv[2] = archive->tmp;
		argv[3] = NULL;
		g_spawn_sync (NULL, argv, NULL,G_SPAWN_SEARCH_PATH,NULL, NULL,NULL,NULL, NULL,NULL);
	}
}

gboolean xa_create_temp_directory (XArchive *archive)
{
	gchar *tmp_dir;
	gchar *value;
	int response;

	if (archive->tmp != NULL)
		return TRUE;

	value = gtk_combo_box_get_active_text (GTK_COMBO_BOX(prefs_window->combo_prefered_temp_dir));
	tmp_dir = g_strconcat(value,"/xa-XXXXXX",NULL);

	if (mkdtemp (tmp_dir) == 0)
	{
		g_free(tmp_dir);
		tmp_dir = NULL;

		response = xa_show_message_dialog (GTK_WINDOW (xa_main_window),GTK_DIALOG_MODAL,GTK_MESSAGE_ERROR,GTK_BUTTONS_OK,_("Can't create temporary directory in /tmp:"),g_strerror(errno));
		return FALSE;
	}

	archive->tmp = tmp_dir;
	return TRUE;
}

gboolean xa_run_command (XArchive *archive,GSList *commands)
{
	int ps;
	gboolean waiting = TRUE;
	gboolean result = TRUE;
	GSList *_commands = commands;

	archive->parse_output = NULL;
	if (xa_main_window)
	{
		gtk_widget_set_sensitive (Stop_button,TRUE);
		if (archive->pb_source == 0)
			archive->pb_source = g_timeout_add (350,(GSourceFunc)xa_flash_led_indicator,archive);
	}
	else
	{
		pb = xa_create_progress_bar(TRUE,archive);
		if (archive->pb_source == 0 && pb->multi_extract == FALSE)
			archive->pb_source = g_timeout_add (100,(GSourceFunc)xa_pulse_progress_bar_window,pb);
	}

	while (_commands)
	{
		xa_spawn_async_process (archive,_commands->data);
		if (archive->child_pid == 0)
		{
			result = FALSE;
			break;
		}
		while (waiting)
		{
			ps = waitpid (archive->child_pid, &status, WNOHANG);
			if (ps < 0)
				break;
			while (gtk_events_pending())
				gtk_main_iteration();
		}
		if (WIFEXITED (status))
		{
			if (WEXITSTATUS (status))
			{
				result = FALSE;
				break;
			}
		}
		_commands = _commands->next;
	}
	xa_watch_child (archive->child_pid, status, archive);
	if (xa_main_window)
		xa_set_button_state (1,1,1,1,archive->can_add,archive->can_extract,archive->has_sfx,archive->has_test,archive->has_properties,1,1);

	g_slist_foreach (commands,(GFunc) g_free,NULL);
	g_slist_free(commands);
	return result;
}

gint xa_find_archive_index (gint page_num)
{
	GtkWidget *scrollwindow;
	gint i;

	scrollwindow = gtk_notebook_get_nth_page(notebook, page_num);
	for (i = 0; i < 99; i++)
	{
		if (archive[i] != NULL && archive[i]->scrollwindow == scrollwindow)
			return i;
	}
	return -1;
}

gint xa_get_new_archive_idx()
{
	gint i;

	for(i = 0; i < 99; i++)
	{
		if (archive[i] == NULL)
			return i;
	}
	return -1;
}

/* This switch is taken from Squeeze source code */
XEntry *xa_alloc_memory_for_each_row (guint nc,GType column_types[])
{
	XEntry *entry = NULL;
	unsigned short int i;
	gint size = 0;

	entry = g_new0(XEntry,1);
	if (entry == NULL)
		return NULL;

	for (i = 0; i < nc+2; i++)
	{
		switch(column_types[i])
		{
			case G_TYPE_STRING:
				size += sizeof(gchar *);
			break;

			case G_TYPE_UINT64:
				size += sizeof(guint64);
			break;
		}
	}
	entry->columns = g_malloc0 (size);
	return entry;
}

void xa_free_entry (XArchive *archive,XEntry *entry)
{
	gpointer current_column;
	unsigned short int i;

	if (entry->child)
		xa_free_entry(archive,entry->child);

	if (entry->next)
		xa_free_entry(archive,entry->next);

	current_column = entry->columns;

	if (strlen(entry->filename) > 0)
	{
		for (i = 0; i < archive->nc; i++)
		{
			switch(archive->column_types[i+2])
			{
				case G_TYPE_STRING:
					g_free (*((gchar **)current_column));
					current_column += sizeof(gchar *);
				break;

				case G_TYPE_UINT64:
					current_column += sizeof(guint64);
				break;
			}
		}
		g_free(entry->columns);
		g_free(entry->filename);
	}
	g_free(entry);
}

XEntry *xa_find_child_entry(XEntry *entry, gchar *string)
{
	if (entry == NULL)
		return NULL;
	if (entry->is_dir && strcmp(entry->filename, string) == 0)
		return entry;

  return xa_find_child_entry(entry->next, string);
}

XEntry *xa_set_archive_entries_for_each_row (XArchive *archive,gchar *filename,gpointer *items)
{
	XEntry *new_entry= NULL;
	XEntry *last_entry = archive->root_entry;
	gchar **components = NULL;
	unsigned short int x = 0;

	components = g_strsplit(filename,"/",-1);

	while (components[x] && strlen(components[x]) > 0)
	{
		new_entry = xa_find_child_entry(last_entry->child,components[x]);
		if (new_entry == NULL)
		{
			new_entry = xa_alloc_memory_for_each_row(archive->nc,archive->column_types);
			new_entry->filename = g_strdup(components[x]);
			new_entry->columns = xa_fill_archive_entry_columns_for_each_row(archive,new_entry,items);
			if (components[x+1] != NULL)
				new_entry->is_dir = TRUE;
			new_entry->next = last_entry->child;
			last_entry->child = new_entry;
			new_entry->prev = last_entry;
		}
		last_entry = new_entry;
		x++;
	}
	g_strfreev(components);
	return new_entry;
}

gpointer *xa_fill_archive_entry_columns_for_each_row (XArchive *archive,XEntry *entry,gpointer *items)
{
	unsigned int i;
	gpointer current_column;

	current_column = entry->columns;

	for (i = 0; i < archive->nc; i++)
	{
		switch(archive->column_types[i+2])
		{
			case G_TYPE_STRING:
				(*((gchar **)current_column)) = g_strdup((gchar*)items[i]);
				//g_message ("%d - %s",i,(*((gchar **)current_column)));
				current_column += sizeof(gchar *);
			break;

			case G_TYPE_UINT64:
				(*((guint64 *)current_column)) = atol(items[i]);
				//g_message ("*%d - %lu",i,(*((guint64 *)current_column)));
				current_column += sizeof(guint64);
			break;
		}
	}
	return entry->columns;
}

XEntry* xa_find_entry_from_path (XEntry *root_entry,const gchar *fullpathname)
{
	gchar **components = NULL;
	unsigned short int x = 0;
	XEntry *new_entry = NULL;

	components = g_strsplit(fullpathname,"/",-1);
	while (components[x] && strlen(components[x]) > 0)
	{
		new_entry = xa_find_child_entry(root_entry->child,components[x]);
		root_entry = new_entry;
		x++;
	}
	g_strfreev(components);
	return new_entry;
}

gchar *xa_build_full_path_name_from_entry(XEntry *entry, XArchive *archive)
{
	gchar *fullpathname = NULL;
	GString *dummy = g_string_new("");
	gint n = 0;

	while (entry)
	{
		if (entry->is_dir)
			dummy = g_string_prepend_c(dummy,'/');
		
		dummy = g_string_prepend(dummy,entry->filename);
		entry = entry->prev;
	}
	if (archive == NULL)
		goto there;

	n = strlen(dummy->str)-1;
	if (archive->status == XA_ARCHIVESTATUS_DELETE && dummy->str[n] == '/' && archive->type != XARCHIVETYPE_ZIP)
		fullpathname = g_strndup(dummy->str,n);
	else
there:
		fullpathname = g_strdup(dummy->str);

	g_string_free(dummy,TRUE);
	return fullpathname;
}

void xa_fill_list_with_recursed_entries(XEntry *entry,GSList **p_file_list)
{
	if (entry == NULL)
		return;

	xa_fill_list_with_recursed_entries(entry->next ,p_file_list);
	xa_fill_list_with_recursed_entries(entry->child,p_file_list);
	*p_file_list = g_slist_prepend (*p_file_list,xa_build_full_path_name_from_entry(entry,NULL));
}

gboolean xa_detect_encrypted_archive (XArchive *archive)
{
	FILE *file;
    unsigned int fseek_offset;
    unsigned short int password_flag;
    unsigned int compressed_size;
    unsigned int uncompressed_size;
    unsigned short int file_length;
    unsigned short int extra_length;

	unsigned char sig[2];
	unsigned short int basic_header_size;
	unsigned short int extended_header_size;
	unsigned int basic_header_CRC;
	unsigned int extended_header_CRC;
	unsigned char arj_flag;
	unsigned char magic[4];
	gboolean flag = FALSE;

	file = fopen (archive->path,"r");
	fread (magic,1,4,file);
	
	fseek (file,6,SEEK_SET);
	if (archive->type == XARCHIVETYPE_ZIP)
	{
		while (memcmp (magic,"\x50\x4b\x03\x04",4) == 0  || memcmp (magic,"\x50\x4b\x05\x06",4) == 0)
		{
			fread (&password_flag,1,2,file);
			if ((password_flag & ( 1<<0)) > 0)
			{
				flag = TRUE;
				break;
			}
			fseek (file,10,SEEK_CUR);
			fread (&compressed_size,1,4,file);
			fread (&uncompressed_size,1,4,file);
			fread (&file_length,1,2,file);
			/* If the zip archive is empty (no files) it should return here */
			if (fread (&extra_length,1,2,file) < 2)
			{
				flag = FALSE;
				break;
			}
			fseek_offset = compressed_size + file_length + extra_length;
			fseek (file,fseek_offset,SEEK_CUR);
			fread (magic,1,4,file);
			fseek (file,2,SEEK_CUR);
		}
	}
	else if (archive->type == XARCHIVETYPE_ARJ)
	{
		fseek (file,magic[2]+magic[3],SEEK_CUR);
		fseek (file,2,SEEK_CUR);
		fread (&extended_header_size,1,2,file);
		if (extended_header_size != 0)
			fread (&extended_header_CRC,1,4,file);
		fread (&sig,1,2,file);
		while ( memcmp (sig,"\x60\xea",2) == 0)
		{
			fread ( &basic_header_size,1,2,file);
			if ( basic_header_size == 0)
				break;
			fseek ( file , 4 , SEEK_CUR);
			fread (&arj_flag,1,1,file);
			if ((arj_flag & ( 1<<0)) > 0)
			{
				flag = TRUE;
				break;
			}
			fseek (file,7,SEEK_CUR);
			fread (&compressed_size,1,4,file);
			fseek (file,basic_header_size - 16,SEEK_CUR);
			fread (&basic_header_CRC,1,4,file);
			fread (&extended_header_size,1,2,file);
			if (extended_header_size != 0)
				fread (&extended_header_CRC,1,4,file);
			fseek (file,compressed_size,SEEK_CUR);
			fread (&sig,1,2,file);
		}
	}
	fclose (file);
	return flag;
}

void xa_browse_dir_sidebar (XEntry *entry, GtkTreeStore *model,gchar *path, GtkTreeIter *containing_iter)
{
	GtkTreeIter child_iter;

	if (!entry)
		return;

	if (strlen(entry->filename) == 0)
		return xa_browse_dir_sidebar(entry->child, model, path, containing_iter);

	if (entry->is_dir)
	{
		gtk_tree_store_append(model,&child_iter,containing_iter);
		gtk_tree_store_set(model,&child_iter,0,"gtk-directory",1,entry->filename,2,entry,-1);
	}
	xa_browse_dir_sidebar(entry->child,model,NULL,&child_iter);
	xa_browse_dir_sidebar(entry->next, model,NULL,containing_iter);

}

void xa_fill_dir_sidebar(XArchive *archive,gboolean force_reload)
{
	GtkTreeIter iter;

	if (gtk_tree_model_get_iter_first(GTK_TREE_MODEL(archive_dir_model),&iter) && force_reload == FALSE)
		return;

	gtk_tree_store_clear(GTK_TREE_STORE(archive_dir_model));
	xa_browse_dir_sidebar(archive->root_entry,archive_dir_model,NULL,NULL);
}

void xa_sidepane_row_selected(GtkTreeSelection *selection, gpointer data)
{
	XEntry *entry;
	GtkTreeIter iter;
	GtkTreeIter parent;
	GtkTreePath *path;
	GtkTreeModel *model;
	GString *string = g_string_new("");
	gchar *dir;
	gint current_page;
	gint idx;

	current_page = gtk_notebook_get_current_page(notebook);
	idx = xa_find_archive_index(current_page);

	if (gtk_tree_selection_get_selected (selection,&model,&iter))
	{
		path = gtk_tree_model_get_path(model,&iter);
		if ( ! gtk_tree_view_row_expanded(GTK_TREE_VIEW(archive_dir_treeview),path))
			gtk_tree_view_expand_to_path(GTK_TREE_VIEW(archive_dir_treeview),path);
		gtk_tree_path_free(path);
		/* Let get the last selected dir */
		gtk_tree_model_get(model,&iter,1,&dir,-1);
		g_string_prepend_c(string,'/');
		g_string_prepend(string,dir);
		/* Get the memory address of entry so to update the main listview */
		gtk_tree_model_get(model,&iter,2,&entry,-1);
		while (gtk_tree_model_iter_parent(model,&parent,&iter))
		{
			gtk_tree_model_get(model,&parent,1,&dir,-1);
			g_string_prepend_c(string,'/');
			g_string_prepend(string,dir);
			iter = parent;
		}
		gtk_entry_set_text(GTK_ENTRY(location_entry),string->str);
		g_string_free(string,TRUE);

		xa_update_window_with_archive_entries(archive[idx],entry);
		xa_set_statusbar_message_for_displayed_rows(archive[idx]);
	}
}

void xa_sidepane_select_row(XEntry *entry)
{
	gtk_tree_model_foreach(GTK_TREE_MODEL(archive_dir_model),(GtkTreeModelForeachFunc)_xa_sidepane_select_row,entry);
}

gboolean _xa_sidepane_select_row(GtkTreeModel *model,GtkTreePath *path,GtkTreeIter *iter,gpointer data)
{
	XEntry *entry = data;
	XEntry *entry2;
	GtkTreeIter parent;
	gboolean value;

	gtk_tree_model_get (model,iter,2,&entry2,-1);
	if (entry == entry2)
	{
		gtk_tree_model_iter_parent(model,&parent,iter);
		if ( ! gtk_tree_view_row_expanded(GTK_TREE_VIEW(archive_dir_treeview),path))
			gtk_tree_view_expand_to_path(GTK_TREE_VIEW(archive_dir_treeview),path);

		gtk_tree_selection_select_iter(gtk_tree_view_get_selection (GTK_TREE_VIEW (archive_dir_treeview)),iter);
		gtk_tree_view_scroll_to_cell (GTK_TREE_VIEW(archive_dir_treeview),path,NULL,FALSE,0,0);
		value = TRUE;
	}
	else
		value = FALSE;

	return value;
}

gint xa_sort_dirs_before_files(GtkTreeModel *model,GtkTreeIter *a,GtkTreeIter *b,gpointer data)
{
	XEntry *entry1, *entry2;
	XArchive *archive = data;

	gtk_tree_model_get(model,a,archive->nc+1,&entry1,-1);
	gtk_tree_model_get(model,b,archive->nc+1,&entry2,-1);
	if (entry1->is_dir != entry2->is_dir)
	{
		if (entry1->is_dir)
			return -1;
		else
			return 1;
	}
	/* This for sorting the files */
	return strcasecmp (entry1->filename,entry2->filename);
}
