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
#include <stdlib.h>
#include "extract_dialog.h"
#include "interface.h"
#include "callbacks.h"
#include "string_utils.h"
#include "support.h"

gboolean stop_flag;
extern gboolean cli;
extern gboolean unrar;
gchar *rar;

Extract_dialog_data *xa_create_extract_dialog (gint selected , XArchive *archive)
{
	Extract_dialog_data *dialog_data;
	stop_flag = FALSE;

	dialog_data = g_new0 (Extract_dialog_data, 1);
	dialog_data->radio_group = NULL;
	dialog_data->dialog1 = gtk_dialog_new ();
	if (archive->type == XARCHIVETYPE_BZIP2 || archive->type == XARCHIVETYPE_GZIP)
		gtk_window_set_title (GTK_WINDOW (dialog_data->dialog1), _("Decompress file"));
	else
		gtk_window_set_title (GTK_WINDOW (dialog_data->dialog1), _("Extract files from archive"));
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

	gchar *dummy = g_strrstr (archive->path, ".");
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
	g_signal_connect ( (gpointer) dialog_data->button1, "clicked", G_CALLBACK (xa_choose_extraction_directory) , dialog_data );

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

	dialog_data->extract_full = gtk_check_button_new_with_mnemonic (_("Extract files with full path"));
	if (archive->type == XARCHIVETYPE_GZIP || archive->type == XARCHIVETYPE_BZIP2 )
		goto here;
	if (cli && (archive->type == XARCHIVETYPE_TAR || archive->type == XARCHIVETYPE_TAR_GZ || archive->type == XARCHIVETYPE_DEB || archive->type == XARCHIVETYPE_TAR_BZ2) )
	{
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (dialog_data->extract_full), TRUE);
		gtk_widget_set_sensitive (dialog_data->extract_full, FALSE);
	}
	else
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (dialog_data->extract_full), archive->full_path);
	gtk_widget_show (dialog_data->extract_full);
	gtk_tooltips_set_tip (dialog_data->option_tooltip,dialog_data->extract_full , _("The archive's directory structure is recreated in the extraction directory."), NULL );
	gtk_box_pack_start (GTK_BOX (dialog_data->vbox4), dialog_data->extract_full, FALSE, FALSE, 0);

	if (archive->type == XARCHIVETYPE_TAR || archive->type == XARCHIVETYPE_TAR_GZ || archive->type == XARCHIVETYPE_TAR_BZ2 || archive->type == XARCHIVETYPE_DEB)
	{
		dialog_data->touch = gtk_check_button_new_with_mnemonic (_("Touch files"));
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (dialog_data->touch), archive->tar_touch);
		gtk_widget_show (dialog_data->touch);
		gtk_tooltips_set_tip (dialog_data->option_tooltip,dialog_data->touch, _("When this option is used, tar leaves the data modification times of the files it extracts as the times when the files were extracted, instead of setting it to the times recorded in the archive."), NULL );
		gtk_box_pack_start (GTK_BOX (dialog_data->vbox4), dialog_data->touch, FALSE, FALSE, 0);
here:
		dialog_data->hbox6 = gtk_hbox_new (FALSE, 2);
		gtk_widget_show (dialog_data->hbox6);
		gtk_box_pack_start (GTK_BOX (dialog_data->vbox4), dialog_data->hbox6, FALSE, FALSE, 0);

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
	dialog_data->extract_image = xa_main_window_find_image("xarchiver-extract_button.png", GTK_ICON_SIZE_SMALL_TOOLBAR);
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

