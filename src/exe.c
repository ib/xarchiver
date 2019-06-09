/*
 *  Copyright (C) 2019 Ingo Brückl
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

#ifdef __APPLE__
#include <libkern/OSByteOrder.h>
#define le32toh(x) OSSwapLittleToHostInt32(x)
#else
#include <endian.h>
#endif
#include <glib.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "exe.h"

static gboolean search (FILE *file, const char *what, unsigned int max)
{
	gboolean success = FALSE;
	long pos;
	size_t wlen;
	unsigned char *buffer, *start, *found;

	pos = ftell(file);
	wlen = strlen(what);
	buffer = calloc(max, sizeof(*what));

	if (buffer)
	{
		if (fread(buffer, sizeof(*what), max, file) == max)
		{
			start = buffer;

			while ((found = memchr(start, *what, max)))
			{
				max -= found - start;

				if (max < wlen)
				{
					found = NULL;
					break;
				}

				if (memcmp(found, what, wlen) == 0)
					break;

				start = found + 1;
				max--;
			}

			if (found &&
			    fseek(file, pos + (found - buffer) + wlen, SEEK_SET) == 0)
				success = TRUE;
		}

		free(buffer);
	}

	return success;
}

ArchiveType exetype (FILE *file)
{
	ArchiveType xa = {XARCHIVETYPE_UNKNOWN, 0};
	uint16_t leshort;
	uint32_t lelong;
	char pe[4];

	if (fseek(file, 0x18, SEEK_SET) == 0 &&
	    fread(&leshort, sizeof(leshort), 1, file) == 1 &&
	    le16toh(leshort) > 0x3f &&
	    fseek(file, 0x3c, SEEK_SET) == 0 &&
	    fread(&lelong, sizeof(lelong), 1, file) == 1 &&
	    fseek(file, le32toh(lelong), SEEK_SET) == 0 &&
	    fread(pe, sizeof(pe), 1, file) == 1 &&
	    memcmp(pe, "PE" "\x00\x00", 4) == 0)
	{
		uint32_t lelong1, lelong2;

		if (fseek(file, le32toh(lelong) + 0xf8, SEEK_SET) == 0 &&
		    search(file, ".rsrc", 0x140))
		{
			if (fseek(file, 0x0f, SEEK_CUR) == 0 &&
			    fread(&lelong1, sizeof(lelong1), 1, file) == 1 &&
			    fseek(file, -sizeof(lelong1) - 4, SEEK_CUR) == 0 &&
			    fread(&lelong2, sizeof(lelong2), 1, file) == 1)
			{
				/* self-extracting Nullsoft Installer */
				if (fseek(file, le32toh(lelong1) + le32toh(lelong2), SEEK_SET) == 0 &&
				    search(file, "Nullsoft", 32))
				{
					xa.type = XARCHIVETYPE_7ZIP;
					xa.tag = 'n';
					goto done;
				}
			}
		}
	}

done:
	fseek(file, 0, SEEK_SET);

	return xa;
}
