//
// sags-passwd - Manage password files of SAGS Server.
// Copyright (C) 2005 Pablo Carmona Amigo
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Library General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
//
// $Source: /home/pablo/Desarrollo/sags-cvs/server/tools/sags-passwd.cpp,v $
// $Revision: 1.1 $
// $Date: 2005/01/17 23:13:02 $
//

#include <iostream>
#include <iomanip>
#include <fstream>
#include <cstring>
#include <cstdlib>
#include <termios.h>
#include "../src/List.hpp"

#define MAX_NAME 80

using namespace std;

//-------------------------------------------------------------------//
//  Utilidades
//-------------------------------------------------------------------//

char **strsplit (const char *str, char delim, int elem, int *count)
{
	int n = 0, i, len;
	const char *p = str, *start;
	char **vect;

	while (*p != '\0')
	{
		if (*p == delim)
			++n;
		++p;
	}
	++n;

	if (elem > n || elem < 0)
		elem = n;

	vect = new char* [elem + 1];
	p = str;

	for (i = 0; i <= elem - 1; ++i)
	{
		start = p;

		for (len = 0; *p != delim; ++len, ++p)
			if (*p == '\0')
				break;

		while (*p == delim)
			++p;

		vect[i] = new char[len + 1];
		memset (vect[i], 0, len + 1);

		if (len > 0)
			strncpy (vect[i], start, len);
		else
			vect[i][0] = '\0';

	}

	vect[elem] = NULL;
	if (count != NULL)
		*count = elem;

	return vect;
}

char *encode_password (const char *password)
{
	char *encoded;
	int i;

	encoded = new char [strlen (password) + 1];
	memset (encoded, 0, strlen (password) + 1);

	for (i = 0; i <= (int) strlen (password) - 1; ++i)
		encoded[i] = password[i] ^ 0xAA;

	return encoded;
}

//-------------------------------------------------------------------//
//  UsersFile: interfaz al archivo de usuarios
//-------------------------------------------------------------------//

struct user
{
	char name[MAX_NAME + 1];
	char pwd[MAX_NAME + 1];
	char procs[MAX_NAME + 1];

	user (const char *n = NULL, const char *p = NULL, const char *pc = NULL)
	{
		memset (name, 0, MAX_NAME + 1);
		memset (pwd, 0, MAX_NAME + 1);
		memset (procs, 0, MAX_NAME + 1);
		if (n != NULL)
			strncpy (name, n, MAX_NAME);
		if (p != NULL)
			strncpy (pwd, p, MAX_NAME);
		if (pc != NULL)
			strncpy (procs, pc, MAX_NAME);
	}

	struct user& operator= (const struct user &usr)
	{	
		strncpy (name, usr.name, MAX_NAME);
		strncpy (pwd, usr.pwd, MAX_NAME);
		strncpy (procs, usr.procs, MAX_NAME);
		return *this;
	}

	bool operator== (const struct user &usr)
	{
		if (!strncmp (name, usr.name, MAX_NAME))
			return true;
		return false;
	}

};

class UsersFile
{
private:
	List<struct user> SAGS_Users;
	char filename[MAX_NAME + 1];
	
public:
	UsersFile ();
	~UsersFile ();

	void SetFile (const char *newname);
	int Read (void);
	int Write (void);
	int GetUserProcesses (const char *username, char *procs);
	int CheckUser (const char *username);
	int DeleteUser (const char *username);
	int UpdateUser (const char *current_name, const char *username,
			const char *password, const char *procs);
	void ListUsers (const char *which);
};

UsersFile::UsersFile ()
{
	memset (filename, 0, MAX_NAME + 1);
}

UsersFile::~UsersFile ()
{
	
}

void UsersFile::SetFile (const char *newname)
{
	if (newname != NULL)
		strncpy (filename, newname, MAX_NAME);
}

