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
// $Source: /home/pablo/Desarrollo/sags-cvs/server/src/Loop.cpp,v $
// $Revision: 1.1 $
// $Date: 2004/04/13 22:00:19 $
//

#include <csignal>
#include <cerrno>

#include <sys/types.h>
#include <unistd.h>

#include "Loop.hpp"
#include "Log.hpp"

using namespace std;

static int killer_events = 0;

//
// El manejador de señales
//
void CatchSignal (int sig)
{
	switch (sig)
	{
		case SIGTERM:
			Logs.Add (Log::Notice, "CatchSignal: Received SIGTERM");
			++killer_events;
			break;
		case SIGINT:
			Logs.Add (Log::Notice, "CatchSignal: Received SIGINT");
			++killer_events;
			break;
		default:
			;
	}
}

SelectLoop::SelectLoop ()
{
	FD_ZERO (&reading);
	FD_ZERO (&writing);

	maxd = 0;
	fdlist = NULL;
}

SelectLoop::~SelectLoop ()
{
	struct fditem *unlink;

	// borramos la lista enlazada de los
	// descriptores de archivo
	while (fdlist)
	{
		unlink = fdlist;
		fdlist = unlink->next;
		delete unlink;
	}
}

void SelectLoop::AddToList (int owner, int fd)
{
	struct fditem *newitem, *searched = fdlist;

	newitem = new struct fditem;
	newitem->fd = fd;
	newitem->owner = owner;
	newitem->next = NULL;

	if (searched)
	{
		// buscamos el último elemento
		while (searched->next)
			searched = searched->next;
		searched->next = newitem;
	}
	else
		fdlist = newitem;  // lista estaba vacía
}

void SelectLoop::RemoveFromList (int owner, int fd)
{
	struct fditem *last = NULL, *searched = fdlist;

	while (searched)
	{
		if (searched->owner == owner && searched->fd == fd)
		{
			if (last)
				last->next = searched->next;
			else
				fdlist = searched->next;
			delete searched;
			return;
		}
		last = searched;
		searched = searched->next;
	}
}

struct fditem *SelectLoop::FindInList (int owner, int fd)
{
	struct fditem *searched = fdlist;

	while (searched)
	{
		if (searched->owner == owner && searched->fd == fd)
			return searched;
		
		searched = searched->next;
	}

	return NULL;
}

void SelectLoop::CalculateMaxd (bool removing, int fd)
{
	if (!removing)
	{
		if (fd > maxd)
			maxd = fd;
	}
	else if (removing)
	{
		if (fd == maxd)
		{
			// encontrar el mayor fd
			struct fditem *searched = fdlist;

			maxd = 0;

			while (searched)
			{
				if (searched->fd > maxd)
					maxd = searched->fd;
				searched = searched->next;
			}
		}
	}
}

void SelectLoop::Init (void)
{
	struct sigaction act;

	sigemptyset (&sigmask);
	sigaddset (&sigmask, SIGTERM);
	sigaddset (&sigmask, SIGINT);
	sigprocmask (SIG_BLOCK, &sigmask, &original_sigmask);

	act.sa_handler = CatchSignal;
	act.sa_flags = 0;
	sigaction (SIGTERM, &act, NULL);
	sigaction (SIGINT, &act, NULL);
}

void SelectLoop::Run (void)
{
	struct fditem *list;
	int select_output;
	fd_set rd, wr;

	while (1)
	{
		// atendemos las señales SIGCHLD
		for (; killer_events > 0; --killer_events)
			SignalEvent ();

		rd = reading;
		wr = writing;

		// pselect no funciona en Linux :(
		select_output = pselect (maxd + 1, &rd, &wr, NULL, NULL,
					 &original_sigmask);
		//select_output = select (maxd + 1, &rd, &wr, NULL, NULL);

		if (select_output < 0 && errno == EINTR)
		{
			// pselect interrumpido por una señal
			Logs.Add (Log::Debug, "pselect was interrupted");
			continue;
		}

		for (list = fdlist; list; list = list->next)
		{
			// buscamos descriptores listos para escribir
			if (FD_ISSET (list->fd, &wr))
			{
				DataEvent (list->owner, list->fd);
				break;
			}

			// buscamos descriptores listos para leer
			if (FD_ISSET (list->fd, &rd))
			{
				DataEvent (list->owner, list->fd);
				break;
			}
		}
	}
}

void SelectLoop::Add (int owner, int fd)
{
	if (owner & Owner::Send)
	{
		FD_SET (fd, &writing);

		struct fditem *finded = FindInList (owner & Owner::Mask, fd);
		if (finded != NULL)
			finded->owner = finded->owner | Owner::Send;
	}
	else
	{
		FD_SET (fd, &reading);
	
		AddToList (owner, fd);
		CalculateMaxd (false, fd);
	}
}

void SelectLoop::Remove (int owner, int fd)
{
	if (owner & Owner::Send)
	{
		FD_CLR (fd, &writing);

		struct fditem *finded = FindInList (owner, fd);
		if (finded != NULL)
			finded->owner = finded->owner & Owner::Mask;
	}
	else
	{
		FD_CLR (fd, &reading);

		RemoveFromList (owner, fd);
		CalculateMaxd (true, fd);
	}
}
