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
// $Revision: 1.9 $
// $Date: 2004/06/18 00:53:23 $
//

#ifndef __MAIN_HPP__
#define __MAIN_HPP__

#include "Loop.hpp"
#include "Packet.hpp"
#include "Client.hpp"
#include "Config.hpp"
#include "List.hpp"
#include "Utils.hpp"

#define PASSLEN  80
#define PROCSLEN 80
#define HASHLEN  32

struct user
{
	char name[CL_MAXNAME + 1];
	char pass[PASSLEN + 1];
	char procs[PROCSLEN + 1];
	char rndstr[HASHLEN + 1];
	char hash[HASHLEN + 1];
	
	user (const char *n = NULL, const char *p = NULL, const char *pa = NULL)
	{
		unsigned int i;

		memset (name, 0, CL_MAXNAME + 1);
		memset (pass, 0, PASSLEN + 1);
		memset (procs, 0, PROCSLEN + 1);
		memset (rndstr, 0, HASHLEN + 1);
		memset (hash, 0, HASHLEN + 1);

		if (n != NULL)
			strncpy (name, n, CL_MAXNAME);
		if (p != NULL)
		{
			for (i = 0; (i <= PASSLEN - 1) && (i <= strlen (p) - 1); ++i)
				pass[i] = p[i] ^ 0xAA;
			if (i <= PASSLEN - 1)
				pass[i] = '\0';
		}
		if (pa != NULL)
			strncpy (procs, pa, PROCSLEN);

		// la cadena aleatoria es calculada antes de enviarla
		// al cliente, por lo que los hash se calcularán en
		// update
	}

	void update (void)
	{
		char *md5hash = NULL, str[HASHLEN + PASSLEN + 1];
		
		random_string (rndstr, HASHLEN);
		snprintf (str, HASHLEN + PASSLEN + 1, "%s%s", rndstr, pass);
		md5hash = md5_password_hash (str);
		strncpy (hash, md5hash, HASHLEN);

		delete[] md5hash;
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
	int ProtoSync (Client *Cl, Packet *Pkt);
	int ProtoAuth (Client *Cl, Packet *Pkt);
	int ProtoSession (Client *Cl, Packet *Pkt);

	bool IsDebugging (void);
	void LoadUsers (void);
	void AddUser (const char *name, const char *pass, const char *procs);
	struct user *FindUser (const char *name);
	void AddAuthorizedProcesses (Client *Cl, struct user *usr);

	void PrintUsage (void);

};

extern Main Application;

#endif // __MAIN_HPP__
