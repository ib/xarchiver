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

#include "config.h"
#include <string.h>
#include "string_utils.h"
#include "utf8-fnmatch.h"

#ifndef HAVE_MKDTEMP
#include <errno.h>
#include <glib/gstdio.h>
gchar *mkdtemp (gchar *tmpl)
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
#include <ctype.h>
#endif

/* This function is from File-Roller code */
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
/* End code from File-Roller */

static char *xa_escape_common_chars (const char *str, const char *meta_chars, const char  prefix, const char postfix)
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
  escaped = g_malloc0 (new_l + 1);

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

static gchar *xa_strip_current_working_dir_from_path (gchar *working_dir, gchar *filename)
{
	gchar *slash;
	int len = 0;

	if (working_dir == NULL)
		return filename;
	len = strlen(working_dir)+1;
	slash = g_strrstr(filename,"/");
	if (slash == NULL || ! g_path_is_absolute(filename))
		return filename;

	return filename+len;
}

static void xa_remove_slash_from_path (gchar *path)
{
	size_t len = strlen(path);

	if (len > 0 && path[len - 1] == '/')
		path[len - 1] = 0;
}

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

gchar *xa_escape_bad_chars (const gchar *string, const gchar *pattern)
{
	return xa_escape_common_chars (string, pattern, '\\', 0);
}

gchar *xa_remove_level_from_path (const gchar *path)
{
	gchar *local_path;
	gchar *_local_path;

	if (path[strlen(path)-1] == '/')
	{
		_local_path = g_strndup(path,strlen(path)-1);
		local_path  = g_path_get_dirname (_local_path);
		g_free(_local_path);
	}
	else
    	local_path = g_path_get_dirname (path);
    return local_path;
}

void xa_set_window_title (GtkWidget *window,gchar *title)
{
	gchar *x 	= NULL;
	gchar *slash= NULL;

	if (title)
	{
		slash = g_strrstr (title , "/");
		if (slash)
			slash++;
	}
	if (!slash)
		slash = title;

	if (title == NULL)
		x = g_strconcat(PACKAGE_NAME, " ", PACKAGE_VERSION, NULL);
	else
		x = g_strconcat(slash, " - ", PACKAGE_NAME, " ", PACKAGE_VERSION, NULL);
	gtk_window_set_title (GTK_WINDOW (window),x);
	g_free (x);
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

GSList *xa_collect_filenames (XArchive *archive, GSList *in)
{
	GSList *list = in, *out = NULL;
	gchar *basename, *name;

	while (list)
	{
		if (archive->location_path)
		{
			if (archive->do_full_path)
			{
				name = g_strconcat(archive->location_path, list->data, NULL);
				out = g_slist_append(out, name);
			}
			else
			{
				basename = xa_strip_current_working_dir_from_path(archive->child_dir ? archive->child_dir : archive->working_dir, list->data);
				name = g_strconcat(archive->location_path, basename, NULL);
				out = g_slist_append(out, name);
			}
		}
		else
		{
			if (archive->do_full_path)
				out = g_slist_append(out, g_strdup(list->data));
			else
			{
				basename = xa_strip_current_working_dir_from_path(archive->child_dir ? archive->child_dir : archive->working_dir, list->data);
				out = g_slist_append(out, g_strdup(basename));
			}
		}

		list = list->next;
	}

	return out;
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

void xa_recurse_local_directory(gchar *path,GSList **list,gboolean recurse,gint type)
{
	DIR *dir;
	struct dirent *dirlist;
	gchar *fullname = NULL,*basename = NULL;
	gboolean is_dir;

	dir = opendir(path);
	is_dir = g_file_test(path,G_FILE_TEST_IS_DIR);
	if (is_dir)
		*list = g_slist_prepend(*list,g_strdup(path));
	if (dir == NULL)
	{
		if (is_dir == FALSE)
		{
			basename = g_path_get_basename(path);
			*list = g_slist_prepend(*list,basename);
			return;
		}
	}

	while ((dirlist = readdir(dir)))
	{
		if (strcmp(dirlist->d_name,".") == 0 || strcmp(dirlist->d_name,"..") == 0)
			continue;
		fullname = g_strconcat (path,"/",dirlist->d_name,NULL);
		is_dir = g_file_test(fullname,G_FILE_TEST_IS_DIR);
		if ( ! is_dir)
			*list = g_slist_prepend(*list,fullname);
		if (recurse && is_dir)
			xa_recurse_local_directory(fullname,list,recurse,type);
	}
	closedir(dir);
}

GString *xa_quote_filenames (GSList *file_list, const gchar *escape, gboolean slash)
{
	GString *files;
	GSList *list;

	files = g_string_new("");
	list= file_list;

	while (list)
	{
		gchar *shellname, *escaped = NULL;

		if (!slash)
			xa_remove_slash_from_path(list->data);

		if (escape)
			escaped = xa_escape_bad_chars(list->data, "\\");

		shellname = g_shell_quote(escaped ? escaped : list->data);

		g_free(escaped);

		if (escape)
			escaped = xa_escape_bad_chars(shellname, escape);

		g_string_prepend(files, escaped ? escaped : shellname);
		g_string_prepend_c(files, ' ');

		g_free(escaped);
		g_free(shellname);

		list = list->next;
	}

	g_slist_foreach(file_list, (GFunc) g_free, NULL);
	g_slist_free(file_list);

	return files;
}

gchar *xa_quote_shell_command (const gchar *string, gboolean unescaped)
{
	gchar *unquoted = NULL, *quoted, *escaped;

	if (!unescaped)
		unquoted = g_shell_unquote(string, NULL);

	quoted = g_shell_quote(unquoted ? unquoted : string);

	escaped = xa_escape_bad_chars(quoted, "\"");

	g_free(quoted);
	g_free(unquoted);

	return escaped;
}
