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
// $Revision: 1.7 $
// $Date: 2004/08/18 03:32:30 $
//

#include "Channel.hpp"
#include "Main.hpp"
#include "Log.hpp"

Channel::Channel ()
{
	memset (topic, 0, CH_MAXTOPIC + 1);
}

Channel::~Channel ()
{
	
}

void Channel::AddOptions (void)
{
	default_topic = Config.Add (Conf::String, "Chat", "DefaultTopic",
				    "Welcome to SAGS Service");
}

void Channel::Start (void)
{
	Logs.Add (Log::Chat | Log::Info,
		  "Chat support for SAGS: 1 channel");

	strncpy (topic, default_topic->string, CH_MAXTOPIC);

	Logs.Add (Log::Chat | Log::Info,
		  "Default channel's topic is \"%s\"",
		  topic);
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

char *Channel::GenerateTopicMessage (const char *from)
{
	char default_msg[] = "%sContent-Type: text/plain; charset=UTF-8\n\n%s";
	char default_from[] = "From: %s\n";
	char newfrom[CL_MAXNAME + 8];
	char *msgtopic = NULL;
	int len;

	memset (newfrom, 0, CL_MAXNAME + 8);

	if (from != NULL)
		snprintf (newfrom, CL_MAXNAME + 8, default_from, from);

	len = strlen (newfrom) + strlen (default_msg) - 4 + strlen (topic);
	msgtopic = new char [len + 1];
	memset (msgtopic, 0, len + 1);

	snprintf (msgtopic, len + 1, default_msg, newfrom, topic);

	// no olvidar liberar
	return msgtopic;
}

char *Channel::GenerateChannelMessage (const char *from, const char *msg)
{
	char *hdr = NULL, *oldhdr = NULL, *body = NULL, *newmsg = NULL;
	int len;

	oldhdr = ExtractHeader (msg);
	body = ExtractBody (msg);

	if (body == NULL)
		return NULL;

	hdr = SetHeaderElement (oldhdr, "From", from);

	if (oldhdr != NULL)
		delete[] oldhdr;

	len = strlen (hdr) + 1 + strlen (body);
	newmsg = new char [len + 1];
	memset (newmsg, 0, len + 1);

	strncat (newmsg, hdr, strlen (hdr));
	strncat (newmsg, "\n", 1);
	strncat (newmsg, body, strlen (body));

	delete[] hdr;
	delete[] body;

	// no olvidar liberar
	return newmsg;
}

char *Channel::ExtractHeader (const char *msg)
{
	const char *p = msg;
	char *header = NULL;
	int n;

	if (msg != NULL)
	{
		for (n = 0; *p != '\0'; ++n, ++p)
			if (*p == '\n' && *(p + 1) == '\n')
				break;

		if (*p != '\0')
			++n;

		header = new char [n + 1];
		memset (header, 0, n + 1);
		strncpy (header, msg, n);

		// no olvidar liberar
		return header;
	}

	return NULL;
}

char *Channel::ExtractBody (const char *msg)
{
	const char *p = msg;
	char *body = NULL;
	int n, len;

	if (msg != NULL)
	{
		for (n = 0; *p != '\0'; ++n, ++p)
			if (*p == '\n' && *(p + 1) == '\n')
				break;

		if (strlen (p) > 2)
			p += 2;
		else
			return NULL;

		len = strlen (p);
		body = new char [len + 1];
		memset (body, 0, len + 1);
		strncpy (body, p, len);

		// no olvidar liberar
		return body;
	}

	return NULL;
}

char *Channel::SetHeaderElement (const char *hdr, const char *name, const char *value)
{
	int i, s, element_len, newhdr_len;
	char *element = NULL, *newheader = NULL;

	element_len = strlen (name) + 2 + strlen (value) + 1;
	element = new char [element_len + 1];
	memset (element, 0, element_len + 1);
	snprintf (element, element_len + 1, "%s: %s\n", name, value);

	if (hdr != NULL)
	{
		s = SearchHeaderElement (hdr, name);
		if (s != -1)
		{
			// s contiene la posición del elemento
			for (i = 0; hdr[s+i] != '\n' && hdr[s+i] != '\0'; ++i)
				;

			newhdr_len = s + element_len + strlen (hdr + s + i + 1);
			newheader = new char [newhdr_len + 1];
			memset (newheader, 0, newhdr_len + 1);
			strncpy (newheader, hdr, s);
			strncat (newheader, element, element_len);
			strncat (newheader, hdr + s + i + 1, strlen (hdr + s + i + 1));
		}
		else
		{
			// no lo encontramos, por lo que lo insertaremos
			// al comienzo de la cabecera
			newhdr_len = element_len + strlen (hdr);
			newheader = new char [newhdr_len + 1];
			memset (newheader, 0, newhdr_len + 1);
			strncpy (newheader, element, element_len);
			strncat (newheader, hdr, strlen (hdr));
		}
	}
	else
	{
		// no tiene cabecera el mensaje, creamos una
		newhdr_len = element_len;
		newheader = new char [newhdr_len + 1];
		memset (newheader, 0, newhdr_len + 1);
		strncpy (newheader, element, element_len);
		strncat (newheader, "\n", 1);
	}

	delete[] element;

	// no olvidar liberar
	return newheader;
}

int Channel::SearchHeaderElement (const char *hdr, const char *name)
{
	int i;
	const char *p;

	if (hdr == NULL)
		return -1;

	for (i = 0, p = hdr; *p != '\0'; ++i, ++p)
		if (!strncasecmp (p, name, strlen (name)))
			break;

	// si llegamos al final de la cabecera
	// es que no encontramos el elemento
	if (*p == '\0')
		return -1;

	return i;
}

void Channel::UserJoin (Client *Cl)
{
	struct channel_user *newuser;
	int i, max_list = Users.GetCount ();
	char *users_list, *msgtopic;

	newuser = new struct channel_user (Cl->GetUsername (), Cl);

	// buscamos un admin
	if (Cl->IsAuthorized (0))
		newuser->status |= STATUS_ADMIN;

	Logs.Add (Log::Chat | Log::Info,
		  "%s user \"%s\" has joined the general channel",
		  (newuser->status & STATUS_ADMIN) ? "Admin" : "Normal", newuser->name);

	// enviar aviso al resto de los usuarios conectados
	for (i = 0; i <= max_list - 1; ++i)
	{
		Users[i]->client->AddBuffer (Session::MainIndex, Session::ChatJoin,
					     newuser->name);
		Application.Add (Owner::Client | Owner::Send, Users[i]->client->ShowSocket ());
	}

	Users << newuser;

	// clientes que se conectan deben recibir la lista
	// de usuarios conectados y el topic
	msgtopic = GenerateTopicMessage ();
	newuser->client->AddBuffer (Session::MainIndex, Session::ChatTopic,
				    msgtopic);

	users_list = GetUserList ();
	newuser->client->AddBuffer (Session::MainIndex, Session::ChatUserList,
				    users_list);

	Application.Add (Owner::Client | Owner::Send, newuser->client->ShowSocket ());

	delete[] users_list;
	delete[] msgtopic;
}

void Channel::UserLeave (Client *Cl)
{
	int i, max_list;
	struct channel_user deluser (Cl->GetUsername ());

	if (Users.Remove (deluser) <= 0)
		return;

	Logs.Add (Log::Chat | Log::Info,
		  "User \"%s\" leaves the general channel",
		  Cl->GetUsername ());

	// enviar aviso al resto de los usuarios conectados
	max_list = Users.GetCount ();
	for (i = 0; i <= max_list - 1; ++i)
	{
		Users[i]->client->AddBuffer (Session::MainIndex, Session::ChatLeave,
					     deluser.name);
		Application.Add (Owner::Client | Owner::Send, Users[i]->client->ShowSocket ());
	}
}

void Channel::ChangeTopic (Client *Cl, Packet *Pkt)
{
	struct channel_user *topic_user, findusr (Cl->GetUsername ());
	char *msgtopic = NULL, *body;
	int i, max_list;

	topic_user = Users.Find (findusr);
	body = ExtractBody (Pkt->GetData ());

	if (topic_user->status & STATUS_ADMIN && body != NULL)
	{
		memset (topic, 0, CH_MAXTOPIC + 1);
		strncpy (topic, body, CH_MAXTOPIC);
		delete[] body;

		Logs.Add (Log::Chat | Log::Info,
			  "User \"%s\" changes the channel's topic",
			  Cl->GetUsername ());
		Logs.Add (Log::Chat | Log::Debug,
			  "New channel's topic is \"%s\"",
			  topic);

		// avisar del cambio a todos los usuarios
		max_list = Users.GetCount ();
		msgtopic = GenerateTopicMessage (Cl->GetUsername ());

		for (i = 0; i <= max_list - 1; ++i)
		{
			Users[i]->client->AddBuffer (Session::MainIndex, Session::ChatTopic,
						     msgtopic);
			Application.Add (Owner::Client | Owner::Send,
					 Users[i]->client->ShowSocket ());
		}

		delete[] msgtopic;
	}
}

int Channel::MessageChannel (Client *Cl, Packet *Pkt)
{
	int i, max_list = Users.GetCount ();
	char *newmsg = NULL;

	if (Pkt->GetIndex () != Session::MainIndex)
		return -1;

	newmsg = GenerateChannelMessage (Cl->GetUsername (), Pkt->GetData ());

	if (newmsg == NULL)
		return -1;

	// replicamos el mensaje a todos los usuarios
	// excepto al que lo envía
	for (i = 0; i <= max_list - 1; ++i)
	{
		if (!strncasecmp (Cl->GetUsername (), Users[i]->client->GetUsername (),
				  CL_MAXNAME))
			continue;
		Users[i]->client->AddBuffer (Pkt->GetIndex (), Pkt->GetCommand (), newmsg);
		Application.Add (Owner::Client | Owner::Send, Users[i]->client->ShowSocket ());
	}

	delete[] newmsg;

	return 0;
}

int Channel::MessagePrivate (Client *Cl, Packet *Pkt)
{
	return -1;
}

// definimos el objeto
Channel GeneralChannel;
