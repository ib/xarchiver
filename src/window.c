/*
 *  Copyright (C) 2008 Giuseppe Torelli - <colossus73@gmail.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License,or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not,write to the Free Software
 *  Foundation,Inc.,59 Temple Street #330,Boston,MA 02111-1307,USA.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "window.h"

#define	XDS_FILENAME "xds.txt"

extern GList *ArchiveType;
extern GList *ArchiveSuffix;
extern gboolean unrar;
extern gboolean xdg_open;
extern Prefs_dialog_data	*prefs_window;
extern Extract_dialog_data	*extract_window;
extern Add_dialog_data		*add_window;
extern Multi_extract_data	*multi_extract_window;

extern gchar *config_file;
extern void xa_free_icon_cache();

gchar *current_open_directory = NULL;
GtkFileFilter *open_file_filter = NULL;
GList *Suffix,*Name;

void xa_watch_child (GPid pid,gint status,XArchive *archive)
{
	int response;

	archive->child_pid = archive->pb_source = 0;
	if (WIFEXITED (status))
	{
		if (WEXITSTATUS (status))
		{
			if (xa_main_window == NULL)
				goto error;
			if ((WEXITSTATUS (status) == 1 && archive->type == XARCHIVETYPE_ZIP) || 
				(WEXITSTATUS (status) == 6 && archive->type == XARCHIVETYPE_ARJ) ||
				(WEXITSTATUS (status) == 1 && is_tar_compressed(archive->type)))
				goto there;
			if ( ! gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(prefs_window->store_output)))
			{
				response = xa_show_message_dialog(GTK_WINDOW(xa_main_window),GTK_DIALOG_MODAL,GTK_MESSAGE_ERROR,GTK_BUTTONS_OK,_("An error occurred!"),_("Please check the 'Store archiver output' option to see it."));	
				return;
			}
			if (xa_main_window)
			{
				archive->status = XA_ARCHIVESTATUS_ERROR;
				gtk_widget_set_sensitive(Stop_button,FALSE);
				xa_set_button_state (1,1,1,1,archive->can_add,archive->can_extract,0,archive->has_test,archive->has_properties,archive->has_passwd,1);
		error:
				archive->create_image = TRUE;
				xa_show_cmd_line_output (NULL,archive);
				/* In case the user supplies a wrong password we reset it so he can try again */
				if ( (archive->status == XA_ARCHIVESTATUS_TEST || archive->status == XA_ARCHIVESTATUS_SFX) && archive->passwd != NULL)
				{
					g_free (archive->passwd);
					archive->passwd = NULL;
				}
				return;
			}
		}
	}
there:
	if (xa_main_window)
	{
		xa_set_button_state (1,1,1,1,archive->can_add,archive->can_extract,0,archive->has_test,archive->has_properties,archive->has_passwd,1);
		archive->child_pid = archive->pb_source = 0;
		gtk_widget_set_sensitive(Stop_button,FALSE);
		gtk_label_set_text(GTK_LABEL(total_label),"");
	}
}

void xa_reload_archive_content(XArchive *_archive)
{
	XEntry *entry;
	gint current_page,idx = 0;

	//TODO: have the status bar notyfing the reload

	current_page = gtk_notebook_get_current_page(notebook);
	idx = xa_find_archive_index (current_page);

	if (xa_main_window == NULL)
		return;

	g_slist_free (_archive->forward);
	_archive->forward = NULL;

	g_slist_free (_archive->back);
	_archive->back = NULL;

	xa_free_entry (_archive,_archive->root_entry);
	if (_archive->column_types != NULL)
		g_free(_archive->column_types);
	xa_remove_columns(_archive);

	entry = g_new0(XEntry,1);
	entry->filename = "";
	_archive->root_entry = entry;

	(*_archive->open_archive) (_archive);

	if (strcmp(_archive->path,archive[idx]->path) == 0)
		xa_fill_dir_sidebar(_archive,TRUE);
}

void xa_show_cmd_line_output(GtkMenuItem *menuitem,XArchive *_archive)
{
	GSList *output = NULL;
	gchar *title = NULL;
	gchar *line = NULL;
	gchar *utf8_line;
	gsize bytes_written;
	GtkWidget *dialog,*label,*image,*hbox,*vbox,*textview,*scrolledwindow;
	GtkTextBuffer *textbuffer;
	GtkTextIter iter;
	gint current_page,idx = -1;
	
	if (_archive == NULL)
	{
		current_page = gtk_notebook_get_current_page(notebook);
		idx = xa_find_archive_index (current_page);
		if (idx < 0)
			return;
		_archive = archive[idx];
	}

	if (xa_main_window)
		title = _("Archiver output");
	else
		title = ("Xarchiver " VERSION);
	dialog = gtk_dialog_new_with_buttons (title,
					      GTK_WINDOW(xa_main_window),GTK_DIALOG_MODAL,GTK_STOCK_OK,GTK_RESPONSE_OK,NULL);
	gtk_dialog_set_default_response (GTK_DIALOG (dialog),GTK_RESPONSE_OK);

	gtk_dialog_set_has_separator (GTK_DIALOG (dialog),FALSE);
	gtk_container_set_border_width (GTK_CONTAINER (dialog),6);
	gtk_container_set_border_width (GTK_CONTAINER (GTK_DIALOG (dialog)->vbox),6);
	gtk_box_set_spacing (GTK_BOX (GTK_DIALOG (dialog)->vbox),8);
	gtk_widget_set_size_request (dialog,400,-1);

	scrolledwindow = gtk_scrolled_window_new (NULL,NULL);
	g_object_set (G_OBJECT (scrolledwindow),"hscrollbar-policy",GTK_POLICY_AUTOMATIC,"shadow-type",GTK_SHADOW_IN,"vscrollbar-policy",GTK_POLICY_AUTOMATIC,NULL);
	gtk_widget_set_size_request (scrolledwindow,-1,200);

	textbuffer = gtk_text_buffer_new (NULL);
	gtk_text_buffer_create_tag (textbuffer,"font","family","monospace",NULL);
	gtk_text_buffer_get_iter_at_offset (textbuffer,&iter,0);

	textview = gtk_text_view_new_with_buffer (textbuffer);
	g_object_unref (textbuffer);
	gtk_text_view_set_editable (GTK_TEXT_VIEW (textview),FALSE);
	gtk_text_view_set_cursor_visible (GTK_TEXT_VIEW (textview),FALSE);

	vbox = gtk_vbox_new (FALSE,6);
	gtk_container_set_border_width (GTK_CONTAINER (vbox),5);
	
	if (_archive->create_image)
	{
		_archive->create_image = FALSE;
		image = gtk_image_new_from_stock (GTK_STOCK_DIALOG_ERROR,GTK_ICON_SIZE_DIALOG);
		gtk_misc_set_alignment (GTK_MISC (image),0.5,0.0);

		label = gtk_label_new (_("An error occurred while accessing the archive:"));
		hbox = gtk_hbox_new (FALSE,6);
		gtk_box_pack_start (GTK_BOX (hbox),image,FALSE,FALSE,0);
		gtk_box_pack_start (GTK_BOX (hbox),label,TRUE,TRUE,0);
		gtk_box_pack_start (GTK_BOX (vbox),hbox,FALSE,FALSE,0);
	}
	gtk_container_add (GTK_CONTAINER (scrolledwindow),textview);
	gtk_box_pack_start (GTK_BOX (vbox),scrolledwindow,TRUE,TRUE,0);
	gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->vbox),vbox,TRUE,TRUE,0);

	output = _archive->error_output;
	while (output)
	{
		line = output->data;
		utf8_line = g_locale_to_utf8 (line,-1,NULL,&bytes_written,NULL);
		gtk_text_buffer_insert_with_tags_by_name (textbuffer,&iter,utf8_line,bytes_written,"font",NULL);
		g_free (utf8_line);
		output = output->next;
	}
	gtk_widget_show_all (vbox);
	gtk_dialog_run (GTK_DIALOG (dialog));
	gtk_widget_destroy (GTK_WIDGET (dialog));
}

void xa_new_archive (GtkMenuItem *menuitem,gpointer user_data)
{
	gint current_page;

	current_page = xa_get_new_archive_idx();
	if (current_page == -1)
		return;

	archive[current_page] = xa_new_archive_dialog (NULL,archive,FALSE);

	if (archive[current_page] == NULL)
		return;

	xa_add_page (archive[current_page]);
	xa_set_button_state (0,0,0,1,1,0,0,0,0,1,0);
    xa_disable_delete_buttons(FALSE);

    archive[current_page]->passwd = NULL;
    archive[current_page]->dummy_size = 0;
    archive[current_page]->nr_of_files = 0;
	xa_set_window_title (xa_main_window ,archive[current_page]->path);
	gtk_label_set_text(GTK_LABEL(total_label),"");
}

int xa_show_message_dialog (GtkWindow *window,int mode,int type,int button,const gchar *message1,const gchar *message2)
{
	int response;

	dialog = gtk_message_dialog_new (window,mode,type,button,message1);
	gtk_dialog_set_default_response (GTK_DIALOG (dialog),GTK_RESPONSE_NO);
	gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dialog),message2);
	response = gtk_dialog_run (GTK_DIALOG (dialog));
	gtk_widget_destroy (GTK_WIDGET (dialog));
	return response;
}

void xa_save_archive (GtkMenuItem *menuitem,gpointer data)
{
	gint current_page;
	gint idx;
	GtkWidget *save = NULL;
	gchar *path = NULL,*command,*filename;
	int response;
	GSList *list = NULL;

	current_page = gtk_notebook_get_current_page(notebook);
	idx = xa_find_archive_index (current_page);

	save = gtk_file_chooser_dialog_new (_("Save the archive as"),
						GTK_WINDOW (xa_main_window),
						GTK_FILE_CHOOSER_ACTION_SAVE,
						GTK_STOCK_CANCEL,
						GTK_RESPONSE_CANCEL,
						"gtk-save",
						GTK_RESPONSE_ACCEPT,
						NULL);
	filename = xa_remove_path_from_archive_name(archive[idx]->escaped_path);
	gtk_file_chooser_set_current_name (GTK_FILE_CHOOSER (save),filename);
	gtk_file_chooser_set_do_overwrite_confirmation(GTK_FILE_CHOOSER (save),TRUE);
	g_free(filename);
	response = gtk_dialog_run (GTK_DIALOG(save));
	if (response == GTK_RESPONSE_ACCEPT)
		path = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER(save));
	gtk_widget_destroy (save);
	if (path != NULL)
	{
		command = g_strconcat ("cp ",archive[idx]->escaped_path," ",path,NULL);
		g_free(path);
		list = g_slist_append(list,command);
		xa_run_command(archive[idx],list);
	}
}

