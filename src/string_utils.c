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

#include <gtk/gtk.h>
#include <glib.h>
#include <string.h>
#include "string_utils.h"

//Taken from xarchive - http://xarchive.sourceforge.net
int is_escaped_char (char c)
{
    switch ( c )
    {
        case ' ':
        case '\'':
        case '"':
        case '(':
        case ')':
        case '$':
        case '\\':
        case ';':
        case '<':
        case '>':
        case '&':
        case '#':
        case '*':
        case '|':
        case '`':
        case '!':
        return 1;
        default:
        return 0;
    }
}

gchar *EscapeBadChars ( gchar *string , gboolean doublesquare)
{
	char *q;
	char *escaped;
	int escapechars = 0;
	char *p = string;

	while (*p != '\000')
	{
        if (is_escaped_char(*p))
			escapechars++;
		/* The following is mine */
		else if ( doublesquare && (*p == '[' || *p == ']') )
			escapechars += 2;
		p++;
    }

	if (!escapechars)
		return g_strdup(string);
	escaped = (char *) g_malloc (strlen(string) + escapechars + 1);

	p = string;
	q = escaped;

	while (*p != '\000')
	{
		if (is_escaped_char(*p))
			*q++ = '\\';
		/* The following is mine */
		else if ( doublesquare && (*p == '[' || *p == ']') )
		{
			*q++ = '\\';
			*q++ = '\\';
		}
		*q++ = *p++;
	}
	*q = '\000';
	return escaped;
}
//End code from xarchive

gchar *StripPathFromFilename ( gchar *name, gchar *pattern )
{
    return g_strrstr ( name , pattern );
}

gchar *JoinPathArchiveName ( const gchar *extract_path , gchar *path )
{
	return g_strconcat (extract_path , path , NULL);
}

int CountCharacter ( gchar *string , int chr )
{
    int n = 0;
    while ( *string )
    {
        if ( *string == chr ) n++;
        string++;
    }
    return n;
}

gchar *RemoveBackSlashes ( gchar *name)
{
    gchar *nome, *q;
    int x = CountCharacter ( name , '\\' );
    nome = (char *) g_malloc (strlen(name) - x + 1);
    q = nome;
    while ( *name )
    {
        if ( *name == '\\' ) name++;
        *q++ = *name++;
    }
    *q = '\000';
    return nome;
}

/* These functions are from File-Roller code */
char *eat_spaces (char *line)
{
	if (line == NULL)
		return NULL;
	while ((*line == ' ') && (*line != 0))
		line++;
	return line;
}

gchar *remove_level_from_path (const gchar *path)
{
    const gchar *ptr = path;
    gint p;
    if (! path) return NULL;
    p = strlen (path) - 1;
    if (p < 0) return NULL;
    while ((ptr[p] != '/') && (p > 0))
        p--;
    if ((p == 0) && (ptr[p] == '/')) p++;
    return g_strndup (path, (guint)p);
}

gboolean file_extension_is (const char *filename, const char *ext)
{
	int filename_l, ext_l;

	filename_l = strlen (filename);
	ext_l = strlen (ext);

    if (filename_l < ext_l)
		return FALSE;
    return strcasecmp (filename + filename_l - ext_l, ext) == 0;
}
/* End code from File-Roller */

gchar *extract_local_path (gchar *path , gchar *filename)
{
    gchar *local_path;
    gchar *local_escaped_path;
	unsigned short int x;

	gchar *no_path = StripPathFromFilename(filename , "/");
	if (no_path != NULL)
	{
		no_path++;
	    x = strlen (path) - strlen ( no_path );
	}
	else
		x = strlen (path) - strlen ( filename );
    //g_print ("%d\t%d\t%d\n",x,strlen (path),strlen (filename));
    local_path = (gchar *) g_malloc ( x + 1);
    strncpy ( local_path, path, x );
    local_path [x] = '\000';
    local_escaped_path = EscapeBadChars ( local_path , 1);
    g_free (local_path);
    return local_escaped_path;
}

