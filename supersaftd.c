/*  supersaftd.c - the SUPERSAFT server
 * 
 *  Copyright (C) 2002 Christopher Loessl <c.loessl@gmx.net>
 *  Copyright (C) 2002 Stefan Strigler <steve@zeank.in-berlin.de>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public Licensse as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.?
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
 *  Created: 			2002 07 07
 *  Last updated	2002 07 26
 *
 *  $Id: supersaftd.c,v 1.15 2002/07/27 10:11:32 hashier Exp $
 *  $Log: supersaftd.c,v $
 *  Revision 1.15  2002/07/27 10:11:32  hashier
 *  Some new features
 *
 *  Revision 1.14  2002/07/22 21:33:52  hashier
 *  use snprintf instead of sprintf
 *
 *  Revision 1.13  2002/07/14 11:54:19  hashier
 *  check if file exists
 *
 *  Revision 1.12  2002/07/14 10:32:25  zeank
 *  "TO <user>" was still buggy
 *
 *  Revision 1.11  2002/07/13 17:44:10  hashier
 *  Bugfixes
 *
 *  Revision 1.10  2002/07/13 11:35:52  zeank
 *  * nicer debugging messages
 *  * little bug-fixes
 *
 *  Revision 1.9  2002/07/13 11:03:50  zeank
 *  checked wrong directory and forgot to close it
 *
 *  Revision 1.8  2002/07/13 02:05:32  hashier
 *  Bugfix and updates
 *
 *  Revision 1.7  2002/07/12 21:12:22  hashier
 *  TODO
 *
 *  Revision 1.6  2002/07/12 09:07:10  hashier
 *  *** empty log message ***
 *
 *  Revision 1.5  2002/07/11 17:10:03  hashier
 *  Work now with less I/O
 *
 *  Revision 1.3  2002/07/11 13:16:03  zeank
 *  removed TODO
 *
 *  Revision 1.2  2002/07/11 11:53:15  zeank
 *  added cvs log messages
 *
 */

#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <resolv.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <dirent.h>
#include <pwd.h>
#include <time.h>
#include <sys/time.h>

#include "common.h"

#define FROM_SET	0x1	
#define TO_SET		0x2
#define	FILE_SET	0x4
#define	SIZE_SET	0x8
#define ALL_SET		0xf
#define DATA_SET	0x10

#define PERMS	0660

char *spooldir;

