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
// $Source: /home/pablo/Desarrollo/sags-cvs/server/src/Main.cpp,v $
// $Revision: 1.2 $
// $Date: 2004/04/14 18:11:19 $
//

#include <iostream>
#include <cstdlib>

#include "Main.hpp"
#include "Process.hpp"
#include "Network.hpp"
#include "Log.hpp"
#include "Utils.hpp"

Main::Main () : SelectLoop ()
{
	debug = false;
	userslist = NULL;
	usersfile = NULL;
}

Main::~Main ()
{
	struct user *unlink;

	while (userslist)
	{
		unlink = userslist;
		userslist = unlink->next;
		delete unlink;
	}

	delete usersfile;
}

void Main::Init (bool debugmode)
{
	debug = debugmode;
	SelectLoop::Init ();
}

void Main::AddOptions (void)
{
	usersfile = Config.Add (Conf::String, "Main", "UsersFile", "sags.users");
}

void Main::SignalEvent (void)
{
	// Esta función se llama cuando hay eventos que
	// quieren que el programa termine. Se debe salir
	// ordenadamente.

	// detenemos el proceso hijo
	Child.Kill ();
	Child.WaitExit ();

	// desconectamos a los clientes
	Server.Shutdown ();

	// finalmente salimos
	Logs.Add (Log::Notice, "Exiting");

	// TODO: Esto debería ser reemplazado por un método Main::Exit ()
	// que debiera ser virtual en SelectLoop
	exit (0);
}

void Main::DataEvent (int owner, int fd)
{
	char buffer[PCKT_MAXDATA + 1];

	Logs.Add (Log::Debug, "DataEvent: owner=0x%02X fd=%d", owner, fd);

	switch (owner & Owner::Mask)
	{
		case Owner::Process:
			Logs.Add (Log::Debug, "Reading from child process");
			Child.Read (buffer, PCKT_MAXDATA);
			// enviar a los clientes lo leído del proceso
			if (buffer[0] != '\0')
				Server.SendToAllClients (Pckt::SessionConsoleOutput, buffer);
			break;

		case Owner::Network:
			Logs.Add (Log::Debug, "Accepting connection");
			Server.AcceptConnection (fd);
			break;

		case Owner::Client:
			if (owner & Owner::Send)
			{
				// socket listo para enviar
				Logs.Add (Log::Debug, "Sending data to client");
				Server.SendData (fd);
			}
			else
			{
				Logs.Add (Log::Debug, "Receiving data from client");
				Server.ReceiveData (fd);
			}
			break;

		default:
			Logs.Add (Log::Warning, "Unknown owner %04X", owner);
	}
}

int Main::GenerateResponse (Client *Cl, Packet *Pkt)
{
	Packet *Ans = NULL;
	struct user *usr = NULL;
	char *md5hash = NULL;

	if (Cl != NULL && Pkt != NULL)
	{
		switch (Pkt->GetType ())
		{
			case Pckt::SyncHello:
				Ans = new Packet (Pckt::SyncHello, 1, 9, "Hola Loco");
				break;

			case Pckt::SyncVersion:
				Ans = new Packet (Pckt::SyncVersion, 1, 1, "1");
				break;

			case Pckt::AuthUsername:
				if (Cl->GetStatus () == Usr::NeedUser)
				{
					Cl->SetUsername (Pkt->GetData ());
					Cl->SetStatus (Usr::NeedPass);
					Ans = new Packet (Pckt::AuthPassword, "User needs password");
				}
				break;

			case Pckt::AuthPassword:
				if (Cl->GetStatus () == Usr::NeedPass)
				{
					usr = FindUser (Cl->GetUsername ());
					if (usr != NULL)
					{
						md5hash = md5_password_hash (Pkt->GetData ());
						if (!strcmp (usr->hash, md5hash))
						{
							// usuario exitosamente autenticado
							Cl->SetStatus (Usr::Real);
							Ans = new Packet (Pckt::AuthSuccessful);
							Logs.Add (Log::Notice,
								  "User %s has logged in",
								  Cl->GetUsername ());
						}
						else
							Logs.Add (Log::Notice,
								  "User %s failed to get logged in",
								  Cl->GetUsername ());
					}
					else
						Logs.Add (Log::Notice,
							  "User %s don't exists",
							  Cl->GetUsername ());
				}
				break;

			case Pckt::SessionConsoleNeedLogs:
				if (Cl->IsValid ())
				{
					Server.SendProcessLogs (Cl);
					return 0;
				}
				return -1;

			case Pckt::SessionConsoleInput:
				if (Cl->IsValid ())
				{
					if (Child.Write (Pkt->GetData ()) > 0)
						Ans = new Packet (Pckt::SessionConsoleSuccess);
					else
						Ans = new Packet (Pckt::ErrorGeneric,
								  "Failed to write to process");
				}
				break;

			case Pckt::SessionDisconnect:
			case Pckt::SessionDrop:
				Ans = new Packet (Pckt::SessionDisconnect);
				break;

			default:
				return -1;
		}
	}
	else
		return -1;
	
	if (Ans != NULL)
	{
		Cl->Add (Ans);
		Add (Owner::Client | Owner::Send, Cl->ShowSocket ());
		return 0;
	}

	return -1;
}

bool Main::IsDebugging (void)
{
	return debug;
}

void Main::LoadUsers (void)
{
	ifstream file (usersfile->string);
	char line[CL_MAXNAME + HASHLEN + 1];
	char **ln;
	int i, j, two, nusers = 0;

	if (!file.is_open ())
	{
		Logs.Add (Log::Warning,
			  "Failed to open the users file %s",
			  usersfile->string);
		return;
	}

	for (i = 1; !file.eof(); ++i)
	{
		file.getline (line, CL_MAXNAME + HASHLEN);
		//Logs.Add (Log::Debug, "%2d %s", i, line);

		ln = strsplit (line, ':', -1, &two);
		if (two != 2)
		{
			//Logs.Add (Log::Debug, "Line %d skipped", i);
			continue;
		}

		Logs.Add (Log::Debug, "Loaded user \"%s\" with hash \"%s\"", ln[0], ln[1]);

		AddUser (ln[0], ln[1]);
		++nusers;

		// ln debe ser liberado
		if (two > 0)
		{
			for (j = two - 1; j >= 0; --j)
				delete[] ln[j];
		}
	}

	file.close ();

	Logs.Add (Log::Info,
		  "Loaded %d users from file %s",
		  nusers, usersfile->string);
}

void Main::AddUser (const char *name, const char *hash)
{
	struct user *newitem, *searched = userslist;

	newitem = new struct user;
	strncpy (newitem->name, name, CL_MAXNAME);
	strncpy (newitem->hash, hash, HASHLEN);
	newitem->next = NULL;

	if (searched)
	{
		// buscamos el último elemento
		while (searched->next)
			searched = searched->next;
		searched->next = newitem;
	}
	else
		userslist = newitem;  // lista estaba vacía
}

struct user *Main::FindUser (const char *name)
{
	struct user *searched = userslist;

	while (searched)
	{
		if (!strncmp (name, searched->name, CL_MAXNAME))
			return searched;

		searched = searched->next;
	}

	Logs.Add (Log::Warning, "User %s not found", name);

	return NULL;
}

void Main::PrintUsage (void)
{
	std::cerr << "Usage: sags [-D] <config-file>" << std::endl;
	std::cerr << "       -D: debug mode" << std::endl;
	exit (EXIT_FAILURE);
}

// definimos el objeto
Main Application;
