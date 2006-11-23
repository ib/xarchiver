/*
 *  Copyright (C) 2006 Giuseppe Torelli - <colossus73@gmail.com>
 *  Copyright (C) 2006 Benedikt Meurer - <benny@xfce.org>
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

#include "callbacks.h"
#include "string_utils.h"
#include "interface.h"
#include "support.h"
#include "main.h"
#include "socket.h"
#include "new_dialog.h"

extern GList *ArchiveType;
extern GList *ArchiveSuffix;
extern gboolean cli;
extern gboolean stop_flag;
extern gboolean unrar;

gchar *current_open_directory = NULL;
GtkFileFilter *open_file_filter = NULL;
GList *Suffix , *Name;

void xa_watch_child ( GPid pid, gint status, gpointer data)
{
	XArchive *archive = data;
	gboolean new	= FALSE;
	gboolean open	= FALSE;
	gboolean add	= FALSE;
	gboolean extract= FALSE;
	gboolean exe	= FALSE;
	gboolean select	= FALSE;
	gboolean check	= FALSE;
	gboolean info	= FALSE;
	gboolean waiting = TRUE;
	int ps;

	if (archive->has_comment)
		gtk_widget_set_sensitive (comment_menu,TRUE);
	else
		gtk_widget_set_sensitive (comment_menu,FALSE);

	if (archive->has_comment && archive->status == XA_ARCHIVESTATUS_OPEN)
		xa_show_archive_comment ( NULL, NULL);

	gtk_widget_set_sensitive (close1,TRUE);
	if ( archive->type == XARCHIVETYPE_BZIP2 || archive->type == XARCHIVETYPE_GZIP )
	{
		new = open = TRUE;
		info = exe = FALSE;
	}
	else if (archive->type == XARCHIVETYPE_RPM || archive->type == XARCHIVETYPE_DEB)
	{
		new = open = extract = select = info = TRUE;
		exe = FALSE;
	}
	else if (archive->type == XARCHIVETYPE_TAR_BZ2 || archive->type == XARCHIVETYPE_TAR_GZ || archive->type == XARCHIVETYPE_TAR )
	{
		new = open = add = extract = select = info = TRUE;
		check = exe = FALSE;
	}
	else if (archive->type == XARCHIVETYPE_LHA)
	{
		new = open = add = extract = select = info = TRUE;
		check = TRUE;
		exe = FALSE;
	}
	else if (archive->type == XARCHIVETYPE_RAR && unrar)
	{
		check = TRUE;
		add = exe = FALSE;
		new = open = extract = select = info = TRUE;
	}
	else
	{
		check = TRUE;
		new = open = add = extract = exe = select = info = TRUE;
	}
	if ( WIFSIGNALED (status) )
	{
		Update_StatusBar ( _("Operation canceled."));
		if (archive->status == XA_ARCHIVESTATUS_EXTRACT)
		{
			gchar *msg = g_strdup_printf(_("Please check \"%s\" since some files could have been already extracted."),archive->extraction_path);

            response = ShowGtkMessageDialog (GTK_WINDOW (MainWindow),GTK_DIALOG_MODAL,GTK_MESSAGE_INFO,	GTK_BUTTONS_OK,"",msg );
            g_free (msg);
		}
		else if (archive->status == XA_ARCHIVESTATUS_OPEN)
			gtk_widget_set_sensitive ( check_menu , FALSE );

		xa_hide_progress_bar_stop_button(archive);
		xa_set_button_state (new,open,add,extract,exe,select);
		gtk_widget_set_sensitive ( check_menu , check);
		gtk_widget_set_sensitive ( properties , info);
		return;
	}

	if ( WIFEXITED (status) )
	{
		if ( WEXITSTATUS (status) )
		{
			gtk_widget_set_sensitive ( check_menu , check);
			gtk_widget_set_sensitive ( properties , info);
			xa_hide_progress_bar_stop_button(archive);
			xa_set_button_state (new,open,add,extract,exe,select);
			Update_StatusBar ( _("Operation failed."));
			response = ShowGtkMessageDialog (GTK_WINDOW	(MainWindow),GTK_DIALOG_MODAL,GTK_MESSAGE_QUESTION,GTK_BUTTONS_YES_NO,_("An error occurred while accessing the archive."),_("Do you want to view the command line output?") );
			if (response == GTK_RESPONSE_YES)
				xa_show_cmd_line_output (NULL);
			/* In case the user supplies a wrong password we reset it so he can try again */
			if ( (archive->status == XA_ARCHIVESTATUS_TEST || archive->status == XA_ARCHIVESTATUS_SFX) && archive->passwd != NULL)
			{
				g_free (archive->passwd);
				archive->passwd = NULL;
			}
			return;
		}
	}

	if (archive->status == XA_ARCHIVESTATUS_SFX && archive->type == XARCHIVETYPE_RAR)
	{
		xa_hide_progress_bar_stop_button(archive);
		gtk_widget_set_sensitive ( exe_menu, FALSE);
		gtk_widget_set_sensitive ( Exe_button, FALSE);
		response = ShowGtkMessageDialog (GTK_WINDOW (MainWindow),GTK_DIALOG_MODAL,GTK_MESSAGE_INFO,	GTK_BUTTONS_OK,_("The sfx archive was saved as:"),archive->tmp );
	}
	if (archive->status == XA_ARCHIVESTATUS_TEST)
		xa_show_cmd_line_output (NULL);
	if ( ! cli )
	{
		/* This to automatically reload the content of the archive after adding or deleting */
		if (archive->status == XA_ARCHIVESTATUS_DELETE || archive->status == XA_ARCHIVESTATUS_ADD)
		{
		    if (archive->type == XARCHIVETYPE_BZIP2 || archive->type == XARCHIVETYPE_GZIP)
				Update_StatusBar ( _("Operation completed."));
			else
			{
				Update_StatusBar ( _("Please wait while the content of the archive is being updated..."));
				xa_remove_columns();
			}
			switch ( archive->type )
			{
				case XARCHIVETYPE_RAR:
			    OpenRar ( archive );
				break;

				case XARCHIVETYPE_TAR:
				OpenTar ( archive );
				break;

				case XARCHIVETYPE_TAR_BZ2:
				OpenBzip2 ( archive );
				break;

				case XARCHIVETYPE_TAR_GZ:
				OpenGzip ( archive );
				break;

				case XARCHIVETYPE_ZIP:
				OpenZip ( archive );
				break;

				case XARCHIVETYPE_7ZIP:
				Open7Zip ( archive );
				break;

				case XARCHIVETYPE_ARJ:
				OpenArj ( archive );
				break;

				case XARCHIVETYPE_LHA:
				OpenLha ( archive );
				break;

				default:
				break;
			}
			archive->status = XA_ARCHIVESTATUS_IDLE;
			while (waiting)
			{
				ps = waitpid ( archive->child_pid, &status, WNOHANG);
				if (ps < 0)
					waiting = FALSE;
				else
					gtk_main_iteration_do (FALSE);
			}
		}
	}

	if (! cli && archive != NULL)
	{
		if ( archive->has_passwd == FALSE && archive->passwd == NULL)
		{
			gtk_widget_hide ( viewport3 );
			gtk_widget_set_sensitive ( password_entry , FALSE);
		}
		else
		{
			gtk_widget_show ( pad_image );
			gtk_widget_show ( viewport3 );
			gtk_widget_set_sensitive ( password_entry , TRUE);
		}
	}
	gtk_widget_set_sensitive ( check_menu , check);
	gtk_widget_set_sensitive ( properties , info);
	xa_set_button_state (new,open,add,extract,exe,select);
	xa_hide_progress_bar_stop_button(archive);

	gtk_widget_grab_focus (GTK_WIDGET(archive->treeview));
	Update_StatusBar ( _("Operation completed."));
}

void xa_new_archive (GtkMenuItem *menuitem, gpointer user_data)
{
	gint current_page;

	current_page = xa_get_new_archive_idx();
	if (current_page == -1)
		return;

	archive[current_page] = xa_new_archive_dialog ( NULL );

	if (archive[current_page]  == NULL)
		return;

	xa_add_page (archive[current_page]);

	xa_set_button_state (1,1,1,0,0,0 );
    EmptyTextBuffer();
    archive[current_page]->has_passwd = FALSE;
    gtk_widget_set_sensitive ( iso_info , FALSE );
    gtk_widget_set_sensitive ( view_shell_output1 , TRUE );
    gtk_widget_set_sensitive ( check_menu , FALSE);
    gtk_widget_set_sensitive ( properties , FALSE );
    /* Let's off the delete and view buttons and the menu entries to avoid strange behaviours */
    OffDeleteandViewButtons ();

  	Update_StatusBar ( _("Choose Add to begin creating the archive."));
    gtk_tooltips_disable ( pad_tooltip );
    gtk_widget_hide ( pad_image );

    archive[current_page]->passwd = NULL;
    archive[current_page]->dummy_size = 0;
    archive[current_page]->nr_of_files = 0;
    archive[current_page]->nr_of_dirs = 0;
	xa_set_window_title (MainWindow , archive[current_page]->path );
	gtk_widget_set_sensitive ( close1 , TRUE );
}

