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
// $Source: /home/pablo/Desarrollo/sags-cvs/server/src/Protocol.cpp,v $
// $Revision: 1.4 $
// $Date: 2004/06/01 00:04:15 $
//

#include <cstring>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

#include "Protocol.hpp"
#include "Log.hpp"

Protocol::Protocol (SSL_CTX *ctx, int sd, struct sockaddr_storage *ss, socklen_t sslen)
{
	int error;
	char cl_addr[INET6_ADDRSTRLEN + 1], cl_port[6];

	connected = true;
	drop = false;
	socketd = sd;

	ssl = SSL_new (ctx);
	SSL_set_fd (ssl, socketd);

	error = SSL_accept (ssl);
	if (error < 1)
	{
		Logs.Add (Log::Client | Log::Warning,
			  "SSL failed: %s",
			  ERR_error_string (SSL_get_error (ssl, error), NULL));

		// este cliente debe ser desconectado!
		drop = true;
	}

	error = getnameinfo ((struct sockaddr *)ss, sslen, cl_addr, sizeof (cl_addr),
			     cl_port, sizeof (cl_port), NI_NUMERICHOST);
	if (error)
		memset (address, 0, sizeof (address));
	else
		snprintf (address, INET6_ADDRSTRLEN + 9, "[%s]:%s", cl_addr, cl_port);
}

Protocol::Protocol (int sd)
{
	connected = false;
	drop = true;
	socketd = sd;
	ssl = NULL;
	memset (address, 0, sizeof (address));
}

Protocol::~Protocol ()
{
	if (connected)
		Disconnect ();
	if (ssl != NULL)
		SSL_free (ssl);
}

int Protocol::ShowSocket (void)
{
	return socketd;
}

char *Protocol::ShowIP (void)
{
	return address;
}

int Protocol::SendPacket (Packet *Pkt)
{
	struct pkt sent;
	int bytes;

	//Logs.Add (Log::Client | Log::Debug,
	//	  "Packet to send: TYPE: %04X SEQ: %d LEN: %d DATA: \"%s\"",
	//	  Pkt->GetType (), Pkt->GetSequence (), Pkt->GetLength (), Pkt->GetData ());

	sent.pkt_header = Pkt->GetHeader ();
	strncpy (sent.pkt_data, Pkt->GetData (), Pkt->GetLength ());

	bytes = SSL_write (ssl, &sent, sizeof (unsigned int) + Pkt->GetLength ());
	if (bytes < 0)
		return -1;

	return bytes;
}

Packet *Protocol::RecvPacket (int *len)
{
	int bytes, total = 0;
	struct pkt header;
	Packet *Pkt = NULL;

	memset (&header, 0, sizeof (header));

	bytes = SSL_read (ssl, &header, sizeof (unsigned int));
	if (bytes <= 0)
		return NULL;
	total += bytes;

	if (((header.pkt_header & Mask::Seq) >> 10) > 0)
	{
		bytes = SSL_read (ssl, header.pkt_data, (header.pkt_header & Mask::Len) + 1);
		if (bytes <= 0)
			return NULL;
		total += bytes;
	}

	Pkt = new Packet (header);

	//Logs.Add (Log::Client | Log::Debug,
	//	  "Packet received: TYPE: %04X SEQ: %d LEN: %d DATA: \"%s\"",
	//	  Pkt->GetType (), Pkt->GetSequence (), Pkt->GetLength (), Pkt->GetData ());

	if (len != NULL)
		*len = total;

	return Pkt;
}

int Protocol::Disconnect (void)
{
	int retval = SSL_shutdown (ssl);

	if (shutdown (socketd, SHUT_RDWR))
		Logs.Add (Log::Client | Log::Warning,
			  "Failed to shutdown socket: %s",
			  strerror (errno));

	if (close (socketd))
		Logs.Add (Log::Client | Log::Warning,
			  "Failed to close socket: %s",
			  strerror (errno));

	connected = false;

	return retval;
}

bool Protocol::IsGood (void)
{
	return !drop;
}

void Protocol::SetDrop (bool val)
{
	drop = val;
}
