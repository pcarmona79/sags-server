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
// $Source: /home/pablo/Desarrollo/sags-cvs/server/src/Network.cpp,v $
// $Revision: 1.6 $
// $Date: 2004/05/22 21:03:48 $
//

#include <iostream>
#include <cstdlib>
#include <cerrno>
#include <cstring>
#include <ctime>

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <unistd.h>

#include "Network.hpp"
#include "Log.hpp"
#include "Main.hpp"
#include "Process.hpp"

#define CHECK_TIMEOUT 15

Network::Network ()
{
	port = NULL;
	maxclients = NULL;
	certificate = NULL;
	connections = 0;
	ssl_method = NULL;
	ssl_context = NULL;
}

Network::~Network ()
{
	if (ssl_context != NULL)
		SSL_CTX_free (ssl_context);
}

Client *Network::AddClient (SSL_CTX *ctx, int sd, struct sockaddr_storage *address, socklen_t sslen)
{
	Client *NewClient;

	// crear un nuevo cliente con address
	// y agregarlo a la aplicación

	NewClient = new Client (ctx, sd, address, sslen);
	ClientList << NewClient;

	// el número de conexiones es igual al de
	// elementos en ClientList
	++connections;

	return NewClient;
}

void Network::RemoveClient (int sd)
{
	Client Searched (sd);
	ClientList.Remove (Searched);
}

Client *Network::FindClient (int sd)
{
	Client Searched (sd);
	return ClientList.Find (Searched);
}

void Network::Add (int sd)
{
	struct sditem *newitem = new struct sditem (sd);

	sdlist << newitem;
	Application.Add (Owner::Network, sd);
}

void Network::Remove (int sd)
{
	struct sditem searched (sd);
	sdlist.Remove (searched);
}

void Network::AddOptions (void)
{
	port = Config.Add (Conf::String, "Network", "Port", "47777");
	maxclients = Config.Add (Conf::Numeric, "Network", "MaxClients", 20);
	certificate = Config.Add (Conf::String, "Network", "CertificateFile", "sags.pem");
}

void Network::Start (void)
{
	int error, sd, success = 0;
	const int reuseaddr = 1;
	struct addrinfo hints, *res, *results;
	char address[INET6_ADDRSTRLEN + 1];

	Logs.Add (Log::Network | Log::Info,
		  "Connections limited to %d", maxclients->value);

	memset (&hints, 0, sizeof (hints));
	hints.ai_family = PF_UNSPEC;
	hints.ai_flags = AI_PASSIVE;
	hints.ai_socktype = SOCK_STREAM;

	error = getaddrinfo (NULL, port->string, &hints, &res);

	if (error)
		Logs.Add (Log::Network | Log::Error,
			  "getaddrinfo failed: %s",
			  gai_strerror (error));
	else
	{
		for (results = res; results; results = results->ai_next)
		{
			error = getnameinfo (results->ai_addr, results->ai_addrlen, address,
					     sizeof(address), NULL, 0, NI_NUMERICHOST);
			if (error)
			{
				Logs.Add (Log::Network | Log::Error,
					  "getnameinfo failed: %s",
					  gai_strerror (error));
				continue;
			}

			Logs.Add (Log::Network | Log::Debug,
				  "Trying to bind to %s", address);

			sd = socket (results->ai_family, results->ai_socktype, 0);
			if (sd < 0)
			{
				Logs.Add (Log::Network | Log::Warning,
					  "Failed to get socket: %s",
					  strerror (errno));
				continue;
			}

			if (setsockopt (sd, SOL_SOCKET, SO_REUSEADDR, &reuseaddr,
					sizeof (reuseaddr)) != 0)
			{
				Logs.Add (Log::Network | Log::Warning,
					  "Failed to reuse address: %s",
					  strerror (errno));
				continue;
			}

			if (bind (sd, results->ai_addr, results->ai_addrlen))
			{
				Logs.Add (Log::Network | Log::Warning,
					  "Failed to bind to %s: %s",
					  address, strerror (errno));
				continue;
			}
			
			if (listen (sd, 20))
			{
				Logs.Add (Log::Network | Log::Warning,
					  "Failed to listen on [%s]:%s: %s",
					  address, port->string, strerror (errno));
				continue;
			}

			// Informamos que tenemos un socket válido
			Logs.Add (Log::Network | Log::Info,
				  "Waiting connections on [%s]:%s",
				  address, port->string);

			// finalmente agregamos el socket válido
			success = 1;
			Add (sd);
		}
		freeaddrinfo (res);
	}

	if (!success)
		Logs.Add (Log::Network | Log::Critical,
			  "Failed to bind to port %s",
			  port->string);
	else
	{
		// iniciamos SSL/TLS
		OpenSSL_add_all_algorithms ();
		SSL_load_error_strings ();
		ssl_method = SSLv23_server_method ();
		ssl_context = SSL_CTX_new (ssl_method);
		SSL_CTX_set_options (ssl_context, SSL_OP_NO_SSLv2); // no aceptamos SSLv2

		// cargamos las claves
		SSL_CTX_use_certificate_file (ssl_context, certificate->string, SSL_FILETYPE_PEM);
		SSL_CTX_use_PrivateKey_file (ssl_context, certificate->string, SSL_FILETYPE_PEM);

		// verificamos las claves
		if (!SSL_CTX_check_private_key (ssl_context))
			Logs.Add (Log::Network | Log::Critical,
				  "Key and certificate don't match");
		else
			Logs.Add (Log::Network | Log::Notice,
				  "Using certificate file \"%s\"",
				  certificate->string);
	}
}

