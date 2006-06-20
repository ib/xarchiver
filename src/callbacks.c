/*
 *  Copyright (C) 2006 Giuseppe Torelli - <colossus73@gmail.com>
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
#include "interface.h"
#include "support.h"
#include "main.h"

extern GList *ArchiveType;
extern GList *ArchiveSuffix;
extern gboolean cli;

#ifndef HAVE_STRCASESTR
/*
 * case-insensitive version of strstr()
 */
const char *strcasestr(const char *haystack, const char *needle)
{
	const char *h;
	const char *n;

	h = haystack;
	n = needle;
	while (*haystack)
	{
		if (tolower((unsigned char) *h) == tolower((unsigned char) *n))
		{
			h++;
			n++;
			if (!*n)
				return haystack;
		} else {
			h = ++haystack;
			n = needle;
		}
	}
	return NULL;
}
#endif /* !HAVE_STRCASESTR */

gchar *CurrentFolder = NULL;
GList *Suffix , *Name;


void xa_watch_child ( GPid pid, gint status, gpointer data)
{
	XArchive *archive = data;
	OffDeleteandViewButtons();
	if ( archive->type == XARCHIVETYPE_BZIP2 || archive->type == XARCHIVETYPE_GZIP )
		xa_set_button_state (1,1,0,0);
	else if (archive->type == XARCHIVETYPE_RPM)
	{
		xa_set_button_state (1,1,0,1);
		gtk_widget_set_sensitive ( check_menu , FALSE);
	}
	else if (archive->type == XARCHIVETYPE_TAR_BZ2 || archive->type == XARCHIVETYPE_TAR_GZ || archive->type == XARCHIVETYPE_TAR )
	{
		xa_set_button_state (1,1,1,1);
        gtk_widget_set_sensitive ( check_menu , FALSE);
	}
	else
	{
		xa_set_button_state (1,1,1,1);
        gtk_widget_set_sensitive ( check_menu , TRUE);
	}

    if ( archive->passwd != NULL || archive->has_passwd )
    {
        gtk_widget_show ( pad_image );
        gtk_tooltips_enable ( pad_tooltip );
    }
    else
    {
        gtk_widget_hide ( pad_image );
		gtk_tooltips_disable ( pad_tooltip );
    }

	if ( WIFSIGNALED (status) )
	{
		Update_StatusBar ( _("Operation canceled."));
		OffTooltipPadlock();
		if (archive->status == XA_ARCHIVESTATUS_EXTRACT)
		{
			gchar *msg = g_strconcat (_("Please check \""),extract_path,_("\" since some files could have been already extracted."),NULL);
            response = ShowGtkMessageDialog (GTK_WINDOW (MainWindow),GTK_DIALOG_MODAL,GTK_MESSAGE_INFO,	GTK_BUTTONS_OK,msg );
            g_free (msg);
		}
		archive->status = XA_ARCHIVESTATUS_IDLE;
		return;
	}
	OffTooltipPadlock();

	if ( WIFEXITED (status) )
	{
		if ( WEXITSTATUS (status) )
		{
			gtk_tooltips_disable ( pad_tooltip );
			gtk_widget_hide ( pad_image );
			gtk_window_set_title ( GTK_WINDOW (MainWindow) , "Xarchiver " VERSION );
			response = ShowGtkMessageDialog (GTK_WINDOW	(MainWindow),GTK_DIALOG_MODAL,GTK_MESSAGE_QUESTION,GTK_BUTTONS_YES_NO,_("An error occurred while accessing the archive.\nDo you want to open the error messages window?") );
			if (response == GTK_RESPONSE_YES)
				ShowShellOutput (NULL);
			/* This in case the user supplies a wrong password we reset it so he can try again */
			if (archive->status == XA_ARCHIVESTATUS_TEST && archive->passwd != NULL)
			{
				g_free (archive->passwd);
				archive->passwd = NULL;
			}
			archive->status = XA_ARCHIVESTATUS_ERROR;
			Update_StatusBar ( _("Operation failed."));
			return;
		}
	}
	
	/* This to automatically reload the content of the archive after adding or deleting */
	if (archive->status == XA_ARCHIVESTATUS_DELETE || archive->status == XA_ARCHIVESTATUS_ADD)
	{
        if (archive->type == XARCHIVETYPE_BZIP2 || archive->type == XARCHIVETYPE_GZIP)
			Update_StatusBar ( _("Operation completed."));
		else
			Update_StatusBar ( _("Please wait while the content of the archive is being updated..."));
        RemoveColumnsListStore();
        archive->status = XA_ARCHIVESTATUS_IDLE;
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
            
            default:
            break;
		}
        return;
	}

	gtk_tree_view_set_model (GTK_TREE_VIEW(treeview1), model);
	g_object_unref (model);

	gtk_window_set_title ( GTK_WINDOW (MainWindow) , archive->path );
	gtk_widget_set_sensitive ( properties , TRUE );
	archive->status = XA_ARCHIVESTATUS_IDLE;
    Update_StatusBar ( _("Operation successfully completed."));
}

void xa_new_archive (GtkMenuItem *menuitem, gpointer user_data)
{
	archive = xa_init_archive_structure (archive);
	gchar *path = Show_File_Dialog ( 1 , "new" );
	if (path == NULL)
		return;
	if ( g_file_test ( path , G_FILE_TEST_EXISTS ) )
	{
		gchar *utf8_path;
		gchar  *msg;

		utf8_path = g_filename_to_utf8 (path, -1, NULL, NULL, NULL);
		msg = g_strdup_printf (_("The archive \"%s\" already exists. Do you want to overwrite it?"), utf8_path);
		response = ShowGtkMessageDialog (GTK_WINDOW (MainWindow),
						GTK_DIALOG_MODAL,
						GTK_MESSAGE_QUESTION,
						GTK_BUTTONS_YES_NO,
						msg
						);
		g_free (utf8_path);
		g_free (msg);		
		if (response != GTK_RESPONSE_YES)
		{
			g_free (path);
            return;
		}
        //The following to avoid to update the archive instead of adding to it since the filename exists
        unlink ( path );
	}
	xa_set_button_state (1,1,1,0 );
	archive->path = g_strdup (path);
	g_free (path);
    archive->escaped_path = EscapeBadChars (archive->path);
    EmptyTextBuffer();
    archive->has_passwd = FALSE;
    gtk_widget_set_sensitive ( iso_info , FALSE );
    gtk_widget_set_sensitive ( view_shell_output1 , TRUE );
    gtk_widget_set_sensitive ( check_menu , FALSE);
    gtk_widget_set_sensitive ( properties , FALSE );
    /* Let's off the delete and view buttons and the menu entries to avoid misterious behaviour */
    OffDeleteandViewButtons ();

	if ( liststore != NULL )
		RemoveColumnsListStore();
	
  	if (archive->type == XARCHIVETYPE_BZIP2 || archive->type == XARCHIVETYPE_GZIP)
	{
		Update_StatusBar ( _("Choose Add File to create the compressed file."));
		xa_set_button_state (1,1,1,0 );
	}
	else
		Update_StatusBar ( _("Choose Add File or Add Folder to begin creating the archive."));

    gtk_tooltips_disable ( pad_tooltip );
    gtk_widget_hide ( pad_image );
    
    archive->passwd = NULL;
    archive->dummy_size = 0;
    archive->nr_of_files = 0;
    archive->nr_of_dirs = 0;
	gtk_window_set_title ( GTK_WINDOW (MainWindow) , archive->path );
}

