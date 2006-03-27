/*
 * File isodump.c - dump iso9660 directory information.
 *
 *
 * Written by Eric Youngdale (1993).
 *
 * Copyright 1993 Yggdrasil Computing, Incorporated
 * Copyright (c) 1999-2004 J. Schilling
 * Copyright (c) 2006 Xarchiver Developers
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <sys/types.h>
#include <unistd.h>
#include "iso.h"

unsigned char buffer[2048];

struct iso_primary_descriptor ipd;
struct iso_directory_record * idr;
struct iso_directory_record * idr_rr;

//int do_listing = 0;  /* imposta a 1 se vuoi far vedere i file presenti nella iso  */
int use_rock = 0;    /*  imposta a 1 se vuoi che output sia quello dei nomi con supporto RR */
char * xtract = 0;   /* imposta a 1 se vuoi estrarre un file */
int do_find = 0;     /*  imposta a 1 se cerchi un file */

unsigned int sector_offset = 0;

struct todo * todo_idr = NULL;

int seek_ecc = 0;
int sector_size = 0;
int sector_data = 2048;
int seek_head = 16;

char * rootname = "/";
char name_buf[256];
struct stat fstat_buf;

char xname[256];
unsigned char date_buf[9];
gchar * g_file_name;

int	su_version = 0;
int	aa_version = 0;


int		found_rr;

char * months[12] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul",
		       "Aug", "Sep", "Oct", "Nov", "Dec"};



const char SYNC_HEADER_IMAGE[12] = { (char) 0x00,
  				     (char) 0xFF,
  				     (char) 0xFF,
  				     (char) 0xFF,
  				     (char) 0xFF,
  				     (char) 0xFF,
  				     (char) 0xFF,
  				     (char) 0xFF,
                                     (char) 0xFF,
                                     (char) 0xFF,
                                     (char) 0xFF,
                                     (char) 0x00
};

const char SYNC_HEADER_IMAGE_2448[12] = { (char) 0x80,
  					  (char) 0xC0,
  					  (char) 0x80,
  					  (char) 0x80,
  					  (char) 0x80,
  					  (char) 0x80,
					  (char) 0x80,
  					  (char) 0xC0,
  					  (char) 0x80,
  					  (char) 0x80,
  					  (char) 0x80,
  					  (char) 0x80
};