void serveSocket(int clientfd)
{
	int	nr;
	unsigned int sizeN; /*sizeC;*/         /*N = normal | C = compressed */
	unsigned char prot_status = 0; /* sets the proto status to '0' */
	char
		buffer[MAXBUF],
		hostname[255],
		*filename,
		*tmpfilename,
		*username,
		*from_str;

	/* send a nice welcome message */
	gethostname(hostname, 255);
	snprintf(buffer, sizeof(buffer), "220 %s SUPERSAFT server ready.", hostname); 
	send_buf(clientfd, buffer);

	while(1) {
		char	*cmd,
					*tail;

		if (prot_status & DATA_SET) { /* user is sending data now (hopefully :) */

			/* ========== GET FILE ========== */

			unsigned int rcvBytes = 0; /* bytes already recevied */
			int filefd;
			FILE *logfile;
			struct timeval now, then;
			DIR *tmpdir;

			/* create tmp dir */
			sprintf(buffer, "%s/%s/tmp", spooldir, username);

			if ((tmpdir = opendir(buffer)) == 0) {
				if ((mkdir(buffer, 0700))<0) {
					perror("mkdir");
					printf("ERROR: couldn't create tmp dir %s\n", buffer);
					break;
				}
			} else
				closedir(tmpdir);

			/* open file for writing */
			if ((filefd = creat(tmpfilename,PERMS)) < 0) { /* houston ... wir haben ein problem! */
				perror("creat");
				printf("ERROR: couldn't create file %s\n", tmpfilename);
				break; /* giving up - disconnect from client */
			}
#ifdef DEBUG
			printf("opened file, now waiting for data\n");
#endif

			bzero(&then, sizeof(then));
			gettimeofday(&then, NULL); /* get time before */
			
      while (rcvBytes < sizeN) {
				int tmpBytes;
				tmpBytes = recv(clientfd, buffer, MAXBUF, 0);
				/* 
					 #ifdef DEBUG
					 printf("received %d bytes\n", tmpBytes);
					 #endif
				*/
				if (rcvBytes+tmpBytes > sizeN) /* hu? received too much */
					tmpBytes = sizeN-rcvBytes;  /* cut it to expected size */
				write(filefd, buffer, tmpBytes);
				rcvBytes += tmpBytes;
				if (tmpBytes == 0) {
					sprintf(buffer, "599 Unknown error.");
					break;
				}
			}
			
			bzero(&now, sizeof(now));
			gettimeofday(&now, NULL);

			close(filefd);
			
			if (sizeN == rcvBytes) {
				/* copy file from to tmp dir to user's spooldir */
				if (rename(tmpfilename, filename) < 0) {
					perror("rename");
					sprintf(buffer, "599 Unknown error.");
				} else
					sprintf(buffer, "201 File has been correctly received.");
			}
			send_buf(clientfd, buffer);

			/* make a logfile entry */
			sprintf(buffer, "%s/supersaftd.log", spooldir);
			if ((logfile = fopen(buffer, "a+")) == NULL)
				perror("fopen");
			else {
				char *datestr;
				filename = strrchr(filename, '/');
				filename++;
				datestr = ctime((time_t *) &now.tv_sec);
				datestr[strlen(datestr)-1] = '\0';
				
				fprintf(logfile, "\"%s\",\"%s\",\"%s\",\"%s\",\"%d\",\"%f\"\n", datestr, from_str, username, filename, sizeN, (now.tv_sec - then.tv_sec)*1.0f + (now.tv_usec - then.tv_usec)/1000000.0f);
				
				fclose(logfile);
			}
			
			prot_status = FROM_SET; /* reset protocoll stack */
			continue;
		}	
	
		/* ========== PARSE COMMANDS ========== */
		
		/* receive a command */
		nr = recv(clientfd, buffer, MAXBUF, 0);

		/* remove trailing \r\n from buffer */
		buffer[nr-2] = '\0'; 
		
		if (verbose)
			printf("<<< %s\n",buffer);
		
		if (nr > 0) { /* recevied a command */

			/* prepare command */
			cmd = buffer;
			if ((tail = strchr(buffer, ' ')) != NULL) 
				/* split buffer on first blank */
				*tail++ = '\0'; 
			else
				tail = NULL;

			/* compare command - serve command */

			if (strcmp(cmd, "FILE") == 0) {

				/* ========== FILE ========== */

				if (tail == NULL) 
					sprintf(buffer, "505 Missing argument.");
				else {
					/* remember state */
					prot_status |= FILE_SET;
					if (strrchr(tail, '/') == NULL)
						filename = strdup(tail);
					else {
						filename = strrchr(strdup(tail), '/');
						filename++;
					}
					sprintf(buffer, "200 Command o.k.");
				}
				send_buf(clientfd, buffer);
			} else if (strcmp(cmd, "SIZE") == 0) {

				/* ========== SIZE ========== */

				if (tail == NULL)
					sprintf(buffer, "505 Missing argument.");
				else {
					/*remember state */
					prot_status |= SIZE_SET;
					sizeN = atoi(strdup(tail));

					sprintf(buffer, "200 Command o.k.");
				}
				send_buf(clientfd, buffer);

			} else if (strcmp(cmd, "FROM") == 0) {

				/* ========== FROM ========== */
				
				if (tail == NULL)
					sprintf(buffer, "505 Missing argument.");
				else {
					prot_status |= FROM_SET;
				  from_str = strdup(tail);
					
					sprintf(buffer, "200 Command o.k.");
				}
				send_buf(clientfd, buffer);

			} else if (strcmp(cmd, "TO") == 0) {

				/* ========== TO ========== */

					if (tail == NULL)
						sprintf(buffer, "505 Missing argument.");
					else {
						struct passwd *user; /* fucking structs */

						setpwent(); /* point to start of /etc/passwd */

						if (prot_status & TO_SET)
							prot_status ^= TO_SET; /* reset TO_SET */

						while ((user=getpwent()) != NULL) { /* for all users in /etc/passwd */
							if (strcmp(tail,user->pw_name) == 0) { /* user found */
								username = strdup(tail);
								prot_status |= TO_SET;
								sprintf(buffer, "200 Command o.k.");
								break;
							}
						}

						if (!(prot_status & TO_SET)) /* there was no user in /etc/passwd */
							sprintf(buffer, "520 User unknown.");
					}
					send_buf(clientfd, buffer);
					
			} else if (strcmp(cmd,"DATA") == 0) {

				/* ========== DATA ========== */

				if ((prot_status & ALL_SET) == ALL_SET) { /* user is now allowed to _send_ _files_ */

					DIR *userdir;
#ifdef DEBUG
					printf("filename: %s\n", filename);
#endif
					/* build the path where the file should be stored */
					snprintf(buffer, sizeof(buffer), "%s/%s/tmp/%s", spooldir, username, filename);
					tmpfilename = strdup(buffer);
					snprintf(buffer, sizeof(buffer), "%s/%s/%s", spooldir, username, filename);
					filename = strdup(buffer);

#ifdef DEBUG
					printf("Path set to %s\n", filename);
#endif

					snprintf(buffer, sizeof(buffer), "%s/%s", spooldir, username);

					if ((userdir=opendir(buffer)) == NULL) { /* we've got to create the directory first */

						if ((mkdir(buffer, 0700)) < 0) {
							sprintf(buffer, "411 Can`t create user spool directory.");
						} else {
							sprintf(buffer,"302 Header ok, send data.");
							prot_status |= DATA_SET; 
						}
					} else { /* directory already exists */
						int tmpfd;
						closedir(userdir); /* close dir */

						/* check if file already exists */
						if ((tmpfd = open(filename, O_RDONLY)) < 0) { /* no such file */
							/* remember state */
							prot_status |= DATA_SET; 
							sprintf(buffer,"302 Header ok, send data.");
						} else { /* file exists */
							sprintf(buffer,"531 This file has been already received.");
							close(tmpfd);
						}
					}
				} else
					sprintf(buffer,"503 Bad sequence of commands.");
				send_buf(clientfd, buffer);
			} else if (strcmp(cmd,"QUIT") == 0) {

				/*
				 *========== QUIT ==========
				 */
				sprintf(buffer, "221 Goodbye.");
				send_buf(clientfd, buffer);
				break;
				
			} else {
				sprintf(buffer,"502 Command not implemented.");
				send_buf(clientfd, buffer);
			}
		} else { /* connection killed by client */ /* no more data */
			break;
		}

#ifdef DEBUG
		printf("prot_status ===> %d\n\n", prot_status);
#endif
	}

	/* Detected closed connection */
	printf("#%d: Got a %d => close connection\n",clientfd,nr);
	close(clientfd);
}

