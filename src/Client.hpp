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
// $Source: /home/pablo/Desarrollo/sags-cvs/server/src/Client.hpp,v $
// $Revision: 1.3 $
// $Date: 2004/05/19 02:53:43 $
//

#ifndef __CLIENT_HPP__
#define __CLIENT_HPP__

#include <ctime>

#include "Protocol.hpp"
#include "Packet.hpp"
#include "List.hpp"

#define CL_MAXNAME 80

namespace Usr
{
	typedef enum
	{
		Real = 0,
		Bad,
		NeedPass,
		NeedUser
	} Status;
}

class Client : public Protocol
{
private:
	List<Packet> Outgoing;
	Usr::Status ClientStatus;
	char username[CL_MAXNAME + 1];
	time_t cltime;

public:
	Client (SSL_CTX *ctx, int sd, struct sockaddr_storage *ss, socklen_t sslen);
	Client (int sd);
	~Client ();

	int Send (void);
	Packet *Receive (void);
	void Add (Packet *NewItem);
	void AddFirst (Packet *NewItem);
	void AddBuffer (unsigned int type, const char *data);
	int Disconnect (Pckt::Type pkt_type = Pckt::SessionDisconnect);

	Usr::Status GetStatus (void);
	void SetStatus (Usr::Status NewStatus);
	bool IsValid (void);
	void SetUsername (const char *name);
	char *GetUsername (void);
	time_t GetTime (void);
	void UpdateTime (void);

	bool operator== (Client &Cl);
};

#endif // __CLIENT_HPP__
