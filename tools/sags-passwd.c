/*
 * sags-passwd - Manage password files of SAGS Server.
 * Copyright (C) 2004 Pablo Carmona Amigo.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307, USA.
 *
 * $Source: /home/pablo/Desarrollo/sags-cvs/server/tools/Attic/sags-passwd.c,v $
 * $Revision: 1.2 $
 * $Date: 2004/06/17 00:21:01 $
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>

#define MAX_NAME 80

char *encode_password (const char *password)
{
	char *encoded;
	int i;

	encoded = calloc (strlen (password) + 1, sizeof (char));

	for (i = 0; i <= strlen (password) - 1; ++i)
		encoded[i] = password[i] ^ 0xAA;

	return encoded;
}

int add_to_file (FILE *f, const char *user, const char *hash, const char *procs)
{
	return fprintf (f, "%s:%s:%s\n", user, hash, procs);
}

void print_usage (FILE *f)
{
	fprintf (f, "Usage: sags-passwd <users-file> [<new-user>]\n");
}

int main (int argc, char **argv)
{
	FILE *users_file = NULL;
	struct termios old_flags, new_flags;
	char filename[MAX_NAME + 1], username[MAX_NAME + 1];
	char password1[MAX_NAME + 1], password2[MAX_NAME + 1];
	char procs[MAX_NAME + 1];
	char *md5hash, *encodedhash;

	if (argc < 3 || argc > 3)
	{
		print_usage (stderr);
		exit (EXIT_FAILURE);
	}

	strncpy (filename, argv[1], MAX_NAME);
	strncpy (username, argv[2], MAX_NAME);

	// ahora obtenemos la password
	tcgetattr (fileno(stdin), &old_flags);
	new_flags = old_flags;
	new_flags.c_lflag &= ~ECHO;
	new_flags.c_lflag |= ECHONL;

	if ((tcsetattr (fileno(stdin), TCSAFLUSH, &new_flags)) != 0)
	{
		tcsetattr (fileno(stdin), TCSANOW, &old_flags);
		perror ("Failed to set attributes");
		exit (EXIT_FAILURE);
	}

	printf ("Password: ");
	fgets (password1, MAX_NAME, stdin);
	printf ("Re-type password: ");
	fgets (password2, MAX_NAME, stdin);

	tcsetattr (fileno(stdin), TCSANOW, &old_flags);

	// passwords contienen los '\n'
	password1[strlen (password1) - 1] = '\0';
	password2[strlen (password2) - 1] = '\0';

	if (strlen (password1) < 1 || strlen (password2) < 1)
	{
		fprintf (stderr, "Empty passwords are not allowed\n");
		exit (EXIT_FAILURE);
	}

	if (strncmp (password1, password2, MAX_NAME))
	{
		fprintf (stderr, "Passwords don't match\n");
		exit (EXIT_FAILURE);
	}

	printf ("Process allowed for this user (insert colon to separate values)\n"
		">> ");
	fgets (procs, MAX_NAME, stdin);
	procs[strlen (procs) - 1] = '\0';

	encodedhash = encode_password (password1);
	users_file = fopen (filename, "a");

	if (users_file == NULL)
	{
		perror ("fopen");
		exit (EXIT_FAILURE);
	}

	if (add_to_file (users_file, username, encodedhash, procs) <= 0)
	{
		perror ("fprintf");
		fclose (users_file);
		exit (EXIT_FAILURE);
	}

	printf ("Added user %s to %s\n", username, filename);

	fclose (users_file);
	free (encodedhash);
	exit (EXIT_SUCCESS);
}
