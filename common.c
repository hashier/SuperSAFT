/*  common.c - common shared routines
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
 *  Created:			2002 07 10
 *  Last updated:	2002 07 13
 *
 *  $Id: common.c,v 1.4 2002/07/13 17:46:43 hashier Exp $
 *  $Log: common.c,v $
 *  Revision 1.4  2002/07/13 17:46:43  hashier
 *  new
 *
 *  Revision 1.3  2002/07/13 11:35:52  zeank
 *  * nicer debugging messages
 *  * little bug-fixes
 *
 *  Revision 1.2  2002/07/11 11:53:15  zeank
 *  added cvs log messages
 *
 */

#include "common.h"
#include <string.h>


int send_buf(int fd, char *str) {
	int len = strlen(str);
	char buf[len+2];

	sprintf(buf, "%s\r\n", str);
	if (send(fd, buf, len+2, 0) < 0) {
		perror("send");
		return -1;
	}

#ifdef DEBUG
	printf(">>> %s.\n", str);
#endif
	return 0;
}