void xa_open_archive (GtkMenuItem *menuitem,gpointer data)
{
	gchar *path = NULL;
	gchar *utf8_path,*msg;
	gint current_page;
	gint x,response;
	XArchiveType type;

	path = (gchar *)data;
	if (path == NULL)
    {
		path = xa_open_file_dialog ();
		if (path == NULL)
			return;
	}

	/* Let's check if the archive is already opened */
	for (x = 0; x < gtk_notebook_get_n_pages (notebook); x++)
	{
		current_page = xa_find_archive_index (x);
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

	if (type == 0 || type == 1)
	{
		utf8_path = g_filename_to_utf8 (path,-1,NULL,NULL,NULL);
		msg = g_strdup_printf (_("Can't open file \"%s\":"),utf8_path);
		if (type == 0)
			response = xa_show_message_dialog (GTK_WINDOW (xa_main_window),GTK_DIALOG_MODAL,GTK_MESSAGE_ERROR,GTK_BUTTONS_OK,msg,_("Archive format is not recognized!"));
		else
			response = xa_show_message_dialog (GTK_WINDOW (xa_main_window),GTK_DIALOG_MODAL,GTK_MESSAGE_ERROR,GTK_BUTTONS_OK,msg,g_strerror(errno));
	
		g_free (utf8_path);
		g_free (msg);
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
			response = xa_show_message_dialog (GTK_WINDOW (xa_main_window),GTK_DIALOG_MODAL,GTK_MESSAGE_ERROR,GTK_BUTTONS_OK,_("Sorry,this archive format is not supported:"),_("the proper archiver is not installed!"));
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
		response = xa_show_message_dialog (GTK_WINDOW (xa_main_window),GTK_DIALOG_MODAL,GTK_MESSAGE_ERROR,GTK_BUTTONS_OK,_("Can't allocate memory for the archive structure:"),_("Operation aborted!"));
		g_free (path);
		return;
	}
	/* Detect archive comment,rar one is detected in rar.c */
	if (type == XARCHIVETYPE_ZIP)
		archive[current_page]->has_comment = xa_detect_archive_comment (XARCHIVETYPE_ZIP,path,archive[current_page]);
	else if (type == XARCHIVETYPE_ARJ)
		archive[current_page]->has_comment = xa_detect_archive_comment (XARCHIVETYPE_ARJ,path,archive[current_page]);

	if (g_path_is_absolute(path) == FALSE)
		archive[current_page]->path = g_strconcat(g_get_current_dir(),"/",path,NULL);
	else
		archive[current_page]->path = g_strdup(path);

	archive[current_page]->escaped_path = xa_escape_bad_chars (archive[current_page]->path,"$\'`\"\\!?* ()&|@#:;");
	archive[current_page]->status = XA_ARCHIVESTATUS_OPEN;
	xa_add_page (archive[current_page]);

	xa_disable_delete_buttons (FALSE);
	g_free (path);

	gtk_widget_set_sensitive (Stop_button,TRUE);
	gtk_widget_set_sensitive (listing,FALSE);
	xa_set_button_state (0,0,0,0,0,0,0,0,0,0,0);
	gtk_label_set_text(GTK_LABEL(total_label),_("Opening archive,please wait..."));
	(*archive[current_page]->open_archive) (archive[current_page]);

	archive[current_page]->passwd = NULL;
	xa_fill_dir_sidebar(archive[current_page],TRUE);
}

void xa_test_archive (GtkMenuItem *menuitem,gpointer user_data)
{
	gint current_page;
	gint id;

	current_page = gtk_notebook_get_current_page (notebook);
	id = xa_find_archive_index (current_page);

	if ( archive[id]->has_passwd)
	{
		if ( archive[id]->passwd == NULL)
		{
			archive[id]->passwd = xa_create_password_dialog (archive[id]);
			if ( archive[id]->passwd == NULL)
				return;
		}
	}
	gtk_label_set_text(GTK_LABEL(total_label),_("Testing archive,please wait..."));
	(*archive[id]->test) (archive[id]);
}

void xa_list_archive (GtkMenuItem *menuitem,gpointer data)
{
	unsigned short int bp = GPOINTER_TO_UINT(data);
	gint current_page;
	gint idx;
	FILE *stream;
	GtkWidget *save = NULL;
	gchar *t,*_filename,*filename,*filename_plus = NULL, *pref_path;
	int response;
	struct stat my_stat;
	unsigned long long int file_size;

	current_page = gtk_notebook_get_current_page (notebook);
	idx = xa_find_archive_index (current_page);

	if (bp)
		_filename = _("Print the archive content as HTML");
	else
		_filename = _("Print the archive content as text");

	save = gtk_file_chooser_dialog_new (_filename,
						GTK_WINDOW (xa_main_window),
						GTK_FILE_CHOOSER_ACTION_SAVE,
						GTK_STOCK_CANCEL,
						GTK_RESPONSE_CANCEL,
						"gtk-save",
						GTK_RESPONSE_ACCEPT,
						NULL);

	filename = xa_remove_path_from_archive_name(archive[idx]->escaped_path);
	_filename = strstr(filename,".");
	if (_filename)
		_filename = g_strndup(filename,(_filename-filename));
	else
		_filename = filename;

	filename_plus = g_strconcat (_filename,bp ? ".html" : ".txt",NULL);
	gtk_file_chooser_set_current_name (GTK_FILE_CHOOSER (save),filename_plus);
	gtk_file_chooser_set_do_overwrite_confirmation(GTK_FILE_CHOOSER (save),TRUE);
	g_free(filename);
	g_free(filename_plus);
	filename = NULL;

	pref_path = gtk_combo_box_get_active_text (GTK_COMBO_BOX(prefs_window->combo_prefered_extract_dir));
	if (current_open_directory != NULL || pref_path != NULL)
		gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER (save), pref_path ? pref_path : current_open_directory);
	g_free (pref_path);
	response = gtk_dialog_run (GTK_DIALOG(save));

	if (current_open_directory != NULL)
		g_free (current_open_directory);

	current_open_directory = gtk_file_chooser_get_current_folder(GTK_FILE_CHOOSER(save));
	
	if (response == GTK_RESPONSE_ACCEPT)
		filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER(save));
	gtk_widget_destroy (save);

	if (filename != NULL)
	{
		stream = fopen (filename,"w");
		g_free(filename);
		if (bp)
		{
			g_fprintf (stream,"<html><head><meta name=GENERATOR content=\"Xarchiver " VERSION "\"><title>%s</title>\n",archive[idx]->escaped_path);
			g_fprintf (stream,"<style>\ntd     { font: normal .7em ; }\nth     { font: bold 0.7em ; color: #FFFFFF; text-align: left; background: #42578A}\n.row1  { background-color: #DDDDDD; }\n.row2  { background-color: #EEEEEE; }\n</style>\n");
			g_fprintf (stream,"<body bgcolor=#FFFFFF>\n");
			g_fprintf (stream,"<b><u>");
		}
		g_fprintf (stream,_("Archive contents:\n"));

		if (bp)
			g_fprintf (stream,"</b></u><br><br><b>");
		g_fprintf (stream,_("\nName: "));
		if (bp)
			g_fprintf (stream,"</b><a href=\"file://%s\">",archive[idx]->escaped_path);
		g_fprintf (stream,"%s\n",archive[idx]->escaped_path);
		if (bp)
			g_fprintf(stream,"</a><br><br><b>");
		stat (archive[idx]->path ,&my_stat);
    	file_size = my_stat.st_size;
    	t = xa_set_size_string(file_size);
		g_fprintf (stream,_("Compressed   size: "));
    	if (bp)
    		g_fprintf (stream,"</b>");
    	g_fprintf (stream,"%s\n",t);
    	g_free(t);
    	if (bp)
			g_fprintf(stream,"<br><br><b>");
    	g_fprintf (stream,_("Uncompressed size: "));
    	t = xa_set_size_string(archive[idx]->dummy_size);
    	if (bp)
    		g_fprintf (stream,"</b>");
    	g_fprintf (stream,"%s\n",t);
    	g_free(t);
    	if (bp)
			g_fprintf(stream,"<br><br><b>");
    	g_fprintf (stream,_("Number of files: "));
    	if (bp)
			g_fprintf(stream,"</b>");
    	 g_fprintf (stream,"%d\n",archive[idx]->nr_of_files);
		if (bp)
			g_fprintf(stream,"<br><br><b>");
		if (archive[idx]->has_comment)
		{
			g_fprintf (stream,_("Comment:\n"));
			if (bp)
				g_fprintf(stream,"</b><pre>");
			g_fprintf (stream,archive[idx]->comment->str);
			if (bp)
				g_fprintf(stream,"</pre>");
			g_fprintf (stream,"\n");
			if (bp)
				g_fprintf(stream,"<br>");
		}
		if ( ! bp)
		{
			g_fprintf (stream,"-------------------------------------------------------------------------------------------------------------\n");
			g_fprintf (stream,_("Files:%*s%s"),80," ",_("|Compressed\n"));
			g_fprintf (stream,"-------------------------------------------------------------------------------------------------------------\n");
		}
		else
		{
			g_fprintf(stream,"<br><table border=0 cellpadding=6 cellspacing=1><tr>");
			g_fprintf(stream,_("<th>Files:</th>"));
			g_fprintf(stream,_("<th>Compressed:</th>"));
			g_fprintf(stream,"</th></tr>");
			
		}
		xa_print_entry_in_file(archive[idx]->root_entry,idx,0,stream,bp);
		if (bp)
			g_fprintf (stream,"</table></body></html>");
		fclose (stream);
	}
}

void xa_print_entry_in_file(XEntry *entry,gint idx,unsigned long long int size,FILE *stream,int bp)
{
	gchar *path;
	static int x = 1;
	gint i;
	gpointer current_column;
	unsigned long long int file_size = 0;

	if (!entry)
		return;

    if (entry->filename && !entry->is_dir)
    {
    	current_column = entry->columns;
		/* Let's retrieve the sizes of the entry from its column */
		path = xa_build_full_path_name_from_entry(entry,NULL);
		if (strlen(path) == 0)
			goto here;
		for (i = 0; i < archive[idx]->nc; i++)
		{
			switch(archive[idx]->column_types[i+2])
			{
				case G_TYPE_STRING:
					current_column += sizeof(gchar *);
				break;

				case G_TYPE_UINT64:
					file_size = (*((guint64 *)current_column));
					current_column += sizeof(guint64);
				break;
			}
		}
		if (bp)
		{
			g_fprintf(stream,"<tr class=\"row%d\">",x);
			g_fprintf(stream,"<td>%s</td><td>%lld</td></tr>",path,file_size);
			g_fprintf(stream,"</td></tr>");
			if (x == 2)
				x = 1;
			else
				x = 2;
		}
		else
			g_fprintf(stream,"%-90s %lld\n",path,file_size);

		g_free(path);
	}
here:
	xa_print_entry_in_file(entry->child,idx,file_size,stream,bp);
	xa_print_entry_in_file(entry->next,idx,file_size,stream,bp);
}

void xa_close_archive (GtkMenuItem *menuitem,gpointer user_data)
{
	gint current_page;
	gint idx;
	GtkWidget *scrollwindow = user_data;

	current_page = gtk_notebook_page_num(notebook,scrollwindow);
	idx = xa_find_archive_index (current_page);
	gtk_notebook_remove_page ( notebook ,current_page);

	current_page = gtk_notebook_get_n_pages(notebook);
	if (current_page == 0)
	{
		gtk_widget_set_sensitive (up_button,FALSE);
		gtk_widget_set_sensitive (home_button,FALSE);
		gtk_widget_set_sensitive (view_shell_output1,FALSE);
		gtk_widget_set_sensitive (deselect_all,FALSE);
		gtk_widget_set_sensitive (comment_menu,FALSE);
		xa_disable_delete_buttons (FALSE);
		xa_set_button_state (1,1,0,0,0,0,0,0,0,0,0);
		xa_set_window_title (xa_main_window,NULL);
		gtk_tree_store_clear(GTK_TREE_STORE(archive_dir_model));
		gtk_entry_set_text(GTK_ENTRY(location_entry),"");
		gtk_label_set_text(GTK_LABEL(total_label),_("Select \"New\" to create or \"Open\" to open an archive"));
		gtk_widget_hide(selected_frame);
	}
	else if ( current_page == 1)
		gtk_notebook_set_show_tabs (notebook,FALSE);
	else
		gtk_notebook_set_show_tabs (notebook,TRUE);

	xa_clean_archive_structure (archive[idx]);
	archive[idx] = NULL;
}

gboolean xa_quit_application (GtkWidget *widget, GdkEvent *event, gpointer data)
{
	gint i;
	gint idx;

	i = gtk_notebook_get_current_page (notebook);
	idx = xa_find_archive_index (i);
	if (idx > -1 && archive[idx]->child_pid)
		return TRUE;
	
	g_list_free ( Suffix);
	g_list_free ( Name);

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
	gtk_widget_destroy(prefs_window->dialog1);
	g_free(prefs_window);
	
	gtk_widget_destroy (extract_window->dialog1);
	g_free(extract_window);
	
	gtk_widget_destroy (add_window->dialog1);
	g_free(add_window);

	gtk_widget_destroy (multi_extract_window->multi_extract);
	g_free(multi_extract_window);

	gtk_widget_destroy(xa_popup_menu);
	g_free (config_file);
	xa_free_icon_cache();

#ifdef HAVE_SOCKET
	socket_finalize();
#endif
	gtk_main_quit();
	return FALSE;
}

void xa_delete_archive (GtkMenuItem *menuitem,gpointer user_data)
{
	GList  *row_list = NULL;
	XEntry *entry = NULL;
	GtkTreeIter iter;
	GSList *list = NULL;
	gint current_page,id,response;

	current_page = gtk_notebook_get_current_page (notebook);
	id = xa_find_archive_index (current_page);

	GtkTreeSelection *selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (archive[id]->treeview));

	row_list = gtk_tree_selection_get_selected_rows(selection,&archive[id]->model);
	if (row_list != NULL)
	{
		archive[id]->status = XA_ARCHIVESTATUS_DELETE;
		while (row_list)
		{
			gtk_tree_model_get_iter(archive[id]->model,&iter,row_list->data);
			gtk_tree_model_get (archive[id]->model,&iter,archive[id]->nc+1,&entry,-1);
			gtk_tree_path_free (row_list->data);
			if (entry->is_dir)
			{
				if (archive[id]->type == XARCHIVETYPE_TAR || is_tar_compressed(archive[id]->type))
					goto one_file;
				else
				{	
					list = g_slist_prepend (list,xa_build_full_path_name_from_entry(entry,archive[id]));
					xa_fill_list_with_recursed_entries(entry->child,&list);
				}
			}
			else
			{
				one_file:
				list = g_slist_prepend (list,xa_build_full_path_name_from_entry(entry,archive[id]));
			}
			row_list = row_list->next;
		}
		g_list_free (row_list);
	}
	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(prefs_window->confirm_deletion)))
	{
		response = xa_show_message_dialog (GTK_WINDOW (xa_main_window),GTK_DIALOG_MODAL,GTK_MESSAGE_QUESTION,GTK_BUTTONS_OK_CANCEL,_("You are about to delete entries from the archive."),_( "Are you sure you want to do this?"));
		if (response == GTK_RESPONSE_CANCEL || response == GTK_RESPONSE_DELETE_EVENT)
			return;
	}
	(*archive[id]->delete) (archive[id],list);
}

