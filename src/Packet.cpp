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
// $Revision: 1.5 $
// $Date: 2004/06/20 19:17:00 $
//

#include <cstring>
#include <cstdio>

#include "Packet.hpp"

Packet::Packet (unsigned int idx, unsigned int com, unsigned int seq,
		unsigned int len, const char *data)
{
	memset (&pkt_header, 0, sizeof (pkt_header));
	SetIndex (idx);
	SetCommand (com);
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

void Packet::SetHeader (struct pkt_hdr hdr)
{
	pkt_header = hdr;
}

struct pkt_hdr Packet::GetHeader (void)
{
	return pkt_header;
}

void Packet::SetIndex (unsigned int idx)
{
	pkt_header.pkt_idx = idx;
}

unsigned int Packet::GetIndex (void)
{
	return pkt_header.pkt_idx;
}

void Packet::SetCommand (unsigned int com)
{
	pkt_header.pkt_com = com;
}

unsigned int Packet::GetCommand (void)
{
	return pkt_header.pkt_com;
}

void Packet::SetSequence (unsigned int seq)
{
	pkt_header.pkt_seq = seq;
}

unsigned int Packet::GetSequence (void)
{
	return pkt_header.pkt_seq;
}

void Packet::SetLength (unsigned int len)
{
	if (len > PCKT_MAXDATA)
		len = PCKT_MAXDATA;

	if (len > 1)
		pkt_header.pkt_len = len - 1;
	else
		pkt_header.pkt_len = 0;
}

unsigned int Packet::GetLength (void)
{
	if (GetSequence () == 0)
		return 0;
	return pkt_header.pkt_len + 1;
}

void Packet::SetData (const char *data)
{
	memset (pkt_data, 0, PCKT_MAXDATA + 1);

	if (data != NULL)
	{
		if (data[0] != '\0' && GetLength () >= 1)
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

bool Packet::IsSync (void)
{
	return (pkt_header.pkt_idx == Sync::Index);
}

bool Packet::IsAuth (void)
{
	return (pkt_header.pkt_idx == Auth::Index);
}

bool Packet::IsSession (void)
{
	return (pkt_header.pkt_idx <= Session::MaxIndex);
}

bool Packet::operator== (const Packet &Pkt)
{
	if (pkt_header.pkt_idx == Pkt.pkt_header.pkt_idx &&
	    pkt_header.pkt_com == Pkt.pkt_header.pkt_com &&
	    pkt_header.pkt_seq == Pkt.pkt_header.pkt_seq &&
	    pkt_header.pkt_len == Pkt.pkt_header.pkt_len)
	{
		if (!strncmp (pkt_data, Pkt.pkt_data, PCKT_MAXDATA))
			return true;
	}

	return false;
}
