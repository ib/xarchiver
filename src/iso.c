/*
 *  Copyright 1993 Yggdrasil Computing, Incorporated
 *  Copyright (c) 1999-2004 J. Schilling
 *  Copyright (C) 2006 Giuseppe Torelli <colossus73@gmail.com>
 *  Copyright (c) 2006 Salvatore Santagati <salvatore.santagati@gmail.com>
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
 */

#include "iso.h"

unsigned char buffer[2048];

struct iso_primary_descriptor ipd;
struct iso_directory_record * idr;
struct iso_directory_record * idr_rr;

gboolean use_rock;
gboolean use_joilet;
char * xtract = 0; 
int do_find = 0;    
int ucs_level = 0;

unsigned int sector_offset = 0;

struct todo * todo_idr = NULL;

char *rootname = "/";
char name_buf[256];
struct stat fstat_buf;

char xname[256];
unsigned char date_buf[9];

gchar *g_file_name;
gchar *g_file_permissions;
gchar *g_file_date;
unsigned long long int g_file_size;
unsigned long long int g_file_offset;

int	su_version = 0;
int	aa_version = 0;

int	found_rr;

char *months[12] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

int parse_rr(unsigned char * pnt, int len, int cont_flag)
{
	int slen;
	int xlen;
	int ncount;
	int extent;
	int cont_extent, cont_offset, cont_size;
	int flag1, flag2;
	unsigned char *pnts;
	char symlinkname[1024];
	int goof;

	symlinkname[0] = 0;

	cont_extent = cont_offset = cont_size = 0;
	ncount = 0;
	flag1 = flag2 = 0;
	while (len >= 4) {
		if (pnt[3] != 1 && pnt[3] != 2) {
			printf("**BAD RRVERSION (%d)\n", pnt[3]);
			return (0);		/* JS ??? Is this right ??? */
		}
		ncount++;
		if (pnt[0] == 'R' && pnt[1] == 'R') flag1 = pnt[4] & 0xff;
		if (strncmp((char *)pnt, "PX", 2) == 0) flag2 |= 1;	/* POSIX attributes */
		if (strncmp((char *)pnt, "PN", 2) == 0) flag2 |= 2;	/* POSIX device number */
		if (strncmp((char *)pnt, "SL", 2) == 0) flag2 |= 4;	/* Symlink */
		if (strncmp((char *)pnt, "NM", 2) == 0) flag2 |= 8;	/* Alternate Name */
		if (strncmp((char *)pnt, "CL", 2) == 0) flag2 |= 16;	/* Child link */
		if (strncmp((char *)pnt, "PL", 2) == 0) flag2 |= 32;	/* Parent link */
		if (strncmp((char *)pnt, "RE", 2) == 0) flag2 |= 64;	/* Relocated Direcotry */
		if (strncmp((char *)pnt, "TF", 2) == 0) flag2 |= 128;	/* Time stamp */
		if (strncmp((char *)pnt, "SP", 2) == 0) {
			flag2 |= 1024;					/* SUSP record */
			su_version = pnt[3] & 0xff;
		}
		if (strncmp((char *)pnt, "AA", 2) == 0)
		{
			flag2 |= 2048;					/* Apple Signature record */
			aa_version = pnt[3] & 0xff;
		}

		if (strncmp((char *)pnt, "PX", 2) == 0)
		{		/* POSIX attributes */
			fstat_buf.st_mode = iso_733(pnt+4);
			fstat_buf.st_nlink = iso_733(pnt+12);
			fstat_buf.st_uid = iso_733(pnt+20);
			fstat_buf.st_gid = iso_733(pnt+28);
		}

		if (strncmp((char *)pnt, "NM", 2) == 0)
		{		/* Alternate Name */
	  		strncpy(name_buf, pnt+5, pnt[2] - 5);
		        name_buf[pnt[2] - 5] = 0;
		}

		if (strncmp((char *)pnt, "CE", 2) == 0)
		{		/* Continuation Area */
			cont_extent = iso_733(pnt+4);
			cont_offset = iso_733(pnt+12);
			cont_size = iso_733(pnt+20);
		}

		if (strncmp((char *)pnt, "PL", 2) == 0 || strncmp((char *)pnt, "CL", 2) == 0) {
			extent = iso_733(pnt+4);
		}

		if (strncmp((char *)pnt, "SL", 2) == 0)
		{		/* Symlink */
			int	cflag;

			cflag = pnt[4];
			pnts = pnt+5;
			slen = pnt[2] - 5;
			while (slen >= 1) {
				switch (pnts[0] & 0xfe) {
				case 0:
					strncat(symlinkname, (char *)(pnts+2), pnts[1]);
					symlinkname[pnts[1]] = 0;
					break;
				case 2:
					strcat(symlinkname, ".");
					break;
				case 4:
					strcat(symlinkname, "..");
					break;
				case 8:
					strcat(symlinkname, "/");
					break;
				case 16:
					strcat(symlinkname, "/mnt");
					printf("Warning - mount point requested");
					break;
				case 32:
					strcat(symlinkname, "kafka");
					printf("Warning - host_name requested");
					break;
				default:
					printf("Reserved bit setting in symlink");
					goof++;
					break;
				}
				if ((pnts[0] & 0xfe) && pnts[1] != 0) {
					printf("Incorrect length in symlink component");
				}
				if (xname[0] == 0) strcpy(xname, "-> ");
				strcat(xname, symlinkname);
				symlinkname[0] = 0;
				xlen = strlen(xname);
				if ((pnts[0] & 1) == 0 && xname[xlen-1] != '/') strcat(xname, "/");

				slen -= (pnts[1] + 2);
				pnts += (pnts[1] + 2);
			}
			symlinkname[0] = 0;
		}

		len -= pnt[2];
		pnt += pnt[2];
		if (len <= 3 && cont_extent) {
			unsigned char	sector[2048];


		lseek(fileno(iso_stream), ((off_t)(cont_extent - sector_offset)) << 11, SEEK_SET);
		read(fileno(iso_stream), sector, sizeof (sector));

		flag2 |= parse_rr(&sector[cont_offset], cont_size, 1);
		}
	}
	/*
	 * for symbolic links, strip out the last '/'
	 */
	if (xname[0] != 0 && xname[strlen(xname)-1] == '/') {
		xname[strlen(xname)-1] = '\0';
	}
	return (flag2);
}

