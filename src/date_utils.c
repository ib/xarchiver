/*
 *  Copyright (C) 2020 Ingo Brückl
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

#include <stdio.h>
#include <string.h>
#include "date_utils.h"

static guint month (const gchar *date)
{
	if (strncmp(date, "Jan", 3) == 0) return 1;
	if (strncmp(date, "Feb", 3) == 0) return 2;
	if (strncmp(date, "Mar", 3) == 0) return 3;
	if (strncmp(date, "Apr", 3) == 0) return 4;
	if (strncmp(date, "May", 3) == 0) return 5;
	if (strncmp(date, "Jun", 3) == 0) return 6;
	if (strncmp(date, "Jul", 3) == 0) return 7;
	if (strncmp(date, "Aug", 3) == 0) return 8;
	if (strncmp(date, "Sep", 3) == 0) return 9;
	if (strncmp(date, "Oct", 3) == 0) return 10;
	if (strncmp(date, "Nov", 3) == 0) return 11;
	if (strncmp(date, "Dec", 3) == 0) return 12;

	return 0;
}

gchar *date_MMM_dD_HourYear (const gchar *date)
{
	static gchar iso8601[11];
	gchar mm[3];

	iso8601[4] = '-';
	iso8601[7] = '-';
	iso8601[10] = 0;

	sprintf(mm,"%02u", month(date));

	strncpy(iso8601 + 5, mm, 2);
	strncpy(iso8601 + 8, date + 4, 2);

	if (iso8601[8] == ' ')
		iso8601[8] = '0';

	if (date[9] == ':')
	{
		// to do: HH:MM -> YYYY
	}
	else
		strncpy(iso8601, date + 7, 4);

	return iso8601;
}

gchar *date_YY_MM_DD (const gchar *date)
{
	static gchar iso8601[11];
	guint yy;

	strncpy(iso8601 + 2, date, 8);
	iso8601[10] = 0;

	yy = 10 * (date[0] - '0') + (date[1] - '0');

	if (yy >= 70 && yy <= 99)
		strncpy(iso8601, "19", 2);
	else
		strncpy(iso8601, "20", 2);

	return iso8601;
}
