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
// $Revision: 1.7 $
// $Date: 2004/06/19 23:58:08 $
//

#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <csignal>
#include <cerrno>

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

Process::Process (int idx, int fd)
{
	dead = true;
	args = NULL;
	nargs = 0;
	env = NULL;
	nenv = 0;

	name = NULL;
	description = NULL;
	type = NULL;
	command = NULL;
	environment = NULL;
	workdir = NULL;
	respawn = NULL;
	historylength = NULL;

	process_logs = NULL;
	index = idx;
	pty = 0;

	memset (current_command, 0, CONF_MAX_STRING + 1);
	memset (current_environment, 0, CONF_MAX_STRING + 1);
	memset (current_workdir, 0, CONF_MAX_STRING + 1);
	current_historylength = 0;

	num_start = 0;
	memset (&last_start, 0, sizeof (last_start));

	if (index == 0)
	{
		pty = fd;
	}
	else
	{
		// obtener las opciones e iniciarse
		GetOptions ();
		Start ();
	}
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

	// los punteros a estructuras de configuración
	// no deben borrarse ya que son eliminados al
	// borrarse la lista de opciones de la clase
	// Configuration.

	// Liberar process_logs
	if (process_logs != NULL)
		delete[] process_logs;
}

bool Process::operator== (Process &P)
{
	return (this->pty == P.pty);
}

void Process::GetOptions (void)
{
	char group[11];

	memset (group, 0, 11);
	snprintf (group, 11, "Process%d", index);

	// obtenemos las opciones que nos interesan
	name = Config.Get (Conf::String, group, "Name", group);
	description = Config.Get (Conf::String, group, "Description", "Default process");
	type = Config.Get (Conf::String, group, "Type", "unknown");
	command = Config.Get (Conf::String, group, "Command", "/bin/ls -al");
	environment = Config.Get (Conf::String, group, "Environment", "PATH=/usr/bin:/bin");
	workdir = Config.Get (Conf::String, group, "WorkingDirectory", ".");
	respawn = Config.Get (Conf::Boolean, group, "Respawn", 0);
	historylength = Config.Get (Conf::Numeric, group, "HistoryLength", 10240);

	// guardamos estos valores para su posterior comprobación
	strncpy (current_command, command->string, CONF_MAX_STRING);
	strncpy (current_environment, environment->string, CONF_MAX_STRING);
	strncpy (current_workdir, workdir->string, CONF_MAX_STRING);
	current_historylength = historylength->value;
}

int Process::CheckOptions (void)
{
	char group[11];

	memset (group, 0, 11);
	snprintf (group, 11, "Process%d", index);

	// primero revisamos que exista nuestro grupo
	if (!Config.Check (group, "Command"))
	{
		// no existe, entonces salimos
		Logs.Add (Log::Process | Log::Info,
			  "Process%d not exists on configuration",
			  index);
		return -1;
	}

	// comprobamos si cambió alguna opción esencial
	if (strncmp (command->string, current_command, CONF_MAX_STRING) ||
	    strncmp (environment->string, current_environment, CONF_MAX_STRING) ||
	    strncmp (workdir->string, current_workdir, CONF_MAX_STRING))
	{
		// guardamos estos valores para su posterior comprobación
		strncpy (current_command, command->string, CONF_MAX_STRING);
		strncpy (current_environment, environment->string, CONF_MAX_STRING);
		strncpy (current_workdir, workdir->string, CONF_MAX_STRING);
		current_historylength = historylength->value;

		Logs.Add (Log::Process | Log::Info,
			  "Process%d will be restarted",
			  index);

		Kill ();
		WaitExit ();
		Start ();
	}
	else if (historylength->value != current_historylength)
	{
		char *tmplogs = NULL;
		int delpos = 0;

		// hay que reasignar process_logs
		if (historylength->value < current_historylength)
		{
			if (process_logs != NULL)
			{
				delpos = current_historylength - historylength->value;
				while (process_logs[delpos] != '\n')
					++delpos;
				tmplogs = substring (process_logs, delpos,
						     strlen (process_logs));
			}
		}
		else
		{
			if (process_logs != NULL)
				tmplogs = substring (process_logs, 0,
						     strlen (process_logs));
		}

		// liberamos
		if (process_logs != NULL)
		{
			delete[] process_logs;
			process_logs = NULL;
		}

		Logs.Add (Log::Process | Log::Info,
			  "Reasignating %.1f KB for process logs",
			  (float) (historylength->value) / 1024.0);

		process_logs = new char [historylength->value + AUXLEN + 1];
		memset (process_logs, 0, historylength->value + AUXLEN + 1);

		// por último copiamos
		if (tmplogs != NULL)
		{
			strncpy (process_logs, tmplogs, strlen (tmplogs));
			delete[] tmplogs;
		}
	}
	else
		Logs.Add (Log::Process | Log::Debug,
			  "Process%d is unchanged",
			  index);

	return 0;
}

