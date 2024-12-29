/*
 *  Copyright (C) 2024 Ingo Brückl
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
#include "appimage.h"

gboolean isAppImage (FILE *file)
{
	gboolean success = FALSE;
	long offset;
	size_t bytes;
	char buffer[11];

	fseek(file, 0, SEEK_SET);

	if (fread(buffer, sizeof(buffer), 1, file) == 1)
	{
		if (memcmp(buffer + 1, "ELF", 3) == 0 && memcmp(buffer + 8, "AI\x02", 3) == 0)
		{
			offset = (buffer[4] == 1 ? 52 : (buffer[4] == 2 ? 64 : 0));

			if (offset > 0)
			{
				/* skip ELF header */
				fseek(file, offset, SEEK_SET);

				/* seek the SquashFS */
				while ((bytes = fread(buffer, 4, 1, file)) == 1)
				{
					if (memcmp(buffer, "hsqs", 4) == 0)
					{
						success = TRUE;
						break;
					}
				}
			}
		}
	}

	fseek(file, 0, SEEK_SET);

	return success;
}