int usage(char *progname) {
	printf("\nusage: %s [-d spooldir] [-p port]\n", progname);
	printf("options: -d spooldir\twhere to store incoming files (default: .)\n");
	printf("         -p port\tport we are listening on (default: %d)\n", MY_PORT);
	return (-1);
}

int main(int argc, char *argv[])
{	 
	int sockfd,
			opt,
			port;
	struct sockaddr_in self;

	/* vars for getopt */
	extern char *optarg;
	extern int optind, opterr, optopt;
							
	/* parse command line */
	/* options:
	 * -d spooldir	- optional (default .)
	 * -p port			- optional (default (see MY_PORT))
	 */

	port = MY_PORT;
	spooldir = strdup("./");

	while ((opt=getopt(argc,argv,"vd:p:h?")) > 0) {
		switch (opt) {
		case ':' :
		case 'h' :
		case '?' : exit(usage(argv[0])); break;
		case 'd' : spooldir=optarg; break;
		case 'p' : port=atoi(optarg); break;	
		case 'v' : verbose = 1; break;
		}
	}

	if (verbose)
		printf("Verbose: %d\nspooldir set to: %s, port set to: %d\n", verbose, spooldir, port);
	
	/* Create streaming socket */
	if ((sockfd = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
		perror("socket");
		exit(errno);
	}

	/* Initialize address/port structure */

	/* fill self with zeros */
	bzero(&self, sizeof(self));

	self.sin_family = PF_INET;

	/* htonX() converts machine byte order to network byte order */
	self.sin_port = htons(port);
	self.sin_addr.s_addr = INADDR_ANY;

	/* Assign a port number to the socket */
	if (bind(sockfd, (struct sockaddr*)&self, sizeof(self)) != 0) {
		perror("bind");
		exit(errno);
	}

	/* its a listening socket */
	if (listen(sockfd, 20) != 0) { /* max 20 pending conns */
		perror("listen");
		exit(errno);
	}

	printf("\nDaemon started successfully ...\n");

	/* Forever... */
	while (1) {
		int clientfd;
		int pid;

		struct sockaddr_in client_addr;
		int addrlen=sizeof(client_addr);

		/* accept a connection (creating a data pipe) */
		clientfd = accept(sockfd, (struct sockaddr*)&client_addr, &addrlen);

		printf("%s:%d connected\n", 
					 inet_ntoa(client_addr.sin_addr), 
					 ntohs(client_addr.sin_port));

		pid = fork();
		if (pid == 0) {
			/* Child will become server */
			serveSocket(clientfd);
			break;	/* kill the child :D */
		} else {
			/* look if we've got a dead child */
			waitpid(-1,NULL,WNOHANG);
		}
	}
	return 0;
}
