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
#include "add_dialog.h"
#include "interface.h"
#include "window.h"
#include "support.h"

Add_dialog_data *xa_create_add_dialog (XArchive *archive)
{
	GtkWidget *label1,*label2,*label3,*label4,*label5,*label7,*hbox1,*hbox2,*hbox3,*hbox4,*option_notebook_vbox;
	GtkWidget *dialog_action_area1,*alignment1,*alignment2,*alignment3,*vbox3,*frame2,*frame3,*frame4,*alignment4;
	Add_dialog_data *add_dialog;
	unsigned short int default_value, max_value;
	default_value = max_value = 0;
	gchar *compression_msg = NULL;
	gboolean flag = FALSE;

	add_dialog = g_new0 (Add_dialog_data, 1);
	add_dialog->path_group = NULL;
	add_dialog->option_tooltip = gtk_tooltips_new ();

	add_dialog->dialog1 = gtk_dialog_new ();
	gtk_widget_set_size_request (add_dialog->dialog1, 520, 370);
	gtk_window_set_title (GTK_WINDOW (add_dialog->dialog1), _("Add files to the archive"));
	gtk_window_set_resizable(GTK_WINDOW (add_dialog->dialog1),FALSE);
	gtk_window_set_transient_for (GTK_WINDOW (add_dialog->dialog1),GTK_WINDOW(MainWindow));
	gtk_dialog_set_has_separator (GTK_DIALOG (add_dialog->dialog1),FALSE);

	add_dialog->add_option_tooltip = gtk_tooltips_new ();
	add_dialog->dialog_vbox1 = GTK_DIALOG (add_dialog->dialog1)->vbox;

	add_dialog->notebook1 = gtk_notebook_new ();
	gtk_box_pack_start (GTK_BOX (add_dialog->dialog_vbox1), add_dialog->notebook1, TRUE, TRUE, 0);
	gtk_container_set_border_width (GTK_CONTAINER (add_dialog->notebook1), 4);

	/* Selection page */
	vbox1 = gtk_vbox_new (FALSE, 0);
	gtk_container_add (GTK_CONTAINER (add_dialog->notebook1), vbox1);

	label1 = gtk_label_new (_("Selection"));
	gtk_notebook_set_tab_label (GTK_NOTEBOOK (add_dialog->notebook1), gtk_notebook_get_nth_page (GTK_NOTEBOOK (add_dialog->notebook1), 0), label1);
	
	add_dialog->filechooserwidget1 = gtk_file_chooser_widget_new (GTK_FILE_CHOOSER_ACTION_OPEN);
	gtk_file_chooser_set_select_multiple(GTK_FILE_CHOOSER(add_dialog->filechooserwidget1),TRUE);
	gtk_file_chooser_unselect_all(GTK_FILE_CHOOSER(add_dialog->filechooserwidget1));
	gtk_box_pack_start (GTK_BOX (vbox1), add_dialog->filechooserwidget1, TRUE, TRUE, 0);

	add_dialog->frame1 = gtk_frame_new (NULL);
	gtk_box_pack_start (GTK_BOX (vbox1), add_dialog->frame1, FALSE, TRUE, 5);
	gtk_container_set_border_width (GTK_CONTAINER (add_dialog->frame1), 5);

	alignment1 = gtk_alignment_new (0.5, 0.5, 1, 1);
	gtk_container_add (GTK_CONTAINER (add_dialog->frame1), alignment1);
	gtk_alignment_set_padding (GTK_ALIGNMENT (alignment1), 0, 0, 20, 20);

	label3 = gtk_label_new (_("File Paths: "));
	gtk_frame_set_label_widget (GTK_FRAME (add_dialog->frame1), label3);

	hbox1 = gtk_hbox_new (TRUE, 0);
	gtk_container_add (GTK_CONTAINER (alignment1), hbox1);

	add_dialog->radiobutton3 = gtk_radio_button_new_with_mnemonic (NULL, _("Store full paths"));
	gtk_box_pack_start (GTK_BOX (hbox1), add_dialog->radiobutton3, FALSE, FALSE, 0);
	gtk_radio_button_set_group (GTK_RADIO_BUTTON (add_dialog->radiobutton3), add_dialog->path_group);
	add_dialog->path_group = gtk_radio_button_get_group (GTK_RADIO_BUTTON (add_dialog->radiobutton3));
	gtk_button_set_focus_on_click (GTK_BUTTON (add_dialog->radiobutton3), FALSE);

	add_dialog->radiobutton4 = gtk_radio_button_new_with_mnemonic (NULL, _("Do not store paths"));
	gtk_box_pack_start (GTK_BOX (hbox1), add_dialog->radiobutton4, FALSE, FALSE, 0);
	gtk_radio_button_set_group (GTK_RADIO_BUTTON (add_dialog->radiobutton4), add_dialog->path_group);
	add_dialog->path_group = gtk_radio_button_get_group (GTK_RADIO_BUTTON (add_dialog->radiobutton4));
	gtk_button_set_focus_on_click (GTK_BUTTON (add_dialog->radiobutton4), FALSE);
	//gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(add_dialog->radiobutton4),TRUE);

	/* Options page */
	option_notebook_vbox = gtk_vbox_new (FALSE, 0);
	gtk_container_add (GTK_CONTAINER (add_dialog->notebook1), option_notebook_vbox);

	label2 = gtk_label_new (_("Options"));
	gtk_notebook_set_tab_label (GTK_NOTEBOOK (add_dialog->notebook1), gtk_notebook_get_nth_page (GTK_NOTEBOOK (add_dialog->notebook1), 1), label2);
	
	hbox2 = gtk_hbox_new (TRUE, 10);
	gtk_box_pack_start (GTK_BOX (option_notebook_vbox), hbox2, TRUE, TRUE, 0);
	gtk_container_set_border_width (GTK_CONTAINER (hbox2), 5);

	frame4 = gtk_frame_new (NULL);
	gtk_box_pack_start (GTK_BOX (hbox2), frame4, TRUE, TRUE, 0);
	gtk_frame_set_shadow_type (GTK_FRAME (frame4), GTK_SHADOW_OUT);

	alignment4 = gtk_alignment_new (0.5, 0.5, 1, 1);
	gtk_container_add (GTK_CONTAINER (frame4),alignment4);
	gtk_alignment_set_padding (GTK_ALIGNMENT(alignment4), 0, 0, 12, 0);

	vbox3 = gtk_vbox_new (TRUE, 0);
	gtk_container_add (GTK_CONTAINER (alignment4),vbox3);

	add_dialog->update = gtk_check_button_new_with_mnemonic (_("Update"));
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (add_dialog->update), archive->update);
	gtk_tooltips_set_tip (add_dialog->option_tooltip,add_dialog->update, _("This option will add any new files and update any files which have been modified since the archive was last created/modified."), NULL );

	if (archive->type != XARCHIVETYPE_7ZIP)
		g_signal_connect (G_OBJECT (add_dialog->update),"toggled",G_CALLBACK (add_update_fresh_toggled_cb) , add_dialog);
	gtk_box_pack_start (GTK_BOX (vbox3), add_dialog->update, FALSE, FALSE, 0);

	add_dialog->freshen = gtk_check_button_new_with_mnemonic (_("Freshen"));
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (add_dialog->freshen), archive->freshen);
	gtk_tooltips_set_tip (add_dialog->option_tooltip,add_dialog->freshen , _("This option affects the archive only if it has been modified more recently than the version already in the archive; unlike the update option it will not add files that are not already in the archive."), NULL );
	if (archive->type != XARCHIVETYPE_7ZIP && archive->type != XARCHIVETYPE_LHA)
		gtk_widget_set_sensitive(add_dialog->freshen,TRUE);
	gtk_box_pack_start (GTK_BOX (vbox3), add_dialog->freshen, FALSE, FALSE, 0);
	g_signal_connect (G_OBJECT (add_dialog->freshen),"toggled",G_CALLBACK (add_fresh_update_toggled_cb) , add_dialog);

	add_dialog->recurse = gtk_check_button_new_with_mnemonic (_("Include subdirectories"));
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (add_dialog->recurse), archive->add_recurse);
	if(archive->type == XARCHIVETYPE_LHA)
		gtk_widget_set_sensitive(add_dialog->recurse,FALSE);
	gtk_box_pack_start (GTK_BOX (vbox3), add_dialog->recurse, FALSE, FALSE, 0);

	add_dialog->solid_archive = gtk_check_button_new_with_mnemonic (_("Create a solid archive"));
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (add_dialog->solid_archive), archive->solid_archive);
	gtk_tooltips_set_tip (add_dialog->option_tooltip,add_dialog->solid_archive , _("In a solid archive the files are grouped together featuring a better compression ratio."), NULL);
	if (archive->type == XARCHIVETYPE_RAR || archive->type == XARCHIVETYPE_7ZIP)
		flag = TRUE;
	gtk_widget_set_sensitive(add_dialog->solid_archive,flag);
	gtk_box_pack_start (GTK_BOX (vbox3), add_dialog->solid_archive, FALSE, FALSE, 0);

	add_dialog->remove_files = gtk_check_button_new_with_mnemonic (_("Delete files after adding"));
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (add_dialog->remove_files), archive->remove_files);
	gtk_box_pack_start (GTK_BOX (vbox3),add_dialog->remove_files, FALSE, FALSE, 0);

	label7 = gtk_label_new (_("Actions; "));
	gtk_frame_set_label_widget (GTK_FRAME (frame4), label7);
	
	hbox3 = gtk_hbox_new (TRUE, 10);
	gtk_box_pack_start (GTK_BOX (option_notebook_vbox), hbox3, TRUE, TRUE, 0);
	gtk_container_set_border_width (GTK_CONTAINER (hbox3), 5);

	frame2 = gtk_frame_new (NULL);
	gtk_box_pack_start (GTK_BOX (hbox3), frame2, TRUE, TRUE, 0);
	gtk_frame_set_shadow_type (GTK_FRAME (frame2), GTK_SHADOW_OUT);

	alignment2 = gtk_alignment_new (0.5, 0.5, 1, 1);
	gtk_container_add (GTK_CONTAINER (frame2), alignment2);
	gtk_alignment_set_padding (GTK_ALIGNMENT (alignment2), 0, 0, 5, 5);
	
	if (archive->type != XARCHIVETYPE_TAR && archive->type != XARCHIVETYPE_TAR_GZ && archive->type != XARCHIVETYPE_TAR_LZMA && archive->type != XARCHIVETYPE_TAR_BZ2)
	{
		flag = TRUE;
		if (archive->type == XARCHIVETYPE_7ZIP)
		{
			compression_msg = _("0 = no compression, 5 is default, 9 = best compression but slowest");
			default_value = 5;
			max_value = 9;
		}

		else if (archive->type == XARCHIVETYPE_ZIP)
		{
			compression_msg = _("0 = no compression, 6 is default, 9 = best compression but slowest");
			default_value = 6;
			max_value = 9;
		}

		else if (archive->type == XARCHIVETYPE_RAR)
		{
			compression_msg = _("0 = no compression, 3 is default, 5 = best compression but slowest");
			default_value = 3;
			max_value = 5;
		}

		else if (archive->type == XARCHIVETYPE_ARJ)
		{
			compression_msg = _("0 = no compression, 1 is default, 4 = fastest but least compression");
			default_value = 1;
			max_value = 4;
		}

		else if (archive->type == XARCHIVETYPE_LHA)
		{
			compression_msg = _("5 = default compression, 7 = max compression");
			default_value = 5;
			max_value = 7;
		}
	}
	else
		flag = FALSE;

	if (archive->type == XARCHIVETYPE_7ZIP)
		add_dialog->compression_value = gtk_adjustment_new (default_value, 0, max_value, 2, 2, 0);
	else if (archive->type == XARCHIVETYPE_LHA)
		add_dialog->compression_value = gtk_adjustment_new (default_value, 5, max_value, 7, 7, 0);
	else
		add_dialog->compression_value = gtk_adjustment_new (default_value, 0, max_value, 0, 0, 0);

	add_dialog->compression_scale = gtk_hscale_new (GTK_ADJUSTMENT(add_dialog->compression_value));
	gtk_widget_set_sensitive(add_dialog->compression_scale,flag);
	gtk_container_add (GTK_CONTAINER (alignment2), add_dialog->compression_scale);
	gtk_scale_set_value_pos (GTK_SCALE (add_dialog->compression_scale), GTK_POS_TOP);
	gtk_scale_set_digits (GTK_SCALE (add_dialog->compression_scale), 0);
	if (archive->compression_level == 0)
		archive->compression_level = default_value;
	gtk_adjustment_set_value (GTK_ADJUSTMENT(add_dialog->compression_value), archive->compression_level);

	if (archive->type == XARCHIVETYPE_ARJ)
		gtk_range_set_inverted (GTK_RANGE (add_dialog->compression_scale), TRUE);
	else if (archive->type == XARCHIVETYPE_7ZIP)
		g_signal_connect (G_OBJECT (add_dialog->compression_value),"value-changed",G_CALLBACK (fix_adjustment_value), NULL);

	gtk_tooltips_set_tip (add_dialog->option_tooltip,add_dialog->compression_scale, compression_msg, NULL );

	label4 = gtk_label_new (_("Compression: "));
	gtk_frame_set_label_widget (GTK_FRAME (frame2), label4);

	frame3 = gtk_frame_new (NULL);
	gtk_box_pack_start (GTK_BOX (hbox3), frame3, TRUE, TRUE, 0);
	gtk_frame_set_shadow_type (GTK_FRAME (frame3), GTK_SHADOW_OUT);

	alignment3 = gtk_alignment_new (0.5, 0.5, 1, 1);
	gtk_container_add (GTK_CONTAINER (frame3), alignment3);
	gtk_alignment_set_padding (GTK_ALIGNMENT (alignment3), 0, 0, 5, 5);

	hbox4 = gtk_hbox_new (FALSE, 0);
	gtk_container_add (GTK_CONTAINER (alignment3), hbox4);

	if (archive->type == XARCHIVETYPE_TAR || archive->type == XARCHIVETYPE_TAR_GZ || archive->type == XARCHIVETYPE_TAR_LZMA || archive->type == XARCHIVETYPE_TAR_BZ2)
		flag = FALSE;
	else
		flag = TRUE;

	add_dialog->add_password = gtk_check_button_new_with_mnemonic (_("Password:"));
	gtk_box_pack_start (GTK_BOX (hbox4), add_dialog->add_password, FALSE, FALSE, 0);
	gtk_widget_set_sensitive(add_dialog->add_password,flag);
	g_signal_connect (G_OBJECT (add_dialog->add_password), "toggled",G_CALLBACK (password_toggled_cb) , add_dialog);

	add_dialog->add_password_entry = gtk_entry_new ();
	gtk_box_pack_start (GTK_BOX (hbox4), add_dialog->add_password_entry, FALSE, FALSE, 0);
	gtk_entry_set_visibility (GTK_ENTRY (add_dialog->add_password_entry), FALSE);
	gtk_entry_set_width_chars (GTK_ENTRY (add_dialog->add_password_entry), 15);
	gtk_widget_set_sensitive (add_dialog->add_password_entry, FALSE);

	label5 = gtk_label_new (_("Encryption: "));
	gtk_frame_set_label_widget (GTK_FRAME (frame3), label5);
  
	dialog_action_area1 = GTK_DIALOG (add_dialog->dialog1)->action_area;
	gtk_button_box_set_layout (GTK_BUTTON_BOX (dialog_action_area1), GTK_BUTTONBOX_END);

	add_dialog->cancel_button = gtk_button_new_from_stock ("gtk-cancel");
	gtk_dialog_add_action_widget (GTK_DIALOG (add_dialog->dialog1), add_dialog->cancel_button, GTK_RESPONSE_CANCEL);
	GTK_WIDGET_SET_FLAGS (add_dialog->cancel_button, GTK_CAN_DEFAULT);

	add_dialog->add_button = gtk_button_new();
	add_dialog->add_image = xa_main_window_find_image("xarchiver-add.png", GTK_ICON_SIZE_SMALL_TOOLBAR);
	add_dialog->add_hbox = gtk_hbox_new(FALSE, 4);
	add_dialog->add_label = gtk_label_new_with_mnemonic(_("_Add"));

	alignment2 = gtk_alignment_new (0.5, 0.5, 0, 0);
	gtk_container_add (GTK_CONTAINER (alignment2), add_dialog->add_hbox);

	gtk_box_pack_start(GTK_BOX(add_dialog->add_hbox), add_dialog->add_image, FALSE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(add_dialog->add_hbox), add_dialog->add_label, FALSE, TRUE, 0);
	gtk_container_add(GTK_CONTAINER(add_dialog->add_button), alignment2);

	gtk_dialog_add_action_widget (GTK_DIALOG (add_dialog->dialog1), add_dialog->add_button, GTK_RESPONSE_OK);
	GTK_WIDGET_SET_FLAGS (add_dialog->add_button, GTK_CAN_DEFAULT);
	gtk_dialog_set_default_response (GTK_DIALOG (add_dialog->dialog1), GTK_RESPONSE_OK);
	
	gtk_widget_show_all (add_dialog->dialog_vbox1);
	return add_dialog;
}

