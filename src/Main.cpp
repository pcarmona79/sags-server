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
// $Revision: 1.26 $
// $Date: 2005/02/10 21:55:03 $
//

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <iostream>
#include <cstdlib>
#include <csignal>

#include "Main.hpp"
#include "ProcTree.hpp"
#include "Network.hpp"
#include "Log.hpp"
#include "Channel.hpp"

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

int Main::SignalEvent (int sig)
{
	switch (sig)
	{
		case SIGHUP:
			// releeemos los archivos de configuración y
			// el archivo de usuarios

			Logs.Add (Log::Notice, "Reading configuration file");
			Config.GetOptionsFromFile ();

			Logs.Add (Log::Notice, "Reading users file");
			Application.LoadUsers ();

			Logs.Add (Log::Notice, "Checking network");
			Server.CheckOptions ();

			Logs.Add (Log::Notice, "Checking processes");
			ProcMaster.Check ();

			break;

		case SIGTERM:
		case SIGINT:
			// Este bloque se llama cuando hay eventos que
			// quieren que el programa termine. Se debe salir
			// ordenadamente.

			// detenemos los procesos
			ProcMaster.CloseAll ();

			// desconectamos a los clientes
			Server.Shutdown ();

			// una salida distinta de cero terminará el programa
			return sig;

		default:
			Logs.Add (Log::Warning, "Unhandled event %d", sig);
	}

	return 0;
}

