# Makefile for supersaft the bedder file transfer
# programm from 
# 
# Copyright (C) 2002 Christopher Loessl <cloessl@x-berg.de>
# Copyright (C) 2002 Stefan Strigler <steve@zeank.in-berlin.de>
#
# This programm is under the GPL 2
#
#  Created:				2002 07 08
#  Last updated:	2002 07 11
#
# $Id: Makefile,v 1.2 2002/07/11 11:53:15 zeank Exp $
# $Log: Makefile,v $
# Revision 1.2  2002/07/11 11:53:15  zeank
# added cvs log messages
#
#

CC = gcc
#CFLAGS = -ggdb -Wall -pedantic
CFLAGS = -ggdb -Wall

TARGETS = supersaftd supersaftc
DOBJS = supersaftd.o common.o
COBJS = supersaftc.o common.o

# how to make objects (we want debugging messages)
.c.o:
	$(CC) -c $(CFLAGS) -o $@ $<

all: $(TARGETS)

supersaftd: $(DOBJS)
	$(CC) $(CFLAGS) $(DOBJS) -o supersaftd

supersaftc: $(COBJS)
	$(CC) $(CFLAGS) $(COBJS) -o supersaftc


clean:
	@rm -f $(TARGETS) $(COBJS) $(DOBJS)

very-clean: clean
	@rm -f *~ *.bak *.tmp \#*

tar-ball:
	@tar cvfz supersaft.tgz *.h *.c Makefile README AUTHORS COPYING TODO ChangeLog

bz2-ball:
	@tar cvfj supersaft.tbz *.h *.c Makefile README AUTHORS COPYING TODO ChangeLog

.SUFFIXES: .c .o
.PHONY: clean