void add_fresh_update_toggled_cb (GtkToggleButton *button, Add_dialog_data *data)
{
	gboolean active = gtk_toggle_button_get_active (button);
	if (active)
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (data->update), FALSE);
}

void add_update_fresh_toggled_cb (GtkToggleButton *button, Add_dialog_data *data)
{
	if (data->freshen == NULL)
		return;
	gboolean active = gtk_toggle_button_get_active (button);
	if (active)
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (data->freshen), FALSE);
}

void password_toggled_cb ( GtkButton* button , gpointer _add_dialog )
{
	Add_dialog_data *add_dialog = _add_dialog;
	if ( gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(add_dialog->add_password)) )
	{
		gtk_widget_set_sensitive (add_dialog->add_password_entry, TRUE);
		gtk_widget_grab_focus (add_dialog->add_password_entry);
	}
	else
		gtk_widget_set_sensitive (add_dialog->add_password_entry, FALSE);
}

void fix_adjustment_value (GtkAdjustment *adjustment, gpointer user_data)
{
	unsigned short int digit = gtk_adjustment_get_value (adjustment);
	if (digit & 1)
		return;
	else
		gtk_adjustment_set_value (adjustment, digit-1);
}

gchar *xa_parse_add_dialog_options ( XArchive *archive , Add_dialog_data *add_dialog )
{
	gchar *command = NULL;
	gchar *temp_password = NULL;
	gchar *compression_string = NULL;
	gchar *first_item = NULL;
	gboolean done = FALSE;
	GSList *files = NULL;
	GString *names;

	while ( ! done )
	{
		switch (gtk_dialog_run ( GTK_DIALOG (add_dialog->dialog1 ) ) )
		{
			case GTK_RESPONSE_CANCEL:
			case GTK_RESPONSE_DELETE_EVENT:
			done = TRUE;
			break;

			case GTK_RESPONSE_OK:
			files = gtk_file_chooser_get_filenames(GTK_FILE_CHOOSER(add_dialog->filechooserwidget1));
			if (g_slist_length(files) == 0)
			{
				response = xa_show_message_dialog (GTK_WINDOW (MainWindow),GTK_DIALOG_MODAL,GTK_MESSAGE_ERROR,GTK_BUTTONS_OK,_("Can't add files to the archive:"), _("You haven't selected any files to add!") );
				break;
			}
			if ( add_dialog->add_password != NULL && gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(add_dialog->add_password)) )
			{
				temp_password  = g_strdup (gtk_entry_get_text ( GTK_ENTRY (add_dialog->add_password_entry) ));
				if (strlen(temp_password) == 0)
				{
					response = xa_show_message_dialog (GTK_WINDOW (MainWindow),GTK_DIALOG_MODAL,GTK_MESSAGE_ERROR,GTK_BUTTONS_OK, _("You missed the password!"),_("Please enter it!") );
					g_free (temp_password);
					break;
				}
				else
					archive->passwd = temp_password;
			}
			done = TRUE;
			if (add_dialog->recurse != NULL)
				archive->add_recurse = gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON (add_dialog->recurse));
/* TODO: look at the radio button in the select page
			if (add_dialog->add_full_path != NULL)
				archive->full_path = gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON (add_dialog->add_full_path));
*/
			if (add_dialog->update != NULL)
				archive->update = gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON (add_dialog->update));

			if (add_dialog->remove_files != NULL)
				archive->remove_files = gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON (add_dialog->remove_files));

			if (add_dialog->freshen != NULL)
				archive->freshen = gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON (add_dialog->freshen));

			if (add_dialog->solid_archive)
				archive->solid_archive = gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON (add_dialog->solid_archive));

			if (add_dialog->compression_scale != NULL)
			{
				archive->compression_level = gtk_adjustment_get_value (GTK_ADJUSTMENT (add_dialog->compression_value));
				compression_string = g_strdup_printf ("%d",archive->compression_level);
			}

			//Set the current dir so to avoid archiving the leading directory inside the archive
			//gtk_tree_model_get_iter_first(GTK_TREE_MODEL(add_dialog->file_liststore),&iter);
			//gtk_tree_model_get (GTK_TREE_MODEL(add_dialog->file_liststore),&iter,1,&first_item,-1);
			gchar *current_dir = g_path_get_dirname (first_item);
			g_free (first_item);
			chdir (current_dir);
			g_free (current_dir);

			 /* Let's concatenate the files to add */
			names = g_string_new ( " " );
			archive->status = XA_ARCHIVESTATUS_ADD;

			/*if (archive->full_path == 1 && (archive->type == XARCHIVETYPE_TAR || archive->type == XARCHIVETYPE_TAR_BZ2 || archive->type == XARCHIVETYPE_TAR_GZ || archive->type == XARCHIVETYPE_TAR_LZMA) )
			{
				while (gtk_tree_model_get_iter_first(GTK_TREE_MODEL(add_dialog->file_liststore), &iter) )
				{
					xa_cat_filenames_basename ( GTK_TREE_MODEL(add_dialog->file_liststore), NULL, &iter, names );
					gtk_list_store_remove (add_dialog->file_liststore, &iter);
				}
			}

			else
			{
				while (gtk_tree_model_get_iter_first(GTK_TREE_MODEL(add_dialog->file_liststore), &iter) )
				{
					xa_cat_filenames ( GTK_TREE_MODEL(add_dialog->file_liststore), NULL, &iter, names );
					gtk_list_store_remove (add_dialog->file_liststore, &iter);
				}
			}*/
			gtk_widget_set_sensitive (Stop_button,TRUE);
			gtk_widget_set_sensitive (check_menu,FALSE);
			gtk_widget_set_sensitive (close1,FALSE);
			gtk_widget_set_sensitive (properties,FALSE);
			xa_set_button_state (0,0,0,0,0,0,0,0);
			gtk_widget_hide (add_dialog->dialog1);

			command = xa_add_single_files (archive,names,compression_string);
			g_string_free ( names, TRUE);
			g_slist_free(files);
			if (compression_string != NULL)
				g_free (compression_string);
		}
	}
	return command;
}

