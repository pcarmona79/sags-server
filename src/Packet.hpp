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
// $Source: /home/pablo/Desarrollo/sags-cvs/server/src/Packet.hpp,v $
// $Revision: 1.2 $
// $Date: 2004/05/19 02:53:43 $
//

#ifndef __PACKET_HPP__
#define __PACKET_HPP__

#include <cstdlib>

#define PCKT_MAXDATA 1024

namespace Pckt
{
	typedef enum
	{
		// mensaje nulo
		Null = 0x0000,

		// mensajes de sesión
		SessionConsoleInput    = 0x0001,
		SessionConsoleOutput   = 0x0002,
		SessionConsoleSuccess  = 0x0003,
		SessionConsoleNeedLogs = 0x0004,
		SessionConsoleLogs     = 0x0005,

		SessionProcessExits    = 0x0006,
		SessionProcessStart    = 0x0007,

		SessionDisconnect      = 0x000A,
		SessionDrop            = 0x000B,

		// mensajes de autenticación
		AuthUsername   = 0xFD00,
		AuthRandomHash = 0xFD01,
		AuthPassword   = 0xFD02,
		AuthSuccessful = 0xFD03,

		// mensajes de sincronización
		SyncHello   = 0xFE00,
		SyncVersion = 0xFE01,

		// mensajes de error
		// desconetan:
		ErrorServerFull         = 0xFF00,
		ErrorNotValidVersion    = 0xFF01,
		ErrorLoginFailed        = 0xFF02,
		ErrorAuthTimeout        = 0xFF03,
		ErrorServerQuit         = 0xFF04,
		// no desconectan:
		ErrorBadProcess         = 0xFF80,
		ErrorCantWriteToProcess = 0xFF81,
		ErrorGeneric            = 0xFFFF
	} Type;
};

namespace Mask
{
	const int Type = 0xFFFF0000;
	const int Seq  = 0x0000FC00;
	const int Len  = 0x000003FF;
};

struct pkt
{
	unsigned int pkt_header;
	char pkt_data[PCKT_MAXDATA + 1];
};

class Packet
{
private:
	unsigned int pkt_header;
	char pkt_data[PCKT_MAXDATA + 1];

public:
	Packet (unsigned int type = 0, unsigned int seq = 0, unsigned int len = 0, const char *data = NULL);
	Packet (struct pkt packet);
	Packet (const Packet &Pkt);
	~Packet ();

	void SetHeader (unsigned int hdr);
	unsigned int GetHeader (void);

	void SetType (unsigned int type);
	unsigned int GetType (void);

	void SetSequence (unsigned int seq);
	unsigned int GetSequence (void);

	void SetLength (unsigned int len);
	unsigned int GetLength (void);

	void SetData (const char *data);
	char *GetData (void);

	bool operator== (const Packet &Pkt);
};

#endif // __PACKET_HPP__
