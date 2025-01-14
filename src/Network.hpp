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
// $Source: /home/pablo/Desarrollo/sags-cvs/server/src/Network.hpp,v $
// $Revision: 1.8 $
// $Date: 2005/02/10 21:55:05 $
//

#ifndef __NETWORK_HPP__
#define __NETWORK_HPP__

#include <ctime>
#include <openssl/ssl.h>
#include <openssl/err.h>

#include "Client.hpp"
#include "Config.hpp"
#include "Packet.hpp"
#include "List.hpp"

struct sditem
{
	int sd;

	sditem (int s = 0) : sd (s)
	{ }

	bool operator== (struct sditem &s)
	{
		return (sd == s.sd);
	}
};

struct checkcl
{
	time_t timeout;
	int sd;

	checkcl (time_t t = 0, int s = 0) : timeout (t), sd (s)
	{ }

	bool operator== (struct checkcl &chk)
	{
		if (sd == chk.sd)
			return true;
		return false;
	}
};

class Network
{
private:
	struct option *port;
	struct option *maxclients;
	struct option *certificate;
	List<Client> ClientList;
	List<struct sditem> sdlist;
	List<struct checkcl> checklist;
	const SSL_METHOD *ssl_method;
	SSL_CTX *ssl_context;
	int current_maxclients;

	Client *AddClient (SSL_CTX *ctx, int sd, struct sockaddr_storage *address,
			   socklen_t sslen);
	void RemoveClient (int sd);
	Client *FindClient (int sd);
	void Add (int sd);
	void Remove (int sd);

public:
	Network ();
	~Network ();

	void AddOptions (void);
	void CheckOptions (void);
	void Start (void);
	void Shutdown (void);

	int AcceptConnection (int sd);
	int DropClient (Client *Cl);
	int DropConnection (int sd);
	void CloseConnection (int sd, unsigned int idx = Session::MainIndex,
			      unsigned int com = Session::Disconnect);

	int ReceiveData (int sd);
	int SendData (int sd);

	void SendToAllClients (unsigned int idx, unsigned int com, char *buf = NULL);
	void SendProcessLogs (Client *Cl, unsigned int idx);

	void AddWatch (Client *Cl);
	void RemoveWatch (Client *Cl);
	void DropNotValidClients (void);
	void DropDuplicatedClients (Client *Search);
	void DropExtraClients (void);
};

extern Network Server;

#endif // __NETWORK_HPP__