void find_rr(idr, pntp, lenp)
	struct iso_directory_record *idr;
	unsigned char	**pntp;
	int	*lenp;
{
	struct iso_xa_dir_record *xadp;
	int len;
	unsigned char * pnt;

	len = idr->length[0] & 0xff;
	len -= offsetof(struct iso_directory_record, name[0]);
	len -= idr->name_len[0];

	pnt = (unsigned char *) idr;
	pnt += offsetof(struct iso_directory_record, name[0]);
	pnt += idr->name_len[0];
	if ((idr->name_len[0] & 1) == 0) {
		pnt++;
		len--;
	}
	if (len >= 14) {
		xadp = (struct iso_xa_dir_record *)pnt;

		if (xadp->signature[0] == 'X' && xadp->signature[1] == 'A' &&
		    xadp->reserved[0] == '\0') {
			len -= 14;
			pnt += 14;
		}
	}
	*pntp = pnt;
	*lenp = len;
}


int dump_rr(idr)
	struct iso_directory_record *idr;
{
	int len;
	unsigned char * pnt;

	find_rr(idr, &pnt, &len);
	return (parse_rr(pnt, len, 0));
}


void dump_stat(gchar *dir_name , int extent, XArchive *archive)
{
  int i;
  char outline[80];

  GValue *filename = NULL;
  GValue *permissions = NULL;
  GValue *size = NULL;
  GValue *date = NULL;
  GValue *offset = NULL;

  memset(outline, ' ', sizeof(outline));

  if(S_ISREG(fstat_buf.st_mode))
    outline[0] = '-';
  else   if(S_ISDIR(fstat_buf.st_mode))
    outline[0] = 'd';
  else   if(S_ISLNK(fstat_buf.st_mode))
    outline[0] = 'l';
  else   if(S_ISCHR(fstat_buf.st_mode))
    outline[0] = 'c';
  else   if(S_ISBLK(fstat_buf.st_mode))
    outline[0] = 'b';
  else   if(S_ISFIFO(fstat_buf.st_mode))
    outline[0] = 'f';
  else   if(S_ISSOCK(fstat_buf.st_mode))
    outline[0] = 's';
  else
    outline[0] = '?';

  memset(outline+1, '-', 9);
  if( fstat_buf.st_mode & S_IRUSR )
    outline[1] = 'r';
  if( fstat_buf.st_mode & S_IWUSR )
    outline[2] = 'w';
  if( fstat_buf.st_mode & S_IXUSR )
    outline[3] = 'x';

  if( fstat_buf.st_mode & S_IRGRP )
    outline[4] = 'r';
  if( fstat_buf.st_mode & S_IWGRP )
    outline[5] = 'w';
  if( fstat_buf.st_mode & S_IXGRP )
    outline[6] = 'x';

  if( fstat_buf.st_mode & S_IROTH )
    outline[7] = 'r';
  if( fstat_buf.st_mode & S_IWOTH )
    outline[8] = 'w';
  if( fstat_buf.st_mode & S_IXOTH )
    outline[9] = 'x';

  g_sprintf(outline+11, "%3d", fstat_buf.st_nlink); 
  g_sprintf(outline+15, "%4o", fstat_buf.st_uid);  
  g_sprintf(outline+20, "%4o", fstat_buf.st_gid); 
  
  g_file_size = fstat_buf.st_size ;

	if (date_buf[1] >= 1 && date_buf[1] <= 12)
		memcpy(outline+41, months[date_buf[1]-1], 3);

	sprintf(outline+45, "%2d", date_buf[2]);
		outline[63] = 0;
	sprintf(outline+48, "%4d", date_buf[0]+1900);

	sprintf(outline+53, "[%7d", extent);	/* XXX up to 20 GB */
	sprintf(outline+61, " %02X]", idr->flags[0]);

	for (i = 0; i < 66; i++) {
		if (outline[i] == 0) outline[i] = ' ';
	}
	outline[66] = 0;

  g_file_date = g_strndup (&outline[41] , 11);

  for(i=0; i<10; i++)
    if(outline[i] == 0) outline[i] = ' ';
  outline[10] = 0;

  g_file_permissions = outline;
  g_file_offset = extent;
  g_file_size = fstat_buf.st_size;

    if ( (!use_rock) && (!use_joilet) )
		strcpy (name_buf + strlen (name_buf)- 2, "  "); /* remove ";1" from file name */
	
	g_file_name = g_strconcat (dir_name, name_buf , NULL);     
	if (outline[0] == 'd')
	{
		if (strcmp (name_buf,"..") == 0 || strcmp (name_buf,".") == 0)
		{
			g_free (g_file_name);
			return;
		}
		unsigned long long zero = 0;
		gchar *empty = " ";

		filename    = g_new0(GValue, 1);
		permissions = g_new0(GValue, 1);
		size        = g_new0(GValue, 1);
		date        = g_new0(GValue, 1);
		offset      = g_new0(GValue, 1);
		
		filename = g_value_init(filename, G_TYPE_STRING);
		g_value_set_string ( filename , g_strdup (g_file_name));

		permissions = g_value_init(permissions, G_TYPE_STRING);
		g_value_set_string ( permissions , g_strdup ( empty ));
		
		size = g_value_init(size, G_TYPE_UINT64);
		g_value_set_uint64 ( size , zero );

		date = g_value_init(date, G_TYPE_STRING);
		g_value_set_string ( date , g_strdup ( empty ));

		offset = g_value_init(offset, G_TYPE_UINT64);
		g_value_set_uint64 ( offset , zero );

		archive->nr_of_dirs++;
	}

	else
	{
		filename    = g_new0(GValue, 1);
		permissions = g_new0(GValue, 1);
		size        = g_new0(GValue, 1);
		date        = g_new0(GValue, 1);
		offset      = g_new0(GValue, 1);

		filename = g_value_init(filename, G_TYPE_STRING);
		g_value_set_string ( filename , g_strdup ( g_file_name ));

		permissions = g_value_init(permissions, G_TYPE_STRING);
		g_value_set_string ( permissions , g_strdup ( g_file_permissions ));
		
		size = g_value_init(size, G_TYPE_UINT64);
		g_value_set_uint64 ( size , g_file_size );

		date = g_value_init(date, G_TYPE_STRING);
		g_value_set_string ( date , g_strdup ( g_file_date ));
		g_free (g_file_date);

		offset = g_value_init(offset, G_TYPE_UINT64);
		g_value_set_uint64 ( offset , g_file_offset );
		
		archive->nr_of_files++;
	}
	g_free (g_file_name);
	archive->dummy_size+= g_value_get_uint64 (size);

	archive->row = g_list_prepend (archive->row , filename );
	archive->row = g_list_prepend (archive->row , permissions);
	archive->row = g_list_prepend (archive->row,  size);
	archive->row = g_list_prepend (archive->row,  date);
	archive->row = g_list_prepend (archive->row,  offset);
}

