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
// $Revision: 1.4 $
// $Date: 2004/06/16 00:52:49 $
//

#ifndef __CONFIG_HPP__
#define __CONFIG_HPP__

#include <fstream>
#include <cstring>
#include "List.hpp"

using namespace std;

#define CONF_MAX_NAME      50
#define CONF_MAX_STRING   100
#define CONF_MAX_LINE     (CONF_MAX_NAME*2 + CONF_MAX_STRING + 2)

namespace Conf
{
	typedef enum
	{
		Null = 0,
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

	option (const char *g = NULL, const char *n = NULL, Conf::OpType t = Conf::Null)
	{
		type = t;
		memset (group, 0, CONF_MAX_NAME);
		memset (name, 0, CONF_MAX_NAME);
		if (g != NULL)
			strncpy (group, g, CONF_MAX_NAME);
		if (n != NULL)
			strncpy (name, n, CONF_MAX_NAME);
		value = 0;
		memset (string, 0, CONF_MAX_STRING);
	}

	bool operator== (const struct option &opt)
	{
		if (!strncasecmp (group, opt.group, CONF_MAX_NAME) &&
		    !strncasecmp (name, opt.name, CONF_MAX_NAME))
			return true;
		return false;
	}
};

class Configuration
{
private:
	List<struct option> list;
	ifstream *ConfigFile;
	const char *FileName;

public:
	Configuration ();
	~Configuration ();

	void GetOptionsFromFile (ifstream *file = NULL, const char *filename = NULL);
	struct option *Add (Conf::OpType type, const char *group, const char *name, int val);
	struct option *Add (Conf::OpType type, const char *group, const char *name, const char *val);
	struct option *Get (Conf::OpType type, const char *group, const char *name, int val);
	struct option *Get (Conf::OpType type, const char *group, const char *name, const char *val);
	void Set (Conf::OpType type, const char *group, const char *name, int val);
	void Set (Conf::OpType type, const char *group, const char *name, const char *val);
	bool Check (const char *group, const char *name);
};

extern Configuration Config;

#endif // __CONFIG_HPP__
