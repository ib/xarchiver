/*
 *  Xarchiver
 *
 *  Copyright (C) 2005 Giuseppe Torelli - Colossus
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
#include <config.h>
#endif

#include "callbacks.h"
#include "interface.h"
#include "support.h"
#include "main.h"

extern GList *ArchiveName;
extern GList *ArchiveSuffix;
extern gboolean file_to_open;
extern gchar *tmp;

gchar *CurrentFolder = NULL;
GList *Suffix , *Name;

GChildWatchFunc *ExitStatus (GPid pid , gint status , gpointer temp)
{
    //temp contains the filename created in /tmp to add/delete on tar.bzip2 / tar.gzip archives (look in bzip.c )
    if ( temp != NULL)
    {
        unlink ( temp );
        g_free (temp);
    }
	if ( WIFEXITED(status) )
	{
		if ( WEXITSTATUS (status) )
		{
			archive_error = TRUE;
			SetButtonState (1,1,0,0,0);
			gtk_window_set_title ( GTK_WINDOW (MainWindow) , "Xarchiver " VERSION );
			response = ShowGtkMessageDialog (GTK_WINDOW
			(MainWindow),GTK_DIALOG_MODAL,GTK_MESSAGE_QUESTION,GTK_BUTTONS_YES_NO,_("An error occurred while reading the archive.\nDo you want to view the shell output ?") );
			if (response == GTK_RESPONSE_YES) ShowShellOutput();
			return;
		}
	}
	archive_error = FALSE;
	//This to automatically reload the content of the archive after adding or deleting
	if (action == add || action == delete)
	{
        RemoveColumnsListStore();
		switch ( CurrentArchiveType )
		{
			//case 0:
			//Bzip2: not needed
			
			//case 1:
			//Gzip: not needed
			
			case 2:
			OpenRar ( FALSE , path );
			break;

			case 3:
			OpenTar ( FALSE , path );
			break;

            case 4:
            OpenBzip2 ( FALSE , path );
            break;

            case 5:
            OpenGzip ( FALSE , path );
            break;

            case 7:
			case 8:
			OpenZip ( FALSE , path );
			break;

            //case 9:
            //RPM: only open and extract
		}
		action = inactive;
	}
    //This to enable only the Extract button in case an RPM was opened
    if (CurrentArchiveType == 9)
    {
        gtk_window_set_title ( GTK_WINDOW (MainWindow) , path );
        SetButtonState (1,1,0,0,1);
        return;
    }
	if ( ! bz_gz )
    {
        gtk_window_set_title ( GTK_WINDOW (MainWindow) , path );
	    SetButtonState ( 1,1,1,1,1 );
    }
    //This to disable the Add and Delete buttons in case a bzip2 / gzip file has been decompressed
    else SetButtonState (1,1,0,0,0);
}

void on_new1_activate (GtkMenuItem *menuitem, gpointer user_data)
{
	path = Show_File_Dialog ( 1 , "new" );
	if (path == NULL) return;
	if ( g_file_test ( path , G_FILE_TEST_EXISTS ) )
	{
		response = ShowGtkMessageDialog (GTK_WINDOW (MainWindow),
						GTK_DIALOG_MODAL,
						GTK_MESSAGE_QUESTION,
						GTK_BUTTONS_YES_NO,
						_("This archive already exists. Do you want to overwrite it ?")
						);
		if (response != GTK_RESPONSE_YES)
		{
			g_free (path);
			return;
		}
        unlink ( path );
	}
    //This to delete a CPIO temp file in /tmp if existing
    if ( tmp != NULL )
    {
        unlink (tmp);
        tmp = NULL;
    }
	if ( liststore != NULL )
	{
		RemoveColumnsListStore();
		EmptyTextBuffer();
	}
	gtk_window_set_title ( GTK_WINDOW ( MainWindow) , path );
	if ( CurrentArchiveType == 0 || CurrentArchiveType == 1 )
    {
        bz_gz = TRUE;
        SetButtonState (1,1,1,0,0 );
    }
    else
    {
        SetButtonState (1,1,1,1,0 );
        bz_gz = FALSE;
    }
    if ( CurrentArchiveType == 2 || CurrentArchiveType == 7 || CurrentArchiveType == 8) gtk_widget_set_sensitive ( add_pwd , TRUE );
        else gtk_widget_set_sensitive ( add_pwd , FALSE );
    if (password != NULL) g_free (password);
    password = NULL;
	//g_print ("You choose to create %d of type %s\n",CurrentArchiveType,g_list_nth_data (ArchiveName,CurrentArchiveType));
}

int ShowGtkMessageDialog ( GtkWindow *window, int mode,int type,int button, gchar *message)
{
	dialog = gtk_message_dialog_new (window, mode, type, button,message);
	gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_YES);
	response = gtk_dialog_run (GTK_DIALOG (dialog) );
	gtk_widget_destroy (GTK_WIDGET (dialog) );
	return response;
}

void on_open1_activate (GtkMenuItem *menuitem, gpointer user_data)
{
	if ( ! file_to_open )
	{
		path = Show_File_Dialog ( 1 , "open" );
		if (path == NULL) return;
	}
	file_to_open = FALSE;
	if ( liststore != NULL )
	{
		RemoveColumnsListStore();
		EmptyTextBuffer();
	}
    if ( CurrentArchiveType == 9 ) unlink (tmp);
	CurrentArchiveType = DetectArchiveType ( path );
	path = EscapeBadChars ( path );
	bz_gz = FALSE;
	switch ( CurrentArchiveType )
	{
		case 0:
		OpenBzip2 ( TRUE , path );
		break;

		case 1:
		OpenGzip ( TRUE , path );
		break;

		case 2:
		OpenRar ( TRUE , path );
		break;

		case 3:
		OpenTar ( TRUE , path );
		break;

		case 8:
		OpenZip ( TRUE , path );
		break;

        case 9:
        OpenRPM ( TRUE , path );
        break;
	}
    if ( CurrentArchiveType == 2 || CurrentArchiveType == 7 || CurrentArchiveType == 8) gtk_widget_set_sensitive ( add_pwd , TRUE );
        else gtk_widget_set_sensitive ( add_pwd , FALSE );
    if (password != NULL) g_free (password);
    password = NULL;
	//g_print ("Archive Type: %d\n" , CurrentArchiveType);
}

void on_quit1_activate (GtkMenuItem *menuitem, gpointer user_data)
{
	g_list_free ( Suffix);
	g_list_free ( Name);
	if ( path != NULL) g_free (path);
	gtk_main_quit();
}

void on_delete1_activate (GtkMenuItem *menuitem, gpointer user_data)
{
	gchar *command = NULL;
	GtkTreeSelection *selection = gtk_tree_view_get_selection ( GTK_TREE_VIEW (treeview1) );
	names = g_string_new ( " " );
	gtk_tree_selection_selected_foreach (selection, (GtkTreeSelectionForeachFunc) ConcatenateFileNames, names );
	action = delete;
	switch (CurrentArchiveType)
	{
		case 2:
		command = g_strconcat ( "rar d " , path , names->str , NULL );
		break;
		
        case 3:
		command = g_strconcat ( "tar --delete -vf " , path , names->str , NULL );
		break;
		
        case 8:
		command = g_strconcat ( "zip -d " , path , names->str , NULL );
		break;
		
        case 4:
        DecompressBzipGzip ( names , path , 0  , 0 );
        break;
		
        case 5:
        DecompressBzipGzip ( names , path , 1 , 0 );
		break;	
	}
	if (CurrentArchiveType == 2 || CurrentArchiveType == 3 || CurrentArchiveType == 8)
    {
        ExtractAddDelete ( command );
        g_free (command);
    }
	gtk_widget_set_sensitive (Delete_button , FALSE);
	gtk_widget_set_sensitive (delete_menu, FALSE);
	g_string_free (names , FALSE );
}

void on_add_files_activate ( GtkMenuItem *menuitem, gpointer user_data )
{
    gchar *name = NULL;
	gchar *command = NULL;
	GSList *Files_to_Add  = Add_File_Dialog ( user_data );
	if ( Files_to_Add == NULL) return;
	names = g_string_new ( " " );
    Files_to_Add = g_slist_reverse (Files_to_Add);
    //Set the current dir so to avoid archiving the leading directory inside the archive
    name = g_path_get_dirname ( Files_to_Add->data );
	chdir ( name );
    g_free ( name );
    while ( Files_to_Add != NULL )
	{
        //Strip the path from the filename
        name = g_path_get_basename ( Files_to_Add->data );
        ConcatenateFileNames2 ( name , names );
        Files_to_Add = g_slist_next ( Files_to_Add );
	}	
		action = add;	
		switch (CurrentArchiveType)
		{
			case 0:
			Bzip2Add ( names->str , 0 );
			break;
            
			case 1:
			Bzip2Add ( names->str , 1 );
			break;
			
			case 2:
            if (password != NULL) command = g_strconcat ( "rar a -p" , password, " -o+ -ep1 -idp " , path , names->str , NULL );
            else command = g_strconcat ( "rar a -o+ -ep1 -idp " , path , names->str , NULL );
			ExtractAddDelete ( command );
			break;
			
			case 3:
			if ( g_file_test ( path , G_FILE_TEST_EXISTS ) ) command = g_strconcat ( "tar rvvf " , path , names->str , NULL );
            else
                command = g_strconcat ( "tar cvvf " , path , names->str , NULL );
            ExtractAddDelete ( command );
    		break;
			
            case 4:
            if ( g_file_test ( path , G_FILE_TEST_EXISTS ) )
            {
                DecompressBzipGzip ( names , path , 0 , 1 );
            }
            else
            {
                command = g_strconcat ("tar cvvfj " , path , names->str , NULL );
                ExtractAddDelete ( command );
            }
            break;

            case 5:
            if ( g_file_test ( path , G_FILE_TEST_EXISTS ) )
            {
                DecompressBzipGzip ( names , path , 1 , 1 );
            }
            else
            { 
                command = g_strconcat ("tar cvvfz " , path , names->str , NULL );
                ExtractAddDelete ( command );
            }
            break;

			case 7:
			case 8:
            if (password != NULL) command = g_strconcat ( "zip -P " , password , " -r " , path , names->str , NULL );
                else command = g_strconcat ( "zip -r " , path , names->str , NULL );
			ExtractAddDelete ( command );
			break;
		}
	g_string_free (names , FALSE );
	g_slist_free ( Files_to_Add );
	g_free (command);
}

void on_extract1_activate ( GtkMenuItem *menuitem , gpointer user_data )
{
	gchar *command = NULL;
	GtkTreeSelection *selection = gtk_tree_view_get_selection ( GTK_TREE_VIEW (treeview1) );
	gint selected = gtk_tree_selection_count_selected_rows ( selection );
	extract_window = prefs (selected);
	gtk_dialog_set_default_response (GTK_DIALOG (extract_window), GTK_RESPONSE_OK);
    if ( PasswordProtectedArchive )
    {
            Show_pwd_Window ( NULL , NULL );
            if ( password == NULL) return;
    }
    done = FALSE;
    while ( ! done )
	{
		switch (gtk_dialog_run ( GTK_DIALOG (extract_window ) ) )
		{
			case GTK_RESPONSE_CANCEL:
			case GTK_RESPONSE_DELETE_EVENT:
			done = TRUE;
			break;
			
			case GTK_RESPONSE_OK:
			extract_path = gtk_entry_get_text ( GTK_ENTRY (entry1) );
			if ( strlen ( extract_path ) > 0 )
			{
				done = TRUE;
				if ( selected < 1 )
				{
					switch ( CurrentArchiveType )
					{
						case 0:
						//Bzip2 extraction is handled when the the file is opened
						//code execution never reaches here
						break;
						
						case 1:
						//Gzip extraction is handled when the the file is opened
						//code execution never reaches here
						break;
						
						case 2:
                        if (password !=NULL) command = g_strconcat ( "rar x -p",password," -o+ -idp " , path , " " , extract_path , NULL );
                        else command = g_strconcat ( "rar x -o+ -idp " , path , " " , extract_path , NULL );
						break;
						
						case 3:
						command = g_strconcat ( "tar xfv " , path , " -C " , extract_path , NULL );
						break;

						case 4:
						command = g_strconcat ( "tar xfjv " , path , " -C " , extract_path , NULL );
						break;
						
						case 5:
						command = g_strconcat ( "tar xfzv " , path , " -C " , extract_path , NULL );
						break;

                        case 7:
						case 8:
                        if ( password != NULL ) command = g_strconcat ( "unzip -P " , password , " " , path , " -d " , extract_path , NULL );
                            else command = g_strconcat ( "unzip " , path , " -d " , extract_path , NULL );
						break;

                        case 9:
                        chdir ( extract_path );
                        SpawnCPIO ( "cpio -id" , tmp , 0 , 1 );
                        break;
					}
                    if ( ! CurrentArchiveType == 9)
                    {
                        ExtractAddDelete ( command );
                        g_free (command); 
                    }
				}
				else
				{
					names = g_string_new ( " " );
					gtk_tree_selection_selected_foreach (selection, (GtkTreeSelectionForeachFunc) ConcatenateFileNames, names );
					switch (CurrentArchiveType)
					{
						case 0:
						//Bzip2 extraction is handled when the the file is opened
						//code execution never reaches here
						break;
						
						case 1:
                        //Gzip extraction is handled when the the file is opened
						//code execution never reaches here
						break;
						
						case 2:
						if (password != NULL) command = g_strconcat ( "rar x -p",password, " -o+ -idp " , path , " " , names->str , extract_path , NULL );
                        else command = g_strconcat ( "rar x -o+ -idp " , path , " " , names->str , extract_path ,NULL);
						break;
						
						case 3:
					    command = g_strconcat ( "tar xfv " , path , " -C " , extract_path , names->str , NULL );
						break;
						
						case 4:
						command = g_strconcat ( "tar xfjv " , path , " -C " , extract_path ,  names->str , NULL );
						break;

						case 5:
                        command = g_strconcat ( "tar xfvz " , path , " -C " , extract_path , names->str , NULL );
						break;

                        case 7:
						case 8:
                       if ( password != NULL ) command = g_strconcat ( "unzip -P " , password , " " , path , names->str , " -d " , extract_path , NULL );
                            else command = g_strconcat ( "unzip " , path , names->str , " -d " , extract_path , NULL );
						break;
                        
                        case 9:
                        chdir ( extract_path );
                        command = g_strconcat ( "cpio -id " , names->str , NULL );
                        SpawnCPIO ( command , tmp , 0 , 1 );
                        g_free (command);
                        break;
					}
                    if ( ! CurrentArchiveType == 9)
                    {
                        ExtractAddDelete ( command );
                        g_free (command); 
                    }
                    g_string_free (names , FALSE );
				}
			}
			else response = ShowGtkMessageDialog (GTK_WINDOW (MainWindow),GTK_DIALOG_MODAL,GTK_MESSAGE_ERROR,GTK_BUTTONS_OK, "Please select where to extract files !" );
			break;
		}
	}
	gtk_widget_destroy ( extract_window );
}

void on_about1_activate (GtkMenuItem *menuitem, gpointer user_data)
{
	static GtkWidget *about = NULL;
	const char *authors[] = {"Giuseppe Torelli - Colossus\n<gt67@users.sourceforge.net>\n\nThanks to Chris Vine, Andreas Stricker\nand Olivier Sessink", NULL};
	if (about != NULL)
	{
		gtk_window_present (GTK_WINDOW (about));
		return;
	}
	about = gtk_about_dialog_new ();
	g_object_set (about,
		      "name",  "Xarchiver",
		      "version", VERSION,
		      "copyright", "Copyright @2005 Giuseppe Torelli",
		      "comments", "A GTK+2 only archive manager for Linux.",
		      "authors", authors,
		      "documenters", NULL,
		      "translator_credits", NULL,
		      "logo_icon_name", "xarchiver",
		      "website", "http://xarchiver.sourceforge.net",
		      "website_label", "Home Page",
		      "license",    "Copyright @2005 Giuseppe Torelli - Colossus <gt67@users.sourceforge.net>\n\n"
		      			"This library is free software; you can redistribute it and/or\n"
    					"modify it under the terms of the GNU Library General Public License as\n"
    					"published by the Free Software Foundation; either version 2 of the\n"
    					"License, or (at your option) any later version.\n"
    					"\n"
    					"This library is distributed in the hope that it will be useful,\n"
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


void on_options1_activate (GtkMenuItem *menuitem, gpointer user_data)
{
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
	if (CurrentArchiveType == 0 || CurrentArchiveType == 1 ) gtk_file_chooser_set_select_multiple ( GTK_FILE_CHOOSER (File_Selector) , FALSE );
        else gtk_file_chooser_set_select_multiple ( GTK_FILE_CHOOSER (File_Selector) , TRUE );
	if (CurrentFolder != NULL) gtk_file_chooser_set_current_folder ( GTK_FILE_CHOOSER (File_Selector) , CurrentFolder );
	response = gtk_dialog_run (GTK_DIALOG (File_Selector) );
	if (response == GTK_RESPONSE_ACCEPT) list = gtk_file_chooser_get_filenames ( GTK_FILE_CHOOSER (File_Selector) );
	gtk_widget_destroy (File_Selector);
	return list;
}

gchar *Show_File_Dialog ( int dummy , gpointer mode )
{
	GtkWidget *hbox;
	GtkWidget *combo_box;
	GtkWidget *archive_types;
	GtkWidget *check_button;
	GtkFileFilter *filter;
	GtkTooltips *FilterToolTip ;
	gchar *path;
	const gchar *flag2;
	int i , flag;
	
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
		response = gtk_dialog_run (GTK_DIALOG (File_Selector));
		if (response == GTK_RESPONSE_ACCEPT) gtk_entry_set_text (GTK_ENTRY(entry1),gtk_file_chooser_get_filename ( GTK_FILE_CHOOSER (File_Selector) ) );
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
	if (CurrentFolder != NULL) gtk_file_chooser_set_current_folder ( GTK_FILE_CHOOSER (File_Selector) , CurrentFolder );
	gtk_dialog_set_default_response (GTK_DIALOG (File_Selector), GTK_RESPONSE_ACCEPT);
	
	filter = gtk_file_filter_new ();
	gtk_file_filter_set_name ( filter , _("All files") );
	gtk_file_filter_add_pattern ( filter, "*" );
	gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (File_Selector), filter);
	
	filter = gtk_file_filter_new ();
	gtk_file_filter_set_name ( filter , _("Only archives") );
	
	Suffix = g_list_first ( ArchiveSuffix );
	Name = g_list_first ( ArchiveName );
	
	while ( Suffix != NULL )
	{
		gtk_file_filter_add_pattern (filter, Suffix->data);
		Suffix = g_list_next ( Suffix );
	}
	gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (File_Selector), filter);	
	
	Suffix = g_list_first ( ArchiveSuffix );
		
	while ( Suffix != NULL )
	{
		if ( Suffix->data != "" )	//To avoid double filtering when opening the archive
		{
			filter = gtk_file_filter_new ();
			gtk_file_filter_set_name (filter, Name->data );
			gtk_file_filter_add_pattern (filter, Suffix->data );
			gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (File_Selector), filter);	
		}
		
		Suffix = g_list_next ( Suffix );
		Name = g_list_next ( Name );
	}
	
	if ( mode == "new" )
	{
		hbox = gtk_hbox_new (FALSE, 12);
		gtk_box_pack_start (GTK_BOX (hbox),gtk_label_new (_("Archive type:")),FALSE, FALSE, 0);
		combo_box = gtk_combo_box_new_text ();
		FilterToolTip = gtk_tooltips_new();
		gtk_tooltips_set_tip (FilterToolTip,combo_box, _("Choose the archive type to create") , NULL);
		Name = g_list_first ( ArchiveName );
		while ( Name != NULL )
		{
			if (Name->data != ".tgz" && Name->data != ".rpm" ) gtk_combo_box_append_text (GTK_COMBO_BOX (combo_box), Name->data );
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
			CurrentArchiveType = gtk_combo_box_get_active (GTK_COMBO_BOX (combo_box));
			if (CurrentArchiveType == 6) CurrentArchiveType++;		//This to avoid return .tgz
			else if (CurrentArchiveType == 7) CurrentArchiveType = 8;	//This to avoid return .jar instead of .zip
			if ( gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(check_button) ) )
			{
				if ( ! g_str_has_suffix ( path , g_list_nth_data (ArchiveName,CurrentArchiveType ) ) )
				{
					gtk_widget_destroy (File_Selector);
					return g_strconcat ( path, g_list_nth_data (ArchiveName,CurrentArchiveType) , NULL);
				}
				
			}
		}
		gtk_widget_destroy (File_Selector);
		return ( path );
	}
	else if ( (response == GTK_RESPONSE_CANCEL) || (response == GTK_RESPONSE_DELETE_EVENT) )
	{
		gtk_widget_destroy (File_Selector);
		return NULL;
	}
}

gboolean isTar ( FILE *ptr )
{
	unsigned char magic[7];
	if (fseek ( ptr, 253 , SEEK_CUR ) ) return FALSE;
	if ( fread ( magic, 1, 7, ptr ) == 0 )
	{
		fclose ( ptr );
		response = ShowGtkMessageDialog (GTK_WINDOW (MainWindow),GTK_DIALOG_MODAL,GTK_MESSAGE_ERROR,GTK_BUTTONS_OK,(char *) g_strerror(errno) );
		return FALSE;
	}
	if ( memcmp ( magic,"\x75\x73\x74\x61\x72\x00\x30",7 ) == 0 || memcmp (magic,"\x75\x73\x74\x61\x72\x20\x20",7 ) == 0) return TRUE;
		else return FALSE;
}

int DetectArchiveType ( gchar *path )
{
	FILE *dummy_ptr = NULL;
    int xx = -1;
	unsigned char magic[4];
	dummy_ptr = fopen ( path , "r" );
	if (dummy_ptr == NULL)
	{
		gchar *msg = g_strdup_printf (_("Can't open archive %s:\n%s") , path , g_strerror (errno) ); 
		response = ShowGtkMessageDialog (GTK_WINDOW (MainWindow) , GTK_DIALOG_MODAL,GTK_MESSAGE_ERROR,GTK_BUTTONS_OK,
		msg);
		g_free (msg);
		return xx;
	 }
	if ( fread ( magic, 1, 4, dummy_ptr ) == 0 )
	{
		fclose ( dummy_ptr);
		response = ShowGtkMessageDialog (GTK_WINDOW (MainWindow),GTK_DIALOG_MODAL,GTK_MESSAGE_ERROR,GTK_BUTTONS_OK,(char *)g_strerror(errno));
		return xx;
	}
    if ( memcmp ( magic,"\x50\x4b\x03\x04",4 ) == 0 || memcmp ( magic,"\x50\x4b\x05\x06",4 ) == 0 )
    {
        PasswordProtectedArchive = DetectPasswordProtectedArchive ( dummy_ptr );
        xx = 8;	// zip
    }
	else if ( memcmp ( magic,"\x52\x61\x72\x21",4 ) == 0 ) xx = 2;	// rar
    else if ( memcmp ( magic,"\x42\x5a\x68\x39",4 ) == 0 ) xx = 0;	// bzip2
	else if ( memcmp ( magic,"\x1f\x8b\x08",3 ) == 0) xx = 1;		// gz
    else if ( memcmp ( magic,"\xed\xab\xee\xdb",4 ) == 0) xx = 9;	// rpm
    //else if ( memcmp ( magic,"\x52\x61\x72\x21",4 ) == 0 ) xx = ??;	// 7zip
    else if ( isTar ( dummy_ptr ) ) xx = 3;
	else
	{
		gtk_window_set_title ( GTK_WINDOW (MainWindow) , "Xarchiver " VERSION );
		response = ShowGtkMessageDialog (GTK_WINDOW (MainWindow),GTK_DIALOG_MODAL,GTK_MESSAGE_ERROR,GTK_BUTTONS_OK,
		_("The format of this archive is not recognized !") );
	}
	fclose ( dummy_ptr );
	return xx;
}

gboolean DetectPasswordProtectedArchive ( FILE *stream )
{
    unsigned char bytes[1];
    //This is for zip archives; rar encrypted archives contains an asterisk just before the filenames
    fseek ( stream, 6 , SEEK_SET );
    fread ( bytes, 1, 1, stream );
    return (( bytes[0] & (1<<0)) > 0);
}

gulong SpawnAsyncProcess (const gchar *command , gboolean ExitStatusFlag , gboolean input)
{
	GError *error = NULL;
	gchar **argv;
	int argcp;
	g_shell_parse_argv ( command , &argcp , &argv , NULL);
	if ( ! g_spawn_async_with_pipes (
		NULL,
		argv,
		NULL,
		G_SPAWN_SEARCH_PATH | G_SPAWN_DO_NOT_REAP_CHILD,
		NULL,
		NULL,
		&child_pid,
		input ? &input_fd : NULL,
		&output_fd,
		&error_fd,
		&error) )
	{
		response = ShowGtkMessageDialog (GTK_WINDOW (MainWindow),GTK_DIALOG_MODAL,GTK_MESSAGE_ERROR,GTK_BUTTONS_OK, error->message);
		g_error_free (error);
		g_strfreev ( argv );
        return FALSE;
	}
	g_strfreev ( argv );
    //The following callback is needed for evaluating the exit status of the launched compressor
	if ( ExitStatusFlag ) g_child_watch_add  ( child_pid , (GChildWatchFunc) ExitStatus , NULL );
	return (gulong) child_pid;
}

GIOChannel *SetIOChannel (gint fd, GIOCondition cond, GIOFunc func, gpointer data)
{
	GIOChannel *ioc;
	ioc = g_io_channel_unix_new ( fd );
	g_io_add_watch (ioc, cond, func, data);
	g_io_channel_set_encoding (ioc, "ISO8859-1" , NULL);
	g_io_channel_set_flags ( ioc , G_IO_FLAG_NONBLOCK , NULL );
	return ioc;
}

GIOChannel *SetIOChannelEncondingNULL (gint fd, GIOCondition cond, GIOFunc func, gpointer data)
{
	GIOChannel *ioc;
	ioc = g_io_channel_unix_new ( fd );
	g_io_add_watch (ioc, cond, func, data);
	g_io_channel_set_encoding (ioc, NULL , NULL);
	g_io_channel_set_flags ( ioc , G_IO_FLAG_NONBLOCK , NULL );
	return ioc;
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


char *get_last_field (char *line,int last_field)
{
	char *field;
	int i;

	if (line == NULL)
		return NULL;

	last_field--;
	field = eat_spaces (line);
	for (i = 0; i < last_field; i++) {
		if (field == NULL)
			return NULL;
		field = strchr (field, ' ');
		field = eat_spaces (field);
	}
	//The following line is mine, I replace the \n with the null terminated
	field [ strlen(field) -1 ] = '\0';
	return field;
}

char **split_line (char *line,int n_fields)
{
	char **fields;
	char *scan, *field_end;
	int i;

	fields = g_new0 (char *, n_fields + 1);
	fields[n_fields] = NULL;

	scan = eat_spaces (line);
	for (i = 0; i < n_fields; i++)
	{
		if (scan == NULL)
		{
			fields[i] = NULL;
			continue;
		}
		field_end = strchr (scan, ' ');
		//The following line is mine, I added the case when the last field ends with a newline
		if (field_end == NULL) field_end = strchr (scan, '\n');
		if (field_end != NULL)
		{
			fields[i] = g_strndup (scan, field_end - scan);
			scan = eat_spaces (field_end);
		}
	}

	return fields;
}
/* End code from File-Roller */