gchar *xa_add_single_files ( XArchive *archive , GString *names, gchar *compression_string)
{
	gchar *command = NULL;
	gchar *tar;

	tar = g_find_program_in_path ("gtar");
	if (tar == NULL)
		tar = g_strdup ("tar");

	Update_StatusBar ( _("Adding files to the archive, please wait..."));

	switch (archive->type)
	{
		case XARCHIVETYPE_RAR:
		if (compression_string == NULL)
			compression_string = "3";
		if (archive->passwd != NULL)
			command = g_strconcat ( "rar a ",
									archive->update ? "-u " : "",
									archive->freshen ? "-f " : "",
									archive->solid_archive ? "-s " : "",
									archive->remove_files ? "-df " : "",
									"-p" , archive->passwd,
									archive->add_recurse ? " -r " : "",
									archive->full_path ? "-ep " : "",
									"-idp ",
									"-m",compression_string," ",
									archive->escaped_path,
									names->str , NULL );
		else
			command = g_strconcat ( "rar a ",
									archive->update ? "-u " : "",
									archive->freshen ? "-f " : "",
									archive->solid_archive ? "-s " : "",
									archive->remove_files ? "-df " : "",
									archive->add_recurse ? " -r " : "",
									archive->full_path ? "-ep " : "",
									"-idp ",
									"-m",compression_string," ",
									archive->escaped_path,
									names->str , NULL );
		break;

		case XARCHIVETYPE_TAR:
		if ( g_file_test ( archive->escaped_path , G_FILE_TEST_EXISTS ) )
			command = g_strconcat (tar, " ",
									archive->add_recurse ? "" : "--no-recursion ",
									archive->remove_files ? "--remove-files " : "",
									archive->update ? "-uvvf " : "-rvvf ",
									archive->escaped_path,
									names->str , NULL );
		else
			command = g_strconcat (tar, " ",
									archive->add_recurse ? "" : "--no-recursion ",
									archive->remove_files ? "--remove-files " : "",
									"-cvvf ",archive->escaped_path,
									names->str , NULL );
		break;

		case XARCHIVETYPE_TAR_BZ2:
		if ( g_file_test ( archive->escaped_path , G_FILE_TEST_EXISTS ) )
			xa_add_delete_tar_bzip2_gzip ( names , archive, 0 , 1 );
		else
			command = g_strconcat (tar, " ",
									archive->add_recurse ? "" : "--no-recursion ",
									archive->remove_files ? "--remove-files " : "",
									"-cvvjf ",archive->escaped_path,
									names->str , NULL );
		break;

		case XARCHIVETYPE_TAR_GZ:
		if ( g_file_test ( archive->escaped_path , G_FILE_TEST_EXISTS ) )
		{
			g_print ("Son qui\n");
			xa_add_delete_tar_bzip2_gzip ( names , archive, 1 , 1 );
		}
		/* This is executed when the archive is newly created */
		else
			command = g_strconcat (tar, " ",
									archive->add_recurse ? "" : "--no-recursion ",
									archive->remove_files ? "--remove-files " : "",
									"-cvvzf ",archive->escaped_path,
									names->str , NULL );
		break;
		
		case XARCHIVETYPE_TAR_LZMA:
		if ( g_file_test ( archive->escaped_path , G_FILE_TEST_EXISTS ) )
			xa_add_delete_tar_lzma ( names , archive, 1 );
		else
			command = g_strconcat (tar, " ",
									archive->add_recurse ? "" : "--no-recursion ",
									archive->remove_files ? "--remove-files " : "",
									"--use-compress-program=lzma -cvvf ",archive->escaped_path,
									names->str , NULL );
		break;

		case XARCHIVETYPE_ZIP:
		if (compression_string == NULL)
			compression_string = "6";
		if (archive->passwd != NULL)
			command = g_strconcat ( "zip ",
									archive->update ? "-u " : "",
									archive->freshen ? "-f " : "",
									archive->add_recurse ? "-r " : "",
									archive->remove_files ? "-m " : "",
									archive->full_path ? "-j " : "",
									"-P ", archive->passwd," ",
									"-",compression_string," ",
									archive->escaped_path,
									names->str , NULL );
		else
			command = g_strconcat ( "zip ",
									archive->update ? "-u " : "",
									archive->freshen ? "-f " : "",
									archive->add_recurse ? "-r " : "",
									archive->remove_files ? "-m " : "",
									archive->full_path ? "-j " : "",
									"-",compression_string," ",
									archive->escaped_path,
									names->str , NULL );
		break;

		case XARCHIVETYPE_7ZIP:
		if (compression_string == NULL)
			compression_string = "5";
		if (archive->passwd != NULL)
			command = g_strconcat ( "7za ",
									archive->update ? "u " : "a ",
									archive->solid_archive ? "-ms=on " : "-ms=off ",
									"-p" , archive->passwd, " ",
									archive->escaped_path,
									archive->add_recurse ? " -r " : " ",
									"-mx=",compression_string,"",
									names->str , NULL );
		else
			command = g_strconcat ( "7za ",
									archive->update ? "u " : "a ",
									archive->solid_archive ? "-ms=on " : "-ms=off ",
									archive->escaped_path,
									archive->add_recurse ? " -r " : " ",
									"-mx=",compression_string,"",
									names->str , NULL );
		break;

		case XARCHIVETYPE_ARJ:
		if (compression_string == NULL)
			compression_string = "1";
		if (archive->passwd != NULL)
			command = g_strconcat ( "arj a ",
									archive->update ? "-u " : "",
									archive->freshen ? "-f " : "",
									archive->add_recurse ? "-r " : "",
									archive->remove_files ? "-d1 " : "",
									archive->full_path ? "-e " : "",
									"-g" , archive->passwd , " -i ",
									"-m",compression_string," ",
									archive->escaped_path,
									names->str , NULL );
		else
			command = g_strconcat ( "arj a ",
									archive->update ? "-u " : "",
									archive->freshen ? "-f " : "",
									archive->add_recurse ? "-r " : "",
									archive->remove_files ? "-d1 " : "",
									archive->full_path ? "-e " : "",
									" -i ",
									"-m",compression_string," ",
									archive->escaped_path,
									names->str , NULL );
		break;

		case XARCHIVETYPE_LHA:
		if (compression_string == NULL)
			compression_string = "5";
		command = g_strconcat( "lha ",
								archive->remove_files ? "m" : "a",
								archive->update ? "u" : "",
								"o",compression_string,
								" ",
								archive->escaped_path,
								names->str, NULL);
		break;

		default:
		command = NULL;
	}
	g_free (tar);
	return command;
}