int UsersFile::Read (void)
{
	ifstream file (filename); // filename no deberia estar vacio
	char line[3*MAX_NAME + 1], **ln, *pwd_decoded;
	int i, j, three;

	if (!file.is_open ())
	{
		cerr << "Error: Failed to open the users file \"" <<
			filename << "\"" << endl;
		return -1;
	}

	for (i = 1; !file.eof(); ++i)
	{
		file.getline (line, 3*MAX_NAME);

		if (line[0] == '\0')
			continue;

		ln = strsplit (line, ':', -1, &three);
		if (three != 3)
		{
			cerr << "Skipping malformed line " << i << endl;
			continue;
		}

		// decodificamos la password y guardamos
		pwd_decoded = encode_password (ln[1]);
		UpdateUser (ln[0], ln[0], pwd_decoded, ln[2]);

		// ln debe ser liberado
		if (three > 0)
		{
			for (j = three - 1; j >= 0; --j)
				delete[] ln[j];
		}
		delete[] pwd_decoded;
	}

	file.close ();
	cout << "Loaded " << SAGS_Users.GetCount() <<
		" users from file " << filename << endl;

	return SAGS_Users.GetCount();
}

int UsersFile::Write (void)
{
	ofstream file (filename); // filename no deberia estar vacio
	int i;
	char *pwd_encoded;

	if (!file.is_open ())
	{
		cerr << "Error: Failed to open the users file \"" <<
			filename << "\"" << endl;
		return -1;
	}

	for (i = 0; i <= (int) SAGS_Users.GetCount () - 1; ++i)
	{
		pwd_encoded = encode_password (SAGS_Users[i]->pwd);
		file << SAGS_Users[i]->name << ":" <<
			pwd_encoded << ":" <<
			SAGS_Users[i]->procs << endl;
		delete[] pwd_encoded;
	}

	file.close ();
	cout << "Saved " << SAGS_Users.GetCount() <<
		" users to file " << filename << endl;

	return SAGS_Users.GetCount();
}

int UsersFile::GetUserProcesses (const char *username, char *procs)
{
	struct user searched (username);
	struct user *finded = SAGS_Users.Find (searched);

	if (finded == NULL)
		return -1;

	strncpy (procs, finded->procs, MAX_NAME);
	return 0;
}

int UsersFile::CheckUser (const char *username)
{
	struct user searched (username);
	struct user *finded = SAGS_Users.Find (searched);

	if (finded == NULL)
		return 0;
	return 1;
}

int UsersFile::DeleteUser (const char *username)
{
	struct user searched (username);

	return SAGS_Users.Remove (searched);
}

int UsersFile::UpdateUser (const char *current_name, const char *username,
			   const char *password, const char *procs)
{
	struct user searched (current_name);
	struct user *finded = SAGS_Users.Find (searched);

	// buscar un usuario con username y actualizar los datos
	// si no existe el usuario, crearlo

	if (finded == NULL)
	{
		// creamos el usuario
		finded = new struct user (username, password, procs);
		SAGS_Users << finded;

		return 1;
	}

	// actualizamos los datos solo si no estan vacios
	if (strlen (username) > 0)
		strncpy (finded->name, username, MAX_NAME);
	if (strlen (password) > 0)
		strncpy (finded->pwd, password, MAX_NAME);
	if (strlen (procs) > 0)
		strncpy (finded->procs, procs, MAX_NAME);

	return 0;
}

void UsersFile::ListUsers (const char *which)
{
	struct user searched (which);
	struct user *finded = SAGS_Users.Find (searched);

	cout << "+--------------------+--------------------+" << endl <<
		"|Username            |Allowed processes   |" << endl <<
		"+--------------------+--------------------+" << endl;

	if (strlen (which) > 0)
	{
		if (finded == NULL)
			cerr << "Error: User \"" << which << "\" don't exists." << endl;
		else
			cout << "|" << setw (20) << left << finded->name <<
				"|" << setw (20) << left << finded->procs <<
				"|" << endl;
	}
	else
	{
		// desplegamos la lista
		for (int i = 0; i <= (int) SAGS_Users.GetCount () - 1; ++i)
			cout << "|" << setw (20) << left << SAGS_Users[i]->name <<
				"|" << setw (20) << left << SAGS_Users[i]->procs <<
				"|" << endl;
	}

	cout << "+--------------------+--------------------+" << endl;
}

//-------------------------------------------------------------------//
//  UsersAdmin: Administrador de las tareas a hacer con el archivo
//              de usuarios de SAGS
//-------------------------------------------------------------------//

enum { MODE_CREATE = 1, MODE_EDIT, MODE_DELETE, MODE_LIST };
enum { ASK_NAME = 1, ASK_PWD = 2, ASK_PROCS = 4, ASK_NEWNAME = 8 };