gchar *xa_parse_extract_dialog_options ( XArchive *archive , Extract_dialog_data *dialog_data, GtkTreeSelection *selection)
{
	gchar *command = NULL;
	gchar *tar;
	gchar *destination_path = NULL;
	gboolean done = FALSE;
	gboolean end = FALSE;
	GtkTreeIter iter;
	GString *names;

	if (unrar)
		rar = "unrar";
	else
		rar = "rar";

    while ( ! done )
	{
		switch (gtk_dialog_run ( GTK_DIALOG (dialog_data->dialog1 ) ) )
		{
			case GTK_RESPONSE_CANCEL:
			case GTK_RESPONSE_DELETE_EVENT:
			done = TRUE;
			if (archive->type == XARCHIVETYPE_GZIP || archive->type == XARCHIVETYPE_BZIP2)
			{
				gtk_widget_set_sensitive (Stop_button,FALSE);
				Update_StatusBar (_("Operation canceled.") );
				gtk_widget_hide (viewport2);
				xa_set_button_state (1,1,0,0,0,0,0);
				archive->status = XA_ARCHIVESTATUS_IDLE;
			}
			break;

			case GTK_RESPONSE_OK:
			destination_path = g_strdup (gtk_entry_get_text ( GTK_ENTRY (dialog_data->destination_path_entry) ));
			if (archive->type != XARCHIVETYPE_ISO)
				archive->extraction_path = EscapeBadChars ( destination_path , "$\'`\"\\!?* ()&|@#:;" );
			else
				archive->extraction_path = g_strdup ( destination_path );

			if ( strlen ( archive->extraction_path ) == 0 )
			{
				response = ShowGtkMessageDialog (GTK_WINDOW (MainWindow),GTK_DIALOG_MODAL,GTK_MESSAGE_ERROR,GTK_BUTTONS_OK, _("You missed where to extract the files!"),_("Please enter the extraction path.") );
				break;
			}
			if (archive->has_passwd)
				archive->passwd  = g_strdup (gtk_entry_get_text ( GTK_ENTRY (dialog_data->password_entry) ));

			if (archive->has_passwd && strlen( archive->passwd ) == 0 )
			{
				response = ShowGtkMessageDialog (GTK_WINDOW (MainWindow),GTK_DIALOG_MODAL,GTK_MESSAGE_ERROR,GTK_BUTTONS_OK, _("This archive is encrypted!"),_("Please enter the password.") );
				break;
			}
			if (g_file_test (destination_path , G_FILE_TEST_EXISTS) == FALSE)
			{
				int result = mkdir (destination_path , S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IXGRP | S_IROTH | S_IXGRP);
				if (result == -1)
				{
					gchar *msg = g_strdup_printf(_("Can't create directory \"%s\""), destination_path);
					response = ShowGtkMessageDialog (GTK_WINDOW (MainWindow),GTK_DIALOG_MODAL,GTK_MESSAGE_ERROR,GTK_BUTTONS_OK, msg, g_strerror(errno ) );
					g_free (msg);
					break;
				}
			}
			if ( g_file_test (destination_path , G_FILE_TEST_IS_DIR) && access (destination_path, R_OK | W_OK | X_OK ) )
			{
				gchar *utf8_path;
				gchar  *msg;

                utf8_path = g_filename_to_utf8 (destination_path, -1, NULL, NULL, NULL);
                msg = g_strdup_printf (_("You don't have the right permissions to extract the files to the folder \"%s\"."), utf8_path);
				response = ShowGtkMessageDialog (GTK_WINDOW (MainWindow),GTK_DIALOG_MODAL,GTK_MESSAGE_ERROR,GTK_BUTTONS_OK, _("Can't perform extraction!"),msg );
                g_free (utf8_path);
				g_free (msg);
				g_free (destination_path);
				break;
			}
			done = TRUE;
			archive->overwrite = gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON ( dialog_data->overwrite_check ));
			if ( dialog_data->touch != NULL)
				archive->tar_touch = gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON ( dialog_data->touch ));

			if (dialog_data->extract_full != NULL)
				archive->full_path = gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON ( dialog_data->extract_full ));

			if (dialog_data->fresh != NULL)
				archive->freshen = gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON ( dialog_data->fresh ));

			if (dialog_data->update != NULL)
				archive->update = gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON ( dialog_data->update ));

			gtk_widget_set_sensitive (Stop_button,TRUE);
			gtk_widget_hide (dialog_data->dialog1);
			archive->status = XA_ARCHIVESTATUS_EXTRACT;
			/* Are all files selected? */
			if ( gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON ( dialog_data->all_files_radio )) )
			{
				if ( ! cli )
				{
					gchar *text = g_strdup_printf(_("Extracting files to %s"), destination_path);
					Update_StatusBar ( text );
					g_free (text);
				}
				g_free (destination_path);
        tar = g_find_program_in_path ("gtar");
        if (tar == NULL)
          tar = g_strdup ("tar");
				switch ( archive->type )
				{
					case XARCHIVETYPE_BZIP2:
					gzip_bzip2_extract (archive , 0);
					break;

					case XARCHIVETYPE_GZIP:
					gzip_bzip2_extract (archive , 1);
					break;

					case XARCHIVETYPE_RAR:
					if (archive->passwd != NULL)
						command = g_strconcat ( rar, " " , archive->full_path ? "x " : "e ",
												archive->freshen ? "-f " : "" , archive->update ? "-u " : "",
												" -p",archive->passwd,
												archive->overwrite ? " -o+" : " -o-",
												" -idp ",
												archive->escaped_path , " " , archive->extraction_path , NULL );
					else
						command = g_strconcat ( rar, " ", archive->full_path ? "x " : "e ",
												archive->freshen ? "-f " : "" , archive->update ? "-u " : "",
												archive->overwrite ? "-o+" : "-o-",
												" -idp ",
												archive->escaped_path , " " , archive->extraction_path , NULL );
					break;

					case XARCHIVETYPE_TAR:
					if (archive->full_path == 1)
					{
						command = g_strconcat (tar, " -xvf ", archive->escaped_path,
											archive->overwrite ? " --overwrite" : " --keep-old-files",
											archive->tar_touch ? " --touch" : "",
											" -C " , archive->extraction_path , NULL );
					}
					else
					{
						xa_extract_tar_without_directories ( "tar -xvf " , archive->escaped_path, archive->overwrite,archive->tar_touch,archive->extraction_path, FALSE );
						command = NULL;
					}
					break;

					case XARCHIVETYPE_TAR_BZ2:
					if (archive->full_path == 1)
					{
						command = g_strconcat (tar, " -xvjf " , archive->escaped_path,
											archive->overwrite ? " --overwrite" : " --keep-old-files",
											archive->tar_touch ? " --touch" : "",
											" -C " , archive->extraction_path , NULL );
					}
					else
					{
						xa_extract_tar_without_directories ( "tar -xvjf " , archive->escaped_path, archive->overwrite,archive->tar_touch,archive->extraction_path , FALSE );
						command = NULL;
					}
					break;

					case XARCHIVETYPE_TAR_GZ:
					if (archive->full_path == 1)
					{
						command = g_strconcat (tar, " -xvzf " , archive->escaped_path,
											archive->overwrite ? " --overwrite" : " --keep-old-files",
											archive->tar_touch ? " --touch" : "",
											" -C " , archive->extraction_path , NULL );
					}
					else
					{
						xa_extract_tar_without_directories ( "tar -xvzf " , archive->escaped_path, archive->overwrite,archive->tar_touch,archive->extraction_path, FALSE );
						command = NULL;
					}
					break;

					case XARCHIVETYPE_DEB:
					if (archive->full_path == 1)
					{
						command = g_strconcat (tar, " -xvzf " , archive->tmp,
											archive->overwrite ? " --overwrite" : " --keep-old-files",
											archive->tar_touch ? " --touch" : "",
											" -C " , archive->extraction_path , NULL );
					}
					else
					{
						xa_extract_tar_without_directories ( "tar -xvzf " , archive->tmp, archive->overwrite,archive->tar_touch,archive->extraction_path, FALSE );
						command = NULL;
					}
					break;

                    case XARCHIVETYPE_ZIP:
                    if ( archive->passwd != NULL )
						command = g_strconcat ( "unzip ", archive->freshen ? "-f " : "",
												archive->update ? "-u " : "" ,
												archive->overwrite ? "-o" : "-n",
												" -P " , archive->passwd,
												archive->full_path ? "" : " -j ",
												archive->escaped_path , " -d ", archive->extraction_path , NULL );
                    else
						command = g_strconcat ( "unzip ", archive->freshen ? "-f " : "",
												archive->update ? "-u " : "",
												archive->overwrite ? "-o " : "-n ",
												archive->full_path ? "" : " -j ",
												archive->escaped_path , " -d ", archive->extraction_path , NULL );
					break;

					case XARCHIVETYPE_RPM:
					if (archive->full_path == 1)
					{
						chdir ( archive->extraction_path );
						command = g_strconcat ( "cpio --make-directories -F " , archive->tmp , " -i" , NULL );
					}
					else
					{
						xa_extract_tar_without_directories ( "tar -xvzf " , archive->escaped_path, archive->overwrite,archive->tar_touch,archive->extraction_path , TRUE);
						command = NULL;
					}
                    break;

                    case XARCHIVETYPE_7ZIP:
                    if (archive->passwd != NULL)
						command = g_strconcat ( "7za " , archive->full_path ? "x " : "e ",
												archive->overwrite ? "-aoa" : "-aos",
												" -bd -p",archive->passwd," ",
												archive->escaped_path , " -o" , archive->extraction_path , NULL );
					else
						command = g_strconcat ( "7za " , archive->full_path ? "x " : "e ",
												archive->overwrite ? "-aoa" : "-aos",
												" -bd ",
												archive->escaped_path , " -o" , archive->extraction_path , NULL );
                    break;

					case XARCHIVETYPE_ARJ:
					if (archive->passwd != NULL)
						command = g_strconcat ( "arj " , archive->full_path ? "x " : "e ",
												"-g",archive->passwd,
												archive->overwrite ? "" : " -n" , " -i ",
												archive->freshen ? "-f " : "" , archive->update ? "-u " : "",
												"-y " , archive->escaped_path , " " , archive->extraction_path , NULL );
                    else
						command = g_strconcat ( "arj " , archive->full_path ? "x " : "e ",
												archive->overwrite ? "" : " -n" , " -i ",
												archive->freshen ? "-f " : "" , archive->update ? "-u " : "",
												"-y " , archive->escaped_path , " " , archive->extraction_path , NULL );
					break;

					case XARCHIVETYPE_LHA:
					command = g_strconcat ("lha ", archive->full_path ? "x" : "xi",
											archive->overwrite ? "f" : "", "w=",
											archive->extraction_path, " ", archive->escaped_path , NULL);
					break;

					case XARCHIVETYPE_ISO:
					end = gtk_tree_model_get_iter_first (archive->model , &iter);
					gtk_widget_show ( viewport2 );
					g_timeout_add (200, xa_progressbar_pulse, NULL );
					while (end)
					{
						xa_set_button_state (0,0,0,0,0,0,0);
						if (stop_flag)
							break;
						gtk_tree_model_get (archive->model, &iter,
						0, &name,
						1, &permissions,
						2, &file_size,
						4, &file_offset,
						-1);
						if (xa_extract_iso_file (archive, permissions, archive->extraction_path, name , file_size, file_offset ) == FALSE )
						{
							g_free (name);
							g_free (permissions);
							return NULL;
						}
						end = gtk_tree_model_iter_next (archive->model,&iter);
						g_free (name);
						g_free (permissions);
					}
					//xa_set_button_state (1,1,0,1,0,1);
					xa_hide_progress_bar_stop_button(archive);
					Update_StatusBar ( _("Operation completed.") );
					break;

					default:
					command = NULL;
				}
				g_free (tar);

				if ( command != NULL )
					return command;
			}
			else
			{
				names = g_string_new ( " " );
				gtk_tree_selection_selected_foreach (selection, (GtkTreeSelectionForeachFunc) ConcatenateFileNames, names );
				command = xa_extract_single_files ( archive , names, archive->extraction_path );
				g_string_free (names, TRUE);
			}
		}
	}
	return command;
}