void RemoveColumnsListStore()
{
    SetButtonState (1,1,0,0,0);
	gtk_window_set_title ( GTK_WINDOW (MainWindow) , "Xarchiver " VERSION );
	GList *columns = gtk_tree_view_get_columns ( GTK_TREE_VIEW (treeview1) );
	while (columns != NULL)
	{
		gtk_tree_view_remove_column (GTK_TREE_VIEW (treeview1) , columns->data);
		columns = columns->next;
	}
	g_list_free (columns);
}

void EmptyTextBuffer()
{
	if (textbuf != NULL)
	{
		gtk_text_buffer_get_start_iter (textbuf,&start);
		gtk_text_buffer_get_end_iter (textbuf,&end);
		gtk_text_buffer_delete (textbuf,&start,&end);
		gtk_text_buffer_get_start_iter(textbuf, &enditer);
	}
}

void CreateListStore ( int nc, gchar *columns_names[] , GType columns_types[])
{
	liststore = gtk_list_store_newv ( nc , (GType *)columns_types);
	gtk_tree_view_set_model ( GTK_TREE_VIEW (treeview1), GTK_TREE_MODEL (liststore) );
	gtk_tree_view_set_rules_hint ( GTK_TREE_VIEW (treeview1) , TRUE );
	GtkTreeSelection *sel = gtk_tree_view_get_selection( GTK_TREE_VIEW (treeview1) );
	gtk_tree_selection_set_mode(sel, GTK_SELECTION_MULTIPLE);
	g_signal_connect ((gpointer) sel, "changed", G_CALLBACK (Activate_delete_button), NULL);
	for (x = 0; x <= nc-1; x++)
	{
		renderer = gtk_cell_renderer_text_new ();
		column = gtk_tree_view_column_new_with_attributes ( columns_names[x],renderer,"text",x,NULL);
		gtk_tree_view_column_set_resizable (column, TRUE);
		gtk_tree_view_column_set_sort_column_id (column, x);
		gtk_tree_view_append_column (GTK_TREE_VIEW (treeview1), column);
	}
}