void parse_dir (gchar *dir_name , int extent, int len, XArchive *archive)
{
	char testname[256];
	struct todo * td;
	int i;
	struct iso_directory_record * idr;

	while(len > 0 )
	{
		lseek(fileno(iso_stream), (extent - sector_offset) << 11, 0);
		read(fileno(iso_stream), buffer, sizeof(buffer));
		len -= sizeof(buffer);
		extent++;
		i = 0;
		while( 1==1 )
		{
			idr = (struct iso_directory_record *) &buffer[i];
			if (idr->length[0] == 0)
				break;
			memset(&fstat_buf, 0, sizeof(fstat_buf));
			name_buf[0] = xname[0] = 0;
			fstat_buf.st_size = iso_733((unsigned char *)idr->size);
			if( idr->flags[0] & 2)
				fstat_buf.st_mode |= S_IFDIR;
			else
				fstat_buf.st_mode |= S_IFREG;
			if(idr->name_len[0] == 1 && idr->name[0] == 0)
				strcpy(name_buf, ".");
			else if(idr->name_len[0] == 1 && idr->name[0] == 1)
				strcpy(name_buf, "..");
			else
			{
				switch (ucs_level)
				{
					case 3:
					/*
					* Unicode name.  Convert as best we can.
					*/
					{
						int i;
						for(i=0; i < idr->name_len[0] / 2; i++)
						{
							name_buf[i] = idr->name[i*2+1];
						}
						name_buf[idr->name_len[0]/2] = '\0';
					}
					break;
					case 0:
					/*
					* Normal non-Unicode name.
					*/
					strncpy(name_buf, idr->name, idr->name_len[0]);
					name_buf[idr->name_len[0]] = 0;
					break;
	    
					/*
					* Don't know how to do these yet.  Maybe they are the same
					* as one of the above.
					*/
				}
			};
			memcpy(date_buf, idr->date, 9);
			if(use_rock) dump_rr(idr);
			if(   (idr->flags[0] & 2) != 0
			&& (idr->name_len[0] != 1
	       || (idr->name[0] != 0 && idr->name[0] != 1)))
		  {
		    /*
			 * Add this directory to the todo list.
			*/
			td = todo_idr;
			if( td != NULL ) 
		    {
				while(td->next != NULL)
					td = td->next;
				td->next = (struct todo *) malloc(sizeof(*td));
				td = td->next;
			}
			else
			{
				todo_idr = td = (struct todo *) malloc(sizeof(*td));
			}
			td->next = NULL;
			td->extent = iso_733((unsigned char *)idr->extent);
			td->length = iso_733((unsigned char *)idr->size);
			td->name = (char *) malloc(strlen(rootname) 
				       + strlen(name_buf) + 2);
			strcpy(td->name, rootname);
			strcat(td->name, name_buf);
			strcat(td->name, "/");
		}
		else
		{
		    strcpy(testname, rootname);
			strcat(testname, name_buf);
		}
		if( do_find
	   && (idr->name_len[0] != 1
	       || (idr->name[0] != 0 && idr->name[0] != 1)))
		{
			strcpy(testname, rootname);
			strcat(testname, name_buf);
		}
		dump_stat(dir_name , iso_733((unsigned char *)idr->extent), archive);
		i += buffer[i];
		if (i > 2048 - sizeof(struct iso_directory_record))
			break;
		}
    }
}

