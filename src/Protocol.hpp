//
// SAGS - Secure Administrator of Game Servers
// Copyright (C) 2004 Pablo Carmona Amigo
// 
// This file is part of SAGS Server.
//
// SAGS Server is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// SAGS Server is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Library General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
//
// $Source: /home/pablo/Desarrollo/sags-cvs/server/src/Protocol.hpp,v $
// $Revision: 1.1 $
// $Date: 2004/04/13 22:00:19 $
//

#ifndef __PROTOCOL_HPP__
#define __PROTOCOL_HPP__

#include <sys/socket.h>
#include <netinet/in.h>
#include <openssl/ssl.h>
#include <openssl/err.h>

#include "Packet.hpp"

class Protocol
{
protected:
	bool drop;
	SSL *ssl;
	int socketd;
	char address[INET6_ADDRSTRLEN];

public:
	Protocol (SSL_CTX *ctx, int sd, struct sockaddr_storage *ss, socklen_t sslen);
	~Protocol ();

	int ShowSocket (void);
	char *ShowIP (void);
	int SendPacket (Packet *Pkt);
	Packet *RecvPacket (int *len = NULL);
	int Disconnect (void);
	bool IsGood (void);
};

#endif // __PROTOCOL_HPP__
