/*  supersaftc - the SUPERSAFT client
 * 
 *  Copyright (C) 2002 Christopher Loessl <c.loessl@gmx.net>
 *  Copyright (C) 2002 Stefan Strigler <steve@zeank.in-berlin.de>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public Licensse as published by
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
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 *  Created:			2002 07 09
 *  Last updated:	2002 07 13
 *
 *  $Id: supersaftc.c,v 1.12 2002/07/22 21:33:33 hashier Exp $
 *  $Log: supersaftc.c,v $
 *  Revision 1.12  2002/07/22 21:33:33  hashier
 *  use snprintf
 *  use of getopt
 *
 *  Revision 1.11  2002/07/14 10:34:32  zeank
 *  enhanced progress bar
 *
 *  Revision 1.10  2002/07/14 00:04:29  hashier
 *  Setvbuf to unbuffer stdout
 *  kb/s
 *
 *  Revision 1.9  2002/07/13 17:44:17  hashier
 *  Bugfixes
 *
 *  Revision 1.8  2002/07/13 11:35:52  zeank
 *  * nicer debugging messages
 *  * little bug-fixes
 *
 *  Revision 1.7  2002/07/13 02:05:24  hashier
 *  Bugfix and updates
 *
 *  Revision 1.6  2002/07/12 21:12:22  hashier
 *  TODO
 *
 *  Revision 1.5  2002/07/12 09:07:10  hashier
 *  *** empty log message ***
 *
 *  Revision 1.4  2002/07/11 15:35:22  hashier
 *  removed again. doing time stop instead
 *
 *  Revision 1.3  2002/07/11 14:45:59  hashier
 *  added status messages
 *
 *  Revision 1.2  2002/07/11 11:51:25  zeank
 *  added cvs log messages
 *
 */

#include <stdio.h>
#include <unistd.h>
#include <resolv.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <netdb.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <errno.h>

#include "common.h"

void usage(char *programname) {
	printf("Usage: %s [-v] <file> <user>@<host>\n", programname);
	exit(0);
}

/* --------------------------------------------------------------------------
 * get_answer - gets answer from fd
 *
 * returns an int value representing an answer according to the SAFT 
 * protocol
 * if you want to get the corresponding text answer you have to supply 
 * retval which is set to this text.
 *
 * e.g.: server answers "200 Command ok."
 * 200 gets returned and retval ist set to "Command ok."
 * --------------------------------------------------------------------------
 */

int get_answer(int fd, char *retval) {
	int rcvBytes; /* bytes received */
	static char buffer[MAXBUF];

	rcvBytes = recv(fd, buffer, MAXBUF, 0);
	buffer[rcvBytes] = '\0'; /* terminate string */
#ifdef DEBUG
	printf("<<< %s\n", buffer);
#endif

	/* split buffer on first blank */
	retval = strchr(buffer, ' '); 
	*retval++ = '\0'; /* let retval point to tail wich pointes to the text */

	return(atoi(buffer)); /* return proto code only */
}