const char ISO_9660[8] = { (char) 0x01,
  			   (char) 0x43,
  			   (char) 0x44,
  			   (char) 0x30,
  			   (char) 0x30,
  			   (char) 0x31,
  			   (char) 0x01,
  			   (char) 0x00
};

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
		if (strncmp((char *)pnt, "AA", 2) == 0) {
			flag2 |= 2048;					/* Apple Signature record */
			aa_version = pnt[3] & 0xff;
		}

		if (strncmp((char *)pnt, "PX", 2) == 0) {		/* POSIX attributes */
			fstat_buf.st_mode = iso_733(pnt+4);
			fstat_buf.st_nlink = iso_733(pnt+12);
			fstat_buf.st_uid = iso_733(pnt+20);
			fstat_buf.st_gid = iso_733(pnt+28);
		}

		if (strncmp((char *)pnt, "NM", 2) == 0) {		/* Alternate Name */
			int	l = strlen(name_buf);

			if (!found_rr)
				l = 0;
			strncpy(&name_buf[l], (char *)(pnt+5), pnt[2] - 5);
			name_buf[l + pnt[2] - 5] = 0;
			found_rr = 1;
		}

		if (strncmp((char *)pnt, "CE", 2) == 0) {		/* Continuation Area */
			cont_extent = iso_733(pnt+4);
			cont_offset = iso_733(pnt+12);
			cont_size = iso_733(pnt+20);
		}

		if (strncmp((char *)pnt, "PL", 2) == 0 || strncmp((char *)pnt, "CL", 2) == 0) {
			extent = iso_733(pnt+4);
		}

		if (strncmp((char *)pnt, "SL", 2) == 0) {		/* Symlink */
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

void
find_rr(idr, pntp, lenp)
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


int
dump_rr(idr)
	struct iso_directory_record *idr;
{
	int len;
	unsigned char * pnt;

	find_rr(idr, &pnt, &len);
	return (parse_rr(pnt, len, 0));
}


dump_stat(int extent)
{
  int i;
  char outline[80];
  char file_dir;

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

  g_sprintf(outline+11, "%3d", fstat_buf.st_nlink); /* link simbolico */
  g_sprintf(outline+15, "%4o", fstat_buf.st_uid);  /* Ti da UID del file **/
  g_sprintf(outline+20, "%4o", fstat_buf.st_gid);  /* ti da il gid del file*/ 
  g_sprintf(outline+33, "%8d", fstat_buf.st_size); /* Dimensione file */

  if( date_buf[1] >= 1 && date_buf[1] <= 12 )
    {
      memcpy(outline+42, months[date_buf[1]-1], 3);
    }

  sprintf(outline+46, "%2d", date_buf[2]);
  sprintf(outline+49, "%4d", date_buf[0]+1900);

  sprintf(outline+54, "[%6d]", extent);

  for(i=0; i<63; i++)
    if(outline[i] == 0) outline[i] = ' ';
  outline[63] = 0;



  if (use_rock);
	else 
		strcpy (name_buf + strlen (name_buf)- 2, "  "); /* remove ";1" from output file */
     
    g_file_name = g_strconcat (rootname, name_buf,NULL);

    if (outline[0] == 'd'); /** g_message ("DIR"); ****/
    	else g_print ("%s %s %d byte - %d UID - %d GID\n",g_file_name, xname, fstat_buf.st_size, fstat_buf.st_uid, fstat_buf.st_gid);
}

extract_file(struct iso_directory_record * idr)
{
  int extent, len, tlen;
  unsigned char buff[2048];

  extent = iso_733(idr->extent);
  len = iso_733(idr->size);

  while(len > 0)
    {
      lseek(fileno(iso_stream), (extent - sector_offset) << 11, 0);
     
      tlen = (len > sizeof(buff) ? sizeof(buff) : len);
      read(fileno(iso_stream), buff, tlen);
      len -= tlen;
      extent++;
      write(1, buff, tlen);
    }
}

parse_dir(int extent, int len){
  unsigned int k;
  char testname[256];
  struct todo * td;
  int i, j;

  struct iso_directory_record * idr;
 
  g_print ("%s\n", rootname);
    
  while(len > 0 )
    {
      
      g_print ("Sector %d\n", ((extent - sector_offset) << 11));
      lseek(fileno(iso_stream), ((extent - sector_offset) << 11) + seek_ecc, 0); 
      /*lseek(fileno(iso_stream), (extent * block) - header, 0);*/ 
      	
      read(fileno(iso_stream), buffer, sizeof(buffer));
      len -= sizeof(buffer);
      	
      extent++;
      
      i = 0;
      while(1==1){
	idr = (struct iso_directory_record *) &buffer[i];
	if(idr->length[0] == 0) break;
	memset(&fstat_buf, 0, sizeof(fstat_buf));
	name_buf[0] = xname[0] = 0;
	fstat_buf.st_size = iso_733(idr->size);
	if( idr->flags[0] & 2)
	  fstat_buf.st_mode |= S_IFDIR;
	else
	  fstat_buf.st_mode |= S_IFREG;	
	if(idr->name_len[0] == 1 && idr->name[0] == 0)
	  strcpy(name_buf, ".");
	else if(idr->name_len[0] == 1 && idr->name[0] == 1)
	  strcpy(name_buf, "..");
	else {
	  strncpy(name_buf, idr->name, idr->name_len[0]);
	  name_buf[idr->name_len[0]] = 0;
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
		while(td->next != NULL) td = td->next;
		td->next = (struct todo *) malloc(sizeof(*td));
		td = td->next;
	      }
	    else
	      {
		todo_idr = td = (struct todo *) malloc(sizeof(*td));
	      }
	    td->next = NULL;
	    td->extent = iso_733(idr->extent);
	    td->length = iso_733(idr->size);
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
	    if(xtract && strcmp(xtract, testname) == 0)
	      {
		extract_file(idr);  
	      }
	  }
	if(   do_find
	   && (idr->name_len[0] != 1
	       || (idr->name[0] != 0 && idr->name[0] != 1)))
	  {
	    strcpy(testname, rootname);
	    strcat(testname, name_buf);
	    printf("%s\n", testname);
	  }
	  dump_stat(iso_733(idr->extent));
	i += buffer[i];
	if (i > 2048 - sizeof(struct iso_directory_record)) break; 
        lseek(fileno(iso_stream), ((extent - sector_offset) << 11) + seek_head, 0); 
      }
    }
	
}