void xa_add_files_archive (GtkMenuItem *menuitem,gpointer data)
{
	gint current_page,idx;

	current_page = gtk_notebook_get_current_page (notebook);
	idx = xa_find_archive_index (current_page);

	xa_set_add_dialog_options(add_window,archive[idx]);
	xa_parse_add_dialog_options (archive[idx],add_window);
}

void xa_extract_archive (GtkMenuItem *menuitem,gpointer user_data)
{
	gint current_page,idx,selected;

	current_page = gtk_notebook_get_current_page(notebook);
	idx = xa_find_archive_index (current_page);

	GtkTreeSelection *selection = gtk_tree_view_get_selection (GTK_TREE_VIEW(archive[idx]->treeview));
	selected = gtk_tree_selection_count_selected_rows (selection);
	xa_set_extract_dialog_options(extract_window,selected,archive[idx]);
    xa_parse_extract_dialog_options(archive[idx],extract_window,selection);
}

void xa_show_prefs_dialog (GtkMenuItem *menuitem,gpointer user_data)
{
	int response;

	if (prefs_window == NULL)
		prefs_window = xa_create_prefs_dialog();

	gtk_widget_show_all (prefs_window->dialog1);
	response = gtk_dialog_run (GTK_DIALOG(prefs_window->dialog1));
	gtk_widget_hide (prefs_window->dialog1);

	if (response == GTK_RESPONSE_OK)
		xa_apply_prefs_option(prefs_window);
}

void xa_convert_sfx (GtkMenuItem *menuitem ,gpointer user_data)
{
	gchar *command = NULL;
	GSList *list = NULL;
	gboolean result;
	gint current_page;
	gint idx;

	current_page = gtk_notebook_get_current_page (notebook);
	idx = xa_find_archive_index ( current_page);

    archive[idx]->status = XA_ARCHIVESTATUS_SFX;
    switch ( archive[idx]->type)
	{
		case XARCHIVETYPE_RAR:
			command = g_strconcat ("rar s -o+ ",archive[idx]->escaped_path,NULL);
			list = g_slist_append(list,command);
        	xa_run_command (archive[idx],list);
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
			int response;

			archive_name = xa_open_sfx_file_selector();
			if (archive_name == NULL)
			{
				gtk_widget_set_sensitive (Stop_button,FALSE);
				return;
			}
			archive_name_escaped = xa_escape_bad_chars (archive_name ,"$\'`\"\\!?* ()[]&|@#:;");
			unzipsfx_path = g_find_program_in_path ("unzipsfx");
			if (unzipsfx_path != NULL)
			{
				/* Load the unzipsfx executable in memory,about 50 KB */
				result = g_file_get_contents (unzipsfx_path,&content,&length,&error);
				if ( ! result)
				{
					response = xa_show_message_dialog (GTK_WINDOW (xa_main_window),GTK_DIALOG_MODAL,GTK_MESSAGE_ERROR,GTK_BUTTONS_OK,_("Can't convert the archive to self-extracting:"),error->message);
					g_error_free (error);
					g_free (unzipsfx_path);
					return;
				}
				g_free (unzipsfx_path);

				/* Write unzipsfx to a new file */
				sfx_archive = fopen ( archive_name ,"w");
				if (sfx_archive == NULL)
				{
					response = xa_show_message_dialog (GTK_WINDOW (xa_main_window),GTK_DIALOG_MODAL,GTK_MESSAGE_ERROR,GTK_BUTTONS_OK,_("Can't write the unzipsfx module to the archive:"),g_strerror(errno));
					return;
				}
				archive_not_sfx = fopen ( archive[idx]->path ,"r");
				fwrite (content,1,length,sfx_archive);
				g_free (content);

				/* Read archive data and write it after the sfx module in the new file */
				while ( ! feof(archive_not_sfx))
				{
					fread (&buffer ,1,1024,archive_not_sfx);
					fwrite (&buffer,1,1024,sfx_archive);
				}
				fclose (archive_not_sfx);
				fclose (sfx_archive);

				command = g_strconcat ("chmod 755 ",archive_name_escaped,NULL);
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
			int response;
			GtkWidget *locate_7zcon = NULL;
			GtkFileFilter *sfx_filter;

        	archive_name = xa_open_sfx_file_selector ();

			if (archive_name == NULL)
				return;
			archive_name_escaped = xa_escape_bad_chars (archive_name,"$\'`\"\\!?* ()[]&|@#:;");

			if (g_file_test ( "/usr/lib/p7zip/7zCon.sfx",G_FILE_TEST_EXISTS))
				sfx_path = g_strdup("/usr/lib/p7zip/7zCon.sfx");
			else if (g_file_test ( "/usr/local/lib/p7zip/7zCon.sfx",G_FILE_TEST_EXISTS))
				sfx_path = g_strdup ("/usr/local/lib/p7zip/7zCon.sfx");
			else if (g_file_test ( "/usr/libexec/p7zip/7zCon.sfx",G_FILE_TEST_EXISTS))
				sfx_path = g_strdup ("/usr/libexec/p7zip/7zCon.sfx");
			else
			{
				sfx_filter = gtk_file_filter_new ();
				gtk_file_filter_set_name (sfx_filter,"");
				gtk_file_filter_add_pattern (sfx_filter,"*.sfx");

				locate_7zcon = gtk_file_chooser_dialog_new ( _("Please select the 7zCon.sfx module"),
						GTK_WINDOW (xa_main_window),
						GTK_FILE_CHOOSER_ACTION_OPEN,
						GTK_STOCK_CANCEL,
						GTK_RESPONSE_CANCEL,
						"gtk-open",
						GTK_RESPONSE_ACCEPT,
						NULL);

				gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (locate_7zcon),sfx_filter);
				gtk_dialog_set_default_response (GTK_DIALOG (locate_7zcon),GTK_RESPONSE_ACCEPT);
				response = gtk_dialog_run (GTK_DIALOG(locate_7zcon));
				if (response == GTK_RESPONSE_ACCEPT)
				{
					sfx_path = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(locate_7zcon));
					gtk_widget_destroy (locate_7zcon);
				}
				else
				{
					gtk_widget_destroy (locate_7zcon);
					return;
				}
			}
			if ( sfx_path != NULL)
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
				sfx_archive = fopen ( archive_name ,"w");
				if (sfx_archive == NULL)
				{
					response = xa_show_message_dialog (GTK_WINDOW (xa_main_window),GTK_DIALOG_MODAL,GTK_MESSAGE_ERROR,GTK_BUTTONS_OK,_("Can't write the unzipsfx module to the archive:"),g_strerror(errno));
					return;
				}
				archive_not_sfx = fopen ( archive[idx]->path,"r");
				fwrite (content,1,length,sfx_archive);
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
        	command = g_strconcat ("arj y -je1 " ,archive[idx]->escaped_path,NULL);
        	list = g_slist_append(list,command);
        	xa_run_command (archive[idx],list);
		break;

		default:
		command = NULL;
	}
}

void xa_about (GtkMenuItem *menuitem,gpointer user_data)
{
    static GtkWidget *about = NULL;
    const char *authors[] = {"\nMain developer:\nGiuseppe Torelli <colossus73@gmail.com>\n\nArchive navigation code:\nJohn Berthels\n\nCode fixing:\nEnrico Tröger\n\nLHA and DEB support:\nŁukasz Zemczak <sil2100@vexillium.org>\n\nLZMA support:\nThomas Dy <dysprosium66@gmail.com>\n\nLZOP support:\nKevin Day\n",NULL};
    const char *documenters[] = {"\nSpecial thanks to Bjoern Martensen for\nbugs hunting and Xarchiver Tango logo.\n\nThanks to:\nBenedikt Meurer\nStephan Arts\nBruno Jesus <00cpxxx@gmail.com>\nUracile for the stunning logo\n",NULL};

	if (about == NULL)
	{
		about = gtk_about_dialog_new ();
		gtk_about_dialog_set_email_hook (xa_activate_link,NULL,NULL);
		gtk_about_dialog_set_url_hook (xa_activate_link,NULL,NULL);
		gtk_window_set_position (GTK_WINDOW (about),GTK_WIN_POS_CENTER_ON_PARENT);
		gtk_window_set_transient_for (GTK_WINDOW (about),GTK_WINDOW (xa_main_window));
		gtk_window_set_destroy_with_parent (GTK_WINDOW (about),TRUE);
		g_object_set (about,
			"name", "xarchiver",
			"version",PACKAGE_VERSION,
			"copyright","Copyright \xC2\xA9 2005-2008 Giuseppe Torelli",
			"comments",_("A GTK+2 only lightweight archive manager"),
			"authors",authors,
			"documenters",documenters,
			"translator_credits",_("translator-credits"),
			"logo_icon_name","xarchiver",
			"website","http://xarchiver.xfce.org",
			"license","Copyright \xC2\xA9 2005-2008 Giuseppe Torelli - Colossus <colossus73@gmail.com>\n\n"
		    			"This is free software; you can redistribute it and/or\n"
    					"modify it under the terms of the GNU Library General Public License as\n"
    					"published by the Free Software Foundation; either version 2 of the\n"
    					"License,or (at your option) any later version.\n"
    					"\n"
    					"This software is distributed in the hope that it will be useful,\n"
    					"but WITHOUT ANY WARRANTY; without even the implied warranty of\n"
    					"MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU\n"
    					"Library General Public License for more details.\n"
    					"\n"
    					"You should have received a copy of the GNU Library General Public\n"
    					"License along with the Gnome Library; see the file COPYING.LIB.  If not,\n"
    					"write to the Free Software Foundation,Inc.,59 Temple Place - Suite 330,\n"
    					"Boston,MA 02111-1307,USA.\n",
		      NULL);
	}
	gtk_dialog_run ( GTK_DIALOG(about));
	gtk_widget_hide (about);
}

gchar *xa_open_sfx_file_selector ()
{
	gchar *sfx_name = NULL;
	GtkWidget *sfx_file_selector = NULL;
	int response;

	sfx_file_selector = gtk_file_chooser_dialog_new ( _("Save the self-extracting archive as"),
						GTK_WINDOW (xa_main_window),
						GTK_FILE_CHOOSER_ACTION_SAVE,
						GTK_STOCK_CANCEL,
						GTK_RESPONSE_CANCEL,
						"gtk-save",
						GTK_RESPONSE_ACCEPT,
						NULL);

	gtk_dialog_set_default_response (GTK_DIALOG (sfx_file_selector),GTK_RESPONSE_ACCEPT);
	gtk_file_chooser_set_do_overwrite_confirmation(GTK_FILE_CHOOSER (sfx_file_selector),TRUE);
	response = gtk_dialog_run ( GTK_DIALOG(sfx_file_selector));

	if (response == GTK_RESPONSE_ACCEPT)
		sfx_name = gtk_file_chooser_get_filename ( GTK_FILE_CHOOSER (sfx_file_selector));

	gtk_widget_destroy (sfx_file_selector);
	return sfx_name;
}