gchar *xa_extract_single_files ( XArchive *archive , GString *files, gchar *path)
{
	gchar *command = NULL;
	gchar *tar;
	GtkTreeIter iter;

	if (unrar)
		rar = "unrar";
	else
		rar = "rar";

	gchar *msg = g_strdup_printf ( _("Extracting archive to %s") , path);
	Update_StatusBar (msg);
	g_free (msg);
	tar = g_find_program_in_path ("gtar");
	if (tar == NULL)
		tar = g_strdup ("tar");
	switch (archive->type)
	{
		case XARCHIVETYPE_BZIP2:
		gzip_bzip2_extract (archive , 0);
		break;

		case XARCHIVETYPE_GZIP:
		gzip_bzip2_extract (archive , 1);
		break;

		case XARCHIVETYPE_RAR:
		if (archive->passwd != NULL)
			command = g_strconcat ( rar," ", archive->full_path ? "x " : "e " ,
									archive->freshen ? "-f " : "" , archive->update ? "-u " : "",
									" -p",archive->passwd,
									archive->overwrite ? " -o+" : " -o-",
									" -idp ",
									archive->escaped_path , " " , files->str , " " , path , NULL );
        else
			command = g_strconcat ( rar," ", archive->full_path ? "x " : "e " ,
									archive->freshen ? "-f " : "" , archive->update ? "-u " : "",
									archive->overwrite ? "-o+" : "-o-",
									" -idp ",
									archive->escaped_path , " " , files->str , " ", path ,NULL);
		break;

		case XARCHIVETYPE_TAR:
		if (archive->full_path == 1)
		{
			command = g_strconcat (tar, " -xvf " , archive->escaped_path,
								archive->overwrite ? " --overwrite" : " --keep-old-files",
								archive->tar_touch ? " --touch" : "",
								" -C " , path , files->str , NULL );
		}
		else
		{
			xa_extract_tar_without_directories ( "tar -xvf " , archive->escaped_path, archive->overwrite,archive->tar_touch,path, FALSE );
			command = NULL;
		}
		break;

		case XARCHIVETYPE_TAR_BZ2:
		if (archive->full_path == 1)
		{
			command = g_strconcat (tar, " -xjvf " , archive->escaped_path,
								archive->overwrite ? " --overwrite" : " --keep-old-files",
								archive->tar_touch ? " --touch" : "",
								" -C " , path , files->str , NULL );
		}
		else
		{
			xa_extract_tar_without_directories ( "tar -xjvf " , archive->escaped_path, archive->overwrite,archive->tar_touch,path, FALSE );
			command = NULL;
		}
		break;

		case XARCHIVETYPE_TAR_GZ:
		if (archive->full_path == 1)
		{
			command = g_strconcat (tar, " -xzvf " , archive->escaped_path,
								archive->overwrite ? " --overwrite" : " --keep-old-files",
								archive->tar_touch ? " --touch" : "",
								" -C " , path , files->str , NULL );
		}
		else
		{
			xa_extract_tar_without_directories ( "tar -xzvf " , archive->escaped_path, archive->overwrite,archive->tar_touch,path, FALSE );
			command = NULL;
		}
		break;

		case XARCHIVETYPE_DEB:
		if (archive->full_path == 1)
		{
			command = g_strconcat (tar, " -xvzf " , archive->tmp,
					archive->overwrite ? " --overwrite" : " --keep-old-files",
					archive->tar_touch ? " --touch" : "",
					" -C " , archive->extraction_path , files->str, NULL );
		}
		else
		{
			xa_extract_tar_without_directories ( "tar -xvzf " , archive->tmp, archive->overwrite,archive->tar_touch,archive->extraction_path, FALSE );
			command = NULL;
		}
		break;

		case XARCHIVETYPE_ZIP:
        if ( archive->passwd != NULL )
			command = g_strconcat ( "unzip ", archive->freshen ? "-f " : "",
									archive->update ? "-u " : "",
									archive->overwrite ? "-o" : "-n",
									" -P " , archive->passwd,
									archive->full_path ? " " : " -j ",
									archive->escaped_path , files->str," -d " , path , NULL );
        else
			command = g_strconcat ( "unzip ", archive->freshen ? "-f " : "",
									archive->update ? "-u " : "",
									archive->overwrite ? "-o " : "-n ",
									archive->full_path ? "" : " -j ",
									archive->escaped_path , files->str," -d " , path , NULL );
		break;

        case XARCHIVETYPE_RPM:
        if (archive->full_path == 1)
		{
			chdir ( path );
			command = g_strconcat ( "cpio --make-directories " , files->str , " -F " , archive->tmp , " -i" , NULL);
		}
		else
		{
			xa_extract_tar_without_directories ( "tar -xvzf " , archive->escaped_path, archive->overwrite,archive->tar_touch,path , TRUE);
			command = NULL;
		}
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

		case XARCHIVETYPE_LHA:
			command = g_strconcat ("lha ", archive->full_path ? "x" : "xi",
											archive->overwrite ? "f" : "", "w=",
											path, " ", archive->escaped_path , files->str, NULL);
		break;

		case XARCHIVETYPE_ISO:
		{
			GList *row_list = NULL;
			GtkTreeSelection *selection;

			selection = gtk_tree_view_get_selection ( GTK_TREE_VIEW (archive->treeview) );
			row_list = gtk_tree_selection_get_selected_rows (selection, &archive->model);
			while (row_list)
			{
				xa_set_button_state (0,0,0,0,0,0,0);
				if (stop_flag)
					break;
				gtk_tree_model_get_iter(archive->model, &iter, row_list->data);
				gtk_tree_model_get (archive->model, &iter,
				0, &name,
				1, &permissions,
				2, &file_size,
				4, &file_offset,
				-1);
				gtk_tree_path_free (row_list->data);

				xa_extract_single_iso_file (archive, permissions, archive->extraction_path, name , file_size, file_offset );
				g_free (name);
				g_free (permissions);
				row_list = row_list->next;
			}
			g_list_free (row_list);
			//xa_set_button_state (1,1,0,1,0,1);
			xa_hide_progress_bar_stop_button(archive);
			Update_StatusBar ( _("Operation completed.") );
			command = NULL;
		}
		break;

		default:
		command = NULL;
    }
	g_free (tar);
	return command;
}

