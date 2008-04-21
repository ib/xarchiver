/*
* C++ Interface: glib-mem
*
* Description: Compatibility macros for older versions of glib
*
*
* Author: Hong Jen Yee (PCMan) <pcman.tw (AT) gmail.com>, (C) 2006
*
* Copyright: See COPYING file that comes with this distribution
*
*/

#include "glib-utils.h"

/* older versions of glib don't provde these API */
#if ! GLIB_CHECK_VERSION(2, 8, 0)

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

int g_mkdir_with_parents(const gchar *pathname, int mode)
{
    struct stat statbuf;
    char *dir, *sep;
    dir = g_strdup( pathname );
    sep = dir[0] == '/' ? dir + 1 : dir;
    do {
        sep = strchr( sep, '/' );
        if( G_LIKELY( sep ) )
            *sep = '\0';

        if( stat( dir, &statbuf) == 0 )
        {
            if( ! S_ISDIR(statbuf.st_mode) )    /* parent not dir */
                goto err;
        }
        else    /* stat failed */
        {
            if( errno == ENOENT )   /* not exists */
            {
                if( mkdir( dir, mode ) == -1 )
                    goto err;
            }
            else
                goto err;   /* unknown error */
        }

        if( G_LIKELY( sep ) )
        {
            *sep = '/';
            ++sep;
        }
        else
            break;
    }while( sep );
    g_free( dir );
    return 0;
err:
    g_free( dir );
    return -1;
}
#endif