int ShowGtkMessageDialog ( GtkWindow *window, int mode,int type,int button, const gchar *message1,const gchar *message2)
{
	dialog = gtk_message_dialog_new (window, mode, type, button,message1);
	gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_YES);
	gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dialog), message2);
	response = gtk_dialog_run (GTK_DIALOG (dialog) );
	gtk_widget_destroy (GTK_WIDGET (dialog) );
	return response;
}

void xa_open_archive (GtkMenuItem *menuitem, gpointer data)
{
	gchar *path = NULL;
	gint current_page;
	gint x;
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
			return;
	}

	current_page = xa_get_new_archive_idx();
	if (current_page == -1)
		return;

	archive[current_page] = xa_init_archive_structure();
	if (archive[current_page] == NULL)
	{
		response = ShowGtkMessageDialog (GTK_WINDOW (MainWindow),GTK_DIALOG_MODAL,GTK_MESSAGE_ERROR,GTK_BUTTONS_OK,_("Can't allocate memory for the archive structure:"),"Operation aborted!");
		g_free (path);
		return;
	}

	archive[current_page]->path = g_strdup (path);
	archive[current_page]->escaped_path = EscapeBadChars ( archive[current_page]->path , "$\'`\"\\!?* ()&|@#:;" );

	xa_add_page (archive[current_page]);

	OffDeleteandViewButtons();
	gtk_widget_set_sensitive ( iso_info , FALSE );
	gtk_widget_set_sensitive ( view_shell_output1 , TRUE );

	archive[current_page]->type = xa_detect_archive_type ( archive[current_page] , NULL );
	if ( archive[current_page]->type == -2 )
	{
		xa_close_archive ( NULL, NULL);
		xa_set_button_state (1,1,0,0,0,0);
		return;
	}
	if ( archive[current_page]->type == -1 )
	{
		xa_close_archive ( NULL, NULL);
		gchar *utf8_path,*msg;
		utf8_path = g_filename_to_utf8 (path, -1, NULL, NULL, NULL);
		msg = g_strdup_printf (_("Can't open file \"%s\":"), utf8_path);
		xa_set_window_title (MainWindow , NULL);
		response = ShowGtkMessageDialog (GTK_WINDOW (MainWindow),GTK_DIALOG_MODAL,GTK_MESSAGE_ERROR,GTK_BUTTONS_OK,msg,
		_("Archive format is not recognized!"));
		xa_set_button_state ( 1,1,0,0,0,0);
		gtk_widget_set_sensitive ( check_menu , FALSE );
		gtk_widget_set_sensitive ( properties , FALSE );
		g_free (utf8_path);
		g_free (msg);
		g_free (path);
		return;
	}
	g_free (path);
	EmptyTextBuffer();

	//Does the user open an archive from the command line whose archiver is not installed ?
	gchar *ext = NULL;
	if ( archive[current_page]->type == XARCHIVETYPE_RAR )
		ext = ".rar";
	else if ( archive[current_page]->type == XARCHIVETYPE_7ZIP )
		ext = ".7z";
	else if ( archive[current_page]->type == XARCHIVETYPE_ARJ )
		ext = ".arj";
	else if ( archive[current_page]->type == XARCHIVETYPE_LHA )
		ext = ".lzh";
	if ( ext != NULL )
		if ( ! g_list_find ( ArchiveType , ext ) )
		{
			response = ShowGtkMessageDialog (GTK_WINDOW (MainWindow),GTK_DIALOG_MODAL,GTK_MESSAGE_ERROR, GTK_BUTTONS_OK,_("Sorry, this archive format is not supported:"),_("the proper archiver is not installed!") );
			return;
		}

	gtk_widget_set_sensitive (Stop_button,TRUE);
	gtk_widget_show ( viewport2 );

	if ( archive[current_page]->type == XARCHIVETYPE_ISO )
		Update_StatusBar ( _("Please wait while the content of the ISO image is being read..."));
	else
		Update_StatusBar ( _("Please wait while the content of the archive is being read..."));

	archive[current_page]->status = XA_ARCHIVESTATUS_OPEN;
	gtk_widget_set_sensitive ( close1 , 	FALSE);
	gtk_widget_set_sensitive ( check_menu , FALSE);
	gtk_widget_set_sensitive ( properties , FALSE);
	xa_set_button_state ( 0,0,0,0,0,0);
	switch ( archive[current_page]->type )
	{
		case XARCHIVETYPE_ARJ:
		OpenArj (archive[current_page]);
		break;

		case XARCHIVETYPE_DEB:
		OpenDeb (archive[current_page]);
		break;

		case XARCHIVETYPE_BZIP2:
		OpenBzip2 (archive[current_page]);
		break;

		case XARCHIVETYPE_GZIP:
		OpenGzip ( archive[current_page] );
		break;

		case XARCHIVETYPE_ISO:
		OpenISO (archive[current_page]);
		break;

		case XARCHIVETYPE_RAR:
		OpenRar (archive[current_page]);
		break;

		case XARCHIVETYPE_RPM:
		OpenRPM (archive[current_page]);
		break;

		case XARCHIVETYPE_TAR:
		OpenTar (archive[current_page]);
		break;

		case XARCHIVETYPE_ZIP:
		OpenZip (archive[current_page]);
		break;

		case XARCHIVETYPE_7ZIP:
		Open7Zip (archive[current_page]);
		break;

		case XARCHIVETYPE_LHA:
		OpenLha (archive[current_page]);
		break;

		default:
		break;
	}
	archive[current_page]->passwd = NULL;
}

void xa_test_archive (GtkMenuItem *menuitem, gpointer user_data)
{
	gchar *command;
	gchar *rar;
	gint current_page;
	gint id;

	current_page = gtk_notebook_get_current_page (notebook);
	id = xa_find_archive_index ( current_page );
	if (unrar)
		rar = "unrar";
	else
		rar = "rar";
	if ( archive[id]->has_passwd )
	{
		if ( archive[id]->passwd == NULL)
		{
			archive[id]->passwd = password_dialog ();
			if ( archive[id]->passwd == NULL)
				return;
		}
	}
    Update_StatusBar ( _("Testing archive integrity, please wait..."));
    gtk_widget_set_sensitive (Stop_button,TRUE);
    gtk_widget_set_sensitive ( check_menu , FALSE );
    xa_set_button_state (0,0,0,0,0,0);
    switch ( archive[id]->type )
	{
		case XARCHIVETYPE_RAR:
		if (archive[id]->passwd != NULL)
			command = g_strconcat (rar," t -idp -p" , archive[id]->passwd ," " , archive[id]->escaped_path, NULL);
		else
			command = g_strconcat (rar," t -idp " , archive[id]->escaped_path, NULL);
        break;

        case XARCHIVETYPE_ZIP:
        if (archive[id]->passwd != NULL)
			command = g_strconcat ("unzip -P ", archive[id]->passwd, " -t " , archive[id]->escaped_path, NULL);
        else
			command = g_strconcat ("unzip -t " , archive[id]->escaped_path, NULL);
        break;

        case XARCHIVETYPE_7ZIP:
        if (archive[id]->passwd != NULL)
			command = g_strconcat ( "7za t -p" , archive[id]->passwd , " " , archive[id]->escaped_path, NULL);
		else
			command = g_strconcat ("7za t " , archive[id]->escaped_path, NULL);
		break;

		case XARCHIVETYPE_ARJ:
        if (archive[id]->passwd != NULL)
			command = g_strconcat ("arj t -g" , archive[id]->passwd , " -i " , archive[id]->escaped_path, NULL);
		else
			command = g_strconcat ("arj t -i " , archive[id]->escaped_path, NULL);
		break;

		case XARCHIVETYPE_LHA:
			command = g_strconcat ("lha t " , archive[id]->escaped_path, NULL);
		break;

		default:
		command = NULL;
	}
	archive[id]->status = XA_ARCHIVESTATUS_TEST;
    xa_run_command ( command , 1);
    g_free (command);
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
	if ( current_page == 0)
	{
		gtk_widget_set_sensitive (check_menu,FALSE);
		gtk_widget_set_sensitive (properties,FALSE);
		gtk_widget_set_sensitive (close1,FALSE);
		OffDeleteandViewButtons();
		xa_set_button_state (1,1,0,0,0,0);
		xa_set_window_title (MainWindow,NULL);
	}
	else if ( current_page == 1)
		gtk_notebook_set_show_tabs (notebook,FALSE);
	else
		gtk_notebook_set_show_tabs (notebook,TRUE);

	EmptyTextBuffer();
	xa_clean_archive_structure (archive[idx]);

	gtk_widget_hide ( viewport3 );
	Update_StatusBar (_("Ready."));
}

