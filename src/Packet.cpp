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
// $Source: /home/pablo/Desarrollo/sags-cvs/server/src/Packet.cpp,v $
// $Revision: 1.1 $
// $Date: 2004/04/13 22:00:19 $
//

#include <cstring>
#include <cstdio>
#include <cmath>

#include "Packet.hpp"

Packet::Packet (unsigned int type, unsigned int seq, unsigned int len, const char *data)
{
	pkt_header = 0;
	SetType (type);
	SetSequence (seq);
	SetLength (len);
	SetData (data);
	Next = NULL;
}

Packet::Packet (unsigned int type, const char *data)
{
	const char *p = data;
	int s;
	Packet *Pkt = this;

	// data NO DEBE ser nulo!!!

	// calculamos cuantos paquetes necesitaremos
	// que corresponde a la parte entera más uno de
	// TamañoTotal / 1024
	s = (int) trunc (strlen (data) / 1024) + 1;

	pkt_header = 0;
	SetType (type);
	SetSequence (s--);
	SetLength (strlen (p)); // asigna hasta 1024 bytes
	SetData (p);

	if (GetLength () == 1024 && strlen (p) > 1024)
	{
		p += 1024;
		while (strlen (p) >= 1024)
		{
			Pkt->Next = new Packet (type, s--, strlen (p), p); // asigna hasta 1024 bytes
			p += 1024;
			Pkt = Pkt->Next;
		}
		if (strlen (p) > 0 && strlen (p) < 1024)
			Pkt->Next = new Packet (type, s--, strlen (p), p);
	}
	else
		Next = NULL;

	//printf ("New packet: TYPE: %04X SEQ: %d LEN: %d DATA: \"%s\"\n",
	//	this->GetType (), this->GetSequence (), this->GetLength (), this->GetData ());
	//if (this->Next != NULL)
	//	printf ("2º New packet: TYPE: %04X SEQ: %d LEN: %d DATA: \"%s\"\n",
	//		this->Next->GetType (), this->Next->GetSequence (),
	//		this->Next->GetLength (), this->Next->GetData ());
}

Packet::Packet (struct pkt packet)
{
	pkt_header = packet.pkt_header;
	SetData (packet.pkt_data);
	Next = NULL;
}

Packet::~Packet ()
{
	
}

void Packet::SetHeader (unsigned int hdr)
{
	pkt_header = hdr;
}

unsigned int Packet::GetHeader (void)
{
	return pkt_header;
}

void Packet::SetType (unsigned int type)
{
	pkt_header &= ~(Mask::Type);
	pkt_header |= (type << 16) & Mask::Type;
}

unsigned int Packet::GetType (void)
{
	return (pkt_header & Mask::Type) >> 16;
}

void Packet::SetSequence (unsigned int seq)
{
	pkt_header &= ~(Mask::Seq);
	pkt_header |= (seq << 10) & Mask::Seq;
}

unsigned int Packet::GetSequence (void)
{
	return (pkt_header & Mask::Seq) >> 10;
}

void Packet::SetLength (unsigned int len)
{
	if (len > PCKT_MAXDATA)
		len = PCKT_MAXDATA;

	if (len > 1)
		pkt_header |= (len - 1) & Mask::Len;
	else
		pkt_header &= ~(Mask::Len);
}

unsigned int Packet::GetLength (void)
{
	if (GetSequence () == 0)
		return 0;
	return (pkt_header & Mask::Len) + 1;
}

void Packet::SetData (const char *data)
{
	memset (pkt_data, 0, PCKT_MAXDATA + 1);

	if (data != NULL)
	{
		if (data[0] != '\0' && (pkt_header & Mask::Len) >= 0)
			strncpy (pkt_data, data, GetLength ());
		else
			SetLength (0);
	}
	else
		SetLength (0);
}

char *Packet::GetData (void)
{
	return pkt_data;
}