void Main::DataEvent (int owner, int fd, bool writing)
{
	Logs.Add (Log::Debug,
		  "DataEvent: owner=%04X fd=%d writing=%s",
		  owner, fd, (writing) ? "true": "false");

	switch (owner & Owner::Mask)
	{
		case Owner::Process:
			//Logs.Add (Log::Debug, "Reading from child process");
			ProcMaster.ReadFrom (fd);
			break;

		case Owner::Network:
			//Logs.Add (Log::Debug, "Accepting connection");
			Server.AcceptConnection (fd);
			break;

		case Owner::Client:
			if ((owner & Owner::Send) && writing)
			{
				// socket listo para enviar
				//Logs.Add (Log::Debug, "Sending data to client");
				Server.SendData (fd);
			}
			else
			{
				//Logs.Add (Log::Debug, "Receiving data from client");
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
	int dropit = -1;

	if (Cl != NULL && Pkt != NULL)
	{
		switch (Pkt->GetIndex ())
		{
			case Sync::Index:
				dropit = ProtoSync (Cl, Pkt);
				break;

			case Auth::Index:
				dropit = ProtoAuth (Cl, Pkt);
				break;

			case Error::Index:
				// clientes no envían mensajes de error
				return -1;

			default:
				// paquetes de sesión de un proceso
				if (Pkt->GetIndex () <= Session::MaxIndex)
					dropit = ProtoSession (Cl, Pkt);
				else
					return -1;
		}
	}

	return dropit;
}

int Main::ProtoSync (Client *Cl, Packet *Pkt)
{
	char hello_str[81];

	switch (Pkt->GetCommand ())
	{
	case Sync::Hello:

		snprintf (hello_str, 81, "Welcome %s to SAGS Service", Cl->ShowIP ());

		Cl->AddBuffer (Sync::Index, Sync::Hello, hello_str);
		Add (Owner::Client | Owner::Send, Cl->ShowSocket ());
		break;

	case Sync::Version:

		// chequeamos las versiones
		if (!strncmp (Pkt->GetData (), PCKT_VERSION, 1))
		{
			Cl->Add (new Packet (Sync::Index, Sync::Version, 1, 1, PCKT_VERSION));
			Add (Owner::Client | Owner::Send, Cl->ShowSocket ());
		}
		else
			Server.CloseConnection (Cl->ShowSocket (), Error::Index,
						Error::NotValidVersion);
		break;

	default:
		return -1;
	}

	return 0;
}

int Main::ProtoAuth (Client *Cl, Packet *Pkt)
{
	struct user *usr = NULL;

	switch (Pkt->GetCommand ())
	{
	case Auth::Username:

		if (Cl->GetStatus () == Usr::NeedUser)
		{
			Cl->SetUsername (Pkt->GetData ());
			Cl->SetStatus (Usr::NeedPass);

			// ahora le enviamos la cadena aleatoria
			usr = FindUser (Cl->GetUsername ());
			if (usr == NULL)
			{
				Logs.Add (Log::Warning,
					  "User \"%s\" don't exists",
					  Cl->GetUsername ());

				// enviamos una cadena aleatoria
				// ficticia para despistar :)
				char dumbstr[HASHLEN + 1];
				random_string (dumbstr, HASHLEN);
				Cl->AddBuffer (Auth::Index, Auth::RandomHash, dumbstr);
			}
			else
			{
				// actualizamos la cadena aleatoria
				usr->update ();
				Cl->AddBuffer (Auth::Index, Auth::RandomHash, usr->rndstr);
			}

			Logs.Add (Log::Info,
				  "Sending random string to user \"%s\"",
				  Cl->GetUsername ());
			Add (Owner::Client | Owner::Send, Cl->ShowSocket ());
		}
		break;

	case Auth::Password:

		if (Cl->GetStatus () == Usr::NeedPass)
		{
			usr = FindUser (Cl->GetUsername ());
			if (usr != NULL)
			{
				int retval = strncmp (usr->hash, Pkt->GetData (),
						      strlen (usr->hash));
				//Logs.Add (Log::Debug, "Checking \"%s\" <%d> \"%s\"",
				//	  usr->hash, retval, Pkt->GetData ());

				if (!retval)
				{
					// removemos clientes conectados con el mismo
					// nombre de usuario
					Server.DropDuplicatedClients (Cl);

					// usuario exitosamente autenticado
					Cl->SetStatus (Usr::Real);
					Cl->Add (new Packet (Auth::Index, Auth::Successful));
					Add (Owner::Client | Owner::Send, Cl->ShowSocket ());

					Logs.Add (Log::Notice,
						  "User \"%s\" has logged in from %s",
						  Cl->GetUsername (), Cl->ShowIP ());

					// sacamos el timeout
					Server.RemoveWatch (Cl);

					// agregamos al cliente los servidores autorizados
					// y su modo de mantenimiento
					AddAuthorizedProcesses (Cl, usr);

					// ingresamos al usuario al canal general
					GeneralChannel.UserJoin (Cl);
				}
				else
					Logs.Add (Log::Warning,
						  "User \"%s\" failed to get logged in",
						  Cl->GetUsername ());
			}
			else
				Logs.Add (Log::Warning,
					  "User \"%s\" don't exists",
					  Cl->GetUsername ());
		}
		if (Cl->GetStatus () != Usr::Real)
		{
			Server.CloseConnection (Cl->ShowSocket (), Error::Index,
						Error::LoginFailed);
			return 0;
		}
		break;

	default:
		return -1;
	}

	return 0;
}

int Main::ProtoSession (Client *Cl, Packet *Pkt)
{
	char serv_idx[3], *bufinfo = NULL;

	switch (Pkt->GetCommand ())
	{
	case Session::ConsoleNeedLogs:

		if (Cl->IsAuthorized (Pkt->GetIndex ()))
		{
			Server.SendProcessLogs (Cl, Pkt->GetIndex ());
			break;
		}
		return -1;

	case Session::ConsoleInput:

		if (Cl->IsAuthorized (Pkt->GetIndex ()))
		{
			if (ProcMaster.WriteToProcess (Pkt->GetIndex (), Pkt->GetData ()) > 0)
				Cl->Add (new Packet (Pkt->GetIndex (),
						     Session::ConsoleSuccess));
			else
			{
				snprintf (serv_idx, 3, "%02X", Pkt->GetIndex ());
				Cl->Add (new Packet (Error::Index, Error::CantWriteToProcess,
						     1, strlen (serv_idx), serv_idx));
			}

			Add (Owner::Client | Owner::Send, Cl->ShowSocket ());
			break;
		}
		return -1;

	case Session::ProcessGetInfo:

		// el cliente solicita la información del
		// proceso dado por el índice
		if (Cl->IsAuthorized (Pkt->GetIndex ()))
		{
			bufinfo = ProcMaster.GetProcessInfo (Pkt->GetIndex ());

			if (bufinfo != NULL)
			{
				Cl->AddBuffer (Pkt->GetIndex (), Session::ProcessInfo,
					       bufinfo);
				Add (Owner::Client | Owner::Send, Cl->ShowSocket ());

				// liberamos el espacio asignado
				delete[] bufinfo;
				bufinfo = NULL;
			}
			break;
		}
		return -1;

	case Session::ProcessKill:

		if (Cl->IsAuthorized (Pkt->GetIndex ()))
		{
			if (ProcMaster.KillProcess (Pkt->GetIndex ()))
			{
				snprintf (serv_idx, 3, "%02X", Pkt->GetIndex ());
				Cl->Add (new Packet (Error::Index, Error::ProcessNotKilled,
						     1, strlen (serv_idx), serv_idx));
				Add (Owner::Client | Owner::Send, Cl->ShowSocket ());
			}
			break;
		}
		return -1;

	case Session::ProcessLaunch:

		if (Cl->IsAuthorized (Pkt->GetIndex ()))
		{
			if (ProcMaster.LaunchProcess (Pkt->GetIndex ()))
			{
				snprintf (serv_idx, 3, "%02X", Pkt->GetIndex ());
				Cl->Add (new Packet (Error::Index, Error::ProcessNotLaunched,
						     1, strlen (serv_idx), serv_idx));
				Add (Owner::Client | Owner::Send, Cl->ShowSocket ());
			}
			break;
		}
		return -1;

	case Session::ProcessRestart:

		if (Cl->IsAuthorized (Pkt->GetIndex ()))
		{
			if (ProcMaster.RestartProcess (Pkt->GetIndex ()))
			{
				snprintf (serv_idx, 3, "%02X", Pkt->GetIndex ());
				Cl->Add (new Packet (Error::Index, Error::ProcessNotRestarted,
						     1, strlen (serv_idx), serv_idx));
				Add (Owner::Client | Owner::Send, Cl->ShowSocket ());
			}
			break;
		}
		return -1;

	case Session::Disconnect:

		// ahora basta que el cliente envíe uno
		// de estos paquetes para ser desconectado
		Logs.Add (Log::Notice,
			  "User \"%s\" has logged out",
			  Cl->GetUsername ());
		Cl->SetDrop (true);
		Server.CloseConnection (Cl->ShowSocket (), 0, 0);
		break;

	case Session::ChatTopicChange:

		GeneralChannel.ChangeTopic (Cl, Pkt);
		break;

	case Session::ChatMessage:
	case Session::ChatAction:
	case Session::ChatNotice:

		if (GeneralChannel.MessageChannel (Cl, Pkt))
			return -1;
		break;

	case Session::ChatPrivMessage:
	case Session::ChatPrivAction:
	case Session::ChatPrivNotice:

		if (GeneralChannel.MessagePrivate (Cl, Pkt))
			return -1;
		break;

	case Session::MaintenanceOn:

		// cambiar a modo de mantenimiento al proceso indicado
		// en DATA, sólo si es administrador
		if (Cl->IsAuthorized (0))
		{
			Logs.Add (Log::Notice,
				  "User \"%s\" is changing maintenance mode of process %d",
				  Cl->GetUsername (), Pkt->GetIndex ());
			if (ProcMaster.SetMaintenanceMode (Pkt->GetIndex (), true))
			{
				Cl->Add (new Packet (Error::Index, Error::MaintenanceDenied));
				Add (Owner::Client | Owner::Send, Cl->ShowSocket ());
			}
			else
			{
				// informamos a todos del cambio
				Server.SendToAllClients (Pkt->GetIndex (),
							 Session::MaintenanceOn);
			}
		}
		else
		{
			// enviamos error
			Cl->Add (new Packet (Error::Index, Error::MaintenanceDenied));
			Add (Owner::Client | Owner::Send, Cl->ShowSocket ());
		}
		break;

	case Session::MaintenanceOff:

		// salir del modo de mantenimiento del proceso indicado
		// en DATA, sólo si es administrador
		if (Cl->IsAuthorized (0))
		{
			Logs.Add (Log::Notice,
				  "User \"%s\" is changing maintenance mode of process %d",
				  Cl->GetUsername (), Pkt->GetIndex ());

			if (ProcMaster.SetMaintenanceMode (Pkt->GetIndex (), false))
			{
				Cl->Add (new Packet (Error::Index, Error::MaintenanceDenied));
				Add (Owner::Client | Owner::Send, Cl->ShowSocket ());
			}
			else
			{
				// informamos a todos del cambio
				Server.SendToAllClients (Pkt->GetIndex (),
							 Session::MaintenanceOff);
			}
		}
		else
		{
			// enviamos error
			Cl->Add (new Packet (Error::Index, Error::MaintenanceDenied));
			Add (Owner::Client | Owner::Send, Cl->ShowSocket ());
		}
		break;

	default:
		return -1;
	}

	return 0;
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
	int i, j, three;

	if (!file.is_open ())
	{
		Logs.Add (Log::Warning,
			  "Failed to open the users file \"%s\"",
			  usersfile->string);
		return;
	}

	// limpiamos la lista de usuarios
	if (userslist.GetCount () > 0)
		userslist.Clear ();

	for (i = 1; !file.eof(); ++i)
	{
		file.getline (line, CL_MAXNAME + HASHLEN);
		//Logs.Add (Log::Debug, "%2d %s", i, line);

		if (line[0] == '\0')
			continue;

		ln = strsplit (line, ':', -1, &three);
		if (three != 3)
		{
			Logs.Add (Log::Warning, "Skipping malformed line %d", i);
			continue;
		}

		//Logs.Add (Log::Debug, "Loaded user \"%s\" (\"%s\",\"%s\")",
		//	  ln[0], ln[1], ln[2]);

		AddUser (ln[0], ln[1], ln[2]);

		// ln debe ser liberado
		if (three > 0)
		{
			for (j = three - 1; j >= 0; --j)
				delete[] ln[j];
		}
	}

	file.close ();

	Logs.Add (Log::Info,
		  "Loaded %d users from file \"%s\"",
		  userslist.GetCount (), usersfile->string);
}

void Main::AddUser (const char *name, const char *pass, const char *procs)
{
	struct user *newitem = new struct user (name, pass, procs);
	userslist << newitem;
}

struct user *Main::FindUser (const char *name)
{
	struct user searched (name);
	return userslist.Find (searched);
}

void Main::AddAuthorizedProcesses (Client *Cl, struct user *usr)
{
	char **pcs;
	int np = 0, i, idx;

	pcs = strsplit (usr->procs, ',', -1, &np);

	for (i = 0; i <= np - 1; ++i)
	{
		idx = (int) strtol (pcs[i], (char **)NULL, 10);

		if (idx >= 0 && idx <= 252)
			Cl->SetAuthorizedProcess (idx);
	}

	for (i = np - 1; i >= 0; --i)
		delete[] pcs[i];
}

void Main::PrintUsage (void)
{
	std::cerr << "Usage: " PACKAGE " [-D] [<config-file>]" << std::endl;
	std::cerr << "       -D: debug mode" << std::endl;
	exit (EXIT_FAILURE);
}

// definimos el objeto
Main Application;
