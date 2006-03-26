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
#include "config.h"
#endif

#include "callbacks.h"
#include "interface.h"
#include "support.h"
#include "main.h"

extern GList *ArchiveType;
extern GList *ArchiveSuffix;
extern gchar *tmp;

gchar *CurrentFolder = NULL;
GList *Suffix , *Name;

void WaitExitStatus ( GPid child_pid , gchar *temp_file)
{
    int status;
    int waiting = TRUE;
    int ps;

    OffDeleteandViewButtons();
    while (waiting)
    {
        ps = waitpid ( (pid_t)child_pid, &status, WNOHANG);
        if (ps < 0) waiting = FALSE;
        else
        {
            gtk_progress_bar_pulse ( GTK_PROGRESS_BAR (progressbar) );
            while (gtk_events_pending())
                gtk_main_iteration();
        }
    }
    //temp_file contains the filename created in /tmp to add/delete on tar.bzip2 / tar.gzip archives (look in bzip2.c)
    if ( temp_file != NULL)
    {
        unlink ( temp_file );
        g_free (temp_file);
    }
     
    if ( WIFSIGNALED (status) )
    {
        Update_StatusBar ( _("Operation canceled."));
        OffTooltipPadlock();
        if (action == extract)
        {
            gchar *msg = g_strconcat (_("Please check \""),extract_path,_("\" since some files could have been already extracted."),NULL);
            response = ShowGtkMessageDialog (GTK_WINDOW (MainWindow),GTK_DIALOG_MODAL,GTK_MESSAGE_INFO,	GTK_BUTTONS_OK,msg );
            g_free (msg);
        }
        action = inactive;
        SetButtonState (1,1,0,0,0);
        return;
    }
    OffTooltipPadlock();
    if (action == test)
    {
        action =  inactive;
        ShowShellOutput ( NULL , FALSE );
        SetButtonState (1,1,1,1,1);
        gtk_widget_set_sensitive ( check_menu , TRUE );
        Update_StatusBar ( _("Operation completed."));
        return;
    }

    if ( WIFEXITED (status) )
	{
		if ( WEXITSTATUS (status) )
		{
            Update_StatusBar ( _("Operation failed."));
            gtk_tooltips_disable ( pad_tooltip );
            gtk_widget_hide ( pad_image );
			archive_error = TRUE;
			SetButtonState (1,1,0,0,0);
			gtk_window_set_title ( GTK_WINDOW (MainWindow) , "Xarchiver " VERSION );
			response = ShowGtkMessageDialog (GTK_WINDOW
			(MainWindow),GTK_DIALOG_MODAL,GTK_MESSAGE_QUESTION,GTK_BUTTONS_YES_NO,_("An error occurred while accessing the archive.\nDo you want to view the shell output ?") );
			if (response == GTK_RESPONSE_YES) ShowShellOutput (NULL,FALSE);
            CurrentArchiveType = -1;
			return;
		}
	}
    archive_error = FALSE;
	//This to automatically reload the content of the archive after adding or deleting
	if (action == add || action == delete)
	{
        gtk_widget_show ( viewport2 );
        gtk_widget_set_sensitive ( Stop_button , TRUE );
        Update_StatusBar ( _("Please wait while the content of the archive is being updated..."));
        RemoveColumnsListStore();
        action = inactive;
		switch ( CurrentArchiveType )
		{
			//case 0:
			//Bzip2: not needed

			//case 1:
			//Gzip: not needed

			case 2:
            OpenRar ( FALSE , escaped_path );
			break;

			case 3:
			OpenTar ( FALSE , escaped_path );
			break;

            case 4:
            OpenBzip2 ( FALSE , escaped_path );
            break;

            case 5:
            OpenGzip ( FALSE , escaped_path );
            break;

            case 6:
			case 7:
			OpenZip ( FALSE , escaped_path );
			break;

            //case 8:
            //RPM: only open and extract

            case 9:
            Open7Zip ( FALSE, escaped_path );
            break;

            case 10:
            OpenArj ( FALSE, escaped_path );
            break;
		}
        return;
	}
    //This to disable the Add and Delete buttons in case a bzip2 / gzip file has been decompressed
    if ( ! bz_gz ) SetButtonState ( 1,1,1,1,1 );
        else SetButtonState (1,1,0,0,0);
    //Let's restore the liststore
    if ( CurrentArchiveType != 0 && CurrentArchiveType != 1)
    {
        gtk_tree_view_set_model (GTK_TREE_VIEW(treeview1), model);
        g_object_unref (model);
    }
    if ( password != NULL || PasswordProtectedArchive )
    {
        gtk_widget_show ( pad_image );
        gtk_tooltips_enable ( pad_tooltip );
    }
    else
    {
        gtk_tooltips_disable ( pad_tooltip );
        gtk_widget_hide ( pad_image );
    }
    //This to enable only the Extract button in case an RPM was opened
    if (CurrentArchiveType == 8) SetButtonState (1,1,0,0,1);

    if ( CurrentArchiveType == 2 || CurrentArchiveType == 6 || CurrentArchiveType == 7 || CurrentArchiveType == 9 || CurrentArchiveType == 10)
    {
        gtk_widget_set_sensitive ( add_pwd , TRUE );
        gtk_widget_set_sensitive ( check_menu , TRUE);
    }
    else
    {
        gtk_widget_set_sensitive ( add_pwd , FALSE );
        gtk_widget_set_sensitive ( check_menu , FALSE);
    }
    gtk_window_set_title ( GTK_WINDOW (MainWindow) , removed_bs_path );
    gtk_widget_set_sensitive ( properties , TRUE );
    Update_StatusBar ( _("Operation successfully completed."));
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
            path = NULL;
            return;
		}
        //The following to avoid the archivers updates the archive instead of adding to it since the filename exists
        unlink ( path );
	}
    EmptyTextBuffer();
    removed_bs_path = RemoveBackSlashes ( path );
	escaped_path = EscapeBadChars ( path );
    PasswordProtectedArchive = FALSE;
    gtk_widget_set_sensitive ( iso_info , FALSE );
    gtk_widget_set_sensitive ( view_shell_output1 , TRUE );
    gtk_widget_set_sensitive ( check_menu , FALSE);
    gtk_widget_set_sensitive ( properties , FALSE );
    //Let's off the delete and view buttons and the menu entries to avoid misterious behaviour
    OffDeleteandViewButtons ();
    //This to delete a possible RPM file from /tmp since we don't need it anymore
    if ( tmp != NULL )
    {
        unlink (tmp);
        tmp = NULL;
    }
	if ( liststore != NULL ) RemoveColumnsListStore();
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
    
    if ( CurrentArchiveType == 2 || CurrentArchiveType == 6 || CurrentArchiveType == 7 || CurrentArchiveType == 9 || CurrentArchiveType == 10 )
    {
        gtk_widget_set_sensitive ( add_pwd , TRUE );
        Update_StatusBar ( _("Choose Action->Set Password to create a password protected archive."));
    }
    else
    {
        gtk_widget_set_sensitive ( add_pwd , FALSE );
        if (CurrentArchiveType == 0 || CurrentArchiveType == 1)  Update_StatusBar ( _("Choose Add File to create the compressed file."));
            else Update_StatusBar ( _("Choose Add File or Add Folder to create the archive."));
    }
    gtk_tooltips_disable ( pad_tooltip );
    gtk_widget_hide ( pad_image );
    if (password != NULL) g_free (password);
    password = NULL;
    dummy_size = 0;
    number_of_files = 0;
    number_of_dirs = 0;
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
    path = (gchar *)user_data;
    if ( path == NULL)
    {
		path = Show_File_Dialog ( 1 , "open" );
		if (path == NULL) return;
	}
  	if ( removed_bs_path != NULL ) g_free ( removed_bs_path );
    if ( escaped_path != NULL ) g_free (escaped_path);
    
    escaped_path = EscapeBadChars (path);
    removed_bs_path = RemoveBackSlashes ( escaped_path );
    g_free (path);
    path = NULL;
    PasswordProtectedArchive = FALSE;
    OffDeleteandViewButtons();
    gtk_widget_set_sensitive ( iso_info , FALSE );
    gtk_widget_set_sensitive ( view_shell_output1 , TRUE );
    
    CurrentArchiveType = DetectArchiveType ( removed_bs_path );
    if ( CurrentArchiveType == -2 ) return;
    if ( CurrentArchiveType == -1 )
    {
        gtk_window_set_title ( GTK_WINDOW (MainWindow) , "Xarchiver " VERSION );
		response = ShowGtkMessageDialog (GTK_WINDOW (MainWindow),GTK_DIALOG_MODAL,GTK_MESSAGE_ERROR,GTK_BUTTONS_OK,
		_("The format of this archive is not recognized !") );
        return;
	}
	if ( liststore != NULL )
	{
		RemoveColumnsListStore();
		EmptyTextBuffer ();
	}
    //Did the user previously open an RPM file ? Delete it from /tmp since we don't need it anymore
    if ( CurrentArchiveType == 8 ) unlink (tmp);
    EmptyTextBuffer();
    if ( GTK_WIDGET_VISIBLE (OutputWindow )) gtk_widget_hide (OutputWindow);
   
    //Does the user open an archive from the command line whose archiver is not installed ?
    ext = NULL;
    if ( CurrentArchiveType == 2 ) ext = ".rar";
    else if ( CurrentArchiveType == 9 ) ext = ".7z";
    else if ( CurrentArchiveType == 10 ) ext = ".arj";
    if ( ext != NULL )
        if ( ! g_list_find ( ArchiveType , ext ) )
        {
            response = ShowGtkMessageDialog (GTK_WINDOW (MainWindow),GTK_DIALOG_MODAL,GTK_MESSAGE_ERROR, GTK_BUTTONS_OK,_("Sorry, this archive format is not supported since you haven't installed the proper archiver !") );
            return;
        }

    gtk_widget_set_sensitive (Stop_button,TRUE);
	bz_gz = FALSE;
    gtk_widget_show ( viewport2 );
    if ( CurrentArchiveType == 11 ) Update_StatusBar ( _("Please wait while the content of the ISO image is being read..."));
        else Update_StatusBar ( _("Please wait while the content of the archive is being read..."));
    SetButtonState (0,0,0,0,0);
	switch ( CurrentArchiveType )
	{
		case 0:
		OpenBzip2 ( TRUE , escaped_path );
		break;

		case 1:
		OpenGzip ( TRUE , escaped_path );
		break;

		case 2:
		OpenRar ( TRUE , escaped_path );
		break;

		case 3:
		OpenTar ( TRUE , escaped_path );
		break;

		case 7:
		OpenZip ( TRUE , escaped_path );
		break;

        case 8:
        OpenRPM ( TRUE , escaped_path );
        break;

        case 9:
        Open7Zip ( TRUE , escaped_path );
        break;

		case 10:
		OpenArj ( TRUE , escaped_path );
		break;

        case 11:
        OpenISO ( TRUE , escaped_path );
	}
    if (password != NULL) g_free (password);
    password = NULL;
}