gboolean xa_extract_iso_file (XArchive *archive, gchar *destination_path, gchar *filename , unsigned long long int file_size, unsigned long long file_offset )
{
	FILE *fdest;
	FILE *fsource;
	gchar *_filename;
	unsigned long long int tlen;
	char buf[2048];
		
	_filename = g_strconcat (destination_path , filename , NULL);

	if ((fdest = fopen (_filename, "w")) == NULL)
	{
		g_free (_filename);
		return FALSE;
	}
	g_free (_filename);

	if ((fsource = fopen (archive->path, "r")) == NULL)
		return FALSE;

	while (file_size > 0)
	{
		fseek(fsource, (off_t)file_offset << 11, SEEK_SET);
		tlen = (file_size > sizeof (buf) ? sizeof (buf) : file_size);
		fread (buf, 1 , tlen , fsource);
		file_size -= tlen;
		file_offset++;
		fwrite (buf, 1 , tlen , fdest);
	}

	fclose(fdest);
	fclose(fsource);
	return TRUE;
}

void OpenISO ( XArchive *archive )
{
    archive->dummy_size = 0;
    archive->nr_of_files = 0;
    archive->nr_of_dirs = 0;
	char *names[]= {(_("Filename")),(_("Permission")),(_("Size")),(_("Date")),(_("Offset"))};
	GType types[]= {G_TYPE_STRING,G_TYPE_STRING,G_TYPE_UINT64,G_TYPE_STRING,G_TYPE_UINT64};
	xa_create_liststore ( 5, names , (GType *)types );
	int	c;
	struct todo * td;
	int extent;
	int block;	
	int toc_offset = 0;

	iso_stream = fopen ( archive->path ,"r" );
	
	lseek(fileno(iso_stream),(16 + toc_offset) <<11, 0);    
  	read(fileno(iso_stream), &ipd, sizeof(ipd));

  	idr = (struct iso_directory_record *) &ipd.root_directory_record;

	extent = iso_733((unsigned char *)idr->extent);
	
	lseek(fileno(iso_stream),((off_t)(extent - sector_offset)) <<11, SEEK_SET);
	
	read(fileno(iso_stream), buffer, sizeof (buffer));
	idr_rr = (struct iso_directory_record *) buffer;
 
	/* Detect Rock Ridge exstension */
	if ((c = dump_rr(idr_rr)) != 0) 
	{			
		if (c & 1024) 
		{
			use_rock = TRUE;			
			g_print ("Rock Ridge signatures version %d found\n",su_version);	
		} 
		else 
			g_print ("Bad Rock Ridge signatures found (SU record missing)\n");
		     
		/*
		 * This is currently a no op!
		 * We need to check the first plain file instead of
		 * the '.' entry in the root directory.
		 */
		if (c & 2048) 
			g_print ("Apple signatures version %d found\n",aa_version);
	}				
		else 
			g_print ("NO Rock Ridge present\n");


		if( ! use_rock)
		{ 
			lseek(fileno(iso_stream), (16 + toc_offset) <<11, 0);
			read(fileno(iso_stream), &ipd, sizeof(ipd));
      
    		block = 16;

      		while( (unsigned char) ipd.type[0] != ISO_VD_END )
			{
				/*
				* Find the UCS escape sequence.
				*/
				if(    ipd.escape_sequences[0] == '%'
						&& ipd.escape_sequences[1] == '/'
		      			&& ipd.escape_sequences[3] == '\0'
		      			&& (    ipd.escape_sequences[2] == '@'
			   	   || ipd.escape_sequences[2] == 'C'
				   || ipd.escape_sequences[2] == 'E') )
					break;

				block++;
				lseek(fileno(iso_stream), (block + sector_offset) <<11, 0);
				read(fileno(iso_stream), &ipd, sizeof(ipd));
			}
		
      		if( (unsigned char) ipd.type[0] == ISO_VD_END )
				g_print ("Unable to find Joliet SVD\n");
			else 
			{
				use_joilet = 1;
				g_print ("This image is Joilet\n");
			}

			switch(ipd.escape_sequences[2])
			{
				case '@':
				ucs_level = 1;
				break;
				case 'C':
				ucs_level = 2;
				break;
				case 'E':
				ucs_level = 3;
				break;
			}

			/*if( ucs_level < 3 )
				g_print ("Don't know what ucs_level == %d means\n", ucs_level);*/
		}
		idr = (struct iso_directory_record *) &ipd.root_directory_record;
		if (!use_joilet) 
		{
			lseek(fileno(iso_stream),(16 + toc_offset) <<11, 0);   
			read(fileno(iso_stream), &ipd, sizeof(ipd));
		}
	parse_dir ("/" , iso_733((unsigned char *)idr->extent), iso_733((unsigned char *)idr->size), archive);
	xa_append_rows ( archive , 5 );
	td = todo_idr;	

	while(td)
	{	
		rootname = td->name;
		parse_dir( rootname , td->extent, td->length, archive);
		xa_append_rows ( archive , 5 );
		td = td->next;
	}
	fclose(iso_stream);
	use_rock = FALSE;
	use_joilet = FALSE;
	SetButtonState (1,1,0,0,1);
	OffTooltipPadlock();
	gtk_widget_set_sensitive ( properties , TRUE );
	Update_StatusBar ( _("Operation completed.") );
}

