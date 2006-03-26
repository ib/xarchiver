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

#ifndef CALLBACK_H
#define CALLBACK_H

#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <errno.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <time.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include "arj.h"
#include "iso.h"
#include "zip.h"
#include "rar.h"
#include "tar.h"
#include "7zip.h"
#include "bzip2.h"
#include "gzip.h"

enum {inactive,add,delete,extract,test} action;
short int response, CurrentArchiveType;
int input_fd, output_fd, error_fd, child_pid, x, number_of_files, number_of_dirs;
unsigned long long int dummy_size;
double content_size;
gboolean archive_error , done , bz_gz , PasswordProtectedArchive;
gulong compressor_pid;
GtkWidget *dialog , *textview, *textview1, *scrollwin, *vbox, *OutputWindow , *File_Selector , *extract_window, *pwd_window, *password_entry , *repeat_password, *view_window,*archive_properties_win;
GtkTextBuffer *textbuf , *viewtextbuf;
GtkTextIter enditer , start, end;
GtkTextIter viewenditer, viewstart, viewend;
GtkListStore *liststore;
GtkTreeIter iter;
GtkCellRenderer *renderer;
GtkTreeViewColumn *column;
gchar *removed_bs_path, *escaped_path , *path , *title , *password, *ComboArchiveType, *extract_path , *ext , *es_path;
GtkTreeModel *model;
GString *names;
GSList *Files_to_Add;

struct File_Chooser_Data
{
    GtkListStore *ls;
    GtkTreeView *tv;
    GtkFileChooser *fc;
};

void on_new1_activate (GtkMenuItem *menuitem, gpointer user_data);
void on_open1_activate (GtkMenuItem *menuitem, gpointer user_data );
void on_check_activate (GtkMenuItem *menuitem, gpointer user_data);
void on_quit1_activate (GtkMenuItem *menuitem, gpointer user_data);
void on_delete1_activate (GtkMenuItem *menuitem, gpointer user_data);
void on_about1_activate (GtkMenuItem *menuitem, gpointer user_data);
void on_options1_activate (GtkMenuItem *menuitem, gpointer user_data);
void on_extract1_activate ( GtkMenuItem *menuitem, gpointer user_data);
void ShowShellOutput ( GtkMenuItem *menuitem, gboolean iso_title);
void Show_pwd_Window ( GtkMenuItem *menuitem, gpointer user_data);
void ShowArchiveProperties ( GtkMenuItem *menuitem , gpointer user_data );
void View_File_Window ( GtkMenuItem *menuitem , gpointer user_data);
void Cancel_Operation ( GtkMenuItem *menuitem , gpointer user_data);
gboolean key_press_function ( GtkWidget* widget, GdkEventKey* event,gpointer data);
gboolean treeview_select_search (GtkTreeModel *model,gint column,const gchar *key,GtkTreeIter *iter,gpointer search_data);
void on_add_files_activate ( GtkMenuItem *menuitem, gpointer user_data );
void on_New_button_clicked (GtkToolButton   *toolbutton, gpointer user_data);
void Activate_buttons();
void on_drag_data_received (GtkWidget *widget,GdkDragContext *context, int x,int y,GtkSelectionData *data, unsigned int info, unsigned int time, gpointer user_data);

void drag_begin (GtkWidget *treeview1,GdkDragContext *context, gpointer data);
void drag_end (GtkWidget *treeview1, GdkDragContext *context, gpointer data);
void drag_data_get (GtkWidget *widget, GdkDragContext *dc, GtkSelectionData *selection_data, guint info, guint t, gpointer data);

char *Show_File_Dialog (int dummy , gpointer title);
GSList *Add_File_Dialog ( gchar *mode );
GIOChannel *SetIOChannel (gint fd, GIOCondition cond, GIOFunc func, gpointer data);
GIOChannel *SetIOChannelEncondingNULL (gint fd, GIOCondition cond, GIOFunc func, gpointer data);
int ShowGtkMessageDialog ( GtkWindow *window, int mode,int type,int button, gchar *message);
gulong SpawnAsyncProcess (const gchar *command , gboolean ExitStatusFlag , gboolean input );
int DetectArchiveType ( gchar *path );
void RemoveColumnsListStore ();
void EmptyTextBuffer ();
void CreateListStore ( int nc, gchar *columns_names[] , GType columns_types[]);
GChildWatchFunc *ViewFileFromArchive (GPid pid , gint status , GString *data);
gboolean isTar ( FILE *ptr );
gboolean isISO ( FILE *ptr );
void ConcatenateFileNames (GtkTreeModel *model, GtkTreePath *treepath, GtkTreeIter *iter, GString *data);
void ConcatenateFileNames2 (gchar *filename , GString *data);
void ExtractAddDelete ( gchar *command );
gboolean GenOutput (GIOChannel *ioc, GIOCondition cond, gpointer data);
gboolean GenError (GIOChannel *ioc, GIOCondition cond, gpointer data);
gboolean DetectPasswordProtectedArchive ( int type , FILE *dummy_ptr , unsigned char magic[6]);
gchar *EscapeBadChars ( gchar *path );
int is_escaped_char(char c);
gchar *StripPathFromFilename ( gchar *name );
gchar *JoinPathArchiveName ( const gchar * , gchar * );
char *eat_spaces (char *line);
char *get_last_field (char *line,int last_field);
char **split_line (char *line,int n_fields);
void OffDeleteandViewButtons();
void OffTooltipPadlock();
gchar *ChooseCommandtoExecute ( gboolean full_path , GString *files);
int CountCharacter ( gchar *string , int chr );
gchar *remove_level_from_path (const gchar *path);
gchar *extract_local_path (gchar *path , gchar *filename);
gchar *RemoveBackSlashes ( gchar *name);
void Update_StatusBar (gchar *msg);
void WaitExitStatus ( GPid child_pid , gchar *temp_file );
#endif

