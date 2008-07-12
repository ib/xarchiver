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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "window.h"
#include "archive.h"
#include "mime.h"
#include "string_utils.h"
#include "interface.h"
#include "support.h"
#include "main.h"
#include "socket.h"

extern GList *ArchiveType;
extern GList *ArchiveSuffix;
extern gboolean unrar;
extern gboolean xdg_open;
extern Prefs_dialog_data *prefs_window;
extern gchar *config_file;
extern void xa_free_icon_cache();

gchar *current_open_directory = NULL;
GtkFileFilter *open_file_filter = NULL;
GList *Suffix, *Name;

gboolean xa_check_child_for_error_on_exit(XArchive *archive,gint status)
{
	if (xa_main_window)
	{
		gtk_widget_set_sensitive(Stop_button,FALSE);
		gtk_widget_hide(viewport2);
	}
	if (WIFEXITED (status))
	{
		if (WEXITSTATUS (status))
		{
			if (WEXITSTATUS (status) == 1 && archive->type == XARCHIVETYPE_ZIP)
				return TRUE;
			if ( ! gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(prefs_window->store_output)))
			{
				response = xa_show_message_dialog(GTK_WINDOW(xa_main_window),GTK_DIALOG_MODAL,GTK_MESSAGE_ERROR,GTK_BUTTONS_OK,_("An error occurred!"),_("Please check the 'Store archiver output' option to see it.") );	
				return FALSE;
			}
			xa_set_button_state (1,1,1,1,archive->can_add,archive->can_extract,0,archive->has_test,archive->has_properties);
			response = xa_show_message_dialog(GTK_WINDOW(xa_main_window),GTK_DIALOG_MODAL,GTK_MESSAGE_QUESTION,GTK_BUTTONS_OK_CANCEL,_("An error occurred while accessing the archive."),_("Do you want to view the command line output?") );
			if (response == GTK_RESPONSE_OK)
				xa_show_cmd_line_output (NULL);
			/* In case the user supplies a wrong password we reset it so he can try again */
			if ( (archive->status == XA_ARCHIVESTATUS_TEST || archive->status == XA_ARCHIVESTATUS_SFX) && archive->passwd != NULL)
			{
				g_free (archive->passwd);
				archive->passwd = NULL;
			}
			archive->status = XA_ARCHIVESTATUS_IDLE;
			return FALSE;
		}
	}
	return TRUE;
}

void xa_archive_operation_finished(XArchive *archive)
{
	if(xa_main_window)
	{
		gtk_widget_set_sensitive(Stop_button,FALSE);
		gtk_widget_hide(viewport2);

		if (archive->has_comment)
			gtk_widget_set_sensitive (comment_menu,TRUE);
		else
			gtk_widget_set_sensitive (comment_menu,FALSE);

		xa_set_button_state (1,1,1,1,archive->can_add,archive->can_extract,archive->has_sfx,archive->has_test,archive->has_properties);

		if (archive->has_comment && archive->status == XA_ARCHIVESTATUS_OPEN && gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(prefs_window->check_show_comment)))
			xa_show_archive_comment (NULL, NULL);

		gtk_widget_grab_focus (GTK_WIDGET(archive->treeview));
	}
	if (archive->status == XA_ARCHIVESTATUS_ADD || archive->status == XA_ARCHIVESTATUS_DELETE)
		xa_reload_archive_content(archive);

	else if (archive->status == XA_ARCHIVESTATUS_SFX && archive->type == XARCHIVETYPE_RAR)
	{
		if(xa_main_window)
			gtk_widget_set_sensitive ( exe_menu, FALSE);
		response = xa_show_message_dialog (GTK_WINDOW (xa_main_window),GTK_DIALOG_MODAL,GTK_MESSAGE_INFO,GTK_BUTTONS_OK,_("The sfx archive was saved as:"),archive->tmp );
	}
	if (archive->status == XA_ARCHIVESTATUS_TEST)
		xa_show_cmd_line_output (NULL);

	archive->status = XA_ARCHIVESTATUS_IDLE;
}

void xa_reload_archive_content(XArchive *archive)
{
	XEntry *entry;
	if (xa_main_window != NULL)
	{
		if (archive->type == XARCHIVETYPE_BZIP2 || archive->type == XARCHIVETYPE_GZIP || archive->type == XARCHIVETYPE_LZMA)
			Update_StatusBar ( _("Operation completed."));
		else
		{
			Update_StatusBar ( _("Please wait while the content of the archive is being updated..."));
			xa_free_entry (archive,archive->root_entry);
			if (archive->column_types != NULL)
				g_free(archive->column_types);
			xa_remove_columns();
			entry = g_new0(XEntry,1);
			entry->filename = "";
			archive->root_entry = entry;
		}

		switch (archive->type)
		{
			case XARCHIVETYPE_RAR:
			xa_open_rar (archive);
			break;

			case XARCHIVETYPE_TAR:
			xa_open_tar (archive);
			break;

			case XARCHIVETYPE_TAR_BZ2:
			xa_open_bzip2_lzma (archive,NULL);
			break;

			case XARCHIVETYPE_TAR_GZ:
			xa_open_gzip (archive,NULL);
			break;

			case XARCHIVETYPE_TAR_LZMA:
			xa_open_bzip2_lzma (archive,NULL);
			break;

			case XARCHIVETYPE_ZIP:
			xa_open_zip (archive);
			break;

			case XARCHIVETYPE_7ZIP:
			xa_open_7zip (archive);
			break;

			case XARCHIVETYPE_ARJ:
			xa_open_arj (archive);
			break;

			case XARCHIVETYPE_LHA:
			xa_open_lha (archive);
			break;

			default:
			break;
		}
	}
	xa_fill_dir_sidebar(archive,TRUE);
}

void xa_watch_child (GPid pid,gint status,gpointer data)
{
	XArchive *archive = data;
	gboolean result;

	if (WIFSIGNALED (status) )
	{
		Update_StatusBar (_("Operation canceled."));
		if (archive->status == XA_ARCHIVESTATUS_EXTRACT)
		{
			gchar *msg = g_strdup_printf(_("Please check \"%s\" since some files could have been already extracted."),archive->extraction_path);

            response = xa_show_message_dialog (GTK_WINDOW (xa_main_window),GTK_DIALOG_MODAL,GTK_MESSAGE_INFO,GTK_BUTTONS_OK,"",msg );
            g_free (msg);
		}
		else if (archive->status == XA_ARCHIVESTATUS_OPEN)
			gtk_widget_set_sensitive (check_menu,FALSE );

		xa_set_button_state (1,1,1,1,archive->can_add,archive->can_extract,archive->has_sfx,archive->has_test,archive->has_properties);
		archive->status = XA_ARCHIVESTATUS_IDLE;
		return;
	}
	result = xa_check_child_for_error_on_exit (archive,status);
	if ( xa_main_window != NULL && archive != NULL)
	{
		if (archive->has_passwd == FALSE && archive->passwd == NULL)
			gtk_widget_set_sensitive (password_entry_menu,FALSE);
		else
			gtk_widget_set_sensitive (password_entry_menu,TRUE);
	}
	xa_archive_operation_finished(archive);
}

void xa_new_archive (GtkMenuItem *menuitem, gpointer user_data)
{
	gint current_page;

	current_page = xa_get_new_archive_idx();
	if (current_page == -1)
		return;

	archive[current_page] = xa_new_archive_dialog (NULL,archive,FALSE);

	if (archive[current_page] == NULL)
		return;

	xa_add_page (archive[current_page]);

	xa_set_button_state (0,0,0,1,1,0,0,0,0);
    archive[current_page]->has_passwd = FALSE;
    gtk_widget_set_sensitive(check_menu,FALSE);
    gtk_widget_set_sensitive(properties,FALSE );
    xa_disable_delete_buttons(FALSE);

  	Update_StatusBar ( _("Choose Add to begin creating the archive."));

    archive[current_page]->passwd = NULL;
    archive[current_page]->dummy_size = 0;
    archive[current_page]->nr_of_files = 0;
    archive[current_page]->nr_of_dirs = 0;
	xa_set_window_title (xa_main_window , archive[current_page]->path );
}

int xa_show_message_dialog (GtkWindow *window,int mode,int type,int button,const gchar *message1,const gchar *message2)
{
	dialog = gtk_message_dialog_new (window, mode, type, button,message1);
	gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_NO);
	gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dialog), message2);
	response = gtk_dialog_run (GTK_DIALOG (dialog) );
	gtk_widget_destroy (GTK_WIDGET (dialog) );
	return response;
}

void xa_save_archive (GtkMenuItem *menuitem,gpointer data)
{
	gint current_page;
	gint idx;
	GtkWidget *save = NULL;
	gchar *path = NULL,*command,*filename;
	gboolean response;
	GSList *list = NULL;

	current_page = gtk_notebook_get_current_page(notebook);
	idx = xa_find_archive_index (current_page);

	save = gtk_file_chooser_dialog_new ( _("Save the archive as"),
						GTK_WINDOW (xa_main_window),
						GTK_FILE_CHOOSER_ACTION_SAVE,
						GTK_STOCK_CANCEL,
						GTK_RESPONSE_CANCEL,
						"gtk-save",
						GTK_RESPONSE_ACCEPT,
						NULL);
	filename = xa_remove_path_from_archive_name(archive[idx]->escaped_path);
	gtk_file_chooser_set_current_name (GTK_FILE_CHOOSER (save), filename);
	g_free(filename);
	response = gtk_dialog_run (GTK_DIALOG(save));
	if (response == GTK_RESPONSE_ACCEPT)
		path = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER(save));
	gtk_widget_destroy (save);
	if (path != NULL)
	{
		command = g_strconcat ("cp ",archive[idx]->escaped_path," ",path,NULL);
		list = g_slist_append(list,command);
		xa_run_command(archive[idx],list);
	}
}