int ShowGtkMessageDialog ( GtkWindow *window, int mode,int type,int button, gchar *message)
{
	dialog = gtk_message_dialog_new (window, mode, type, button,message);
	gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_YES);
	response = gtk_dialog_run (GTK_DIALOG (dialog) );
	gtk_widget_destroy (GTK_WIDGET (dialog) );
	return response;
}

void xa_open_archive (GtkMenuItem *menuitem, gpointer data)
{
	path = (gchar *)data;
	if ( path == NULL)
    {
		path = Show_File_Dialog ( 1 , "open" );
		if (path == NULL)
			return;
	}
	if ( liststore != NULL )
	{
		RemoveColumnsListStore();
		EmptyTextBuffer ();
	}
	archive = xa_init_archive_structure(archive);
	archive->path = g_strdup (path);
	g_free (path);
	archive->escaped_path = EscapeBadChars ( archive->path );
    
	OffDeleteandViewButtons();
    gtk_widget_set_sensitive ( iso_info , FALSE );
    gtk_widget_set_sensitive ( view_shell_output1 , TRUE );

    archive->type = DetectArchiveType ( archive );
    if ( archive->type == -2 )
		return;
    if ( archive->type == -1 )
    {
        gtk_window_set_title ( GTK_WINDOW (MainWindow) , "Xarchiver " VERSION );
		response = ShowGtkMessageDialog (GTK_WINDOW (MainWindow),GTK_DIALOG_MODAL,GTK_MESSAGE_ERROR,GTK_BUTTONS_OK,
		_("The format of this archive is not recognized !") );
		xa_set_button_state ( 1,1,0,0);
        return;
	}
    EmptyTextBuffer();
    
    //Does the user open an archive from the command line whose archiver is not installed ?
    gchar *ext = NULL;
    if ( archive->type == XARCHIVETYPE_RAR )
		ext = ".rar";
	else if ( archive->type == XARCHIVETYPE_7ZIP )
		ext = ".7z";
    else if ( archive->type == XARCHIVETYPE_ARJ )
		ext = ".arj";
    if ( ext != NULL )
        if ( ! g_list_find ( ArchiveType , ext ) )
        {
            response = ShowGtkMessageDialog (GTK_WINDOW (MainWindow),GTK_DIALOG_MODAL,GTK_MESSAGE_ERROR, GTK_BUTTONS_OK,_("Sorry, this archive format is not supported since the proper archiver has't been installed!") );
            return;
        }

    gtk_widget_set_sensitive (Stop_button,TRUE);
    gtk_widget_show ( viewport2 );
    if ( archive->type == XARCHIVETYPE_ISO )
		Update_StatusBar ( _("Please wait while the content of the ISO image is being read..."));
    else
		Update_StatusBar ( _("Please wait while the content of the archive is being read..."));
    xa_set_button_state (1,1,1,1);
	
	switch ( archive->type )
	{
		case XARCHIVETYPE_ARJ:
		OpenArj (archive);
		break;

		case XARCHIVETYPE_BZIP2:
		OpenBzip2 (archive);
		break;

		case XARCHIVETYPE_GZIP:
		OpenGzip ( archive );
		break;

		
        case XARCHIVETYPE_ISO:
        OpenISO (archive);
		break;

		case XARCHIVETYPE_RAR:
		OpenRar (archive);
		break;

		case XARCHIVETYPE_RPM:
        OpenRPM (archive);
        break;

		case XARCHIVETYPE_TAR:
		OpenTar (archive);
		break;

		case XARCHIVETYPE_ZIP:
		OpenZip (archive);
		break;

        case XARCHIVETYPE_7ZIP:
        Open7Zip (archive);
        break;
        
        default:
        break;
	}
	if (archive->passwd != NULL)
		g_free (archive->passwd);
	archive->passwd = NULL;
}

void xa_test_archive (GtkMenuItem *menuitem, gpointer user_data)
{
    gchar *command;
    if ( archive->has_passwd )
    {
        if ( archive->passwd == NULL)
		{
			archive->passwd = password_dialog ();
			if ( archive->passwd == NULL)
				return;
		}
	}
    Update_StatusBar ( _("Testing archive integrity, please wait..."));
    gtk_widget_set_sensitive (Stop_button,TRUE);
    gtk_widget_set_sensitive ( check_menu , FALSE );
    xa_set_button_state (0,0,0,0);
    switch ( archive->type )
	{
		case XARCHIVETYPE_RAR:
		if (archive->passwd != NULL)
			command = g_strconcat ("rar t -idp -p" , archive->passwd ," " , archive->escaped_path, NULL);
		else
			command = g_strconcat ("rar t -idp " , archive->escaped_path, NULL);
        break;

        case XARCHIVETYPE_ZIP:
        if (archive->passwd != NULL)
			command = g_strconcat ("unzip -P ", archive->passwd, " -t " , archive->escaped_path, NULL);
        else
			command = g_strconcat ("unzip -t " , archive->escaped_path, NULL);
        break;

        case XARCHIVETYPE_7ZIP:
        if (archive->passwd != NULL)
			command = g_strconcat ( "7za t -p" , archive->passwd , " " , archive->escaped_path, NULL);
		else
			command = g_strconcat ("7za t " , archive->escaped_path, NULL);
		break;

		case XARCHIVETYPE_ARJ:
        if (archive->passwd != NULL)
			command = g_strconcat ("arj t -g" , archive->passwd , " -i " , archive->escaped_path, NULL);
		else
			command = g_strconcat ("arj t -i " , archive->escaped_path, NULL);
		break;
		
		default:
		command = NULL;
	}
	archive->status = XA_ARCHIVESTATUS_TEST;
    ExtractAddDelete ( command );
    g_free (command);
}

void xa_quit_application (GtkMenuItem *menuitem, gpointer user_data)
{
    if ( GTK_WIDGET_VISIBLE (viewport2) )
    {
        Update_StatusBar ( _("Please hit the Stop button first!"));
        return;
    }
    g_list_free ( Suffix );
	g_list_free ( Name );
	if (archive != NULL)
	{
		if (archive->path)
			g_free(archive->path);

		if (archive->escaped_path)
			g_free(archive->escaped_path);

		if (archive->tmp)
		{
			unlink (archive->tmp);
			g_free(archive->tmp);
		}

		if (archive->passwd)
			g_free(archive->passwd);
		g_free (archive);
	}

	if ( extract_path != NULL )
    {
        if ( strcmp (extract_path,"/tmp/") != 0)
			g_free (extract_path);
        g_free (destination_path);
    }
    gtk_main_quit();
}