void ShowShellOutput ()
{
	if (OutputWindow != NULL)
	{
		gtk_window_present ( GTK_WINDOW (OutputWindow) ); 
		return;
	}
	OutputWindow = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	gtk_widget_set_name (OutputWindow, "OutputWindow");
	gtk_window_set_title (GTK_WINDOW (OutputWindow), _("Shell output") );
	gtk_window_set_position (GTK_WINDOW (OutputWindow), GTK_WIN_POS_CENTER);
	gtk_window_set_default_size(GTK_WINDOW(OutputWindow), 450, 300);
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
	gtk_text_buffer_create_tag (textbuf, "red_foreground","foreground", "red", NULL);
	
	gtk_widget_show (vbox);
	gtk_widget_show (scrollwin);
	gtk_widget_show (textview);
}

void Show_pwd_Window ( GtkMenuItem *menuitem , gpointer user_data )
{
    pwd_window = passwd_win ();
    password_entry = lookup_widget ( pwd_window , "pwd_entry" );
    repeat_password = lookup_widget ( pwd_window , "entry2");
        
	gtk_dialog_set_default_response (GTK_DIALOG (pwd_window), GTK_RESPONSE_OK);
    done = FALSE;
	while ( ! done )
	{
		switch ( gtk_dialog_run ( GTK_DIALOG ( pwd_window ) ) )
		{
			case GTK_RESPONSE_CANCEL:
			case GTK_RESPONSE_DELETE_EVENT:
			done = TRUE;
			break;
			
			case GTK_RESPONSE_OK:
			password  = g_strdup ( gtk_entry_get_text( GTK_ENTRY ( password_entry ) ) );
            if ( strcmp (password , gtk_entry_get_text( GTK_ENTRY ( repeat_password ) ) ) )
            {
                response = ShowGtkMessageDialog (GTK_WINDOW (MainWindow),GTK_DIALOG_MODAL,GTK_MESSAGE_ERROR,GTK_BUTTONS_OK,"The passwords don't match !!" );
                gtk_entry_set_text ( GTK_ENTRY ( password_entry ) , "" );
                gtk_entry_set_text ( GTK_ENTRY ( repeat_password ) , "" );
            }
            else done = TRUE;
            break;
        }
    }
    gtk_widget_destroy ( pwd_window );
}