void xa_open_archive (GtkMenuItem *menuitem,gpointer data)
{
	gchar *path = NULL;
	gint current_page;
	gint x;
	XArchiveType type;

	path = (gchar *)data;
	if ( path == NULL)
    {
		path = xa_open_file_dialog ();
		if (path == NULL)
			return;
	}

	/* Let's check if the archive is already opened */
	for (x = 0; x < gtk_notebook_get_n_pages ( notebook) ; x++)
	{
		current_page = xa_find_archive_index ( x );
		if (current_page == -1)
			break;
		if (strcmp (path,archive[current_page]->path) == 0)
		{
			g_free (path);
			gtk_notebook_set_current_page (notebook,current_page);
			return;
		}
	}
	type = xa_detect_archive_type (path);

	if (type == -1)
	{
		gchar *utf8_path,*msg;
		utf8_path = g_filename_to_utf8 (path,-1,NULL,NULL,NULL);
		msg = g_strdup_printf (_("Can't open file \"%s\":"), utf8_path);
		response = xa_show_message_dialog (GTK_WINDOW (xa_main_window),GTK_DIALOG_MODAL,GTK_MESSAGE_ERROR,GTK_BUTTONS_OK,msg,
		_("Archive format is not recognized!"));
		g_free (utf8_path);
		g_free (msg);
		g_free (path);
		return;
	}
	else if (type == -2)
	{
		g_free (path);
		return;
	}
	
	/* Does the user open an archive from the command line whose archiver is not installed? */
	gchar *ext = NULL;
	if (type == XARCHIVETYPE_RAR)
		ext = "rar";
	else if (type == XARCHIVETYPE_7ZIP)
		ext = "7z";
	else if (type == XARCHIVETYPE_ARJ)
		ext = "arj";
	else if (type == XARCHIVETYPE_LHA)
		ext = "lzh";
	if (ext != NULL)
	{
		if (!g_list_find (ArchiveType,ext))
		{
			response = xa_show_message_dialog (GTK_WINDOW (xa_main_window),GTK_DIALOG_MODAL,GTK_MESSAGE_ERROR, GTK_BUTTONS_OK,_("Sorry, this archive format is not supported:"),_("the proper archiver is not installed!") );
			g_free (path);
			return;
		}
	}
	current_page = xa_get_new_archive_idx();
	if (current_page == -1)
	{
		g_free (path);
		return;
	}
	archive[current_page] = xa_init_archive_structure(type);
	if (archive[current_page] == NULL)
	{
		response = xa_show_message_dialog (GTK_WINDOW (xa_main_window),GTK_DIALOG_MODAL,GTK_MESSAGE_ERROR,GTK_BUTTONS_OK,_("Can't allocate memory for the archive structure:"),"Operation aborted!");
		g_free (path);
		return;
	}
	/* Detect archive comment, only ZIP and ARJ are supported */
	if (type == XARCHIVETYPE_ZIP)
		archive[current_page]->has_comment = xa_detect_archive_comment (XARCHIVETYPE_ZIP,path,archive[current_page]);
	else if (type == XARCHIVETYPE_ARJ)
		archive[current_page]->has_comment = xa_detect_archive_comment (XARCHIVETYPE_ARJ,path,archive[current_page]);

	archive[current_page]->type = type;
	archive[current_page]->path = g_strdup (path);
	archive[current_page]->escaped_path = xa_escape_bad_chars (archive[current_page]->path,"$\'`\"\\!?* ()&|@#:;");
	archive[current_page]->status = XA_ARCHIVESTATUS_OPEN;
	xa_add_page (archive[current_page]);

	xa_disable_delete_buttons (FALSE);
	g_free (path);

	gtk_widget_set_sensitive (Stop_button,TRUE);
	gtk_widget_show (viewport2);

	Update_StatusBar (_("Please wait while the content of the archive is being read..."));

	gtk_widget_set_sensitive (check_menu,FALSE);
	gtk_widget_set_sensitive (properties,FALSE);
	xa_set_button_state ( 0,0,0,0,0,0,0,0,0);
	switch (archive[current_page]->type)
	{
		case XARCHIVETYPE_ARJ:
		xa_open_arj (archive[current_page]);
		break;

		case XARCHIVETYPE_DEB:
		xa_open_deb (archive[current_page]);
		break;

		case XARCHIVETYPE_BZIP2:
		xa_open_bzip2_lzma (archive[current_page],NULL);
		break;

		case XARCHIVETYPE_GZIP:
		xa_open_gzip ( archive[current_page],NULL);
		break;

		case XARCHIVETYPE_LZMA:
		xa_open_bzip2_lzma ( archive[current_page],NULL);
		break;

		case XARCHIVETYPE_RAR:
		xa_open_rar (archive[current_page]);
		break;

		case XARCHIVETYPE_RPM:
		xa_open_rpm (archive[current_page]);
		break;

		case XARCHIVETYPE_TAR:
		xa_open_tar (archive[current_page]);
		break;

		case XARCHIVETYPE_ZIP:
		xa_open_zip (archive[current_page]);
		break;

		case XARCHIVETYPE_7ZIP:
		xa_open_7zip (archive[current_page]);
		break;

		case XARCHIVETYPE_LHA:
		xa_open_lha (archive[current_page]);
		break;

		default:
		break;
	}
	archive[current_page]->passwd = NULL;
	xa_fill_dir_sidebar(archive[current_page],TRUE);
}

void xa_test_archive (GtkMenuItem *menuitem, gpointer user_data)
{
	gint current_page;
	gint id;

	current_page = gtk_notebook_get_current_page (notebook);
	id = xa_find_archive_index (current_page);

	if ( archive[id]->has_passwd )
	{
		if ( archive[id]->passwd == NULL)
		{
			archive[id]->passwd = xa_create_password_dialog (NULL);
			if ( archive[id]->passwd == NULL)
				return;
		}
	}
	Update_StatusBar (_("Testing archive integrity, please wait..."));
	(*archive[id]->test) (archive[id]);
}

void xa_close_archive (GtkMenuItem *menuitem, gpointer user_data)
{
	gint current_page;
	gint idx;
	GtkWidget *scrollwindow = user_data;

	current_page = gtk_notebook_page_num(notebook, scrollwindow);
	idx = xa_find_archive_index (current_page);
	gtk_notebook_remove_page ( notebook , current_page);

	current_page = gtk_notebook_get_n_pages(notebook);
	if (current_page == 0)
	{
		gtk_widget_set_sensitive (check_menu,FALSE);
		gtk_widget_set_sensitive (properties,FALSE);
		gtk_widget_set_sensitive (up_button,FALSE);
		gtk_widget_set_sensitive (home_button,FALSE);
		gtk_widget_set_sensitive (deselect_all,FALSE);
		xa_disable_delete_buttons (FALSE);
		xa_set_button_state (1,1,0,0,0,0,0,0,0);
		xa_set_window_title (xa_main_window,NULL);
		gtk_tree_store_clear(GTK_TREE_STORE(archive_dir_model));
		gtk_entry_set_text(GTK_ENTRY(location_entry),"");
	}
	else if ( current_page == 1)
		gtk_notebook_set_show_tabs (notebook,FALSE);
	else
		gtk_notebook_set_show_tabs (notebook,TRUE);

	xa_clean_archive_structure (archive[idx]);
	archive[idx] = NULL;

	Update_StatusBar (_("Ready."));
}

void xa_quit_application (GtkMenuItem *menuitem, gpointer user_data)
{
	gint i;
	gint idx;

	if (GTK_WIDGET_VISIBLE (viewport2))
	{
		Update_StatusBar ( _("Please hit the Stop button first!"));
		return;
	}
	g_list_free ( Suffix );
	g_list_free ( Name );

	for (i = 0; i < gtk_notebook_get_n_pages(notebook) ; i++)
	{
		idx = xa_find_archive_index (i);
		if (archive[idx] != NULL)
		{
			xa_clean_archive_structure (archive[idx]);
			archive[idx] = NULL;
		}
	}

	if (current_open_directory != NULL)
		g_free (current_open_directory);

	xa_prefs_save_options (prefs_window,config_file);
	g_free (config_file);
	xa_free_icon_cache();

#ifdef HAVE_SOCKET
	socket_finalize();
#endif
	gtk_main_quit();
}

void xa_delete_archive (GtkMenuItem *menuitem, gpointer user_data)
{
	GList  *row_list = NULL;
	XEntry *entry = NULL;
	GtkTreeIter iter;
	GSList *list = NULL;
	gint current_page,id;

	current_page = gtk_notebook_get_current_page (notebook);
	id = xa_find_archive_index (current_page);
	GtkTreeSelection *selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (archive[id]->treeview));

	row_list = gtk_tree_selection_get_selected_rows(selection, &archive[id]->model);
	if (row_list != NULL)
	{
		while (row_list)
		{
			gtk_tree_model_get_iter(archive[id]->model, &iter,row_list->data);
			gtk_tree_model_get (archive[id]->model,&iter,archive[id]->nc+1,&entry,-1);
			gtk_tree_path_free (row_list->data);
			if (entry->is_dir)
			{
				if (archive[id]->type == XARCHIVETYPE_TAR || is_tar_compressed(archive[id]->type))
					goto one_file;
				else
					xa_fill_list_with_recursed_entries(entry, &list,"");
			}
			else
			{
				one_file:
				list = g_slist_prepend (list,xa_build_full_path_name_from_entry(entry));
			}
			row_list = row_list->next;
		}
		g_list_free (row_list);
	}

	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(prefs_window->confirm_deletion)))
	{
		response = xa_show_message_dialog (GTK_WINDOW (xa_main_window),GTK_DIALOG_MODAL,GTK_MESSAGE_QUESTION,GTK_BUTTONS_OK_CANCEL,"You are about to delete entries from the archive.",_( "Are you sure you want to do this?") );
		if (response == GTK_RESPONSE_CANCEL || response == GTK_RESPONSE_DELETE_EVENT)
			return;
	}

	Update_StatusBar ( _("Deleting files from the archive, please wait..."));
	archive[id]->status = XA_ARCHIVESTATUS_DELETE;
	(*archive[id]->delete) (archive[id],list);
}

