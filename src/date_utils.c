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
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 *  MA 02110-1301 USA.
 */

#include <stdio.h>
#include <string.h>
#include <time.h>
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

	memcpy(iso8601 + 5, mm, 2);
	memcpy(iso8601 + 8, date + 4, 2);

	if (iso8601[8] == ' ')
		iso8601[8] = '0';

	if (date[9] == ':')
	{
		time_t now;
		struct tm *current;
		gchar mm_dd[6], yyyy[5];

		time(&now);
		current = localtime(&now);
		sprintf(mm_dd, "%02u-%02u", current->tm_mon + 1, current->tm_mday);

		if (strcmp(iso8601 + 5, mm_dd) > 0)
			current->tm_year--;

		sprintf(yyyy,"%04u", current->tm_year + 1900);
		memcpy(iso8601, yyyy, 4);
	}
	else
		memcpy(iso8601, date + (date[7] == ' ' ? 8 : 7), 4);

	return iso8601;
}

gchar *date_YY_MM_DD (const gchar *date)
{
	static gchar iso8601[11];
	guint yy;

	memcpy(iso8601 + 2, date, 8);
	iso8601[10] = 0;

	yy = 10 * (date[0] - '0') + (date[1] - '0');

	if (yy >= 70 && yy <= 99)
		memcpy(iso8601, "19", 2);
	else
		memcpy(iso8601, "20", 2);

	return iso8601;
}

gchar *date_DD_MM_YY (const gchar *date)
{
	gchar yy_mm_dd[9];

	yy_mm_dd[0] = date[6];
	yy_mm_dd[1] = date[7];
	yy_mm_dd[2] = '-';
	yy_mm_dd[3] = date[3];
	yy_mm_dd[4] = date[4];
	yy_mm_dd[5] = '-';
	yy_mm_dd[6] = date[0];
	yy_mm_dd[7] = date[1];
	yy_mm_dd[8] = 0;

	return date_YY_MM_DD(yy_mm_dd);
}

gchar *date_YY_MMM_DD (const gchar *date)
{
	gchar yy_mm_dd[9], mm[3];

	yy_mm_dd[0] = date[0];
	yy_mm_dd[1] = date[1];
	yy_mm_dd[2] = '-';

	sprintf(mm,"%02u", month(date + 3));

	yy_mm_dd[3] = mm[0];
	yy_mm_dd[4] = mm[1];
	yy_mm_dd[5] = '-';
	yy_mm_dd[6] = date[7];
	yy_mm_dd[7] = date[8];
	yy_mm_dd[8] = 0;

	return date_YY_MM_DD(yy_mm_dd);
}
