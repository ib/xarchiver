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

#include <gtk/gtk.h>
#include "extract_dialog.h"
#include "interface.h"
#include "callbacks.h"
#include "support.h"

gchar *strip_string = NULL;

Extract_dialog_data *xa_create_extract_dialog (gint selected , XArchive *archive)
{
	Extract_dialog_data *dialog_data;

	dialog_data = g_new0 (Extract_dialog_data, 1);
	dialog_data->radio_group = NULL;
	dialog_data->dialog1 = gtk_dialog_new ();
	gtk_window_set_title (GTK_WINDOW (dialog_data->dialog1), _("Extract Dialog"));
	gtk_window_set_type_hint (GTK_WINDOW (dialog_data->dialog1), GDK_WINDOW_TYPE_HINT_DIALOG);
	gtk_window_set_transient_for ( GTK_WINDOW (dialog_data->dialog1) , GTK_WINDOW (MainWindow) );

	dialog_data->option_tooltip = gtk_tooltips_new ();
	dialog_data->dialog_vbox1 = GTK_DIALOG (dialog_data->dialog1)->vbox;
	gtk_widget_show (dialog_data->dialog_vbox1);

	dialog_data->vbox1 = gtk_vbox_new (FALSE, 0);
	gtk_widget_show (dialog_data->vbox1);
	gtk_box_pack_start (GTK_BOX (dialog_data->dialog_vbox1), dialog_data->vbox1, TRUE, TRUE, 0);

	dialog_data->hbox3 = gtk_hbox_new (FALSE, 0);
	gtk_widget_show (dialog_data->hbox3);
	gtk_box_pack_start (GTK_BOX (dialog_data->vbox1), dialog_data->hbox3, FALSE, TRUE, 17);

	dialog_data->extract_to_label = gtk_label_new (_("Extract to:"));
	gtk_widget_show (dialog_data->extract_to_label);
	gtk_box_pack_start (GTK_BOX (dialog_data->hbox3), dialog_data->extract_to_label, FALSE, TRUE, 0);

	dialog_data->destination_path_entry = gtk_entry_new ();
	gtk_widget_show (dialog_data->destination_path_entry);
	gtk_box_pack_start (GTK_BOX (dialog_data->hbox3), dialog_data->destination_path_entry, TRUE, TRUE, 0);
	gtk_widget_set_size_request (dialog_data->destination_path_entry, 385, -1);
	gtk_entry_set_activates_default (GTK_ENTRY (dialog_data->destination_path_entry), TRUE);

	gchar *dummy = StripPathFromFilename(archive->path, ".");
	if (dummy != NULL)
	{
		dummy++;
		unsigned short int x = strlen (archive->path) - strlen ( dummy );
		gchar *extraction_string = (gchar *) g_malloc ( x + 1);
		strncpy ( extraction_string, archive->path, x );
		extraction_string [x-1] = '\0';

		if ( strstr (extraction_string , ".tar") )
		{
			extraction_string = g_realloc ( extraction_string, x - 5);
			strncpy ( extraction_string, archive->path, x - 5);
			extraction_string[x-5] = '\0';
		}
		gtk_entry_set_text (GTK_ENTRY(dialog_data->destination_path_entry), extraction_string);
		g_free (extraction_string);
	}
	else
		gtk_entry_set_text (GTK_ENTRY(dialog_data->destination_path_entry), archive->path);

	dialog_data->button1 = gtk_button_new ();
	gtk_widget_set_size_request (dialog_data->button1, 33, 27);
	gtk_widget_show (dialog_data->button1);

	dialog_data->image1 = gtk_image_new_from_stock ("gtk-open", GTK_ICON_SIZE_BUTTON);
	gtk_widget_show (dialog_data->image1);
	gtk_box_pack_end(GTK_BOX (dialog_data->hbox3), dialog_data->button1, FALSE, TRUE, 0);
	gtk_widget_set_size_request (dialog_data->image1, 30, 30);

	gtk_container_add(GTK_CONTAINER(dialog_data->button1), dialog_data->image1);
	gtk_tooltips_set_tip (dialog_data->option_tooltip,dialog_data->button1 , _("Choose a folder where to extract files"), NULL );
	g_signal_connect ( (gpointer) dialog_data->button1, "clicked", G_CALLBACK (Show_File_Dialog) ,  "extract" );

	dialog_data->hbox4 = gtk_hbox_new (TRUE, 7);
	gtk_widget_show (dialog_data->hbox4);
	gtk_box_pack_start (GTK_BOX (dialog_data->vbox1), dialog_data->hbox4, FALSE, FALSE, 0);

	dialog_data->frame1 = gtk_frame_new (NULL);
	gtk_widget_show (dialog_data->frame1);
	gtk_box_pack_start (GTK_BOX (dialog_data->hbox4), dialog_data->frame1, TRUE, TRUE, 0);
	gtk_widget_set_size_request (dialog_data->frame1, 1, -1);
	gtk_frame_set_shadow_type (GTK_FRAME (dialog_data->frame1), GTK_SHADOW_OUT);

	dialog_data->alignment1 = gtk_alignment_new (0.5, 0.5, 1, 1);
	gtk_widget_show (dialog_data->alignment1);
	gtk_container_add (GTK_CONTAINER (dialog_data->frame1), dialog_data->alignment1);
	gtk_alignment_set_padding (GTK_ALIGNMENT (dialog_data->alignment1), 0, 0, 12, 0);

	dialog_data->vbox3 = gtk_vbox_new (FALSE, 0);
	gtk_widget_show (dialog_data->vbox3);
	gtk_container_add (GTK_CONTAINER (dialog_data->alignment1), dialog_data->vbox3);

	dialog_data->all_files_radio = gtk_radio_button_new_with_mnemonic (NULL, _("All"));
	gtk_widget_show (dialog_data->all_files_radio);
	gtk_box_pack_start (GTK_BOX (dialog_data->vbox3), dialog_data->all_files_radio, FALSE, FALSE, 0);
	gtk_radio_button_set_group (GTK_RADIO_BUTTON (dialog_data->all_files_radio), dialog_data->radio_group);
	dialog_data->radio_group = gtk_radio_button_get_group (GTK_RADIO_BUTTON (dialog_data->all_files_radio));

	dialog_data->selected_files_radio = gtk_radio_button_new_with_mnemonic (NULL, _("Only selected"));
	gtk_widget_show (dialog_data->selected_files_radio);
	gtk_box_pack_start (GTK_BOX (dialog_data->vbox3), dialog_data->selected_files_radio, FALSE, FALSE, 0);
	gtk_radio_button_set_group (GTK_RADIO_BUTTON (dialog_data->selected_files_radio), dialog_data->radio_group);
	dialog_data->radio_group = gtk_radio_button_get_group (GTK_RADIO_BUTTON (dialog_data->selected_files_radio));

	if (selected)
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (dialog_data->selected_files_radio), TRUE);
	else
		gtk_widget_set_sensitive (dialog_data->selected_files_radio , FALSE);
	
	dialog_data->files_frame_label = gtk_label_new (_("<b>Files to extract </b>"));
	gtk_widget_show (dialog_data->files_frame_label);
	gtk_frame_set_label_widget (GTK_FRAME (dialog_data->frame1), dialog_data->files_frame_label);
	gtk_label_set_use_markup (GTK_LABEL (dialog_data->files_frame_label), TRUE);

	dialog_data->frame2 = gtk_frame_new (NULL);
	gtk_widget_show (dialog_data->frame2);
	gtk_box_pack_start (GTK_BOX (dialog_data->hbox4), dialog_data->frame2, TRUE, TRUE, 0);
	gtk_frame_set_shadow_type (GTK_FRAME (dialog_data->frame2), GTK_SHADOW_OUT);

	dialog_data->alignment2 = gtk_alignment_new (0.5, 0.5, 1, 1);
	gtk_widget_show (dialog_data->alignment2);
	gtk_container_add (GTK_CONTAINER (dialog_data->frame2), dialog_data->alignment2);
	gtk_alignment_set_padding (GTK_ALIGNMENT (dialog_data->alignment2), 0, 0, 12, 0);

	dialog_data->vbox4 = gtk_vbox_new (FALSE, 0);
	gtk_widget_show (dialog_data->vbox4);
	gtk_container_add (GTK_CONTAINER (dialog_data->alignment2), dialog_data->vbox4);

	dialog_data->overwrite_check = gtk_check_button_new_with_mnemonic (_("Overwrite existing files"));
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (dialog_data->overwrite_check), archive->overwrite);
	gtk_widget_show (dialog_data->overwrite_check);
	gtk_box_pack_start (GTK_BOX (dialog_data->vbox4), dialog_data->overwrite_check, FALSE, FALSE, 0);

	if (archive->type == XARCHIVETYPE_RAR || archive->type == XARCHIVETYPE_ZIP || archive->type == XARCHIVETYPE_ARJ || archive->type == XARCHIVETYPE_7ZIP || archive->type == XARCHIVETYPE_ISO) 
	{
		dialog_data->extract_full = gtk_check_button_new_with_mnemonic (_("Extract files with full path"));
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (dialog_data->extract_full), archive->full_path);
		gtk_widget_show (dialog_data->extract_full);
		gtk_tooltips_set_tip (dialog_data->option_tooltip,dialog_data->extract_full , _("The archive's directory structure is recreated in the extraction directory."), NULL );
		gtk_box_pack_start (GTK_BOX (dialog_data->vbox4), dialog_data->extract_full, FALSE, FALSE, 0);
	}

	if (archive->type == XARCHIVETYPE_TAR || archive->type == XARCHIVETYPE_TAR_GZ || archive->type == XARCHIVETYPE_TAR_BZ2)
	{
		dialog_data->touch = gtk_check_button_new_with_mnemonic (_("Touch files"));
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (dialog_data->touch), archive->tar_touch);
		gtk_widget_show (dialog_data->touch);
		gtk_tooltips_set_tip (dialog_data->option_tooltip,dialog_data->touch, _("When this option is used, tar leaves the data modification times of the files it extracts as the times when the files were extracted, instead of setting it to the times recorded in the archive."), NULL );
		gtk_box_pack_start (GTK_BOX (dialog_data->vbox4), dialog_data->touch, FALSE, FALSE, 0);

		dialog_data->hbox6 = gtk_hbox_new (FALSE, 2);
		gtk_widget_show (dialog_data->hbox6);
		gtk_box_pack_start (GTK_BOX (dialog_data->vbox4), dialog_data->hbox6, FALSE, FALSE, 0);

		dialog_data->strip = gtk_check_button_new_with_mnemonic (_("Strip directories:"));
		gtk_widget_show (dialog_data->strip);
		gtk_box_pack_start (GTK_BOX (dialog_data->hbox6), dialog_data->strip, FALSE, FALSE, 0);
		gtk_tooltips_set_tip (dialog_data->option_tooltip,dialog_data->strip , _("This option extracts the files without the directory in which they are contained. You have to specify the number of directories to strip."), NULL );

		dialog_data->strip_entry = gtk_entry_new ();
		gtk_widget_set_size_request (dialog_data->strip_entry, 24, -1);
		gtk_entry_set_max_length (GTK_ENTRY (dialog_data->strip_entry), 2);

		if ( ! archive->full_path )
		{
			gchar *strip_text;
			strip_text = g_strdup_printf ( "%d",archive->tar_strip_value);
			gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (dialog_data->strip), TRUE);
			gtk_entry_set_text (GTK_ENTRY (dialog_data->strip_entry), strip_text );
			g_free (strip_text);
			gtk_widget_set_sensitive (dialog_data->strip_entry , TRUE);
		}
		else
			gtk_widget_set_sensitive (dialog_data->strip_entry , FALSE);
		gtk_widget_show (dialog_data->strip_entry);
		gtk_box_pack_start (GTK_BOX (dialog_data->hbox6), dialog_data->strip_entry, FALSE, FALSE, 0);
		g_signal_connect ( (gpointer) dialog_data->strip, "toggled", G_CALLBACK (show_hide_strip_entry) , dialog_data );
	}
	else
		dialog_data->touch = NULL;

	if (archive->type == XARCHIVETYPE_RAR || archive->type == XARCHIVETYPE_ZIP || archive->type == XARCHIVETYPE_ARJ)
	{
		dialog_data->fresh = gtk_check_button_new_with_mnemonic (_("Freshen existing files"));
		gtk_tooltips_set_tip (dialog_data->option_tooltip,dialog_data->fresh , _("Extract only those files that already exist on disk and that are newer than the disk copies."), NULL );
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (dialog_data->fresh), archive->freshen);
		gtk_widget_show (dialog_data->fresh);
		gtk_box_pack_start (GTK_BOX (dialog_data->vbox4), dialog_data->fresh, FALSE, FALSE, 0);
		g_signal_connect (G_OBJECT (dialog_data->fresh),"toggled",G_CALLBACK (fresh_update_toggled_cb) , dialog_data);

		dialog_data->update = gtk_check_button_new_with_mnemonic (_("Update existing files"));
		gtk_tooltips_set_tip (dialog_data->option_tooltip,dialog_data->update , _("This option performs the same function as the freshen one, extracting files that are newer than those with the same name on disk, and in addition it extracts those files that do not already exist on disk."), NULL );
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (dialog_data->update), archive->update);
		gtk_widget_show (dialog_data->update);
		gtk_box_pack_start (GTK_BOX (dialog_data->vbox4), dialog_data->update, FALSE, FALSE, 0);
		g_signal_connect (G_OBJECT (dialog_data->update),"toggled",G_CALLBACK (update_fresh_toggled_cb) , dialog_data);
	}
	else
	{
		dialog_data->fresh = NULL;
		dialog_data->update = NULL;
	}
	dialog_data->hbox5 = gtk_hbox_new (FALSE, 2);
	gtk_widget_show (dialog_data->hbox5);
	gtk_box_pack_start (GTK_BOX (dialog_data->vbox4), dialog_data->hbox5, FALSE, FALSE, 0);

	dialog_data->label_password = gtk_label_new (_("Password:"));
	gtk_widget_show (dialog_data->label_password);
	gtk_box_pack_start (GTK_BOX (dialog_data->hbox5), dialog_data->label_password, FALSE, FALSE, 0);

	dialog_data->password_entry = gtk_entry_new ();
	gtk_widget_show (dialog_data->password_entry);
	gtk_box_pack_start (GTK_BOX (dialog_data->hbox5), dialog_data->password_entry, FALSE, FALSE, 0);
	gtk_entry_set_visibility (GTK_ENTRY (dialog_data->password_entry), FALSE);
	if ( archive->has_passwd )
    {
		gtk_widget_set_sensitive (dialog_data->label_password, TRUE);
		gtk_widget_set_sensitive (dialog_data->password_entry, TRUE);
		if (archive->passwd != NULL)
			gtk_entry_set_text (GTK_ENTRY(dialog_data->password_entry) , archive->passwd);
    }
	else
	{
		gtk_widget_set_sensitive (dialog_data->label_password, FALSE);
		gtk_widget_set_sensitive (dialog_data->password_entry, FALSE);
	}

	dialog_data->options_frame_label = gtk_label_new (_("<b>Options </b>"));
	gtk_widget_show (dialog_data->options_frame_label);
	gtk_frame_set_label_widget (GTK_FRAME (dialog_data->frame2), dialog_data->options_frame_label);
	gtk_label_set_use_markup (GTK_LABEL (dialog_data->options_frame_label), TRUE);

	dialog_data->dialog_action_area1 = GTK_DIALOG (dialog_data->dialog1)->action_area;
	gtk_widget_show (dialog_data->dialog_action_area1);
	gtk_button_box_set_layout (GTK_BUTTON_BOX (dialog_data->dialog_action_area1), GTK_BUTTONBOX_END);

	dialog_data->cancel_button = gtk_button_new_from_stock(GTK_STOCK_CANCEL);
	gtk_widget_show (dialog_data->cancel_button);
	gtk_dialog_add_action_widget (GTK_DIALOG (dialog_data->dialog1), dialog_data->cancel_button, GTK_RESPONSE_CANCEL);
	GTK_WIDGET_SET_FLAGS (dialog_data->cancel_button, GTK_CAN_DEFAULT);

	dialog_data->extract_button = gtk_button_new();
	dialog_data->extract_image = xa_main_window_find_image("extract_button.png", GTK_ICON_SIZE_SMALL_TOOLBAR);
	dialog_data->extract_hbox = gtk_hbox_new(FALSE, 4);
	dialog_data->extract_label = gtk_label_new_with_mnemonic(_("_Extract"));

	dialog_data->alignment3 = gtk_alignment_new (0.5, 0.5, 0, 0);
	gtk_widget_show (dialog_data->alignment3);
	gtk_container_add (GTK_CONTAINER (dialog_data->alignment3), dialog_data->extract_hbox);

	gtk_box_pack_start(GTK_BOX(dialog_data->extract_hbox), dialog_data->extract_image, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(dialog_data->extract_hbox), dialog_data->extract_label, FALSE, FALSE, 0);
	gtk_widget_show_all(dialog_data->extract_hbox);
	gtk_container_add(GTK_CONTAINER(dialog_data->extract_button), dialog_data->alignment3);
	gtk_widget_show (dialog_data->extract_button);
	gtk_dialog_add_action_widget (GTK_DIALOG (dialog_data->dialog1), dialog_data->extract_button, GTK_RESPONSE_OK);
	GTK_WIDGET_SET_FLAGS (dialog_data->extract_button, GTK_CAN_DEFAULT);
	gtk_dialog_set_default_response (GTK_DIALOG (dialog_data->dialog1), GTK_RESPONSE_OK);
	return dialog_data;
}

