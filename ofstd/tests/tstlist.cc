/*
 *
 *  Copyright (C) 1997-2005, OFFIS
 *
 *  This software and supporting documentation were developed by
 *
 *    Kuratorium OFFIS e.V.
 *    Healthcare Information and Communication Systems
 *    Escherweg 2
 *    D-26121 Oldenburg, Germany
 *
 *  THIS SOFTWARE IS MADE AVAILABLE,  AS IS,  AND OFFIS MAKES NO  WARRANTY
 *  REGARDING  THE  SOFTWARE,  ITS  PERFORMANCE,  ITS  MERCHANTABILITY  OR
 *  FITNESS FOR ANY PARTICULAR USE, FREEDOM FROM ANY COMPUTER DISEASES  OR
 *  ITS CONFORMITY TO ANY SPECIFICATION. THE ENTIRE RISK AS TO QUALITY AND
 *  PERFORMANCE OF THE SOFTWARE IS WITH THE USER.
 *
 *  Module:  ofstd
 *
 *  Author:  Andreas Barth
 *
 *  Purpose: test programm for classes OFList and OFListIterator
 *
 *  Last Update:      $Author: meichel $
 *  Update Date:      $Date: 2005/12/08 15:49:09 $
 *  CVS/RCS Revision: $Revision: 1.8 $
 *  Status:           $State: Exp $
 *
 *  CVS/RCS Log at end of file
 *
 */

#include "dcmtk/config/osconfig.h"

#include "dcmtk/ofstd/ofstream.h"
#include "dcmtk/ofstd/ofalgo.h"
#include "dcmtk/ofstd/oflist.h"
#include "dcmtk/ofstd/ofconsol.h"


void print(OFList<int> & l)
{
    OFListIterator(int) e(l.end());
    COUT << "Werte der List: ";
    for (OFListIterator(int) i(l.begin()); i != e; ++i)
	COUT << *i << " ";
    COUT << endl;
}


int main()
{
    OFList<int> l;
    l.push_front(1);
    l.push_front(2);
    l.push_back(3);
    l.push_back(4);
    l.push_front(5);

    print(l);

    OFListIterator(int) a(l.begin());
    ++a; ++a;
    COUT << "2xa++ :Was ist jetzt a? " << *a << endl;
    l.insert(a, 50);
    l.insert(a, 10, 100);

    OFListIterator(int) e(l.end());

    COUT << "Finde 50: ";
    if (OFFind(OFListIterator(int), int, l.begin(), e, 50) != e)
	COUT << "gefunden, wie erwartet\n";
    else
	COUT << "nicht gefunden, Fehler\n";

    COUT << "Ausgabe der Liste l\n";
    print(l);

    COUT << "Kopiere Liste l in l1,\n";
    OFList<int> l1(l);
    COUT << "Ausgabe der List l1\n";
    print(l1);

    COUT << "Loesche alle 100er (nacheinander) aus l" << endl;

    OFListIterator(int) del;
    a = l.begin();
    while(a != e)
    {
	del = a++;
	if (*del == 100)
	{
	    l.erase(del);
	    COUT << "Ein 100er Element geloescht\n";
	    print(l);
	}
    }


    COUT << "Anzahl Elemente in l: " << l.size() << endl;
    COUT << "Loesche Werte mit pop_front aus l: ";
    while(!l.empty())
    {
	COUT << l.front() << " ";
	l.pop_front();
    }
    COUT << endl;

    COUT << "Loesche letztes Element aus l1: " << l1.back()
	 << " und gebe l aus:\n";
    l1.pop_back();
    print(l1);

    COUT << "Suche 1 in l1: " << endl;
    a = OFFind(OFIterator<int>, int, l1.begin(), l1.end(), 1);
    if ( a == l1.end())
	COUT << "Fehler, 1 nicht gefunden\n";
    else
	COUT << "1 gefunden (wie erwartet)\n";

    COUT << "Loesche alle Elemente hinter ab der  1 und gebe Ergebnis aus.\n";
    l1.erase(a, l1.end());
    print(l1);
    COUT << "Loesche die 50 und geben restliche List aus\n";
    l1.remove(50);
    print(l1);

    COUT << " Aufruf des Destruktors \n";
}


/*
**
** CVS/RCS Log:
** $Log: tstlist.cc,v $
** Revision 1.8  2005/12/08 15:49:09  meichel
** Changed include path schema for all DCMTK header files
**
** Revision 1.7  2004/01/16 10:37:23  joergr
** Removed acknowledgements with e-mail addresses from CVS log.
**
** Revision 1.6  2002/04/16 13:37:01  joergr
** Added configurable support for C++ ANSI standard includes (e.g. streams).
**
** Revision 1.5  2001/06/01 15:51:41  meichel
** Updated copyright header
**
** Revision 1.4  2000/03/08 16:36:08  meichel
** Updated copyright header.
**
** Revision 1.3  2000/03/03 14:02:52  meichel
** Implemented library support for redirecting error messages into memory
**   instead of printing them to stdout/stderr for GUI applications.
**
** Revision 1.2  1998/11/27 12:42:10  joergr
** Added copyright message to source files and changed CVS header.
**
**
*/
