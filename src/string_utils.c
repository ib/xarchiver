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

gchar *EscapeBadChars ( gchar *string , gchar *pattern)
{
	return escape_str_common (string, pattern, '\\', 0);
}


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
static int count_chars_to_escape (const char *str, const char *meta_chars)
{
        int         meta_chars_n = strlen (meta_chars);
        const char *s;
        int         n = 0;

        for (s = str; *s != 0; s++) {
                int i;
                for (i = 0; i < meta_chars_n; i++)
                        if (*s == meta_chars[i]) {
                                n++;
                                break;
                        }
        }
        return n;
}
char *escape_str_common (const char *str, const char *meta_chars, const char  prefix, const char  postfix)
{
        int         meta_chars_n = strlen (meta_chars);
        char       *escaped;
        int         i, new_l, extra_chars = 0;
        const char *s;
        char       *t;

        if (str == NULL)
                return NULL;

        if (prefix)
                extra_chars++;
        if (postfix)
                extra_chars++;

        new_l = strlen (str) + (count_chars_to_escape (str, meta_chars) * extra_chars);
        escaped = g_malloc (new_l + 1);

        s = str;
        t = escaped;
        while (*s) {
                gboolean is_bad = FALSE;
                for (i = 0; (i < meta_chars_n) && !is_bad; i++)
                        is_bad = (*s == meta_chars[i]);
                if (is_bad && prefix)
                        *t++ = prefix;
                *t++ = *s++;
                if (is_bad && postfix)
                        *t++ = postfix;
        }
        *t = 0;

        return escaped;
}

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
    local_path = (gchar *) g_malloc ( x + 1);
    strncpy ( local_path, path, x );
    local_path [x] = '\000';
    local_escaped_path = EscapeBadChars ( local_path ,"$\'`\"\\!?* ()[]&|@#:;");
    g_free (local_path);
    return local_escaped_path;
}