gboolean xa_extract_tar_without_directories ( gchar *string, gchar *escaped_path, gboolean overwrite, gboolean tar_touch, gchar *extract_path, gboolean cpio_flag )
{
	gchar *command = NULL;
	gchar *name = NULL;
	gchar *permission = NULL;
	gchar tmp_dir[14] = "";
	GtkTreeSelection *selection;
	GString *names, *unescaped_names;
	gboolean end = FALSE;
	GtkTreeIter iter;
	GList *row_list;
	GSList *filenames = NULL;
	gboolean result;
	gint current_page;

	current_page = gtk_notebook_get_current_page(notebook);
	names = g_string_new ("");
	unescaped_names = g_string_new ("");

	selection = gtk_tree_view_get_selection ( GTK_TREE_VIEW (archive[current_page]->treeview) );
	row_list = gtk_tree_selection_get_selected_rows (selection, &archive[current_page]->model);

	if (row_list != NULL)
	{
		while (row_list)
		{
			gtk_tree_model_get_iter(archive[current_page]->model, &iter, row_list->data);
			gtk_tree_model_get (archive[current_page]->model, &iter,
								0, &name,
								1, &permission,
								-1);
			gtk_tree_path_free (row_list->data);

			if (strstr (permission ,"d") == NULL)
			{
				ConcatenateFileNames2 ( name , names );
				filenames = g_slist_append ( filenames,name );
			}
			g_free (permission);
			row_list = row_list->next;
		}
		g_list_free (row_list);
	}
	else
	{
		end = gtk_tree_model_get_iter_first (archive[current_page]->model , &iter);
		while (end)
		{
			gtk_tree_model_get (archive[current_page]->model, &iter,	0, &name,
												1, &permission, -1);
			if (strstr (permission ,"d") == NULL)
			{
				ConcatenateFileNames2 ( name , names );
				filenames = g_slist_append ( filenames,name );
			}
			g_free (permission);
			end = gtk_tree_model_iter_next (archive[current_page]->model,&iter);
		}
	}
	result = xa_create_temp_directory (tmp_dir);
	if (result == 0)
		return FALSE;

	if (cpio_flag)
	{
		chdir (tmp_dir);
		command = g_strconcat ( "cpio --make-directories -F " , archive[current_page]->tmp , " -i" , NULL );
	}
	else
		command = g_strconcat ( string, escaped_path,
										overwrite ? " --overwrite" : " --keep-old-files",
										tar_touch ? " --touch" : "",
										" -C " , tmp_dir , names->str, NULL );
	result = xa_run_command (command , 0);
	g_string_free (names, TRUE);
	g_free (command);

	if (result == 0 || stop_flag)
	{
		xa_delete_temp_directory ( tmp_dir, 0 );
		gtk_widget_hide (viewport2);
		Update_StatusBar (_("Operation canceled."));
		return FALSE;
	}
	chdir (tmp_dir);
	while (filenames)
	{
		gchar *unescaped = EscapeBadChars ( filenames->data , "$\'`\"\\!?* ()[]&|@#:;");
		g_string_prepend ( unescaped_names, unescaped );
		g_string_prepend_c (unescaped_names, ' ');
		g_free (unescaped);
		filenames = filenames->next;
	}
	command = g_strconcat ( "mv -f ", unescaped_names->str, " " , extract_path , NULL );
	result = xa_run_command (command , 0);
	g_free (command);
	g_slist_free (filenames);
	g_string_free ( unescaped_names, TRUE );

	if (result == 0 || stop_flag)
	{
		xa_delete_temp_directory ( tmp_dir, 0 );
		gtk_widget_hide (viewport2);
		Update_StatusBar (_("Operation canceled."));
		return FALSE;
	}
	if (cpio_flag)
		xa_delete_temp_directory ( tmp_dir, 0 );
	else
		xa_delete_temp_directory ( tmp_dir, 1 );

	return result;
}

