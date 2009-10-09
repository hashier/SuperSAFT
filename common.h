/*  common.h - common header
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
 *  $Id: common.h,v 1.7 2002/08/01 15:03:45 hashier Exp $
 *  $Log: common.h,v $
 *  Revision 1.7  2002/08/01 15:03:45  hashier
 *  bugfix
 *
 *  Revision 1.6  2002/07/13 17:44:53  hashier
 *  added cvs log message
 *
 *  Revision 1.5  2002/07/12 21:12:22  hashier
 *  TODO
 *
 *  Revision 1.4  2002/07/11 11:53:15  zeank
 *  added cvs log messages
 *
 */

#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>

#define MAXBUF 1024
#define MY_PORT 2546

#define DEBUG

int verbose;

/* ----------------------------------------
 * Just sending files 
 * ----------------------------------------
 */
int send_buf(int fd, char *str);