void xa_add_files_archive (GtkMenuItem *menuitem,gpointer data)
{
	gint current_page;
	gint idx;

	current_page = gtk_notebook_get_current_page (notebook);
	idx = xa_find_archive_index (current_page);

	add_window = xa_create_add_dialog (archive[idx]);
	xa_parse_add_dialog_options (archive[idx],add_window);
	gtk_widget_destroy (add_window->dialog1);
	add_window->dialog1 = NULL;
	g_free (add_window);
	add_window = NULL;
}

void xa_extract_archive (GtkMenuItem *menuitem,gpointer user_data)
{
	gint current_page;
	gint idx;

	current_page = gtk_notebook_get_current_page(notebook);
	idx = xa_find_archive_index (current_page);

	GtkTreeSelection *selection = gtk_tree_view_get_selection (GTK_TREE_VIEW(archive[idx]->treeview));
	gint selected = gtk_tree_selection_count_selected_rows (selection);

    extract_window = xa_create_extract_dialog (selected,archive[idx]);
	if (archive[idx]->extraction_path != NULL)
		gtk_entry_set_text (GTK_ENTRY(extract_window->destination_path_entry),archive[idx]->extraction_path);

    xa_parse_extract_dialog_options(archive[idx],extract_window,selection);
	gtk_widget_destroy (extract_window->dialog1);
	extract_window->dialog1 = NULL;
	g_free (extract_window);
	extract_window = NULL;
}

void xa_show_prefs_dialog (GtkMenuItem *menuitem,gpointer user_data)
{
	gboolean response;

	if (prefs_window == NULL)
		prefs_window = xa_create_prefs_dialog();

	gtk_widget_show_all (prefs_window->dialog1);
	response = gtk_dialog_run (GTK_DIALOG(prefs_window->dialog1));
	gtk_widget_hide (prefs_window->dialog1);

	if (response == GTK_RESPONSE_OK)
		xa_apply_prefs_option(prefs_window);
}

void xa_convert_sfx (GtkMenuItem *menuitem , gpointer user_data)
{
	gchar *command = NULL;
	GSList *list = NULL;
	gboolean result;
	unsigned short int l = 0;
	gint current_page;
	gint idx;

	current_page = gtk_notebook_get_current_page (notebook);
	idx = xa_find_archive_index ( current_page );

	Update_StatusBar ( _("Converting archive to self-extracting, please wait..."));
    gtk_widget_set_sensitive (Stop_button,TRUE);
    archive[idx]->status = XA_ARCHIVESTATUS_SFX;
    switch ( archive[idx]->type )
	{
		case XARCHIVETYPE_RAR:
		{
			command = g_strconcat ("rar s -o+ " , archive[idx]->escaped_path , NULL);
			list = g_slist_append(list,command);
			if (strstr(archive[idx]->escaped_path,".rar") )
			{
				archive[idx]->tmp = g_strdup (archive[idx]->escaped_path);
				archive[idx]->tmp[strlen(archive[idx]->tmp) - 3] = 's';
				archive[idx]->tmp[strlen(archive[idx]->tmp) - 2] = 'f';
				archive[idx]->tmp[strlen(archive[idx]->tmp) - 1] = 'x';
			}
			else
			{
				archive[idx]->tmp = (gchar *) g_malloc ( strlen(archive[idx]->escaped_path) + 4 );
				l = strlen (archive[idx]->escaped_path);
				strncpy ( archive[idx]->tmp, archive[idx]->escaped_path , l);
				archive[idx]->tmp[l] 	= '.';
				archive[idx]->tmp[l + 1] = 's';
				archive[idx]->tmp[l + 2] = 'f';
				archive[idx]->tmp[l + 3] = 'x';
				archive[idx]->tmp[l + 4] = 0;
			}
			xa_run_command(archive[idx],list);
		}
		break;

        case XARCHIVETYPE_ZIP:
        {
        	gchar *archive_name = NULL;
        	gchar *archive_name_escaped = NULL;
			FILE *sfx_archive;
			FILE *archive_not_sfx;
			gchar *content;
            gsize length;
            GError *error = NULL;
			gchar *unzipsfx_path = NULL;
			gchar buffer[1024];

			archive_name = xa_open_sfx_file_selector ();

			if (archive_name == NULL)
			{
				gtk_widget_set_sensitive (Stop_button,FALSE);
				Update_StatusBar ( _("Operation canceled."));
				return;
			}
			archive_name_escaped = xa_escape_bad_chars ( archive_name ,"$\'`\"\\!?* ()[]&|@#:;" );
			unzipsfx_path = g_find_program_in_path ( "unzipsfx" );
			if ( unzipsfx_path != NULL )
			{
				/* Load the unzipsfx executable in memory, about 50 KB */
				result = g_file_get_contents (unzipsfx_path,&content,&length,&error);
				if ( ! result)
				{
					gtk_widget_set_sensitive (Stop_button,FALSE);
					response = xa_show_message_dialog (GTK_WINDOW (xa_main_window),GTK_DIALOG_MODAL,GTK_MESSAGE_ERROR,GTK_BUTTONS_OK,_("Can't convert the archive to self-extracting:"),error->message);
					g_error_free (error);
					g_free (unzipsfx_path);
					return;
				}
				g_free (unzipsfx_path);

				/* Write unzipsfx to a new file */
				sfx_archive = g_fopen ( archive_name ,"w" );
				if (sfx_archive == NULL)
				{
					gtk_widget_set_sensitive (Stop_button,FALSE);
					response = xa_show_message_dialog (GTK_WINDOW (xa_main_window),GTK_DIALOG_MODAL,GTK_MESSAGE_ERROR,GTK_BUTTONS_OK,_("Can't write the unzipsfx module to the archive:"),g_strerror(errno) );
					return;
				}
				archive_not_sfx = g_fopen ( archive[idx]->path ,"r" );
				fwrite (content, 1, length, sfx_archive);
				g_free (content);

				/* Read archive data and write it after the sfx module in the new file */
				while ( ! feof(archive_not_sfx) )
				{
					fread (&buffer, 1, 1024, archive_not_sfx);
					fwrite (&buffer, 1, 1024, sfx_archive);
				}
				fclose (archive_not_sfx);
				fclose (sfx_archive);

				command = g_strconcat ("chmod 755 ", archive_name_escaped , NULL);
				list = g_slist_append(list,command);
				command = g_strconcat ("zip -A ",archive_name_escaped,NULL);
				list = g_slist_append(list,command);
				xa_run_command (archive[idx],list);
			}
			g_free (archive_name);
			g_free (archive_name_escaped);
        }
        break;

        case XARCHIVETYPE_7ZIP:
        {
        	gchar *archive_name = NULL;
        	gchar *archive_name_escaped = NULL;
			FILE *sfx_archive;
			FILE *archive_not_sfx;
			gchar *content;
            gsize length;
            GError *error = NULL;
			gchar *sfx_path = NULL;
			gchar buffer[1024];
			gboolean response;
			GtkWidget *locate_7zcon = NULL;
			GtkFileFilter *sfx_filter;

        	archive_name = xa_open_sfx_file_selector ();

			if (archive_name == NULL)
			{
				gtk_widget_set_sensitive (Stop_button,FALSE);
				Update_StatusBar ( _("Operation canceled."));
				return;
			}
			archive_name_escaped = xa_escape_bad_chars ( archive_name ,"$\'`\"\\!?* ()[]&|@#:;" );

			if (g_file_test ( "/usr/lib/p7zip/7zCon.sfx" , G_FILE_TEST_EXISTS) )
				sfx_path = g_strdup("/usr/lib/p7zip/7zCon.sfx");
			else if (g_file_test ( "/usr/local/lib/p7zip/7zCon.sfx" , G_FILE_TEST_EXISTS) )
				sfx_path = g_strdup ("/usr/local/lib/p7zip/7zCon.sfx");
			else if (g_file_test ( "/usr/libexec/p7zip/7zCon.sfx" , G_FILE_TEST_EXISTS) )
				sfx_path = g_strdup ("/usr/libexec/p7zip/7zCon.sfx");
			else
			{
				sfx_filter = gtk_file_filter_new ();
				gtk_file_filter_set_name (sfx_filter, "" );
				gtk_file_filter_add_pattern (sfx_filter, "*.sfx" );

				locate_7zcon = gtk_file_chooser_dialog_new ( _("Please select the 7zCon.sfx module"),
						GTK_WINDOW (xa_main_window),
						GTK_FILE_CHOOSER_ACTION_OPEN,
						GTK_STOCK_CANCEL,
						GTK_RESPONSE_CANCEL,
						"gtk-open",
						GTK_RESPONSE_ACCEPT,
						NULL);

				gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (locate_7zcon), sfx_filter);
				gtk_dialog_set_default_response (GTK_DIALOG (locate_7zcon), GTK_RESPONSE_ACCEPT);
				response = gtk_dialog_run (GTK_DIALOG(locate_7zcon) );
				if (response == GTK_RESPONSE_ACCEPT)
				{
					sfx_path = gtk_file_chooser_get_filename ( GTK_FILE_CHOOSER (locate_7zcon) );
					gtk_widget_destroy ( locate_7zcon );
				}
				else
				{
					gtk_widget_destroy ( locate_7zcon );
					Update_StatusBar (_("Operation canceled."));
					return;
				}
			}
			if ( sfx_path != NULL )
			{
				/* Load the 7zCon.sfx executable in memory ~ 500 KB; is it too much for 128 MB equipped PCs ? */
				result = g_file_get_contents (sfx_path,&content,&length,&error);
				if ( ! result)
				{
					response = xa_show_message_dialog (GTK_WINDOW (xa_main_window),GTK_DIALOG_MODAL,GTK_MESSAGE_ERROR,GTK_BUTTONS_OK,_("Can't convert the archive to self-extracting:"),error->message);
					g_error_free (error);
					g_free (sfx_path);
					return;
				}
				g_free (sfx_path);

				/* Write 7zCon.sfx to a new file */
				sfx_archive = g_fopen ( archive_name ,"w" );
				if (sfx_archive == NULL)
				{
					response = xa_show_message_dialog (GTK_WINDOW (xa_main_window),GTK_DIALOG_MODAL,GTK_MESSAGE_ERROR,GTK_BUTTONS_OK,_("Can't write the unzipsfx module to the archive:"),g_strerror(errno) );
					return;
				}
				archive_not_sfx = g_fopen ( archive[idx]->path ,"r" );
				fwrite (content, 1, length, sfx_archive);
				g_free (content);

				/* Read archive data and write it after the sfx module in the new file */
				while ( ! feof(archive_not_sfx))
				{
					fread (&buffer,1,1024,archive_not_sfx);
					fwrite (&buffer,1,1024,sfx_archive);
				}
				fclose (archive_not_sfx);
				fclose (sfx_archive);

				command = g_strconcat ("chmod 755 ",archive_name_escaped,NULL);
				list = g_slist_append(list,command);
				xa_run_command (archive[idx],list);
			}
			g_free (archive_name);
			g_free (archive_name_escaped);
        }
		break;

		case XARCHIVETYPE_ARJ:
        command = g_strconcat ("arj y -je1 " , archive[idx]->escaped_path, NULL);
        list = g_slist_append(list,command);
        xa_run_command (archive[idx],list);
		break;

		default:
		command = NULL;
	}
}

