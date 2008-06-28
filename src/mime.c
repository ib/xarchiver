/*
 *  Copyright (c) 2007 Giuseppe Torelli <colossus73@gmail.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 */
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "glib-mem.h"	/* provide compatibility macros for g_slice */
#include "mime.h"
#include "mime-type/mime-type.h"

typedef struct _XAMimeType
{
	char* name;
	char* icon_name;
	char* desc;
	GdkPixbuf* icon;
}XAMimeType;

static GHashTable *mime_cache = NULL;
static GdkPixbuf *folder_icon = NULL;
static GdkPixbuf *lock_icon = NULL;
	
static void xa_mime_type_free( XAMimeType* type )
{
	g_free( type->name );
	g_free( type->icon_name );
	g_free( type->desc );
	if( type->icon )
		g_object_unref( type->icon );
	g_slice_free( XAMimeType, type );
}

void xa_mime_type_init()
{
#ifdef USE_MIMETYPE
/*
    GtkIconTheme * theme;
    MimeCache** caches;
    int i, n_caches;
*/
    mime_type_init();

#if 0
    /* install file alteration monitor for mime-cache */
    caches = mime_type_get_caches( &n_caches );
    mime_caches_monitor = g_new0( VFSFileMonitor*, n_caches );
    for( i = 0; i < n_caches; ++i )
    {
        VFSFileMonitor* fm = vfs_file_monitor_add_file( caches[i]->file_path,
                                                                on_mime_cache_changed, caches[i] );
        mime_caches_monitor[i] = fm;
    }
    theme = gtk_icon_theme_get_default();
    theme_change_notify = g_signal_connect( theme, "changed",
                                            G_CALLBACK( on_icon_theme_changed ),
                                            NULL );
#endif

#endif

    mime_cache = g_hash_table_new_full( g_str_hash, g_str_equal,
                                       NULL, (GDestroyNotify)xa_mime_type_free );

}

static XAMimeType* lookup_mime_type( const char* name )
{
    XAMimeType* type;
    type = (XAMimeType*)g_hash_table_lookup( mime_cache, name );
    if( G_UNLIKELY( ! type ) )
    {
    	type = g_slice_new0( XAMimeType );
    	type->name = g_strdup( name );
    	g_hash_table_insert( mime_cache, type->name, type);
    }
	return type;
}

const char *xa_get_stock_mime_icon(const char *filename, const char *mime)
{
	const char *icon_name = NULL;

	//g_print ("%s\t%s\n",filename,mime);
	if (strstr(filename,".ogg") || strstr(filename,".flac") )
		icon_name = "sound";
	else if( NULL == mime )
		return NULL;

	if (strncmp(mime,"image/",6) == 0)
		icon_name = "image";
	else if (strcmp(mime,"text/html") == 0)
		icon_name = "html";
	else if (strncmp(mime,"text/",5) == 0)
		icon_name = "txt";
	else if (strcmp(mime,"application/rtf") == 0 || strcmp(mime,"application/pdf") == 0 || strcmp(mime,"application/msword") == 0
		|| strcmp (mime,"application/vnd.oasis.opendocument.text") == 0)
		icon_name = "document";
	else if (strcmp(mime,"audio/mpeg") == 0 || strcmp(mime,"audio/midi") == 0 )
		icon_name = "sound";
	else if (strcmp(mime,"application/vnd.ms-excel") == 0 || strcmp(mime,"application/vnd.oasis.opendocument.spreadsheet") == 0)
		icon_name = "gnome-mime-application-vnd.ms-excel";
	else if (strcmp(mime,"application/vnd.ms-powerpoint") == 0 || strcmp (mime,"application/vnd.oasis.opendocument.presentation") == 0)
		icon_name = "gnome-mime-application-vnd.ms-powerpoint";
	else if (strcmp(mime,"application/zip") == 0 || strcmp(mime,"application/x-rar") == 0 || strcmp(mime,"application/x-tar") == 0
		|| strcmp(mime,"application/x-7z-compressed") == 0 || strcmp(mime,"application/x-bzip-compressed-tar") == 0
		|| strcmp (mime,"application/x-compressed-tar") == 0 || strcmp (mime,"application/x-lha") == 0
		|| strcmp (mime,"application/x-rpm") == 0 || strcmp (mime,"application/x-deb") == 0
		|| strcmp (mime,"application/x-bzip") == 0  || strcmp (mime,"application/x-gzip") == 0)
		icon_name = "package";
	else if (strcmp(mime,"application/x-shockwave-flash") == 0 || strcmp(mime,"video/mpeg") == 0 || strcmp(mime,"video/quicktime") == 0
		|| strcmp(mime,"video/x-msvideo") == 0 || strcmp(mime,"application/x-flash-video") == 0)
		icon_name = "video";
	else if (strcmp(mime,"application/x-cd-image") == 0)
		icon_name = "application-x-cd-image";
	else if (strcmp(mime,"application/x-php") == 0)
		icon_name = "gnome-mime-application-x-php";
	else if (strcmp(mime,"application/x-perl") == 0 || strcmp (mime,"application/x-csh") == 0 || strcmp (mime,"application/x-shellscript") == 0)
		icon_name = "gnome-mime-application-x-perl";
	else if (strcmp(mime,"application/x-font-ttf") == 0)
		icon_name = "gnome-mime-application-x-font-ttf";
	return icon_name;
}