int iso_723 ( unsigned char *p)
{
    return (( p[0] & 0xff) | ((p[1] & 0xff) << 8));
}

int iso_731 ( unsigned char *p)
{
    return ((p[0] & 0xff) | ((p[1] & 0xff) << 8) | ((p[2] & 0xff) << 16) | ((p[3] & 0xff) << 24));
}

int iso_733 (unsigned char * p)
{
	return (iso_731 (p));
}

int DetectImage (FILE *iso)
{
	char buf[12];

	fseek (iso, 0L, SEEK_SET);
	fseek (iso, 32768, SEEK_CUR);
	fread (buf, sizeof (char), 8, iso);
	if (memcmp ("\x01\x43\x44\x30\x30\x31\x01\x00", buf, 8))
	{
		fseek (iso, 0L, SEEK_SET);
		fread (buf, sizeof (char), 12, iso);
		if (!memcmp ("\x00\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\x00", buf, 12))
		{
			fseek (iso, 0L, SEEK_SET);
			fseek (iso, 2352, SEEK_CUR);
			fread (buf, sizeof (char), 12, iso);
			if (!memcmp ("\x80\xC0\x80\x80\x80\x80\x80\xC0\x80\x80\x80\x80", buf, 12))
				return (39184);
			else
				return(37648);
		}
	else
		return (-1);
	}
	else
		return(32768);
}