void xa_about (GtkMenuItem *menuitem, gpointer user_data)
{
    static GtkWidget *about = NULL;
    const char *authors[] = {"\nMain developer:\nGiuseppe Torelli <colossus73@gmail.com>\n\nArchive navigation code:\nJohn Berthels\n\nCode fixing:\nEnrico Tröger\n\nLHA and DEB support:\nŁukasz Zemczak <sil2100@vexillium.org>\n\nLZMA support:\nThomas Dy <dysprosium66@gmail.com>\n",NULL};
    const char *documenters[] = {"\nSpecial thanks to Bjoern Martensen for\nbugs hunting and Xarchiver Tango logo.\n\nThanks to:\nBenedikt Meurer\nStephan Arts\nBruno Jesus <00cpxxx@gmail.com>\nUracile for the stunning logo\n", NULL};

	if (about == NULL)
	{
		about = gtk_about_dialog_new ();
		gtk_about_dialog_set_email_hook (xa_activate_link, NULL, NULL);
		gtk_about_dialog_set_url_hook (xa_activate_link, NULL, NULL);
		gtk_window_set_destroy_with_parent (GTK_WINDOW (about) , TRUE);
		g_object_set (about,
			"name",  "xarchiver",
			"version", PACKAGE_VERSION,
			"copyright", "Copyright \xC2\xA9 2005-2008 Giuseppe Torelli",
			"comments", "The best GUI for handling archives on Linux",
			"authors", authors,
			"documenters",documenters,
			"translator_credits", _("translator-credits"),
			"logo_icon_name", "xarchiver",
			"website", "http://xarchiver.xfce.org",
			"license",    "Copyright \xC2\xA9 2005-2007 Giuseppe Torelli - Colossus <colossus73@gmail.com>\n\n"
		    			"This is free software; you can redistribute it and/or\n"
    					"modify it under the terms of the GNU Library General Public License as\n"
    					"published by the Free Software Foundation; either version 2 of the\n"
    					"License, or (at your option) any later version.\n"
    					"\n"
    					"This software is distributed in the hope that it will be useful,\n"
    					"but WITHOUT ANY WARRANTY; without even the implied warranty of\n"
    					"MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU\n"
    					"Library General Public License for more details.\n"
    					"\n"
    					"You should have received a copy of the GNU Library General Public\n"
    					"License along with the Gnome Library; see the file COPYING.LIB.  If not,\n"
    					"write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,\n"
    					"Boston, MA 02111-1307, USA.\n",
		      NULL);
		gtk_window_set_position (GTK_WINDOW (about), GTK_WIN_POS_CENTER);
	}
	gtk_dialog_run ( GTK_DIALOG(about) );
	gtk_widget_hide (about);
}

gchar *xa_open_sfx_file_selector ()
{
	gchar *sfx_name = NULL;
	GtkWidget *sfx_file_selector = NULL;
	gboolean response;

	sfx_file_selector = gtk_file_chooser_dialog_new ( _("Save the self-extracting archive as"),
						GTK_WINDOW (xa_main_window),
						GTK_FILE_CHOOSER_ACTION_SAVE,
						GTK_STOCK_CANCEL,
						GTK_RESPONSE_CANCEL,
						"gtk-save",
						GTK_RESPONSE_ACCEPT,
						NULL);

	gtk_dialog_set_default_response (GTK_DIALOG (sfx_file_selector), GTK_RESPONSE_ACCEPT);
	response = gtk_dialog_run ( GTK_DIALOG(sfx_file_selector) );

	if (response == GTK_RESPONSE_ACCEPT)
		sfx_name = gtk_file_chooser_get_filename ( GTK_FILE_CHOOSER (sfx_file_selector) );

	gtk_widget_destroy (sfx_file_selector);
	return sfx_name;
}

gchar *xa_open_file_dialog ()
{
	static GtkWidget *File_Selector = NULL;
	GtkFileFilter *filter;
	gchar *path = NULL;

	if (File_Selector == NULL)
	{
		File_Selector = gtk_file_chooser_dialog_new ( _("Open an archive"),
						GTK_WINDOW (xa_main_window),
						GTK_FILE_CHOOSER_ACTION_OPEN,
						GTK_STOCK_CANCEL,
						GTK_RESPONSE_CANCEL,
						"gtk-open",
						GTK_RESPONSE_ACCEPT,
						NULL);

		gtk_dialog_set_default_response (GTK_DIALOG (File_Selector), GTK_RESPONSE_ACCEPT);
		gtk_window_set_destroy_with_parent (GTK_WINDOW (File_Selector) , TRUE);

		filter = gtk_file_filter_new ();
		gtk_file_filter_set_name ( filter , _("All files") );
		gtk_file_filter_add_pattern ( filter, "*" );
		gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (File_Selector), filter);

		filter = gtk_file_filter_new ();
		gtk_file_filter_set_name ( filter , _("Only archives") );
		Suffix = g_list_first ( ArchiveSuffix );
		while ( Suffix != NULL )
		{
			gtk_file_filter_add_pattern (filter, Suffix->data);
			Suffix = g_list_next ( Suffix );
		}
		gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (File_Selector), filter);

		Suffix = g_list_first ( ArchiveSuffix );
		while ( Suffix != NULL )
		{
			if ( strcmp(Suffix->data, "") != 0 )	/* To avoid double filtering when opening the archive */
			{
				filter = gtk_file_filter_new ();
				gtk_file_filter_set_name (filter, Suffix->data );
				gtk_file_filter_add_pattern (filter, Suffix->data );
				gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (File_Selector), filter);
			}
			Suffix = g_list_next ( Suffix );
		}
		gtk_window_set_modal (GTK_WINDOW (File_Selector),TRUE);
	}
	if (open_file_filter != NULL)
		gtk_file_chooser_set_filter (GTK_FILE_CHOOSER (File_Selector) , open_file_filter );

	if (current_open_directory != NULL)
		gtk_file_chooser_set_current_folder ( GTK_FILE_CHOOSER (File_Selector) , current_open_directory );

	response = gtk_dialog_run (GTK_DIALOG (File_Selector));

	if (current_open_directory != NULL)
		g_free (current_open_directory);

	current_open_directory = gtk_file_chooser_get_current_folder ( GTK_FILE_CHOOSER (File_Selector) );
	open_file_filter = gtk_file_chooser_get_filter ( GTK_FILE_CHOOSER (File_Selector) );

	if (response == GTK_RESPONSE_ACCEPT)
		path = gtk_file_chooser_get_filename ( GTK_FILE_CHOOSER (File_Selector) );
	else if ( (response == GTK_RESPONSE_CANCEL) || (response == GTK_RESPONSE_DELETE_EVENT) )
		path = NULL;

	/* Hiding the window instead of destroying it will preserve the pointers to the file chooser stuff */
	gtk_widget_hide (File_Selector);
	return path;
}

