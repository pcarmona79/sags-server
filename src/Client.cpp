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
// $Source: /home/pablo/Desarrollo/sags-cvs/server/src/Client.cpp,v $
// $Revision: 1.1 $
// $Date: 2004/04/13 22:00:20 $
//

#include <cstring>
#include <cerrno>

#include "Client.hpp"
#include "Log.hpp"

Client::Client (SSL_CTX *ctx, int sd, struct sockaddr_storage *ss, socklen_t sslen)
		: Protocol (ctx, sd, ss, sslen)
{
	// este constructor se invoca después
	// del constructor de Protocol, por lo
	// que podemos generar logs con la ip
	Logs.Add (Log::Client | Log::Info,
		  "New client connected from %s", ShowIP ());

	Outgoing = NULL;
	ClientStatus = Usr::NeedUser;
}

Client::~Client ()
{
	Packet *UnLink;

	// liberamos la lista de paquetes
	while (Outgoing)
	{
		UnLink = Outgoing;
		Outgoing = UnLink->Next;
		delete UnLink;
	}
}

int Client::Send (void)
{
	Packet *Sending = Outgoing;
	int bytes = 0, total = 0;

	while (Outgoing)
	{
		bytes = SendPacket (Sending);

		if (bytes < 0)
		{
			// paquete no se envió
			Logs.Add (Log::Client | Log::Warning,
				  "Failed to send packet: %s",
				  strerror (errno));

			// esto causará que el cliente sea removido
			return -1; 
		}

		// cliente desconectandose
		if (Sending->GetType () == Pckt::SessionDisconnect)
			return -2;

		// avanzamos la lista en un paquete
		// y borramos el paquete usado
		total += bytes;
		Outgoing = Outgoing->Next;
		delete Sending;
		Sending = Outgoing;
	}

	Logs.Add (Log::Client | Log::Debug,
		  "%d bytes sent to %s",
		  total, ShowIP ());

	return bytes;
}

Packet *Client::Receive (void)
{
	int bytes;
	Packet *Pkt = RecvPacket (&bytes);
		
	Logs.Add (Log::Client | Log::Debug,
		  "%d bytes received from %s",
		  bytes, ShowIP ());

	return Pkt;
}

void Client::Add (Packet *NewItem)
{
	Packet *Searched = Outgoing;

	if (Searched)
	{
		// buscamos el último elemento
		while (Searched->Next)
			Searched = Searched->Next;
		Searched->Next = NewItem;
	}
	else
		Outgoing = NewItem;  // lista estaba vacía
}

void Client::AddFirst (Packet *NewItem)
{
	NewItem->Next = Outgoing;
	Outgoing = NewItem;
}

int Client::Disconnect (void)
{
	Packet *Disc;

	// enviar un paquete de desconexión
	Disc = new Packet (Pckt::SessionDisconnect);

	if (SendPacket (Disc) < 0)
		return -1;

	// cerramos la conexión SSL
	Protocol::Disconnect ();

	return 0;
}

Usr::Status Client::GetStatus (void)
{
	return ClientStatus;
}

void Client::SetStatus (Usr::Status NewStatus)
{
	ClientStatus = NewStatus;
}

bool Client::IsValid (void)
{
	return !ClientStatus;
}

void Client::SetUsername (const char *name)
{
	strncpy (username, name, CL_MAXNAME);
}

char *Client::GetUsername (void)
{
	return username;
}
