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

#ifndef __PREF_DIALOG_H
#define __PREF_DIALOG_H

#include "archive.h"

typedef struct
{
	GtkWidget *dialog1;
	GtkWidget *dialog_vbox1;
	GtkNotebook *prefs_notebook;
	GtkWidget *label1, *label2, *label3, *label4, *vbox1, *vbox2, *vbox3, *vbox4, *frame1, *frame2, *frame3;
	GtkWidget *hbox1, *hbox2, *alignment1, *combo_box, *label5, *check_save_add_dialog, *check_save_extract_dialog;
	GtkWidget *allow_dir_extract_with_dnd;
	GtkTooltips *tooltip_dnd;
} Prefs_dialog_data;

Prefs_dialog_data *xa_create_prefs_dialog ();

#endif