int xa_detect_archive_type (gchar *filename)
{
	FILE *dummy_ptr = NULL;
    int xx = -1;
	unsigned char magic[14]={0,0,0,0,0,0,0,0,0,0,0,0,0,0}; /* avoid problems with garbage */

	if (filename != NULL)
		dummy_ptr = fopen (filename,"r");

	if (dummy_ptr == NULL)
	{
		if (xa_main_window != NULL)
		{
			gchar *utf8_path,*msg;
			utf8_path = g_filename_to_utf8 (filename, -1, NULL, NULL, NULL);
			msg = g_strdup_printf (_("Can't open archive \"%s\":") , utf8_path );
			response = xa_show_message_dialog (GTK_WINDOW (xa_main_window) , GTK_DIALOG_MODAL,GTK_MESSAGE_ERROR,GTK_BUTTONS_OK,
			msg,g_strerror (errno));
			g_free (msg);
			g_free (utf8_path);
			return -2;
		}
		else
			return -2;
	}
	fread (magic, 1, 14, dummy_ptr);
	if (memcmp (magic,"\x50\x4b",2) == 0)
		xx = XARCHIVETYPE_ZIP;
	else if (memcmp (magic,"\x60\xea",2 ) == 0)
		xx = XARCHIVETYPE_ARJ;
	else if (memcmp ( magic,"\x52\x61\x72\x21",4 ) == 0)
		xx = XARCHIVETYPE_RAR;
	else if (memcmp ( magic,"\x42\x5a\x68",3 ) == 0)
		xx = XARCHIVETYPE_BZIP2;
	else if (memcmp ( magic,"\x1f\x8b",2) == 0 || memcmp ( magic,"\x1f\x9d",2 ) == 0)
		xx = XARCHIVETYPE_GZIP;
	else if (memcmp ( magic,"\x5d\x00\x00\x80",4 ) == 0)
		xx = XARCHIVETYPE_LZMA;
	else if (memcmp ( magic,"\xed\xab\xee\xdb",4 ) == 0)
		xx = XARCHIVETYPE_RPM;
	else if (memcmp ( magic,"\x37\x7a\xbc\xaf\x27\x1c",6) == 0)
		xx = XARCHIVETYPE_7ZIP;
	else if (isTar ( dummy_ptr))
		xx = XARCHIVETYPE_TAR;
	else if (isLha ( dummy_ptr))
		xx = XARCHIVETYPE_LHA;
	else if (memcmp ( magic,"!<arch>\ndebian",14) == 0)
		xx = XARCHIVETYPE_DEB;
	fclose (dummy_ptr);
	return xx;
}

gboolean xa_detect_archive_comment (int type, gchar *filename, XArchive *archive)
{
	FILE *stream;
	char sig;
	guint cmt_len = 0;
	int byte;
	unsigned char eocds[] = { 0x50, 0x4b, 0x05, 0x06 };
	unsigned long long int eocds_position = 0;

	unsigned short int len = 0;
	int eof;
	size_t seqptr = 0;

	stream = fopen (filename,"r");
	if (stream == NULL)
		return FALSE;

	if (type == XARCHIVETYPE_ZIP)
	{
		/* Let's position the file indicator to 64KB before the end of the archive */
		fseek(stream, 0L, SEEK_SET);
		/* Let's reach the end of central directory signature now */
		while( ! feof(stream) )
		{
			byte = (eof = fgetc(stream));
			if (eof == EOF)
				break;
			if (byte == eocds[seqptr])
			{
				if (++seqptr == sizeof(eocds))
				{
					eocds_position = ftell(stream) + 16 ;
					seqptr = 0;
				}
				continue;
			}
			else
			{
				if (seqptr)
					seqptr = 0;
			}
		}
		fseek (stream,eocds_position,SEEK_SET);
		fread (&len,1,2,stream);
		if (len == 0)
			return FALSE;
		else
		{
			archive->comment = g_string_new("");
			while (cmt_len != len)
			{
				fread (&sig,1,1,stream);
				g_string_append_c (archive->comment,sig);
				cmt_len++;
			}
			return TRUE;
		}
	}
	else if (type == XARCHIVETYPE_ARJ)
	{
		/* Let's avoid the archive name */
		fseek ( stream, 39 , SEEK_SET );
		while (sig != 0)
		{
			fread (&sig,1,1,stream);
			cmt_len++;
		}
		fseek ( stream, 39 + cmt_len , SEEK_SET );
		sig = 1;
		/* Let's read the archive comment byte after byte now */
		archive->comment = g_string_new("");
		while (sig != 0)
		{
			fread (&sig,1,1,stream);

			if (sig == 0 && archive->comment->len == 0)
			{
				g_string_free (archive->comment,FALSE);
				archive->comment = NULL;
				return FALSE;
			}
			else
				g_string_append (archive->comment,&sig);
		}
		return TRUE;
	}
	return FALSE;
}

void xa_remove_columns()
{
	gint current_page;
	gint idx;

	current_page = gtk_notebook_get_current_page (notebook);
	idx = xa_find_archive_index (current_page);

	GList *columns = gtk_tree_view_get_columns ( GTK_TREE_VIEW (archive[idx]->treeview) );
	while (columns != NULL)
	{
		gtk_tree_view_remove_column (GTK_TREE_VIEW (archive[idx]->treeview) , columns->data);
		columns = columns->next;
	}
	g_list_free (columns);
}

void xa_create_liststore (XArchive *archive, gchar *columns_names[])
{
	unsigned short int x;
	GtkCellRenderer *renderer;
	GtkTreeViewColumn *column;

	archive->liststore = gtk_list_store_newv ( archive->nc+2 , archive->column_types);
	gtk_tree_view_set_model ( GTK_TREE_VIEW (archive->treeview), GTK_TREE_MODEL (archive->liststore) );

	archive->model = gtk_tree_view_get_model(GTK_TREE_VIEW(archive->treeview));
	//gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(archive->model),1,GTK_SORT_ASCENDING);
	g_object_ref(archive->model);
	gtk_tree_view_set_model(GTK_TREE_VIEW(archive->treeview), NULL);

	/* First column: icon + text */
	column = gtk_tree_view_column_new();
	archive->renderer = gtk_cell_renderer_pixbuf_new();
	g_object_set(G_OBJECT(archive->renderer), "stock-size", (3 - gtk_combo_box_get_active(GTK_COMBO_BOX(prefs_window->combo_icon_size))), NULL);
	gtk_tree_view_column_pack_start(column, archive->renderer, FALSE);
	gtk_tree_view_column_set_attributes(column, archive->renderer, "pixbuf",0,NULL);

	renderer = gtk_cell_renderer_text_new ();
	gtk_tree_view_column_pack_start(column, renderer, TRUE);
	gtk_tree_view_column_set_attributes( column,renderer,"text",1,NULL);
	gtk_tree_view_column_set_title(column, _("Filename"));
	gtk_tree_view_column_set_resizable (column, TRUE);
	gtk_tree_view_column_set_sort_column_id (column, 1);
	gtk_tree_view_append_column (GTK_TREE_VIEW (archive->treeview), column);
	gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);

	/* All the others */
	for (x = 0; x < archive->nc; x++)
	{
		if (x+1 == archive->nc)
		{
			column = gtk_tree_view_column_new();
			gtk_tree_view_column_set_visible(column,FALSE);
		}
		else
		{
			renderer = gtk_cell_renderer_text_new();
			column = gtk_tree_view_column_new_with_attributes ( columns_names[x],renderer,"text",x+2,NULL);
			gtk_tree_view_column_set_resizable (column, TRUE);
			gtk_tree_view_column_set_sort_column_id (column, x+2);
		}
		gtk_tree_view_append_column (GTK_TREE_VIEW (archive->treeview), column);
	}
}

gboolean treeview_select_search (GtkTreeModel *model,gint column,const gchar *key,GtkTreeIter *iter,gpointer search_data)
{
    char *filename;
    gboolean result;

    gtk_tree_model_get (model, iter, 1, &filename, -1);
    if (strcasestr (filename, key))
    	result = FALSE;
	else
		result = TRUE;
    g_free (filename);
    return result;
}

void xa_show_cmd_line_output (GtkMenuItem *menuitem)
{
	GSList *output = NULL;
	widget_data *xa_cmd_line_output = NULL;
	gchar *line = NULL;
	gchar *utf8_line;
	gsize bytes_written;
	gint current_page;
	gint idx;

	current_page = gtk_notebook_get_current_page (notebook);
	idx = xa_find_archive_index (current_page);
	
	if (archive[idx] == NULL)
		return;
	xa_cmd_line_output = xa_create_output_window(_("Command line output"));

	if ( ! archive[idx]->list_reversed)
	{
		archive[idx]->error_output = g_slist_reverse(archive[idx]->error_output);
		archive[idx]->list_reversed = TRUE;
	}

	output = archive[idx]->error_output;
	while (output)
	{
		line = output->data;
		utf8_line = g_locale_to_utf8 (line, -1, NULL, &bytes_written, NULL);
		gtk_text_buffer_insert_with_tags_by_name (xa_cmd_line_output->textbuffer, &xa_cmd_line_output->iter, utf8_line, bytes_written, "font", NULL);
		g_free (utf8_line);
		output = output->next;
	}
	gtk_dialog_run (GTK_DIALOG(xa_cmd_line_output->dialog1));
	gtk_widget_destroy (xa_cmd_line_output->dialog1);
	g_free(xa_cmd_line_output);
}

void xa_cancel_archive (GtkMenuItem *menuitem,gpointer data)
{
	gint current_page;
	gint idx;

	current_page = gtk_notebook_get_current_page(notebook);
	idx = xa_find_archive_index (current_page);

	if (archive[idx]->status == XA_ARCHIVESTATUS_ADD || archive[idx]->status == XA_ARCHIVESTATUS_SFX)
	{
		response = xa_show_message_dialog (GTK_WINDOW(xa_main_window),GTK_DIALOG_MODAL,GTK_MESSAGE_QUESTION,GTK_BUTTONS_OK_CANCEL,_("Doing so will probably corrupt your archive!"),_("Do you really want to cancel?") );
		if (response == GTK_RESPONSE_CANCEL)
			return;
	}
	Update_StatusBar (_("Operation aborted."));
	if (archive[idx]->child_pid)
	{
		kill (archive[idx]->child_pid,SIGABRT);
		archive[idx]->child_pid = 0;
	}
	/* This in case the user cancels the opening of a password protected archive */
	if (archive[idx]->status != XA_ARCHIVESTATUS_ADD)
		if (archive[idx]->has_passwd)
			archive[idx]->has_passwd = FALSE;
	xa_archive_operation_finished(archive[idx]);
}