gboolean xa_delete_temp_directory ( gchar *dir_name, gboolean flag)
{
	gchar *command;
	gboolean result;

	command = g_strconcat ( "rm -rf ", dir_name , NULL );
	result = xa_run_command (command , flag );
	g_free (command);
	return result;
}

gboolean xa_create_temp_directory ( gchar tmp_dir[] )
{
	strcpy (tmp_dir,"/tmp/xa-XXXXXX");
	if ( mkdtemp ( tmp_dir ) == 0)
	{
		response = ShowGtkMessageDialog (GTK_WINDOW (MainWindow),GTK_DIALOG_MODAL,GTK_MESSAGE_ERROR,GTK_BUTTONS_OK,_("Can't create temporary directory in /tmp:"),g_strerror(errno) );
		gtk_widget_set_sensitive (Stop_button, FALSE);
		Update_StatusBar (_("Operation failed."));
		return FALSE;
	}
	return TRUE;
}

void xa_choose_extraction_directory (GtkWidget *widget, gpointer data)
{
	Extract_dialog_data *dialog_data = data;
	GtkWidget *File_Selector;
	int response;
	gchar *path;

	File_Selector = gtk_file_chooser_dialog_new ( _("Choose the destination folder where to extract the current archive"),
					GTK_WINDOW (MainWindow),
					GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER,
					GTK_STOCK_CANCEL,
					GTK_RESPONSE_CANCEL,
					GTK_STOCK_OPEN,
					GTK_RESPONSE_ACCEPT,
					NULL );
	response = gtk_dialog_run (GTK_DIALOG (File_Selector));
	if (response == GTK_RESPONSE_ACCEPT)
	{
		path = gtk_file_chooser_get_filename ( GTK_FILE_CHOOSER (File_Selector) );
		gtk_entry_set_text (GTK_ENTRY(dialog_data->destination_path_entry),path);
		g_free (path);
	}
	gtk_widget_destroy (File_Selector);
}
