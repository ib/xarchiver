/* -*- mode: C; c-file-style: "gnu" -*- */
/* xdgmimealias.c: Private file.  Datastructure for storing the hierarchy.
 *
 * More info can be found at http://www.freedesktop.org/standards/
 *
 * Copyright (C) 2004  Red Hat, Inc.
 * Copyright (C) 2004  Matthias Clasen <mclasen@redhat.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later or AFL-2.0
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "xdgmimeparent.h"
//#include "xdgmimeint.h"
#include <stdlib.h>
#include <stdio.h>
//#include <assert.h>
#include <string.h>
//#include <fnmatch.h>

//#ifndef	FALSE
//#define	FALSE	(0)
//#endif

//#ifndef	TRUE
//#define	TRUE	(!FALSE)
//#endif

typedef struct XdgMimeParents XdgMimeParents;

struct XdgMimeParents
{
  char *mime;
  char **parents;
  int n_parents;
};

struct XdgParentList
{
  struct XdgMimeParents *parents;
  int n_mimes;
};

XdgParentList *
_xdg_mime_parent_list_new (void)
{
  XdgParentList *list;

  list = malloc (sizeof (XdgParentList));

  list->parents = NULL;
  list->n_mimes = 0;

  return list;
}

void         
_xdg_mime_parent_list_free (XdgParentList *list)
{
  int i;
  char **p;

  if (list->parents)
    {
      for (i = 0; i < list->n_mimes; i++)
	{
	  for (p = list->parents[i].parents; *p; p++)
	    free (*p);

	  free (list->parents[i].parents);
	  free (list->parents[i].mime);
	}
      free (list->parents);
    }
  free (list);
}

static int
parent_entry_cmp (const void *v1, const void *v2)
{
  return strcmp (((XdgMimeParents *)v1)->mime, ((XdgMimeParents *)v2)->mime);
}

//const char **
//_xdg_mime_parent_list_lookup (XdgParentList *list,
//			      const char    *mime)
//{
//  XdgMimeParents *entry;
//  XdgMimeParents key;
//
//  if (list->n_mimes > 0)
//    {
//      key.mime = (char *)mime;
//      key.parents = NULL;
//      key.n_parents = 0;
//
//      entry = bsearch (&key, list->parents, list->n_mimes,
//		       sizeof (XdgMimeParents), &parent_entry_cmp);
//      if (entry)
//        return (const char **)entry->parents;
//    }
//
//  return NULL;
//}

void
_xdg_mime_parent_read_from_file (XdgParentList *list,
				 const char    *file_name)
{
  FILE *file;
  char line[255];
  int i, alloc;
  XdgMimeParents *entry;

  file = fopen (file_name, "r");

  if (file == NULL)
    return;

  /* FIXME: Not UTF-8 safe.  Doesn't work if lines are greater than 255 chars.
   * Blah */
  alloc = list->n_mimes + 16;
  list->parents = realloc (list->parents, alloc * sizeof (XdgMimeParents));
  while (fgets (line, 255, file) != NULL)
    {
      char *sep;
      if (line[0] == '#')
	continue;

      sep = strchr (line, ' ');
      if (sep == NULL)
	continue;
      *(sep++) = '\000';
      sep[strlen (sep) -1] = '\000';
      entry = NULL;
      for (i = 0; i < list->n_mimes; i++)
	{
	  if (strcmp (list->parents[i].mime, line) == 0)
	    {
	      entry = &(list->parents[i]);
	      break;
	    }
	}
      
      if (!entry)
	{
	  if (list->n_mimes == alloc)
	    {
	      alloc <<= 1;
	      list->parents = realloc (list->parents, 
				       alloc * sizeof (XdgMimeParents));
	    }
	  list->parents[list->n_mimes].mime = strdup (line);
	  list->parents[list->n_mimes].parents = NULL;
	  entry = &(list->parents[list->n_mimes]);
	  list->n_mimes++;
	}

      if (!entry->parents)
	{
	  entry->n_parents = 1;
	  entry->parents = malloc ((entry->n_parents + 1) * sizeof (char *));
	}
      else
	{
	  entry->n_parents += 1;
	  entry->parents = realloc (entry->parents, 
				    (entry->n_parents + 2) * sizeof (char *));
	}
      entry->parents[entry->n_parents - 1] = strdup (sep);
      entry->parents[entry->n_parents] = NULL;
    }

  list->parents = realloc (list->parents, 
			   list->n_mimes * sizeof (XdgMimeParents));

  fclose (file);  
  
  if (list->n_mimes > 1)
    qsort (list->parents, list->n_mimes, 
           sizeof (XdgMimeParents), &parent_entry_cmp);
}


//void         
//_xdg_mime_parent_list_dump (XdgParentList *list)
//{
//  int i;
//  char **p;
//
//  if (list->parents)
//    {
//      for (i = 0; i < list->n_mimes; i++)
//	{
//	  for (p = list->parents[i].parents; *p; p++)
//	    printf ("%s %s\n", list->parents[i].mime, *p);
//	}
//    }
//}