void xa_archive_properties (GtkMenuItem *menuitem,gpointer user_data)
{
	struct stat my_stat;
    gchar *utf8_string , *measure,*dummy_string;
    char date[64];
    gchar *t;
    unsigned long long int file_size;
    gint current_page;
	gint idx;

    current_page = gtk_notebook_get_current_page (notebook);
	idx = xa_find_archive_index (current_page);
    stat (archive[idx]->path , &my_stat );
    file_size = my_stat.st_size;
    archive_properties_window = create_archive_properties_window();
    utf8_string = xa_remove_path_from_archive_name(archive[idx]->escaped_path);
	gtk_entry_set_text ( GTK_ENTRY (name_data), utf8_string );
	g_free (utf8_string);
    //Path
    dummy_string = remove_level_from_path (archive[idx]->path);
    if (strlen(dummy_string) == 0 || strcmp(dummy_string,"..") == 0)
		utf8_string = g_filename_display_name (g_get_current_dir ());
    else
		utf8_string = g_filename_display_name (dummy_string);
    g_free ( dummy_string );

    gtk_entry_set_text ( GTK_ENTRY (path_data), utf8_string );
    g_free ( utf8_string );
	//Type
	gtk_entry_set_text ( GTK_ENTRY (type_data), archive[idx]->format );
    //Modified Date
    strftime (date, 64, "%c", localtime (&my_stat.st_mtime) );
    t = g_locale_to_utf8 ( date, -1, 0, 0, 0);
    gtk_entry_set_text ( GTK_ENTRY (modified_data), t);
    g_free (t);
    //Archive Size
	if (file_size > 1024*1024*1024 )
	{
		content_size = (double)file_size / (1024*1024*1024);
		measure = " GB";
	}
	else if (file_size > 1024*1024 )
	{
		content_size = (double)file_size / (1024*1024);
		measure = " MB";
	}

    else if (file_size > 1024 )
	{
		content_size = (double)file_size / 1024;
		measure = " KB";
	}
	else
	{
		measure = " Bytes";
		content_size = file_size;
	}

    t = g_strdup_printf ("%.1f %s", content_size,measure);
    gtk_entry_set_text ( GTK_ENTRY (size_data), t );
    g_free (t);
    //content_size
    if (archive[idx]->dummy_size > 1024*1024*1024 )
    {
        content_size = (double)archive[idx]->dummy_size / (1024*1024*1024);
        measure = " GB";
    }
        else if (archive[idx]->dummy_size > 1024*1024 )
        {
            content_size = (double)archive[idx]->dummy_size / (1024*1024);
            measure = " MB";
        }

        else if (archive[idx]->dummy_size > 1024 )
        {
            content_size = (double)archive[idx]->dummy_size / 1024;
            measure = " KB";
        }
        else
        {
            measure = " Bytes";
            content_size = archive[idx]->dummy_size;
        }
    t = g_strdup_printf ( "%.1f %s", content_size,measure);
    gtk_entry_set_text ( GTK_ENTRY (content_data), t );
    g_free (t);
    //Has Comment
    if (archive[idx]->has_comment)
		gtk_entry_set_text ( GTK_ENTRY (comment_data), _("Yes") );
	else
		gtk_entry_set_text ( GTK_ENTRY (comment_data), _("No") );

    //Compression_ratio
    if (content_size != 0)
		content_size = (double)archive[idx]->dummy_size / file_size;
    else
		content_size = 0.0;
    t = g_strdup_printf ( "%.2f", content_size);
    gtk_entry_set_text ( GTK_ENTRY (compression_data), t );
    g_free (t);
    //Number of files
    t = g_strdup_printf ( "%d", archive[idx]->nr_of_files);
    gtk_entry_set_text ( GTK_ENTRY (number_of_files_data), t );
    g_free (t);
    //Number of dirs
    t = g_strdup_printf ( "%d", archive[idx]->nr_of_dirs);
    gtk_entry_set_text ( GTK_ENTRY (number_of_dirs_data), t );
    g_free (t);
    gtk_widget_show_all (archive_properties_window);
}

void xa_handle_selected_rows (GtkTreeSelection *selection,gpointer data)
{
	XArchive *archive = data;
	GList *list = NULL;
	gchar *measure = NULL;
	gchar *info = NULL;
	GtkTreeIter iter;
	GtkTreeModel *model;
	GtkTreePath *path = NULL;
	gint selected,n_elem = 0,pos = 0;
	unsigned long int total_size = 0;
	unsigned int size = 0;

	/*if ( ! GTK_WIDGET_VISIBLE (Extract_button) )
		return;*/

	switch (archive->type)
	{
		case XARCHIVETYPE_GZIP:
		case XARCHIVETYPE_BZIP2:
		case XARCHIVETYPE_LZMA:
		case XARCHIVETYPE_RPM:
		pos = 3;
		break;

		case XARCHIVETYPE_RAR:
		case XARCHIVETYPE_ARJ:
		case XARCHIVETYPE_7ZIP:
		pos = 2;
		break;

		case XARCHIVETYPE_LHA:
		pos = 4;
		break;

		case XARCHIVETYPE_TAR_GZ:
		case XARCHIVETYPE_TAR_BZ2:
		case XARCHIVETYPE_TAR_LZMA:
		case XARCHIVETYPE_TAR:
		case XARCHIVETYPE_DEB:
		case XARCHIVETYPE_ZIP:
		pos = 5;
		break;

		default:
		size = 0;
	}
	model = gtk_tree_view_get_model(GTK_TREE_VIEW(archive->treeview));
	if (selection == NULL)
		goto here;
	selected = gtk_tree_selection_count_selected_rows (selection);

	if (selected == 0 )
		xa_disable_delete_buttons (FALSE);
	else
	{
		if (archive->type == XARCHIVETYPE_RAR && unrar)
			gtk_widget_set_sensitive ( delete_menu,FALSE);
		else if ( archive->type != XARCHIVETYPE_RPM && archive->type != XARCHIVETYPE_DEB)
			gtk_widget_set_sensitive (delete_menu,TRUE);
	}
	if (selected > 0)
	{
		list = gtk_tree_selection_get_selected_rows(selection,NULL);
		while (list)
		{
			gtk_tree_model_get_iter(model,&iter,list->data);
			gtk_tree_model_get (model,&iter,pos,&size,-1);
			gtk_tree_path_free (list->data);
			total_size += size;
			list = list->next;
		}
		g_list_free(list);
		if (total_size > 1024*1024*1024 )
		{
			content_size = (double)total_size / (1024*1024*1024);
			measure = "GB";
		}
		else if (total_size > 1024*1024 )
		{
			content_size = (double)total_size / (1024*1024);
			measure = "MB";
		}
    	else if (total_size > 1024 )
		{
			content_size = (double)total_size / 1024;
			measure = "KB";
		}
		else
		{
			measure = "Bytes";
			content_size = total_size;
		}
		info = g_strdup_printf(ngettext ("%d item selected (%.1f %s)", "%d items selected (%.1f %s)", selected),selected,content_size,measure);
	}
	else
	{
here:
		if (model == NULL)
			return;
		path = gtk_tree_path_new_first();
		if (gtk_tree_model_get_iter (model, &iter, path) == FALSE)
			return;
		do
    	{
    		n_elem++;
    		gtk_tree_model_get (model,&iter,pos,&size,-1);
	  		total_size += size;
    	}
  		while (gtk_tree_model_iter_next (model,&iter));
  		if (total_size > 1024*1024*1024 )
		{
			content_size = (double)total_size / (1024*1024*1024);
			measure = "GB";
		}
		else if (total_size > 1024*1024 )
		{
			content_size = (double)total_size / (1024*1024);
			measure = "MB";
		}
    	else if (total_size > 1024 )
		{
			content_size = (double)total_size / 1024;
			measure = "KB";
		}
		else
		{
			measure = "Bytes";
			content_size = total_size;
		}
		info = g_strdup_printf(ngettext ("%d item (%.1f %s)", "%d items (%.1f %s)", n_elem),n_elem,content_size,measure);
	}
	Update_StatusBar(info);
	g_free(info);
}

void Update_StatusBar (gchar *msg)
{
	if (xa_main_window == NULL)
		return;
	gtk_label_set_text (GTK_LABEL(info_label),msg);
}

void drag_begin (GtkWidget *treeview1,GdkDragContext *context, gpointer data)
{
	XArchive *archive = data;
    GtkTreeSelection *selection;
    GtkTreeIter       iter;
    gchar            *name;
    GList            *row_list;
	XEntry *entry;
	
	//gtk_drag_source_set_icon_name (treeview1, DATADIR "/pixmaps/xarchiver.png" );
    selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (archive->treeview));

	row_list = gtk_tree_selection_get_selected_rows (selection, NULL);
	if ( row_list == NULL)
		return;

	gtk_tree_model_get_iter(archive->model,&iter,(GtkTreePath*) (row_list->data));
	gtk_tree_model_get (GTK_TREE_MODEL (archive->liststore),&iter,archive->nc+1,&entry, -1);
	name = xa_build_full_path_name_from_entry(entry);
	gchar *no_slashes = g_strrstr(name,"/");
	if (no_slashes != NULL)
		no_slashes++;
	gdk_property_change (context->source_window,
		               gdk_atom_intern ("XdndDirectSave0", FALSE),
			           gdk_atom_intern ("text/plain", FALSE), 8,
				       GDK_PROP_MODE_REPLACE,
					   (const guchar *) (no_slashes != NULL ? no_slashes : name), no_slashes != NULL ? strlen (no_slashes) : strlen (name) );

	g_list_foreach (row_list, (GFunc) gtk_tree_path_free, NULL);
	g_list_free (row_list);
	g_free (name);
}

void drag_end (GtkWidget *treeview1,GdkDragContext *context, gpointer data)
{
   /* Nothing to do */
}

