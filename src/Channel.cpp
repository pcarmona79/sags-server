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
// $Source: /home/pablo/Desarrollo/sags-cvs/server/src/Channel.cpp,v $
// $Revision: 1.4 $
// $Date: 2004/08/07 22:34:58 $
//

#include "Channel.hpp"
#include "Main.hpp"

Channel::Channel ()
{
	
}

Channel::~Channel ()
{
	
}

char *Channel::GetUserList (void)
{
	int i, len = 0, max_list = Users.GetCount ();
	char *usrlst, current_name[CL_MAXNAME + 2];

	for (i = 0; i <= max_list - 1; ++i)
		len += strlen (Users[i]->name) + 1;

	usrlst = new char [len + 2];
	memset (usrlst, 0, len + 2);

	for (i = 0; i <= max_list - 1; ++i)
	{
		snprintf (current_name, CL_MAXNAME + 2, "%s\n", Users[i]->name);
		strncat (usrlst, current_name, strlen (current_name));
	}
	strncat (usrlst, "\n", 1);

	// no olvidar liberar
	return usrlst;
}

char *Channel::GenerateMessage (const char *from, const char *to, const char *msg)
{
	char from_header[CL_MAXNAME + 8], to_header[CL_MAXNAME + 6], *message;
	int len = 0;

	memset (from_header, 0, CL_MAXNAME + 8);
	memset (to_header, 0, CL_MAXNAME + 6);

	if (from != NULL)
	{
		snprintf (from_header, CL_MAXNAME + 8, "From: %s\n", from);
		len += strlen (from_header);
	}

	if (to != NULL)
	{
		snprintf (to_header, CL_MAXNAME + 6, "To: %s\n", to);
		len += strlen (to_header);
	}

	if (msg != NULL)
		len += strlen (msg) + 2;

	message = new char [len + 1];
	memset (message, 0, len + 1);

	if (from != NULL)
		strncat (message, from_header, strlen (from_header));
	if (to != NULL)
		strncat (message, to_header, strlen (to_header));
	if (msg != NULL)
	{
		strncat (message, "\n", 1);
		strncat (message, msg, strlen (msg));
		strncat (message, "\n", 1);
	}

	// no olvidar liberar
	return message;
}

void Channel::UserJoin (Client *Cl)
{
	struct channel_user *newuser;
	int i, max_list = Users.GetCount ();
	char *users_list;

	newuser = new struct channel_user (Cl->GetUsername (), Cl);

	// buscamos un admin
	if (Cl->IsAuthorized (0))
		newuser->status |= STATUS_ADMIN;

	// enviar aviso al resto de los usuarios conectados
	for (i = 0; i <= max_list - 1; ++i)
	{
		Users[i]->client->AddBuffer (Session::MainIndex, Session::ChatJoin,
					     newuser->name);
		Application.Add (Owner::Client | Owner::Send, Users[i]->client->ShowSocket ());
	}

	Users << newuser;

	// clientes que se conectan deben recibir la lista
	// de usuarios conectados
	users_list = GetUserList ();
	newuser->client->AddBuffer (Session::MainIndex, Session::ChatUserList,
				    users_list);
	Application.Add (Owner::Client | Owner::Send, newuser->client->ShowSocket ());

	delete[] users_list;
}

void Channel::UserLeave (Client *Cl)
{
	int i, max_list;
	struct channel_user deluser (Cl->GetUsername ());

	Users.Remove (deluser);

	// enviar aviso al resto de los usuarios conectados
	max_list = Users.GetCount ();
	for (i = 0; i <= max_list - 1; ++i)
	{
		Users[i]->client->AddBuffer (Session::MainIndex, Session::ChatLeave,
					     deluser.name);
		Application.Add (Owner::Client | Owner::Send, Users[i]->client->ShowSocket ());
	}
}

int Channel::MessageChannel (Client *Cl, Packet *Pkt)
{
	int i, max_list = Users.GetCount ();

	if (Pkt->GetIndex () != Session::MainIndex)
		return -1;

	// replicamos el mensaje a todos los usuarios
	for (i = 0; i <= max_list - 1; ++i)
	{
		Users[i]->client->Add (new Packet (*Pkt));
		Application.Add (Owner::Client | Owner::Send, Users[i]->client->ShowSocket ());
	}

	return 0;
}

int Channel::MessagePrivate (Client *Cl, Packet *Pkt)
{
	return -1;
}

// definimos el objeto
Channel GeneralChannel;