gchar *xa_open_file_dialog ()
{
	static GtkWidget *File_Selector = NULL;
	GtkFileFilter *filter;
	gchar *path = NULL;
	int response;

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

		gtk_dialog_set_default_response (GTK_DIALOG (File_Selector),GTK_RESPONSE_ACCEPT);
		gtk_window_set_destroy_with_parent (GTK_WINDOW (File_Selector) ,TRUE);

		filter = gtk_file_filter_new ();
		gtk_file_filter_set_name ( filter ,_("All files"));
		gtk_file_filter_add_pattern ( filter,"*");
		gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (File_Selector),filter);

		filter = gtk_file_filter_new ();
		gtk_file_filter_set_name ( filter ,_("Only archives"));
		Suffix = g_list_first ( ArchiveSuffix);
		while ( Suffix != NULL)
		{
			gtk_file_filter_add_pattern (filter,Suffix->data);
			Suffix = g_list_next ( Suffix);
		}
		gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (File_Selector),filter);

		Suffix = g_list_first ( ArchiveSuffix);
		while ( Suffix != NULL)
		{
			if ( strcmp(Suffix->data,"") != 0)	/* To avoid double filtering when opening the archive */
			{
				filter = gtk_file_filter_new ();
				gtk_file_filter_set_name (filter,Suffix->data);
				gtk_file_filter_add_pattern (filter,Suffix->data);
				gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (File_Selector),filter);
			}
			Suffix = g_list_next ( Suffix);
		}
		gtk_window_set_modal (GTK_WINDOW (File_Selector),TRUE);
	}
	if (open_file_filter != NULL)
		gtk_file_chooser_set_filter (GTK_FILE_CHOOSER (File_Selector) ,open_file_filter);

	if (current_open_directory != NULL)
		gtk_file_chooser_set_current_folder ( GTK_FILE_CHOOSER (File_Selector) ,current_open_directory);

	response = gtk_dialog_run (GTK_DIALOG (File_Selector));

	if (current_open_directory != NULL)
		g_free (current_open_directory);

	current_open_directory = gtk_file_chooser_get_current_folder ( GTK_FILE_CHOOSER (File_Selector));
	open_file_filter = gtk_file_chooser_get_filter ( GTK_FILE_CHOOSER (File_Selector));

	if (response == GTK_RESPONSE_ACCEPT)
		path = gtk_file_chooser_get_filename ( GTK_FILE_CHOOSER (File_Selector));
	else if ( (response == GTK_RESPONSE_CANCEL) || (response == GTK_RESPONSE_DELETE_EVENT))
		path = NULL;

	/* Hiding the window instead of destroying it will preserve the pointers to the file chooser stuff */
	gtk_widget_hide (File_Selector);
	return path;
}

XArchiveType xa_detect_archive_type (gchar *filename)
{
	FILE *dummy_ptr = NULL;
    int xx = XARCHIVETYPE_UNKNOWN;
	unsigned char magic[14]={0,0,0,0,0,0,0,0,0,0,0,0,0,0}; /* avoid problems with garbage */

	dummy_ptr = fopen (filename,"r");

	if (dummy_ptr == NULL)
		return XARCHIVETYPE_NOT_FOUND;

	fread (magic,1,14,dummy_ptr);
	if (memcmp (magic,"\x50\x4b",2) == 0)
		xx = XARCHIVETYPE_ZIP;
	else if (memcmp (magic,"\x60\xea",2) == 0)
		xx = XARCHIVETYPE_ARJ;
	else if (memcmp ( magic,"\x52\x61\x72\x21",4) == 0)
		xx = XARCHIVETYPE_RAR;
	else if (memcmp ( magic,"\x42\x5a\x68",3) == 0)
		xx = XARCHIVETYPE_BZIP2;
	else if (memcmp ( magic,"\x1f\x8b",2) == 0 || memcmp ( magic,"\x1f\x9d",2) == 0)
		xx = XARCHIVETYPE_GZIP;
	else if (memcmp ( magic,"\x5d\x00\x00\x80",4) == 0)
		xx = XARCHIVETYPE_LZMA;
	else if (memcmp ( magic,"\211LZO",4) == 0)
		xx = XARCHIVETYPE_LZOP;
	else if (memcmp ( magic,"\xed\xab\xee\xdb",4) == 0)
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

gboolean xa_detect_archive_comment (int type,gchar *filename,XArchive *archive)
{
	FILE *stream;
	char sig = '1';
	guint cmt_len = 0;
	int byte;
	unsigned char eocds[] = { 0x50,0x4b,0x05,0x06 };
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
		fseek(stream,0L,SEEK_SET);
		/* Let's reach the end of central directory signature now */
		while( ! feof(stream))
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
		fseek ( stream,39 ,SEEK_SET);
		while (sig != 0)
		{
			fread (&sig,1,1,stream);
			cmt_len++;
		}
		fseek ( stream,39 + cmt_len ,SEEK_SET);
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
				g_string_append_c (archive->comment,sig);
		}
		return TRUE;
	}
	return FALSE;
}

void xa_remove_columns(XArchive *archive)
{
	GList *columns = gtk_tree_view_get_columns ( GTK_TREE_VIEW (archive->treeview));
	while (columns != NULL)
	{
		gtk_tree_view_remove_column (GTK_TREE_VIEW (archive->treeview),columns->data);
		columns = columns->next;
	}
	g_list_free (columns);
}

void xa_create_liststore (XArchive *archive,gchar *columns_names[])
{
	unsigned short int x;
	GtkCellRenderer *renderer;
	GtkTreeViewColumn *column;

	archive->liststore = gtk_list_store_newv ( archive->nc+2 ,archive->column_types);
	gtk_tree_view_set_model ( GTK_TREE_VIEW (archive->treeview),GTK_TREE_MODEL (archive->liststore));

	archive->model = GTK_TREE_MODEL(archive->liststore);
	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(prefs_window->check_sort_filename_column)))
		gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(archive->model),1,GTK_SORT_ASCENDING);
	gtk_tree_sortable_set_sort_func(GTK_TREE_SORTABLE(archive->liststore),1,xa_sort_dirs_before_files,archive,NULL);

	g_object_ref(archive->model);
	gtk_tree_view_set_model(GTK_TREE_VIEW(archive->treeview),NULL);

	/* First column: icon + text */
	column = gtk_tree_view_column_new();
	archive->renderer = gtk_cell_renderer_pixbuf_new();
	g_object_set(G_OBJECT(archive->renderer),"stock-size",(3 - gtk_combo_box_get_active(GTK_COMBO_BOX(prefs_window->combo_icon_size))),NULL);
	gtk_tree_view_column_pack_start(column,archive->renderer,FALSE);
	gtk_tree_view_column_set_attributes(column,archive->renderer,"pixbuf",0,NULL);

	archive->renderer_text = gtk_cell_renderer_text_new ();
	gtk_tree_view_column_pack_start(column,archive->renderer_text,TRUE);
	gtk_tree_view_column_set_attributes( column,archive->renderer_text,"text",1,NULL);
	gtk_tree_view_column_set_title(column,_("Filename"));
	gtk_tree_view_column_set_resizable (column,TRUE);
	gtk_tree_view_column_set_sort_column_id (column,1);
	gtk_tree_view_append_column (GTK_TREE_VIEW (archive->treeview),column);
	gtk_tree_view_column_set_sizing (column,GTK_TREE_VIEW_COLUMN_AUTOSIZE);
	g_signal_connect (archive->renderer_text,"editing-canceled",G_CALLBACK (xa_rename_cell_edited_canceled),archive);
	g_signal_connect (archive->renderer_text,"edited",G_CALLBACK (xa_rename_cell_edited),archive);

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
			gtk_tree_view_column_set_resizable (column,TRUE);
			gtk_tree_view_column_set_sort_column_id (column,x+2);
		}
		gtk_tree_view_append_column (GTK_TREE_VIEW (archive->treeview),column);
	}
}

gboolean treeview_select_search (GtkTreeModel *model,gint column,const gchar *key,GtkTreeIter *iter,gpointer search_data)
{
    char *filename;
    gboolean result;

    gtk_tree_model_get(model,iter,1,&filename,-1);
    if (strcasestr (filename,key))
    	result = FALSE;
	else
		result = TRUE;
    g_free (filename);
    return result;
}

void xa_cancel_archive (GtkMenuItem *menuitem,gpointer data)
{
	gint current_page,idx,response;

	current_page = gtk_notebook_get_current_page(notebook);
	idx = xa_find_archive_index (current_page);
	gtk_widget_set_sensitive(Stop_button,FALSE);
	if (GTK_WIDGET_VISIBLE(multi_extract_window->multi_extract))
	{
		multi_extract_window->stop_pressed = TRUE;
		kill (multi_extract_window->archive->child_pid,SIGINT);
	}
	else
	{
		if (archive[idx]->status == XA_ARCHIVESTATUS_ADD || archive[idx]->status == XA_ARCHIVESTATUS_SFX)
		{
			response = xa_show_message_dialog (GTK_WINDOW(xa_main_window),GTK_DIALOG_MODAL,GTK_MESSAGE_QUESTION,GTK_BUTTONS_OK_CANCEL,_("Doing so will probably corrupt your archive!"),_("Do you really want to cancel?"));
			if (response == GTK_RESPONSE_CANCEL)
				return;
		}
		if (archive[idx]->child_pid)
		{
			kill (archive[idx]->child_pid,SIGINT);
			archive[idx]->child_pid = 0;
		}
		/* This in case the user cancels the opening of a password protected archive */
		if (archive[idx]->status != XA_ARCHIVESTATUS_ADD)
			if (archive[idx]->has_passwd)
				archive[idx]->has_passwd = FALSE;

		gtk_label_set_text(GTK_LABEL(total_label),"");
	}
}

void xa_archive_properties (GtkMenuItem *menuitem,gpointer user_data)
{
	struct stat my_stat;
    gchar *utf8_string ,*dummy_string,*t;
    char date[64];
    gint current_page;
	gint idx;
	unsigned long long int file_size;
	double content_size;

    current_page = gtk_notebook_get_current_page (notebook);
	idx = xa_find_archive_index (current_page);
    stat (archive[idx]->path ,&my_stat);
    file_size = my_stat.st_size;
    archive_properties_window = xa_create_archive_properties_window();
    utf8_string = xa_remove_path_from_archive_name(archive[idx]->escaped_path);
	gtk_label_set_text(GTK_LABEL(name_data),utf8_string);
	g_free (utf8_string);
    /* Path */
    dummy_string = xa_remove_level_from_path (archive[idx]->path);
    if (strlen(dummy_string) == 0 || strcmp(dummy_string,"..") == 0 || strcmp(dummy_string,".") == 0)
		utf8_string = g_filename_display_name (g_get_current_dir());
    else
		utf8_string = g_filename_display_name (dummy_string);
    g_free ( dummy_string);

    gtk_label_set_text(GTK_LABEL(path_data),utf8_string);
    g_free ( utf8_string);
	/* Type */
	gtk_label_set_text(GTK_LABEL(type_data),archive[idx]->format);
    /* Modified Date */
    strftime (date,64,"%c",localtime (&my_stat.st_mtime));
    t = g_locale_to_utf8 (date,-1,0,0,0);
    gtk_label_set_text(GTK_LABEL(modified_data),t);
    g_free (t);
    /* Archive Size */
	t = xa_set_size_string(file_size);
    gtk_label_set_text(GTK_LABEL(size_data),t);
    g_free (t);
    /* content_size */
    t = xa_set_size_string(archive[idx]->dummy_size);
    gtk_label_set_text(GTK_LABEL(content_data),t);
    g_free (t);
    /* Has Comment */
    if (archive[idx]->has_comment)
		gtk_label_set_text(GTK_LABEL(comment_data),_("Yes"));
	else
		gtk_label_set_text(GTK_LABEL(comment_data),_("No"));

    /* Compression_ratio */
    content_size = (double)archive[idx]->dummy_size / file_size;
    t = g_strdup_printf ( "%.2f",content_size);
    gtk_label_set_text(GTK_LABEL(compression_data),t);
    g_free (t);
    /* Number of files */
    t = g_strdup_printf ( "%d",archive[idx]->nr_of_files);
    gtk_label_set_text(GTK_LABEL(number_of_files_data),t);
    g_free (t);
    
    if (archive[idx]->has_passwd)
    	gtk_label_set_text(GTK_LABEL(encrypted_data),_("Yes"));
	else
		gtk_label_set_text(GTK_LABEL(encrypted_data),_("No"));
    gtk_widget_show_all (archive_properties_window);
}

gchar *xa_set_size_string (unsigned long long int file_size)
{
	gchar *message = NULL;
	gchar *measure;
	double content_size;

	if (file_size > 1024*1024*1024)
	{
		content_size = (double)file_size / (1024*1024*1024);
		measure = "GB";
	}
	else if (file_size > 1024*1024)
	{
		content_size = (double)file_size / (1024*1024);
		measure = "MB";
	}

    else if (file_size > 1024)
	{
		content_size = (double)file_size / 1024;
		measure = "KB";
	}
	else
	{
		measure = "Bytes";
		content_size = file_size;
	}
    message = g_strdup_printf ("%.1f %s",content_size,measure);
	return message;
}