void xa_quit_application (GtkMenuItem *menuitem, gpointer user_data)
{
	gint i;
	gint idx;

	if ( GTK_WIDGET_VISIBLE (viewport2 ) )
	{
		Update_StatusBar ( _("Please hit the Stop button first!"));
		return;
	}
	g_list_free ( Suffix );
	g_list_free ( Name );

	for (i = 0; i < gtk_notebook_get_n_pages(notebook) ; i++)
	{
		idx = xa_find_archive_index ( i );
		if (archive[idx] != NULL)
			xa_clean_archive_structure (archive[idx]);
	}

	if (current_open_directory != NULL)
		g_free (current_open_directory);

#ifdef HAVE_SOCKET
	socket_finalize();
#endif
	gtk_main_quit();
}

void xa_delete_archive (GtkMenuItem *menuitem, gpointer user_data)
{
	gchar *command = NULL;
	gchar *tar;
	gint x;
	GString *names;
	gint current_page;
	gint id;

	current_page = gtk_notebook_get_current_page ( notebook);
	id = xa_find_archive_index ( current_page);

	GtkTreeSelection *selection = gtk_tree_view_get_selection ( GTK_TREE_VIEW (archive[id]->treeview) );
	names = g_string_new ( " " );
	gtk_tree_selection_selected_foreach (selection, (GtkTreeSelectionForeachFunc) ConcatenateFileNames, names );

	x = gtk_tree_selection_count_selected_rows (selection);
	gchar *msg = g_strdup_printf (_("You are about to delete %d file(s) from the archive."),x);
	response = ShowGtkMessageDialog (GTK_WINDOW (MainWindow),GTK_DIALOG_MODAL,GTK_MESSAGE_QUESTION,GTK_BUTTONS_YES_NO,msg,_( "Are you sure you want to do this?") );
	g_free (msg);

	if ( response == GTK_RESPONSE_NO)
		return;

	Update_StatusBar ( _("Deleting files from the archive, please wait..."));
	archive[id]->status = XA_ARCHIVESTATUS_DELETE;

	tar = g_find_program_in_path ("gtar");
	if (tar == NULL)
		tar = g_strdup ("tar");

	switch (archive[id]->type)
	{
		case XARCHIVETYPE_RAR:
		command = g_strconcat ( "rar d " , archive[id]->escaped_path , names->str , NULL );
		break;

        case XARCHIVETYPE_TAR:
		command = g_strconcat (tar, " --delete -vf " , archive[id]->escaped_path , names->str , NULL );
		break;

        case XARCHIVETYPE_TAR_BZ2:
        xa_add_delete_tar_bzip2_gzip ( names , archive[id] , 0 , 0 );
        break;

        case XARCHIVETYPE_TAR_GZ:
        xa_add_delete_tar_bzip2_gzip ( names , archive[id] , 1 , 0 );
		break;

        case XARCHIVETYPE_ZIP:
		command = g_strconcat ( "zip -d " , archive[id]->escaped_path , names->str , NULL );
		break;

        case XARCHIVETYPE_7ZIP:
        command = g_strconcat ( "7za d " , archive[id]->escaped_path , names->str , NULL );
        break;

        case XARCHIVETYPE_ARJ:
        command = g_strconcat ( "arj d " , archive[id]->escaped_path , names->str, NULL);
        break;

		case XARCHIVETYPE_LHA:
		command = g_strconcat("lha d ", archive[id]->escaped_path, names->str, NULL);
		break;

        default:
        break;
	}
	if (command != NULL)
    {
    	xa_set_button_state (0,0,0,0,0,0);
    	gtk_widget_set_sensitive (Stop_button,TRUE);
        xa_run_command ( command , 1);
        g_free (command);
    }
    g_string_free (names , TRUE );
    g_free (tar);
}

void xa_add_files_archive ( GtkMenuItem *menuitem, gpointer data )
{
	gchar *command = NULL;
	gint current_page;
	gint idx;

	current_page = gtk_notebook_get_current_page (notebook);
	idx = xa_find_archive_index ( current_page );

	add_window = xa_create_add_dialog (archive[idx]);
	command = xa_parse_add_dialog_options ( archive[idx], add_window );
	gtk_widget_destroy ( add_window->dialog1 );
	if (command != NULL)
	{
		xa_run_command (command , 1);
		g_free (command);
	}
	g_free ( add_window );
	add_window = NULL;
}

void xa_extract_archive ( GtkMenuItem *menuitem , gpointer user_data )
{
	gchar *command = NULL;
	gint current_page;
	gint idx;

	current_page = gtk_notebook_get_current_page (notebook);
	idx = xa_find_archive_index ( current_page );

	GtkTreeSelection *selection = gtk_tree_view_get_selection ( GTK_TREE_VIEW (archive[idx]->treeview) );
	gint selected = gtk_tree_selection_count_selected_rows ( selection );
    extract_window = xa_create_extract_dialog (selected , archive[idx]);
	if (archive[idx]->extraction_path != NULL)
		gtk_entry_set_text (GTK_ENTRY(extract_window->destination_path_entry),archive[idx]->extraction_path);
    command = xa_parse_extract_dialog_options ( archive[idx] , extract_window, selection );
	if (extract_window->dialog1 != NULL)
	{
		gtk_widget_destroy ( extract_window->dialog1 );
		extract_window->dialog1 = NULL;
	}

	if (command != NULL)
	{
		gtk_widget_set_sensitive ( check_menu , FALSE);
		gtk_widget_set_sensitive ( close1 , 	FALSE);
		gtk_widget_set_sensitive ( properties , FALSE);
		xa_set_button_state (0,0,0,0,0,0);
		xa_run_command (command , 1);
		g_free (command);
	}
	g_free (extract_window);
	extract_window = NULL;
}

void xa_convert_sfx ( GtkMenuItem *menuitem , gpointer user_data )
{
	gchar *command = NULL;
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
			archive_name_escaped = EscapeBadChars ( archive_name ,"$\'`\"\\!?* ()[]&|@#:;" );
			unzipsfx_path = g_find_program_in_path ( "unzipsfx" );
			if ( unzipsfx_path != NULL )
			{
				/* Load the unzipsfx executable in memory, about 50 KB */
				result = g_file_get_contents (unzipsfx_path,&content,&length,&error);
				if ( ! result)
				{
					Update_StatusBar (_("Operation failed."));
					gtk_widget_set_sensitive (Stop_button,FALSE);
					response = ShowGtkMessageDialog (GTK_WINDOW (MainWindow),GTK_DIALOG_MODAL,GTK_MESSAGE_ERROR,GTK_BUTTONS_OK,_("Can't convert the archive to self-extracting:"),error->message);
					g_error_free (error);
					g_free (unzipsfx_path);
					return;
				}
				g_free (unzipsfx_path);

				/* Write unzipsfx to a new file */
				sfx_archive = g_fopen ( archive_name ,"w" );
				if (sfx_archive == NULL)
				{
					Update_StatusBar (_("Operation failed."));
					gtk_widget_set_sensitive (Stop_button,FALSE);
					response = ShowGtkMessageDialog (GTK_WINDOW (MainWindow),GTK_DIALOG_MODAL,GTK_MESSAGE_ERROR,GTK_BUTTONS_OK,_("Can't write the unzipsfx module to the archive:"),g_strerror(errno) );
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
				result = xa_run_command (command , 0);
				g_free (command);

				command = g_strconcat ("zip -A ",archive_name_escaped,NULL);
				result = xa_run_command (command , 1);
				g_free (command);
				command = NULL;
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
			archive_name_escaped = EscapeBadChars ( archive_name ,"$\'`\"\\!?* ()[]&|@#:;" );

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
						GTK_WINDOW (MainWindow),
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
					xa_hide_progress_bar_stop_button (archive[idx]);
					return;
				}
			}
			if ( sfx_path != NULL )
			{
				/* Load the 7zCon.sfx executable in memory ~ 500 KB; is it too much for 128 MB equipped PCs ? */
				result = g_file_get_contents (sfx_path,&content,&length,&error);
				if ( ! result)
				{
					Update_StatusBar (_("Operation failed."));
					xa_hide_progress_bar_stop_button (archive[idx]);
					response = ShowGtkMessageDialog (GTK_WINDOW (MainWindow),GTK_DIALOG_MODAL,GTK_MESSAGE_ERROR,GTK_BUTTONS_OK,_("Can't convert the archive to self-extracting:"),error->message);
					g_error_free (error);
					g_free (sfx_path);
					return;
				}
				g_free (sfx_path);

				/* Write 7zCon.sfx to a new file */
				sfx_archive = g_fopen ( archive_name ,"w" );
				if (sfx_archive == NULL)
				{
					Update_StatusBar (_("Operation failed."));
					xa_hide_progress_bar_stop_button (archive[idx]);
					response = ShowGtkMessageDialog (GTK_WINDOW (MainWindow),GTK_DIALOG_MODAL,GTK_MESSAGE_ERROR,GTK_BUTTONS_OK,_("Can't write the unzipsfx module to the archive:"),g_strerror(errno) );
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
				result = xa_run_command (command , 1);
				g_free (command);
				command = NULL;
			}
			g_free (archive_name);
			g_free (archive_name_escaped);
        }
		break;

		case XARCHIVETYPE_ARJ:
        command = g_strconcat ("arj y -je1 " , archive[idx]->escaped_path, NULL);
		break;

		default:
		command = NULL;
	}
	if (command != NULL)
	{
		xa_run_command ( command , 1);
		g_free (command);
	}
}

