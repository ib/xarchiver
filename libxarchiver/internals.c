#include <stdio.h>
#include <string.h>
#include <glib.h>
#include "internals.h"

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

gchar *escape_filename( gchar *string )
{
	char *q;
	char *escaped;
	int escapechars = 0;
	char *p = string;

	while (*p != '\000')
	{
        	if (is_escaped_char(*p)) escapechars++;
	        p++;
    }

	if (!escapechars) return g_strdup(string);
	escaped = (char *) g_malloc (strlen(string) + escapechars + 1);

	p = string;
	q = escaped;

	while (*p != '\000')
	{
        if (is_escaped_char(*p)) *q++ = '\\';
		*q++ = *p++;
	}
	*q = '\000';
	return escaped;
}

//End code from xarchive

GString *concatenatefilenames ( GSList *list )
{
    gchar *filename , *esc_filename;
    
    GString *names = g_string_new(" ");
    while( list )
	{
		filename = g_path_get_basename(list->data);
		esc_filename = escape_filename(filename);
		g_string_prepend(names, esc_filename);
		g_string_prepend_c(names, ' ');
		g_free(esc_filename);
		g_free(filename);
		list = g_slist_next(list);
	}
    return names;
}

int countcharacters ( gchar *string , int chr )
{
    int n = 0;
    while ( *string )
    {
        if ( *string == chr ) n++;
        string++;
    }
    return n;
}