void xa_set_statusbar_message_for_displayed_rows(XArchive *archive)
{
	gchar *info = NULL;
	GtkTreePath *path = NULL;
	GtkTreeIter iter;
	gint n_elem = 0,pos = 0,dirs = 0;
	unsigned long int total_size = 0;
	guint64 size = 0;
	XEntry *entry = NULL;

	path = gtk_tree_path_new_first();
	if (! GTK_IS_TREE_MODEL(archive->model) || gtk_tree_model_get_iter (archive->model,&iter,path) == FALSE)
	{
		gtk_tree_path_free(path);
		return;
	}

	switch (archive->type)
	{
		case XARCHIVETYPE_GZIP:
		case XARCHIVETYPE_BZIP2:
		case XARCHIVETYPE_LZMA:
		case XARCHIVETYPE_LZOP:
		case XARCHIVETYPE_RPM:
		pos = 3;
		break;

		case XARCHIVETYPE_RAR:
		case XARCHIVETYPE_ARJ:
		case XARCHIVETYPE_7ZIP:
		pos = 2;
		break;

		case XARCHIVETYPE_DEB:
		case XARCHIVETYPE_LHA:
		pos = 4;
		break;

		case XARCHIVETYPE_TAR_GZ:
		case XARCHIVETYPE_TAR_BZ2:
		case XARCHIVETYPE_TAR_LZMA:
		case XARCHIVETYPE_TAR_LZOP:
		case XARCHIVETYPE_TAR:
		case XARCHIVETYPE_ZIP:
		pos = 5;
		break;

		default:
		break;
	}
	gtk_tree_path_free(path);
	do
	{
		gtk_tree_model_get (archive->model,&iter,pos,&size,-1);
		gtk_tree_model_get (archive->model,&iter,archive->nc+1,&entry,-1);
		if (entry == NULL)
			return;
		if (entry->is_dir)
			dirs++;
		else
			n_elem++;
		total_size += size;
	}
	while (gtk_tree_model_iter_next (archive->model,&iter));
	info = xa_get_statusbar_message(total_size,n_elem,dirs,FALSE);
	gtk_label_set_text (GTK_LABEL(total_label),info);
	g_free(info);
}

void xa_row_selected (GtkTreeSelection *selection,XArchive *archive)
{
	GList *list = NULL;
	gchar *msg = NULL;
	GtkTreeIter iter;
	GtkTreeModel *model;
	gint selected = 0,pos = 0,dirs = 0;
	unsigned long int total_size = 0;
	guint64 size = 0;
	XEntry *entry;

	switch (archive->type)
	{
		case XARCHIVETYPE_GZIP:
		case XARCHIVETYPE_BZIP2:
		case XARCHIVETYPE_LZMA:
		case XARCHIVETYPE_LZOP:
		case XARCHIVETYPE_RPM:
		pos = 3;
		break;

		case XARCHIVETYPE_RAR:
		case XARCHIVETYPE_ARJ:
		case XARCHIVETYPE_7ZIP:
		pos = 2;
		break;

		case XARCHIVETYPE_LHA:
		case XARCHIVETYPE_DEB:
		pos = 4;
		break;

		case XARCHIVETYPE_TAR_GZ:
		case XARCHIVETYPE_TAR_BZ2:
		case XARCHIVETYPE_TAR_LZMA:
		case XARCHIVETYPE_TAR_LZOP:
		case XARCHIVETYPE_TAR:
		case XARCHIVETYPE_ZIP:
		pos = 5;
		break;

		default:
		break;
	}
	model = gtk_tree_view_get_model(GTK_TREE_VIEW(archive->treeview));
	selected = gtk_tree_selection_count_selected_rows (selection);

	if (selected == 0)
	{
		xa_disable_delete_buttons (FALSE);
		gtk_widget_hide(selected_frame);
		return;
	}
	else
	{
		gtk_widget_show(selected_frame);
		gtk_widget_set_sensitive(deselect_all,TRUE);
	}
	if ( (archive->type == XARCHIVETYPE_RAR && unrar) || archive->type == XARCHIVETYPE_BZIP2 || archive->type == XARCHIVETYPE_GZIP)
	{
		gtk_widget_set_sensitive (delete_menu,FALSE);
		gtk_widget_set_sensitive (rename_menu,FALSE);
	}
	else if (archive->type != XARCHIVETYPE_RPM && archive->type != XARCHIVETYPE_DEB)
	{
		gtk_widget_set_sensitive (delete_menu,TRUE);
		gtk_widget_set_sensitive (rename_menu,TRUE);
	}
	if (selected > 1)
		gtk_widget_set_sensitive (rename_menu,FALSE);

	selected = 0;
	list = gtk_tree_selection_get_selected_rows(selection,NULL);
	while (list)
	{
		gtk_tree_model_get_iter(model,&iter,list->data);
		gtk_tree_model_get (model,&iter,pos,&size,-1);
		gtk_tree_model_get (archive->model,&iter,archive->nc+1,&entry,-1);
		if (entry->is_dir)
			dirs++;
		else
			selected++;
		gtk_tree_path_free (list->data);
		total_size += size;
		list = list->next;
	}
	g_list_free(list);
	msg = xa_get_statusbar_message(total_size,selected,dirs,TRUE);
	gtk_label_set_text (GTK_LABEL(selected_label),msg);
	g_free(msg);
}

gchar *xa_get_statusbar_message(unsigned long int total_size,gint n_elem,gint dirs,gboolean selection)
{
	gchar *measure = NULL,*info = NULL;
	gchar *text = "";

	measure = xa_set_size_string(total_size);
	if (selection)
		text = _("selected");

	if (dirs)
	{
		if (n_elem)
			info = g_strdup_printf(ngettext ("%d file and %d dir %s (%s)","%d files and %d dirs %s (%s)",n_elem),n_elem,dirs,text,measure);
		else
			info = g_strdup_printf(ngettext ("%d dir %s (%s)","%d dirs %s (%s)",dirs),dirs,text,measure);
	}
	else
		info = g_strdup_printf(ngettext ("%d file %s (%s)","%d files %s (%s)",n_elem),n_elem,text,measure);

	g_free(measure);
	return info;
}

void drag_begin (GtkWidget *treeview1,GdkDragContext *context,XArchive *archive)
{
    GtkTreeSelection *selection;
    GtkTreeIter       iter;
    GList            *row_list;
	XEntry *entry;
	
	gtk_drag_source_set_icon_name (archive->treeview,"xarchiver");
    selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (archive->treeview));

	row_list = gtk_tree_selection_get_selected_rows (selection,NULL);
	if (row_list == NULL)
		return;

	gtk_tree_model_get_iter(archive->model,&iter,(GtkTreePath*) (row_list->data));
	gtk_tree_model_get (GTK_TREE_MODEL (archive->liststore),&iter,archive->nc+1,&entry,-1);

	gdk_property_change (context->source_window,
					gdk_atom_intern ("XdndDirectSave0",FALSE),
					gdk_atom_intern ("text/plain",FALSE),
					8,GDK_PROP_MODE_REPLACE,
					(guchar *) XDS_FILENAME,
			     	strlen (XDS_FILENAME));

	g_list_foreach (row_list,(GFunc) gtk_tree_path_free,NULL);
	g_list_free (row_list);
}

void drag_end (GtkWidget *treeview1,GdkDragContext *context,gpointer data)
{
	/* Nothing to do */
}

void drag_data_get (GtkWidget *widget,GdkDragContext *dc,GtkSelectionData *selection_data,guint info,guint t,XArchive *archive)
{
	GtkTreeSelection *selection;
	GList *row_list = NULL;
	GSList *names = NULL;
	guchar *_destination;
	gchar *destination,*to_send;
	int response;

	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (archive->treeview));
	row_list = gtk_tree_selection_get_selected_rows (selection,NULL);

	if (row_list == NULL)
		return;

	if (archive->child_pid!= 0)
	{
		response = xa_show_message_dialog (GTK_WINDOW (xa_main_window),GTK_DIALOG_MODAL,GTK_MESSAGE_ERROR,GTK_BUTTONS_OK,_("Can't perform another extraction:"),_("Please wait until the completion of the current one!"));
		return;
	}
	gdk_property_get (	dc->source_window,
						gdk_atom_intern ("XdndDirectSave0",FALSE),
						gdk_atom_intern ("text/plain",FALSE),
						0,4096,FALSE,NULL,NULL,NULL,&_destination );
	if (_destination)
	{
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
		destination = g_filename_from_uri((gchar*)_destination,NULL,NULL);
		g_free(_destination);

		archive->extraction_path = xa_remove_level_from_path (destination);
		g_free(destination);

		if (access (archive->extraction_path,R_OK | W_OK | X_OK))
		{
			gchar *utf8_path;
			gchar  *msg;

			utf8_path = g_filename_to_utf8 (archive->extraction_path,-1,NULL,NULL,NULL);
			msg = g_strdup_printf (_("You don't have the right permissions to extract the files to the directory \"%s\"."),utf8_path);
			response = xa_show_message_dialog (GTK_WINDOW (xa_main_window),GTK_DIALOG_MODAL,GTK_MESSAGE_ERROR,GTK_BUTTONS_OK,_("Can't perform extraction!"),msg );
			g_free (utf8_path);
			g_free (msg);
			to_send = "E";
		}
		else
		{
			gtk_tree_selection_selected_foreach (selection,(GtkTreeSelectionForeachFunc) xa_concat_selected_filenames,&names);
			archive->full_path = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON (extract_window->extract_full));
			archive->overwrite = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON (extract_window->overwrite_check));
			(*archive->extract) (archive,names);

			g_list_foreach (row_list,(GFunc) gtk_tree_path_free,NULL);
			g_list_free (row_list);
			to_send = "S";
		}
		if (archive->extraction_path != NULL)
		{
			g_free (archive->extraction_path);
			archive->extraction_path = NULL;
		}
		gtk_selection_data_set (selection_data,selection_data->target,8,(guchar*)to_send,1);
	}
}

void on_drag_data_received (GtkWidget *widget,GdkDragContext *context,int x,int y,GtkSelectionData *data,unsigned int info,unsigned int time,gpointer user_data)
{
	gchar **array = NULL;
	gchar *filename = NULL;
	gchar *_current_dir = NULL;
	GSList *list = NULL;
	gboolean one_file;
	unsigned int len = 0;
	gint current_page,idx,response;

	current_page = gtk_notebook_get_current_page(notebook);
	idx = xa_find_archive_index (current_page);
	array = gtk_selection_data_get_uris(data);

	if (array == NULL)
	{
		response = xa_show_message_dialog (GTK_WINDOW (xa_main_window),GTK_DIALOG_MODAL,GTK_MESSAGE_ERROR,GTK_BUTTONS_OK,_("Sorry,I could not perform the operation!"),"");
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
		response = xa_show_message_dialog (GTK_WINDOW (xa_main_window),GTK_DIALOG_MODAL,GTK_MESSAGE_ERROR,GTK_BUTTONS_OK,_("Can't perform this action:"),_("You have to install rar package!"));
		return;
	}
	if (archive[idx]->type == XARCHIVETYPE_DEB || archive[idx]->type == XARCHIVETYPE_RPM)
	{
		gchar *msg;
		if (archive[idx]->type == XARCHIVETYPE_DEB)
			msg = _("You can't add content to deb packages!");
		else if (archive[idx]->type == XARCHIVETYPE_RPM)
			msg = _("You can't add content to rpm packages!");
		else
			msg = _("The archiver doesn't support this feature!");
		response = xa_show_message_dialog (GTK_WINDOW (xa_main_window),GTK_DIALOG_MODAL,GTK_MESSAGE_ERROR,GTK_BUTTONS_OK,_("Can't perform this action:"),msg);
		gtk_drag_finish(context,FALSE,FALSE,time);
		return;
	}
	_current_dir = g_path_get_dirname(array[0]);
	if (archive[idx]->working_dir != NULL)
	{
		g_free(archive[idx]->working_dir);
		archive[idx]->working_dir = NULL;
	}
	archive[idx]->working_dir = g_filename_from_uri (_current_dir,NULL,NULL);
	g_free(_current_dir);
	while (array[len])
	{
		filename = g_filename_from_uri (array[len],NULL,NULL);
		list = g_slist_append(list,filename);
		len++;
	}
	archive[idx]->full_path 	= gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON (add_window->store_path));
	archive[idx]->add_recurse	= gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON (prefs_window->allow_sub_dir));
	xa_execute_add_commands(archive[idx],list,NULL);
	g_strfreev (array);
}

void xa_concat_selected_filenames (GtkTreeModel *model,GtkTreePath *treepath,GtkTreeIter *iter,GSList **data)
{
	XEntry *entry = NULL;
	gchar *filename = NULL;
	gint current_page;
	gint idx;

	current_page = gtk_notebook_get_current_page(notebook);
	idx = xa_find_archive_index (current_page);
	
	gtk_tree_model_get (model,iter,archive[idx]->nc+1,&entry,-1);
	if (entry->is_dir)
		xa_fill_list_with_recursed_entries(entry->child,data);
	else
		filename = xa_build_full_path_name_from_entry(entry,archive[idx]);
	*data = g_slist_prepend (*data,filename);
}

