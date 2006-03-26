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
#include <errno.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <string.h>
#include "zip.h"
#include "rar.h"
#include "bzip2.h"
#include "gzip.h"

enum {inactive,add,delete} action;
int response , CurrentArchiveType;
int input_fd , output_fd,error_fd,child_pid,num_cols, x;
gboolean archive_error , done , bz_gz , PasswordProtectedArchive;
gulong compressor_pid;
GtkWidget *dialog , *textview, *scrollwin, *vbox, *OutputWindow , *File_Selector , *extract_window, *pwd_window, *password_entry , *repeat_password;
GtkTextBuffer *textbuf;
GtkTextIter enditer , start, end;
GtkListStore *liststore;
GtkTreeIter iter;
GtkCellRenderer *renderer;
GtkTreeViewColumn *column;
gchar *path , *title , *password;
const gchar *extract_path;
GString *names;

struct File_Chooser_Data
{
    GtkListStore *ls;
    GtkTreeView *tv;
    GtkFileChooser *fc;
};

void on_new1_activate (GtkMenuItem *menuitem, gpointer user_data);
void on_open1_activate (GtkMenuItem *menuitem, gpointer user_data );
void on_quit1_activate (GtkMenuItem *menuitem, gpointer user_data);
void on_delete1_activate (GtkMenuItem *menuitem, gpointer user_data);
void on_about1_activate (GtkMenuItem *menuitem, gpointer user_data);
void on_options1_activate (GtkMenuItem *menuitem, gpointer user_data);
void on_extract1_activate ( GtkMenuItem *menuitem, gpointer user_data);
void Show_pwd_Window ( GtkMenuItem *menuitem, gpointer user_data);
void on_add_files_activate ( GtkMenuItem *menuitem, gpointer user_data );
void on_New_button_clicked (GtkToolButton   *toolbutton, gpointer user_data);
void Activate_delete_button();

char *Show_File_Dialog (int dummy , gpointer title);
GSList *Add_File_Dialog ( gchar *mode );
GIOChannel *SetIOChannel (gint fd, GIOCondition cond, GIOFunc func, gpointer data);
GIOChannel *SetIOChannelEncondingNULL (gint fd, GIOCondition cond, GIOFunc func, gpointer data);
int ShowGtkMessageDialog ( GtkWindow *window, int mode,int type,int button, gchar *message);
gulong SpawnAsyncProcess (const gchar *command , gboolean ExitStatusFlag , gboolean input );
int DetectArchiveType ( gchar *path );
void RemoveColumnsListStore ();
void EmptyTextBuffer();
void CreateListStore ( int nc, gchar *columns_names[] , GType columns_types[]);
void ShowShellOutput ();
GChildWatchFunc *ExitStatus (GPid pid,gint status,gpointer tmp);
gboolean isTar ( FILE *ptr );
void ConcatenateFileNames (GtkTreeModel *model, GtkTreePath *treepath, GtkTreeIter *iter, GString *data);
void ConcatenateFileNames2 (gchar *filename , GString *data);
void ExtractAddDelete ( gchar *command );
gboolean GenOutput (GIOChannel *ioc, GIOCondition cond, gpointer data);
gboolean GenError (GIOChannel *ioc, GIOCondition cond, gpointer data);
gboolean DetectPasswordProtectedArchive ( FILE *dummy_ptr );

gchar *EscapeBadChars ( gchar *path );
int is_escaped_char(char c);
gchar *StripPathFromFilename ( gchar *name );
gchar *JoinPathArchiveName ( const gchar * , gchar * );
char *eat_spaces (char *line);
char *get_last_field (char *line,int last_field);
char **split_line (char *line,int n_fields);
#endif

