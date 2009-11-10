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
 *  Purpose: test programm for class OFStack
 *
 *  Last Update:      $Author: meichel $
 *  Update Date:      $Date: 2005/12/08 15:49:11 $
 *  CVS/RCS Revision: $Revision: 1.9 $
 *  Status:           $State: Exp $
 *
 *  CVS/RCS Log at end of file
 *
 */

#include "dcmtk/config/osconfig.h"

#include "dcmtk/ofstd/ofstream.h"
#include "dcmtk/ofstd/ofstack.h"
#include "dcmtk/ofstd/ofconsol.h"


int main()
{
    OFStack<int> st;
    st.push(1);
    st.push(2);
    st.push(3);

    OFStack<int> nst(st);
    COUT << "Output of number of Elements in st: " << st.size() << endl;
    COUT << "Output and deletion of st: ";
    while(!st.empty())
    {
	COUT << st.top() << " ";
	st.pop();
    }
    COUT << endl;

    COUT << "Output of number of Elements in copy from st: " << nst.size() << endl;
    COUT << "Output and deletion of copy from st: ";
    while(!nst.empty())
    {
	COUT << nst.top() << " ";
	nst.pop();
    }
    COUT << endl;
}


/*
**
** CVS/RCS Log:
** $Log: tststack.cc,v $
** Revision 1.9  2005/12/08 15:49:11  meichel
** Changed include path schema for all DCMTK header files
**
** Revision 1.8  2004/01/16 10:37:23  joergr
** Removed acknowledgements with e-mail addresses from CVS log.
**
** Revision 1.7  2002/04/16 13:37:01  joergr
** Added configurable support for C++ ANSI standard includes (e.g. streams).
**
** Revision 1.6  2001/06/01 15:51:41  meichel
** Updated copyright header
**
** Revision 1.5  2000/03/08 16:36:08  meichel
** Updated copyright header.
**
** Revision 1.4  2000/03/03 14:02:53  meichel
** Implemented library support for redirecting error messages into memory
**   instead of printing them to stdout/stderr for GUI applications.
**
** Revision 1.3  1998/11/27 12:42:11  joergr
** Added copyright message to source files and changed CVS header.
**
**
**
*/