void xa_select_all(GtkMenuItem *menuitem,gpointer user_data)
{
	gint idx;
	gint current_page;

	current_page = gtk_notebook_get_current_page (notebook);
	idx = xa_find_archive_index (current_page);

	current_page = gtk_notebook_get_current_page (notebook);
	gtk_tree_selection_select_all ( gtk_tree_view_get_selection (GTK_TREE_VIEW (archive[idx]->treeview)));
	gtk_widget_set_sensitive (select_all,FALSE);
	gtk_widget_set_sensitive (deselect_all,TRUE);
}

void xa_deselect_all (GtkMenuItem *menuitem,gpointer user_data)
{
	gint current_page;
	gint idx;

	current_page = gtk_notebook_get_current_page (notebook);
	idx = xa_find_archive_index (current_page);

	gtk_tree_selection_unselect_all (gtk_tree_view_get_selection(GTK_TREE_VIEW(archive[idx]->treeview)));
	gtk_widget_set_sensitive (select_all,TRUE);
	gtk_widget_set_sensitive (deselect_all,FALSE);
}

void xa_activate_link (GtkAboutDialog *about,const gchar *link,gpointer data)
{
	gboolean result;
	int response;

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
		xa_launch_external_program("xdg-open",(gchar*)link);
}

void xa_determine_program_to_run(gchar *file)
{
	gchar *program = NULL;

	if ( !xdg_open)
	{
		if (strstr(file,".html"))
		{
			program = gtk_combo_box_get_active_text (GTK_COMBO_BOX(prefs_window->combo_prefered_web_browser));
		}
		else if (strstr(file,".txt"))
		{
			program = gtk_combo_box_get_active_text (GTK_COMBO_BOX(prefs_window->combo_prefered_editor));
		}
		else if (strstr(file,".png") || strstr(file,".gif") || strstr(file,".jpg") || strstr(file,".bmp") ||
				 strstr(file,".tif") || strstr(file,".tiff")|| strstr(file,".svg") || strstr(file,".png") ||
				 strstr(file,".tga"))
			program = gtk_combo_box_get_active_text (GTK_COMBO_BOX(prefs_window->combo_prefered_viewer));
		else
		{
			xa_show_message_dialog (GTK_WINDOW (xa_main_window),GTK_DIALOG_MODAL,GTK_MESSAGE_ERROR,GTK_BUTTONS_OK,_("This file type is not supported!"),_("Please install xdg-utils package."));
			return;
		}
	}
	else
		program = g_strdup("xdg-open");

	if (program == NULL)
	{
		xa_show_message_dialog (GTK_WINDOW (xa_main_window),GTK_DIALOG_MODAL,GTK_MESSAGE_INFO,GTK_BUTTONS_OK,_("You didn't set which program to use for opening this file!"),_("Please go to Preferences->Advanced and set it."));
		return;
	}
	xa_launch_external_program(program,file);
	g_free(program);
}

gboolean xa_launch_external_program(gchar *program,gchar *arg)
{
	GtkWidget *message;
	GError *error = NULL;
	gchar *command_line = NULL;
	gchar **argv;
	GdkScreen *screen;

	command_line = g_strconcat(program," ",arg,NULL);
	g_shell_parse_argv(command_line,NULL,&argv,NULL);
	g_free(command_line);

	screen = gtk_widget_get_screen (GTK_WIDGET (xa_main_window));
	if (!gdk_spawn_on_screen (screen,NULL,argv,NULL,G_SPAWN_SEARCH_PATH,NULL,NULL,NULL,&error))
	{
		message = gtk_message_dialog_new (GTK_WINDOW (xa_main_window),
										GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
										GTK_MESSAGE_ERROR,
										GTK_BUTTONS_CLOSE,
										_("Failed to launch the application!"));
		gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (message),"%s.",error->message);
		gtk_dialog_run (GTK_DIALOG (message));
		gtk_widget_destroy (message);
		g_error_free (error);
		return FALSE;
	}
	return TRUE;
}

void xa_show_help (GtkMenuItem *menuitem,gpointer user_data)
{
	gchar *uri = g_strconcat ("file://",DATADIR,"/doc/",PACKAGE,"/html/index.html",NULL);
	xa_activate_link (NULL,uri,NULL);
	g_free (uri);
}

void xa_enter_password (GtkMenuItem *menuitem ,gpointer user_data)
{
	gint current_page;
	gint idx;

	current_page = gtk_notebook_get_current_page(notebook);
	idx = xa_find_archive_index ( current_page);

	if (archive[idx] == NULL)
		return;
	else
	{
		g_free (archive[idx]->passwd);
		archive[idx]->passwd = NULL;
	}
	archive[idx]->passwd = xa_create_password_dialog (archive[idx]);
}

void xa_show_archive_comment (GtkMenuItem *menuitem,gpointer user_data)
{
	gchar *utf8_line;
	gsize len;
	gint current_page;
	gint idx;
	GtkWidget *textview;
	GtkWidget *dialog_vbox1;
	GtkWidget *scrolledwindow1;
	GtkWidget *dialog_action_area1;
	GtkWidget *tmp_image,*file,*clear,*close,*cancel,*file_hbox,*file_label;
	GtkTextBuffer *textbuffer;
	GtkTextIter iter;

	current_page = gtk_notebook_get_current_page(notebook);
	idx = xa_find_archive_index (current_page);

	comment_dialog = gtk_dialog_new_with_buttons (_("Comment"),GTK_WINDOW(xa_main_window),GTK_DIALOG_MODAL,NULL);
	gtk_window_set_position (GTK_WINDOW (comment_dialog),GTK_WIN_POS_CENTER_ON_PARENT);
	gtk_window_set_type_hint (GTK_WINDOW (comment_dialog),GDK_WINDOW_TYPE_HINT_DIALOG);
	gtk_dialog_set_has_separator (GTK_DIALOG (comment_dialog),FALSE);
	dialog_vbox1 = GTK_DIALOG (comment_dialog)->vbox;
	gtk_widget_set_size_request(comment_dialog,500,330);

	scrolledwindow1 = gtk_scrolled_window_new (NULL,NULL);
	gtk_widget_show (scrolledwindow1);
	gtk_box_pack_start (GTK_BOX (dialog_vbox1),scrolledwindow1,TRUE,TRUE,0);
	g_object_set (G_OBJECT (scrolledwindow1),"hscrollbar-policy",GTK_POLICY_AUTOMATIC,"shadow-type",GTK_SHADOW_IN,"vscrollbar-policy",GTK_POLICY_AUTOMATIC,NULL);

	textbuffer = gtk_text_buffer_new (NULL);
	gtk_text_buffer_create_tag (textbuffer,"font","family","monospace",NULL);
	gtk_text_buffer_get_iter_at_offset (textbuffer,&iter,0);

	textview = gtk_text_view_new_with_buffer (textbuffer);
	g_object_unref (textbuffer);
	gtk_container_add (GTK_CONTAINER (scrolledwindow1),textview);

	dialog_action_area1 = GTK_DIALOG (comment_dialog)->action_area;
	gtk_button_box_set_layout (GTK_BUTTON_BOX (dialog_action_area1),GTK_BUTTONBOX_END);

	clear = gtk_button_new_from_stock ("gtk-clear");
	gtk_dialog_add_action_widget (GTK_DIALOG (comment_dialog),clear,0);
	g_signal_connect (G_OBJECT (clear),"clicked",G_CALLBACK (xa_clear_comment_window),textbuffer);

	file = gtk_button_new();
	tmp_image = gtk_image_new_from_stock ("gtk-harddisk",GTK_ICON_SIZE_BUTTON);
	file_hbox = gtk_hbox_new(FALSE,4);
	file_label = gtk_label_new_with_mnemonic(_("From File"));

	alignment2 = gtk_alignment_new (0.5,0.5,0,0);
	gtk_container_add (GTK_CONTAINER (alignment2),file_hbox);
	gtk_box_pack_start(GTK_BOX(file_hbox),tmp_image,FALSE,TRUE,0);
	gtk_box_pack_start(GTK_BOX(file_hbox),file_label,FALSE,TRUE,0);
	gtk_container_add(GTK_CONTAINER(file),alignment2);
	gtk_dialog_add_action_widget (GTK_DIALOG (comment_dialog),file,0);
	g_signal_connect (G_OBJECT (file),"clicked",G_CALLBACK (xa_load_comment_window_from_file),textbuffer);

	cancel = gtk_button_new_from_stock ("gtk-cancel");
	gtk_dialog_add_action_widget (GTK_DIALOG (comment_dialog),cancel,GTK_RESPONSE_CANCEL);
	g_signal_connect (G_OBJECT (cancel),"clicked",G_CALLBACK (xa_destroy_comment_window),comment_dialog);

	close = gtk_button_new_from_stock ("gtk-ok");
	gtk_dialog_add_action_widget (GTK_DIALOG (comment_dialog),close,GTK_RESPONSE_OK);
	g_signal_connect (G_OBJECT (close),"clicked",G_CALLBACK (xa_comment_window_insert_in_archive),textbuffer);

	if (archive[idx]->comment)
	{
		utf8_line = g_locale_to_utf8 (archive[idx]->comment->str,-1,NULL,&len,NULL);
		gtk_text_buffer_insert_with_tags_by_name (textbuffer,&iter,utf8_line,len,"font",NULL);
		g_free(utf8_line);
	}
	gtk_widget_show_all(comment_dialog);
}

void xa_comment_window_insert_in_archive(GtkButton *button,gpointer data)
{
	GtkTextBuffer *buf = data;
	GtkTextIter start,end;
	FILE *stream;
	gint current_page,idx;
	gboolean result;
	gchar *command = NULL,*content,*tmp = NULL;
	GSList *list = NULL;

	current_page = gtk_notebook_get_current_page(notebook);
	idx = xa_find_archive_index (current_page);

	gtk_text_buffer_get_iter_at_offset(buf,&start,0);
	gtk_text_buffer_get_end_iter(buf,&end);
	content = gtk_text_buffer_get_text(buf,&start,&end,FALSE);

	result = xa_create_temp_directory(archive[idx]);
	tmp = g_strconcat(archive[idx]->tmp,"/xa_tmp_file",NULL);
	gtk_widget_destroy(comment_dialog);
	
	if (archive[idx]->comment == NULL)
	{
		archive[idx]->comment = g_string_new("");
		archive[idx]->has_comment = TRUE;
	}
	/* Return if the user hasn't modified the comment */
	if (strcmp(archive[idx]->comment->str,content) == 0)
		return;

	stream = fopen (tmp,"w");
	fwrite (content,1,strlen(content),stream);
	fclose (stream);

	switch (archive[idx]->type)
	{
		case XARCHIVETYPE_ARJ:
		command = g_strconcat ("arj c ",archive[idx]->escaped_path," -z",tmp,NULL);
		break;
			
		case XARCHIVETYPE_RAR:
		command = g_strconcat ("rar c ",archive[idx]->escaped_path," -z",tmp,NULL);
		break;
			
		case XARCHIVETYPE_ZIP:
		command = g_strconcat ("sh -c \"zip ",archive[idx]->escaped_path," -z <",tmp,"\"",NULL);
		break;
			
		default:
		command = NULL;
		break;
	}
	if (strlen(archive[idx]->comment->str) > 0)
		g_string_erase(archive[idx]->comment,0,strlen(archive[idx]->comment->str));
	if (strlen(content) > 0)
		g_string_append(archive[idx]->comment,content);

	if (command != NULL)
	{
		list = g_slist_append(list,command);
		xa_run_command(archive[idx],list);
	}
	g_free(tmp);
}

void xa_load_comment_window_from_file(GtkButton *button,gpointer data)
{
	GtkTextBuffer *buf = data;
	GtkTextMark *textmark;
	GtkTextIter iter;
	GtkWidget *file;
	gchar *path = NULL;
	gchar *utf8_data = NULL;
	gchar *content = NULL;
	GError *error = NULL;
	gboolean response;
	gsize bytes;

	file = gtk_file_chooser_dialog_new (_("Open a text file"),
						GTK_WINDOW (xa_main_window),
						GTK_FILE_CHOOSER_ACTION_OPEN,
						GTK_STOCK_CANCEL,
						GTK_RESPONSE_CANCEL,
						"gtk-open",
						GTK_RESPONSE_ACCEPT,
						NULL);

	response = gtk_dialog_run (GTK_DIALOG(file));
	if (response == GTK_RESPONSE_ACCEPT)
		path = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER(file));
	gtk_widget_destroy (file);
	if (path != NULL)
	{
		response = g_file_get_contents(path,&content,NULL,&error);
		if (response == FALSE)
		{
			gchar *msg = g_strdup_printf (_("Can't open file %s:"),path);
			g_free(path);
			response = xa_show_message_dialog (GTK_WINDOW (xa_main_window),GTK_DIALOG_MODAL,GTK_MESSAGE_ERROR,GTK_BUTTONS_OK,
			msg,error->message);
			g_free (msg);
			g_error_free(error);
			return;
		}
		g_free(path);
		utf8_data = g_locale_to_utf8 (content,-1,NULL,&bytes,NULL);
		g_free(content);
		textmark = gtk_text_buffer_get_insert(buf);
		gtk_text_buffer_get_iter_at_mark(buf,&iter,textmark);
		gtk_text_buffer_insert_with_tags_by_name (buf,&iter,utf8_data,bytes,"font",NULL);
		g_free (utf8_data);
	}
}