void Network::Shutdown (void)
{
	int i, maximus = ClientList.GetCount ();

	// cerrar todas las conexiones con los clientes
	Logs.Add (Log::Network | Log::Notice,
		  "Disconnecting all clients");

	for (i = 0; i <= maximus - 1; ++i)
		ClientList[i]->Disconnect (Pckt::ErrorServerQuit);

	// cerrar todos los sockets del servidor
	Logs.Add (Log::Network | Log::Notice,
		  "Closing all server sockets");

	maximus = sdlist.GetCount ();
	for (i = 0; i <= maximus - 1; ++i)
		DropConnection (sdlist[i]->sd);
}

int Network::AcceptConnection (int sd)
{
	int clsd;
	socklen_t sslen;
	struct sockaddr_storage address;
	Client *Cl;

	sslen = sizeof (address);
	memset (&address, 0, sslen);

	clsd = accept (sd, (struct sockaddr *)&address, &sslen);

	if (clsd < 0)
	{
		Logs.Add (Log::Network | Log::Error, "accept failed: %s",
			  gai_strerror (errno));
		return -1;
	}

	// agregamos el cliente
	Cl = AddClient (ssl_context, clsd, &address, sslen);

	// hay que cerrar la conexión si se alcanza el límite
	if (connections > maxclients->value)
	{
		Logs.Add (Log::Network | Log::Warning,
			  "Server is full. Closing connection");
		CloseConnection (clsd);
		return -1;
	}

	// comprobamos que el cliente agregado sea bueno
	if (Cl != NULL)
		if (!Cl->IsGood ())
		{
			Logs.Add (Log::Network | Log::Warning,
				  "Dropping bad client connected from %s",
				  Cl->ShowIP ());
			DropClient (Cl);
			return -1;
		}

	// finalmente le damos CHECK_TIMEOUT segundos para la autentificación
	AddWatch (Cl);

	return 0;
}

int Network::DropClient (Client *Cl)
{
	int sd = 0;
	char addr[INET6_ADDRSTRLEN + 9];

	if (Cl != NULL)
	{
		sd = Cl->ShowSocket ();
		strncpy (addr, Cl->ShowIP (), INET6_ADDRSTRLEN + 8);
	}
	else
		return -1;

	RemoveClient (sd);
	--connections;

	if (Cl != NULL)
		Logs.Add (Log::Network | Log::Notice,
			  "Dropped client connected from %s",
			  addr);

	return 0;
}

int Network::DropConnection (int sd)
{
	if (shutdown (sd, SHUT_RDWR))
		Logs.Add (Log::Network | Log::Warning,
			  "Failed to shutdown socket: %s",
			  strerror (errno));

	if (close (sd))
	{
		Logs.Add (Log::Network | Log::Warning,
			  "Failed to close socket: %s",
			  strerror (errno));
		return -1;
	}

	return 0;
}

