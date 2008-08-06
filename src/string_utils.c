/*
 *  Copyright (C) 2008 Giuseppe Torelli - <colossus73@gmail.com>
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
#include "utf8-fnmatch.h"
#include "errno.h"

#ifndef HAVE_MKDTEMP
char *mkdtemp (gchar *tmpl)
{
	static const gchar LETTERS[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
	static guint64     value = 0;
	guint64            v;
	gint               len;
	gint               i, j;

	len = strlen (tmpl);
	if (len < 6 || strcmp (&tmpl[len - 6], "XXXXXX") != 0)
    {
		errno = EINVAL;
		return NULL;
	}

	value += ((guint64) time (NULL)) ^ getpid ();

	for (i = 0; i < TMP_MAX; ++i, value += 7777)
    {
		/* fill in the random bits */
		for (j = 0, v = value; j < 6; ++j)
			tmpl[(len - 6) + j] = LETTERS[v % 62]; v /= 62;

		/* try to create the directory */
		if (g_mkdir (tmpl, 0700) == 0)
			return tmpl;
		else if (errno != EEXIST)
			return NULL;
    }

	errno = EEXIST;
	return NULL;
}
#endif

#ifndef HAVE_STRCASESTR
/*
 * case-insensitive version of strstr()
 */
const char *strcasestr(const char *haystack, const char *needle)
{
	const char *h;
	const char *n;

	h = haystack;
	n = needle;
	while (*haystack)
	{
		if (tolower((unsigned char) *h) == tolower((unsigned char) *n))
		{
			h++;
			n++;
			if (!*n)
				return haystack;
		} else {
			h = ++haystack;
			n = needle;
		}
	}
	return NULL;
}
#endif /* !HAVE_STRCASESTR */

gchar *xa_escape_bad_chars ( gchar *string , gchar *pattern)
{
	return xa_escape_common_chars (string, pattern, '\\', 0);
}

/* These functions are from File-Roller code */

static int count_chars_to_escape (const char *str, const char *meta_chars)
{
	int meta_chars_n = strlen (meta_chars);
	const char *s;
	int n = 0;

	for (s = str; *s != 0; s++)
	{
		int i;
		for (i = 0; i < meta_chars_n; i++)
			if (*s == meta_chars[i])
			{
        		n++;
            	break;
			}
		}
	return n;
}
char *xa_escape_common_chars (const char *str, const char *meta_chars, const char  prefix, const char postfix)
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

gchar *remove_level_from_path (const gchar *path)
{
	return g_path_get_dirname(path);
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

gchar *extract_local_path (gchar *path)
{
    gchar *local_path;
    gchar *local_escaped_path;

    local_path = g_path_get_dirname (path);
    local_escaped_path = xa_escape_bad_chars ( local_path ,"$\'`\"\\!?* ()[]&|@#:;");
    g_free (local_path);
    return local_escaped_path;
}

void xa_set_window_title (GtkWidget *window,gchar *title)
{
	gchar *x 	= NULL;
	gchar *slash= NULL;

	if (title == NULL)
		gtk_window_set_title (GTK_WINDOW (window),"Xarchiver " VERSION);
	else
	{
		slash = g_strrstr (title , "/");
		if (slash == NULL)
		{
			x = g_strconcat (title , " - ","Xarchiver ",VERSION,NULL);
			gtk_window_set_title (GTK_WINDOW (window),x);
			g_free (x);
			return;
		}
		else
		{
			x = g_strconcat (slash, " - ","Xarchiver ",VERSION,NULL);
			x++;
			gtk_window_set_title (GTK_WINDOW (window),x);
			x--;
			g_free(x);
		}
	}
}

gboolean match_patterns (char **patterns,const char *string,int flags)
{
	int i;
	int result;

	if (patterns[0] == NULL)
		return TRUE;

	if (string == NULL)
		return FALSE;

	result = FNM_NOMATCH;
	i = 0;
	while ((result != 0) && (patterns[i] != NULL))
	{
		result = g_utf8_fnmatch (patterns[i],string,flags);
		i++;
	}
	return (result == 0);
}

gchar *xa_remove_path_from_archive_name(gchar *name)
{
	gchar *utf8_string,*text;

	text = g_strrstr (name,"/");
	if (text != NULL)
	{
		text++;
		utf8_string = g_filename_display_name (text);
	}
	else
		utf8_string = g_filename_display_name (name);

	return utf8_string;
}

gchar *xa_escape_filename (gchar *filename,gchar *meta_chars)
{
	return xa_escape_common_chars (filename , meta_chars, '\\', 0);
}

void xa_cat_filenames (XArchive *archive,GSList *list,GString *data)
{
	gchar *basename, *name, *e_filename;
	GSList *slist = list;

	while (slist)
	{
		if (archive->location_entry_path != NULL)
		{
			if (archive->full_path == 0)
			{
				basename = g_path_get_basename (slist->data);
				name = g_strconcat(archive->location_entry_path,basename,NULL);
				g_free(basename);
				e_filename = xa_escape_filename(name,"$'`\"\\!?* ()[]&|:;<>#");
				g_string_prepend (data,e_filename);
				g_string_prepend_c (data,' ');
				g_free(name);
			}
			else
			{
				name = g_strconcat(archive->location_entry_path,slist->data,NULL);
				e_filename = xa_escape_filename(name,"$'`\"\\!?* ()[]&|:;<>#");
				g_string_prepend (data,e_filename);
				g_string_prepend_c (data,' ');
				g_free(name);
			}
		}
		else
		{
			if (archive->full_path == 0)
			{
				basename = g_path_get_basename (slist->data);
				e_filename = xa_escape_filename(basename,"$'`\"\\!?* ()[]&|:;<>#");
				g_string_prepend (data,e_filename);
				g_string_prepend_c (data,' ');
				g_free (basename);
			}
			else
			{
				e_filename = xa_escape_filename(slist->data,"$'`\"\\!?* ()[]&|:;<>#");
				g_string_prepend (data,e_filename);
				g_string_prepend_c (data,' ');
			}
		}
		slist = slist->next;
	}
}

GSList *xa_slist_copy(GSList *list)
{
	GSList *x,*y = NULL;
	x = list;

	while (x)
	{
		y = g_slist_prepend(y,g_strdup(x->data));
		x = x->next;
	}
	return g_slist_reverse(y);
}
