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
// $Source: /home/pablo/Desarrollo/sags-cvs/server/src/Loop.hpp,v $
// $Revision: 1.4 $
// $Date: 2004/05/19 02:53:43 $
//

#ifndef __LOOP_HPP__
#define __LOOP_HPP__

#include <sys/types.h>
#include <ctime>
#include "List.hpp"

namespace Owner
{
	typedef enum
	{
		Process = 0x01,
		Network = 0x02,
		Client  = 0x03
	} Class;

	const int Recv = 0x00;
	const int Send = 0xF0;
	const int Mask = 0x0F;
}

struct fditem
{
	int fd;
	int owner;

	fditem (int o = 0, int f = 0)
	{
		owner = o;
		fd = f;
	}

	bool operator== (const struct fditem &fi)
	{
		return (owner == fi.owner && fd == fi.fd);
	}
};

class SelectLoop
{
protected:
	fd_set reading;
	fd_set writing;
	struct timespec *timeout;
	int maxd;
	List<struct fditem> fdlist;
	sigset_t sigmask;
	sigset_t original_sigmask;

	void AddToList (int owner, int fd);
	void RemoveFromList (int owner, int fd);
	struct fditem *FindInList (int owner, int fd);
	void CalculateMaxd (bool removing, int fd);

public:
	SelectLoop ();
	virtual ~SelectLoop ();

	void Init (void);
	void Run (void);
	virtual void SignalEvent (int sig = 0) = 0;
	virtual void DataEvent (int owner, int fd) = 0;
	virtual void TimeoutEvent (void) = 0;
	void Add (int owner, int fd);
	void Remove (int owner, int fd);
	void AddTimeout (int seconds);
	void DeleteTimeout (void);
	struct timespec *GetTimeout (void);
};

#endif // __LOOP_HPP__