//Taken from xarchive - http://xarchive.sourceforge.net
int is_escaped_char(char c)
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

	if (!escapechars) return strdup(string);

	escaped = (char *) malloc (strlen(string) + escapechars + 1);

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

void Activate_delete_button ()
{
	if ( archive_error ) return;
	GtkTreeSelection *selection = gtk_tree_view_get_selection ( GTK_TREE_VIEW (treeview1) );
	gint selected = gtk_tree_selection_count_selected_rows ( selection );
	if (selected == 0 )
	{
		gtk_widget_set_sensitive (Delete_button , FALSE);
		gtk_widget_set_sensitive ( delete_menu, FALSE);
	}
	else
	{
		gtk_widget_set_sensitive ( delete_menu , TRUE);
		gtk_widget_set_sensitive (Delete_button , TRUE);
	}
}

void ConcatenateFileNames2 (gchar *filename , GString *data)
{
	gchar *esc_filename = EscapeBadChars ( filename );
	g_string_prepend (data, esc_filename);
	g_string_prepend_c (data, ' ');
	g_free (esc_filename);
	g_free (filename);
}

void ConcatenateFileNames (GtkTreeModel *model, GtkTreePath *treepath, GtkTreeIter *iter, GString *data)
{
	gchar *filename;
	gtk_tree_model_get (model, iter, 0, &filename, -1);
	ConcatenateFileNames2 ( filename , data );
}

