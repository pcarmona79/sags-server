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
// $Source: /home/pablo/Desarrollo/sags-cvs/server/src/Config.cpp,v $
// $Revision: 1.4 $
// $Date: 2004/06/14 03:12:10 $
//

#include <iostream>
#include <fstream>
#include <cstring>
#include <cstdlib>

#include "Config.hpp"
#include "Log.hpp"
#include "Utils.hpp"

using namespace std;

Configuration::Configuration ()
{
	ConfigFile = NULL;
	FileName = NULL;
}

Configuration::~Configuration ()
{
	
}

void Configuration::GetOptionsFromFile (ifstream *file, const char *filename)
{
	char line[CONF_MAX_LINE + 1];
	char *group = NULL, *name = NULL, *data = NULL;
	int i, value;
	unsigned int n;

	if (filename != NULL)
		FileName = filename;

	if (file != NULL)
	{
		// El archivo ya viene abierto
		ConfigFile = file;
	}
	else
	{
		// reutilizar el archivo
		if (ConfigFile != NULL)
			ConfigFile->open (FileName);
	}

	for (i = 1; !ConfigFile->eof(); ++i)
	{
		ConfigFile->getline (line, CONF_MAX_LINE);
		//Logs.Add (Log::Config | Log::Debug, "%2d %s", i, line);

		// comprobamos que la línea
		// sea de configuración

		if (line[0] == '#')
		{
			// un comentario descartado
			//Logs.Add (Log::Config | Log::Debug,
			//	  "Comment at line %d discarted", i);
			continue;
		}

		if (line[0] == '\0')
		{
			// una línea en blanco descartada
			//Logs.Add (Log::Config | Log::Debug,
			//	  "Blank line %d discarted", i);
			continue;
		}

		// buscamos un grupo
		if (line[0] == '[' && line[strlen(line) - 1] == ']')
		{
			if (group != NULL)
			{
				delete[] group;
				group = NULL;
			}

			group = substring (line, 1, strlen (line) - 2);

			if (group == NULL)
			{
				// no es una línea de configuración
				Logs.Add (Log::Config | Log::Notice,
					  "No group! Line %d isn't a valid configure line", i);
			}
			else
				Logs.Add (Log::Config | Log::Debug,
					  "New group [%s]", group);

			continue;
		}

		for (n = 0; n <= strlen (line) - 1; ++n)
		{
			if (line[n] == '=')
				break;
		}

		if (line[n] == '\0')
		{
			// no es una línea de configuración
			Logs.Add (Log::Config | Log::Notice,
				  "Line %d isn't a valid configure line", i);
			continue;
		}

		name = substring (line, 0, n - 1);
		data = substring (line, n + 1, strlen (line) - 1);

		if (name == NULL || data == NULL)
		{
			// no es una línea de configuración
			Logs.Add (Log::Config | Log::Notice,
				  "Line %d isn't a valid configure line", i);
			continue;
		}

		Logs.Add (Log::Config | Log::Debug, "Option [%s]%s=%s",
			  group, name, data);

		// agregamos a la lista luego de
		// comprobar el tipo de dato

		if (data[0] == '\"')
		{
			// si es una cadena de texto sacamos las comillas
			++data;
			if (data[strlen (data) - 1] == '\"')
				data[strlen (data) - 1] = '\0';
			Set (Conf::String, group, name, data);
			--data;
		}
		else if (!strcasecmp (data, "yes") ||
			 !strcasecmp (data, "true"))
		{
			// variable booleana con valor verdadero
			Set (Conf::Boolean, group, name, 1);
		}
		else if (!strcasecmp (data, "no") ||
			 !strcasecmp (data, "false"))
		{
			// variable booleana con valor falso
			Set (Conf::Boolean, group, name, 0);
		}
		else
		{
			// es un valor numérico
			value = (int) strtol (data, (char **)NULL, 10);
			if (errno == EINVAL && value == 0)
			{
				// número no válido, se descarta la línea
				Logs.Add (Log::Config | Log::Notice,
					  "Number not valid at line %d", i);
				continue;
			}
			Set (Conf::Numeric, group, name, value);
		}

		// liberamos el espacio asignado
		if (name != NULL)
			delete[] name;
		if (data != NULL)
			delete[] data;
	}

	// liberamos el espacio asignado al grupo
	if (group != NULL)
		delete[] group;

	// cerramos el archivo de configuración
	ConfigFile->close ();

	// por último reiniciamos Logs para que
	// tome los nuevos valores
	Logs.Start ();

	Logs.Add (Log::Config | Log::Info, "Processed file \"%s\"", FileName);
}

