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
// $Source: /home/pablo/Desarrollo/sags-cvs/server/src/ProcTree.hpp,v $
// $Revision: 1.1 $
// $Date: 2004/06/16 00:52:49 $
//

#ifndef __PROCTREE_HPP__
#define __PROCTREE_HPP__

#include "Process.hpp"
#include "List.hpp"

class ProcTree
{
private:
	List<Process> ProcList;

public:
	ProcTree ();
	~ProcTree ();

	Process *Find (int fd);

	void Start (void);
	void Check (void);
	void CloseAll (void);
	void ReadFrom (int fd);
	int WriteTo (int fd, char *buffer);
	char *GetProcessData (unsigned int idx, int *len);
	int WriteToProcess (unsigned int idx, char *buffer);
	char *GetProcessInfo (unsigned int idx);
};

extern ProcTree ProcMaster;

#endif // __PROCTREE_HPP__