void xa_clear_comment_window(GtkButton *button,gpointer data)
{
	GtkTextBuffer *buf = data;
	GtkTextIter start,end;

	gtk_text_buffer_get_iter_at_offset(buf,&start,0);
	gtk_text_buffer_get_end_iter(buf,&end);
	gtk_text_buffer_delete(buf,&start,&end);
}

void xa_destroy_comment_window(GtkButton *button,gpointer data)
{
	gtk_widget_destroy(GTK_WIDGET(data));
}

void xa_location_entry_activated (GtkEntry *entry,gpointer user_data)
{
	XEntry *prev_entry = NULL;
	XEntry *new_entry  = NULL;
	gint current_page,idx;

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

int xa_mouse_button_event(GtkWidget *widget,GdkEventButton *event,XArchive *archive)
{
	XEntry *entry;
	GtkTreePath *path;
	GtkTreeIter  iter;
	GtkTreeSelection *selection;
	gint selected;
	GtkClipboard *clipboard;
	GtkSelectionData *clipboard_selection;
	XAClipboard *paste_data;
	gboolean value = FALSE;

	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (archive->treeview));
	selected = gtk_tree_selection_count_selected_rows(selection);
	gtk_tree_view_get_path_at_pos (GTK_TREE_VIEW (archive->treeview),event->x,event->y,&path,NULL,NULL,NULL);
	if (path == NULL)
		return FALSE;
	if ((event->type == GDK_BUTTON_PRESS) && (event->button == 3))
	{
		gtk_tree_model_get_iter (GTK_TREE_MODEL (archive->liststore),&iter,path);
		gtk_tree_path_free (path);
		gtk_tree_model_get(archive->model,&iter,archive->nc+1,&entry,-1);
		if (! gtk_tree_selection_iter_is_selected (selection,&iter))
		{
			gtk_tree_selection_unselect_all (selection);
			gtk_tree_selection_select_iter (selection,&iter);
		}
		if (selected > 1)
		{
			if (entry->is_dir)
				gtk_widget_set_sensitive(open_popupmenu,FALSE);
			else
				gtk_widget_set_sensitive(open_popupmenu,TRUE);
			gtk_widget_set_sensitive(rrename,FALSE);
			gtk_widget_set_sensitive(view,FALSE);
		}
		else
		{
			if (entry->is_dir)
			{
				gtk_widget_set_sensitive(view,FALSE);
				gtk_widget_set_sensitive(open_popupmenu,FALSE);
			}
			else
			{
				gtk_widget_set_sensitive(open_popupmenu,TRUE);
				gtk_widget_set_sensitive(view,TRUE);
			}
			gtk_widget_set_sensitive(rrename,TRUE);
			
		}
		clipboard = gtk_clipboard_get(XA_CLIPBOARD);
		clipboard_selection = gtk_clipboard_wait_for_contents(clipboard,XA_INFO_LIST);
		if (clipboard_selection != NULL)
		{
			paste_data = xa_get_paste_data_from_clipboard_selection((char*)clipboard_selection->data);
			gtk_selection_data_free (clipboard_selection);
			if (strcmp(archive->escaped_path,paste_data->cut_copy_archive->escaped_path) == 0)
				value = FALSE;
			else
				value = TRUE;
		}
		if (archive->type == XARCHIVETYPE_BZIP2 || archive->type == XARCHIVETYPE_GZIP || archive->type == XARCHIVETYPE_DEB || archive->type == XARCHIVETYPE_RPM)
		{
			gtk_widget_set_sensitive(ddelete,FALSE);
			gtk_widget_set_sensitive(rrename,FALSE);
			gtk_widget_set_sensitive(cut  ,FALSE);
			gtk_widget_set_sensitive(copy ,FALSE);
			value = FALSE;
		}
		else
			gtk_widget_set_sensitive(ddelete,TRUE);

		gtk_widget_set_sensitive(paste,value);
		gtk_menu_popup (GTK_MENU (xa_popup_menu),NULL,NULL,NULL,xa_main_window,event->button,event->time);
		return TRUE;
	}
	return FALSE;
}

XAClipboard *xa_clipboard_data_new()
{
	XAClipboard *data = NULL;

	data = g_new0(XAClipboard,1);
	
	return data;
}

void xa_clipboard_cut(GtkMenuItem* item,gpointer data)
{
	gint idx,current_page;
	
	current_page = gtk_notebook_get_current_page (notebook);
	idx = xa_find_archive_index (current_page);

	xa_clipboard_cut_copy_operation(archive[idx],XA_CLIPBOARD_CUT);
}

void xa_clipboard_copy(GtkMenuItem* item,gpointer data)
{
	gint idx,current_page;

	current_page = gtk_notebook_get_current_page (notebook);
	idx = xa_find_archive_index (current_page);
	xa_clipboard_cut_copy_operation(archive[idx],XA_CLIPBOARD_COPY);
}

void xa_clipboard_paste(GtkMenuItem* item,gpointer data)
{
	gint idx,current_page;
	GtkClipboard *clipboard;
	GtkSelectionData *selection;
	XAClipboard *paste_data;
	GSList *list = NULL;

	current_page = gtk_notebook_get_current_page(notebook);
	idx = xa_find_archive_index(current_page);

	clipboard = gtk_clipboard_get(XA_CLIPBOARD);
	selection = gtk_clipboard_wait_for_contents(clipboard,XA_INFO_LIST);
	if (selection == NULL)
		return;
	paste_data = xa_get_paste_data_from_clipboard_selection((char*)selection->data);
	gtk_selection_data_free (selection);

	/* Let's add the already extracted files in the tmp dir to the current archive dir */
	list = xa_slist_copy(paste_data->files);
	archive[idx]->working_dir = g_strdup(paste_data->cut_copy_archive->tmp);
	xa_execute_add_commands(archive[idx],list,NULL);
	if (archive[idx]->status == XA_ARCHIVESTATUS_ERROR)
		return;

	if (paste_data->mode == XA_CLIPBOARD_CUT)
	{
		paste_data->cut_copy_archive->status = XA_ARCHIVESTATUS_DELETE;
		list = xa_slist_copy(paste_data->files);
		(paste_data->cut_copy_archive->delete) (paste_data->cut_copy_archive,list);
	}
}

void xa_clipboard_cut_copy_operation(XArchive *archive,XAClipboardMode mode)
{
	GtkClipboard *clipboard;
	XAClipboard *clipboard_data = NULL;
	GSList *files = NULL;
	gchar *dummy_ex_path = NULL;
	gboolean result = FALSE;
	gboolean overwrite;
	GtkTreeSelection *selection;
	GtkTargetEntry targets[] = 
	{
		{ "application/xarchiver-info-list",0,1 }
	};

	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(archive->treeview));
	gtk_tree_selection_selected_foreach(selection,(GtkTreeSelectionForeachFunc) xa_concat_selected_filenames,&files);

	clipboard = gtk_clipboard_get (XA_CLIPBOARD);
	clipboard_data = xa_clipboard_data_new();
	if (clipboard_data == NULL)
		return;

	clipboard_data->files = xa_slist_copy(files);
	clipboard_data->mode  = mode;
	gtk_clipboard_set_with_data (clipboard,targets,G_N_ELEMENTS (targets),xa_clipboard_get,xa_clipboard_clear,(gpointer)archive);
	archive->clipboard_data = clipboard_data;
	gtk_widget_set_sensitive(paste,TRUE);

	/* Let's extract the selected files to the archive tmp dir */
	if (archive->has_passwd)
	{
		if(xa_create_password_dialog(archive) == NULL)
			return;
	}
	if (archive->extraction_path)
	{
		dummy_ex_path = g_strdup(archive->extraction_path);
		g_free(archive->extraction_path);
	}
	xa_create_temp_directory(archive);
	archive->extraction_path = g_strdup(archive->tmp);
	overwrite = archive->overwrite;
	archive->overwrite = TRUE;

	result = (*archive->extract) (archive,files);
	archive->overwrite = overwrite;
	g_free(archive->extraction_path);

	archive->extraction_path = NULL;
	if (dummy_ex_path)
	{
		archive->extraction_path = g_strdup(dummy_ex_path);
		g_free(dummy_ex_path);
	}
}

XAClipboard *xa_get_paste_data_from_clipboard_selection(const char *data)
{
	gchar **uris;
	gint i;
	XAClipboard *clipboard_data;

	clipboard_data = xa_clipboard_data_new();
	uris = g_strsplit (data,"\r\n",-1);
	clipboard_data->filename = g_strdup (uris[0]);
	clipboard_data->mode = (strcmp (uris[1],"copy") == 0) ? XA_CLIPBOARD_COPY : XA_CLIPBOARD_CUT;
	sscanf(uris[2],"%p",&clipboard_data->cut_copy_archive);
	for (i = 3; uris[i] != NULL; i++)
		if (uris[i][0] != '\0')
			clipboard_data->files = g_slist_prepend (clipboard_data->files,g_strdup (uris[i]));
	clipboard_data->files = g_slist_reverse (clipboard_data->files);
	g_strfreev(uris);
	return clipboard_data;
}

void xa_clipboard_get (GtkClipboard *clipboard,GtkSelectionData *selection_data,guint info,gpointer user_data)
{
	XArchive *archive = user_data;
	GSList *_files = archive->clipboard_data->files;
	GString *params = g_string_new("");
	if (selection_data->target != XA_INFO_LIST)
		return;

	g_string_append (params,g_strdup(archive->escaped_path));
	g_string_append (params,"\r\n");
	g_string_append (params,(archive->clipboard_data->mode == XA_CLIPBOARD_COPY) ? "copy" : "cut");
	g_string_append (params,"\r\n");
 	g_string_append_printf (params,"%p",archive);
 	g_string_append (params,"\r\n");

	while (_files)
	{
		g_string_append (params,_files->data);
		g_string_append (params,"\r\n");
		_files = _files->next;
	}
	gtk_selection_data_set (selection_data,selection_data->target,8,(guchar *) params->str,strlen(params->str));
	g_string_free (params,TRUE);
}

void xa_clipboard_clear (GtkClipboard *clipboard,gpointer data)
{
	XArchive *archive = data;

	if (archive->clipboard_data != NULL)
	{
		if (archive->clipboard_data->files != NULL)
		{
			g_slist_foreach (archive->clipboard_data->files,(GFunc) g_free,NULL);
			g_slist_free (archive->clipboard_data->files);
			archive->clipboard_data->files = NULL;
		}
		g_free (archive->clipboard_data);
		archive->clipboard_data = NULL;
	}
}

void xa_rename_archive(GtkMenuItem* item,gpointer data)
{
	gint current_index,idx;
	GtkTreeSelection *selection;
	GtkTreeViewColumn *column;
	GtkTreeModel *model;
	GList *row_list = NULL;

	current_index = gtk_notebook_get_current_page(notebook);
	idx = xa_find_archive_index(current_index);

	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (archive[idx]->treeview));
	model = gtk_tree_view_get_model(GTK_TREE_VIEW (archive[idx]->treeview));
	row_list = gtk_tree_selection_get_selected_rows(selection,&model);

	g_object_set(archive[idx]->renderer_text,"editable",TRUE,NULL);
	gtk_accel_group_disconnect_key(accel_group,GDK_Delete,GDK_MODE_DISABLED);
	column = gtk_tree_view_get_column(GTK_TREE_VIEW (archive[idx]->treeview),0);
	gtk_tree_view_set_cursor(GTK_TREE_VIEW(archive[idx]->treeview),row_list->data,column,TRUE);
	gtk_tree_path_free (row_list->data);
	g_list_free(row_list);
}

void xa_rename_cell_edited_canceled(GtkCellRenderer *renderer,gpointer data)
{
	g_object_set(renderer,"editable",FALSE,NULL);
	gtk_widget_add_accelerator (delete_menu,"activate",accel_group,GDK_Delete,GDK_MODE_DISABLED,GTK_ACCEL_VISIBLE);
}

