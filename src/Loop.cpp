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
// $Revision: 1.4 $
// $Date: 2004/05/19 02:53:43 $
//

#include <csignal>
#include <cerrno>

#include <sys/types.h>
#include <unistd.h>

#include "Loop.hpp"
#include "Log.hpp"

using namespace std;

static int killer_signal = 0;

//
// El manejador de señales
//
void CatchSignal (int sig)
{
	switch (sig)
	{
		case SIGTERM:
			Logs.Add (Log::Notice, "CatchSignal: Received SIGTERM");
			break;
		case SIGINT:
			Logs.Add (Log::Notice, "CatchSignal: Received SIGINT");
			break;
		default:
			;
	}
	killer_signal = sig;
}

SelectLoop::SelectLoop ()
{
	FD_ZERO (&reading);
	FD_ZERO (&writing);

	timeout = NULL;
	maxd = 0;
}

SelectLoop::~SelectLoop ()
{
	
}

void SelectLoop::AddToList (int owner, int fd)
{
	struct fditem *newitem = new struct fditem (owner, fd);
	fdlist << newitem;
}

void SelectLoop::RemoveFromList (int owner, int fd)
{
	struct fditem searched (owner, fd);
	fdlist.Remove (searched);
}

struct fditem *SelectLoop::FindInList (int owner, int fd)
{
	struct fditem searched (owner, fd);
	return fdlist.Find (searched);
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
			int i, maximus = fdlist.GetCount ();

			for (i = 0, maxd = 0; i <= maximus - 1; ++i)
			{
				if (fdlist[i]->fd > maxd)
					maxd = fdlist[i]->fd;
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
	struct timespec *tmout;
	int i, maximus, select_output;
	fd_set rd, wr;

	while (1)
	{
		// atendemos las señales asesinas ;)
		if (killer_signal)
			SignalEvent (killer_signal);

		rd = reading;
		wr = writing;

		tmout = GetTimeout ();

		// pselect no funciona correctamente en Linux :(
		// pero a mí no me ha dado problemas :o
		select_output = pselect (maxd + 1, &rd, &wr, NULL, tmout,
					 &original_sigmask);

		if (select_output < 0 && errno == EINTR)
		{
			// pselect interrumpido por una señal
			Logs.Add (Log::Debug, "pselect was interrupted");
			continue;
		}

		if (tmout != NULL)
			TimeoutEvent ();

		maximus = fdlist.GetCount ();

		for (i = 0; i <= maximus - 1; ++i)
		{
			list = fdlist[i];

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

void SelectLoop::AddTimeout (int seconds)
{
	if (timeout != NULL)
	{
		if (timeout->tv_sec != seconds)
			delete timeout;
		else
			return;
	}

	timeout = new struct timespec;
	timeout->tv_sec = seconds;
	timeout->tv_nsec = 0;

	Logs.Add (Log::Debug, "Timeout seted to %d seconds", seconds);
}

void SelectLoop::DeleteTimeout (void)
{
	if (timeout != NULL)
	{
		delete timeout;
		timeout = NULL;
		Logs.Add (Log::Debug, "Timeout removed");
	}
}

struct timespec *SelectLoop::GetTimeout (void)
{
	struct timespec *newtimeout = NULL;

	if (timeout != NULL)
	{
		newtimeout = new struct timespec;
		newtimeout->tv_sec = timeout->tv_sec;
		newtimeout->tv_nsec = timeout->tv_sec;
	}

	return newtimeout;
}