GdkPixbuf *xa_get_pixbuf_icon_from_cache(const gchar *filename)
{
    char icon_name[ 100 ];
	const char* mime_type;
	XAMimeType* mime = NULL;

	if (strcmp(filename,"folder") == 0)
	{
		if( G_LIKELY( folder_icon ) )
			return (GdkPixbuf*)g_object_ref( folder_icon );
		folder_icon = gtk_icon_theme_load_icon(icon_theme,"folder", 20, 0, NULL);
		if( G_UNLIKELY( ! folder_icon ) )
			folder_icon = gtk_icon_theme_load_icon(icon_theme,"gnome-fs-directory", 20, 0, NULL);
		return folder_icon ? (GdkPixbuf*)g_object_ref( folder_icon ) : NULL;
	}
	else if (strcmp(filename,"lock") == 0)
	{
		if( G_LIKELY( lock_icon ) )
			return (GdkPixbuf*)g_object_ref( lock_icon );
		lock_icon = gtk_icon_theme_load_icon(icon_theme,GTK_STOCK_DIALOG_AUTHENTICATION, 20, 0,NULL);
		return lock_icon ? (GdkPixbuf*)g_object_ref( lock_icon ) : NULL;
	}
	else
	{
		char* sep;
		GdkPixbuf* icon = NULL;
#ifdef USE_MIMETYPE
		mime_type = mime_type_get_by_filename(filename, NULL);
#else
		mime_type = xdg_mime_get_mime_type_from_file_name(filename);
#endif

		if( mime_type )
		{
			mime = lookup_mime_type( mime_type );

			if( mime->icon )
				return (GdkPixbuf*)g_object_ref( mime->icon );

			sep = strchr( mime->name, '/' );
			if ( sep )
			{
				strcpy( icon_name, mime->name );
				icon_name[ (sep - mime->name) ] = '-';
				icon = gtk_icon_theme_load_icon ( icon_theme, icon_name, 20, 0, NULL );
				if ( ! icon )
				{
					strcpy( icon_name, "gnome-mime-" );
					strncat( icon_name, mime->name, ( sep - mime->name ) );
					strcat( icon_name, "-" );
					strcat( icon_name, sep + 1 );
					icon = gtk_icon_theme_load_icon ( icon_theme, icon_name, 20, 0, NULL );
				}
				if ( G_UNLIKELY( ! icon ) )
				{
					icon_name[ 11 ] = 0;
					strncat( icon_name, mime->name, ( sep - mime->name ) );
					icon = gtk_icon_theme_load_icon ( icon_theme, icon_name, 20, 0, NULL );
				}
			}
		}

		if( G_UNLIKELY( !icon ) )
		{
			const char* fallback = xa_get_stock_mime_icon(filename, mime_type);
			if( fallback )
				icon = gtk_icon_theme_load_icon ( icon_theme, fallback, 20, 0, NULL );			
		}
		if( G_UNLIKELY( !icon ) )
		{
			/* prevent endless recursion of XDG_MIME_TYPE_UNKNOWN */
			if( G_LIKELY( strcmp(mime->name, XDG_MIME_TYPE_UNKNOWN) ) )
			{
				/* FIXME: fallback to icon of parent mime-type */
				icon = xa_get_pixbuf_icon_from_cache( XDG_MIME_TYPE_UNKNOWN );
			}
			else /* unknown */
				icon = gtk_icon_theme_load_icon ( icon_theme, "unknown", 20, 0, NULL );
		}

		mime->icon = icon;
		return (GdkPixbuf*)g_object_ref( icon );
	}
	return NULL;
}

void xa_free_icon_cache()
{
	g_hash_table_destroy( mime_cache );
}