void on_check_activate (GtkMenuItem *menuitem, gpointer user_data)
{
    gchar *command = NULL;
    action = test;
    if ( PasswordProtectedArchive )
    {
        if ( password == NULL) Show_pwd_Window ( NULL , NULL );
        if ( password == NULL) return;
    }
    Update_StatusBar ( _("Testing archive integrity, please wait..."));
    gtk_widget_set_sensitive (Stop_button,TRUE);
    gtk_widget_set_sensitive ( check_menu , FALSE );
    SetButtonState (0,0,0,0,0);
    switch ( CurrentArchiveType )
	{
		case 2:
		if (password != NULL) command = g_strconcat ("rar t -idp -p" , password ," " , escaped_path, NULL);
		    else command = g_strconcat ("rar t -idp " , escaped_path, NULL);
        break;

        case 7:
        if (password != NULL) command = g_strconcat ("unzip -P ", password, " -t " , escaped_path, NULL);
            else command = g_strconcat ("unzip -t " , escaped_path, NULL);
        break;

        case 9:
        if (password != NULL) command = g_strconcat ( "7za t -p" , password , " " , escaped_path, NULL);
		    else command = g_strconcat ("7za t " , escaped_path, NULL);
		break;

		case 10:
        if (password != NULL) command = g_strconcat ("arj t -g" , password , " -i " , escaped_path, NULL);
		    else command = g_strconcat ("arj t -i " , escaped_path, NULL);
		break;
	}
    ExtractAddDelete ( command );
    g_free (command);
}