void xa_rename_cell_edited (GtkCellRendererText *cell,const gchar *path_string,const gchar *new_name,XArchive *archive)
{
	GtkTreeIter iter;
	GtkTreeModel *model;
	XEntry *entry;
	gchar *old_name,*_old_name,*_new_name,*dummy = NULL;
	GSList *names = NULL,*list = NULL;
	gboolean result = FALSE,full_path,overwrite;

	model = gtk_tree_view_get_model(GTK_TREE_VIEW(archive->treeview));
	if (gtk_tree_model_get_iter_from_string(model,&iter,path_string))
	{
		gtk_tree_model_get(model,&iter,archive->nc+1,&entry,-1);
		if (entry->is_encrypted)
		{
			archive->passwd = xa_create_password_dialog(archive);
			if (archive->passwd == NULL)
				return;
		}
		/* Extract the file to the tmp dir */
		if (archive->extraction_path)
		{
			dummy = g_strdup(archive->extraction_path);
			g_free(archive->extraction_path);
		}
		xa_create_temp_directory(archive);
		archive->extraction_path = g_strdup(archive->tmp);
		old_name  = xa_build_full_path_name_from_entry(entry,archive);
		_old_name = xa_escape_filename(old_name,"$'`\"\\!?* ()[]&|:;<>#");
		names = g_slist_append(names,old_name);

		full_path = archive->full_path;
		overwrite = archive->overwrite;
		archive->overwrite = archive->full_path = TRUE;
		result = (*archive->extract) (archive,names);

		archive->overwrite = full_path;
		archive->full_path = overwrite;
		g_free(archive->extraction_path);
		archive->extraction_path = NULL;
		if (dummy)
		{
			archive->extraction_path = g_strdup(dummy);
			g_free(dummy);
		}
		if (result == FALSE)
		{
			g_free(_old_name);
			return;
		}
		/* Rename the file in the tmp dir as the new file entered by the user */
		_new_name = xa_escape_filename((gchar*)new_name,"$'`\"\\!?* ()[]&|:;<>#");
		dummy = g_strconcat("mv -f ",archive->tmp,"/",_old_name," ",archive->tmp,"/",_new_name,NULL);
		g_free(_old_name);
		list = g_slist_append(list,dummy);
		xa_run_command(archive,list);
		list = NULL;

		/* Delete the selected file from the archive */
		archive->status = XA_ARCHIVESTATUS_DELETE;
		old_name = xa_build_full_path_name_from_entry(entry,archive);
		list = g_slist_append(list,old_name);
		archive->status = XA_ARCHIVESTATUS_RENAME;
		(archive->delete) (archive,list);
		list = NULL;

		/* Add the renamed file to the archive */
		old_name = xa_build_full_path_name_from_entry(entry,archive);
		_new_name = g_strdup(new_name);
		list = g_slist_append(list,_new_name);
		chdir (archive->tmp);
		xa_execute_add_commands(archive,list,NULL);
	}
	gtk_widget_add_accelerator (delete_menu,"activate",accel_group,GDK_Delete,GDK_MODE_DISABLED,GTK_ACCEL_VISIBLE);
	g_object_set(cell,"editable",FALSE,NULL);
}

void xa_open_with_from_popupmenu(GtkMenuItem *item,gpointer data)
{
	gboolean result		= FALSE;
	gboolean overwrite  = FALSE;
	gint current_index,idx,nr;
	GtkTreeSelection *selection;
	GtkTreeIter iter;
	GList *row_list = NULL;
	GSList *list = NULL;
	GSList *list_of_files = NULL;
	GString *names = g_string_new("");
	gchar *dummy = NULL,*e_filename = NULL;
	XEntry *entry;

	current_index = gtk_notebook_get_current_page(notebook);
	idx = xa_find_archive_index(current_index);

	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (archive[idx]->treeview));
	row_list = gtk_tree_selection_get_selected_rows(selection,&archive[idx]->model);
	if (row_list == NULL)
		return;

	nr = gtk_tree_selection_count_selected_rows(selection);
	while (row_list)
	{
		gtk_tree_model_get_iter(archive[idx]->model,&iter,row_list->data);
		gtk_tree_model_get(archive[idx]->model,&iter,archive[idx]->nc+1,&entry,-1);
		gtk_tree_path_free(row_list->data);
		if (entry->is_encrypted)
		{
			if (archive[idx]->passwd == NULL)
			{
				archive[idx]->passwd = xa_create_password_dialog(archive[idx]);
				if (archive[idx]->passwd == NULL)
					return;
			}
		}
		list = g_slist_append(list,xa_build_full_path_name_from_entry(entry,archive[idx]));
		row_list = row_list->next;
	}
	g_list_free (row_list);
	if (archive[idx]->extraction_path)
	{
		dummy = g_strdup(archive[idx]->extraction_path);
		g_free(archive[idx]->extraction_path);
	}
	xa_create_temp_directory(archive[idx]);
	archive[idx]->extraction_path = g_strdup(archive[idx]->tmp);

	overwrite = archive[idx]->overwrite;
	archive[idx]->overwrite = TRUE;
	list_of_files = xa_slist_copy(list);

	result = (*archive[idx]->extract) (archive[idx],list);
	archive[idx]->overwrite = overwrite;
	g_free(archive[idx]->extraction_path);
	archive[idx]->extraction_path = NULL;
	if (dummy)
	{
		archive[idx]->extraction_path = g_strdup(dummy);
		g_free(dummy);
	}
	if (result == FALSE)
		return;

	chdir(archive[idx]->tmp);
	do
	{
		dummy = g_path_get_basename(list_of_files->data);
		e_filename = xa_escape_filename(dummy,"$'`\"\\!?* ()[]&|:;<>#");
		g_free(dummy);
		g_string_append (names,e_filename);
		g_string_append_c (names,' ');
		list_of_files = list_of_files->next;
	}
	while (list_of_files);
	xa_create_open_with_dialog(entry->filename,names->str,nr);
	g_slist_foreach(list_of_files,(GFunc)g_free,NULL);
	g_slist_free(list_of_files);	
}

void xa_view_from_popupmenu(GtkMenuItem *item,gpointer data)
{
	GtkTreeSelection *selection;
	GtkTreeIter iter;
	GSList *list = NULL;
	GList *row_list = NULL;
	gboolean result		= FALSE;
	gboolean full_path  = FALSE;
	gboolean overwrite  = FALSE;
	gint current_index,idx;
	gchar *dummy = NULL,*filename = NULL,*e_filename = NULL;
	XEntry *entry;

	current_index = gtk_notebook_get_current_page(notebook);
	idx = xa_find_archive_index(current_index);

	xa_create_temp_directory(archive[idx]);

	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (archive[idx]->treeview));
	row_list = gtk_tree_selection_get_selected_rows(selection,&archive[idx]->model);
	gtk_tree_model_get_iter(archive[idx]->model,&iter,row_list->data);
	gtk_tree_model_get(archive[idx]->model,&iter,archive[idx]->nc+1,&entry,-1);
	gtk_tree_path_free(row_list->data);
	list = g_slist_append(list,xa_build_full_path_name_from_entry(entry,archive[idx]));
	g_list_free(row_list);

	if (entry->is_encrypted)
	{
		archive[idx]->passwd = xa_create_password_dialog(archive[idx]);
		if (archive[idx]->passwd == NULL)
			return;
	}
	filename = g_strconcat(archive[idx]->tmp,"/",entry->filename,NULL);
	if (g_file_test(filename,G_FILE_TEST_EXISTS))
		goto here;
	if (archive[idx]->extraction_path)
	{
		dummy = g_strdup(archive[idx]->extraction_path);
		g_free(archive[idx]->extraction_path);
	}
	archive[idx]->extraction_path = g_strdup(archive[idx]->tmp);
	overwrite = archive[idx]->overwrite;
	full_path = archive[idx]->full_path;
	archive[idx]->full_path = FALSE;
	archive[idx]->overwrite = TRUE;

	result = (*archive[idx]->extract) (archive[idx],list);
	archive[idx]->full_path = full_path;
	archive[idx]->overwrite = overwrite;
	g_free(archive[idx]->extraction_path);
	archive[idx]->extraction_path = NULL;
	if (dummy)
	{
		archive[idx]->extraction_path = g_strdup(dummy);
		g_free(dummy);
	}
	if (result == FALSE)
		return;

here:
	e_filename = xa_escape_filename(filename,"$'`\"\\!?* ()[]&|:;<>#");
	g_free(filename);

	xa_determine_program_to_run(e_filename);
	g_free(e_filename);
}

void xa_treeview_row_activated(GtkTreeView *tree_view,GtkTreePath *path,GtkTreeViewColumn *column,XArchive *archive)
{
	XEntry *entry;
	GtkTreeIter iter;
	gchar *dummy = NULL,*item,*file = NULL,*e_filename = NULL;
	GSList *names = NULL;
	gboolean result = FALSE;
	gboolean overwrite = FALSE;

	if (! gtk_tree_model_get_iter (GTK_TREE_MODEL (archive->liststore),&iter,path))
		return;

	gtk_tree_model_get (GTK_TREE_MODEL (archive->liststore),&iter,archive->nc+1,&entry,-1);
	if (entry->is_dir)
	{
		if (archive->location_entry_path != NULL)
			archive->back = g_slist_prepend(archive->back,xa_find_entry_from_path(archive->root_entry,archive->location_entry_path));
		/* Put NULL so to display the root entry */
		else
			archive->back = g_slist_prepend(archive->back,NULL);
		xa_sidepane_select_row(entry);
	}
	/* The selected entry it's not a dir so extract it to the tmp dir and send it to xa_determine_program_to_run() */
	else
	{
	   	if (archive->extraction_path)
	   	{
	   		dummy = g_strdup(archive->extraction_path);
	   		g_free(archive->extraction_path);
	   	}
	   	xa_create_temp_directory(archive);
	   	archive->extraction_path = g_strdup(archive->tmp);
	   	item = xa_build_full_path_name_from_entry(entry,archive);
	   	names = g_slist_append(names,item);
	   	overwrite = archive->overwrite;
	   	archive->overwrite = TRUE;
		result = (*archive->extract) (archive,names);
		archive->overwrite = overwrite;

		g_free(archive->extraction_path);
		archive->extraction_path = NULL;
		if (dummy)
		{
			archive->extraction_path = g_strdup(dummy);
			g_free(dummy);
		}
		if (result == FALSE)
			return;
		file = g_strconcat(archive->tmp,"/",entry->filename,NULL);
		e_filename = xa_escape_filename(file,"$'`\"\\!?* ()[]&|:;<>#");
		g_free(file);
		xa_determine_program_to_run(e_filename);
		g_free(e_filename);
	}
}

void xa_update_window_with_archive_entries (XArchive *archive,XEntry *entry)
{
	GdkPixbuf *pixbuf = NULL;
	GtkTreeIter iter;
	unsigned short int i;
	gpointer current_column;
	gchar *filename;
	gint size;

	if ( (archive->status == XA_ARCHIVESTATUS_ADD || archive->status == XA_ARCHIVESTATUS_DELETE) && archive->location_entry_path != NULL)
	{
		archive->status = XA_ARCHIVESTATUS_IDLE;
		entry = xa_find_entry_from_path(archive->root_entry,archive->location_entry_path);
	}
	else
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
		if (archive->back == NULL)
			gtk_widget_set_sensitive(back_button,FALSE);
		else
			gtk_widget_set_sensitive(back_button,TRUE);

		gtk_widget_set_sensitive(up_button,TRUE);
		gtk_widget_set_sensitive(home_button,TRUE);
		archive->location_entry_path = xa_build_full_path_name_from_entry(entry,archive);
		gtk_entry_set_text(GTK_ENTRY(location_entry),archive->location_entry_path);
		entry = entry->child;
	}
	gtk_list_store_clear(archive->liststore);

	while (entry)
	{
		current_column = entry->columns;
		gtk_list_store_append (archive->liststore,&iter);
		if(!g_utf8_validate(entry->filename,-1,NULL))
		{
			gchar *dummy = g_convert(entry->filename,-1,"UTF-8","WINDOWS-1252",NULL,NULL,NULL);
			if (dummy != NULL)
			{
				g_free (entry->filename);
				entry->filename = dummy;
			}
		}
		if (entry->is_dir)
			filename = "folder";
		else if (entry->is_encrypted)
		{
			filename = "lock";
			archive->has_passwd = TRUE;
		}
		else
			filename = entry->filename;

		if (gtk_combo_box_get_active (GTK_COMBO_BOX(prefs_window->combo_icon_size)) == 0)
			size = 30;
		else
			size = 20;
		pixbuf = xa_get_pixbuf_icon_from_cache(filename,size);
		gtk_list_store_set (archive->liststore,&iter,archive->nc+1,entry,-1);
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
	xa_set_statusbar_message_for_displayed_rows(archive);
}

void xa_show_multi_extract_dialog (GtkMenuItem *menu_item,gpointer data)
{
	xa_parse_multi_extract_archive(multi_extract_window);
}
