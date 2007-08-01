/*
 *  Copyright (c) 2007 Giuseppe Torelli <colossus73@gmail.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 */

#include "mime.h"
#include <string.h>

const char *xa_get_stock_mime_icon(char *filename)
{
	const char *mime;
	const char *icon_name = "binary";

	mime = xdg_mime_get_mime_type_from_file_name(filename);
	//g_print ("%s\t%s\n",filename,mime);
	if (strncmp(mime,"image/",6) == 0)
		icon_name = "image";
	else if (strcmp(mime,"text/html") == 0)
		icon_name = "html";
	else if (strncmp(mime,"text/",5) == 0)
		icon_name = "txt";
	else if (strcmp(mime,"application/rtf") == 0 || strcmp(mime,"application/pdf") == 0 || strcmp(mime,"application/msword") == 0) 
		icon_name = "document";
	else if (strcmp(mime,"audio/mpeg") == 0 || strcmp(mime,"audio/midi") == 0 )
		icon_name = "sound";
	else if (strcmp(mime,"application/vnd.ms-excel") == 0)
		icon_name = "gnome-mime-application-vnd.ms-excel";
	else if (strcmp(mime,"application/zip") == 0 || strcmp(mime,"application/x-rar") == 0 || strcmp(mime,"application/x-tar") == 0
		|| strcmp(mime,"application/x-7z-compressed") == 0)
		icon_name = "package";
	else if (strcmp(mime,"application/x-shockwave-flash") == 0 || strcmp(mime,"video/mpeg") == 0 || strcmp(mime,"video/quicktime") == 0
		|| strcmp(mime,"video/x-msvideo") == 0)
		icon_name = "video";
	else if (strcmp(mime,"application/x-cd-image") == 0)
		icon_name = "application-x-cd-image";
	else if (strcmp(mime,"application/x-php") == 0)
		icon_name = "gnome-mime-application-x-php";
	else if (strcmp(mime,"application/x-perl") == 0 || strcmp (mime,"application/x-csh") == 0 || strcmp (mime,"application/x-shellscript") == 0)
		icon_name = "gnome-mime-application-x-perl";

	return icon_name;		
}

