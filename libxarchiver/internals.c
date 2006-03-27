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
