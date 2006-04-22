#include <config.h>
#include <gtk/gtk.h>
#include <libintl.h>

#include "extract-dialog.h"

#define _(String) gettext(String)

static void
xa_extract_dialog_init(XAExtractDialog *object);

static void
xa_extract_dialog_finalize(GObject *object);

static void
xa_extract_dialog_class_init(XAExtractDialogClass *klass);

static GtkWidgetClass *xa_extract_dialog_parent_class = 0;

GType
xa_extract_dialog_get_type ()
{
	static GtkType xa_extract_dialog_type = 0;

 	if (!xa_extract_dialog_type)
	{
 		static const GTypeInfo xa_extract_dialog_info = 
		{
			sizeof (XAExtractDialogClass),
			(GBaseInitFunc) NULL,
			(GBaseFinalizeFunc) NULL,
			(GClassInitFunc) xa_extract_dialog_class_init,
			(GClassFinalizeFunc) NULL,
			NULL,
			sizeof (XAExtractDialog),
			0,
			(GInstanceInitFunc) xa_extract_dialog_init,
			NULL
		};

		xa_extract_dialog_type = g_type_register_static (GTK_TYPE_DIALOG, "XAExtractDialog", &xa_extract_dialog_info, 0);
	}
	return xa_extract_dialog_type;
}

static void
xa_extract_dialog_class_init(XAExtractDialogClass *_class)
{
	GtkObjectClass *object_class = (GtkObjectClass *)_class;
	GtkWidgetClass *widget_class = (GtkWidgetClass *)_class;

	G_OBJECT_CLASS(object_class)->finalize = xa_extract_dialog_finalize;

	xa_extract_dialog_parent_class = gtk_type_class(gtk_dialog_get_type());
}

static void
xa_extract_dialog_init(XAExtractDialog *object)
{
	
}

static void
xa_extract_dialog_finalize(GObject *object)
{

}

GtkWidget *
xa_extract_dialog_new(GtkWindow *parent)
{
	GtkWidget *dialog;
	dialog = GTK_WIDGET (g_object_new(xa_extract_dialog_get_type(), NULL));
	gtk_window_set_transient_for(GTK_WINDOW(dialog), parent);
	gtk_window_set_destroy_with_parent(GTK_WINDOW(dialog), TRUE);
	gtk_window_set_title(GTK_WINDOW(dialog), _("Extract to"));
	gtk_widget_set_size_request(dialog, 400, 300);
	gtk_window_set_resizable(GTK_WINDOW(dialog), FALSE);
	return dialog;
}
