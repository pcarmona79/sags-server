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
// $Source: /home/pablo/Desarrollo/sags-cvs/server/src/Main.hpp,v $
// $Revision: 1.5 $
// $Date: 2004/06/07 02:22:58 $
//

#ifndef __MAIN_HPP__
#define __MAIN_HPP__

#include "Loop.hpp"
#include "Packet.hpp"
#include "Client.hpp"
#include "Config.hpp"
#include "List.hpp"

#define HASHLEN 32

struct user
{
	char name[CL_MAXNAME + 1];
	char hash[HASHLEN + 1];
	
	user (const char *n = NULL, const char *h = NULL)
	{
		memset (name, 0, CL_MAXNAME);
		memset (hash, 0, HASHLEN);
		if (n != NULL)
			strncpy (name, n, CL_MAXNAME);
		if (h != NULL)
			strncpy (hash, h, HASHLEN);
	}

	bool operator== (const struct user &usr)
	{
		if (!strncmp (name, usr.name, CL_MAXNAME))
			return true;
		return false;
	}
};

class Main : public SelectLoop
{
private:
	bool debug;
	List<struct user> userslist;
	struct option *usersfile;

public:
	Main ();
	~Main ();

	void Init (bool debugmode = false);
	void AddOptions (void);
	int SignalEvent (int sig = 0);
	void DataEvent (int owner, int fd, bool writing);
	void TimeoutEvent (void);
	int GenerateResponse (Client *Cl, Packet *Pkt);
	bool IsDebugging (void);

	void LoadUsers (void);
	void AddUser (const char *name, const char *hash);
	struct user *FindUser (const char *name);

	void PrintUsage (void);
};

extern Main Application;

#endif // __MAIN_HPP__
