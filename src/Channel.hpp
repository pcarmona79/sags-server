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
// $Source: /home/pablo/Desarrollo/sags-cvs/server/src/Channel.hpp,v $
// $Revision: 1.3 $
// $Date: 2004/08/07 22:34:58 $
//

#ifndef __CHANNEL_HPP__
#define __CHANNEL_HPP__

#include <cstring>
#include "Client.hpp"
#include "List.hpp"
#include "Packet.hpp"

#define STATUS_NORMAL 0x000
#define STATUS_AWAY   0x001
#define STATUS_BUSY   0x002
#define STATUS_ADMIN  0x100

struct channel_user
{
	char name[CL_MAXNAME + 1];
	int status;
	Client *client;

	channel_user (const char *n = NULL, Client *c = NULL)
	{
		memset (name, 0, CL_MAXNAME + 1);
		if (n != NULL)
			strncpy (name, n, CL_MAXNAME);
		status = STATUS_NORMAL;
		client = c;
	}

	bool operator== (const struct channel_user &usr)
	{
		if (!strncmp (name, usr.name, CL_MAXNAME))
			return true;
		return false;
	}
};

class Channel
{
private:
	List<struct channel_user> Users;

public:
	Channel ();
	~Channel ();

	char *GetUserList (void);
	char *GenerateMessage (const char *from, const char *to, const char *msg);

	void UserJoin (Client *Cl);
	void UserLeave (Client *Cl);

	int MessageChannel (Client *Cl, Packet *Pkt);
	int MessagePrivate (Client *Cl, Packet *Pkt);
};

extern Channel GeneralChannel;

#endif // __CHANNEL_HPP__
