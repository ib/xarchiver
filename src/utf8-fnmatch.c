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

/*
 * Copyright (c) 1989, 1993, 1994
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Guido van Rossum.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#if defined(LIBC_SCCS) && !defined(lint)
static char rcsid[] = "$OpenBSD: fnmatch.c,v 1.6 1998/03/19 00:29:59 millert Exp $";
#endif /* LIBC_SCCS and not lint */

/*
 * Function fnmatch() as specified in POSIX 1003.2-1992, section B.6.
 * Compares a filename or pathname to a pattern.
 */

#include <config.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <glib.h>
#include "utf8-fnmatch.h"

#undef	EOS
#define	EOS	'\0'

#define	RANGE_MATCH	1
#define	RANGE_NOMATCH	0
#define	RANGE_ERROR	(-1)


static int
g_utf8_rangematch (const char  *pattern,
		   gunichar     test,
		   int          flags,
		   char       **new_pattern)
{
	gboolean  negate, ok;
	gunichar  c;

	/*
	 * A bracket expression starting with an unquoted circumflex
	 * character produces unspecified results (IEEE 1003.2-1992,
	 * 3.13.2).  This implementation treats it like '!', for
	 * consistency with the regular expression syntax.
	 * J.T. Conklin (conklin@ngai.kaleida.com)
	 */
	if ((negate = (g_utf8_get_char (pattern) == '!' || g_utf8_get_char (pattern) == '^')))
		pattern = g_utf8_next_char (pattern);

	if (flags & FNM_CASEFOLD)
		test = g_unichar_tolower (test);

	/*
	 * A right bracket shall lose its special meaning and represent
	 * itself in a bracket expression if it occurs first in the list.
	 * -- POSIX.2 2.8.3.2
	 */
	ok = FALSE;

	c = g_utf8_get_char (pattern);
	pattern = g_utf8_next_char (pattern);

	do {
		gunichar c2;

		if (c == '\\' && !(flags & FNM_NOESCAPE)) {
			c = g_utf8_get_char (pattern);
			pattern = g_utf8_next_char (pattern);
		}

		if (c == EOS)
			return (RANGE_ERROR);

		if (c == '/' && (flags & FNM_PATHNAME))
			return (RANGE_NOMATCH);

		if ((flags & FNM_CASEFOLD)) {
			c = g_unichar_tolower (c);
		}

		c2 = g_utf8_get_char (g_utf8_next_char (pattern));

		if ((g_utf8_get_char (pattern) == '-') && (c2 != EOS) && (c2 != ']')) {
			pattern = g_utf8_next_char (pattern);
			pattern = g_utf8_next_char (pattern);

			if (c2 == '\\' && !(flags & FNM_NOESCAPE)) {
				c2 = g_utf8_get_char (pattern);
				pattern = g_utf8_next_char (pattern);
			}

			if (c2 == EOS)
				return (RANGE_ERROR);

			if (flags & FNM_CASEFOLD) 
				c2 = g_unichar_tolower (c2);

			if (c <= test && test <= c2)
				ok = TRUE;

		} else if (c == test)
			ok = TRUE;

		c = g_utf8_get_char (pattern);
		pattern = g_utf8_next_char (pattern);

	} while (c != ']');
		
	*new_pattern = (char *) pattern;

	return (ok == negate ? RANGE_NOMATCH : RANGE_MATCH);
}


int
g_utf8_fnmatch (const char *pattern, 
		const char *string, 
		int         flags)
{
	const char *stringstart;
	char       *new_pattern;
	gunichar    c, test;

	for (stringstart = string;;) {
		const char *string_1;
		gunichar    c2;

		c = g_utf8_get_char (pattern);
		pattern = g_utf8_next_char (pattern);

		switch (c) {
		case EOS:
			if ((flags & FNM_LEADING_DIR) && g_utf8_get_char (string) == '/') 
				return 0;
			return (g_utf8_get_char (string) == EOS ? 0 : FNM_NOMATCH);

		case '?':
			if (g_utf8_get_char (string) == EOS)
				return FNM_NOMATCH;

			if (g_utf8_get_char (string) == '/' && (flags & FNM_PATHNAME))
				return FNM_NOMATCH;

			string_1 = g_utf8_prev_char (string);
			if ((g_utf8_get_char (string) == '.') 
			    && (flags & FNM_PERIOD) 
			    && (string == stringstart ||
				((flags & FNM_PATHNAME) && g_utf8_get_char (string_1) == '/')))
				return FNM_NOMATCH;
			string = g_utf8_next_char (string);
			break;

		case '*':
			c = g_utf8_get_char (pattern);
			/* Collapse multiple stars. */
			while (c == '*') {
				pattern = g_utf8_next_char (pattern);
				c = g_utf8_get_char (pattern);
			}

			string_1 = g_utf8_prev_char (string);
			if ((g_utf8_get_char (string) == '.') 
			    && (flags & FNM_PERIOD) 
			    && (string == stringstart ||
				((flags & FNM_PATHNAME) && g_utf8_get_char (string_1) == '/')))
				return FNM_NOMATCH;

			/* Optimize for pattern with * at end or before /. */
			if (c == EOS) {
				if (flags & FNM_PATHNAME)
					return ((flags & FNM_LEADING_DIR) 
						|| (g_utf8_strchr (string, -1, '/') == NULL) ? 0 : FNM_NOMATCH);
				else
					return 0;

			} else if (c == '/' && (flags & FNM_PATHNAME)) {
				if ((string = g_utf8_strchr (string, -1, '/')) == NULL)
					return FNM_NOMATCH;
				break;
			}

			/* General case, use recursion. */
			while ((test = g_utf8_get_char (string)) != EOS) {
				if (! g_utf8_fnmatch (pattern, string, flags & ~FNM_PERIOD))
					return 0;
				if (test == '/' && (flags & FNM_PATHNAME))
					break;
				string = g_utf8_next_char (string);
			}
			return FNM_NOMATCH;

		case '[':
			if (g_utf8_get_char (string) == EOS)
				return FNM_NOMATCH;

			if (g_utf8_get_char (string) == '/' && (flags & FNM_PATHNAME))
				return FNM_NOMATCH;

			string_1 = g_utf8_prev_char (string);
			if ((g_utf8_get_char (string) == '.') 
			    && (flags & FNM_PERIOD) 
			    && (string == stringstart ||
				((flags & FNM_PATHNAME) && g_utf8_get_char (string_1) == '/')))
				return FNM_NOMATCH;

			switch (g_utf8_rangematch (pattern, g_utf8_get_char (string), flags, &new_pattern)) {
			case RANGE_ERROR:
				/* not a good range, treat as normal text */
				goto normal;
			case RANGE_MATCH:
				pattern = new_pattern;
				break;
			case RANGE_NOMATCH:
				return FNM_NOMATCH;
			}
			string = g_utf8_next_char (string);
			break;

		case '\\':
			if (!(flags & FNM_NOESCAPE)) {
				pattern = g_utf8_next_char (pattern);
				c = g_utf8_get_char (pattern);
				if (c == EOS) {
					c = '\\';
					pattern = g_utf8_prev_char (pattern);
				}
			}

			/* FALLTHROUGH */
		default:
		normal:
			c2 = g_utf8_get_char (string);

			if (flags & FNM_CASEFOLD) {
				c = g_unichar_tolower (c);
				c2 = g_unichar_tolower (c2);
			}

			if (c != c2)
				return FNM_NOMATCH;

			string = g_utf8_next_char (string);
			break;
		}
	}
	/* NOTREACHED */
}