void drag_data_get (GtkWidget *widget, GdkDragContext *dc, GtkSelectionData *selection_data, guint info, guint t, gpointer data)
{
	XArchive *archive = data;
	GtkTreeSelection *selection;
	guchar *fm_path;
	int fm_path_len;
	gchar *no_uri_path;
	gchar *to_send = "E";
	GList *row_list = NULL;
	GSList *names = NULL;
	gboolean full_path,overwrite;

    selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (archive->treeview));
	row_list = gtk_tree_selection_get_selected_rows (selection, NULL);

	if ( row_list == NULL)
		return;
	if (archive->status == XA_ARCHIVESTATUS_EXTRACT)
	{
		response = xa_show_message_dialog (GTK_WINDOW (xa_main_window),GTK_DIALOG_MODAL,GTK_MESSAGE_ERROR,GTK_BUTTONS_OK,_("Can't perform another extraction:"),_("Please wait until the completion of the current one!") );
		return;
	}
	if ( gdk_property_get (dc->source_window,
							gdk_atom_intern ("XdndDirectSave0", FALSE),
							gdk_atom_intern ("text/plain", FALSE),
							0, 1024, FALSE, NULL, NULL, &fm_path_len, &fm_path)
							&& fm_path != NULL)
	{
		/*  Zero-Terminate the string */
		fm_path = g_realloc (fm_path, fm_path_len + 1);
		fm_path[fm_path_len] = '\0';
		no_uri_path = g_filename_from_uri ( (gchar*)fm_path, NULL, NULL );
		/* g_message ("%s - %s",fm_path,no_uri_path); */
		g_free ( fm_path );
		if (no_uri_path == NULL)
		{
			gtk_drag_finish (dc, FALSE, FALSE, t);
			return;
		}
		if (archive->has_passwd)
		{
			if (archive->passwd == NULL)
			{
				archive->passwd = xa_create_password_dialog(NULL);
				if ( archive->passwd == NULL)
				{
					gtk_drag_finish (dc,FALSE,FALSE,t);
					return;
				}
			}
		}
		archive->extraction_path = extract_local_path (no_uri_path);
		g_free (no_uri_path);
		if (archive->extraction_path != NULL)
			to_send = "S";

		gtk_tree_selection_selected_foreach (selection, (GtkTreeSelectionForeachFunc) xa_concat_filenames,&names);
		full_path = archive->full_path;
		overwrite = archive->overwrite;
		archive->full_path = 0;
		archive->overwrite = 1;
		(*archive->extract) (archive,names);

		archive->full_path = full_path;
		archive->overwrite = overwrite;
		gtk_selection_data_set (selection_data, selection_data->target,8,(guchar*)to_send,1);
	}

	if (archive->extraction_path != NULL)
	{
		g_free (archive->extraction_path);
		archive->extraction_path = NULL;
	}
	g_list_foreach (row_list,(GFunc) gtk_tree_path_free,NULL);
	g_list_free (row_list);
	archive->status = XA_ARCHIVESTATUS_IDLE;
}

void on_drag_data_received (GtkWidget *widget,GdkDragContext *context,int x,int y,GtkSelectionData *data, unsigned int info,unsigned int time,gpointer user_data)
{
	gchar **array = NULL;
	gchar *filename = NULL;
	gchar *_current_dir = NULL;
	gchar *current_dir = NULL;
	GSList *list = NULL;
	gboolean one_file;
	gboolean dummy_password;
	gboolean full_path,add_recurse;
	unsigned int len = 0;
	gint current_page;
	gint idx;

	current_page = gtk_notebook_get_current_page(notebook);
	array = gtk_selection_data_get_uris(data);
	if (array == NULL || GTK_WIDGET_VISIBLE(viewport2))
	{
		response = xa_show_message_dialog (GTK_WINDOW (xa_main_window),GTK_DIALOG_MODAL,GTK_MESSAGE_ERROR,GTK_BUTTONS_OK,_("Sorry, I could not perform the operation!"),"" );
		gtk_drag_finish(context,FALSE,FALSE,time);
		return;
	}
	gtk_drag_finish (context,TRUE,FALSE,time);
	one_file = (array[1] == NULL);

	if (one_file)
	{
		filename = g_filename_from_uri(array[0],NULL,NULL);
		if (filename == NULL)
			return;
		else if (xa_detect_archive_type(filename) > 0)
		{
			xa_open_archive(NULL,filename);
			g_strfreev(array);
			return;
		}
    }
	if (current_page == -1)
	{
		idx = xa_get_new_archive_idx();
		if (idx == -1)
			return;
		archive[idx] = xa_new_archive_dialog (filename,archive,TRUE);
		if (archive[idx] == NULL)
			return;
		xa_add_page (archive[idx]);
	}
	else
		idx = xa_find_archive_index (current_page);

	if (archive[idx]->type == XARCHIVETYPE_RAR && unrar)
	{
		response = xa_show_message_dialog (GTK_WINDOW (xa_main_window),GTK_DIALOG_MODAL,GTK_MESSAGE_ERROR,GTK_BUTTONS_OK,_("Can't perform this action:"),_("unrar doesn't support archive creation!") );
		return;
	}
	if (archive[idx]->type == XARCHIVETYPE_DEB || archive[idx]->type == XARCHIVETYPE_RPM)
	{
		gchar *msg;
		if (archive[idx]->type == XARCHIVETYPE_DEB)
			msg = _("You can't add content to deb packages!");
		else
			msg = _("You can't add content to rpm packages!");
		response = xa_show_message_dialog (GTK_WINDOW (xa_main_window),GTK_DIALOG_MODAL,GTK_MESSAGE_ERROR,GTK_BUTTONS_OK,_("Can't perform this action:"), msg );
		return;
	}
	_current_dir = g_path_get_dirname (array[0]);
	current_dir = g_filename_from_uri (_current_dir,NULL,NULL);
	g_free (_current_dir);
	chdir ( current_dir );
	g_free (current_dir);

	while (array[len])
	{
		filename = g_filename_from_uri (array[len],NULL,NULL);
		list = g_slist_append(list,filename);
		len++;
	}
	dummy_password = archive[idx]->has_passwd;
	full_path = archive[idx]->full_path;
	add_recurse = archive[idx]->add_recurse;

	archive[idx]->has_passwd = 0;
	archive[idx]->full_path = 0;
	archive[idx]->add_recurse = 1;
	xa_execute_add_commands(archive[idx],list,NULL);

	archive[idx]->has_passwd = dummy_password;
	archive[idx]->full_path = full_path;
	archive[idx]->add_recurse = add_recurse;
	if (list != NULL)
	{
		g_slist_foreach(list,(GFunc) g_free,NULL);
		g_slist_free(list);
	}
	g_strfreev (array);
}

gboolean key_press_function (GtkWidget *widget, GdkEventKey *event, gpointer data)
{
	if (event == NULL)
		return FALSE;
	switch (event->keyval)
	{
		case GDK_Escape:
		if ( GTK_WIDGET_VISIBLE (viewport2) )
			xa_cancel_archive (NULL, NULL);
		break;

	}
	return FALSE;
}

void xa_concat_filenames (GtkTreeModel *model, GtkTreePath *treepath, GtkTreeIter *iter, GSList **data)
{
	XEntry *entry = NULL;
	gchar *filename = NULL;
	gint current_page;
	gint idx;

	current_page = gtk_notebook_get_current_page(notebook);
	idx = xa_find_archive_index (current_page);
	
	gtk_tree_model_get (model,iter,archive[idx]->nc+1,&entry,-1);
	filename = xa_build_full_path_name_from_entry(entry);
	*data = g_slist_prepend (*data,filename);
}

void xa_select_all(GtkMenuItem *menuitem,gpointer user_data)
{
	gint idx;
	gint current_page;

	current_page = gtk_notebook_get_current_page (notebook);
	idx = xa_find_archive_index (current_page);

	current_page = gtk_notebook_get_current_page (notebook);
	gtk_tree_selection_select_all ( gtk_tree_view_get_selection (GTK_TREE_VIEW (archive[idx]->treeview) ) );
	gtk_widget_set_sensitive (select_all,FALSE);
	gtk_widget_set_sensitive (deselect_all,TRUE);
}

void xa_deselect_all ( GtkMenuItem *menuitem , gpointer user_data )
{
	gint current_page;
	gint idx;

	current_page = gtk_notebook_get_current_page (notebook);
	idx = xa_find_archive_index (current_page);

	gtk_tree_selection_unselect_all ( gtk_tree_view_get_selection (GTK_TREE_VIEW (archive[idx]->treeview) ) );
	gtk_widget_set_sensitive (select_all,TRUE);
	gtk_widget_set_sensitive (deselect_all,FALSE);
}

void xa_activate_link (GtkAboutDialog *about,const gchar *link,gpointer data)
{
	gboolean result;

	if ( !xdg_open)
	{
		gchar *browser_path = NULL;
		browser_path = gtk_combo_box_get_active_text(GTK_COMBO_BOX(prefs_window->combo_prefered_web_browser));
		if (strlen(browser_path) == 0)
		{
			response = xa_show_message_dialog (GTK_WINDOW (xa_main_window),GTK_DIALOG_MODAL,GTK_MESSAGE_INFO,GTK_BUTTONS_OK,_("You didn't set which browser to use!"),_("Please go to Preferences->Advanced and set it."));
			g_free (browser_path);
			return;
		}
		result = xa_launch_external_program(browser_path,(gchar *)link);
		if (browser_path != NULL)
			g_free (browser_path);
	}
	else
		xa_launch_external_program("xdg-open",(gchar *)link);
}

