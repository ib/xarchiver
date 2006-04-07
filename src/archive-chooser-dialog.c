#include <gtk/gtk.h>

#include "archive-chooser-dialog.h"

static void
xa_archive_chooser_dialog_init(XAArchiveChooserDialog *object);

static void
xa_archive_chooser_dialog_finalize(XAArchiveChooserDialog *object);

static void
xa_archive_chooser_dialog_class_init(XAArchiveChooserDialogClass *_class);

GType
xa_archive_chooser_dialog_get_type ()
{
	static GtkType xa_archive_chooser_dialog_type = 0;

 	if (!xa_archive_chooser_dialog_type)
	{
 		static const GTypeInfo xa_archive_chooser_dialog_info = 
		{
			sizeof (XAArchiveChooserDialogClass),
			(GBaseInitFunc) xa_archive_chooser_dialog_class_init,
			(GBaseFinalizeFunc) xa_archive_chooser_dialog_finalize,
			(GClassInitFunc) xa_archive_chooser_dialog_class_init,
			(GClassFinalizeFunc) NULL,
			NULL,
			sizeof (XAArchiveChooserDialog),
			0,
			(GInstanceInitFunc) xa_archive_chooser_dialog_init,
			NULL
		};

		xa_archive_chooser_dialog_type = g_type_register_static (GTK_TYPE_DIALOG, "XAArchiveChooserDialog", &xa_archive_chooser_dialog_info, 0);
	}
	return xa_archive_chooser_dialog_type;
}

static void
xa_archive_chooser_dialog_class_init(XAArchiveChooserDialogClass *_class)
{

}

static void
xa_archive_chooser_dialog_init(XAArchiveChooserDialog *object)
{
	
}

static void
xa_archive_chooser_dialog_finalize(XAArchiveChooserDialog *object)
{

}

GtkWidget *
xa_archive_chooser_dialog_new(gchar *title, GtkWindow *parent)
{
	GtkWidget *dialog;
	dialog = GTK_WIDGET (g_object_new(xa_archive_chooser_dialog_get_type(), NULL));
	gtk_window_set_transient_for(GTK_WINDOW(dialog), parent);
	gtk_window_set_title(GTK_WINDOW(dialog), title);
	return dialog;
}