void xa_delete_archive (GtkMenuItem *menuitem, gpointer user_data)
{
	gchar *command = NULL;
    char numbers[6];
	gint x;

	GtkTreeSelection *selection = gtk_tree_view_get_selection ( GTK_TREE_VIEW (treeview1) );
	names = g_string_new ( " " );
	gtk_tree_selection_selected_foreach (selection, (GtkTreeSelectionForeachFunc) ConcatenateFileNames, names );
    x = gtk_tree_selection_count_selected_rows (selection);
    sprintf ( numbers , "%d", x );
    gchar *msg = g_strconcat (_("You are about to delete "),numbers,x > 1 ? _(" files") : _(" file") , _(" from the archive.\nAre you really sure to do this?"),NULL);
    response = ShowGtkMessageDialog (GTK_WINDOW (MainWindow),GTK_DIALOG_MODAL,GTK_MESSAGE_QUESTION,GTK_BUTTONS_YES_NO,msg );
    g_free (msg);
    if ( response == GTK_RESPONSE_NO)
		return;
    Update_StatusBar ( _("Deleting files from the archive, please wait..."));
	archive->status = XA_ARCHIVESTATUS_DELETE;

	switch (archive->type)
	{
		case XARCHIVETYPE_RAR:
		command = g_strconcat ( "rar d " , archive->escaped_path , names->str , NULL );
		break;

        case XARCHIVETYPE_TAR:
		command = g_strconcat ( "tar --delete -vf " , archive->escaped_path , names->str , NULL );
		break;

        case XARCHIVETYPE_TAR_BZ2:
        DecompressBzipGzip ( names , archive , 0  , 0 );
        break;

        case XARCHIVETYPE_TAR_GZ:
        DecompressBzipGzip ( names , archive , 1 , 0 );
		break;
		
        case XARCHIVETYPE_ZIP:
		command = g_strconcat ( "zip -d " , archive->escaped_path , names->str , NULL );
		break;

        case XARCHIVETYPE_7ZIP:
        command = g_strconcat ( "7za d " , archive->escaped_path , names->str , NULL );
        break;

        case XARCHIVETYPE_ARJ:
        command = g_strconcat ( "arj d " , archive->escaped_path , names->str, NULL);
        break;
        
        default:
        break;
	}
	if (command != NULL)
    {
        ExtractAddDelete ( command );
        g_free (command);
    }
    g_string_free (names , FALSE );
}

void xa_add_files_archive ( GtkMenuItem *menuitem, gpointer data )
{
	gchar *command = NULL;
	
	add_window = xa_create_add_dialog (archive);
	command = xa_parse_add_dialog_options ( archive, add_window );
	gtk_widget_destroy ( add_window->dialog1 );
	if (command != NULL)
	{
		ExtractAddDelete (command);
		g_free (command);
	}
	g_free ( add_window );
	add_window = NULL;
}

void xa_extract_archive ( GtkMenuItem *menuitem , gpointer user_data )
{
	gchar *command = NULL;

	GtkTreeSelection *selection = gtk_tree_view_get_selection ( GTK_TREE_VIEW (treeview1) );
	gint selected = gtk_tree_selection_count_selected_rows ( selection );
    extract_window = xa_create_extract_dialog (selected , archive);
	if (extract_path != NULL)
		gtk_entry_set_text (GTK_ENTRY(extract_window->destination_path_entry),extract_path);
    command = xa_parse_extract_dialog_options ( archive , extract_window, selection );
	gtk_widget_destroy ( extract_window->dialog1 );
	if (command != NULL)
	{
		//g_message (command);
		ExtractAddDelete (command);
		g_free (command);
	}
	g_free (extract_window);
	extract_window = NULL;
}

