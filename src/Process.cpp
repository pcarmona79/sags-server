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
// $Source: /home/pablo/Desarrollo/sags-cvs/server/src/Process.cpp,v $
// $Revision: 1.1 $
// $Date: 2004/04/13 22:00:19 $
//

#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <csignal>

#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <pty.h>

#include "Process.hpp"
#include "Main.hpp"
#include "Log.hpp"
#include "Utils.hpp"
#include "Network.hpp"

#define AUXLEN 2*PCKT_MAXDATA

using namespace std;

Process::Process ()
{
	dead = true;
	args = NULL;
	nargs = 0;
	env = NULL;
	nenv = 0;

	command = NULL;
	environment = NULL;
	workdir = NULL;
	respawn = NULL;
	lines = NULL;
	process_logs = NULL;
}

Process::~Process ()
{
	int i;

	if (nargs > 0)
	{
		for (i = nargs - 1; i >= 0; --i)
		{
			delete[] args[i];
		}
	}

	if (nenv > 0)
	{
		for (i = nenv - 1; i >= 0; --i)
		{
			delete[] env[i];
		}
	}

	delete command;
	delete environment;
	delete workdir;
	delete respawn;
	delete lines;

	// Liberar process_logs
	delete[] process_logs;
}

void Process::AddOptions (void)
{
	command = Config.Add (Conf::String, "Process", "Command", "/bin/ls -al");
	environment = Config.Add (Conf::String, "Process", "Environment", "PATH=/usr/bin:/bin");
	workdir = Config.Add (Conf::String, "Process", "WorkingDirectory", ".");
	respawn = Config.Add (Conf::Boolean, "Process", "Respawn", 0);
	lines = Config.Add (Conf::Numeric, "Process", "HistoryLines", 25);
}

void Process::Start (void)
{
	// Asignamos 256 * "HistoryLines".
	// Por defecto se asignan 25 KB (100 líneas). Con 1000 líneas se asignarán 250 KB.
	if (lines->value > 0)
	{
		Logs.Add (Log::Process | Log::Info,
			  "Asignating %.1f KB for process logs (%d lines)",
			  (float)(lines->value * (LOG_MAX_STRING + 1) / 1024),
			  lines->value);

		process_logs = new char [lines->value * (LOG_MAX_STRING + 1) + AUXLEN + 1];
		memset (process_logs, 0, lines->value * (LOG_MAX_STRING + 1) + AUXLEN + 1);
	}
	else
		Logs.Add (Log::Process | Log::Critical,
			  "Can't assign %d lines for process logs",
			  lines->value);

	// lanzamos el proceso
	Launch ();
}

int Process::Launch (void)
{
	char msg[51];

	if (command->string[0])
		args = strsplit (command->string, ' ', -1, &nargs);
	else
		return -1;

	if (environment->string[0])
		env = strsplit (environment->string, ' ', -1, &nenv);

	// lanzamos el proceso en un seudo-terminal en segundo plano
	pid = forkpty (&pty, NULL, NULL, NULL);
	if (pid < 0)
	{
		perror ("forkpty");
		return -1;
	}
	else if (pid == 0)
	{
		// el nuevo proceso creado no debe tener señales bloqueadas
		sigset_t sigmask;
		sigfillset (&sigmask);
		sigprocmask (SIG_UNBLOCK, &sigmask, NULL);

		// cambiamos al directorio de trabajo
		if (chdir (workdir->string) < 0)
		{
			perror ("chdir");
			exit (-1);
		}

		// lanzamos el programa
		execve (args[0], args, env);
		perror ("execve");
		exit (-1);
	}
	else
	{
		Logs.Add (Log::Process | Log::Info,
			  "Launching new process %d with \"%s\" on directory \"%s\"",
			  pid, command->string, workdir->string);

		if (respawn->value)
			strcpy (msg, "Process will be restarted if dies");
		else
			strcpy (msg, "Process will not be restarted if dies");

		Logs.Add (Log::Process | Log::Info, msg);
		dead = false;

		// informar a los clientes que el proceso se inició
		Server.SendToAllClients (Pckt::SessionProcessStart, msg);

		// registrar con la aplicación
		Application.Add (Owner::Process, pty);
	}

	return 0;
}