struct option *Configuration::Add (Conf::OpType type, const char *group, const char *name, int val)
{
	struct option *opt = new struct option (group, name, type);

	//cout << "Adding ('" << opt->group << "', '" << opt->name << "') = " << val << endl;

	// val debe ser convertido según type
	switch (opt->type)
	{
		case Conf::Boolean:
			opt->value = (val) ? 1 : 0;
			opt->string[0] = '\0';
			list << opt;
			break;

		case Conf::Numeric:
			opt->value = val;
			opt->string[0] = '\0';
			list << opt;
			break;

		default:
			delete opt;
			return NULL;
	}

	return opt;
}

struct option *Configuration::Add (Conf::OpType type, const char *group, const char *name, const char *val)
{
	struct option *opt = new struct option (group, name, type);

	//cout << "Adding ('" << opt->group << "', '" << opt->name << "') = " << val << endl;

	// val es char* y ya opt es Conf::String, pero comprobamos igual
	switch (opt->type)
	{
		case Conf::String:
			strncpy (opt->string, val, CONF_MAX_STRING);
			opt->value = 0;
			list << opt;
			break;
		default:
			delete opt;
			return NULL;
	}

	return opt;
}

struct option *Configuration::Get (const char *group, const char *name)
{
	struct option *opt, *found, searched (group, name);

	opt = list.Find (searched);

	if (opt != NULL)
	{
		found = new struct option (opt->group, opt->name, opt->type);

		// el string se asigna si no es nulo, si no se asigna value
		if (opt->string[0] != '\0')
		{
			strncpy (found->string, opt->string, CONF_MAX_STRING);
			found->value = 0;
		}
		else
		{
			found->value = opt->value;
			found->string[0] = '\0';
		}

		// retornamos objeto encontrado
		return found;
	}

	// si llegamos aquí es que es una opción desconocida
	Logs.Add (Log::Config | Log::Warning,
		  "Unknown option [%s]%s",
		  group, name);

	return NULL;
}

void Configuration::Set (Conf::OpType type, const char *group, const char *name, int val)
{
	struct option *opt, searched (group, name);

	opt = list.Find (searched);

	if (opt != NULL)
	{
		// val debe ser convertido según type
		switch (type)
		{
			case Conf::Boolean:
				opt->value = (val) ? 1 : 0;
				opt->string[0] = '\0';
				//Logs.Add (Log::Config | Log::Debug,
				//	  "Changing Boolean %s.%s to %d", group, name, val);
				break;

			case Conf::Numeric:
				opt->value = val;
				opt->string[0] = '\0';
				//Logs.Add (Log::Config | Log::Debug,
				//	  "Changing Numeric %s.%s to %d", group, name, val);
				break;
			default:
				;
		}
	}
	else
		// si no existe la opción, la agregamos
		Add (type, group, name, val);
}

void Configuration::Set (Conf::OpType type, const char *group, const char *name, const char *val)
{
	struct option *opt, searched (group, name);

	opt = list.Find (searched);

	if (opt != NULL)
	{
		// opt ya es de tipo Conf::String, pero por si acaso comprobaremos
		switch (opt->type)
		{
			case Conf::String:
				strncpy (opt->string, val, CONF_MAX_STRING);
				opt->value = 0;
				//Logs.Add (Log::Config | Log::Info,
				//	  "Changing String %s.%s to %s", group, name, val);
				break;
			default:
				;
		}
	}
	else
		// si no existe la opción, la agregamos
		Add (type, group, name, val);
}

// definimos el objeto
Configuration Config;
