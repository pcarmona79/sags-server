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
// $Source: /home/pablo/Desarrollo/sags-cvs/server/src/List.hpp,v $
// $Revision: 1.2 $
// $Date: 2004/06/07 02:22:58 $
//

#ifndef __LIST_HPP__
#define __LIST_HPP__

#include <cassert>

// Plantilla Node

template <class N>
class Node
{
public:
	Node (const N& d, Node<N> *p = NULL, Node<N> *n = NULL);
	Node (N *d, Node<N> *p = NULL, Node<N> *n = NULL);
	Node (Node<N> &Ref);
	~Node ();

	N *Data;
	Node<N> *Prev;
	Node<N> *Next;
};

template <class N>
Node<N>::Node (const N& d, Node<N> *p, Node<N> *n)
{
	Data = new N (d);
	Prev = p;
	Next = n;
}
template <class N>
Node<N>::Node (N *d, Node<N> *p, Node<N> *n)
{
	Data = d;
	Prev = p;
	Next = n;
}

template <class N>
Node<N>::Node (Node<N> &Ref)
{
	Data = new N (*Ref.Data);
	Prev = Ref.Prev;
	Next = Ref.Next;
}

template <class N>
Node<N>::~Node ()
{
	delete Data;
}


// Plantilla List

template <class L>
class List
{
private:
	Node<L> *First;
	Node<L> *Last;
	unsigned int Count;

public:
	List ();
	List (List<L> &lst);
	~List ();

	unsigned int GetCount (void);
	void Add (const L& d, bool atStart = false);
	void Add (L *d, bool atStart = false);
	L* ExtractFirst (void);
	L* ExtractLast (void);
	L* Find (L& d);
	unsigned int Remove (L& d, bool repeat = false);
	L Index (unsigned int n);
	void Clear (void);

	void operator<< (const L& d);
	void operator<< (L *d);
	L* operator[] (unsigned int n);
};

template <class L>
List<L>::List () : First (NULL), Last (NULL)
{
	Count = 0;
}

template <class L>
List<L>::List (List<L> &lst) : First (NULL), Last (NULL)
{
	int i, maxcount = lst.GetCount ();

	Count = 0;
	for (i = 0; i <= maxcount - 1; ++i)
		Add (lst[i]);
}

template <class L>
List<L>::~List ()
{
	Node<L> *Temp = Last;

	while (Last != NULL)
	{
		Temp = Last;
		Last = Last->Prev;
		delete Temp;
	}
	Count = 0;
}

template <class L>
unsigned int List<L>::GetCount (void)
{
	return Count;
}

template <class L>
void List<L>::Add (const L& d, bool atStart)
{
	if (First != NULL && Last != NULL)
	{
		if (atStart)
		{
			First->Prev = new Node<L> (d, NULL, First);
			First = First->Prev;
		}
		else
		{
			Last->Next = new Node<L> (d, Last, NULL);
			Last = Last->Next;
		}
	}
	else
	{
		First = new Node<L> (d, NULL, NULL);
		Last = First;
	}

	++Count;
}

template <class L>
void List<L>::Add (L *d, bool atStart)
{
	if (First != NULL && Last != NULL)
	{
		if (atStart)
		{
			First->Prev = new Node<L> (d, NULL, First);
			First = First->Prev;
		}
		else
		{
			Last->Next = new Node<L> (d, Last, NULL);
			Last = Last->Next;
		}
	}
	else
	{
		First = new Node<L> (d, NULL, NULL);
		Last = First;
	}

	++Count;
}

template <class L>
L* List<L>::ExtractFirst (void)
{
	Node<L> *Temp = First;

	First = First->Next;
	if (First != NULL)
		First->Prev = NULL;
	else
		Last = NULL;

	--Count;

	if (Temp != NULL)
		return Temp->Data;
	
	return NULL;
}

template <class L>
L* List<L>::ExtractLast (void)
{
	Node<L> *Temp = Last;

	Last = Last->Prev;
	if (Last != NULL)
		Last->Next = NULL;
	else
		First = NULL;

	--Count;

	if (Temp != NULL)
		return Temp->Data;
	
	return NULL;
}

template <class L>
L* List<L>::Find (L& d)
{
	Node<L> *Temp;

	for (Temp = First; Temp; Temp = Temp->Next)
	{
		if (d == *Temp->Data)
			return Temp->Data;
	}

	return NULL;
}

template <class L>
unsigned int List<L>::Remove (L& d, bool repeat)
{
	Node<L> *Temp;
	unsigned int del = 0;

	for (Temp = First; Temp; Temp = Temp->Next)
	{
		if (d == *Temp->Data)
		{
			if (Temp->Prev != NULL)
				Temp->Prev->Next = Temp->Next;
			else
				First = Temp->Next;

			if (Temp->Next != NULL)
				Temp->Next->Prev = Temp->Prev;
			else
				Last = Temp->Prev;

			delete Temp;
			--Count;
			++del;

			if (!repeat)
				break;
		}
	}

	return del;
}

template <class L>
L List<L>::Index (unsigned int n)
{
	Node<L> *Temp;
	unsigned int i;

	for (i = 0, Temp = First; i < Count || Temp; ++i, Temp = Temp->Next)
	{
		if (i == n)
			break;
	}

	return *Temp->Data;
}

template <class L>
void List<L>::Clear (void)
{
	Node<L> *Temp = Last;

	while (Last != NULL)
	{
		Temp = Last;
		Last = Last->Prev;
		delete Temp;
		--Count;
	}

	assert (Count == 0);
}

template <class L>
void List<L>::operator<< (const L& d)
{
	Add (d);
}

template <class L>
void List<L>::operator<< (L *d)
{
	Add (d);
}

template <class L>
L* List<L>::operator[] (unsigned int n)
{
	Node<L> *Temp;
	unsigned int i;

	for (i = 0, Temp = First; i < Count || Temp; ++i, Temp = Temp->Next)
	{
		if (i == n)
			break;
	}

	return Temp->Data;
}

#endif // __LIST_HPP__
