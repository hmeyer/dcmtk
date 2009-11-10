/*
 *
 *  Copyright (C) 2003, OFFIS
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
 *  Module:  dcmdata
 *
 *  Author:  Michael Onken
 *
 *  Purpose: Commandline-Application to modify tags in DICOM-Files
 *
 *  Last Update:      $Author: meichel $
 *  Update Date:      $Date: 2005/12/08 15:40:49 $
 *  Source File:      $Source: /share/dicom/cvs-depot/dcmtk/dcmdata/apps/dcmodify.cc,v $
 *  CVS/RCS Revision: $Revision: 1.6 $
 *  Status:           $State: Exp $
 *
 *  CVS/RCS Log at end of file
 *
 */

#include "dcmtk/config/osconfig.h"   // make sure OS specific configuration is included first
#include "mdfconen.h"

#define OFFIS_CONSOLE_APPLICATION "dcmodify"

int main(int argc, char *argv[])
{
    int error_count=0;
    MdfConsoleEngine engine(argc,argv,OFFIS_CONSOLE_APPLICATION);
    error_count=engine.startProvidingService();
    if (error_count > 0)
	    CERR << "There were " << error_count << " error(s)" << endl;
    return(error_count);
}

/*
** CVS/RCS Log:
** $Log: dcmodify.cc,v $
** Revision 1.6  2005/12/08 15:40:49  meichel
** Changed include path schema for all DCMTK header files
**
** Revision 1.5  2004/10/22 16:53:26  onken
** - fixed ignore-errors-option
** - major enhancements for supporting private tags
** - removed '0 Errors' output
** - modifications to groups 0000,0001,0002,0003,0005 and 0007 are blocked,
**   removing tags with group 0001,0003,0005 and 0007 is still possible
** - UID options:
**   - generate new study, series and instance UIDs
**   - When changing UIDs in dataset, related metaheader tags are updated
**     automatically
** - minor code improvements
**
** Revision 1.4  2003/10/13 14:52:59  onken
** error-message adapted to mdfconen.cc
**
** Revision 1.3  2003/09/19 12:47:21  onken
** return-value is now only zero, if no error occurred
**
** Revision 1.2  2003/07/09 12:13:13  meichel
** Included dcmodify in MSVC build system, updated headers
**
** Revision 1.1  2003/06/26 09:17:18  onken
** Added commandline-application dcmodify.
**
**
*/