void xa_about (GtkMenuItem *menuitem, gpointer user_data)
{
    static GtkWidget *about = NULL;
    const char *authors[] = {"\nDeveloper:\nGiuseppe Torelli - Colossus <colossus73@gmail.com>\n",NULL};
    const char *documenters[] = {"\nThanks to:\nBenedikt Meurer for helping me with DnD.\nStephan Arts for hints on code optimization.\nEnrico Troeger for supplying patchs.\nBjoern Martensen for reporting bugs.\nSalvatore Santagati for integrating\nisoinfo code in Xarchiver.\nUracile for the stunning logo.\nThe XFCE translators.\nThe people of gtk-app-devel-list\nwho kindly answered my questions.", NULL};
	if (about != NULL)
	{
		gtk_window_present (GTK_WINDOW (about));
		return;
	}
	about = gtk_about_dialog_new ();
	g_object_set (about,
		      "name",  "Xarchiver",
		      "version", VERSION,
		      "copyright", "Copyright @2005-2006 Giuseppe Torelli",
		      "comments", "A lightweight GTK2 archive manager",
		      "authors", authors,
              "documenters",documenters,
		      "translator_credits", NULL,
		      "logo_icon_name", "xarchiver",
		      "website", "http://xarchiver.xfce.org",
		      "website_label", NULL,
		      "license",    "Copyright @2005-2006 Giuseppe Torelli - Colossus <gt67@users.sourceforge.net>\n\n"
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
	gtk_window_set_destroy_with_parent (GTK_WINDOW (about), TRUE);
	g_signal_connect (G_OBJECT (about), "destroy",  G_CALLBACK (gtk_widget_destroyed), &about);
	gtk_window_set_position (GTK_WINDOW (about), GTK_WIN_POS_CENTER);
	gtk_widget_show (about);
}

GSList *Add_File_Dialog ( gchar *mode )
{
	GSList *list = NULL;
	int flag;
	if ( mode == "file" )
	{
		title = _("Select the files to be added to the current archive; use SHIFT to multiple select");
		flag = GTK_FILE_CHOOSER_ACTION_OPEN;
	}
	else
	{
		if (archive->type == XARCHIVETYPE_ARJ)
			title = _("Select the folder to be added to the current archive");
        else
			title = _("Select the folders to be added to the current archive; use SHIFT to multiple select");
		flag = GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER;
	}
	File_Selector = gtk_file_chooser_dialog_new ( title,
							GTK_WINDOW (MainWindow),
							flag,
							GTK_STOCK_CANCEL,
							GTK_RESPONSE_CANCEL,
							GTK_STOCK_ADD,
							GTK_RESPONSE_ACCEPT,
							NULL);
    /* We set gtk_file_chooser_set_select_multiple to FALSE because a bug in ARJ prevents adding more of two directories */
    if (archive->type == XARCHIVETYPE_BZIP2 || archive->type == XARCHIVETYPE_GZIP || ( archive->type == XARCHIVETYPE_ARJ && mode == "folder" ) )
		gtk_file_chooser_set_select_multiple ( GTK_FILE_CHOOSER (File_Selector) , FALSE );
    else
		gtk_file_chooser_set_select_multiple ( GTK_FILE_CHOOSER (File_Selector) , TRUE );
	if (CurrentFolder != NULL)
		gtk_file_chooser_set_current_folder ( GTK_FILE_CHOOSER (File_Selector) , CurrentFolder );
	response = gtk_dialog_run (GTK_DIALOG (File_Selector) );
	if (response == GTK_RESPONSE_ACCEPT)
		list = gtk_file_chooser_get_filenames ( GTK_FILE_CHOOSER (File_Selector) );
	gtk_widget_destroy (File_Selector);
	return list;
}

gchar *Show_File_Dialog ( int dummy , gpointer mode )
{
	GtkWidget *hbox = NULL;
	GtkWidget *combo_box = NULL;
	GtkWidget *check_button = NULL;
	GtkFileFilter *filter;
	GtkTooltips *FilterToolTip ;
	gchar *path = NULL;
    gchar *dummy_path = NULL;
	const gchar *flag2 = NULL;
	unsigned short int flag = 0;

	if ( mode == "new" )
	{
		title = _("Create a new archive");
		flag = GTK_FILE_CHOOSER_ACTION_SAVE;
		flag2 = "gtk-new";
	}
	else if ( mode == "open" )
	{
		title = _("Open an archive");
		flag = GTK_FILE_CHOOSER_ACTION_OPEN;
		flag2 = "gtk-open";
	}

	else if (mode == "extract" )
	{
		title = _("Choose the destination folder where to extract the current archive");
		File_Selector = gtk_file_chooser_dialog_new ( title,
		GTK_WINDOW (MainWindow),
		GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER,
		GTK_STOCK_CANCEL,
		GTK_RESPONSE_CANCEL,
		GTK_STOCK_OPEN,
		GTK_RESPONSE_ACCEPT,
		NULL );
        if (destination_path != NULL)
			gtk_file_chooser_set_current_folder ( GTK_FILE_CHOOSER (File_Selector) , destination_path );
        response = gtk_dialog_run (GTK_DIALOG (File_Selector));
		if (response == GTK_RESPONSE_ACCEPT)
			gtk_entry_set_text (GTK_ENTRY(extract_window->destination_path_entry),gtk_file_chooser_get_filename ( GTK_FILE_CHOOSER (File_Selector) ) );
		gtk_widget_destroy (File_Selector);
		return NULL;
	}

	File_Selector = gtk_file_chooser_dialog_new ( title,
							GTK_WINDOW (MainWindow),
							flag,
							GTK_STOCK_CANCEL,
							GTK_RESPONSE_CANCEL,
							flag2,
							GTK_RESPONSE_ACCEPT,
							NULL);
	if (CurrentFolder != NULL)
		gtk_file_chooser_set_current_folder ( GTK_FILE_CHOOSER (File_Selector) , CurrentFolder );
	gtk_dialog_set_default_response (GTK_DIALOG (File_Selector), GTK_RESPONSE_ACCEPT);

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

	if ( mode == "new" )
	{
		hbox = gtk_hbox_new (FALSE, 12);
		gtk_box_pack_start (GTK_BOX (hbox),gtk_label_new (_("Archive type:")),FALSE, FALSE, 0);
		combo_box = gtk_combo_box_new_text ();
		FilterToolTip = gtk_tooltips_new();
		gtk_tooltips_set_tip (FilterToolTip,combo_box, _("Choose the archive type to create") , NULL);
		Name = g_list_first ( ArchiveType );
		while ( Name != NULL )
		{
			if (Name->data != ".tgz" && Name->data != ".rpm" && Name->data != ".iso" )
				gtk_combo_box_append_text (GTK_COMBO_BOX (combo_box), Name->data );
			Name = g_list_next ( Name );
		}
		gtk_combo_box_set_active (GTK_COMBO_BOX (combo_box), 0);
		gtk_box_pack_start (GTK_BOX (hbox), combo_box, TRUE, TRUE, 0);
		check_button = gtk_check_button_new_with_label (_("Add the archive extension to the filename"));
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(check_button),TRUE);
		gtk_box_pack_start (GTK_BOX (hbox), check_button, TRUE, TRUE, 0);
		gtk_widget_show_all (hbox);
		gtk_file_chooser_set_extra_widget (GTK_FILE_CHOOSER (File_Selector), hbox);
	}

	gtk_window_set_modal (GTK_WINDOW (File_Selector),TRUE);
	response = gtk_dialog_run (GTK_DIALOG (File_Selector));
	CurrentFolder = gtk_file_chooser_get_current_folder ( GTK_FILE_CHOOSER (File_Selector) );
	if (response == GTK_RESPONSE_ACCEPT)
	{
		path = gtk_file_chooser_get_filename ( GTK_FILE_CHOOSER (File_Selector) );
		if ( mode == "new")
		{
			ComboArchiveType = gtk_combo_box_get_active_text (GTK_COMBO_BOX (combo_box));
            if (strcmp ( ComboArchiveType,".arj") == 0) archive->type = XARCHIVETYPE_ARJ;
                else if (strcmp ( ComboArchiveType,".bz2") == 0) archive->type = XARCHIVETYPE_BZIP2;
                else if (strcmp ( ComboArchiveType,".gz") == 0) archive->type = XARCHIVETYPE_GZIP;
                else if (strcmp ( ComboArchiveType,".rar") == 0) archive->type = XARCHIVETYPE_RAR;
                else if (strcmp ( ComboArchiveType,".tar") == 0) archive->type = XARCHIVETYPE_TAR;
                else if (strcmp ( ComboArchiveType,".tar.bz2") == 0) archive->type = XARCHIVETYPE_TAR_BZ2;
                else if (strcmp ( ComboArchiveType,".tar.gz") == 0) archive->type = XARCHIVETYPE_TAR_GZ;
                else if (strcmp ( ComboArchiveType,".jar") == 0 || strcmp ( ComboArchiveType,".zip") == 0 ) archive->type = XARCHIVETYPE_ZIP;
                else if (strcmp ( ComboArchiveType,".rpm") == 0) archive->type = XARCHIVETYPE_RPM;
                else if (strcmp ( ComboArchiveType,".7z") == 0) archive->type = XARCHIVETYPE_7ZIP;
			if ( gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(check_button) ) )
			{
				if ( ! g_str_has_suffix ( path , ComboArchiveType ) )
				{
					dummy_path = g_strconcat ( path, ComboArchiveType , NULL);
					g_free ( path );
					path = dummy_path;
				}
			}
		}
		gtk_widget_destroy (File_Selector);
	}
	else if ( (response == GTK_RESPONSE_CANCEL) || (response == GTK_RESPONSE_DELETE_EVENT) )
	{
		gtk_widget_destroy (File_Selector);
		path = NULL;
	}
	return path;
}

