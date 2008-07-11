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
#include "config.h"
#include "archive.h"
#include "support.h"
#include "window.h"

extern delete_func	delete	[XARCHIVETYPE_COUNT];
extern add_func		add	[XARCHIVETYPE_COUNT];
extern extract_func 	extract	[XARCHIVETYPE_COUNT];
extern test_func	test	[XARCHIVETYPE_COUNT];
extern Prefs_dialog_data *prefs_window;
extern gboolean batch_mode;

static gboolean xa_process_output (GIOChannel *ioc, GIOCondition cond, gpointer data);

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
	archive->delete =	delete[type];
	archive->add = 		add[type];
	archive->extract = 	extract[type];
	archive->test = 	test[type];
	return archive;
}

void xa_spawn_async_process (XArchive *archive, gchar *command)
{
	GIOChannel *ioc,*err_ioc;
	gchar **argv;
	gint argcp, response;
	GError *error = NULL;

	g_shell_parse_argv ( command,&argcp,&argv,NULL);
	if ( ! g_spawn_async_with_pipes (
		NULL,
		argv,
		NULL,
		G_SPAWN_LEAVE_DESCRIPTORS_OPEN | G_SPAWN_SEARCH_PATH | G_SPAWN_DO_NOT_REAP_CHILD,
		NULL,
		NULL,
		&archive->child_pid,
		NULL,
		&archive->output_fd,
		&archive->error_fd,
		&error) )
	{
		response = xa_show_message_dialog (NULL,GTK_DIALOG_MODAL,GTK_MESSAGE_ERROR,GTK_BUTTONS_OK, _("Can't run the archiver executable:"),error->message);
		g_error_free (error);
		g_strfreev ( argv );
		archive->child_pid = 0;
		xa_set_button_state (1,1,1,1,archive->can_add,archive->can_extract,archive->has_sfx,archive->has_test,archive->has_properties);
		return;
	}
	g_strfreev ( argv );

	if (archive->pb_source == 0)
		archive->pb_source = g_timeout_add (200, xa_progressbar_pulse, NULL );

	if (archive->error_output != NULL)
	{
		g_slist_foreach (archive->error_output, (GFunc) g_free, NULL);
		g_slist_free (archive->error_output);
		archive->error_output = NULL;
	}
	if (archive->parse_output)
	{
		ioc = g_io_channel_unix_new (archive->output_fd);
		g_io_channel_set_encoding (ioc, NULL , NULL);
		g_io_channel_set_flags ( ioc , G_IO_FLAG_NONBLOCK , NULL );
	}

	if (archive->parse_output)
	{
		g_io_add_watch (ioc, G_IO_IN|G_IO_PRI|G_IO_ERR|G_IO_HUP|G_IO_NVAL, xa_process_output, archive);
		g_child_watch_add (archive->child_pid, (GChildWatchFunc)xa_watch_child, archive);
	}

	err_ioc = g_io_channel_unix_new (archive->error_fd);
	g_io_channel_set_encoding (err_ioc,locale,NULL);
	g_io_channel_set_flags (err_ioc,G_IO_FLAG_NONBLOCK,NULL);
	g_io_add_watch (err_ioc,G_IO_IN|G_IO_PRI|G_IO_ERR|G_IO_HUP|G_IO_NVAL, xa_dump_child_error_messages, archive);
}

static gboolean xa_process_output (GIOChannel *ioc, GIOCondition cond, gpointer data)
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
	else if (cond & (G_IO_ERR | G_IO_HUP | G_IO_NVAL) )
	{
	done:
		g_io_channel_shutdown (ioc,TRUE,NULL);
		g_io_channel_unref (ioc);

		xa_update_window_with_archive_entries (archive,NULL);
		gtk_tree_view_set_model (GTK_TREE_VIEW(archive->treeview),archive->model);
		g_object_unref (archive->model);
		return FALSE;
	}
	return TRUE;
}