int main(int argc, char *argv[]) {

	int transfd; /*this FD is connected to the server */
	int filefd; /* the FD which is uses to work with the file */
	int sentBytes = 0; /* number of sen_T_ bytes */
	float time;
	int answer;
	char buffer[MAXBUF];
	char 	*filename,
				*dest, /* user@host */
				*user, 
				*host;
	int verbose = 0;
	struct stat fbuf;
	struct timeval now, then;
	struct hostent *he; /* used by gethostbyname */
	struct sockaddr_in self; /* where do we connect to */

	/* stuff for getopt */
	int opt;
	extern char *optarg;
	extern int optind, opterr, optopt;
	
	while ((opt=getopt(argc,argv,"vh?")) > 0) {
		switch (opt) {
			case ':' :
			case 'h' :
			case '?' : usage(argv[0]); break;
			case 'v' : verbose=1; break;
		}
	}

	if (argc < optind+2)
		usage(argv[0]);

	filename = argv[optind++];
	dest = argv[optind++];
	
	/* open the file read-only */
	if ((filefd = open(filename, O_RDONLY)) < 0) {
			perror("open");
			return -1;
	}

	if ((host=strchr(dest, '@'))) { /* look for an @ */
		*host++ = '\0'; /* split */
		user = dest;
	} else 
		usage(argv[0]);

	/* Nowwe are trying to establish a connection with the server */
	he = gethostbyname(host); /* get the needed infos for the connect */

	if (!he) {
		fprintf (stderr, "%s: host not found\n", host);
		return -1;
	}

	bzero(&self, sizeof(self));
	self.sin_family = PF_INET;  
	self.sin_port = htons(MY_PORT);  /* only converting for intel and mac */
	self.sin_addr = *((struct in_addr*)he->h_addr);

	if ((transfd=socket(PF_INET, SOCK_STREAM, 0)) < 0) { /* hu? */
		perror("socket");
		return -1;
	}

	if (connect(transfd, (struct sockaddr *)&self, sizeof(self)) < 0) {
			perror("connect");
			return -1;
	}

	/* set stdout to unbuffered mode */
	setvbuf(stdout, (char *)NULL, _IONBF, 0);

#ifdef DEBUG
	printf("\nGetting servers welcome msg...\n");
#endif
	get_answer(transfd, NULL);

	/* ------- TO ------- */
	snprintf(buffer, sizeof(buffer), "TO %s", user);
	send_buf(transfd, buffer);

	/* gettin' answer from the fucking server */
	answer = get_answer(transfd, buffer);
	/* cheking if the user exist */
	if(answer != 200) {
		printf("Error occurred: User doen't exist on that system\n");
		sprintf(buffer, "QUIT");
		send_buf(transfd, buffer);
		close(transfd);
		return -1;
	}
	
	/* ------- FILENAME ------- */
	snprintf(buffer, sizeof(buffer), "FILE %s", filename);
	send_buf(transfd, buffer);

	/* get answer */ 
	get_answer(transfd, NULL);

	/* ------- SIZE ------- */
	bzero(&fbuf,sizeof(fbuf));
	stat(filename, &fbuf);
	snprintf(buffer, sizeof(buffer), "SIZE %d", (int)fbuf.st_size);
	send_buf(transfd, buffer);

	/* answer from SIZE sending */
	get_answer(transfd, NULL);
	
	/* ------- DATA ------- */
	send_buf(transfd, "DATA");

	/* the answer from DATA */
	answer = get_answer(transfd, NULL);
	if (answer != 302) {
		printf("Error occurred: unable to send\n");
		sprintf(buffer, "QUIT");
		send_buf(transfd, buffer);
		return -1;
	}


	/* ========== NOW SEND THE FILE ======= */

#ifdef DEBUG
	printf("ok - sending *fucking* bits to the *fucking* server....\n");
#endif

	bzero(&then,sizeof(then));
	gettimeofday(&then,NULL); /* get time before */

	printf("\n%5uK ", sentBytes);
	while (sentBytes < fbuf.st_size) {
		read(filefd, buffer, 1);
		write(transfd, buffer, 1);
		sentBytes++;
		if (sentBytes % (1024) == 0)
			putchar('.');
		if (sentBytes % (1024*10) == 0)
			putchar(' ');
		if (sentBytes % (1024*50) == 0) {
			bzero(&now,sizeof(now));
			gettimeofday(&now,NULL);
			/* sec + nano sec/1.000.000 */
			time = ((now.tv_sec - then.tv_sec)*1.0f + (now.tv_usec - then.tv_usec)/1000000.0f);

			printf("%3d%% @ %.2f KB/s\n%5uK ", (int)((float)sentBytes/(float)fbuf.st_size*100), sentBytes/(1024*time), sentBytes/1024);
		}
	}

	bzero(&now,sizeof(now));
	gettimeofday(&now,NULL); /* get time after sending file */
	/* sec + nano sec/1.000.000 */
	time = ((now.tv_sec - then.tv_sec)*1.0f + (now.tv_usec - then.tv_usec)/1000000.0f);
	printf("\n\n\t%d bytes sent in %.2f sec [%.2f KB/s].\n", sentBytes, time, sentBytes/(1024*time));
		
	/* now closing the file */
	close(filefd);

	/* antwort */
	get_answer(transfd, NULL);

	/* dann QUIT senden */
	sprintf(buffer, "QUIT");
	send_buf(transfd, buffer);

	/* und schaun, was der server davon hält */
	get_answer(transfd, NULL);

	/* und zum schluss sollte man die verbindung natürlich wieder trennen */
	close(transfd);
	return 0;
}