void on_quit1_activate (GtkMenuItem *menuitem, gpointer user_data)
{
    if ( GTK_WIDGET_VISIBLE (viewport2) )
    {
        Update_StatusBar ( _("Please hit the Stop button first !"));
        return;
    }
    g_list_free ( Suffix );
	g_list_free ( Name );
	//if ( path != NULL )
    //{
        g_free (removed_bs_path);
        g_free (escaped_path);
    //}
    if ( extract_path != NULL )
    {
        if ( strcmp (extract_path,"/tmp/") != 0) g_free (extract_path);
        g_free (es_path);
    }
    //This to delete a CPIO temp file in /tmp if existing
    if ( tmp != NULL )
    {
        unlink (tmp);
        tmp = NULL;
    }
    gtk_main_quit();
}

void on_delete1_activate (GtkMenuItem *menuitem, gpointer user_data)
{
	gchar *command = NULL;
    char numbers[6];

	GtkTreeSelection *selection = gtk_tree_view_get_selection ( GTK_TREE_VIEW (treeview1) );
	names = g_string_new ( " " );
	gtk_tree_selection_selected_foreach (selection, (GtkTreeSelectionForeachFunc) ConcatenateFileNames, names );
	action = delete;
    x = gtk_tree_selection_count_selected_rows (selection);
    sprintf ( numbers , "%d", x );
    gchar *msg = g_strconcat (_("You are about to delete "),numbers,x > 1 ? _(" files") : _(" file") , _(" from the archive.\nAre you really sure to do this ?"),NULL);
    response = ShowGtkMessageDialog (GTK_WINDOW (MainWindow),GTK_DIALOG_MODAL,GTK_MESSAGE_QUESTION,GTK_BUTTONS_YES_NO,msg );
    g_free (msg);
    if ( response == GTK_RESPONSE_NO) return;
    Update_StatusBar ( _("Deleting files from the archive, please wait..."));

	switch (CurrentArchiveType)
	{
		case 2:
		command = g_strconcat ( "rar d " , escaped_path , names->str , NULL );
		break;

        case 3:
		command = g_strconcat ( "tar --delete -vf " , escaped_path , names->str , NULL );
		break;

        case 4:
        DecompressBzipGzip ( names , escaped_path , 0  , 0 );
        break;

        case 5:
        DecompressBzipGzip ( names , escaped_path , 1 , 0 );
		break;

        case 6:
        case 7:
		command = g_strconcat ( "zip -d " , escaped_path , names->str , NULL );
		break;

        case 9:
        command = g_strconcat ( "7za d " , escaped_path , names->str , NULL );
        break;

        case 10:
        command = g_strconcat ( "arj d " , escaped_path , names->str, NULL);
        break;
	}
	if (command != NULL)
    {
        ExtractAddDelete ( command );
        g_free (command);
    }
    //g_string_free (names , FALSE );
}

