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
// $Revision: 1.9 $
// $Date: 2004/08/13 00:55:57 $
//

#ifndef __PACKET_HPP__
#define __PACKET_HPP__

#include <cstdlib>

#define PCKT_VERSION "3"
#define PCKT_MAXDATA 256

namespace Session
{
	const unsigned int MainIndex = 0x00;
	const unsigned int MinIndex  = 0x01;
	const unsigned int MaxIndex  = 0xFC;

	typedef enum
	{
		Disconnect      = 0x01,
		Authorized      = 0x02,

		ConsoleInput    = 0x03,
		ConsoleOutput   = 0x04,
		ConsoleSuccess  = 0x05,
		ConsoleNeedLogs = 0x06,
		ConsoleLogs     = 0x07,

		ProcessGetInfo  = 0x08,
		ProcessInfo     = 0x09,
		ProcessExits    = 0x0A,
		ProcessStart    = 0x0B,
		ProcessKill	= 0x0C,
		ProcessLaunch   = 0x0D,
		ProcessRestart  = 0x0E,

		ChatUserList    = 0x0F,
		ChatTopic	= 0x10,
		ChatTopicChange = 0x11,
		ChatJoin        = 0x12,
		ChatLeave       = 0x13,
		ChatMessage     = 0x14,
		ChatAction      = 0x15,
		ChatNotice      = 0x16,
		ChatPrivMessage = 0x17,
		ChatPrivAction  = 0x18,
		ChatPrivNotice  = 0x19
	} Type;
}

namespace Auth
{
	const unsigned int Index = 0xFD;

	typedef enum
	{
		Username   = 0x00,
		RandomHash = 0x01,
		Password   = 0x02,
		Successful = 0x03
	} Type;
}

namespace Sync
{
	const unsigned int Index = 0xFE;
	
	typedef enum
	{
		Hello   = 0x00,
		Version = 0x01
	} Type;
}

namespace Error
{
	const unsigned int Index = 0xFF;

	typedef enum
	{
		// desconectan:
		ServerFull           = 0x00,
		NotValidVersion      = 0x01,
		LoginFailed          = 0x02,
		AuthTimeout          = 0x03,
		ServerQuit           = 0x04,
		LoggedFromOtherPlace = 0x05,

		// no desconectan:
		BadProcess           = 0x80,
		CantWriteToProcess   = 0x81,
		ProcessNotKilled     = 0x82,
		ProcessNotLaunched   = 0x83,
		ProcessNotRestarted  = 0x84,
		Generic              = 0xFF
	} Type;
}

namespace Mask
{
	const int Idx = 0xFF000000;
	const int Com = 0x00FF0000;
	const int Seq = 0x0000FF00;
	const int Len = 0x000000FF;
}

struct pkt_hdr
{
	unsigned int pkt_idx : 8;
	unsigned int pkt_com : 8;
	unsigned int pkt_seq : 8;
	unsigned int pkt_len : 8;
};

struct pkt
{
	struct pkt_hdr pkt_header;
	char pkt_data[PCKT_MAXDATA + 1];
};

class Packet
{
private:
	struct pkt_hdr pkt_header;
	char pkt_data[PCKT_MAXDATA + 1];

public:
	Packet (unsigned int idx = 0, unsigned int com = 0, unsigned int seq = 0,
		unsigned int len = 0, const char *data = NULL);
	Packet (struct pkt packet);
	Packet (const Packet &Pkt);
	~Packet ();

	void SetHeader (struct pkt_hdr hdr);
	struct pkt_hdr GetHeader (void);

	void SetIndex (unsigned int idx);
	unsigned int GetIndex (void);

	void SetCommand (unsigned int com);
	unsigned int GetCommand (void);

	void SetSequence (unsigned int seq);
	unsigned int GetSequence (void);

	void SetLength (unsigned int len);
	unsigned int GetLength (void);

	void SetData (const char *data);
	char *GetData (void);

	bool IsSync (void);
	bool IsAuth (void);
	bool IsSession (void);

	bool operator== (const Packet &Pkt);
};

#endif // __PACKET_HPP__
