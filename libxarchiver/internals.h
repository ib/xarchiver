#ifndef __LIBXARCHIVER_INTERNALS_H__
#define __LIBXARCHIVER_INTERNALS_H__
static int n_cwd = 0;
gchar *escape_filename ( gchar *string );
GString *concatenatefilenames ( GSList *list );
int countcharacters ( gchar *string , int chr );
GSList *split_line (GSList *dummy , gchar *line, unsigned short int n_fields);
gchar *eat_spaces (gchar *line);
GSList *get_last_field (gchar *line,unsigned short int last_field);
#endif
