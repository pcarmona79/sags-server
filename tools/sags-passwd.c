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
 * $Revision: 1.1 $
 * $Date: 2004/04/13 22:00:19 $
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <openssl/md5.h>

#define MAX_NAME 80

char* md5_password_hash (const char *password)
{
	char *md5_password;
	char *md5_password_hex;
	char hexadecimal[3];
	int i, tamano;

	md5_password = calloc (MD5_DIGEST_LENGTH, sizeof (char));
	md5_password_hex = calloc (2 * MD5_DIGEST_LENGTH, sizeof (char));

	tamano = strlen (password);
	MD5 ((unsigned char *) password, tamano, (unsigned char *) md5_password);

	for ( i = 0; i < MD5_DIGEST_LENGTH; ++i ) {
		snprintf (hexadecimal, 3, "%.2x", *(md5_password + i));
		strncat (md5_password_hex, hexadecimal, sizeof (hexadecimal));
	}

	free (md5_password);
	return md5_password_hex;
}

int add_to_file (FILE *f, const char *user, const char *hash)
{
	return fprintf (f, "%s:%s\n", user, hash);
}

void print_usage (FILE *f)
{
	fprintf (f, "Usage: sags-passwd <users-file> [<new-user>]\n");
}

int main (int argc, char **argv)
{
	FILE *users_file = NULL;
	struct termios old_flags, new_flags;
	char filename[81], username[81], password1[81], password2[81];
	char *md5hash;

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

	// sacamos el hash MD5 de la password
	md5hash = md5_password_hash (password1);
	users_file = fopen (filename, "a");

	if (users_file == NULL)
	{
		perror ("fopen");
		exit (EXIT_FAILURE);
	}

	if (add_to_file (users_file, username, md5hash) <= 0)
	{
		perror ("fopen");
		exit (EXIT_FAILURE);
	}

	printf ("Added user %s to %s\n", username, filename);

	free (md5hash);
	exit (EXIT_SUCCESS);
}