int DetectImage (FILE *iso)
{
   char buf[2448];
   
   fseek (iso, 0L, SEEK_SET);
	fseek (iso, 32768, SEEK_CUR);
	fread (buf, sizeof (char), 8, iso);
	if (memcmp (ISO_9660, buf, 8))
	  {
	  	fseek (iso, 0L, SEEK_SET);
	   fread (buf, sizeof (char), 12, iso);
	   if (!memcmp (SYNC_HEADER_IMAGE, buf, 12))
			{
			fseek (iso, 0L, SEEK_SET);
		  	fseek (iso, 2352, SEEK_CUR);
		  	fread (buf, sizeof (char), 12, iso);
		  	if (!memcmp (SYNC_HEADER_IMAGE_2448, buf, 12))
				{
				seek_ecc = 384;
			  	sector_size = 2448;
			  	sector_data = 2048;
			  	seek_head = 16;

			  	return (39184);
				}
				else
					{
					seek_ecc = 288;
			      		sector_size = 2352;
			      		sector_data = 2048;
			      		seek_head = 16;

		  			return(37648);
					}		  			
			}
			else 	
			  return(-1);
			
		}
		else
			return(32768);	  				 

}


void OpenISO ( gchar *path)
{
     int offset_image;
     int	c;
     struct todo * td;
     int extent;
     
    //Let's read the iso information first:
    iso_stream = fopen ( path ,"r" );
    if (iso_stream == NULL)
    {
        gchar *msg = g_strdup_printf (_("Can't open ISO image %s:\n%s") , path , strerror (errno) );
		response = ShowGtkMessageDialog (GTK_WINDOW (MainWindow) , GTK_DIALOG_MODAL,GTK_MESSAGE_ERROR,GTK_BUTTONS_OK, msg);
		g_free (msg);
        return;
    }
    if ( stat ( path , &my_stat ) < 0 )
    {
        response = ShowGtkMessageDialog (GTK_WINDOW (MainWindow) , GTK_DIALOG_MODAL,GTK_MESSAGE_ERROR,GTK_BUTTONS_OK, strerror (errno));
		return;
    }
     
    offset_image = DetectImage (iso_stream);
    if ( fseek (iso_stream , offset_image, SEEK_SET ) < 0)
    {
        response = ShowGtkMessageDialog (GTK_WINDOW (MainWindow) , GTK_DIALOG_MODAL,GTK_MESSAGE_ERROR,GTK_BUTTONS_OK, _("This ISO image is malformed !") );
		return;
    }
    fread ( &ipd, 1, sizeof(ipd), iso_stream );
    ipd.system_id[31] = '\000';
    ipd.volume_id[31] = '\000';
    ipd.volume_set_id[127] = '\000';
    ipd.publisher_id[127] = '\000';
    ipd.preparer_id[127] = '\000';
    ipd.application_id[127] = '\000';
    ipd.copyright_file_id[36] = '\000';
    ipd.abstract_file_id[36] = '\000';
    ipd.bibliographic_file_id[36] = '\000';

    gchar *iso_info = g_strdup_printf (
                _("\n\nFilename        : %s\n"
                "Size               : %lld bytes\n"
                "\nSystem ID      : %s\n"
                "Volume ID      : %s\n"
                "Application      : %s\n"
                "Publisher        : %s\n"
                "Preparer        : %s\n"
                "Volume Set ID   : %s\n"
                "Bibliographic ID: %s\n"
                "Copyright ID    : %s\n"
                "Abstract ID     : %s\n\n"
                "Creation Date   : %4.4s %2.2s %2.2s %2.2s:%2.2s:%2.2s.%2.2s\n"
                "Modified Date   : %4.4s %2.2s %2.2s %2.2s:%2.2s:%2.2s.%2.2s\n"
                "Expiration Date : %4.4s %2.2s %2.2s %2.2s:%2.2s:%2.2s.%2.2s\n"
                "Effective Date  : %4.4s %2.2s %2.2s %2.2s:%2.2s:%2.2s.%2.2s\n\n"
                "Volume Set Size    : %d\n"
                "Volume Sequence n. : %d\n"
                "Volume Space Size  : %d\n"
                "Logical Block Size : %d\n"
                "Path Table Size    : %d\n"
                "Root Dir. Record   : %d\n"),
                path,
                my_stat.st_size,
                ipd.system_id,
                ipd.volume_id,
                ipd.application_id,
                ipd.publisher_id,
                ipd.preparer_id,
                ipd.volume_set_id,
                ipd.bibliographic_file_id,
                ipd.copyright_file_id,
                ipd.abstract_file_id,
                &ipd.creation_date[0],
                &ipd.creation_date[4],
                &ipd.creation_date[6],
                &ipd.creation_date[8],
                &ipd.creation_date[10],
                &ipd.creation_date[12],
                &ipd.creation_date[14],
                &ipd.modification_date[0],
                &ipd.modification_date[4],
                &ipd.modification_date[6],
                &ipd.modification_date[8],
                &ipd.modification_date[10],
                &ipd.modification_date[12],
                &ipd.modification_date[14],
                &ipd.expiration_date[0],
                &ipd.expiration_date[4],
                &ipd.expiration_date[6],
                &ipd.expiration_date[8],
                &ipd.expiration_date[10],
                &ipd.expiration_date[12],
                &ipd.expiration_date[14],
                &ipd.effective_date[0],
                &ipd.effective_date[4],
                &ipd.effective_date[6],
                &ipd.effective_date[8],
                &ipd.effective_date[10],
                &ipd.effective_date[12],
                &ipd.effective_date[14],
                iso_723 (ipd.volume_set_size),
                iso_723 (ipd.volume_sequence_number),
                iso_731 (ipd.volume_space_size),
                iso_723 (ipd.logical_block_size),
                iso_723 (ipd.path_table_size) ,
                iso_731 (ipd.root_directory_record));
    gtk_text_buffer_insert (textbuf, &enditer, iso_info, strlen (iso_info));
    ShowShellOutput ( NULL , TRUE );
    g_free (iso_info);
	char *names[]= {(_("Filename")),(_("Permissions")),(_("Method")),(_("Compressed")),(_("Ratio")),(_("Date")),(_("Time")),(_("CRC-32"))};
	GType types[]= {G_TYPE_STRING,G_TYPE_UINT,G_TYPE_STRING,G_TYPE_UINT,G_TYPE_STRING,G_TYPE_STRING,G_TYPE_STRING,G_TYPE_STRING};
	CreateListStore ( 8, names , (GType *)types );

        lseek(fileno(iso_stream),DetectImage(iso_stream), 0);    
  	read(fileno(iso_stream), &ipd, sizeof(ipd));


  	idr = (struct iso_directory_record *) &ipd.root_directory_record;

	extent = iso_733((unsigned char *)idr->extent);
	
	lseek(fileno(iso_stream),((off_t)(extent - sector_offset)) <<11, SEEK_SET);
	
        read(fileno(iso_stream), buffer, sizeof (buffer));
        idr_rr = (struct iso_directory_record *) buffer;

	if ((c = dump_rr(idr_rr)) != 0) {
			
			if (c & 1024) {
				use_rock = 1;
			
				printf(
				 
				"Rock Ridge signatures version %d found\n",
				su_version);
				
				

			} else {
			
				printf(
				"Bad Rock Ridge signatures found (SU record missing)\n");
				
			}
			/*
			 * This is currently a no op!
			 * We need to check the first plain file instead of
			 * the '.' entry in the root directory.
			 */
			if (c & 2048) {
				printf("Apple signatures version %d found\n",
								aa_version);
			}
		} else {
			printf("NO Rock Ridge present\n");
			}

	lseek(fileno(iso_stream),DetectImage(iso_stream), 0);    
  	read(fileno(iso_stream), &ipd, sizeof(ipd));


  	
	parse_dir(iso_733(idr->extent), iso_733(idr->size));
       
	td = todo_idr;
	
		
	
  	
	while(td)
   	{
      	rootname = td->name;
	
      	parse_dir(td->extent, td->length);
	
      	td = td->next;
   	}
    fclose(iso_stream);
    gtk_tree_view_set_model (GTK_TREE_VIEW(treeview1), model);
    g_object_unref (model);
    OffTooltipPadlock();
    Update_StatusBar ( _("Operation successfully completed."));
}

//The following two functions are taken from isodump.c - By Eric Youngdale (1993)
int iso_723 ( unsigned char *p)
{
    return (( p[0] & 0xff) | ((p[1] & 0xff) << 8));
}

int iso_731 ( unsigned char *p)
{
    return ((p[0] & 0xff) | ((p[1] & 0xff) << 8) | ((p[2] & 0xff) << 16) | ((p[3] & 0xff) << 24));
}

int
iso_733 (unsigned char * p)
{
	return (iso_731 (p));
}