void ExtractAddDelete ( gchar *command )
{
	EmptyTextBuffer();
	compressor_pid = SpawnAsyncProcess ( command , 1 , 0);
	if ( compressor_pid == 0 ) return;
	SetIOChannel (output_fd, G_IO_IN|G_IO_PRI|G_IO_ERR|G_IO_HUP|G_IO_NVAL , GenOutput, NULL );
	SetIOChannel (error_fd, G_IO_IN|G_IO_PRI|G_IO_ERR|G_IO_HUP|G_IO_NVAL , GenError, NULL );
}


gboolean GenError (GIOChannel *ioc, GIOCondition cond, gpointer data)
{
	if (cond & (G_IO_IN | G_IO_PRI) )
	{
		while (gtk_events_pending() )
			gtk_main_iteration();
		gchar *line = NULL;
		g_io_channel_read_line ( ioc, &line, NULL, NULL, NULL );
		if (line != NULL && strcmp (line,"\n") )
		{
			gtk_text_buffer_insert_with_tags_by_name (textbuf, &enditer, line , -1, "red_foreground", NULL);
			g_free (line);
		}
		return TRUE;
	}
	else if (cond & (G_IO_ERR | G_IO_HUP | G_IO_NVAL) )
	{
		g_io_channel_shutdown ( ioc,TRUE,NULL );
        g_io_channel_unref (ioc);
		g_spawn_close_pid ( child_pid );
		return FALSE;
	}
}

gboolean GenOutput (GIOChannel *ioc, GIOCondition cond, gpointer data)
{
	gchar *line = NULL;
	if (cond & (G_IO_IN | G_IO_PRI) )
	{
		while (gtk_events_pending() )
			gtk_main_iteration();
		g_io_channel_read_line ( ioc, &line, NULL, NULL, NULL );
		if (line != NULL )
		{
				gtk_text_buffer_insert (textbuf, &enditer, line, strlen ( line ) );
				g_free (line);
		}
	}
	
	else if (cond & (G_IO_ERR | G_IO_HUP | G_IO_NVAL) )
	{
	    g_io_channel_shutdown ( ioc,TRUE,NULL );
        g_io_channel_unref (ioc);
		g_spawn_close_pid ( child_pid );
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
	gchar *full_path = NULL;
	full_path = g_strconcat (extract_path,path,NULL);
	return full_path;
}