class UsersAdmin
{
private:
	int mode;
	int ask;

	char input_name[MAX_NAME + 1];
	char input_pwd[MAX_NAME + 1];
	char input_procs[MAX_NAME + 1];
	char input_newname[MAX_NAME + 1];

	UsersFile SAGS_UsersFile;

	void PrintUsage (void);
	void PrintHelp (void);

public:
	UsersAdmin ();
	~UsersAdmin ();

	void Init (int argc, char **argv);
	void AskUser (void);
	void Run (void);
};

UsersAdmin::UsersAdmin ()
{
	mode = ask = 0;
	memset (input_name, 0, MAX_NAME + 1);
	memset (input_pwd, 0, MAX_NAME + 1);
	memset (input_procs, 0, MAX_NAME + 1);
	memset (input_newname, 0, MAX_NAME + 1);
}

UsersAdmin::~UsersAdmin ()
{
	
}

void UsersAdmin::PrintUsage (void)
{
	cerr << "Usage: sags-passwd [-]<c|l|d|nwp|h> users_file [user_name]" << endl;
	cerr << "Use \"sags-passwd -h\" to get more help." << endl;
}

void UsersAdmin::PrintHelp (void)
{
	cerr << "Usage: sags-passwd [-]<c|l|d|nwp|h> users_file [user_name]" << endl;
	cerr << "Help!" << endl;
}

void UsersAdmin::Init (int argc, char **argv)
{
	unsigned int i;
	bool getout = false;

	cout << "SAGS Users File Administrator" << endl;

	// buscamos un -h para mostrar la ayuda
	if (argc == 2)
	{
		for (i = 0; i <= strlen (argv[1]) - 1; ++i)
		{
			if (argv[1][i] == 'h')
			{
				PrintHelp ();
				exit (EXIT_SUCCESS);
			}
		}
	}

	// necesitamos de 2 a 3 parametros para funcionar
	if (argc < 3 || argc > 4)
	{
		PrintUsage ();
		exit (EXIT_FAILURE);
	}

	// el primer parametro son las opciones
	// analizamos letra por letra
	for (i = 0; i <= strlen (argv[1]) - 1; ++i)
	{
		switch (argv[1][i])
		{
		case '-':
			continue;
		case 'l':
			mode = MODE_LIST;
			ask = 0;
			getout = true;
			break;
		case 'c':
			mode = MODE_CREATE;
			ask = ASK_NAME | ASK_PWD | ASK_PROCS;
			getout = true;
			break;
		case 'd':
			mode = MODE_DELETE;
			ask = ASK_NAME;
			getout = true;
			break;
		case 'n':
			mode = MODE_EDIT;
			ask |= ASK_NEWNAME;
			break;
		case 'w':
			mode = MODE_EDIT;
			ask |= ASK_PWD;
			break;
		case 'p':
			mode = MODE_EDIT;
			ask |= ASK_PROCS;
		}
		if (getout)
			break;
	}

	// si no se fijo ningun modo, significa
	// que estaban malas las opciones
	if (mode == 0)
	{
		PrintUsage ();
		exit (EXIT_FAILURE);
	}

	// el segundo parametro es el archivo de usuarios
	SAGS_UsersFile.SetFile (argv[2]);

	// cargamos el archivo
	if (SAGS_UsersFile.Read () < 0)
		exit (EXIT_FAILURE);

	// el tercer parametro es opcional y corresponde
	// a un nombre de usuario
	if (argc == 4)
	{
		strncpy (input_name, argv[3], MAX_NAME);
		ask &= ~ASK_NAME;
	}
	else
		if (mode != MODE_LIST)
			ask |= ASK_NAME; // pedimos el nombre solo si no estamos listando

	// indicamos que vamos a hacer
	switch (mode)
	{
	case MODE_CREATE:
		cout << "Creating new user..." << endl;
		break;

	case MODE_EDIT:
		cout << "Editing existent user..." << endl;
		break;

	case MODE_DELETE:
		cout << "Deleting user..." << endl;
		break;

	case MODE_LIST:
		cout << "List of users:" << endl;
	}
}

