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
// $Revision: 1.8 $
// $Date: 2004/05/21 05:14:35 $
//

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

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
	usersfile = NULL;
}

Main::~Main ()
{
	
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

void Main::SignalEvent (int sig)
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
	exit (sig);
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

void Main::TimeoutEvent (void)
{
	Logs.Add (Log::Debug, "Checking clients");

	// desconectar usuarios no válidos
	Server.DropNotValidClients ();
}

int Main::GenerateResponse (Client *Cl, Packet *Pkt)
{
	bool app_add = false;
	struct user *usr = NULL;
	char *md5hash = NULL;
	char hello_str[21], pass_str[81];

	if (Cl != NULL && Pkt != NULL)
	{
		switch (Pkt->GetType ())
		{
			case Pckt::SyncHello:
				snprintf (hello_str, 20, "SAGS Server %s", VERSION);
				Cl->AddBuffer (Pckt::SyncHello, hello_str);
				app_add = true;
				break;

			case Pckt::SyncVersion:
				// chequeamos las versiones
				if (!strncmp (Pkt->GetData (), "1", 1))
				{
					Cl->Add (new Packet (Pckt::SyncVersion, 1, 1, "1"));
					app_add = true;
				}
				else
				{
					Server.CloseConnection (Cl->ShowSocket (),
								Pckt::ErrorNotValidVersion);
					return 0;
				}
				break;

			case Pckt::AuthUsername:
				if (Cl->GetStatus () == Usr::NeedUser)
				{
					Cl->SetUsername (Pkt->GetData ());
					Cl->SetStatus (Usr::NeedPass);
					snprintf (pass_str, 80, "User %s needs password",
						  Cl->GetUsername ());
					Cl->AddBuffer (Pckt::AuthPassword, pass_str);
					app_add = true;
				}
				break;

			case Pckt::AuthPassword:
				if (Cl->GetStatus () == Usr::NeedPass)
				{
					usr = FindUser (Cl->GetUsername ());
					if (usr != NULL)
					{
						md5hash = md5_password_hash (Pkt->GetData ());
						int retval = strncmp (usr->hash, md5hash,
								      strlen (md5hash));
						//Logs.Add (Log::Debug, "Checking \"%s\" <%d> \"%s\"",
						//	  usr->hash, retval, md5hash);
						if (!retval)
						{
							// usuario exitosamente autenticado
							Cl->SetStatus (Usr::Real);
							Cl->Add (new Packet (Pckt::AuthSuccessful));
							app_add = true;

							Logs.Add (Log::Notice,
								  "User \"%s\" has logged in from %s",
								  Cl->GetUsername (), Cl->ShowIP ());

							// sacamos el timeout
							Server.RemoveWatch (Cl);
						}
						else
							Logs.Add (Log::Warning,
								  "User \"%s\" failed to get logged in",
								  Cl->GetUsername ());
						delete[] md5hash;
					}
					else
						Logs.Add (Log::Warning,
							  "User \"%s\" don't exists",
							  Cl->GetUsername ());
				}
				if (Cl->GetStatus () != Usr::Real)
				{
					Server.CloseConnection (Cl->ShowSocket (),
								Pckt::ErrorLoginFailed);
					return 0;
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
						Cl->Add (new Packet (Pckt::SessionConsoleSuccess));
					else
						Cl->Add (new Packet (Pckt::ErrorCantWriteToProcess));
					app_add = true;
				}
				break;

			case Pckt::SessionDisconnect:
			case Pckt::SessionDrop:
				// ahora basta que el cliente envíe uno
				// de estos paquetes para ser desconectado
				Logs.Add (Log::Notice,
					  "User \"%s\" has logged off",
					  Cl->GetUsername ());
				Cl->SetDrop (true);
				Server.CloseConnection (Cl->ShowSocket (), Pckt::Null);
				return 0;

			default:
				return -1;
		}
	}
	else
		return -1;
	
	if (app_add)
	{
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
	char line[CL_MAXNAME + HASHLEN + 2];
	char **ln;
	int i, j, two;

	if (!file.is_open ())
	{
		Logs.Add (Log::Warning,
			  "Failed to open the users file \"%s\"",
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

		//Logs.Add (Log::Debug, "Loaded user \"%s\" with hash \"%s\"", ln[0], ln[1]);
		AddUser (ln[0], ln[1]);

		// ln debe ser liberado
		if (two > 0)
		{
			for (j = two - 1; j >= 0; --j)
				delete[] ln[j];
		}
	}

	file.close ();

	Logs.Add (Log::Info,
		  "Loaded %d users from file \"%s\"",
		  userslist.GetCount (), usersfile->string);
}

void Main::AddUser (const char *name, const char *hash)
{
	struct user *newitem = new struct user (name, hash);
	userslist << newitem;
}

struct user *Main::FindUser (const char *name)
{
	struct user searched (name);
	return userslist.Find (searched);
}

void Main::PrintUsage (void)
{
	std::cerr << "Usage: " PACKAGE " [-D] [<config-file>]" << std::endl;
	std::cerr << "       -D: debug mode" << std::endl;
	exit (EXIT_FAILURE);
}

// definimos el objeto
Main Application;
