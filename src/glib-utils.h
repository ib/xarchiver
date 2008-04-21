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

#ifndef _GLIB_UTILS_H_
#define _GLIB_UTILS_H_

#include <glib.h>

#if ! GLIB_CHECK_VERSION(2, 8, 0)
/* older versions of glib don't provde these API */
int g_mkdir_with_parents(const gchar *pathname, int mode);
#endif

#endif

