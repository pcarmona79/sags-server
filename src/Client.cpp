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
// $Revision: 1.10 $
// $Date: 2004/06/17 08:13:23 $
//

#include <cstring>
#include <cerrno>
#include <cmath>

#include "Client.hpp"
#include "Log.hpp"
#include "Main.hpp"
#include "ProcTree.hpp"

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

void Client::AddBuffer (unsigned int idx, unsigned int com, const char *data)
{
	const char *p = data;
	int s;

	// data NO DEBE ser nulo!!!

	// calculamos cuántos paquetes necesitaremos, lo
	// que corresponde a la parte entera más uno de
	// TamañoTotal / PCKT_MAXDATA
	s = (int) trunc (strlen (data) / PCKT_MAXDATA) + 1;

	while (strlen (p) >= PCKT_MAXDATA)
	{
		Outgoing << new Packet (idx, com, s--, strlen (p), p);
		p += PCKT_MAXDATA;
	}

	if (strlen (p) > 0 && strlen (p) < PCKT_MAXDATA)
		Outgoing << new Packet (idx, com, s--, strlen (p), p);
}

int Client::Disconnect (unsigned int idx, unsigned int com)
{
	if (!drop)
	{
		// enviar un paquete de desconexión o error
		Logs.Add (Log::Client | Log::Info,
			  "Desconnecting client %s", ShowIP ());

		SendPacket (new Packet (idx, com));
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

void Client::SetAuthorizedProcess (unsigned int idx)
{
	int i;

	AuthorizedProcess << idx;
	Add (new Packet (idx, Session::Authorized));

	if (idx == 0)
	{
		// es un administrador, por lo que hay que
		// enviar un paquete por cada servidor
		for (i = 1; ProcMaster.IsProcess (i); ++i)
			Add (new Packet (i, Session::Authorized));
	}
}

bool Client::IsAuthorized (unsigned int idx)
{
	if (IsValid ())
	{
		// buscamos primero a un administrador
		if (AuthorizedProcess.GetCount () == 1)
			if (AuthorizedProcess.Index (0) == 0)
				return true;

		// buscamos idx dentro de la lista
		// de procesos autorizados
		if (AuthorizedProcess.Find (idx) != NULL)
			return true;
	}
	return false;
}

bool Client::operator== (Client &Cl)
{
	return (this->socketd == Cl.socketd);
}