void Process::Start (void)
{
	int i;

	if (nargs > 0)
	{
		for (i = nargs - 1; i >= 0; --i)
		{
			delete[] args[i];
		}
		nargs = 0;
	}

	if (nenv > 0)
	{
		for (i = nenv - 1; i >= 0; --i)
		{
			delete[] env[i];
		}
		nenv = 0;
	}

	if (process_logs != NULL)
	{
		delete[] process_logs;
		process_logs = NULL;
	}

	if (historylength->value > 0)
	{
		Logs.Add (Log::Process | Log::Info,
			  "Asignating %.1f KB for process logs",
			  (float) (historylength->value) / 1024.0);

		process_logs = new char [historylength->value + AUXLEN + 1];
		memset (process_logs, 0, historylength->value + AUXLEN + 1);
	}
	else
		Logs.Add (Log::Process | Log::Critical,
			  "Can't assign %d bytes for process logs",
			  historylength->value);

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

	// tomamos el tiempo
	++num_start;
	time (&last_start);

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
			strncpy (msg, "Process will be restarted if dies", 50);
		else
			strncpy (msg, "Process will not be restarted if dies", 50);

		Logs.Add (Log::Process | Log::Info, msg);
		dead = false;

		// informar a los clientes que el proceso se inició
		Server.SendToAllClients (index, Session::ProcessStart, msg);
		Server.SendToAllClients (index, Session::Authorized);

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

	if (proclen + datalen > historylength->value + AUXLEN)
	{
		// rotar los logs en al menos AUXLEN caracteres
		// buscar a partir de AUXLEN un \n y luego borrar desde ahí
		// hasta el principio.
		while (process_logs[delpos] != '\n')
			++delpos;

		// copiamos el trozo reducido
		reduced = substring (process_logs, delpos + 1, proclen - 1);

		// borramos todos los logs
		memset (process_logs, 0, historylength->value + AUXLEN + 1);

		// copiamos el trozo en process_logs
		strncpy (process_logs, reduced, strlen (reduced));

		delete[] reduced;
	}

	// finalmente agregamos data al buffer process_logs
	strncat (process_logs, data, datalen);
}

char *Process::GetProcessData (int *len)
{
	char *buf = new char [historylength->value + AUXLEN + 1];

	memset (buf, 0, historylength->value + AUXLEN + 1);
	strncpy (buf, process_logs, historylength->value + AUXLEN);

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
				  "Process %d died", pid);
		else
			Logs.Add (Log::Process | Log::Warning,
				  "Failed to read from process %d", pid);

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