void UsersAdmin::AskUser (void)
{
	char check_pwd[MAX_NAME + 1];
	char current_procs[MAX_NAME + 1];
	struct termios old_flags, new_flags;

	memset (check_pwd, 0, MAX_NAME + 1);
	memset (current_procs, 0, MAX_NAME + 1);

	if (ask & ASK_NAME)
	{
		cout << "Name: ";
		fgets (input_name, MAX_NAME, stdin);
		input_name[strlen (input_name) - 1] = '\0';
	}

	if (ask & ASK_NEWNAME)
	{
		cout << "New name: ";
		fgets (input_newname, MAX_NAME, stdin);
		input_newname[strlen (input_newname) - 1] = '\0';
	}

	if (ask & ASK_PWD)
	{
		// ahora obtenemos la password
		tcgetattr (fileno (stdin), &old_flags);
		new_flags = old_flags;
		new_flags.c_lflag &= ~ECHO;
		new_flags.c_lflag |= ECHONL;

		if ((tcsetattr (fileno (stdin), TCSAFLUSH, &new_flags)) != 0)
		{
			tcsetattr (fileno (stdin), TCSANOW, &old_flags);
			perror ("Failed to set attributes");
			exit (EXIT_FAILURE);
		}

		cout << "Password: ";
		fgets (input_pwd, MAX_NAME, stdin);
		cout << "Re-type password: ";
		fgets (check_pwd, MAX_NAME, stdin);

		tcsetattr (fileno(stdin), TCSANOW, &old_flags);

		// passwords contienen los '\n'
		input_pwd[strlen (input_pwd) - 1] = '\0';
		check_pwd[strlen (check_pwd) - 1] = '\0';

		if (strlen (input_pwd) < 1 || strlen (check_pwd) < 1)
		{
			cerr << "Error: Empty passwords are not allowed" << endl;
			exit (EXIT_FAILURE);
		}

		if (strncmp (input_pwd, check_pwd, MAX_NAME))
		{
			cerr << "Error: Passwords don't match" << endl;
			exit (EXIT_FAILURE);
		}
	}

	if (ask & ASK_PROCS)
	{
		cout << "Allowed processes for this user ";
		cout << "(insert colon to separate values)" << endl;

		// mostrar valores anteriores
		SAGS_UsersFile.GetUserProcesses (input_name, current_procs);
		cout << "[" << current_procs << "] >> ";

		fgets (input_procs, MAX_NAME, stdin);
		input_procs[strlen (input_procs) - 1] = '\0';
	}
}

void UsersAdmin::Run (void)
{
	switch (mode)
	{
	case MODE_CREATE:
		SAGS_UsersFile.UpdateUser (input_name, input_name, input_pwd, input_procs);
		cout << "Created new user \"" << input_name << "\"." << endl;
		break;

	case MODE_EDIT:
		if (SAGS_UsersFile.CheckUser (input_name))
		{
			// actualizamos solo si existe el usuario
			SAGS_UsersFile.UpdateUser (input_name, input_newname,
						   input_pwd, input_procs);
			cout << "Updated user \"" << input_name << "\"." << endl;
			if (ask & ASK_NEWNAME)
			{
				cout << "Renamed from \"" << input_name << "\" to \"";
				cout << input_newname << "\"." << endl;
			}
		}
		else
		{
			cerr << "User \"" << input_name << "\" don't exists." << endl;
			exit (EXIT_FAILURE);
		}
		break;

	case MODE_DELETE:
		if (SAGS_UsersFile.DeleteUser (input_name) <= 0)
		{
			cerr << "User \"" << input_name << "\" don't exists." << endl;
			exit (EXIT_FAILURE);
		}
		else
			cout << "Deleted user \"" << input_name << "\"." << endl;
		break;

	case MODE_LIST:
		// usamos input_name para seleccionar el usuario a mostrar
		// si esta vacio, muestra la lista completa
		// si no existe el usuario, avisa.
		SAGS_UsersFile.ListUsers (input_name);
		return;
	}

	// finalmente guardamos el archivo de usuarios
	if (SAGS_UsersFile.Write () < 0)
		exit (EXIT_FAILURE);
}

int main (int argc, char **argv)
{
	UsersAdmin SAGS_Admin;

	SAGS_Admin.Init (argc, argv);
	SAGS_Admin.AskUser ();
	SAGS_Admin.Run ();

	return EXIT_SUCCESS;
}
