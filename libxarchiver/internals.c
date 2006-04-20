#include <stdio.h>
#include <stdlib.h>
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

GString *concatenatefilenames ( GSList *list , gboolean basename_only)
{
    gchar *filename , *esc_filename;
    
    GString *names = g_string_new(" ");
    while( list )
	{
		if(basename_only)
			filename = g_path_get_basename(list->data);
		else
			filename = g_strdup(list->data);

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

//Taken from file-roller - http://fileroller.sourceforge.net

GList *split_line (GList *fields , gchar *line , unsigned short int n_fields)
{
	gchar *scan, *field_end;
	gchar *field = NULL;
	unsigned short int i;
	unsigned long int size = 0;

	scan = eat_spaces (line);
	for (i = 0; i < n_fields; i++)
	{
		field_end = strchr (scan, ' ');
		//The following line is mine, I added the case when the last field ends with a newline
		if (field_end == NULL) field_end = strchr (scan, '\n');
			field = g_strndup (scan, field_end - scan);
		fields = g_list_prepend ( fields, field );
		scan = eat_spaces (field_end);
	}
	/*
	GList *dummy = fields;
	while (dummy)
	{
		g_print ("%d\t%s\n",i,(gchar*)dummy->data);
		dummy = dummy->next;
	}*/
	return fields;
}

gchar *eat_spaces (gchar *line)
{
	if (line == NULL)
		return NULL;
	while ((*line == ' ') && (*line != 0))
		line++;
	return line;
}

GList *get_last_field (GList *dummy , gchar *line,unsigned short int last_field)
{
	gchar *field = NULL;
	gchar *filename = NULL;
	int i;

	if (line == NULL)
		return NULL;

	last_field--;
	field = eat_spaces (line);
	for (i = 0; i < last_field; i++)
	{
		if (field == NULL)
			return NULL;
		field = strchr (field, '\n');
		field = eat_spaces (field);
	}
    if (field != NULL) field [ strlen(field) -1 ] = '\000';
	filename = g_strdup (field);
	dummy = g_list_prepend ( dummy , filename );
	return dummy;
}