#ifdef UTF8_FN_MATCH_TEST


static gboolean noisy = FALSE;


static void
verbose (const gchar *format, ...)
{
	gchar *msg;
	va_list args;
	
	va_start (args, format);
	msg = g_strdup_vprintf (format, args);
	va_end (args);
	
	if (noisy)
		g_print (msg);
	g_free (msg);
}


static gboolean
test_match (gchar *pattern,
            gchar *string,
            gboolean match)
{
	verbose ("matching \"%s\" against \"%s\" \t", string, pattern);
	
	if ((g_utf8_fnmatch (pattern, string, FNM_CASEFOLD) == 0) != match)
		{
			g_print ("failed \t(unexpected %s)\n", (match ? "mismatch" : "match"));
			return FALSE;
		}
	
	verbose ("passed (%s)\n", match ? "match" : "nomatch");
	
	return TRUE;
}


#define TEST_MATCH(pattern, string, match) {	\
  total++;			\
  if (test_match (pattern, string, match)) \
    passed++; \
  else \
    failed++; \
}


int main (int argc, gchar **argv) {
	gint total = 0;
	gint passed = 0;
	gint failed = 0;
	gint i;
	
	for (i = 1; i < argc; i++)
		if (strcmp ("--noisy", argv[i]) == 0)
			noisy = TRUE;
	
	TEST_MATCH("*x", "x", TRUE);
	TEST_MATCH("*x", "xx", TRUE);
	TEST_MATCH("*x", "yyyx", TRUE);
	TEST_MATCH("*x", "yyxy", FALSE);
	TEST_MATCH("?x", "x", FALSE);
	TEST_MATCH("?x", "xx", TRUE);
	TEST_MATCH("?x", "yyyx", FALSE);
	TEST_MATCH("?x", "yyxy", FALSE);
	TEST_MATCH("*?x", "xx", TRUE);
	TEST_MATCH("?*x", "xx", TRUE);
	TEST_MATCH("*?x", "x", FALSE);
	TEST_MATCH("?*x", "x", FALSE);
	TEST_MATCH("*?*x", "yx", TRUE);
	TEST_MATCH("*?*x", "xxxx", TRUE);
	TEST_MATCH("x*??", "xyzw", TRUE);
	TEST_MATCH("*x", "\xc3\x84x", TRUE);
	TEST_MATCH("?x", "\xc3\x84x", TRUE);
	TEST_MATCH("??x", "\xc3\x84x", FALSE);
	TEST_MATCH("ab\xc3\xa4\xc3\xb6", "ab\xc3\xa4\xc3\xb6", TRUE);
	TEST_MATCH("ab\xc3\xa4\xc3\xb6", "abao", FALSE);
	TEST_MATCH("ab?\xc3\xb6", "ab\xc3\xa4\xc3\xb6", TRUE);
	TEST_MATCH("ab?\xc3\xb6", "abao", FALSE);
	TEST_MATCH("ab\xc3\xa4?", "ab\xc3\xa4\xc3\xb6", TRUE);
	TEST_MATCH("ab\xc3\xa4?", "abao", FALSE);
	TEST_MATCH("ab??", "ab\xc3\xa4\xc3\xb6", TRUE);
	TEST_MATCH("ab*", "ab\xc3\xa4\xc3\xb6", TRUE);
	TEST_MATCH("ab*\xc3\xb6", "ab\xc3\xa4\xc3\xb6", TRUE);
	TEST_MATCH("ab*\xc3\xb6", "aba\xc3\xb6x\xc3\xb6", TRUE);
	TEST_MATCH("*.o", "gtkcellrendererthreestates.o", TRUE);
	TEST_MATCH("A*.o", "AA.o", TRUE);
	TEST_MATCH("A*.o", "aaaa.o", TRUE);
	TEST_MATCH("A*.o", "B.o", FALSE);

	verbose ("\n%u tests passed, %u failed\n", passed, failed);
        
	return failed;
}

#endif

