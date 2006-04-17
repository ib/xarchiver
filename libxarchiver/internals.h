#ifndef __LIBXARCHIVER_INTERNALS_H__
#define __LIBXARCHIVER_INTERNALS_H__
static int n_cwd = 0;
gchar *escape_filename ( gchar *string );
GString *concatenatefilenames ( GSList *list , gboolean basename_only);
int countcharacters ( gchar *string , int chr );
GList *split_line (GList *dummy , gchar *line, unsigned short int n_fields);
gchar *eat_spaces (gchar *line);
GList *get_last_field (GList *dummy , gchar *line,unsigned short int last_field);
#endif
