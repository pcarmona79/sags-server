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
// $Source: /home/pablo/Desarrollo/sags-cvs/server/src/ProcTree.cpp,v $
// $Revision: 1.3 $
// $Date: 2004/06/17 08:13:23 $
//

#include "ProcTree.hpp"
#include "Log.hpp"
#include "Config.hpp"

ProcTree::ProcTree ()
{
	
}

ProcTree::~ProcTree ()
{
	
}

Process *ProcTree::Find (int fd)
{
	Process Searched (0, fd);
	return ProcList.Find (Searched);
}

void ProcTree::Start (void)
{
	int i;
	char group[11];

	// 252 es el máximo permitido por el protocolo
	for (i = 1; i <= 252; ++i)
	{
		memset (group, 0, 11);
		snprintf (group, 11, "Process%d", i);

		//Logs.Add (Log::ProcTree | Log::Debug,
		//	  "Searching for group %s",
		//	  group);

		if (Config.Check (group, "Command"))
		{
			// existe el grupo
			Logs.Add (Log::ProcTree | Log::Info,
				  "Creating process %d", i);

			// El constructor de Process obtiene su
			// configuración según su índice y luego
			// se inicia solo
			ProcList << new Process (i);
		}
		else
		{
			// no existe, entonces salimos
			//Logs.Add (Log::ProcTree | Log::Debug,
			//	  "Group %s not found",
			//	  group);
			break;
		}
	}

	if (i > 1)
		Logs.Add (Log::ProcTree | Log::Info,
			  "Launched %d processes", i - 1);
	else
		Logs.Add (Log::ProcTree | Log::Info,
			  "No processes launched");
}

void ProcTree::Check (void)
{
	unsigned int i, maximus = ProcList.GetCount ();
	char group[11];

	for (i = 1; i <= maximus; ++i)
	{
		if (ProcList[i - 1]->CheckOptions ())
		{
			// remover
			Logs.Add (Log::ProcTree | Log::Notice,
				  "Removing process %d",
				  i - 1);
			ProcList.Remove (ProcList[i - 1]);
			break;
		}
	}

	// buscamos más entradas sólo si no se borró un proceso
	if (maximus == ProcList.GetCount ())
	{
		Logs.Add (Log::ProcTree | Log::Debug,
			  "Searching for another processes");

		// 252 es el máximo permitido por el protocolo
		for (i = ProcList.GetCount () + 1; i <= 252; ++i)
		{
			memset (group, 0, 11);
			snprintf (group, 11, "Process%d", i);

			//Logs.Add (Log::ProcTree | Log::Debug,
			//	  "Searching for group %s",
			//	  group);

			if (Config.Check (group, "Command"))
			{
				// existe el grupo
				Logs.Add (Log::ProcTree | Log::Info,
					  "Creating process %d", i);

				// El constructor de Process obtiene su
				// configuración según su índice y luego
				// se inicia solo
				ProcList << new Process (i);
			}
			else
			{
				// no existe, entonces salimos
				//Logs.Add (Log::ProcTree | Log::Debug,
				//	  "Group %s not found",
				//	  group);
				break;
			}
		}

		if (i - 1 > maximus)
			Logs.Add (Log::ProcTree | Log::Info,
				  "Launched %d new processes", i - 1 - maximus);
		else
			Logs.Add (Log::ProcTree | Log::Info,
				  "No new processes launched");
	}
}

void ProcTree::CloseAll (void)
{
	int i, maximus = ProcList.GetCount ();

	for (i = 0; i <= maximus - 1; ++i)
	{
		ProcList[i]->Kill ();
		ProcList[i]->WaitExit ();
	}
}

void ProcTree::ReadFrom (int fd)
{
	Process *Rd = Find (fd);

	if (Rd != NULL)
		Rd->Read ();
}

int ProcTree::WriteTo (int fd, char *buffer)
{
	Process *Wr = Find (fd);

	if (Wr != NULL)
		return Wr->Write (buffer);
	else
		return -1;
}

char *ProcTree::GetProcessData (unsigned int idx, int *len)
{
	if (idx < 1 && idx > ProcList.GetCount ())
	{
		len = 0;
		return NULL;
	}

	return ProcList[idx - 1]->GetProcessData (len);
}

int ProcTree::WriteToProcess (unsigned int idx, char *buffer)
{
	if (idx < 1 && idx > ProcList.GetCount ())
		return -1;

	return ProcList[idx - 1]->Write (buffer);
}

char *ProcTree::GetProcessInfo (unsigned int idx)
{
	// TODO: con idx = 0 se debería entregar información
	//       del servidor

	if (idx < 1 && idx > ProcList.GetCount ())
		return NULL;

	return ProcList[idx - 1]->GetInfo ();

}

bool ProcTree::IsProcess (unsigned int idx)
{
	return (idx >= 1 && idx <= ProcList.GetCount ());
}

// definimos el objeto
ProcTree ProcMaster;
