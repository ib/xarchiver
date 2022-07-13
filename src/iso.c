/*
 *  Copyright (C) 2022 Ingo Brückl
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

#include <string.h>
#include "iso.h"

static gboolean iso_magic (FILE *file, long offset)
{
	unsigned char magic[5];
	gboolean result;

	result = (fseek(file, offset, SEEK_CUR) == 0 &&
	          fread(magic, sizeof(magic), 1, file) == 1 &&
	          memcmp(magic, "CD001", sizeof(magic)) == 0);

	fseek(file, -sizeof(magic), SEEK_CUR);

	return result;
}

gboolean isISO (FILE *file)
{
	gboolean result = FALSE;

	fseek(file, 0, SEEK_SET);

	if (!result)
		result = iso_magic(file, 0x8001);

	if (!result)
		result = iso_magic(file, 0x0800);   // at 0x8801?

	if (!result)
		result = iso_magic(file, 0x0800);   // at 0x9001?

	fseek(file, 0, SEEK_SET);

	return result;
}
