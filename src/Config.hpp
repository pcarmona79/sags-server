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
// $Source: /home/pablo/Desarrollo/sags-cvs/server/src/Config.hpp,v $
// $Revision: 1.1 $
// $Date: 2004/04/13 22:00:20 $
//

#ifndef __CONFIG_HPP__
#define __CONFIG_HPP__

#include <fstream>

using namespace std;

#define CONF_MAX_NAME      50
#define CONF_MAX_STRING   100
#define CONF_MAX_LINE     (CONF_MAX_NAME*2 + CONF_MAX_STRING + 2)

namespace Conf
{
	typedef enum
	{
		Boolean,
		Numeric,
		String
	} OpType;
}

struct option
{
	Conf::OpType type;
	char group[CONF_MAX_NAME + 1];
	char name[CONF_MAX_NAME + 1];
	int value;
	char string[CONF_MAX_STRING + 1];
};

struct option_list
{
	struct option *option;
	struct option_list *next;
};

class Configuration
{
private:
	struct option_list *list;

	void AddToList (struct option *opt);
	struct option *Find (const char *group, const char *name);

public:
	Configuration ();
	~Configuration ();

	void GetOptionsFromFile (ifstream& file);
	struct option *Add (Conf::OpType type, const char *group, const char *name, int val);
	struct option *Add (Conf::OpType type, const char *group, const char *name, const char *val);
	struct option *Get (const char *group, const char *name);
	void Set (Conf::OpType type, const char *group, const char *name, int val);
	void Set (Conf::OpType type, const char *group, const char *name, const char *val);
};

extern Configuration Config;

#endif // __CONFIG_HPP__
