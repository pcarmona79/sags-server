#
# SAGS - Secure Administrator of Game Servers
# Copyright (C) 2004 Pablo Carmona Amigo
# 
# This file is part of SAGS Server.
#
# SAGS Server is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# SAGS Server is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU Library General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
#
# $Source: /home/pablo/Desarrollo/sags-cvs/server/src/Attic/Makefile,v $
# $Revision: 1.1 $
# $Date: 2004/04/13 22:00:19 $
#

# Si se compila con g++ 2.95 se debe agregar -D_GNU_SOURCE a CFLAGS
#CC = g++-2.95 -Wall -g -O2
#CFLAGS += -D_GNU_SOURCE

CC = g++ -Wall -g -O2
#CFLAGS += 
LDFLAGS += -lutil -lssl

OBJS = \
	Client.o   \
	Config.o   \
	Log.o      \
	Loop.o     \
	Main.o     \
	Network.o  \
	Packet.o   \
	Process.o  \
	Program.o  \
	Protocol.o \
	Utils.o

TARGET = sags

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $^

.cpp.o:
	$(CC) $(CFLAGS) -o $@ -c $<

clean:
	-rm -f *~ core* $(OBJS) $(TARGET)