void fresh_update_toggled_cb (GtkToggleButton *button, Extract_dialog_data *data)
{
	gboolean active = gtk_toggle_button_get_active (button);
	if (active)
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (data->update), FALSE);
}

void update_fresh_toggled_cb (GtkToggleButton *button, Extract_dialog_data *data)
{
	if (data->fresh == NULL)
		return;
	gboolean active = gtk_toggle_button_get_active (button);
	if (active)
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (data->fresh), FALSE);
}

void show_hide_strip_entry (GtkToggleButton *button, Extract_dialog_data *data)
{
	gboolean active = FALSE;
	if (gtk_toggle_button_get_active (button) )
		active = TRUE;
	gtk_widget_set_sensitive (data->strip_entry, active);
	gtk_widget_grab_focus (data->strip_entry);
}

gchar *xa_parse_extract_dialog_options ( XArchive *archive , Extract_dialog_data *dialog_data, GtkTreeSelection *selection)
{
	gchar *command = NULL;
	gchar *tar;
	gchar *destination_path = NULL;
	gboolean done = FALSE;
	gboolean end = FALSE;
	GtkTreeIter iter;

    while ( ! done )
	{
		switch (gtk_dialog_run ( GTK_DIALOG (dialog_data->dialog1 ) ) )
		{
			case GTK_RESPONSE_CANCEL:
			case GTK_RESPONSE_DELETE_EVENT:
			done = TRUE;
			break;

			case GTK_RESPONSE_OK:
			destination_path = g_strdup (gtk_entry_get_text ( GTK_ENTRY (dialog_data->destination_path_entry) ));
			extract_path = EscapeBadChars ( destination_path );

			if ( strlen ( extract_path ) == 0 )
			{
				response = ShowGtkMessageDialog (GTK_WINDOW (MainWindow),GTK_DIALOG_MODAL,GTK_MESSAGE_ERROR,GTK_BUTTONS_OK, _("Please select where to extract the files !") );
				break;
			}
			if (archive->has_passwd)
				archive->passwd  = g_strdup (gtk_entry_get_text ( GTK_ENTRY (dialog_data->password_entry) ));

			if (archive->has_passwd && strlen( archive->passwd ) == 0 )
			{
				response = ShowGtkMessageDialog (GTK_WINDOW (MainWindow),GTK_DIALOG_MODAL,GTK_MESSAGE_ERROR,GTK_BUTTONS_OK, _("Please enter the password!") );
				break;
			}
			if (g_file_test (destination_path , G_FILE_TEST_EXISTS) == FALSE )
			{
				int result = mkdir (destination_path , S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IXGRP | S_IROTH | S_IXGRP);
				if (result == -1)
				{
					gchar *msg = g_strconcat (_("Can't create directory "),"\"",destination_path,"\"",": ",g_strerror(errno),NULL);
					response = ShowGtkMessageDialog (GTK_WINDOW (MainWindow),GTK_DIALOG_MODAL,GTK_MESSAGE_ERROR,GTK_BUTTONS_OK, msg );
					g_free (msg);
					break;
				}
			}
			if (access (destination_path, R_OK | W_OK | X_OK) != 0)
			{
				gchar *utf8_path;
				gchar  *msg;

                utf8_path = g_filename_to_utf8 (destination_path, -1, NULL, NULL, NULL);
                msg = g_strdup_printf (_("You don't have the right permissions to extract archives in the folder \"%s\""), utf8_path);
				response = ShowGtkMessageDialog (GTK_WINDOW (MainWindow),GTK_DIALOG_MODAL,GTK_MESSAGE_ERROR,GTK_BUTTONS_OK, msg );
                g_free (utf8_path);
				g_free (msg);
				g_free (destination_path);
				break;
			}
			done = TRUE;
			archive->overwrite = gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON ( dialog_data->overwrite_check ));
			if ( dialog_data->touch != NULL)
				archive->tar_touch = gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON ( dialog_data->touch ));
			
			if ( dialog_data->strip != NULL)
			{
				archive->full_path = ! gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON ( dialog_data->strip ));
				archive->tar_strip_value = atoi (gtk_entry_get_text (GTK_ENTRY(dialog_data->strip_entry)) );
				gchar *digit;
				digit = g_strdup_printf ("%d", archive->tar_strip_value );
				strip_string = g_strconcat ( "--strip-components=" , digit , " " , NULL );
				g_free (digit);
			}
			else
			{
				if (dialog_data->extract_full != NULL)
				{
					archive->full_path = gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON ( dialog_data->extract_full ));
					archive->tar_strip_value = 0;
				}
			}

			if (dialog_data->fresh != NULL)
				archive->freshen = gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON ( dialog_data->fresh ));

			if (dialog_data->update != NULL)
				archive->update = gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON ( dialog_data->update ));

			gtk_widget_set_sensitive (Stop_button,TRUE);
			if ( gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON ( dialog_data->all_files_radio )) )
			{
				gchar *text = g_strconcat (_("Extracting files to "), destination_path , NULL );
				Update_StatusBar ( text );
				g_free (text);
				g_free (destination_path);
        tar = g_find_program_in_path ("gtar");
        if (tar == NULL)
          tar = g_strdup ("tar");
				switch ( archive->type )
				{
					case XARCHIVETYPE_RAR:
					if (archive->passwd != NULL)
						command = g_strconcat ( "rar " , archive->full_path ? "x " : "e ",
												archive->freshen ? "-f " : "" , archive->update ? "-u " : "",
												" -p",archive->passwd,
												archive->overwrite ? " -o+" : " -o-",
												" -idp ",
												archive->escaped_path , " " , extract_path , NULL );
					else
						command = g_strconcat ( "rar " , archive->full_path ? "x " : "e ",
												archive->freshen ? "-f " : "" , archive->update ? "-u " : "",
												archive->overwrite ? "-o+" : "-o-",
												" -idp ",
												archive->escaped_path , " " , extract_path , NULL );
					break;

					case XARCHIVETYPE_TAR:
					command = g_strconcat (tar, " ",archive->full_path ? "" : strip_string,
											"-xvf ", archive->escaped_path,
											archive->overwrite ? " --overwrite" : " --keep-old-files",
											archive->tar_touch ? " --touch" : "",
											" -C " , extract_path , NULL );
					break;

					case XARCHIVETYPE_TAR_BZ2:
					command = g_strconcat (tar, " ",archive->full_path ? "" : strip_string,
											"-xvjf " , archive->escaped_path,
											archive->overwrite ? " --overwrite" : " --keep-old-files",
											archive->tar_touch ? " --touch" : "",
											" -C " , extract_path , NULL );
					break;

					case XARCHIVETYPE_TAR_GZ:
					command = g_strconcat (tar, " ",archive->full_path ? "" : strip_string,
											"-xvzf " , archive->escaped_path,
											archive->overwrite ? " --overwrite" : " --keep-old-files",
											archive->tar_touch ? " --touch" : "",
											" -C " , extract_path , NULL );
					break;

                    case XARCHIVETYPE_ZIP:
                    if ( archive->passwd != NULL )
						command = g_strconcat ( "unzip ", archive->freshen ? "-f " : "",
												archive->update ? "-u " : "" ,
												archive->overwrite ? "-o" : "-n",
												" -P " , archive->passwd,
												archive->full_path ? "" : " -j ",
												archive->escaped_path , " -d ", extract_path , NULL );
                    else
						command = g_strconcat ( "unzip ", archive->freshen ? "-f " : "",
												archive->update ? "-u " : "",
												archive->overwrite ? "-o " : "-n ",
												archive->full_path ? "" : " -j ",
												archive->escaped_path , " -d ", extract_path , NULL );
					break;

					case XARCHIVETYPE_RPM:
                    chdir ( extract_path );
                    command = g_strconcat ( "cpio --make-directories -F " , archive->tmp , " -i" , NULL );
                    break;

                    case XARCHIVETYPE_7ZIP:
                    if (archive->passwd != NULL)
						command = g_strconcat ( "7za " , archive->full_path ? "x " : "e ",
												archive->overwrite ? "-aoa" : "-aos",
												" -bd -p",archive->passwd," ",
												archive->escaped_path , " -o" , extract_path , NULL );
					else
						command = g_strconcat ( "7za " , archive->full_path ? "x " : "e ",
												archive->overwrite ? "-aoa" : "-aos",
												" -bd ",
												archive->escaped_path , " -o" , extract_path , NULL );
                    break;

					case XARCHIVETYPE_ARJ:
					if (archive->passwd != NULL)
						command = g_strconcat ( "arj " , archive->full_path ? "x " : "e ",
												"-g",archive->passwd,
												archive->overwrite ? "" : " -n" , " -i ",
												archive->freshen ? "-f " : "" , archive->update ? "-u " : "",
												"-y " , archive->escaped_path , " " , extract_path , NULL );
                    else
						command = g_strconcat ( "arj " , archive->full_path ? "x " : "e ",
												archive->overwrite ? "" : " -n" , " -i ",
												archive->freshen ? "-f " : "" , archive->update ? "-u " : "",
												"-y " , archive->escaped_path , " " , extract_path , NULL );
					break;

					case XARCHIVETYPE_ISO:
					end = gtk_tree_model_get_iter_first (model , &iter);
					gtk_widget_show ( viewport2 );
					g_timeout_add (200, xa_progressbar_pulse, NULL );
					gtk_widget_destroy (dialog_data->dialog1);
					dialog_data->dialog1 = NULL;
					while (end)
					{
						gtk_tree_model_get (model, &iter,
						0, &name,
						1, &permissions,
						2, &file_size,
						4, &file_offset,
						-1);
						if (xa_extract_iso_file (archive, permissions, extract_path, name , file_size, file_offset ) == FALSE )
						{
							g_free (name);
							g_free (permissions);
							return NULL;
						}
						g_free (name);
						g_free (permissions);
						end = gtk_tree_model_iter_next (model,&iter);
					}
					xa_set_button_state (1,1,0,1,1);
					OffTooltipPadlock();
					Update_StatusBar ( _("Operation completed.") );
					break;
						
					default:
					command = NULL;
				}
				g_free (tar);

				if ( command != NULL )
					return command;
			}
			/* Here we take care of the selected files only */
			else
			{
				/* ISO extraction is different from the other type of archives */
				if (archive->type == XARCHIVETYPE_ISO)
				{
					GList *row_list = NULL;
					selection = gtk_tree_view_get_selection ( GTK_TREE_VIEW (treeview1) );
					row_list = gtk_tree_selection_get_selected_rows (selection, &model);
					while (row_list)
					{
						gtk_tree_model_get_iter(model, &iter, row_list->data);
						gtk_tree_model_get (model, &iter,
						0, &name,
						1, &permissions,
						2, &file_size,
						4, &file_offset,
						-1);
						gtk_tree_path_free (row_list->data);

						xa_extract_iso_file (archive, permissions, extract_path, name , file_size, file_offset );
						g_free (name);
						g_free (permissions);
						row_list = row_list->next;
					}
					g_list_free (row_list);
					xa_set_button_state (1,1,0,1,1);
					OffTooltipPadlock();
					Update_StatusBar ( _("Operation completed.") );
				}
				/* Let's handle the other archive types */
				else
				{
					names = g_string_new ( " " );
					gtk_tree_selection_selected_foreach (selection, (GtkTreeSelectionForeachFunc) ConcatenateFileNames, names );
					command = xa_extract_single_files ( archive , names, extract_path );
					g_string_free (names, TRUE);
				}
			}
		}
	}
	return command;
}