void Network::CloseConnection (int sd, Pckt::Type pkt_type)
{
	Client *Cl;
	char addr[INET6_ADDRSTRLEN + 9];

	// encontrar el cliente y desconectarlo
	Cl = FindClient (sd);
	if (Cl != NULL)
	{
		strncpy (addr, Cl->ShowIP (), INET6_ADDRSTRLEN + 8);
		RemoveWatch (Cl);
		if (pkt_type != Pckt::Null)
			Cl->Disconnect (pkt_type); // no importa si falla
	}

	RemoveClient (sd);
	--connections;

	if (Cl != NULL)
		Logs.Add (Log::Network | Log::Notice,
			  "Removed client connected from %s",
			  addr);
}

int Network::ReceiveData (int sd)
{
	Client *Cl;
	Packet *Message;

	// recibir datos del cliente, procesarlos y generar respuesta

	Cl = FindClient (sd);
	if (Cl == NULL)
	{
		DropConnection (sd);
		return -1;
	}

	Message = Cl->Receive ();
	if (Message == NULL)
	{
		// cliente debe ser removido por fallar
		// la lectura
		Logs.Add (Log::Network | Log::Warning,
			  "Failed to read from client %s",
			  Cl->ShowIP ());
		// cliente fallido, cerrar la conexión TCP
		Cl->SetDrop (true);
		DropClient (Cl);
		return -1;
	}

	if (Application.GenerateResponse (Cl, Message))
	{
		// cliente debe ser removido si no hay
		// respuesta para él
		Logs.Add (Log::Network | Log::Warning,
			  "No response for client %s",
			  Cl->ShowIP ());
		CloseConnection (sd);
		return -1;
	}

	return 0;
}

int Network::SendData (int sd)
{
	Client *Cl;
	int bytes;

	Cl = FindClient (sd);
	if (Cl == NULL)
	{
		CloseConnection (sd);
		return -1;
	}

	bytes = Cl->Send ();

	if (bytes < 0)
	{
		Cl->SetDrop (true);
		DropClient (Cl);
	}

	return bytes;
}

void Network::SendToAllClients (Pckt::Type PktType, char *buf)
{
	Client *Cl = NULL;
	Packet *Output = NULL;
	int len = 0, i , maximus = ClientList.GetCount ();
	
	if (buf != NULL)
		len = strlen (buf);

	for (i = 0; i <= maximus - 1; ++i)
	{
		Cl = ClientList[i];
		if (Cl->IsValid ())
		{
			if (buf != NULL)
				Output = new Packet (PktType, 1, len, buf);
			else
				Output = new Packet (PktType);
			Cl->Add (Output);
			Application.Add (Owner::Client | Owner::Send, Cl->ShowSocket ());
		}
	}
}

void Network::SendProcessLogs (Client *Cl)
{
	int len;
	char *buf= NULL;

	buf = Child.GetProcessData (&len);

	if (Cl->IsValid ())
	{
		if (buf != NULL)
		{
			Cl->AddBuffer (Pckt::SessionConsoleLogs, buf);
			Application.Add (Owner::Client | Owner::Send, Cl->ShowSocket ());
		}
	}

	if (buf != NULL)
		delete[] buf;
}

void Network::AddWatch (Client *Cl)
{
	struct checkcl *newitem;

	newitem = new struct checkcl (Cl->GetTime (), Cl->ShowSocket ());
	checklist << newitem;

	// un timeout de CHECK_TIMEOUT/3 asegura que un cliente no autenticado
	// podrá estar a lo más 1.33*CHECK_TIMEOUT segundos conectado
	Application.AddTimeout ((int)(CHECK_TIMEOUT / 3.0));
}

void Network::RemoveWatch (Client *Cl)
{
	struct checkcl searched (0, Cl->ShowSocket ());

	checklist.Remove (searched);

	if (checklist.GetCount () == 0)
		Application.DeleteTimeout ();
}

void Network::DropNotValidClients (void)
{
	Client *Cl = NULL;
	time_t actualtime;
	int i = 0, maximus = ClientList.GetCount ();

	time (&actualtime);

	while (i <= maximus - 1)
	{
		Cl = ClientList[i];
		if (!Cl->IsValid ())
		{
			if (actualtime >= CHECK_TIMEOUT + Cl->GetTime ())
			{
				Logs.Add (Log::Network | Log::Warning,
					  "Dropping timeouted client connected from %s",
					  Cl->ShowIP ());
				CloseConnection (Cl->ShowSocket (), Pckt::ErrorAuthTimeout);
				
				i = -1;
				maximus = ClientList.GetCount ();
			}
		}
		++i;
	}
}

// definimos el objeto
Network Server;