void on_add_files_activate ( GtkMenuItem *menuitem, gpointer user_data )
{
    gchar *name = NULL;
	gchar *command = NULL;
   	if ( Files_to_Add == NULL)
    {
        Files_to_Add = Add_File_Dialog ( user_data );
	    if ( Files_to_Add == NULL) return;
    }
	names = g_string_new ( " " );
    Files_to_Add = g_slist_reverse (Files_to_Add);
    gtk_widget_set_sensitive (Stop_button , TRUE);
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
        SetButtonState (0,0,0,0,0);
        if (CurrentArchiveType != 0 && CurrentArchiveType != 1) Update_StatusBar ( _("Adding files to the archive, please wait..."));
		switch (CurrentArchiveType)
		{
            case -1:
            command = NULL;
            break;

			case 0:
            Update_StatusBar ( _("Compressing file, please wait..."));
			Bzip2Add ( names->str , 0 );
			break;

			case 1:
            Update_StatusBar ( _("Compressing file, please wait..."));
			Bzip2Add ( names->str , 1 );
			break;

			case 2:
            if (password != NULL) command = g_strconcat ( "rar a -p" , password, " -o+ -ep1 -idp " , escaped_path , names->str , NULL );
                else command = g_strconcat ( "rar a -o+ -ep1 -idp " , escaped_path , names->str , NULL );
			break;

			case 3:
			if ( g_file_test ( escaped_path , G_FILE_TEST_EXISTS ) ) command = g_strconcat ( "tar rvvf " , escaped_path , names->str , NULL );
                else command = g_strconcat ( "tar cvvf " , escaped_path , names->str , NULL );
    		break;

            case 4:
            if ( g_file_test ( escaped_path , G_FILE_TEST_EXISTS ) ) DecompressBzipGzip ( names , escaped_path , 0 , 1 );
                else command = g_strconcat ("tar cvvfj " , escaped_path , names->str , NULL );
            break;

            case 5:
            if ( g_file_test ( escaped_path , G_FILE_TEST_EXISTS ) ) DecompressBzipGzip ( names , escaped_path , 1 , 1 );
                else command = g_strconcat ("tar cvvfz " , escaped_path , names->str , NULL );
            break;

			case 6:
			case 7:
            if (password != NULL) command = g_strconcat ( "zip -P " , password , " -r " , escaped_path , names->str , NULL );
                else command = g_strconcat ( "zip -r " , escaped_path , names->str , NULL );
			break;

            case 9:
            if (password != NULL) command = g_strconcat ( "7za a -ms=off -p" , password , " " , escaped_path , names->str , NULL );
                else command = g_strconcat ( "7za a -ms=off " , escaped_path , names->str , NULL );
            break;

            case 10:
            if (password != NULL) command = g_strconcat ( "arj a -i -r -g" , password , " " , escaped_path , names->str , NULL );
                else command = g_strconcat ( "arj a -i -r " , escaped_path , names->str , NULL );
            break;
		}
    if (command != NULL)
    {
        ExtractAddDelete ( command );
        g_free (command);
    }
	g_string_free (names , FALSE );
	g_slist_foreach (Files_to_Add, (GFunc )g_free, NULL);
    g_slist_free ( Files_to_Add );
    Files_to_Add = NULL;
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
            if (password == NULL) Show_pwd_Window ( NULL , NULL );
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
            gtk_widget_destroy ( extract_window );
			break;

			case GTK_RESPONSE_OK:
            action = extract;
            if (es_path != NULL) g_free (es_path);
            es_path = g_strdup (gtk_entry_get_text ( GTK_ENTRY (entry1) )); 
			extract_path = EscapeBadChars ( es_path );
			if ( strlen ( extract_path ) > 0 )
			{
				done = TRUE;
                gtk_widget_set_sensitive (Stop_button,TRUE);
                gtk_widget_destroy ( extract_window );
				if ( selected < 1 )
				{
                    gchar *text = g_strconcat (_("Extracting files to "), es_path , NULL );
                    Update_StatusBar ( text );
                    g_free (text);
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
                        if (password !=NULL) command = g_strconcat ( "rar x -p",password," -o+ -idp " , escaped_path , " " , extract_path , NULL );
                        else command = g_strconcat ( "rar x -o+ -idp " , escaped_path , " " , extract_path , NULL );
						break;

						case 3:
						command = g_strconcat ( "tar xfv " , escaped_path , " -C " , extract_path , NULL );
						break;

						case 4:
						command = g_strconcat ( "tar xfjv " , escaped_path , " -C " , extract_path , NULL );
						break;

						case 5:
						command = g_strconcat ( "tar xfzv " , escaped_path , " -C " , extract_path , NULL );
						break;

                        case 6:
						case 7:
                        if ( password != NULL ) command = g_strconcat ( "unzip -o -P " , password , " " , escaped_path , " -d " , extract_path , NULL );
                            else command = g_strconcat ( "unzip -o " , escaped_path , " -d " , extract_path , NULL );
						break;

                        case 8:
                        chdir ( extract_path );
                        SpawnCPIO ( "cpio -ivd" , tmp , 0 , 1 );
                        break;

                        case 9:
                        if (password != NULL) command = g_strconcat ( "7za x -aoa -bd -p",password," ", escaped_path , " -o" , extract_path , NULL );
                        else command = g_strconcat ( "7za x -aoa -bd " , escaped_path , " -o" , extract_path , NULL );
                        break;

						case 10:
						if (password !=NULL) command = g_strconcat ( "arj x -g",password," -i -y " , escaped_path , " " , extract_path , NULL );
                        else command = g_strconcat ( "arj x -i -y " , escaped_path , " " , extract_path , NULL );
						break;
					}
                    if ( command != NULL )
                    {
                        ExtractAddDelete ( command );
                        g_free (command);
                    }
				}
				else
				{
					names = g_string_new ( " " );
					gtk_tree_selection_selected_foreach (selection, (GtkTreeSelectionForeachFunc) ConcatenateFileNames, names );
					command = ChooseCommandtoExecute ( 1 , names );
                    if ( command != NULL )
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
}

gchar *ChooseCommandtoExecute ( gboolean full_path , GString *files)
{
    gchar *command;
    unsigned short int levels;
    char digit[2];
    gchar *strip = NULL;
    if ( full_path == 0 )
    {
        levels = CountCharacter ( files->str , '/');
        sprintf ( digit , "%d" , levels );
        strip = g_strconcat ( "--strip-components=" , digit , " " , NULL );
    }
    gchar *msg = g_strconcat ( _("Extracting files to ") , extract_path,NULL);
    Update_StatusBar (msg);
    g_free (msg);
    switch (CurrentArchiveType)
	{
        case -1:
        command = NULL;
        break;

	    case 0:
	    //Bzip2 extraction is handled when the the file is opened
		//code execution never reaches here
		break;

		case 1:
        //idem
		break;

		case 2:
		if (password != NULL) command = g_strconcat ( "rar " , full_path ? "x" : "e" , " -p",password, " -o+ -idp " , escaped_path , " " , files->str , " " , extract_path , NULL );
        else command = g_strconcat ( "rar ", full_path ? "x" : "e" , " -o+ -idp " , escaped_path , " " , files->str , " ", extract_path ,NULL);
		break;

		case 3:
	    command = g_strconcat ( "tar " , full_path ? "" : strip , "-xvf " , escaped_path , " -C " , extract_path , files->str , NULL );
		break;

		case 4:
		command = g_strconcat ( "tar " , full_path ? "" : strip , "-xjvf " , escaped_path , " -C " , extract_path ,  files->str , NULL );
		break;

		case 5:
        command = g_strconcat ( "tar " , full_path ? "" : strip , "-xvzf " , escaped_path , " -C " , extract_path , files->str , NULL );
		break;

        case 6:
		case 7:
        if ( password != NULL ) command = g_strconcat ( "unzip -o -P " , password , full_path ? " " : " -j " , escaped_path , files->str , " -d " , extract_path , NULL );
        else command = g_strconcat ( "unzip -o " , full_path ? "" : "-j " , escaped_path , files->str , " -d " , extract_path , NULL );
		break;

        case 8:
        chdir ( extract_path );
        command = g_strconcat ( "cpio -ivd" , files->str , NULL );
        SpawnCPIO ( command , tmp , 0 , 1 );
        g_free (command);
        command = NULL;
        break;

        case 9:
        if ( password != NULL) command = g_strconcat ("7za " , full_path ? "x" : "e" , " -p",password," -aoa -bd " , escaped_path , files->str , " -o" , extract_path , NULL );
        else command = g_strconcat ( "7za " , full_path ? "x" : "e" ," -aoa -bd " , escaped_path , files->str , " -o" , extract_path , NULL );
        break;

		case 10:
		if (password !=NULL) command = g_strconcat ( "arj x -g",password," -i -y " , escaped_path , " " , extract_path , files->str , NULL );
        else command = g_strconcat ( "arj ",full_path ? "x" : "e"," -i -y " , escaped_path , " " , extract_path , files->str, NULL );
		break;
    }
    if ( strip != NULL) g_free ( strip );
    return command;
}

void on_about1_activate (GtkMenuItem *menuitem, gpointer user_data)
{
    static GtkWidget *about = NULL;
    const char *authors[] = {"\nMain Developer:\nGiuseppe Torelli - Colossus <colossus73@gmail.com>\n\nDevelopers:\nSalvatore Santagati <salvatore.santagati@gmail.com>\n",NULL};
    const char *documenters[] = {"\nThanks to:\nBenedikt Meurer for helping me with DnD.\n\nFabian Pedrosa, Szervac Attila and Iuri Stanchev\nfor testing this release.\n\nUracile for the stunning logo.\n\nEugene Ostapets and Enrico Troeger\nfor russian and german translation.\n\nThe people of gtk-app-devel-list\nwho kindly answered my questions.", NULL};
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
		      "comments", "A lightweight GTK2 archive manager",
		      "authors", authors,
              "documenters",documenters,
		      "translator_credits", NULL,
		      "logo_icon_name", "xarchiver",
		      "website", "http://xarchiver.sourceforge.net",
		      "website_label", NULL,
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
		if (CurrentArchiveType == 10) title = _("Select the folder to be added to the current archive");
            else title = _("Select the folders to be added to the current archive; use SHIFT to multiple select");
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
    //We set gtk_file_chooser_set_select_multiple to FALSE because of a bug in ARJ prevents adding more of two directories
    if (CurrentArchiveType == 0 || CurrentArchiveType == 1 || ( CurrentArchiveType == 10 && mode == "folder" ) ) gtk_file_chooser_set_select_multiple ( GTK_FILE_CHOOSER (File_Selector) , FALSE );
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
    gchar *dummy_path;
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
        if (es_path != NULL) gtk_file_chooser_set_current_folder ( GTK_FILE_CHOOSER (File_Selector) , es_path );
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
			ComboArchiveType = gtk_combo_box_get_active_text (GTK_COMBO_BOX (combo_box));

            if (strcmp ( ComboArchiveType,".arj") == 0) CurrentArchiveType = 10;
                else if (strcmp ( ComboArchiveType,".bz2") == 0) CurrentArchiveType = 0;
                else if (strcmp ( ComboArchiveType,".gz") == 0) CurrentArchiveType = 1;
                else if (strcmp ( ComboArchiveType,".rar") == 0) CurrentArchiveType = 2;
                else if (strcmp ( ComboArchiveType,".tar") == 0) CurrentArchiveType = 3;
                else if (strcmp ( ComboArchiveType,".tar.bz2") == 0) CurrentArchiveType = 4;
                else if (strcmp ( ComboArchiveType,".tar.gz") == 0) CurrentArchiveType = 5;
                else if (strcmp ( ComboArchiveType,".jar") == 0) CurrentArchiveType = 6;
                else if (strcmp ( ComboArchiveType,".zip") == 0) CurrentArchiveType = 7;
                else if (strcmp ( ComboArchiveType,".rpm") == 0) CurrentArchiveType = 8;
                else if (strcmp ( ComboArchiveType,".7z") == 0) CurrentArchiveType = 9;
			if ( gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(check_button) ) )
			{
				if ( ! g_str_has_suffix ( path , ComboArchiveType ) )
				{
					gtk_widget_destroy (File_Selector);
                    dummy_path = g_strconcat ( path, ComboArchiveType , NULL);
                    g_free ( path );
					return dummy_path;
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

gboolean isISO ( FILE *ptr )
{
	/*if ( (DetectImage(ptr)) > 0 )
    {
        gtk_widget_set_sensitive ( iso_info , TRUE );
        gtk_widget_set_sensitive ( view_shell_output1 , FALSE );
        return TRUE;
    }
    else return FALSE;*/
}

gboolean isTar ( FILE *ptr )
{
	unsigned char magic[7];
	fseek ( ptr, 0 , SEEK_SET );
    if ( fseek ( ptr , 100 , SEEK_CUR ) < 0 ) return FALSE;
    if ( fread ( magic, 1, 5, ptr ) == 0 ) return FALSE;
    if ( memcmp ( magic,"\x30\x30\x30\x30\x37",5 ) == 0 ) return TRUE;
    
    //Normally from the beginning of the file the magic is found at offset 257 (decimal) but we are at 100 and we read 5 bytes
    if ( fseek ( ptr , 159 , SEEK_CUR) < 0 ) return FALSE;
	if ( fread ( magic, 1, 7, ptr ) == 0 ) return FALSE;
	if ( memcmp ( magic,"\x75\x73\x74\x61\x72\x00\x30",7 ) == 0 || memcmp (magic,"\x75\x73\x74\x61\x72\x20\x20",7 ) == 0) return TRUE;
		else return FALSE;
}

int DetectArchiveType ( gchar *path )
{
	FILE *dummy_ptr = NULL;
    int xx = -1;
	unsigned char magic[6];
	dummy_ptr = fopen ( path , "r" );

	if (dummy_ptr == NULL)
	{
		gchar *msg = g_strdup_printf (_("Can't open archive %s:\n%s") , path , strerror (errno) );
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
        PasswordProtectedArchive = DetectPasswordProtectedArchive ( 7 , dummy_ptr , magic );
        xx = 7; // zip
    }
	else if ( memcmp ( magic,"\x60\xea",2 ) == 0 )
    {
        PasswordProtectedArchive = DetectPasswordProtectedArchive ( 10 , dummy_ptr , magic );
        xx = 10;    // arj
    }
	else if ( memcmp ( magic,"\x52\x61\x72\x21",4 ) == 0 ) xx = 2;	// rar
    else if ( memcmp ( magic,"\x42\x5a\x68",3 ) == 0 ) xx = 0;	// bzip2
	else if ( memcmp ( magic,"\x1f\x8b\x08",3 ) == 0) xx = 1;		// gz
    else if ( memcmp ( magic,"\xed\xab\xee\xdb",4 ) == 0) xx = 8;	// rpm
    else if ( memcmp ( magic,"\x37\x7a\xbc\xaf\x27\x1c",6 ) == 0 ) xx = 9;	// 7zip
    else if ( isTar ( dummy_ptr ) ) xx = 3;
    else if ( isISO ( dummy_ptr ) ) xx = 11;
	fclose ( dummy_ptr );
	return xx;
}

gboolean DetectPasswordProtectedArchive ( int type , FILE *stream , unsigned char magic[6] )
{
    //ZIP
    unsigned int fseek_offset;
    unsigned short int password_flag;
    unsigned int compressed_size;
    unsigned int uncompressed_size;
    unsigned short int file_length;
    unsigned short int extra_length;

    //ARJ
    unsigned char sig[2];
    unsigned short int basic_header_flag;
    unsigned short int basic_header_size;
    unsigned short int extended_header_size;
    unsigned int basic_header_CRC;
    unsigned int extended_header_CRC;
    unsigned char arj_flag;

    if ( type == 7 )
    {
        while ( memcmp ( magic,"\x50\x4b\x03\x04",4 ) == 0  || memcmp ( magic,"\x50\x4b\x05\x06",4 ) == 0 )
        {
            fread ( &password_flag, 1, 2, stream );
            if (( password_flag & ( 1<<0) ) > 0) return TRUE;
            fseek (stream,10,SEEK_CUR);
            fread (&compressed_size,1,4,stream);
            fread (&uncompressed_size,1,4,stream);
            fread (&file_length,1,2,stream);
            //If the zip archive is empty (no files) it should return here
            if (fread (&extra_length,1,2,stream) < 2 ) return FALSE;
            fseek_offset = compressed_size + file_length + extra_length;
            fseek (stream , fseek_offset , SEEK_CUR);
            fread (magic , 1 , 4 , stream);
            fseek ( stream , 2 , SEEK_CUR);
        }
    }
    else if ( type == 10)
    {
        fseek (stream , magic[2]+magic[3] , SEEK_CUR);
        fseek (stream , 2 , SEEK_CUR);
        fread (&extended_header_size,1,2,stream);
        if (extended_header_size != 0) fread (&extended_header_CRC,1,4,stream);
        fread (&sig,1,2,stream);
        while ( memcmp (sig,"\x60\xea",2) == 0)
        {
            fread ( &basic_header_size , 1 , 2 , stream );
            if ( basic_header_size == 0 ) break;
            fseek ( stream , 4 , SEEK_CUR);
            fread (&arj_flag,1,1,stream);
            if ((arj_flag & ( 1<<0) ) > 0) return TRUE;
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
	return ioc;
}

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

void CreateListStore ( int nc, gchar *columns_names[] , GType columns_types[])
{
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

void ShowShellOutput ( GtkMenuItem *menuitem, gboolean iso_title)
{
	if (OutputWindow != NULL)
	{
         if (iso_title) gtk_window_set_title (GTK_WINDOW (OutputWindow), _("ISO Image Information Window") );
            else gtk_window_set_title (GTK_WINDOW (OutputWindow), _("Shell Output Window") );
        gtk_window_present ( GTK_WINDOW (OutputWindow) );
		return;
	}
	OutputWindow = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	gtk_widget_set_name (OutputWindow, "OutputWindow");
	gtk_window_set_position (GTK_WINDOW (OutputWindow), GTK_WIN_POS_CENTER);
	gtk_window_set_default_size(GTK_WINDOW(OutputWindow), 450, 300);
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
	gtk_text_buffer_create_tag (textbuf, "red_foreground","foreground", "red", NULL);

	gtk_widget_show (vbox);
	gtk_widget_show (scrollwin);
	gtk_widget_show (textview);
}

void Cancel_Operation ( GtkMenuItem *menuitem , gpointer user_data )
{
    gtk_widget_set_sensitive ( Stop_button , FALSE );
    Update_StatusBar (_("Waiting for the process to abort..."));
    if ( kill ( child_pid , SIGABRT ) < 0 )
    {
        response = ShowGtkMessageDialog (GTK_WINDOW (MainWindow),GTK_DIALOG_MODAL,GTK_MESSAGE_ERROR,GTK_BUTTONS_OK, strerror(errno));
	    return;
    }
    //This in case the user cancel the opening of a password protected archive
    if (action != add || action != delete)
        if (PasswordProtectedArchive) PasswordProtectedArchive = FALSE;
}

void View_File_Window ( GtkMenuItem *menuitem , gpointer user_data )
{
    gchar *command = NULL;
    GtkTreeSelection *selection;
    GtkTreeModel *model;
    GtkTreeIter iter;
    gchar *dir;
    gchar *dummy_name;
    unsigned short int COL_NAME = 1;
    gboolean is_dir = FALSE;

    if ( PasswordProtectedArchive )
    {
            Show_pwd_Window ( NULL , NULL );
            if ( password == NULL ) return;
    }
    selection = gtk_tree_view_get_selection ( GTK_TREE_VIEW (treeview1) );
    if ( ! gtk_tree_selection_get_selected (selection, &model, &iter) ) return;
    switch (CurrentArchiveType)
    {
        case 2:
        case 10:
            COL_NAME = 6;
        break;

        case 7:
            COL_NAME = 0;
        break;

        case 9:
            COL_NAME = 3;
        break;
    }
    gtk_tree_model_get (model, &iter, COL_NAME, &dir, -1);
    if (CurrentArchiveType == 7)
    {
        if ( g_str_has_suffix (dir,"/") == TRUE ) is_dir = TRUE;
    }
    else if ( strstr ( dir , "d" ) || strstr ( dir , "D" ) ) is_dir = TRUE;
    if (is_dir)
    {
        response = ShowGtkMessageDialog (GTK_WINDOW (MainWindow),GTK_DIALOG_MODAL,GTK_MESSAGE_ERROR, GTK_BUTTONS_OK,_("Please select a file, not a directory !") );
        g_free ( dummy_name );
        g_free ( dir );
        return;
    }
    g_free ( dir );
    gtk_tree_model_get (model, &iter, 0, &dummy_name, -1);
    dir = EscapeBadChars ( dummy_name );
    g_free (dummy_name);
    extract_path = "/tmp/";
    names = g_string_new (" ");
    g_string_append ( names , dir );
    command = ChooseCommandtoExecute ( 0 , names);
   	compressor_pid = SpawnAsyncProcess ( command , 0 , 0);
    g_free ( command );
	if ( compressor_pid == 0 ) return;
    SetIOChannel (error_fd, G_IO_IN|G_IO_PRI|G_IO_ERR|G_IO_HUP|G_IO_NVAL,GenError, NULL );
    g_child_watch_add ( compressor_pid , (GChildWatchFunc) ViewFileFromArchive , names );
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
	    	archive_error = TRUE;
    		SetButtonState (1,1,0,0,0);
	    	gtk_window_set_title ( GTK_WINDOW (MainWindow) , "Xarchiver " VERSION );
		    response = ShowGtkMessageDialog (GTK_WINDOW(MainWindow),GTK_DIALOG_MODAL,GTK_MESSAGE_QUESTION,GTK_BUTTONS_YES_NO,_("An error occurred while extracting the file to be viewed.\nDo you want to view the shell output ?") );
            if (response == GTK_RESPONSE_YES) ShowShellOutput (NULL , FALSE);
            unlink ( (char*)data );
			return;
        }
    }
    string = StripPathFromFilename ( (char*) data->str );
    //Let's avoid the white space
    data->str++;
    if (  string == NULL ) filename = g_strconcat ( "/tmp/" , data->str , NULL );
    else
    {
        if ( strchr ( string , ' ' ) )
        {
            string = RemoveBackSlashes ( string );
            tofree = TRUE;
        }
        filename = g_strconcat ( "/tmp" , string , NULL );
        if ( tofree ) g_free ( string );
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
}

void ShowArchiveProperties ( GtkMenuItem *menuitem , gpointer user_data )
{
    gchar *utf8_string , *measure, *text, *dummy_string;
    char date[64];
    gchar *t;
    unsigned long long int file_size;

    stat ( removed_bs_path , &my_stat );
    file_size = my_stat.st_size;
    archive_properties_win = create_archive_properties_window();
    //Name
    text = StripPathFromFilename ( removed_bs_path );
    if (text != NULL)
    {
        text++; //This to avoid the / char in the string
        utf8_string = g_filename_display_name (text);
    }
    else utf8_string = g_filename_display_name (removed_bs_path);
    gtk_entry_set_text ( GTK_ENTRY (name_data), utf8_string );
    g_free (utf8_string);
    //Path
    dummy_string = remove_level_from_path (removed_bs_path);
    if ( strlen(dummy_string) != 0) utf8_string = g_filename_display_name (dummy_string);
        else utf8_string = g_filename_display_name ( g_get_current_dir () );
    gtk_entry_set_text ( GTK_ENTRY (path_data), utf8_string );
    g_free ( utf8_string );
    g_free ( dummy_string );
    //Modified Date
    strftime (date, 64, _("%c"), localtime (&my_stat.st_mtime) );
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
    if (dummy_size > 1024*1024*1024 )
    {
        content_size = (double)dummy_size / (1024*1024*1024);
        measure = _(" GB");
    }
        else if (dummy_size > 1024*1024 ) 
        {
            content_size = (double)dummy_size / (1024*1024);
            measure = _(" MB");
        }
    
        else if (dummy_size > 1024 )
        {
            content_size = (double)dummy_size / 1024;
            measure = _(" KB");
        }
        else
        {
            measure = _(" bytes");
            content_size = dummy_size;
        }
    t = g_strdup_printf ( "%.1f %s", content_size,measure);
    gtk_entry_set_text ( GTK_ENTRY (content_data), t );
    g_free (t);
    //Compression_ratio
    if (content_size != 0) content_size = (double)dummy_size / file_size;
        else content_size = 0.0;
    t = g_strdup_printf ( "%.2f", content_size);
    gtk_entry_set_text ( GTK_ENTRY (compression_data), t );
    g_free (t);
    //Number of files
    t = g_strdup_printf ( "%d", number_of_files);
    gtk_entry_set_text ( GTK_ENTRY (number_of_files_data), t );
    g_free (t);
    //Number of dirs
    t = g_strdup_printf ( "%d", number_of_dirs);
    gtk_entry_set_text ( GTK_ENTRY (number_of_dirs_data), t );
    g_free (t);
    gtk_widget_show ( archive_properties_win );
}

void Show_pwd_Window ( GtkMenuItem *menuitem , gpointer user_data )
{
    pwd_window = passwd_win();
    gtk_dialog_set_default_response (GTK_DIALOG (pwd_window), GTK_RESPONSE_OK);
    done = FALSE;
	while ( ! done )
	{
		switch ( gtk_dialog_run ( GTK_DIALOG ( pwd_window ) ) )
		{
			case GTK_RESPONSE_CANCEL:
			case GTK_RESPONSE_DELETE_EVENT:
			done = TRUE;
            password = NULL;
            break;

			case GTK_RESPONSE_OK:
			password  = g_strdup ( gtk_entry_get_text( GTK_ENTRY ( password_entry ) ) );
            if ( strlen ( password ) == 0 || strlen(gtk_entry_get_text( GTK_ENTRY ( repeat_password )) ) == 0 )
            {
                response = ShowGtkMessageDialog (GTK_WINDOW (MainWindow),GTK_DIALOG_MODAL,GTK_MESSAGE_ERROR,GTK_BUTTONS_OK,_("Please type a password !") );
                break;
            }
            if ( strcmp (password , gtk_entry_get_text( GTK_ENTRY ( repeat_password ) ) ) )
            {
                response = ShowGtkMessageDialog (GTK_WINDOW (MainWindow),GTK_DIALOG_MODAL,GTK_MESSAGE_ERROR,GTK_BUTTONS_OK,_("The passwords don't match !!") );
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
	if ( archive_error ) return;
	GtkTreeSelection *selection = gtk_tree_view_get_selection ( GTK_TREE_VIEW (treeview1) );
	gint selected = gtk_tree_selection_count_selected_rows ( selection );
	if (selected == 0 ) OffDeleteandViewButtons();
    else
	{
        if ( CurrentArchiveType != 8 )
        {
            gtk_widget_set_sensitive ( delete_menu , TRUE );
		    gtk_widget_set_sensitive ( Delete_button , TRUE );
        }
        if (selected > 1 || CurrentArchiveType == 8)
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
	EmptyTextBuffer ();
	compressor_pid = SpawnAsyncProcess ( command , 1 , 0);
	if ( compressor_pid == 0 ) return;
    gtk_widget_show ( viewport2 );
	SetIOChannel (output_fd, G_IO_IN|G_IO_PRI|G_IO_ERR|G_IO_HUP|G_IO_NVAL , GenOutput, NULL );
	SetIOChannel (error_fd, G_IO_IN|G_IO_PRI|G_IO_ERR|G_IO_HUP|G_IO_NVAL , GenError, NULL );
    WaitExitStatus ( child_pid , NULL );
}

void Update_StatusBar ( gchar *msg)
{
    gtk_label_set_text (GTK_LABEL (info_label), msg);
}

gboolean GenError (GIOChannel *ioc, GIOCondition cond, gpointer data)
{
	if (cond & (G_IO_IN | G_IO_PRI) )
	{
		gchar *line = NULL;
		g_io_channel_read_line ( ioc, &line, NULL, NULL, NULL );
        while (gtk_events_pending() )
			gtk_main_iteration();
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
		return FALSE;
	}
}

gboolean GenOutput (GIOChannel *ioc, GIOCondition cond, gpointer data)
{
	gchar *line = NULL;
	if (cond & (G_IO_IN | G_IO_PRI) )
	{
		g_io_channel_read_line ( ioc, &line, NULL, NULL, NULL );
        //gtk_progress_bar_pulse ( GTK_PROGRESS_BAR (progressbar) );
        while (gtk_events_pending() )
			gtk_main_iteration();
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
	return g_strconcat (extract_path,path,NULL);
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
    if ( ! PasswordProtectedArchive )
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
    if (field != NULL) field [ strlen(field) -1 ] = '\000';
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

    selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (treeview1));
    if ( ! gtk_tree_selection_get_selected (selection, &model, &iter)) return;
    gtk_tree_model_get (model, &iter, 0, &name, -1);
    gdk_property_change (context->source_window,
                       gdk_atom_intern ("XdndDirectSave0", FALSE),
                       gdk_atom_intern ("text/plain", FALSE), 8,
                       GDK_PROP_MODE_REPLACE, name, strlen (name) );
    g_free (name);
}

void drag_end (GtkWidget *treeview1,GdkDragContext *context, gpointer data)
{
   /*Nothing to do*/ 
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
    
    selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (treeview1));
    if ( ! gtk_tree_selection_get_selected (selection, &model, &iter) ) return;
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
        dummy_path = g_filename_from_uri ( fm_path, NULL, NULL );
        g_free ( fm_path );
        extract_path = extract_local_path ( dummy_path,name );
        g_free ( dummy_path );
        if (extract_path != NULL) to_send = "S";
        names = g_string_new ("");
        gtk_tree_selection_selected_foreach (selection, (GtkTreeSelectionForeachFunc) ConcatenateFileNames, names );
        command = ChooseCommandtoExecute ( 1 , names );
        if ( command != NULL )
        {
            action = extract;
            ExtractAddDelete ( command );
            g_free (command);
        }
        //g_dataset_set_data (dc, "XDS-sent", to_send);
        gtk_selection_data_set (selection_data, gdk_atom_intern ("XA_STRING", FALSE), 8, to_send, 1);
    }
    if (extract_path != NULL) g_free (extract_path);
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
        response = ShowGtkMessageDialog (GTK_WINDOW (MainWindow),GTK_DIALOG_MODAL,GTK_MESSAGE_ERROR,GTK_BUTTONS_OK,_("Sorry, I could not perform the operation.") );
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
            if ( DetectArchiveType ( filename ) >= 0)
            {
                on_open1_activate ( NULL, filename );
                g_strfreev ( array );
                return;
            }
        }
    }
    if ( CurrentArchiveType == -1) on_new1_activate ( NULL , NULL );
    if ( CurrentArchiveType > -1 )
    {
        if ( (CurrentArchiveType == 0 || CurrentArchiveType == 1) && ! one_file)
        {
            response = ShowGtkMessageDialog (GTK_WINDOW (MainWindow),GTK_DIALOG_MODAL,GTK_MESSAGE_ERROR,GTK_BUTTONS_OK,_("Bzip2 or gzip cannot compress more than one file, please choose another archive format !") );
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
        on_add_files_activate ( NULL, NULL );
        g_strfreev ( array );
    }
}

gboolean key_press_function (GtkWidget *widget, GdkEventKey *event, gpointer data)
{
    if (event == NULL) return FALSE;
	switch (event->keyval)
    {
	    case GDK_Escape:
	    if ( GTK_WIDGET_VISIBLE (viewport2) ) Cancel_Operation (NULL, NULL);
	    break;

	    case GDK_Delete:
        if ( GTK_WIDGET_STATE (Delete_button) != GTK_STATE_INSENSITIVE ) on_delete1_activate ( NULL , NULL );
		break;
    }
	return FALSE;
}
