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
// $Revision: 1.2 $
// $Date: 2004/05/19 02:53:43 $
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
}

Packet::Packet (struct pkt packet)
{
	pkt_header = packet.pkt_header;
	SetData (packet.pkt_data);
}

Packet::Packet (const Packet &Pkt)
{	
	pkt_header = Pkt.pkt_header;
	SetData (Pkt.pkt_data);
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

bool Packet::operator== (const Packet &Pkt)
{
	if (pkt_header == Pkt.pkt_header)
	{
		if (!strncmp (pkt_data, Pkt.pkt_data, PCKT_MAXDATA))
			return true;
	}

	return false;
}
