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
// $Source: /home/pablo/Desarrollo/sags-cvs/server/src/Log.hpp,v $
// $Revision: 1.4 $
// $Date: 2004/08/13 00:55:57 $
//

#ifndef __LOG_HPP__
#define __LOG_HPP__

#include <fstream>

#include "Config.hpp"

using namespace std;

#define LOG_MAX_STRING  255
#define LOG_MAX_HEADER   20

namespace Log
{
	typedef enum
	{
		// typical main logs
		Critical = 0x0000,
		Error    = 0x0001,
		Warning  = 0x0002,
		Notice   = 0x0003,
		Info     = 0x0004,
		Debug    = 0x0005,

		// logs for network
		Network  = 0x0010,

		// logs for process
		Process  = 0x0020,

		//logs for logging
		Logging  = 0x0030,

		// logs for configs
		Config   = 0x0040,

		// logs for clients
		Client   = 0x0050,

		// logs for ProcTree
		ProcTree = 0x0060,

		// logs for GeneralChannel
		Chat     = 0x0070
	} Type;
};

class Logging
{
private:
	bool standard_output;
	ofstream file;
	struct option *debug;
	struct option *logfile;
	
public:
	Logging ();
	~Logging ();

	void AddOptions (void);
	void Start (void);
	void Add (int type, const char* fmt, ...);
};

extern Logging Logs;

#endif // __LOG_HPP__