gboolean xa_dump_child_error_messages (GIOChannel *ioc, GIOCondition cond, gpointer data)
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
	else if (cond & (G_IO_ERR | G_IO_HUP | G_IO_NVAL) )
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
	{
		g_free(archive->path);
		archive->path = NULL;
	}

	if (archive->escaped_path != NULL)
	{
		g_free(archive->escaped_path);
		archive->escaped_path = NULL;
	}

	if (archive->tmp != NULL)
	{
		xa_delete_temp_directory (archive,0);
		if(xa_main_window)
			gtk_widget_hide(viewport2);
		g_free (archive->tmp);
		archive->tmp = NULL;
	}

	if (archive->passwd != NULL)
	{
		g_free (archive->passwd);
		archive->passwd = NULL;
	}

	if (archive->extraction_path != NULL)
		g_free (archive->extraction_path);

	if (archive->has_comment)
	{
		if (archive->comment != NULL)
		{
			g_string_free (archive->comment,FALSE);
			archive->comment = NULL;
		}
	}
	g_free (archive);
}

void xa_delete_temp_directory (XArchive *archive,gboolean flag)
{
	GSList *list = NULL;
	gchar *command;

	command = g_strconcat ("rm -rf ",archive->tmp,NULL);
	list = g_slist_append(list,command);
	xa_run_command (archive,list);
}

gboolean xa_create_temp_directory (XArchive *archive)
{
	gchar *tmp_dir;
	gchar *value;

	if (archive->tmp != NULL)
		return TRUE;

	value = gtk_combo_box_get_active_text (GTK_COMBO_BOX(prefs_window->combo_prefered_temp_dir));
	tmp_dir = g_strconcat(value,"/xa-XXXXXX",NULL);

	if (mkdtemp (tmp_dir) == 0)
	{
		g_free(tmp_dir);
		tmp_dir = NULL;

		response = xa_show_message_dialog (GTK_WINDOW (xa_main_window),GTK_DIALOG_MODAL,GTK_MESSAGE_ERROR,GTK_BUTTONS_OK,_("Can't create temporary directory in /tmp:"),g_strerror(errno) );
		return FALSE;
	}

	archive->tmp = tmp_dir;
	return TRUE;
}

