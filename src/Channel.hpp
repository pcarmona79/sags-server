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
// $Revision: 1.1 $
// $Date: 2004/08/05 02:25:29 $
//

#ifndef __CHANNEL_HPP__
#define __CHANNEL_HPP__

#include <cstring>
#include "Client.hpp"
#include "List.hpp"
#include "Packet.hpp"

struct channel_user
{
	char name[CL_MAXNAME + 1];
	int status;

	channel_user (const char *n = NULL)
	{
		memset (name, 0, CL_MAXNAME + 1);
		status = 0;
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

	void UserJoin (Client *Cl);
	void UserLeave (Client *Cl);

	void MessageChannel (Client *Cl, Packet *Pkt);
	void MessagePrivate (Client *Cl, Packet *Pkt);
};

extern Channel GeneralChannel;

#endif // __CHANNEL_HPP__