void Process::Read (void)
{
	int len;
	char buf[PCKT_MAXDATA + 1];

	memset (buf, 0, PCKT_MAXDATA + 1);
	len = ReadData (buf, PCKT_MAXDATA);

	if (len > 0)
	{
		Logs.Add (Log::Process | Log::Debug,
			  "Read %d bytes from process %d",
			  len, pid);

		// los datos leídos deben ponerse en el log del proceso
		AddProcessData (buf);

		// enviamos a todos los clientes
		Server.SendToAllClients (index, Session::ConsoleOutput, buf);
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
	int status, retval;
	char msg[51];
	time_t last_death, time_diff;

	if (dead)
		return;

	dead = true;
	retval = waitpid (pid, &status, WNOHANG);

	if (retval == 0)
	{
		// el proceso no ha terminado, asesinando
		Logs.Add (Log::Process | Log::Warning,
			  "Process %d couldn't be terminated: %s",
			  pid, strerror (errno));
		errno = 0;
		SendSignal (SIGKILL);
		waitpid (pid, &status, WNOHANG);
	}

	// tomamos el tiempo para confrontarlo con su tiempo de inicio
	time (&last_death);

	snprintf (msg, 51, "Process %d has died with %d", pid, status);
	Server.SendToAllClients (index, Session::ProcessExits, msg);
	Logs.Add (Log::Process | Log::Info, msg);

	time_diff = last_death - last_start;

	// proceso murió muy rápido (dentro de 2 segundos de haberse iniciado)
	if (time_diff <= 2)
	{
		respawn->value = 0;
		strncpy (msg, "Bad process will not be restarted", 50);
		Server.SendToAllClients (index, Error::BadProcess);
		Logs.Add (Log::Process | Log::Critical, msg);
	}

	// proceso no se pudo iniciar
	//if (status == 0xFF00)
	if (WEXITSTATUS(status))
	{
		respawn->value = 0;
		strncpy (msg, "Bad process will not be restarted", 50);
		Server.SendToAllClients (index, Error::BadProcess);
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
		  "Sending signal %d to process %d",
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

char* Process::GetInfo (void)
{
	char *process_info = NULL, histlenbuf[81], laststartbuf[81], numstartbuf[81];
	int info_length;

	snprintf (histlenbuf, 81, "%d", historylength->value);
	snprintf (numstartbuf, 81, "%d", num_start);
	strncpy (laststartbuf, ctime (&last_start), 80);

	info_length = strlen ("Name") + strlen (name->string) + 2 +
		      strlen ("Description") + strlen (description->string) + 2 +
		      strlen ("Type") + strlen (type->string) + 2 +
		      strlen ("Command") + strlen (command->string) + 2 +
		      strlen ("Environment") + strlen (environment->string) + 2 +
		      strlen ("WorkingDirectory") + strlen (workdir->string) + 2 +
		      strlen ("Respawn") + 3 + 2 +
		      strlen ("HistoryLength") + strlen (histlenbuf) + 2 +
		      strlen ("Starts") + strlen (numstartbuf) + 2 +
		      strlen ("LastStart") + strlen (laststartbuf) + 2;

	process_info = new char [info_length + 1];
	memset (process_info, 0, info_length + 1);

	strncat (process_info, "Name=", 5);
	strncat (process_info, name->string, strlen (name->string));
	strncat (process_info, "\n", 1);

	strncat (process_info, "Description=", 12);
	strncat (process_info, description->string, strlen (description->string));
	strncat (process_info, "\n", 1);

	strncat (process_info, "Type=", 5);
	strncat (process_info, type->string, strlen (type->string));
	strncat (process_info, "\n", 1);

	strncat (process_info, "Command=", 8);
	strncat (process_info, command->string, strlen (command->string));
	strncat (process_info, "\n", 1);

	strncat (process_info, "Environment=", 12);
	strncat (process_info, environment->string, strlen (environment->string));
	strncat (process_info, "\n", 1);

	strncat (process_info, "WorkingDirectory=", 17);
	strncat (process_info, workdir->string, strlen (workdir->string));
	strncat (process_info, "\n", 1);

	strncat (process_info, "Respawn=", 8);
	if (respawn->value)
		strncat (process_info, "yes", 3);
	else
		strncat (process_info, "no", 2);
	strncat (process_info, "\n", 1);

	strncat (process_info, "HistoryLength=", 14);
	strncat (process_info, histlenbuf, strlen (histlenbuf));
	strncat (process_info, "\n", 1);

	strncat (process_info, "Starts=", 7);
	strncat (process_info, numstartbuf, strlen (numstartbuf));
	strncat (process_info, "\n", 1);

	strncat (process_info, "LastStart=", 10);
	strncat (process_info, laststartbuf, strlen (laststartbuf));
	strncat (process_info, "\n", 1);

	// no olvidar liberar la memoria usada
	return process_info;
}