gboolean isISO ( FILE *ptr )
{
	if ( DetectImage(ptr) > 0 )
        return TRUE;
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

int DetectArchiveType ( XArchive *archive )
{
	FILE *dummy_ptr = NULL;
    int xx = -1;
	unsigned char magic[6];
	dummy_ptr = fopen ( archive->path , "r" );
	
	if (dummy_ptr == NULL)
	{
		gchar *msg = g_strdup_printf (_("Can't open archive %s:\n%s") , archive->path , strerror (errno) );
		response = ShowGtkMessageDialog (GTK_WINDOW (MainWindow) , GTK_DIALOG_MODAL,GTK_MESSAGE_ERROR,GTK_BUTTONS_OK,
		msg);
		g_free (msg);
		return -2;
	 }
	if ( fread ( magic, 1, 6, dummy_ptr ) == 0 )
	{
		fclose ( dummy_ptr);
		return -2;
	}
    if ( memcmp ( magic,"\x50\x4b\x03\x04",4 ) == 0 || memcmp ( magic,"\x50\x4b\x05\x06",4 ) == 0 )
    {
        archive->has_passwd = DetectPasswordProtectedArchive ( XARCHIVETYPE_ZIP , dummy_ptr , magic );
        xx = XARCHIVETYPE_ZIP;
    }
	else if ( memcmp ( magic,"\x60\xea",2 ) == 0 )
    {
        archive->has_passwd = DetectPasswordProtectedArchive ( XARCHIVETYPE_ARJ , dummy_ptr , magic );
        xx = XARCHIVETYPE_ARJ;
    }
	else if ( memcmp ( magic,"\x52\x61\x72\x21",4 ) == 0 ) xx = XARCHIVETYPE_RAR;
    else if ( memcmp ( magic,"\x42\x5a\x68",3 ) == 0 ) xx = XARCHIVETYPE_BZIP2;
	else if ( memcmp ( magic,"\x1f\x8b\x08",3 ) == 0) xx = XARCHIVETYPE_GZIP;
    else if ( memcmp ( magic,"\xed\xab\xee\xdb",4 ) == 0) xx = XARCHIVETYPE_RPM;
    else if ( memcmp ( magic,"\x37\x7a\xbc\xaf\x27\x1c",6 ) == 0 ) xx = XARCHIVETYPE_7ZIP;
    else if ( isTar ( dummy_ptr ) ) xx = XARCHIVETYPE_TAR;
    else if ( isISO ( dummy_ptr ) ) xx = XARCHIVETYPE_ISO;
	fclose ( dummy_ptr );
	return xx;
}

gboolean DetectPasswordProtectedArchive ( int type , FILE *stream , unsigned char magic[6] )
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
        if (extended_header_size != 0) fread (&extended_header_CRC,1,4,stream);
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
            if (extended_header_size != 0) fread (&extended_header_CRC,1,4,stream);
            fseek ( stream , compressed_size , SEEK_CUR);
            fread (&sig,1,2,stream);
        }
    }
    return FALSE;
}

