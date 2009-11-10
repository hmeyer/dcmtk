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
 *  Purpose: test program for C++ string class
 *
 *  This file is derived from a file with the following copyright statement:
 *  Copyright (C) 1994 Free Software Foundation
 *
 *  This file is part of the GNU ANSI C++ Library.  This library is free
 *  software; you can redistribute it and/or modify it under the terms of
 *  the GNU General Public License as published by the Free Software
 *  Foundation; either version 2, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this library; see the file COPYING.  If not, write to the Free
 *  Software Foundation, 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 *
 *  Last Update:      $Author: meichel $
 *  Update Date:      $Date: 2005/12/08 15:49:10 $
 *  Source File:      $Source: /share/dicom/cvs-depot/dcmtk/ofstd/tests/tstring.cc,v $
 *  CVS/RCS Revision: $Revision: 1.7 $
 *  Status:           $State: Exp $
 *
 *  CVS/RCS Log at end of file
 *
 */

#include "dcmtk/config/osconfig.h"
#include "dcmtk/ofstd/ofstring.h"
#include "dcmtk/ofstd/ofconsol.h"
#include "dcmtk/ofstd/ofstd.h"

#define INCLUDE_CASSERT
#include "dcmtk/ofstd/ofstdinc.h"


OFString X = "Hello";
OFString Y = "world";
OFString N = "123";
OFString c;
const char *s = ",";

void decltest()
{
  OFString x;
  COUT << "an empty string:" << x << "\n";
  assert(x == "");

  OFString y = "Hello";
  COUT << "A string initialized to Hello:" << y << "\n";
  assert(y == "Hello");

  if (y[y.length()-1] == 'o')
	y = y + '\n';
  assert(y == "Hello\n");
  y = "Hello";

  OFString a = y;
  COUT << "A string initialized to previous string:" << a << "\n";
  assert(a == "Hello");
  assert(a == y);

  OFString b (a, 1, 2);
  COUT << "A string initialized to (previous string, 1, 2):" << b << "\n";
  assert(b == "el");

  char ch = '@';
  OFString z (1, ch);
  COUT << "A string initialized to @:" << z << "\n";
  assert (z == "@");

  OFString n ("20");
  COUT << "A string initialized to 20:" << n << "\n";
  assert(n == "20");

  int i = atoi(n.c_str ());
  double f = OFStandard::atof(n.c_str ());
  COUT << "n = " << n << " atoi(n) = " << i << " atof(n) = " << f << "\n";
  assert(i == 20);
  assert(f == 20);

}

void cattest()
{
  OFString x = X;
  OFString y = Y;
  OFString z = x + y;
  COUT << "z = x + y = " << z << "\n";
  assert(z == "Helloworld");

  x += y;
  COUT << "x += y; x = " << x << "\n";
  assert(x == "Helloworld");

  y = Y;
  x = X;
  y.insert (0, x);
  COUT << "y.insert (0, x); y = " << y << "\n";
  assert(y == "Helloworld");

  y = Y;
  x = X;
  x = x + y + x;
  COUT << "x = x + y + x; x = " << x << "\n";
  assert(x == "HelloworldHello");

  y = Y;
  x = X;
  x = y + x + x;
  COUT << "x = y + x + x; x = " << x << "\n";
  assert(x == "worldHelloHello");

  x = X;
  y = Y;
  z = x + s + ' ' + y.substr (y.find ('w'), 1) + y.substr (y.find ('w') + 1) + ".";
  COUT << "z = x + s +  + y.substr (y.find (w), 1) + y.substr (y.find (w) + 1) + . = " << z << "\n";
  assert(z == "Hello, world.");
}

void comparetest()
{  
  OFString x = X;
  OFString y = Y;
  OFString n = N;
  OFString z = x + y;

  assert(x != y);
  assert(x == "Hello");
  assert(x != z.substr (0, 4));
  assert(x.compare (y) < 0);
  assert(x.compare (z.substr (0, 6)) < 0);

  assert(x.find ("lo") == 3);
  assert(x.find ("l", 2) == 2);
  assert(x.rfind ("l") == 3);
}

void substrtest()
{
  OFString x = X;

  char ch = x[0];
  COUT << "ch = x[0] = " << ch << "\n";
  assert(ch == 'H');

  OFString z = x.substr (2, 3);
  COUT << "z = x.substr (2, 3) = " << z << "\n";
  assert(z == "llo");

  x.replace (2, 2, "r");
  COUT << "x.replace (2, 2, r); x = " << x << "\n";
  assert(x == "Hero");

  x = X;
  x.replace (0, 1, 1, 'j');
  COUT << "x.replace (0, 1, 'j'); x = " << x << "\n";
  assert(x == "jello");
}

void iotest()
{
  OFString z;
  COUT << "enter a word:";
  cin >> z;
  COUT << "word =" << z << " ";
  COUT << "length = " << z.length() << "\n";
}

void identitytest(OFString a, OFString b)
{
  OFString x = a;
  OFString y = b;
  x += b;
  y.insert (0, a);
  assert((a + b) == x);
  assert((a + b) == y);
  assert(x == y);
  
  assert((a + b + a) == (a + (b + a)));

  x.erase (x.rfind (b));
  assert(x == a);

  y.replace (0, y.rfind (b), b);
  assert(y == (b + b));
  y.replace (y.find (b), b.length (), a);
  assert(y == (a + b));
}

int main()
{
  decltest();
  cattest();
  comparetest();
  substrtest();
  identitytest(X, X);
  identitytest(X, Y);
  identitytest(X+Y+N+X+Y+N, "A string that will be used in identitytest but is otherwise just another useless string.");
  iotest();
  COUT << "\nEnd of test\n";
  return 0;
}


/*
**
** CVS/RCS Log:
** $Log: tstring.cc,v $
** Revision 1.7  2005/12/08 15:49:10  meichel
** Changed include path schema for all DCMTK header files
**
** Revision 1.6  2003/07/04 13:31:52  meichel
** Fixed issues with compiling with HAVE_STD_STRING
**
** Revision 1.5  2002/06/20 12:04:39  meichel
** Changed toolkit to use OFStandard::atof instead of atof, strtod or
**   sscanf for all string to double conversions that are supposed to
**   be locale independent
**
** Revision 1.4  2002/05/24 09:48:29  joergr
** Added "const" modifier to char pointer to avoid warnings reported by gcc
** 2.95.3 with additional options.
**
** Revision 1.3  2000/03/03 14:02:52  meichel
** Implemented library support for redirecting error messages into memory
**   instead of printing them to stdout/stderr for GUI applications.
**
** Revision 1.2  1998/11/27 12:42:10  joergr
** Added copyright message to source files and changed CVS header.
**
**
**
*/