gboolean xa_launch_external_program(gchar *program,gchar *arg)
{
	GtkWidget *message;
	GError *error = NULL;
	gchar *argv[3];
	GdkScreen *screen;

	argv[0] = program;
	argv[1] = arg;
	argv[2] = NULL;

	screen = gtk_widget_get_screen (GTK_WIDGET (xa_main_window));
	if (!gdk_spawn_on_screen (screen,NULL,argv,NULL,G_SPAWN_SEARCH_PATH,NULL,NULL,NULL,&error))
	{
		message = gtk_message_dialog_new (GTK_WINDOW (xa_main_window),
										GTK_DIALOG_MODAL
										| GTK_DIALOG_DESTROY_WITH_PARENT,
										GTK_MESSAGE_ERROR,
										GTK_BUTTONS_CLOSE,
										_("Failed to launch the application!"));
		gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (message), "%s.", error->message);
		gtk_dialog_run (GTK_DIALOG (message));
		gtk_widget_destroy (message);
		g_error_free (error);
		return FALSE;
	}
	return TRUE;
}

void xa_show_help (GtkMenuItem *menuitem,gpointer user_data )
{
	gchar *uri = g_strconcat ("file://", DATADIR, "/doc/", PACKAGE, "/html/index.html", NULL);
	xa_activate_link (NULL,uri,NULL);
	g_free (uri);
}

void xa_reset_password (GtkMenuItem *menuitem , gpointer user_data )
{
	gint current_page;
	gint idx;

	current_page = gtk_notebook_get_current_page(notebook);
	idx = xa_find_archive_index ( current_page );

	if (archive[idx] == NULL)
		return;

	if (archive[idx]->passwd != NULL)
	{
		g_free (archive[idx]->passwd);
		archive[idx]->passwd = NULL;
		Update_StatusBar (_("The password has been reset."));
	}
	else
		Update_StatusBar (_("Please enter the password first!"));
}

void xa_show_archive_comment (GtkMenuItem *menuitem,gpointer user_data)
{
	widget_data *comment_window;
	gchar *utf8_line;
	gsize len;
	gint current_page;
	gint idx;

	current_page = gtk_notebook_get_current_page(notebook);
	idx = xa_find_archive_index (current_page);

	comment_window = xa_create_output_window(_("Archive Comment"));

	gtk_text_buffer_insert (comment_window->textbuffer, &comment_window->iter, "\n", 1);
	utf8_line = g_locale_to_utf8 (archive[current_page]->comment->str, -1, NULL, &len, NULL);
	gtk_text_buffer_insert_with_tags_by_name (comment_window->textbuffer, &comment_window->iter, utf8_line, len, "font", NULL);
	g_free(utf8_line);

	gtk_dialog_run (GTK_DIALOG(comment_window->dialog1));
	gtk_widget_destroy (comment_window->dialog1);
	g_free(comment_window);
}

void xa_location_entry_activated (GtkEntry *entry, gpointer user_data)
{
	XEntry *prev_entry = NULL;
	XEntry *new_entry  = NULL;
	gint current_page;
	gint idx;

	current_page = gtk_notebook_get_current_page (notebook);
	idx = xa_find_archive_index (current_page);

	/* Avoid segfault if there's no file opened */
	if(idx<0)
		return;

	if (strlen(gtk_entry_get_text(GTK_ENTRY(location_entry))) == 0)
	{
		xa_update_window_with_archive_entries(archive[idx],new_entry);
		return;
	}

	new_entry  = xa_find_entry_from_path(archive[idx]->root_entry,gtk_entry_get_text(GTK_ENTRY(location_entry)));
	if (new_entry == NULL)
	{
		if (archive[idx]->location_entry_path != NULL)
			gtk_entry_set_text(GTK_ENTRY(location_entry),archive[idx]->location_entry_path);
		return;
	}

	if (archive[idx]->location_entry_path != NULL)
		prev_entry = xa_find_entry_from_path(archive[idx]->root_entry,archive[idx]->location_entry_path);

	if (prev_entry != NULL)
		archive[idx]->back = g_slist_prepend(archive[idx]->back,prev_entry);
	else
		archive[idx]->back = g_slist_prepend(archive[idx]->back,NULL);

	xa_sidepane_select_row(new_entry);
	xa_update_window_with_archive_entries(archive[idx],new_entry);
}

int xa_mouse_button_event(GtkWidget *widget,GdkEventButton *event,gpointer data)
{
	XArchive *archive = data;
	GtkTreePath *path;
	GtkTreeIter  iter;
	GtkTreeSelection *selection;

	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (archive->treeview));
	gtk_tree_view_get_path_at_pos (GTK_TREE_VIEW (archive->treeview),event->x, event->y,&path,NULL,NULL,NULL);
	if (path == NULL)
		return FALSE;
	if ((event->type == GDK_BUTTON_PRESS) && (event->button == 3))
	{
		gtk_tree_model_get_iter (GTK_TREE_MODEL (archive->liststore),&iter,path);
		gtk_tree_path_free (path);
		if (! gtk_tree_selection_iter_is_selected (selection,&iter))
		{
			gtk_tree_selection_unselect_all (selection);
			gtk_tree_selection_select_iter (selection,&iter);
		}
		gtk_menu_popup (GTK_MENU (xa_popup_menu),NULL,NULL,NULL,xa_main_window,event->button,event->time);
		return TRUE;
	}
	return FALSE;
}

void xa_treeview_row_activated(GtkTreeView *tree_view,GtkTreePath *path,GtkTreeViewColumn *column,gpointer user_data)
{
	XArchive *archive = user_data;
	XEntry *entry;
	GtkTreeIter iter;

	if (! gtk_tree_model_get_iter (GTK_TREE_MODEL (archive->liststore),&iter,path))
		return;

	gtk_tree_model_get (GTK_TREE_MODEL (archive->liststore),&iter,archive->nc+1,&entry,-1);
	if (! entry->is_dir)
		return;
	if (archive->location_entry_path != NULL)
		archive->back = g_slist_prepend(archive->back,xa_find_entry_from_path(archive->root_entry,archive->location_entry_path));
	/* Put NULL so to display the root entry */
	else
		archive->back = g_slist_prepend(archive->back,NULL);

	xa_sidepane_select_row(entry);
	xa_update_window_with_archive_entries(archive,entry);
}

void xa_update_window_with_archive_entries (XArchive *archive,XEntry *entry)
{
	GdkPixbuf *pixbuf = NULL;
	GtkTreeIter iter;
	GtkTreeIter *last_dir_iter = NULL;
	unsigned short int i;
	gpointer current_column;

	archive->current_entry = entry;
	if (entry == NULL)
	{
		entry = archive->root_entry->child;
		gtk_entry_set_text(GTK_ENTRY(location_entry),"\0");
		gtk_tree_selection_unselect_all(gtk_tree_view_get_selection (GTK_TREE_VIEW (archive_dir_treeview)));
		if (archive->location_entry_path != NULL)
		{
			g_free(archive->location_entry_path);
			archive->location_entry_path = NULL;
		}
		gtk_widget_set_sensitive(back_button,FALSE);
		gtk_widget_set_sensitive(up_button,FALSE);
		gtk_widget_set_sensitive(home_button,FALSE);
	}
	else
	{
		if (archive->location_entry_path != NULL)
		{
			g_free(archive->location_entry_path);
			archive->location_entry_path = NULL;
		}
		gtk_widget_set_sensitive(back_button,TRUE);
		gtk_widget_set_sensitive(up_button,TRUE);
		gtk_widget_set_sensitive(home_button,TRUE);
		archive->location_entry_path = xa_build_full_path_name_from_entry(entry);
		gtk_entry_set_text(GTK_ENTRY(location_entry),archive->location_entry_path);
		entry = entry->child;
	}
	gtk_list_store_clear(archive->liststore);

	while (entry)
	{
		current_column = entry->columns;
		//gtk_list_store_append (archive->liststore, &iter);
		if (entry->is_dir)
		{
			if (last_dir_iter == NULL)
				gtk_list_store_prepend(archive->liststore, &iter);
			else
			{
				gtk_list_store_insert_after(archive->liststore, &iter, last_dir_iter);
				gtk_tree_iter_free(last_dir_iter);
			}
			last_dir_iter = gtk_tree_iter_copy(&iter);
		}
		else
			gtk_list_store_append(archive->liststore, &iter);

		if(!g_utf8_validate(entry->filename, -1, NULL) )
		{
			gchar *dummy = g_convert(entry->filename, -1, "UTF-8", "WINDOWS-1252", NULL, NULL, NULL);
			if (dummy != NULL)
			{
				g_free (entry->filename);
				entry->filename = dummy;
			}
		}
		if (entry->is_dir)
			pixbuf = xa_get_pixbuf_icon_from_cache("folder");
		else if (entry->is_encrypted)
		{
			pixbuf = xa_get_pixbuf_icon_from_cache("lock");
			archive->has_passwd = TRUE;
		}
		else
			pixbuf = xa_get_pixbuf_icon_from_cache(entry->filename);

		gtk_list_store_set (archive->liststore,&iter,archive->nc+1, entry,-1);
		gtk_list_store_set (archive->liststore,&iter,0,pixbuf,1,entry->filename,-1);

		for (i = 0; i < archive->nc; i++)
		{
			switch(archive->column_types[i+2])
			{
				case G_TYPE_STRING:
					//g_message ("%d - %s",i,(*((gchar **)current_column)));
					gtk_list_store_set (archive->liststore,&iter,i+2,(*((gchar **)current_column)),-1);
					current_column += sizeof(gchar *);
				break;

				case G_TYPE_UINT64:
					//g_message ("*%d - %lu",i,(*((guint64 *)current_column)));
					gtk_list_store_set (archive->liststore,&iter,i+2,(*((guint64 *)current_column)),-1);
					current_column += sizeof(guint64);
				break;
			}
		}
		entry = entry->next;
	}
	xa_fill_dir_sidebar(archive,FALSE);
	xa_handle_selected_rows(NULL,archive);
}
