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
// $Revision: 1.7 $
// $Date: 2004/05/31 01:39:45 $
//

#include <cstring>
#include <cerrno>
#include <cmath>

#include "Client.hpp"
#include "Log.hpp"
#include "Main.hpp"

Client::Client (SSL_CTX *ctx, int sd, struct sockaddr_storage *ss, socklen_t sslen)
		: Protocol (ctx, sd, ss, sslen)
{
	// este constructor se invoca después
	// del constructor de Protocol, por lo
	// que podemos generar logs con la ip

	ClientStatus = Usr::NeedUser;
	time (&cltime);

	Application.Add (Owner::Client, socketd);

	Logs.Add (Log::Client | Log::Info,
		  "New client connected from %s", ShowIP ());
}

Client::Client (int sd)
	: Protocol (sd)
{
	// un constructor simple para utilizarlo
	// en búsqueda de otros elementos
	ClientStatus = Usr::NeedUser;
	time (&cltime);
}

Client::~Client ()
{
	if (connected)
		Disconnect ();
}

int Client::Send (void)
{
	Packet *Sending = Outgoing.ExtractFirst ();
	int bytes = 0;

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

	// borramos el paquete ya usado
	delete Sending;

	Logs.Add (Log::Client | Log::Debug,
		  "%d bytes sent to %s",
		  bytes, ShowIP ());

	if (Outgoing.GetCount () == 0)
		Application.Remove (Owner::Client | Owner::Send, ShowSocket ());

	return bytes;
}

Packet *Client::Receive (void)
{
	int bytes = 0;
	Packet *Pkt = RecvPacket (&bytes);
		
	Logs.Add (Log::Client | Log::Debug,
		  "%d bytes received from %s",
		  bytes, ShowIP ());

	return Pkt;
}

void Client::Add (Packet *NewItem)
{
	Outgoing << NewItem;
}

void Client::AddFirst (Packet *NewItem)
{
	Outgoing.Add (NewItem, true);
}

void Client::AddBuffer (unsigned int type, const char *data)
{
	const char *p = data;
	int s;

	// data NO DEBE ser nulo!!!

	// calculamos cuantos paquetes necesitaremos
	// que corresponde a la parte entera más uno de
	// TamañoTotal / 1024
	s = (int) trunc (strlen (data) / 1024) + 1;

	while (strlen (p) >= 1024)
	{
		Outgoing << new Packet (type, s--, strlen (p), p); // asigna hasta 1024 bytes
		p += 1024;
	}

	if (strlen (p) > 0 && strlen (p) < 1024)
		Outgoing << new Packet (type, s--, strlen (p), p);
}

int Client::Disconnect (Pckt::Type pkt_type)
{
	if (!drop)
	{
		// enviar un paquete de desconexión o error
		Logs.Add (Log::Client | Log::Info,
			  "Desconnecting client %s", ShowIP ());

		SendPacket (new Packet (pkt_type));
	}

	// cerramos la conexión SSL
	Protocol::Disconnect ();

	Application.Remove (Owner::Client | Owner::Send, socketd);
	Application.Remove (Owner::Client, socketd);

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

time_t Client::GetTime (void)
{
	return cltime;
}

void Client::UpdateTime (void)
{
	time (&cltime);
}

bool Client::operator== (Client &Cl)
{
	return (this->socketd == Cl.socketd);
}
