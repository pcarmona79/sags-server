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
// $Source: /home/pablo/Desarrollo/sags-cvs/server/src/Utils.cpp,v $
// $Revision: 1.5 $
// $Date: 2005/03/16 21:33:33 $
//

#include <iostream>
#include <cstring>
#include <cmath>
#include <openssl/md5.h>
#include <openssl/evp.h>

using namespace std;

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

	//cout << "n = " << n << " delim = '" << delim << "'" << endl;

	for (i = 0; i <= elem - 1; ++i)
	{
		start = p;

		for (len = 0; *p != delim; ++len, ++p)
		{
			//cout << "p = '" << p << "'" << endl;
			if (*p == '\0')
				break;
		}

		//cout << "For passed! p = '" << *p << "'" << endl;

		while (*p == delim)
			++p;

		vect[i] = new char[len + 1];
		memset (vect[i], 0, len + 1);

		//cout << "len = " << len << endl;

		if (len > 0)
		{
			//cout << "start = '" << start << "'" << endl;
			strncpy (vect[i], start, len);
		}
		else
			vect[i][0] = '\0';

		//cout << "vect[" << i << "] = '" << vect[i] << "'" << endl;
	}

	//strncpy (vect[elem], p, strlen (p));
	//vect[elem + 1][0] = '\0';
	vect[elem] = NULL;

	if (count != NULL)
		*count = elem;

	return vect;
}

char *substring (const char *src, int from, int to)
{
	char *dest = (char*) NULL;
	int len_src, len_dest, i, step;

	if ( src == NULL ) return NULL;

	len_src = strlen (src);

	if (len_src > 0 && from >= 0 && to >= 0)
	{
		if ( to - from >= 0 )
		{
			len_dest = to - from + 1;
			step = 1;
		}
		else
		{
			len_dest = from - to + 1;
			step = -1;
		}

		if ( len_dest <= len_src )
		{
			dest = new char[len_dest + 1];
			memset (dest, 0, len_dest + 1);

			for ( i = 0; i < len_dest; ++i )
				*(dest + i) = *(src + from + i*step);

			*(dest + len_dest) = (char) NULL;
		}
	}

	return dest;
}

unsigned char* calculateMD5(unsigned char* buf, unsigned int bufsize) {
	EVP_MD_CTX* mdctx;
	unsigned char* md5digest;
	unsigned int md5digestlen = EVP_MD_size(EVP_md5());

	// MD5_Init
	mdctx = EVP_MD_CTX_new();
	EVP_DigestInit_ex(mdctx, EVP_md5(), NULL);

	// MD5_Update
	EVP_DigestUpdate(mdctx, buf, bufsize);

	// MD5_Final
	md5digest = (unsigned char*) OPENSSL_malloc(md5digestlen);
	EVP_DigestFinal_ex(mdctx, md5digest, &md5digestlen);
	EVP_MD_CTX_free(mdctx);

	return md5digest;
}

char *md5_password_hash (const char *password)
{
	unsigned char *md5_password;
	char *md5_password_hex;
	char hexadecimal[3];
	int i, tamano;

	md5_password = new unsigned char [MD5_DIGEST_LENGTH + 1];
	memset (md5_password, 0, MD5_DIGEST_LENGTH + 1);
	md5_password_hex = new char [2 * MD5_DIGEST_LENGTH + 1];
	memset (md5_password_hex, 0, 2 * MD5_DIGEST_LENGTH + 1);

	tamano = strlen (password);
	md5_password = calculateMD5 ((unsigned char *) password, tamano);

	for ( i = 0; i < MD5_DIGEST_LENGTH; ++i ) {
		snprintf (hexadecimal, 3, "%.2x", *(md5_password + i));
		strncat (md5_password_hex, hexadecimal, 3);
	}

	delete[] md5_password;
	return md5_password_hex;
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

void random_string (char *str, int size)
{
	int i, rndval;

	for (i = 0; i <= size - 1; ++i)
	{
		rndval = (int) trunc (61.0 * rand () / (RAND_MAX + 1.0));

		if (rndval >= 0 && rndval <= 9)
			str[i] = (char) (rndval + 48);
		else if (rndval >= 10 && rndval <= 35)
			str[i] = (char) (rndval + 55);
		else if (rndval >= 36 && rndval <= 61)
			str[i] = (char) (rndval + 61);
		else
			str[i] = '\0'; // ups!
	}
}
