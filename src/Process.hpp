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
// $Source: /home/pablo/Desarrollo/sags-cvs/server/src/Process.hpp,v $
// $Revision: 1.5 $
// $Date: 2004/07/21 00:10:24 $
//

#ifndef __PROCESS_HPP__
#define __PROCESS_HPP__

#include <sys/types.h>
#include <ctime>

#include "Config.hpp"

class Process
{
private:
	bool dead;
	int index;
	int pty;
	pid_t pid;

	char **args;
	int nargs;
	char **env;
	int nenv;
	char *process_logs;

	struct option *name;
	struct option *description;
	struct option *type;
	struct option *command;
	struct option *environment;
	struct option *workdir;
	struct option *respawn;
	struct option *historylength;

	char current_command[CONF_MAX_STRING + 1];
	char current_environment[CONF_MAX_STRING + 1];
	char current_workdir[CONF_MAX_STRING + 1];
	int current_historylength;

	int num_start;
	time_t last_start;
	int last_value_returned;

public:
	Process (int idx, int fd = 0);
	~Process ();

	bool operator== (Process &P);

	void GetOptions (void);
	int CheckOptions (void);
	void Start (void);
	int Launch (void);
	void AddProcessData (const char *data);
	char *GetProcessData (int *len);
	int ReadData (char *buffer, int length);
	int WriteData (char *buffer);
	void Read (void);
	int Write (char *buffer);
	void WaitExit (void);
	int Kill (void);
	int SendSignal (int sig);
	void Restart (void);
	char *GetInfo (void);
	bool IsRunning (void);
};

#endif // __PROCESS_HPP__