void RemoveColumnsListStore()
{
	gtk_window_set_title ( GTK_WINDOW (MainWindow) , "Xarchiver " VERSION );
	GList *columns = gtk_tree_view_get_columns ( GTK_TREE_VIEW (treeview1) );
	while (columns != NULL)
	{
		gtk_tree_view_remove_column (GTK_TREE_VIEW (treeview1) , columns->data);
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

void xa_create_liststore ( unsigned short int nc, gchar *columns_names[] , GType columns_types[])
{
	unsigned short int x;

	liststore = gtk_list_store_newv ( nc , (GType *)columns_types);
	gtk_tree_view_set_model ( GTK_TREE_VIEW (treeview1), GTK_TREE_MODEL (liststore) );
	gtk_tree_view_set_rules_hint ( GTK_TREE_VIEW (treeview1) , TRUE );
    gtk_tree_view_set_search_equal_func (GTK_TREE_VIEW (treeview1),(GtkTreeViewSearchEqualFunc) treeview_select_search, NULL, NULL);
	GtkTreeSelection *sel = gtk_tree_view_get_selection( GTK_TREE_VIEW (treeview1) );
	gtk_tree_selection_set_mode(sel, GTK_SELECTION_MULTIPLE);
	g_signal_connect ((gpointer) sel, "changed", G_CALLBACK (Activate_buttons), NULL);
    
	model = gtk_tree_view_get_model(GTK_TREE_VIEW(treeview1));
    g_object_ref(model);
	gtk_tree_view_set_model(GTK_TREE_VIEW(treeview1), NULL);

	for (x = 0; x <= nc-1; x++)
	{
		renderer = gtk_cell_renderer_text_new ();
		column = gtk_tree_view_column_new_with_attributes ( columns_names[x],renderer,"text",x,NULL);
		gtk_tree_view_column_set_resizable (column, TRUE);
		gtk_tree_view_column_set_sort_column_id (column, x);
		gtk_tree_view_append_column (GTK_TREE_VIEW (treeview1), column);
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

void ShowShellOutput ( GtkMenuItem *menuitem )
{
	if (OutputWindow != NULL)
	{
		gtk_window_set_title (GTK_WINDOW (OutputWindow), _("Error Messages Window") );
		gtk_window_present ( GTK_WINDOW (OutputWindow) );
		return;
	}
	OutputWindow = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	gtk_window_set_position (GTK_WINDOW (OutputWindow), GTK_WIN_POS_CENTER);
	gtk_window_set_default_size(GTK_WINDOW(OutputWindow), 380, 250);
	gtk_window_set_destroy_with_parent (GTK_WINDOW (OutputWindow), TRUE);
	g_signal_connect (G_OBJECT (OutputWindow), "delete_event",  G_CALLBACK (gtk_widget_hide), &OutputWindow);

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
	if (archive->status == XA_ARCHIVESTATUS_ADD)
	{
		response = ShowGtkMessageDialog (GTK_WINDOW	(MainWindow),GTK_DIALOG_MODAL,GTK_MESSAGE_QUESTION,GTK_BUTTONS_YES_NO,_("Doing so will probably corrupt your archive.\nDo you really want to cancel?") );
		if (response == GTK_RESPONSE_NO)
			return;
	}
	gtk_widget_set_sensitive ( Stop_button , FALSE );
    Update_StatusBar (_("Waiting for the process to abort..."));
    if ( kill ( archive->child_pid , SIGABRT ) < 0 )
    {
        response = ShowGtkMessageDialog (GTK_WINDOW (MainWindow),GTK_DIALOG_MODAL,GTK_MESSAGE_ERROR,GTK_BUTTONS_OK, strerror(errno));
	    return;
    }
    /* This in case the user cancels the opening of a password protected archive */
    if (archive->status != XA_ARCHIVESTATUS_ADD || archive->status != XA_ARCHIVESTATUS_DELETE)
        if (archive->has_passwd)
			archive->has_passwd = FALSE;
	archive->status = XA_ARCHIVESTATUS_IDLE;
}

void View_File_Window ( GtkMenuItem *menuitem , gpointer user_data )
{
	gchar *command = NULL;
	GtkTreeSelection *selection;
	GtkTreeModel *model;
	GtkTreeIter iter;
	gchar *dir;
	gchar *dummy_name;
	unsigned short int COL_NAME;
	gboolean is_dir = FALSE;
	gboolean dummy;
	GList *row_list = NULL;

	if ( archive->has_passwd )
	{
		//TODO: archive->passwd = password_dialog ();
		if ( archive->passwd == NULL )
			return;
	}
	selection = gtk_tree_view_get_selection ( GTK_TREE_VIEW (treeview1) );

	/* if no or more than one rows selected, do nothing, just for sanity */
	if ( gtk_tree_selection_count_selected_rows (selection) != 1)
		return;

	row_list = gtk_tree_selection_get_selected_rows (selection, &model);
	if ( row_list == NULL )
		return;

	gtk_tree_model_get_iter(model, &iter, row_list->data);

	gtk_tree_path_free(row_list->data);
	g_list_free (row_list);

	switch (archive->type)
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
	if (archive->type == XARCHIVETYPE_ZIP)
	{
		if ( g_str_has_suffix (dir,"/") == TRUE )
			is_dir = TRUE;
	}
	else if ( strstr ( dir , "d" ) || strstr ( dir , "D" ) ) is_dir = TRUE;
	if (is_dir)
	{
		response = ShowGtkMessageDialog (GTK_WINDOW (MainWindow),GTK_DIALOG_MODAL,GTK_MESSAGE_ERROR, GTK_BUTTONS_OK,_("Please select a file, not a directory!") );
		g_free ( dir );
		return;
	}
	g_free ( dir );
	gtk_tree_model_get (model, &iter, 0, &dummy_name, -1);
	dir = EscapeBadChars ( dummy_name );
	g_free (dummy_name);
	names = g_string_new (" ");
	g_string_append ( names , dir );
	dummy = archive->full_path;
	archive->full_path = 0;
	command = xa_extract_single_files ( archive , names, "/tmp");
	archive->full_path = dummy;
	archive->parse_output = 0;
	SpawnAsyncProcess ( archive , command , 0, 0);
	g_free ( command );
	if ( archive->child_pid == 0 )
		return;
	g_child_watch_add ( archive->child_pid , (GChildWatchFunc) ViewFileFromArchive , names );
}

GChildWatchFunc *ViewFileFromArchive (GPid pid , gint status , GString *data)
{
	GIOChannel *ioc_view = NULL;
	gchar *line = NULL;
	gchar *filename = NULL;
	GError *error = NULL;
	gchar *string = NULL;
	gboolean tofree = FALSE;

	if ( WIFEXITED( status ) )
	{
		if ( WEXITSTATUS ( status ) )
		{
			xa_set_button_state (1,1,0,0);
	    	gtk_window_set_title ( GTK_WINDOW (MainWindow) , "Xarchiver " VERSION );
			response = ShowGtkMessageDialog (GTK_WINDOW(MainWindow),GTK_DIALOG_MODAL,GTK_MESSAGE_QUESTION,GTK_BUTTONS_YES_NO,_("An error occurred while extracting the file to be viewed.\nDo you want to open the error messages window?") );
			if (response == GTK_RESPONSE_YES)
				ShowShellOutput (NULL);
			unlink ( (char*)data );
			return NULL;
		}
	}
	string = StripPathFromFilename ( (char*) data->str );
	//Let's avoid the white space
	data->str++;
	if (  string == NULL )
		filename = g_strconcat ( "/tmp/" , data->str , NULL );
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
    view_window = view_win();
	ioc_view = g_io_channel_new_file ( filename , "r" , &error );
    if (error == NULL)
    {
        g_io_channel_set_encoding (ioc_view, "ISO8859-1" , NULL);
        g_io_channel_set_flags ( ioc_view , G_IO_FLAG_NONBLOCK , NULL );
        g_io_channel_read_to_end ( ioc_view , &line , NULL, NULL );
        gtk_text_buffer_get_end_iter ( viewtextbuf, &viewenditer );
        gtk_text_buffer_insert (viewtextbuf, &viewenditer, line, strlen ( line ) );
        g_free ( line );
        g_io_channel_shutdown ( ioc_view , TRUE , NULL );
        g_io_channel_unref (ioc_view);
	}
	unlink ( filename );
	gtk_widget_show (view_window);
	g_free (filename);
	//Let's restore the pointer to its correct memory address
	data->str--;
	g_free ( data->str );
	g_string_free (data , FALSE);
	Update_StatusBar (_("Operation completed."));
	return NULL;
}

void xa_iso_properties ( GtkMenuItem *menuitem , gpointer user_data )
{
    unsigned long long int file_size;
	GtkWidget *iso_properties_win;

    stat ( archive->path , &my_stat );
    file_size = my_stat.st_size;
    iso_properties_win = create_iso_properties_window();
	gtk_widget_show (iso_properties_win);
}

void xa_archive_properties ( GtkMenuItem *menuitem , gpointer user_data )
{
    gchar *utf8_string , *measure, *text, *dummy_string;
    char date[64];
    gchar *t;
    unsigned long long int file_size;

    stat ( archive->path , &my_stat );
    file_size = my_stat.st_size;
    archive_properties_win = create_archive_properties_window();
    //Name
    text = StripPathFromFilename ( archive->path );
    if (text != NULL)
    {
        text++; //This to avoid the / char in the string
        utf8_string = g_filename_display_name (text);
    }
    else
		utf8_string = g_filename_display_name (archive->path);
    gtk_entry_set_text ( GTK_ENTRY (name_data), utf8_string );
    g_free (utf8_string);
    //Path
    dummy_string = remove_level_from_path (archive->path);
    if ( strlen(dummy_string) != 0)
		utf8_string = g_filename_display_name (dummy_string);
    else
		utf8_string = g_filename_display_name ( g_get_current_dir () );
    gtk_entry_set_text ( GTK_ENTRY (path_data), utf8_string );
    g_free ( utf8_string );
    g_free ( dummy_string );
    //Modified Date
    strftime (date, 64, "%c", localtime (&my_stat.st_mtime) );
    t = g_locale_to_utf8 ( date, -1, 0, 0, 0);
    gtk_entry_set_text ( GTK_ENTRY (modified_data), t);
    g_free (t);
    //Archive Size
    if (file_size > 1024*1024*1024 )
    {
        content_size = (double)file_size / (1024*1024*1024);
        measure = _(" GB");
    }
        else if (file_size > 1024*1024 )
        {
            content_size = (double)file_size / (1024*1024);
            measure = _(" MB");
        }

        else if (file_size > 1024 )
        {
            content_size = (double)file_size / 1024;
            measure = _(" KB");
        }
        else
        {
            measure = _(" bytes");
            content_size = file_size;
        }

    t = g_strdup_printf ("%.1f %s", content_size,measure);
    gtk_entry_set_text ( GTK_ENTRY (size_data), t );
    g_free (t);
    //content_size
    if (archive->dummy_size > 1024*1024*1024 )
    {
        content_size = (double)archive->dummy_size / (1024*1024*1024);
        measure = _(" GB");
    }
        else if (archive->dummy_size > 1024*1024 )
        {
            content_size = (double)archive->dummy_size / (1024*1024);
            measure = _(" MB");
        }

        else if (archive->dummy_size > 1024 )
        {
            content_size = (double)archive->dummy_size / 1024;
            measure = _(" KB");
        }
        else
        {
            measure = _(" bytes");
            content_size = archive->dummy_size;
        }
    t = g_strdup_printf ( "%.1f %s", content_size,measure);
    gtk_entry_set_text ( GTK_ENTRY (content_data), t );
    g_free (t);
    //Compression_ratio
    if (content_size != 0)
		content_size = (double)archive->dummy_size / file_size;
    else
		content_size = 0.0;
    t = g_strdup_printf ( "%.2f", content_size);
    gtk_entry_set_text ( GTK_ENTRY (compression_data), t );
    g_free (t);
    //Number of files
    t = g_strdup_printf ( "%d", archive->nr_of_files);
    gtk_entry_set_text ( GTK_ENTRY (number_of_files_data), t );
    g_free (t);
    //Number of dirs
    t = g_strdup_printf ( "%d", archive->nr_of_dirs);
    gtk_entry_set_text ( GTK_ENTRY (number_of_dirs_data), t );
    g_free (t);
    gtk_widget_show ( archive_properties_win );
}

//Taken from xarchive - http://xarchive.sourceforge.net
int is_escaped_char (char c)
{
    switch ( c )
    {
        case ' ':
        case '\'':
        case '"':
        case '(':
        case ')':
        case '$':
        case '\\':
        case ';':
        case '<':
        case '>':
        case '&':
        case '#':
        case '*':
        case '|':
        case '`':
        case '!':
        return 1;
        default:
        return 0;
    }
}

gchar *EscapeBadChars ( gchar *string )
{
	char *q;
	char *escaped;
	int escapechars = 0;
	char *p = string;

	while (*p != '\000')
	{
        	if (is_escaped_char(*p)) escapechars++;
	        p++;
    }

	if (!escapechars) return g_strdup(string);
	escaped = (char *) g_malloc (strlen(string) + escapechars + 1);

	p = string;
	q = escaped;

	while (*p != '\000')
	{
        if (is_escaped_char(*p)) *q++ = '\\';
		*q++ = *p++;
	}
	*q = '\000';
	return escaped;
}
//End code from xarchive

void Activate_buttons ()
{
	if ( ! GTK_WIDGET_VISIBLE (Extract_button) )
		return;
	
	GtkTreeSelection *selection = gtk_tree_view_get_selection ( GTK_TREE_VIEW (treeview1) );
	gint selected = gtk_tree_selection_count_selected_rows ( selection );
	if (selected == 0 )
		OffDeleteandViewButtons();
	else
	{
		if ( archive->type != XARCHIVETYPE_RPM && archive->type != XARCHIVETYPE_ISO )
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
	gchar *esc_filename = EscapeBadChars ( filename );
	g_string_prepend (data, esc_filename);
	g_string_prepend_c (data, ' ');
	g_free (esc_filename);
}

void ConcatenateFileNames (GtkTreeModel *model, GtkTreePath *treepath, GtkTreeIter *iter, GString *data)
{
	gchar *filename;
	gtk_tree_model_get (model, iter, 0, &filename, -1);
	ConcatenateFileNames2 ( filename , data );
	g_free (filename);
}

void ConcatenateFileNames3 (GtkTreeModel *model, GtkTreePath *treepath, GtkTreeIter *iter, GString *data)
{
	gchar *fullname;

	gtk_tree_model_get (model, iter, 1, &fullname, -1);
	ConcatenateFileNames2 ( fullname , data );
	g_free (fullname);
}

void xa_cat_filenames_for_tar (GtkTreeModel *model, GtkTreePath *treepath, GtkTreeIter *iter, GString *data)
{
	gchar *fullname;
	gchar *name;

	gtk_tree_model_get (model, iter, 1, &fullname, -1);
	name = g_path_get_basename ( fullname );
	g_free (fullname);
	ConcatenateFileNames2 ( name , data );
	g_free (name);
}

void ExtractAddDelete ( gchar *command )
{
	EmptyTextBuffer ();
	archive->parse_output = 0;
	SpawnAsyncProcess ( archive , command , 0, 1);
	if ( archive->child_pid == 0 )
		return;
    if ( ! cli )
		gtk_widget_show ( viewport2 );
    g_child_watch_add ( archive->child_pid, (GChildWatchFunc)xa_watch_child, archive);

}

void Update_StatusBar ( gchar *msg)
{
    gtk_label_set_text (GTK_LABEL (info_label), msg);
}

gboolean xa_report_child_stderr (GIOChannel *ioc, GIOCondition cond, gpointer data)
{
	if (cond & (G_IO_IN | G_IO_PRI) )
	{
		gchar *line = NULL;
		g_io_channel_read_line ( ioc, &line, NULL, NULL, NULL );
		while (gtk_events_pending() )
			gtk_main_iteration();
		if (line == NULL)
			return TRUE;

		gtk_text_buffer_insert (textbuf, &enditer, line, strlen ( line ) );
		//gtk_text_buffer_insert_with_tags_by_name (textbuf, &enditer, line , -1, "red_foreground", NULL);
		g_free (line);
		return TRUE;
	}
	else if (cond & (G_IO_ERR | G_IO_HUP | G_IO_NVAL) )
	{
		g_io_channel_shutdown ( ioc,TRUE,NULL );
        g_io_channel_unref (ioc);
		return FALSE;
	}
	return TRUE;
}

gchar *StripPathFromFilename ( gchar *name )
{
    return g_strrstr ( name , "/" );
}

gchar *JoinPathArchiveName ( const gchar *extract_path , gchar *path )
{
	return g_strconcat (extract_path , path , NULL);
}

void OffDeleteandViewButtons()
{
    gtk_widget_set_sensitive ( Delete_button, FALSE);
    gtk_widget_set_sensitive ( delete_menu, FALSE);
    gtk_widget_set_sensitive ( View_button, FALSE);
    gtk_widget_set_sensitive ( view_menu, FALSE);
}

void OffTooltipPadlock()
{
    gtk_widget_set_sensitive ( Stop_button , FALSE );
    gtk_widget_hide (viewport2);
    if ( ! archive->has_passwd )
    {
        gtk_tooltips_disable ( pad_tooltip );
        gtk_widget_hide ( pad_image );
    }
}

int CountCharacter ( gchar *string , int chr )
{
    int n = 0;
    while ( *string )
    {
        if ( *string == chr ) n++;
        string++;
    }
    return n;
}

gchar *RemoveBackSlashes ( gchar *name)
{
    gchar *nome, *q;
    int x = CountCharacter ( name , '\\' );
    nome = (char *) g_malloc (strlen(name) - x + 1);
    q = nome;
    while ( *name )
    {
        if ( *name == '\\' ) name++;
        *q++ = *name++;
    }
    *q = '\000';
    return nome;
}

/* These three functions are from File-Roller code */
char *eat_spaces (char *line)
{
	if (line == NULL)
		return NULL;
	while ((*line == ' ') && (*line != 0))
		line++;
	return line;
}

gchar *remove_level_from_path (const gchar *path)
{
    const gchar *ptr = path;
    gint p;
    if (! path) return NULL;
    p = strlen (path) - 1;
    if (p < 0) return NULL;
    while ((ptr[p] != '/') && (p > 0))
        p--;
    if ((p == 0) && (ptr[p] == '/')) p++;
    return g_strndup (path, (guint)p);
}
/* End code from File-Roller */

gchar *extract_local_path (gchar *path , gchar *filename)
{
    gchar *local_path;
    gchar *local_escaped_path;

    unsigned short int x;
    x = strlen (path) - strlen ( filename );
    //g_print ("%d\t%d\t%d\n",x,strlen (path),strlen (filename));
    local_path = (gchar *) g_malloc ( x + 1);
    strncpy ( local_path, path, x );
    local_path [x] = '\000';
    local_escaped_path = EscapeBadChars ( local_path );
    g_free (local_path);
    return local_escaped_path;
}

void drag_begin (GtkWidget *treeview1,GdkDragContext *context, gpointer data)
{
    GtkTreeSelection *selection;
    GtkTreeModel     *model;
    GtkTreeIter       iter;
    gchar            *name;
    GList            *row_list = NULL;

    selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (treeview1));

    /* if no or more than one rows selected, do nothing, just for sanity */
    if ( gtk_tree_selection_count_selected_rows (selection) != 1)
		return;

    row_list = gtk_tree_selection_get_selected_rows (selection, &model);
	if ( row_list == NULL )
		return;

	gtk_tree_model_get_iter(model, &iter, row_list->data);

    gtk_tree_path_free(row_list->data);
    g_list_free (row_list);

    gtk_tree_model_get (model, &iter, 0, &name, -1);
	//TODO: fix bug 1879, name must be without path.
    gdk_property_change (context->source_window,
                       gdk_atom_intern ("XdndDirectSave0", FALSE),
                       gdk_atom_intern ("text/plain", FALSE), 8,
                       GDK_PROP_MODE_REPLACE, (guchar *)name, strlen (name) );
    g_free (name);
}

void drag_end (GtkWidget *treeview1,GdkDragContext *context, gpointer data)
{
   /* Nothing to do */
}

void drag_data_get (GtkWidget *widget, GdkDragContext *dc, GtkSelectionData *selection_data, guint info, guint t, gpointer data)
{
    GtkTreeSelection *selection;
    GtkTreeModel *model;
    GtkTreeIter iter;
    guchar *fm_path;
    int fm_path_len;
    gchar *command , *dummy_path , *name;
    gchar *to_send = "E";
    GList *row_list = NULL;

    selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (treeview1));
    /* if no or more than one rows selected, do nothing, just for sanity */
    if ( gtk_tree_selection_count_selected_rows (selection) != 1)
		return;

    row_list = gtk_tree_selection_get_selected_rows (selection, &model);
	if ( row_list == NULL )
		return;

	gtk_tree_model_get_iter(model, &iter, row_list->data);

    gtk_tree_path_free(row_list->data);
    g_list_free (row_list);

    gtk_tree_model_get (model, &iter, 0, &name, -1);
    if ( gdk_property_get (dc->source_window,
                            gdk_atom_intern ("XdndDirectSave0", FALSE),
                            gdk_atom_intern ("text/plain", FALSE),
                            0, 1024, FALSE, NULL, NULL, &fm_path_len, &fm_path)
                            && fm_path != NULL)
    {
        /* Zero-Terminate the string */
        fm_path = g_realloc (fm_path, fm_path_len + 1);
        fm_path[fm_path_len] = '\000';
        dummy_path = g_filename_from_uri ( (gchar*)fm_path, NULL, NULL );
        g_free ( fm_path );
        extract_path = extract_local_path ( dummy_path,name );
        g_free ( dummy_path );
        if (extract_path != NULL)
			to_send = "S";
        names = g_string_new ("");
        gtk_tree_selection_selected_foreach (selection, (GtkTreeSelectionForeachFunc) ConcatenateFileNames, names );
        command = xa_extract_single_files ( archive , names, extract_path );
        if ( command != NULL )
        {
            ExtractAddDelete ( command );
            g_free (command);
        }
        //g_dataset_set_data (dc, "XDS-sent", to_send);
        gtk_selection_data_set (selection_data, gdk_atom_intern ("XA_STRING", FALSE), 8, (guchar*)to_send, 1);
    }
    if (extract_path != NULL)
		g_free (extract_path);
    extract_path = NULL;
    g_free (name);
}

void on_drag_data_received (GtkWidget *widget,GdkDragContext *context, int x,int y,GtkSelectionData *data, unsigned int info, unsigned int time, gpointer user_data)
{
    gchar **array = NULL;
    gchar *filename = NULL;
    gboolean one_file;
    unsigned int len = 0;

    array = gtk_selection_data_get_uris ( data );
    if (array == NULL)
    {
        response = ShowGtkMessageDialog (GTK_WINDOW (MainWindow),GTK_DIALOG_MODAL,GTK_MESSAGE_ERROR,GTK_BUTTONS_OK,_("Sorry, I could not perform the operation!") );
        gtk_drag_finish (context, FALSE, FALSE, time);
        return;
    }
    gtk_drag_finish (context, TRUE, FALSE, time);
    one_file = (array[1] == NULL);
    if (one_file)
    {
        filename = g_filename_from_uri ( array[0] , NULL, NULL );
        if ( filename != NULL)
        {
			xa_open_archive ( NULL, filename );
			g_strfreev ( array );
            return;
        }
    }
    if ( archive == NULL)
		xa_new_archive ( NULL , NULL );
    if ( archive->type != XARCHIVETYPE_UNKNOWN )
    {
        if ( (archive->type == XARCHIVETYPE_BZIP2 || archive->type == XARCHIVETYPE_GZIP) && ! one_file)
        {
            response = ShowGtkMessageDialog (GTK_WINDOW (MainWindow),GTK_DIALOG_MODAL,GTK_MESSAGE_ERROR,GTK_BUTTONS_OK,_("Bzip2 or gzip cannot compress more than one file, please choose another archive format!") );
            gtk_window_set_title ( GTK_WINDOW (MainWindow) , "Xarchiver " VERSION );
            Update_StatusBar ( _("Operation failed."));
            return;
        }
        while (array[len])
        {
            filename = g_filename_from_uri ( array[len] , NULL, NULL );
            Files_to_Add = g_slist_prepend ( Files_to_Add , filename );
            len++;
        }
        xa_add_files_archive ( NULL, NULL );
        g_strfreev ( array );
    }
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

void xa_append_rows ( XArchive *archive , unsigned short int nc )
{
	unsigned short int i = 0;
	
	if (archive->row == NULL)
		return;
	archive->row = g_list_reverse ( archive->row );

	gtk_list_store_append (liststore, &iter);
	while ( archive->row )
	{
		if ( i == nc )
		{
			gtk_list_store_append (liststore, &iter);
			i = 0;
		}
		else
		{
			gtk_list_store_set_value(liststore, &iter, i, archive->row->data);
			archive->row = archive->row->next;
			i++;
		}
	}
	while ( gtk_events_pending() )
		gtk_main_iteration();

	g_list_foreach(archive->row, (GFunc)g_value_unset, NULL);
	g_list_foreach(archive->row, (GFunc)g_free, NULL);
	g_list_free(archive->row);
	archive->row = NULL;
}