gchar *xa_extract_single_files ( XArchive *archive , GString *files, gchar *path)
{
	gchar *command;
	gchar *tar;

	if ( archive->full_path == 0)
	{
		if (archive->tar_strip_value == 0)
			archive->tar_strip_value = CountCharacter ( files->str , '/');
		gchar *digit;
		digit = g_strdup_printf ( "%d" , archive->tar_strip_value );
		strip_string = g_strconcat ( "--strip-components=" , digit , " " , NULL );
		g_free (digit);
	}
	gchar *msg = g_strconcat ( _("Extracting files to ") , path, NULL);
	Update_StatusBar (msg);
	g_free (msg);
	tar = g_find_program_in_path ("gtar");
	if (tar == NULL)
		tar = g_strdup ("tar");
	switch (archive->type)
	{
		case XARCHIVETYPE_RAR:
		if (archive->passwd != NULL)
			command = g_strconcat ( "rar " , archive->full_path ? "x " : "e " ,
									archive->freshen ? "-f " : "" , archive->update ? "-u " : "",
									" -p",archive->passwd,
									archive->overwrite ? " -o+" : " -o-",
									" -idp ",
									archive->escaped_path , " " , files->str , " " , path , NULL );
        else
			command = g_strconcat ( "rar ", archive->full_path ? "x " : "e " ,
									archive->freshen ? "-f " : "" , archive->update ? "-u " : "",
									archive->overwrite ? "-o+" : "-o-",
									" -idp ",
									archive->escaped_path , " " , files->str , " ", path ,NULL);
		break;

		case XARCHIVETYPE_TAR:
	    command = g_strconcat (tar, " ",archive->full_path ? "" : strip_string,
								"-xvf " , archive->escaped_path,
								archive->overwrite ? " --overwrite" : " --keep-old-files",
								archive->tar_touch ? " --touch" : "",
								" -C " , path , files->str , NULL );
		break;

		case XARCHIVETYPE_TAR_BZ2:
		command = g_strconcat (tar, " ",archive->full_path ? "" : strip_string,
								"-xjvf " , archive->escaped_path,
								archive->overwrite ? " --overwrite" : " --keep-old-files",
								archive->tar_touch ? " --touch" : "",
								" -C " , path , files->str , NULL );
		break;

		case XARCHIVETYPE_TAR_GZ:
        command = g_strconcat (tar, " ",archive->full_path ? "" : strip_string,
								"-xzvf " , archive->escaped_path,
								archive->overwrite ? " --overwrite" : " --keep-old-files",
								archive->tar_touch ? " --touch" : "",
								" -C " , path , files->str , NULL );
		break;

		case XARCHIVETYPE_ZIP:
        if ( archive->passwd != NULL )
			command = g_strconcat ( "unzip ", archive->freshen ? "-f " : "",
									archive->update ? "-u " : "",
									archive->overwrite ? "-o" : "-n",
									" -P " , archive->passwd,
									archive->full_path ? "" : " -j ",
									archive->escaped_path , files->str," -d " , path , NULL );
        else
			command = g_strconcat ( "unzip ", archive->freshen ? "-f " : "",
									archive->update ? "-u " : "",
									archive->overwrite ? "-o " : "-n ",
									archive->full_path ? "" : " -j ",
									archive->escaped_path , files->str," -d " , path , NULL );
		break;

        case XARCHIVETYPE_RPM:
        chdir ( path );
        command = g_strconcat ( "cpio --make-directories " , files->str , " -F " , archive->tmp , " -i" , NULL);
        break;

        case XARCHIVETYPE_7ZIP:
        if ( archive->passwd != NULL)
			command = g_strconcat ("7za " , archive->full_path ? "x" : "e",
									" -p",archive->passwd,
									archive->overwrite ? " -aoa" : " -aos",
									" -bd ",
									archive->escaped_path , files->str , " -o" , path , NULL );
        else
			command = g_strconcat ( "7za " , archive->full_path ? "x" : "e",
									archive->overwrite ? " -aoa" : " -aos",
									" -bd ",
									archive->escaped_path , files->str , " -o" , path , NULL );
        break;

		case XARCHIVETYPE_ARJ:
		if (archive->passwd != NULL)
			command = g_strconcat ( "arj ",archive->full_path ? "x" : "e",
									" -g",archive->passwd,
									archive->overwrite ? "" : " -n" , 
									" -i " ,
									archive->freshen ? "-f " : "" ,
									archive->update ? "-u " : " ",
									"-y ",
									archive->escaped_path , " " , path , files->str , NULL );
        else
			command = g_strconcat ( "arj ",archive->full_path ? "x" : "e",
									archive->overwrite ? "" : " -n" ,
									" -i " , archive->freshen ? "-f " : "",
									archive->update ? "-u " : " ",
									"-y ",
									archive->escaped_path , " " , path , files->str, NULL );
		break;
		
		default:
		command = NULL;
    }
	g_free (tar);
    if ( strip_string != NULL)
	{
		g_free ( strip_string );
		strip_string = NULL;
	}
    return command;
}