void Process::AddProcessData (const char *data)
{
	int datalen = strlen (data);
	int proclen = strlen (process_logs);
	int delpos = AUXLEN;
	char *reduced = NULL;

	// FIXME: estamos suponiendo que no agregamos
	// más de AUXLEN bytes de datos.

	if (proclen + datalen > lines->value * (LOG_MAX_STRING + 1) + AUXLEN)
	{
		// rotar los logs en al menos AUXLEN caracteres
		// buscar a partir de AUXLEN un \n y luego borrar desde ahí
		// hasta el principio.
		while (process_logs[delpos] != '\n')
			++delpos;

		// copiamos el trozo reducido
		reduced = substring (process_logs, delpos + 1, proclen - 1);

		// borramos todos los logs
		memset (process_logs, 0, lines->value * (LOG_MAX_STRING + 1) + AUXLEN + 1);

		// copiamos el trozo en process_logs
		strncpy (process_logs, reduced, strlen (reduced));

		delete[] reduced;
	}

	// finalmente agregamos data al buffer process_logs
	strncat (process_logs, data, datalen);
}

char *Process::GetProcessData (int *len)
{
	char *buf = new char [lines->value * (LOG_MAX_STRING + 1) + AUXLEN + 1];

	memset (buf, 0, lines->value * (LOG_MAX_STRING + 1) + AUXLEN + 1);
	strncpy (buf, process_logs, lines->value * (LOG_MAX_STRING + 1) + AUXLEN);

	if (len != NULL)
		*len = strlen (buf);

	return buf;
}

int Process::ReadData (char *buffer, int length)
{
	int retval;

	retval = read (pty, buffer, length);

	// Si se leen 0 bytes significa que el descriptor fue cerrado
	if (retval <= 0)
	{
		if (retval == 0)
			Logs.Add (Log::Process | Log::Warning,
				  "Process died");
		else
			Logs.Add (Log::Process | Log::Warning,
				  "Failed to read from process");

		// descriptor cerrado debe ser removido
		Application.Remove (Owner::Process, pty);
		WaitExit ();

		// proceso hijo a muerto por lo que debería ser reiniciado
		Restart ();
	}

	return retval;
}

int Process::WriteData (char *buffer)
{
	int len;

	if (dead)
		return -1;

	if (buffer != NULL)
		len = write (pty, buffer, strlen (buffer));
	else
		return -1;

	return len;
}

void Process::Read (char *buf, int buflen)
{
	int len;

	memset (buf, 0, buflen + 1);
	len = ReadData (buf, buflen);

	if (len > 0)
	{
		Logs.Add (Log::Process | Log::Debug,
			  "Read %d bytes from %d",
			  len, pid);

		// los datos leídos deben ponerse en el log del proceso
		AddProcessData (buf);
	}
}

int Process::Write (char *buffer)
{
	int len;

	len = WriteData (buffer);
	if (len < 0)
		return len;

	Logs.Add (Log::Process | Log::Debug,
		  "Writed %d bytes to process %d",
		  len, pid);

	return len;
}

void Process::WaitExit (void)
{
	int status;
	char msg[51];

	if (dead)
		return;

	dead = true;

	waitpid (pid, &status, WNOHANG);

	snprintf (msg, 50, "Process %d has died with %d", pid, status);
	Server.SendToAllClients (Pckt::SessionProcessExits, msg);
	Logs.Add (Log::Process | Log::Notice, msg);

	// proceso no se pudo iniciar
	if (status == 0xFF00)
	{
		respawn->value = 0;
		strncpy (msg, "Bad process will not be restarted", 50);
		Server.SendToAllClients (Pckt::SessionProcessExits, msg);
		Logs.Add (Log::Process | Log::Critical, msg);
	}
}

int Process::Kill (void)
{
	if (dead)
		return -1;

	Logs.Add (Log::Process | Log::Notice,
		  "Terminating process %d", pid);

	return kill (pid, SIGTERM);
}

int Process::SendSignal (int sig)
{
	if (dead)
		return -1;

	Logs.Add (Log::Process | Log::Notice,
		  "Sending signal nº %d to process %d",
		  sig, pid);

	return kill (pid, sig);
}

void Process::Restart (void)
{
	// reiniciamos sólo si por configuración
	// se pide reiniciar
	if (respawn->value)
	{
		Logs.Add (Log::Process | Log::Notice,
			  "Restarting process");
		Launch ();
	}
}

// definimos el objeto
Process Child;