gboolean xa_run_command (XArchive *archive,GSList *commands)
{
	int ps,argcp;
	gboolean waiting = TRUE;
	gboolean result = FALSE;
	GSList *_commands = commands;

	GError *error = NULL;
    gchar *std_out,*std_err,*new_std_err,*dummy;
    gchar **argv;

	if (batch_mode)
	{
		while (_commands)
		{
			g_print ("%s\n",(gchar*)_commands->data);
			g_shell_parse_argv(_commands->data,&argcp,&argv,NULL);
			if ( ! g_spawn_sync(
				NULL,
				argv,
				NULL,
				G_SPAWN_SEARCH_PATH,
				NULL,
				NULL,
				&std_out,
				&std_err,
				&status,
				&error))
			{
				response = xa_show_message_dialog (GTK_WINDOW (xa_main_window),GTK_DIALOG_MODAL,GTK_MESSAGE_ERROR,GTK_BUTTONS_OK, _("Can't spawn the command:"),error->message);
				g_error_free (error);
				g_strfreev (argv);
				goto here;
			}
			if (WIFEXITED(status))
			{
				if (WEXITSTATUS(status))
				{
					if (strlen(std_err) > 1954)
					{
						new_std_err = g_strndup(std_err,1954);
						dummy = g_strconcat(new_std_err,_("\n\n** Output was shortened; too many errors!"),NULL);
						g_free(new_std_err);
						response = xa_show_message_dialog (NULL,GTK_DIALOG_MODAL,GTK_MESSAGE_ERROR,GTK_BUTTONS_OK,_("An error occurred!"),dummy);
						g_free(dummy);
					}
					else
						response = xa_show_message_dialog (NULL,GTK_DIALOG_MODAL,GTK_MESSAGE_ERROR,GTK_BUTTONS_OK,_("An error occurred!"),std_err);
				}
				else
					result = TRUE;
			}
			_commands = _commands->next;
		}
		g_strfreev (argv);
		goto here;
	}
	else
	{
		archive->parse_output = 0;
		gtk_widget_show (viewport2);
		gtk_widget_set_sensitive (Stop_button,TRUE);
		while (_commands)
		{
			g_print ("%s\n",(gchar*)_commands->data);
			xa_spawn_async_process (archive,_commands->data);
			if (archive->child_pid == 0)
				break;
			while (waiting)
			{
				ps = waitpid (archive->child_pid, &status, WNOHANG);
				if (ps < 0)
					break;
				else if(xa_main_window)
					gtk_main_iteration_do (FALSE);
			}
			result = xa_check_child_for_error_on_exit(archive,status);
			if (result == FALSE)
				break;
			_commands = _commands->next;
		}
		xa_archive_operation_finished(archive);
here:
		g_slist_foreach (commands,(GFunc) g_free,NULL);
		g_slist_free(commands);
		
	}
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
			{
				new_entry->is_dir = TRUE;
				archive->nr_of_dirs++;
			}
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

gchar *xa_build_full_path_name_from_entry(XEntry *entry)
{
	gchar *fullpathname = NULL;
	GString *dummy = g_string_new("");
	
	while (entry)
	{
		if (entry->is_dir)
			dummy = g_string_prepend_c(dummy,'/');
		dummy = g_string_prepend(dummy,entry->filename);
		entry = entry->prev;
	}
	fullpathname = g_strdup(dummy->str);
	g_string_free(dummy,TRUE);
	return fullpathname;
}

void xa_fill_list_with_recursed_entries(XEntry *entry,GSList **p_file_list,gchar *current_path)
{
	gchar *full_path, *_full_path = NULL;
	gint idx,current_page;

	if (entry == NULL)
		return;

	current_page = gtk_notebook_get_current_page (notebook);
	idx = xa_find_archive_index (current_page);

	/* Recurse to siblings with the same path */
 	if (entry->prev->is_dir)
 		xa_fill_list_with_recursed_entries(entry->next,p_file_list,current_path);

	if (strlen(current_path) == 0)
		full_path = g_strdup(entry->filename);
	else
		full_path = g_strconcat(current_path,"/",entry->filename,NULL);

	if (entry->child)
	{
		xa_fill_list_with_recursed_entries(entry->child, p_file_list,full_path);
		g_free(full_path);
	}
	else
	{
		if (archive[idx]->location_entry_path != NULL)
		{
			if (entry->is_dir)
				_full_path = g_strconcat(archive[idx]->location_entry_path,full_path,"/",NULL);
			else
				_full_path = g_strconcat(archive[idx]->location_entry_path,full_path,NULL);
			g_free (full_path);
			full_path = _full_path;
		}
		*p_file_list = g_slist_prepend (*p_file_list,full_path);
	}
	return;
}

void xa_entries_to_filelist(XEntry *entry,GSList **p_file_list,gchar *current_path)
{
	gchar *full_path;
	gchar *quoted_path = NULL;

	if (entry == NULL)
		return;

	/* Recurse to siblings with the same path */
	xa_entries_to_filelist(entry->next,p_file_list,current_path);

	if (strlen(current_path) == 0)
		full_path = g_strdup(entry->filename);
	else
		full_path = g_strconcat(current_path,"/",entry->filename,NULL);

	if (entry->child)
	{
		xa_entries_to_filelist(entry->child, p_file_list,full_path);
		g_free(full_path);
	}
	else
	{
		*p_file_list = g_slist_prepend (*p_file_list,quoted_path);
	}
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
			if ((password_flag & ( 1<<0) ) > 0)
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
			if ( basic_header_size == 0 )
				break;
			fseek ( file , 4 , SEEK_CUR);
			fread (&arj_flag,1,1,file);
			if ((arj_flag & ( 1<<0) ) > 0)
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
		xa_handle_selected_rows(NULL,archive[idx]);
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

	gtk_tree_model_get (model,iter,2,&entry2,-1);
	
	if (entry == entry2)
	{
		gtk_tree_model_iter_parent(model,&parent,iter);
		if ( ! gtk_tree_view_row_expanded(GTK_TREE_VIEW(archive_dir_treeview),path))
			gtk_tree_view_expand_to_path(GTK_TREE_VIEW(archive_dir_treeview),path);

		gtk_tree_selection_select_iter(gtk_tree_view_get_selection (GTK_TREE_VIEW (archive_dir_treeview)),iter);
	}
	else
	{
		return FALSE;
		gtk_tree_selection_unselect_iter(gtk_tree_view_get_selection (GTK_TREE_VIEW (archive_dir_treeview)),iter);
	}

	return FALSE;
}

