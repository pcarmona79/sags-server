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
// $Source: /home/pablo/Desarrollo/sags-cvs/server/src/Log.cpp,v $
// $Revision: 1.1 $
// $Date: 2004/04/13 22:00:19 $
//

#include <iostream>
#include <fstream>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <ctime>

#include "Log.hpp"
#include "Main.hpp"

using namespace std;

Logging::Logging ()
{
	standard_output = false;
	debug = NULL;
	logfile = NULL;
}

Logging::~Logging ()
{
	file.close ();
	delete debug;
	delete logfile;
}

void Logging::AddOptions (void)
{
	logfile = Config.Add (Conf::String, "Logging", "File", "/dev/null");
	debug = Config.Add (Conf::Boolean, "Logging", "Debug", 0);
}

void Logging::Start (void)
{
	if (Application.IsDebugging ())
	{
		standard_output = true;
		if (file.is_open ())
			file.close ();
		debug->value = 1;
	}
	else if (!strncasecmp (logfile->string, "stdout", 6))
	{
		standard_output = true;
		if (file.is_open ())
			file.close ();
	}
	else
	{
		if (file.is_open ())
			file.close ();

		file.open (logfile->string, ios::out | ios::app);

		if (!file.is_open ())
			standard_output = true;
		else
			standard_output = false;
	}
}

void Logging::Add (int type, const char* fmt, ...)
{
	char msg[LOG_MAX_STRING + 1], hdr[LOG_MAX_HEADER + 1], tm[LOG_MAX_STRING + 1];
	va_list ap;
	bool shutdown = false;
	time_t logtime;

	time (&logtime);
	strncpy (tm, ctime (&logtime), LOG_MAX_STRING);
	tm[strlen (tm) - 1] = '\0'; // borramos el último \n

	va_start (ap, fmt);
	vsnprintf (msg, LOG_MAX_STRING, fmt, ap);
	va_end (ap);

	switch (type & 0xF0)
	{
		case Log::Network:
			strncpy (hdr, "[Network] ", LOG_MAX_HEADER);
			break;
		case Log::Process:
			strncpy (hdr, "[Process] ", LOG_MAX_HEADER);
			break;
		case Log::Logging:
			strncpy (hdr, "[Logging] ", LOG_MAX_HEADER);
			break;
		case Log::Config:
			strncpy (hdr, "[Config] ", LOG_MAX_HEADER);
			break;
		case Log::Client:
			strncpy (hdr, "[Client] ", LOG_MAX_HEADER);
			break;
		default:
			strncpy (hdr, "[Main] ", LOG_MAX_HEADER);
	}
	switch (type & 0x0F)
	{
		case Log::Critical:
			strncat (hdr, "Critical: ", LOG_MAX_HEADER);
			shutdown = true;
			break;
		case Log::Error:
			strncat (hdr, "Error: ", LOG_MAX_HEADER);
			break;
		case Log::Warning:
			strncat (hdr, "Warning: ", LOG_MAX_HEADER);
			break;
		case Log::Notice:
			strncat (hdr, "Notice: ", LOG_MAX_HEADER);
			break;
		case Log::Info:
			strncat (hdr, "Info: ", LOG_MAX_HEADER);
			break;
		case Log::Debug:
			if (!debug->value)
				return;
			strncat (hdr, "Debug: ", LOG_MAX_HEADER);
			break;
		default:
			strncat (hdr, "Unknown: ", LOG_MAX_HEADER);
	}

	if (standard_output)
		cout << tm << " " << hdr << msg << endl;
	else
		file << tm << " " << hdr << msg << endl;

	if (shutdown)
		Application.SignalEvent ();
}

// definimos el objeto
Logging Logs;
