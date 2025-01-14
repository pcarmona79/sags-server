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
// $Revision: 1.5 $
// $Date: 2004/08/18 03:32:30 $
//

#ifndef __CHANNEL_HPP__
#define __CHANNEL_HPP__

#include <cstring>
#include "Client.hpp"
#include "List.hpp"
#include "Packet.hpp"
#include "Config.hpp"

#define STATUS_NORMAL 0x000
#define STATUS_AWAY   0x001
#define STATUS_BUSY   0x002
#define STATUS_ADMIN  0x100

#define CH_MAXTOPIC 255

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
	char topic[CH_MAXTOPIC + 1];
	struct option *default_topic;

public:
	Channel ();
	~Channel ();

	void AddOptions (void);
	void Start (void);

	char *GetUserList (void);
	char *GenerateTopicMessage (const char *from = NULL);
	char *GenerateChannelMessage (const char *from, const char *msg);
	char *ExtractHeader (const char *msg);
	char *ExtractBody (const char *msg);
	char *SetHeaderElement (const char *hdr, const char *name, const char *value);
	int SearchHeaderElement (const char *hdr, const char *name);

	void UserJoin (Client *Cl);
	void UserLeave (Client *Cl);
	void ChangeTopic (Client *Cl, Packet *Pkt);
	int MessageChannel (Client *Cl, Packet *Pkt);
	int MessagePrivate (Client *Cl, Packet *Pkt);
};

extern Channel GeneralChannel;

#endif // __CHANNEL_HPP__