void xa_about (GtkMenuItem *menuitem, gpointer user_data)
{
    static GtkWidget *about = NULL;
    const char *authors[] = {"\nMain developer: Giuseppe Torelli <colossus73@gmail.com>\nISO support: Salvatore Santagati <salvatore.santagati@gmail.com>\nLHA and DEB support: Łukasz Zemczak <sil2100@vexillium.org>",NULL};
    const char *documenters[] = {"\nSpecial thanks to Bjoern Martensen for discovering\nmany bugs in the Xarchiver development code.\n\nThanks to:\nBenedikt Meurer\nStephan Arts\nEnrico Tröger\nUracile for the stunning logo\nThe people of gtk-app-devel-list.", NULL};

	if (about == NULL)
	{
		about = gtk_about_dialog_new ();
		gtk_about_dialog_set_email_hook (xa_activate_link, NULL, NULL);
		gtk_about_dialog_set_url_hook (xa_activate_link, NULL, NULL);
		gtk_window_set_destroy_with_parent (GTK_WINDOW (about) , TRUE);
		g_object_set (about,
				"name",  "Xarchiver",
				"version", PACKAGE_VERSION,
				"copyright", "Copyright \xC2\xA9 2005-2006 Giuseppe Torelli",
				"comments", "A lightweight GTK+2 archive manager",
				"authors", authors,
				"documenters",documenters,
				"translator_credits", _("translator-credits"),
				"logo_icon_name", "xarchiver",
				"website", "http://xarchiver.xfce.org",
				"license",    "Copyright \xC2\xA9 2005-2006 Giuseppe Torelli - Colossus <colossus73@gmail.com>\n\n"
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
						GTK_WINDOW (MainWindow),
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
						GTK_WINDOW (MainWindow),
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
			if ( Suffix->data != "" )	/* To avoid double filtering when opening the archive */
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

gboolean isISO ( FILE *ptr )
{
	int offset_image;
	if ((offset_image = DetectImage(ptr)) > 0 )
	{
		fseek ( ptr , offset_image, SEEK_SET );
		fread ( &ipd, 1, sizeof(ipd), ptr );
		system_id = g_strndup ( ipd.system_id, 30);
		volume_id = g_strndup ( ipd.volume_id, 30);
		application_id = g_strndup ( ipd.application_id, 126);
		publisher_id = g_strndup ( ipd.publisher_id, 126);
		preparer_id = g_strndup ( ipd.preparer_id, 126);

		creation_date = g_strdup_printf ("%4.4s %2.2s %2.2s %2.2s:%2.2s:%2.2s.%2.2s",&ipd.creation_date[0],&ipd.creation_date[4],&ipd.creation_date[6],&ipd.creation_date[8],&ipd.creation_date[10],&ipd.creation_date[12],&ipd.creation_date[14]);

		modified_date = g_strdup_printf ("%4.4s %2.2s %2.2s %2.2s:%2.2s:%2.2s.%2.2s",&ipd.modification_date[0],&ipd.modification_date[4],&ipd.modification_date[6],&ipd.modification_date[8],&ipd.modification_date[10],&ipd.modification_date[12],&ipd.modification_date[14]);

		expiration_date = g_strdup_printf ("%4.4s %2.2s %2.2s %2.2s:%2.2s:%2.2s.%2.2s",&ipd.expiration_date[0],&ipd.expiration_date[4],&ipd.expiration_date[6],&ipd.expiration_date[8],&ipd.expiration_date[10],&ipd.expiration_date[12],&ipd.expiration_date[14]);

		effective_date = g_strdup_printf ("%4.4s %2.2s %2.2s %2.2s:%2.2s:%2.2s.%2.2s",&ipd.effective_date[0],&ipd.effective_date[4],&ipd.effective_date[6],&ipd.effective_date[8],&ipd.effective_date[10],&ipd.effective_date[12],&ipd.effective_date[14]);
        return TRUE;
	}
    else
		return FALSE;
}

gboolean isTar ( FILE *ptr )
{
	unsigned char magic[7];
	fseek ( ptr, 0 , SEEK_SET );
    if ( fseek ( ptr , 257 , SEEK_CUR) < 0 )
		return FALSE;
    if ( fread ( magic, 1, 7, ptr ) == 0 )
		return FALSE;
    if ( memcmp ( magic,"\x75\x73\x74\x61\x72\x00\x30",7 ) == 0 || memcmp (magic,"\x75\x73\x74\x61\x72\x20\x20",7 ) == 0)
		return TRUE;
    else
		return FALSE;
}

gboolean isLha ( FILE *ptr )
{
	unsigned char magic[2];
	fseek(ptr, 0, SEEK_SET);
	if(fseek(ptr, 19, SEEK_CUR) < 0)
		return FALSE;
	if(fread(magic, 1, 2, ptr) == 0)
		return FALSE;

	if(magic[0] == 0x20 && magic[1] <= 0x03)
	{
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}

int xa_detect_archive_type ( XArchive *archive , gchar *filename )
{
	FILE *dummy_ptr = NULL;
    int xx = -1;
	unsigned char magic[12];
	if (filename != NULL)
		dummy_ptr = fopen ( filename , "r" );
	else
		dummy_ptr = fopen ( archive->path , "r" );

	if (dummy_ptr == NULL)
	{
		if ( !cli )
		{
			gchar *utf8_path,*msg;
			utf8_path = g_filename_to_utf8 (archive->path, -1, NULL, NULL, NULL);
			msg = g_strdup_printf (_("Can't open archive \"%s\":") , utf8_path );
			response = ShowGtkMessageDialog (GTK_WINDOW (MainWindow) , GTK_DIALOG_MODAL,GTK_MESSAGE_ERROR,GTK_BUTTONS_OK,
			msg,g_strerror (errno));
			g_free (msg);
			g_free (utf8_path);
			return -2;
		}
		else
			return -2;
	}
	if ( fread ( magic, 1, 12, dummy_ptr ) == 0 )
	{
		fclose ( dummy_ptr);
		return -2;
	}

	if ( memcmp ( magic,"\x50\x4b\x03\x04",4 ) == 0 || memcmp ( magic,"\x50\x4b\x05\x06",4 ) == 0 )
	{
		if (archive != NULL)
		{
			archive->has_passwd = xa_detect_encrypted_archive ( XARCHIVETYPE_ZIP , dummy_ptr , magic );
			archive->has_comment = xa_detect_archive_comment ( XARCHIVETYPE_ZIP , dummy_ptr, archive );
		}
		xx = XARCHIVETYPE_ZIP;
	}
	else if ( memcmp ( magic,"\x60\xea",2 ) == 0 )
	{
		if (archive != NULL)
		{
			archive->has_passwd = xa_detect_encrypted_archive ( XARCHIVETYPE_ARJ , dummy_ptr, magic );
			archive->has_comment = xa_detect_archive_comment ( XARCHIVETYPE_ARJ , dummy_ptr, archive );
		}
		xx = XARCHIVETYPE_ARJ;
	}
	else if ( memcmp ( magic,"\x52\x61\x72\x21",4 ) == 0 )
	{
		if (archive != NULL)
			archive->has_comment = xa_detect_archive_comment ( XARCHIVETYPE_RAR , dummy_ptr, archive );
		 xx = XARCHIVETYPE_RAR;
	}
	else if ( memcmp ( magic,"\x42\x5a\x68",3 ) == 0 ) xx = XARCHIVETYPE_BZIP2;
	else if ( memcmp ( magic,"\x1f\x8b",2) == 0 || memcmp ( magic,"\x1f\x9d",2 ) == 0 )  xx = XARCHIVETYPE_GZIP;
	else if ( memcmp ( magic,"\xed\xab\xee\xdb",4 ) == 0) xx = XARCHIVETYPE_RPM;
	else if ( memcmp ( magic,"\x37\x7a\xbc\xaf\x27\x1c",6 ) == 0 ) xx = XARCHIVETYPE_7ZIP;
	else if ( isTar ( dummy_ptr ) ) xx = XARCHIVETYPE_TAR;
	else if ( isISO ( dummy_ptr ) ) xx = XARCHIVETYPE_ISO;
	else if ( isLha ( dummy_ptr ) ) xx = XARCHIVETYPE_LHA;
	else if ( memcmp ( magic,"!<arch>\n", 8 ) == 0) xx = XARCHIVETYPE_DEB;
	//else if ( memcmp (magic,"\x00\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\x00",12) == 0 ) xx = XARCHIVETYPE_BIN;
	fclose ( dummy_ptr );
	return xx;
}

gboolean xa_detect_archive_comment ( int type, FILE *stream, XArchive *archive )
{
	char sig;
	guint cmt_len = 0;
	unsigned char eocds[] = { 0x50, 0x4b, 0x05, 0x06 };
	int byte;
	unsigned long long int eocds_position = 0;

	unsigned short int len = 0;
	int eof;
	size_t seqptr = 0;

	if (type == XARCHIVETYPE_ZIP)
	{
		/* Let's position the file indicator to 64KB before the end of the archive */
		fseek(stream, 0L, SEEK_END);
        eocds_position = ftell(stream);
        fseek(stream, eocds_position - 200, SEEK_SET);
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
				g_string_append (archive->comment,&sig);
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
	else if (type == XARCHIVETYPE_RAR)
	{
	}
	return FALSE;
}

gboolean xa_detect_encrypted_archive ( int type, FILE *stream, unsigned char magic[6] )
{
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

	fseek ( stream, 6 , SEEK_SET );
	if ( type == XARCHIVETYPE_ZIP )
	{
		while ( memcmp ( magic,"\x50\x4b\x03\x04",4 ) == 0  || memcmp ( magic,"\x50\x4b\x05\x06",4 ) == 0 )
		{
			fread ( &password_flag, 1, 2, stream );
			if (( password_flag & ( 1<<0) ) > 0)
				return TRUE;
			fseek (stream,10,SEEK_CUR);
			fread (&compressed_size,1,4,stream);
			fread (&uncompressed_size,1,4,stream);
			fread (&file_length,1,2,stream);
			/* If the zip archive is empty (no files) it should return here */
			if (fread (&extra_length,1,2,stream) < 2 )
				return FALSE;
			fseek_offset = compressed_size + file_length + extra_length;
			fseek (stream , fseek_offset , SEEK_CUR);
			fread (magic , 1 , 4 , stream);
			fseek ( stream , 2 , SEEK_CUR);
		}
	}
	else if ( type == XARCHIVETYPE_ARJ)
	{
		fseek (stream , magic[2]+magic[3] , SEEK_CUR);
		fseek (stream , 2 , SEEK_CUR);
		fread (&extended_header_size,1,2,stream);
		if (extended_header_size != 0)
			fread (&extended_header_CRC,1,4,stream);
		fread (&sig,1,2,stream);
		while ( memcmp (sig,"\x60\xea",2) == 0)
		{
			fread ( &basic_header_size , 1 , 2 , stream );
			if ( basic_header_size == 0 )
				break;
			fseek ( stream , 4 , SEEK_CUR);
			fread (&arj_flag,1,1,stream);
			if ((arj_flag & ( 1<<0) ) > 0)
				return TRUE;
			fseek ( stream , 7 , SEEK_CUR);
			fread (&compressed_size,1,4,stream);
			fseek ( stream , basic_header_size - 16 , SEEK_CUR);
			fread (&basic_header_CRC,1,4,stream);
			fread (&extended_header_size,1,2,stream);
			if (extended_header_size != 0)
				fread (&extended_header_CRC,1,4,stream);
			fseek ( stream , compressed_size , SEEK_CUR);
			fread (&sig,1,2,stream);
		}
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

void EmptyTextBuffer ()
{
	if (textbuf != NULL)
	{
		gtk_text_buffer_get_start_iter (textbuf,&start);
		gtk_text_buffer_get_end_iter (textbuf,&end);
		gtk_text_buffer_delete (textbuf,&start,&end);
		gtk_text_buffer_get_start_iter(textbuf, &enditer);
	}
}

void xa_create_liststore ( unsigned short int nc, gchar *columns_names[] , GType columns_types[], XArchive *archive)
{
	unsigned short int x;
	GtkCellRenderer *renderer;
	GtkTreeViewColumn *column;

	archive->liststore = gtk_list_store_newv ( nc , (GType *)columns_types);
	gtk_tree_view_set_model ( GTK_TREE_VIEW (archive->treeview), GTK_TREE_MODEL (archive->liststore) );

	archive->model = gtk_tree_view_get_model(GTK_TREE_VIEW(archive->treeview));
	g_object_ref(archive->model);
	gtk_tree_view_set_model(GTK_TREE_VIEW(archive->treeview), NULL);

	for (x = 0; x <= nc-1; x++)
	{
		renderer = gtk_cell_renderer_text_new ();
		column = gtk_tree_view_column_new_with_attributes ( columns_names[x],renderer,"text",x,NULL);
		gtk_tree_view_column_set_resizable (column, TRUE);
		gtk_tree_view_column_set_sort_column_id (column, x);
		gtk_tree_view_append_column (GTK_TREE_VIEW (archive->treeview), column);
	}
}

gboolean treeview_select_search (GtkTreeModel *model,gint column,const gchar *key,GtkTreeIter *iter,gpointer search_data)
{
    char *filename;
    gboolean result;

    gtk_tree_model_get (model, iter, 0, &filename, -1);
    if ( strcasestr (filename, key) ) result = FALSE;
        else result = TRUE;
    g_free (filename);
    return result;
}

void xa_show_cmd_line_output( GtkMenuItem *menuitem )
{
	if (OutputWindow != NULL)
	{
		gtk_window_set_title (GTK_WINDOW (OutputWindow), _("Command line output") );
		gtk_window_present ( GTK_WINDOW (OutputWindow) );
		return;
	}
	OutputWindow = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	gtk_window_set_position (GTK_WINDOW (OutputWindow), GTK_WIN_POS_CENTER);
	gtk_window_set_default_size(GTK_WINDOW(OutputWindow), 380, 250);
	gtk_window_set_destroy_with_parent (GTK_WINDOW (OutputWindow), TRUE);
	g_signal_connect (G_OBJECT (OutputWindow), "delete-event",  G_CALLBACK (gtk_widget_hide), &OutputWindow);

	vbox = gtk_vbox_new ( FALSE, 2 );
	scrollwin = gtk_scrolled_window_new ( NULL,NULL );
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW( scrollwin ), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

	textview = gtk_text_view_new();
	gtk_text_view_set_editable (GTK_TEXT_VIEW(textview), FALSE);
	gtk_container_add (GTK_CONTAINER(scrollwin), textview);
	gtk_box_pack_start (GTK_BOX(vbox), scrollwin, TRUE, TRUE, 0);
	gtk_container_add (GTK_CONTAINER(OutputWindow), vbox);
	textbuf = gtk_text_view_get_buffer ( GTK_TEXT_VIEW(textview) );
	gtk_text_buffer_get_start_iter (textbuf, &enditer);
	//gtk_text_buffer_create_tag (textbuf, "red_foreground","foreground", "red", NULL);

	gtk_widget_show (vbox);
	gtk_widget_show (scrollwin);
	gtk_widget_show (textview);
}

void xa_cancel_archive ( GtkMenuItem *menuitem , gpointer data )
{
	gint current_page;
	gint idx;

	current_page = gtk_notebook_get_current_page(notebook);
	idx = xa_find_archive_index (current_page);

	if (archive[idx]->status == XA_ARCHIVESTATUS_ADD || archive[idx]->status == XA_ARCHIVESTATUS_SFX)
	{
		response = ShowGtkMessageDialog (GTK_WINDOW	(MainWindow),GTK_DIALOG_MODAL,GTK_MESSAGE_QUESTION,GTK_BUTTONS_YES_NO,_("Doing so will probably corrupt your archive!"),_("Do you really want to cancel?") );
		if (response == GTK_RESPONSE_NO)
			return;
	}
	xa_hide_progress_bar_stop_button (archive[idx]);
    Update_StatusBar (_("Waiting for the process to abort..."));
	stop_flag = TRUE;
	if (archive[idx]->type != XARCHIVETYPE_ISO)
	{
		if ( kill ( archive[idx]->child_pid , SIGABRT ) < 0 )
	    {
		    response = ShowGtkMessageDialog (GTK_WINDOW (MainWindow),GTK_DIALOG_MODAL,GTK_MESSAGE_ERROR,GTK_BUTTONS_OK,_("An error occurred while trying to kill the process:"),g_strerror(errno));
			return;
	    }
	}
    /* This in case the user cancels the opening of a password protected archive */
    if (archive[idx]->status != XA_ARCHIVESTATUS_ADD || archive[idx]->status != XA_ARCHIVESTATUS_DELETE)
		if (archive[idx]->has_passwd)
			archive[idx]->has_passwd = FALSE;
}

void xa_view_file_inside_archive ( GtkMenuItem *menuitem , gpointer user_data )
{
	gchar *filename = NULL;
	GError *error = NULL;
	gchar *string = NULL;
	gchar *command = NULL;
	GtkTreeSelection *selection;
	GtkTreeModel *model;
	GtkTreeIter iter;
	gchar *dir;
	gchar *dummy_name;
	unsigned short int COL_NAME;
	gboolean is_dir = FALSE;
	gboolean tofree = FALSE;
	gboolean result = FALSE;
	GList *row_list = NULL;
	GString *names;
	gchar *content;
	gsize length;
	gsize new_length;
	gchar *t;
	gint current_page;
	gint idx;

	current_page = gtk_notebook_get_current_page (notebook);
	idx = xa_find_archive_index (current_page);

	if ( archive[idx]->has_passwd )
	{
		if ( archive[idx]->passwd == NULL)
		{
			archive[idx]->passwd = password_dialog ();
			if ( archive[idx]->passwd == NULL)
				return;
		}
	}
	selection = gtk_tree_view_get_selection ( GTK_TREE_VIEW (archive[idx]->treeview) );

	/* if no or more than one rows selected, do nothing, just for sanity */
	if ( gtk_tree_selection_count_selected_rows (selection) != 1)
		return;

	row_list = gtk_tree_selection_get_selected_rows (selection, &model);
	if ( row_list == NULL )
		return;

	gtk_tree_model_get_iter(model, &iter, row_list->data);

	gtk_tree_path_free(row_list->data);
	g_list_free (row_list);

	switch (archive[idx]->type)
	{
		case XARCHIVETYPE_RAR:
		case XARCHIVETYPE_ARJ:
		COL_NAME = 6;
		break;

		case XARCHIVETYPE_ZIP:
		COL_NAME = 0;
		break;

		case XARCHIVETYPE_7ZIP:
		COL_NAME = 3;
		break;

		default:
		COL_NAME = 1;
	}
	gtk_tree_model_get (model, &iter, COL_NAME, &dir, -1);
	if (archive[idx]->type == XARCHIVETYPE_ZIP)
	{
		if ( g_str_has_suffix (dir,"/") == TRUE )
			is_dir = TRUE;
	}
	else if ( strstr ( dir , "d" ) || strstr ( dir , "D" ) ) is_dir = TRUE;
	if (is_dir)
	{
		response = ShowGtkMessageDialog (GTK_WINDOW (MainWindow),GTK_DIALOG_MODAL,GTK_MESSAGE_ERROR, GTK_BUTTONS_OK,"Can't perform the action:",_("Please select a file, not a directory!") );
		g_free ( dir );
		return;
	}
	g_free ( dir );

	full_path = archive[idx]->full_path;
	overwrite = archive[idx]->overwrite;

	archive[idx]->full_path = 0;
	archive[idx]->overwrite = 1;

	names = g_string_new (" ");
	gtk_tree_model_get (model, &iter, 0, &dummy_name, -1);
	archive[idx]->status = XA_ARCHIVESTATUS_EXTRACT;
	ConcatenateFileNames2 ( dummy_name , names );

	if (archive[idx]->type == XARCHIVETYPE_ISO)
	{
		gtk_tree_model_get (model, &iter,
			0, &dummy_name,
			1, &permissions,
			2, &file_size,
			4, &file_offset,
			-1);
		xa_extract_iso_file (archive[idx], permissions, "/tmp/", dummy_name , file_size, file_offset );
		g_free (permissions);
	}
	else
		command = xa_extract_single_files ( archive[idx] , names, "/tmp");

	archive[idx]->full_path = full_path;
	archive[idx]->overwrite = overwrite;
	EmptyTextBuffer();
	if (command != NULL)
	{
		result = xa_run_command (command , 0);
		g_free (command);
		if ( result == 0 )
		{
			unlink (dummy_name);
			g_free (dummy_name);
			g_string_free (names,TRUE);
			return;
		}
	}
	view_window = view_win( names->str );
	g_string_free (names,TRUE);
	string = g_strrstr ( dummy_name, "/" );
	if (  string == NULL )
		filename = g_strconcat ( "/tmp/" , dummy_name, NULL );
	else
	{
		if ( strchr ( string , ' ' ) )
		{
			string = RemoveBackSlashes ( string );
			tofree = TRUE;
		}
		filename = g_strconcat ( "/tmp" , string , NULL );
		if ( tofree )
			g_free ( string );
	}
	g_free (dummy_name);

	result = g_file_get_contents (filename,&content,&length,&error);
	if ( ! result)
	{
		gtk_widget_hide (viewport2);
		unlink ( filename );
		Update_StatusBar ( _("Operation failed."));
		response = ShowGtkMessageDialog (GTK_WINDOW (MainWindow),GTK_DIALOG_MODAL,GTK_MESSAGE_ERROR,GTK_BUTTONS_OK,_("An error occurred while extracting the file to be viewed:") , error->message);
		g_error_free (error);
		g_free (filename);
		return;
	}
	t = g_locale_to_utf8 ( content, length, NULL, &new_length, &error);
	g_free ( content );
	if ( t == NULL)
	{
		response = ShowGtkMessageDialog (GTK_WINDOW (MainWindow),GTK_DIALOG_MODAL,GTK_MESSAGE_ERROR,GTK_BUTTONS_OK,_("An error occurred while converting the file content to the UTF8 encoding:") , error->message);
		g_free (error);
	}
	else
	{
		gtk_widget_show (view_window);
		gtk_text_buffer_insert (viewtextbuf, &viewenditer, t, new_length );
	}
	unlink ( filename );
	g_free (filename);
	xa_hide_progress_bar_stop_button(archive[idx]);
	Update_StatusBar (_("Operation completed."));
}

void xa_iso_properties ( GtkMenuItem *menuitem , gpointer user_data )
{
	gchar *utf8_string, *text, *measure;
	unsigned long long int file_size;
	GtkWidget *iso_properties_win;
	gint current_page;
	gint idx;

	current_page = gtk_notebook_get_current_page (notebook);
	idx = xa_find_archive_index (current_page);

	stat ( archive[idx]->path , &my_stat );
	file_size = my_stat.st_size;
	iso_properties_win = create_iso_properties_window();
	//Name
	text = g_strrstr ( archive[idx]->path, "/" );
	if (text != NULL)
	{
	    text++; //This to avoid the / char in the string
	    utf8_string = g_filename_display_name (text);
	}
	else
		utf8_string = g_filename_display_name (archive[idx]->path);
	gtk_entry_set_text ( GTK_ENTRY (filename_entry), utf8_string );
	g_free (utf8_string);
    //Size
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
		measure = _(" bytes");
		content_size = file_size;
	}

    text = g_strdup_printf ("%.1f %s", content_size,measure);
    gtk_entry_set_text ( GTK_ENTRY (size_entry), text );
    g_free (text);

	gtk_entry_set_text ( GTK_ENTRY (image_type_entry),archive[idx]->tmp);

	gtk_entry_set_text ( GTK_ENTRY (system_id_entry),system_id);

	gtk_entry_set_text ( GTK_ENTRY (volume_id_entry),volume_id);

	gtk_entry_set_text ( GTK_ENTRY (application_entry),application_id);

	gtk_entry_set_text ( GTK_ENTRY (publisher_entry),publisher_id);
	gtk_widget_show (iso_properties_win);

	gtk_entry_set_text ( GTK_ENTRY (preparer_entry),preparer_id);
	gtk_widget_show (iso_properties_win);

	gtk_entry_set_text ( GTK_ENTRY (creation_date_entry),creation_date);

	gtk_entry_set_text ( GTK_ENTRY (modified_date_entry),modified_date);
	gtk_widget_show (iso_properties_win);

	gtk_entry_set_text ( GTK_ENTRY (expiration_date_entry),expiration_date);

	gtk_entry_set_text ( GTK_ENTRY (effective_date_entry),effective_date);
	gtk_widget_show (iso_properties_win);
}

void xa_archive_properties ( GtkMenuItem *menuitem , gpointer user_data )
{
    gchar *utf8_string , *measure, *text, *dummy_string;
    char date[64];
    gchar *t;
    unsigned long long int file_size;
    gint current_page;
	gint idx;

    current_page = gtk_notebook_get_current_page (notebook);
	idx = xa_find_archive_index (current_page);

    stat ( archive[idx]->path , &my_stat );
    file_size = my_stat.st_size;
    archive_properties_win = create_archive_properties_window();
    //Name
    text = g_strrstr ( archive[idx]->path, "/" );
    if (text != NULL)
    {
        text++;
        utf8_string = g_filename_display_name (text);
    }
    else
		utf8_string = g_filename_display_name (archive[idx]->path);
    gtk_entry_set_text ( GTK_ENTRY (name_data), utf8_string );
    g_free (utf8_string);
    //Path
    dummy_string = remove_level_from_path (archive[idx]->path);
    if ( strlen(dummy_string) != 0)
		utf8_string = g_filename_display_name (dummy_string);
    else
		utf8_string = g_filename_display_name ( g_get_current_dir () );
    gtk_entry_set_text ( GTK_ENTRY (path_data), utf8_string );
    g_free ( utf8_string );
    g_free ( dummy_string );
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
    gtk_widget_show_all ( archive_properties_win );
}

void xa_activate_delete_and_view ()
{
	gint current_page;
	gint idx;

	current_page = gtk_notebook_get_current_page (notebook);
	idx = xa_find_archive_index (current_page);

	if ( ! GTK_WIDGET_VISIBLE (Extract_button) )
		return;

	GtkTreeSelection *selection = gtk_tree_view_get_selection ( GTK_TREE_VIEW (archive[idx]->treeview) );
	gint selected = gtk_tree_selection_count_selected_rows ( selection );
	if (selected == 0 )
		OffDeleteandViewButtons();
	else
	{
		if ( archive[idx]->type != XARCHIVETYPE_RPM && archive[idx]->type != XARCHIVETYPE_ISO && archive[idx]->type != XARCHIVETYPE_DEB)
		{
			gtk_widget_set_sensitive ( delete_menu , TRUE );
			gtk_widget_set_sensitive ( Delete_button , TRUE );
		}
		if (selected > 1 )
		{
			gtk_widget_set_sensitive ( View_button , FALSE);
			gtk_widget_set_sensitive ( view_menu, FALSE );
		}
		else
		{
			gtk_widget_set_sensitive ( View_button , TRUE );
			gtk_widget_set_sensitive ( view_menu, TRUE );
		}
	}
}

void ConcatenateFileNames2 (gchar *filename , GString *data)
{
	gchar *esc_filename = NULL;
	gchar *escaped = NULL;
	gchar *escaped2 = NULL;
	gint current_page;
	gint idx;

	current_page = gtk_notebook_get_current_page (notebook);
	idx = xa_find_archive_index (current_page);

	if ( strstr (filename, "[") || strstr (filename, "]"))
	{
		if (archive[idx]->type == XARCHIVETYPE_ZIP)
		{
			if (archive[idx]->status == XA_ARCHIVESTATUS_ADD)
			{
				esc_filename = EscapeBadChars ( filename ,"$\'`\"\\!?* ()[]&|@#:;" );
				g_string_prepend (data, esc_filename);
			}
			else
			{
				escaped = EscapeBadChars ( filename ,"$\'`\"\\!?* ()[]&|@#:;");
				escaped2 = escape_str_common (escaped , "*?[]", '\\', 0);
				g_free (escaped);
				esc_filename = escaped2;
				g_string_prepend (data, esc_filename);
			}
		}
		else if ( archive[idx]->type == XARCHIVETYPE_TAR_BZ2 || archive[idx]->type == XARCHIVETYPE_TAR_GZ || archive[idx]->type == XARCHIVETYPE_TAR )
		{
			if (archive[idx]->status == XA_ARCHIVESTATUS_ADD)
			{
				esc_filename = EscapeBadChars ( filename ,"$\'`\"\\!?* ()[]&|@#:;" );
				g_string_prepend (data, esc_filename);
			}
			else
			{
				escaped = EscapeBadChars ( filename ,"?*\\'& !|()@#:;");
				escaped2 = escape_str_common ( escaped , "[]", '[', ']');
				g_free (escaped);
				esc_filename = escaped2;
				g_string_prepend (data, esc_filename);
			}
		}
	}
	else
	{
		esc_filename = EscapeBadChars ( filename , "$\'`\"\\!?* ()[]&|@#:;" );
		g_string_prepend (data, esc_filename);
	}
	g_string_prepend_c (data, ' ');
	g_free (esc_filename);
}

void ConcatenateFileNames (GtkTreeModel *model, GtkTreePath *treepath, GtkTreeIter *iter, GString *data)
{
	gchar *filename = NULL;

	gtk_tree_model_get (model, iter, 0, &filename, -1);
	ConcatenateFileNames2 ( filename , data );
	g_free (filename);
}

void xa_cat_filenames_basename (GtkTreeModel *model, GtkTreePath *treepath, GtkTreeIter *iter, GString *data)
{
	gchar *fullname;
	gchar *name;

	gtk_tree_model_get (model, iter, 1, &fullname, -1);
	name = g_path_get_basename ( fullname );

	ConcatenateFileNames2 ( name , data );
	g_free (fullname);
}

void xa_cat_filenames (GtkTreeModel *model, GtkTreePath *treepath, GtkTreeIter *iter, GString *data)
{
	gchar *fullname;

	gtk_tree_model_get (model, iter, 1, &fullname, -1);
	ConcatenateFileNames2 ( fullname , data );
	g_free (fullname);
}

gboolean xa_run_command ( gchar *command , gboolean watch_child_flag )
{
	gint current_page;
	gint idx;
	int status;
	gboolean waiting = TRUE;
	int ps;

	current_page = gtk_notebook_get_current_page (notebook);
	idx = xa_find_archive_index ( current_page );

	if (watch_child_flag)
		EmptyTextBuffer();
	archive[idx]->parse_output = 0;
	SpawnAsyncProcess ( archive[idx] , command , 0, 1);
	if ( archive[idx]->child_pid == 0 )
		return FALSE;

	gtk_widget_show (viewport2);
	while (waiting)
	{
		ps = waitpid ( archive[idx]->child_pid, &status, WNOHANG);
		if (ps < 0)
			waiting = FALSE;
		else
			gtk_main_iteration_do (FALSE);
	}
	if (watch_child_flag)
	{
		xa_watch_child (archive[idx]->child_pid, status, archive[idx]);
		return TRUE;
	}
	else
	{
		if ( WIFEXITED (status) )
		{
			if ( WEXITSTATUS (status) )
			{
				gtk_tooltips_disable ( pad_tooltip );
				gtk_widget_hide ( pad_image );
				gtk_widget_hide ( viewport2 );
				xa_set_window_title (MainWindow , NULL);
				response = ShowGtkMessageDialog (GTK_WINDOW	(MainWindow),GTK_DIALOG_MODAL,GTK_MESSAGE_QUESTION,GTK_BUTTONS_YES_NO,_("An error occurred while accessing the archive."),_("Do you want to view the command line output?") );
				if (response == GTK_RESPONSE_YES)
					xa_show_cmd_line_output (NULL);
				archive[idx]->status = XA_ARCHIVESTATUS_IDLE;
				gtk_widget_set_sensitive (Stop_button,FALSE);
				Update_StatusBar ( _("Operation failed."));
				return FALSE;
			}
		}
	}
	return TRUE;
}

void Update_StatusBar ( gchar *msg)
{
    gtk_label_set_text (GTK_LABEL (info_label), msg);
}

gboolean xa_report_child_stderr (GIOChannel *ioc, GIOCondition cond, gpointer data)
{
	GIOStatus status;
	gchar     buffer[4096];
	gsize     bytes_read;

	if (cond & (G_IO_IN | G_IO_PRI))
	{
		do
		{
			status = g_io_channel_read_chars (ioc, buffer, sizeof (buffer), &bytes_read, NULL);
			if (bytes_read > 0)
				gtk_text_buffer_insert (textbuf, &enditer, buffer, bytes_read);
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

void OffDeleteandViewButtons()
{
    gtk_widget_set_sensitive ( Delete_button, FALSE);
    gtk_widget_set_sensitive ( delete_menu, FALSE);
    gtk_widget_set_sensitive ( View_button, FALSE);
    gtk_widget_set_sensitive ( view_menu, FALSE);
}

void xa_hide_progress_bar_stop_button( XArchive *archive)
{
	archive->status = XA_ARCHIVESTATUS_IDLE;
    gtk_widget_set_sensitive ( Stop_button , FALSE );
    if (archive->pb_source != 0)
		g_source_remove (archive->pb_source);
    archive->pb_source = 0;
    gtk_widget_hide (viewport2);
}

void drag_begin (GtkWidget *treeview1,GdkDragContext *context, gpointer data)
{
    GtkTreeSelection *selection;
    GtkTreeIter       iter;
    gchar            *name;
    GList            *row_list;
	gint current_page;

	//gtk_drag_source_set_icon_name (treeview1, DATADIR "/pixmaps/xarchiver.png" );
	current_page = gtk_notebook_get_current_page(notebook);
    selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (archive[current_page]->treeview));

	row_list = gtk_tree_selection_get_selected_rows (selection, NULL);
	if ( row_list == NULL )
		return;

	gtk_tree_model_get_iter (archive[current_page]->model, &iter, (GtkTreePath*) (row_list->data) );
	gtk_tree_model_get (archive[current_page]->model, &iter, 0, &name, -1);
	gchar *no_slashes = g_strrstr ( name, "/" );
	if (no_slashes != NULL)
		no_slashes++;
	gdk_property_change (context->source_window,
		               gdk_atom_intern ("XdndDirectSave0", FALSE),
			           gdk_atom_intern ("text/plain", FALSE), 8,
				       GDK_PROP_MODE_REPLACE,
					   (const guchar *) no_slashes != NULL ? no_slashes : name, no_slashes != NULL ? strlen (no_slashes) : strlen (name) );

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
	GtkTreeSelection *selection;
	guchar *fm_path;
	int fm_path_len;
	gchar *command , *no_uri_path;
	gchar *to_send = "E";
	GList *row_list;
	GString *names;
	gint current_page;
	gint idx;

	current_page = gtk_notebook_get_current_page(notebook);
	idx = xa_find_archive_index (current_page);

    selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (archive[idx]->treeview));
	row_list = gtk_tree_selection_get_selected_rows (selection, NULL);

	if ( row_list == NULL)
		return;
	if ( archive[idx]->status == XA_ARCHIVESTATUS_EXTRACT )
	{
		response = ShowGtkMessageDialog (GTK_WINDOW (MainWindow),GTK_DIALOG_MODAL,GTK_MESSAGE_ERROR,GTK_BUTTONS_OK,_("Can't perform another extraction:"),_("Please wait until the completion of the current one!") );
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
		if ( archive[idx]->has_passwd )
		{
			if ( archive[idx]->passwd == NULL)
			{
				archive[idx]->passwd = password_dialog ();
				if ( archive[idx]->passwd == NULL)
				{
					gtk_drag_finish (dc, FALSE, FALSE, t);
					return;
				}
			}
		}
		archive[idx]->extraction_path = extract_local_path ( no_uri_path );
		g_free ( no_uri_path );
		if (archive[idx]->extraction_path != NULL)
			to_send = "S";

		names = g_string_new ("");
		gtk_tree_selection_selected_foreach (selection, (GtkTreeSelectionForeachFunc) ConcatenateFileNames, names );
		full_path = archive[idx]->full_path;
		overwrite = archive[idx]->overwrite;
		archive[idx]->full_path = 0;
		archive[idx]->overwrite = 1;
		command = xa_extract_single_files ( archive[idx] , names, archive[idx]->extraction_path );
		g_string_free (names, TRUE);
		if ( command != NULL )
		{
			archive[idx]->status = XA_ARCHIVESTATUS_EXTRACT;
			xa_run_command ( command , 1);
			g_free (command);
		}
		archive[idx]->full_path = full_path;
		archive[idx]->overwrite = overwrite;
		gtk_selection_data_set (selection_data, selection_data->target, 8, (guchar*)to_send, 1);
	}

	if (archive[idx]->extraction_path != NULL)
	{
		g_free (archive[idx]->extraction_path);
		archive[idx]->extraction_path = NULL;
	}
	g_list_foreach (row_list, (GFunc) gtk_tree_path_free, NULL);
	g_list_free (row_list);
	archive[idx]->status = XA_ARCHIVESTATUS_IDLE;
}

void on_drag_data_received (GtkWidget *widget,GdkDragContext *context, int x,int y,GtkSelectionData *data, unsigned int info, unsigned int time, gpointer user_data)
{
	gchar **array = NULL;
	gchar *filename = NULL;
	gchar *command = NULL;
	gchar *name = NULL;
	gchar *_current_dir = NULL;
	gchar *current_dir = NULL;
	gboolean one_file;
	gboolean dummy_password;
	unsigned int len = 0;
	gint current_page;
	gint idx;

	current_page = gtk_notebook_get_current_page (notebook);
	array = gtk_selection_data_get_uris ( data );
	if (array == NULL || GTK_WIDGET_VISIBLE (viewport2) )
	{
		response = ShowGtkMessageDialog (GTK_WINDOW (MainWindow),GTK_DIALOG_MODAL,GTK_MESSAGE_ERROR,GTK_BUTTONS_OK,_("Sorry, I could not perform the operation!"),"" );
		gtk_drag_finish (context, FALSE, FALSE, time);
		return;
	}
	gtk_drag_finish (context, TRUE, FALSE, time);
	one_file = (array[1] == NULL);

	if (one_file)
	{
		filename = g_filename_from_uri ( array[0] , NULL, NULL );
		if ( filename == NULL)
			return;
		else if ( xa_detect_archive_type ( NULL , filename ) > 0 )
		{
			xa_open_archive ( NULL, filename );
			g_strfreev ( array );
			return;
		}
    }
	if (current_page == -1)
	{
		idx = xa_get_new_archive_idx();
		if (idx == -1)
			return;
		archive[idx] = xa_new_archive_dialog ( filename );
		if (archive[idx] == NULL)
			return;
		xa_add_page (archive[idx]);
	}
	else
		idx = xa_find_archive_index ( current_page );

	if (archive[idx]->type == XARCHIVETYPE_RAR && unrar)
	{
		response = ShowGtkMessageDialog (GTK_WINDOW (MainWindow),GTK_DIALOG_MODAL,GTK_MESSAGE_ERROR,GTK_BUTTONS_OK,_("Can't perform this action:"),_("unrar doesn't support archive creation!") );
		return;
	}

	if (archive[idx]->type == XARCHIVETYPE_DEB || archive[idx]->type == XARCHIVETYPE_RPM)
	{
		gchar *msg;
		if (archive[idx]->type == XARCHIVETYPE_DEB)
			msg = _("You can't add content to deb packages!");
		else
			msg = _("You can't add content to rpm packages!");
		response = ShowGtkMessageDialog (GTK_WINDOW (MainWindow),GTK_DIALOG_MODAL,GTK_MESSAGE_ERROR,GTK_BUTTONS_OK,_("Can't perform this action:"), msg );
		return;
	}

	GString *names = g_string_new (" ");
	_current_dir = g_path_get_dirname ( array[0] );
	current_dir = g_filename_from_uri ( _current_dir, NULL, NULL );
	g_free (_current_dir);
	chdir ( current_dir );
	g_free (current_dir);
	archive[idx]->status = XA_ARCHIVESTATUS_ADD;

	while (array[len])
	{
		filename = g_filename_from_uri ( array[len] , NULL, NULL );
		name = g_path_get_basename ( filename );
		g_free (filename);
		ConcatenateFileNames2 ( name, names );
		g_free (name);
		len++;
	}
	dummy_password = archive[idx]->has_passwd;
	full_path = archive[idx]->full_path;
	add_recurse = archive[idx]->add_recurse;

	archive[idx]->has_passwd = 0;
	archive[idx]->full_path = 0;
	archive[idx]->add_recurse = 1;

	command = xa_add_single_files ( archive[idx], names, NULL );

	archive[idx]->has_passwd = dummy_password;
	archive[idx]->full_path = full_path;
	archive[idx]->add_recurse = add_recurse;

	if (command != NULL)
	{
		gtk_widget_set_sensitive ( Stop_button , TRUE);
		gtk_widget_set_sensitive ( check_menu , FALSE);
		gtk_widget_set_sensitive ( close1 , 	FALSE);
		gtk_widget_set_sensitive ( properties , FALSE);
		xa_set_button_state (0,0,0,0,0,0);
		xa_run_command (command , 1);
		g_free (command);
	}
	g_string_free (names, TRUE);
	g_strfreev ( array );

}

gboolean key_press_function (GtkWidget *widget, GdkEventKey *event, gpointer data)
{
    if (event == NULL) return FALSE;
	switch (event->keyval)
    {
	    case GDK_Escape:
	    if ( GTK_WIDGET_VISIBLE (viewport2) )
			xa_cancel_archive (NULL, NULL);
	    break;

	    case GDK_Delete:
        if ( GTK_WIDGET_STATE (Delete_button) != GTK_STATE_INSENSITIVE )
			xa_delete_archive ( NULL , NULL );
		break;
    }
	return FALSE;
}

void xa_select_all ( GtkMenuItem *menuitem , gpointer user_data )
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

void xa_activate_link (GtkAboutDialog *about, const gchar *link, gpointer data)
{
	GdkScreen *screen;
	GtkWidget *message;
	GError *error = NULL;
	gchar *argv[3];
	gchar *browser_path;

	browser_path = g_find_program_in_path ("firefox");
	if ( browser_path == NULL)
		browser_path = g_find_program_in_path ("opera");

	if ( browser_path == NULL)
		browser_path = g_find_program_in_path ("mozilla");

	argv[0] = browser_path;
	argv[1] = (gchar *) link;
	argv[2] = NULL;

	if (about == NULL)
		screen = gtk_widget_get_screen (GTK_WIDGET (MainWindow));
	else
		screen = gtk_widget_get_screen (GTK_WIDGET (about));

	if (!gdk_spawn_on_screen (screen, NULL, argv, NULL, G_SPAWN_SEARCH_PATH, NULL, NULL, NULL, &error))
	{
		message = gtk_message_dialog_new (GTK_WINDOW (about),
										GTK_DIALOG_MODAL
										| GTK_DIALOG_DESTROY_WITH_PARENT,
										GTK_MESSAGE_ERROR,
										GTK_BUTTONS_CLOSE,
										_("Failed to open link."));
		gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (message), "%s.", error->message);
		gtk_dialog_run (GTK_DIALOG (message));
		gtk_widget_destroy (message);
		g_error_free (error);
	}
	if (browser_path != NULL)
		g_free (browser_path);
}

void xa_show_help (GtkMenuItem *menuitem , gpointer user_data )
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

void xa_show_archive_comment ( GtkMenuItem *menuitem , gpointer user_data )
{
	GtkWidget *comment_window;
	gint current_page;
	gint idx;

	current_page = gtk_notebook_get_current_page(notebook);
	idx = xa_find_archive_index (current_page);
	comment_window = view_win ( _("Archive comment window") );
	gtk_text_buffer_create_tag (viewtextbuf, "bold","weight", PANGO_WEIGHT_BOLD, NULL);
	gtk_text_buffer_insert (viewtextbuf, &viewenditer, "\n", 1);
	gtk_text_buffer_insert_with_tags_by_name (viewtextbuf, &viewenditer, archive[idx]->comment->str, archive[idx]->comment->len, "bold", NULL);
	gtk_widget_show (comment_window);
}

