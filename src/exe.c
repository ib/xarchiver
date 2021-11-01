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
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 *  MA 02110-1301 USA.
 */

#ifdef __APPLE__
#include <libkern/OSByteOrder.h>
#define le32toh(x) OSSwapLittleToHostInt32(x)
#elif defined(__FreeBSD__)
#include <sys/endian.h>
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
	uint16_t magic, offset_rlctbl;
	uint32_t pe_start;
	char pe[4], buffer[9];

	if (fseek(file, 0, SEEK_SET) == 0 &&
	    fread(&magic, sizeof(magic), 1, file) == 1 &&
	    memcmp(&magic, "MZ", 2) == 0 &&
	    fseek(file, 0x18, SEEK_SET) == 0 &&
	    fread(&offset_rlctbl, sizeof(offset_rlctbl), 1, file) == 1 &&
	    le16toh(offset_rlctbl) > 0x3f &&
	    fseek(file, 0x3c, SEEK_SET) == 0 &&
	    fread(&pe_start, sizeof(pe_start), 1, file) == 1 &&
	    fseek(file, le32toh(pe_start), SEEK_SET) == 0 &&
	    fread(pe, sizeof(pe), 1, file) == 1 &&
	    memcmp(pe, "PE" "\x00\x00", 4) == 0)
	{
		/* a section starts at offset with a section directory */
		uint32_t dirsize, offset;

		if (fseek(file, le32toh(pe_start) + 0xf8, SEEK_SET) == 0 &&
		    search(file, ".rsrc", 0x140))
		{
			if (fseek(file, (8 - 5) + 0x08, SEEK_CUR) == 0 &&
			    fread(&dirsize, sizeof(dirsize), 1, file) == 1 &&
			    fread(&offset, sizeof(offset), 1, file) == 1)
			{
				/* self-extracting Nullsoft Installer */
				if (fseek(file, le32toh(offset) + le32toh(dirsize), SEEK_SET) == 0 &&
				    search(file, "Nullsoft", 32))
				{
					xa.type = XARCHIVETYPE_7ZIP;
					xa.tag = 'n';
					goto done;
				}

				/* self-extracting 7-ZIP archive */
				if (fseek(file, le32toh(offset) + le32toh(dirsize), SEEK_SET) == 0 &&
				    fread(buffer, 6, 1, file) == 1 && memcmp(buffer, "7z" "\xbc\xaf\x27\x1c", 6) == 0)
				{
					xa.type = XARCHIVETYPE_7ZIP;
					goto done;
				}

				/* self-extracting RAR archive */
				if (fseek(file, le32toh(offset) + le32toh(dirsize), SEEK_SET) == 0 &&
				    fread(buffer, 4, 1, file) == 1 && memcmp(buffer, "Rar!", 4) == 0)
				{
					xa.type = XARCHIVETYPE_RAR;
					goto done;
				}
			}
		}

		if (fseek(file, le32toh(pe_start) + 0xf8, SEEK_SET) == 0 &&
		    search(file, "UPX2", 0x140))
		{
			if (fseek(file, (8 - 4) + 0x08, SEEK_CUR) == 0 &&
			    fread(&dirsize, sizeof(dirsize), 1, file) == 1 &&
			    fread(&offset, sizeof(offset), 1, file) == 1)
			{
				/* self-extracting (Info-)ZIP archive */
				if (fseek(file, le32toh(offset) + le32toh(dirsize), SEEK_SET) == 0 &&
				    fread(buffer, 4, 1, file) == 1 && memcmp(buffer, "PK" "\x03\x04", 4) == 0)
				{
					xa.type = XARCHIVETYPE_ZIP;
					goto done;
				}
			}
		}

		/* self-extracting ARJ archive */
		if (fseek(file, 0x20, SEEK_SET) == 0 && search(file, "aRJsfX", 0xe0))
		{
			xa.type = XARCHIVETYPE_ARJ;
			goto done;
		}

		/* self-extracting LHA archive */
		if (fseek(file, 0x24, SEEK_SET) == 0 && fread(buffer, 9, 1, file) == 1 &&
		    g_ascii_strncasecmp("LHA's SFX", buffer, 9) == 0)
		{
			xa.type = XARCHIVETYPE_LHA;
			goto done;
		}
	}

done:
	fseek(file, 0, SEEK_SET);

	return xa;
}
