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
// $Source: /home/pablo/Desarrollo/sags-cvs/server/src/Program.cpp,v $
// $Revision: 1.5 $
// $Date: 2004/06/16 00:52:49 $
//

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <sys/types.h>
#include <unistd.h>
#include <cstdio>
#include <cstdlib>
#include <cerrno>
#include <sys/stat.h>

#include "Config.hpp"
#include "Log.hpp"
#include "ProcTree.hpp"
#include "Network.hpp"
#include "Main.hpp"

int main (int argc, char **argv)
{
	char *config_file = NULL, *path = NULL, *absolute_config_file = NULL;
	ifstream configuration;
	bool debug_mode = false;
	int i, retval;
	pid_t pid;

	// revisamos la línea de comandos
	if (argc >= 2)
	{
		for (i = 1; i <= argc - 1; ++i)
		{
			if (argv[i][0] == '-')
			{
				switch (argv[i][1])
				{
					case 'D':
						debug_mode = true;
						break;
					default:
						Application.PrintUsage ();
				}
			}
			else
			{
				if (config_file == NULL)
				{
					config_file = new char [strlen (argv[i]) + 1];
					strncpy (config_file, argv[i], strlen (argv[i]));
				}
				else
					Application.PrintUsage ();
			}
		}
	}

	// si no se especificó un archivo de configuración
	// usamos /etc/sags/sags.conf
	if (config_file == NULL)
	{
		int len = strlen (PACKAGE_SYSCONF_DIR "/" PACKAGE ".conf");
		config_file = new char [len + 1];
		strncpy (config_file, PACKAGE_SYSCONF_DIR "/" PACKAGE ".conf", len);
	}

	// si debug_mode es false hacemos fork
	if (!debug_mode)
	{
		pid = fork ();
		if (pid < 0)
		{
			perror ("fork");
			exit (EXIT_FAILURE);
		}
		if (pid > 0)
			exit (EXIT_SUCCESS);

		// ahora estamos en el hijo

		if (setsid () < 0)
		{
			perror ("setsid");
			exit (EXIT_FAILURE);
		}

		// config_file debe tener la ruta absoluta
		// TODO: habría que borrar los .. y . de la ruta
		if (config_file[0] != '/')
		{
			path = getcwd (NULL, 0);
			absolute_config_file = new char [strlen (path) + strlen (config_file) + 2];
			strncpy (absolute_config_file, path, strlen (path));
			strncat (absolute_config_file, "/", 1);
			strncat (absolute_config_file, config_file, strlen (config_file));
		}

		if (chdir ("/") < 0)
		{
			perror ("chdir");
			exit (EXIT_FAILURE);
		}

		// abrimos el archivo de configuración
		if (absolute_config_file != NULL)
			configuration.open (absolute_config_file);
		else
			configuration.open (config_file);

		if (!configuration.is_open ())
		{
			if (absolute_config_file != NULL)
				fprintf (stderr, "open: %s: %s\n",
					 absolute_config_file, strerror (errno));
			else
				fprintf (stderr, "open: %s: %s\n",
					 config_file, strerror (errno));
			exit (EXIT_FAILURE);
		}

		// establecemos el modo de archivo y
		// cerramos descriptores innecesarios
		umask (0);
		close (STDIN_FILENO);
		close (STDOUT_FILENO);
		close (STDERR_FILENO);
	}
	else
	{
		// abrimos el archivo de configuración
		configuration.open (config_file);
		if (!configuration.is_open ())
		{
			fprintf (stderr, "open: %s: %s\n", config_file, strerror (errno));
			exit (EXIT_FAILURE);
		}
	}


	// el programa principal comienza aquí
	Application.Init (debug_mode);

	Server.AddOptions ();
	Logs.AddOptions ();
	Application.AddOptions ();

	Logs.Start ();

	if (absolute_config_file != NULL)
		Config.GetOptionsFromFile (&configuration, absolute_config_file);
	else
		Config.GetOptionsFromFile (&configuration, config_file);

	Logs.Add (Log::Info, "SAGS Server version %s", VERSION);

	Application.LoadUsers ();
	ProcMaster.Start ();
	Server.Start ();

	retval = Application.Run ();

	// si estamos aquí es que estamos saliendo
	Logs.Add (Log::Notice, "Exiting with %d", retval);

	if (config_file != NULL)
		delete[] config_file;
	if (path != NULL)
		delete[] path;
	if (absolute_config_file != NULL)
		delete[] absolute_config_file;

	return retval;
}
