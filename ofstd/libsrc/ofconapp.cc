/*
 *
 *  Copyright (C) 1999-2005, OFFIS
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
 *  Author:  Joerg Riesmeier
 *
 *  Purpose: Handle console applications (Source)
 *
 *  Last Update:      $Author: meichel $
 *  Update Date:      $Date: 2005/12/08 15:48:50 $
 *  Source File:      $Source: /share/dicom/cvs-depot/dcmtk/ofstd/libsrc/ofconapp.cc,v $
 *  CVS/RCS Revision: $Revision: 1.21 $
 *  Status:           $State: Exp $
 *
 *  CVS/RCS Log at end of file
 *
 */


#include "dcmtk/config/osconfig.h"

#include "dcmtk/ofstd/ofconapp.h"
#include "dcmtk/ofstd/ofstring.h"



/*------------------*
 *  implementation  *
 *------------------*/

OFConsoleApplication::OFConsoleApplication(const char *app,
                                           const char *desc,
                                           const char *rcsid)
 : Name(app),
   Description(desc),
   Identification(rcsid),
   QuietMode(OFFalse),
   CmdLine(NULL)
{
}


OFConsoleApplication::~OFConsoleApplication()
{
}


OFBool OFConsoleApplication::parseCommandLine(OFCommandLine &cmd,
                                              int argCount,
                                              char *argValue[],
                                              const int flags,
                                              const int startPos)
{
    CmdLine = &cmd;                           // store reference to cmdline object
    OFCommandLine::E_ParseStatus status = cmd.parseLine(argCount, argValue, flags, startPos);
    OFBool result = OFFalse;
    switch (status)
    {
        case OFCommandLine::PS_NoArguments:
            /* check whether to print the "usage text" or not */
            if (((cmd.getMinParamCount() > 0) && !cmd.hasExclusiveOption()) || cmd.findOption("--help"))
                printUsage();
            else
                result = OFTrue;
            break;
        case OFCommandLine::PS_Normal:
            result = OFTrue;
            break;
        default:
            {
                OFString str;
                cmd.getStatusString(status, str);
                printError(str.c_str());
            }
            break;
    }
    return result;
}


void OFConsoleApplication::printHeader(const OFBool hostInfo,
                                       const OFBool stdError)
{
    /* lock output stream */
    ostream *output = (stdError) ? &ofConsole.lockCerr() : &ofConsole.lockCout();
    if (!Identification.empty())
        (*output) << Identification << endl << endl;
    (*output) << Name;
    if (!Description.empty())
        (*output) << ": " << Description;
    (*output) << endl;
    /* print optional host information */
    if (hostInfo)
        (*output) << endl << "Host type: " << CANONICAL_HOST_TYPE << endl;
    /* release output stream */
    if (stdError)
        ofConsole.unlockCerr();
    else
        ofConsole.unlockCout();
}


void OFConsoleApplication::printUsage(const OFCommandLine *cmd)
{
    if (cmd == NULL)
        cmd = CmdLine;
    printHeader();
    ostream &output = ofConsole.lockCout();
    output << "usage: " << Name;
    if (cmd != NULL)
    {
        OFString str;
        cmd->getSyntaxString(str);
        output << str << endl;
        cmd->getParamString(str);
        if (str.length() > 0)
            output << endl << str;
        cmd->getOptionString(str);
        if (str.length() > 0)
            output << endl << str;
    }
    output << endl;
    ofConsole.unlockCout();
    /* exit code: no error */
    exit(0);
}


void OFConsoleApplication::printError(const char *str,
                                      const int code)
{
    if (!QuietMode)
    {
        printHeader(OFFalse /*hostInfo*/, OFTrue /*stdError*/);
        ofConsole.lockCerr() << "error: " << str << endl;
        ofConsole.unlockCerr();
    }
    exit(code);
}


void OFConsoleApplication::printWarning(const char *str,
                                        const char *prefix)
{
    if (!QuietMode)
    {
        ofConsole.lockCerr() << Name << ": ";
        if ((prefix != NULL) && (strlen(prefix) > 0))
            ofConsole.getCerr() << prefix << ": ";
        ofConsole.getCerr() << str << endl;
        ofConsole.unlockCerr();
    }
}


void OFConsoleApplication::printMessage(const char *str)
{
    if (!QuietMode)
    {
        ofConsole.lockCerr() << str << endl;
        ofConsole.unlockCerr();
    }
}


OFBool OFConsoleApplication::quietMode() const
{
    return QuietMode;
}


void OFConsoleApplication::setQuietMode(const OFBool mode)
{
    QuietMode = mode;
}


void OFConsoleApplication::checkValue(const OFCommandLine::E_ValueStatus status,
                                      OFCommandLine *cmd)
{
    if (cmd == NULL)
        cmd = CmdLine;
    if (status != OFCommandLine::VS_Normal)
    {
        OFString str;
        if (cmd != NULL)
            cmd->getStatusString(status, str);
        if (str.length() > 0)
            printError(str.c_str());
    }
}


void OFConsoleApplication::checkParam(const OFCommandLine::E_ParamValueStatus status,
                                      OFCommandLine *cmd)
{
    if (cmd == NULL)
        cmd = CmdLine;
    if (status != OFCommandLine::PVS_Normal)
    {
        OFString str;
        if (cmd != NULL)
            cmd->getStatusString(status, str);
        if (str.length() > 0)
            printError(str.c_str());
    }
}


void OFConsoleApplication::checkDependence(const char *subOpt,
                                           const char *baseOpt,
                                           OFBool condition)
{
    if (!condition)
    {
        OFString str = subOpt;
        str += " only allowed with ";
        str += baseOpt;
        printError(str.c_str());
    }
}


void OFConsoleApplication::checkConflict(const char *firstOpt,
                                         const char *secondOpt,
                                         OFBool condition)
{
    if (condition)
    {
        OFString str = firstOpt;
        str += " not allowed with ";
        str += secondOpt;
        printError(str.c_str());
    }
}


/*
 *
 * CVS/RCS Log:
 * $Log: ofconapp.cc,v $
 * Revision 1.21  2005/12/08 15:48:50  meichel
 * Changed include path schema for all DCMTK header files
 *
 * Revision 1.20  2003/06/12 13:22:43  joergr
 * Enhanced method printWarning(). Added method quietMode().
 *
 * Revision 1.19  2002/11/26 12:57:07  joergr
 * Changed syntax usage output for command line applications from stderr to
 * stdout.
 *
 * Revision 1.18  2002/09/24 15:29:17  joergr
 * Optionally print command line application header with "host type" (as
 * reported by 'config.guess').
 *
 * Revision 1.17  2002/09/23 14:57:00  joergr
 * Prepared code for future support of 'config.guess' host identifiers.
 *
 * Revision 1.16  2002/09/19 08:30:33  joergr
 * Added general support for "exclusive" command line options besides "--help",
 * e.g. "--version".
 *
 * Revision 1.15  2001/06/01 15:51:37  meichel
 * Updated copyright header
 *
 * Revision 1.14  2000/04/14 15:17:16  meichel
 * Adapted all ofstd library classes to consistently use ofConsole for output.
 *
 * Revision 1.13  2000/03/08 16:36:06  meichel
 * Updated copyright header.
 *
 * Revision 1.12  2000/03/07 15:38:54  joergr
 * Changed behaviour of class OFConsoleApplication to support automatic
 * evaluation of "--help" option for command line application with no
 * mandatory parameter.
 *
 * Revision 1.11  2000/03/03 14:02:50  meichel
 * Implemented library support for redirecting error messages into memory
 *   instead of printing them to stdout/stderr for GUI applications.
 *
 * Revision 1.10  2000/03/02 12:40:39  joergr
 * Fixed inconsistency: console applications with no or only optional
 * parameters could not be started without any command line argument
 * because this was always regarded identical with "--help" (print usage).
 *
 * Revision 1.9  1999/09/13 16:38:00  joergr
 * Added methods for output of warning and other messages.
 * Added method to switch on/off all output messages (quiet mode).
 *
 * Revision 1.8  1999/05/03 11:02:13  joergr
 * Minor code purifications to keep Sun CC 2.0.1 quiet.
 *
 * Revision 1.7  1999/04/30 16:41:03  meichel
 * Minor code purifications to keep Sun CC 2.0.1 quiet
 *
 * Revision 1.6  1999/04/27 17:48:54  joergr
 * Corrected bug: option '--help' could not be used when mandatory parameters
 * were missing.
 * Changed output of usage text (moved some newlines to support output when
 * parameters and/or options are absent).
 *
 * Revision 1.5  1999/04/27 16:27:09  joergr
 * Introduced list of valid parameters used for syntax output and error
 * checking.
 * Added method to check conflicts between two options (incl. error output).
 *
 * Revision 1.4  1999/04/26 16:36:22  joergr
 * Added support to check dependences between different options and report
 * error messages if necessary.
 *
 * Revision 1.3  1999/04/21 12:41:06  meichel
 * Added method OFConsoleApplication::checkParam()
 *
 * Revision 1.2  1999/03/24 17:03:57  joergr
 * Modified output of usage string: "[options]" are only printed if valid
 * options exist.
 *
 * Revision 1.1  1999/02/08 12:00:14  joergr
 * Added class to handle console applications (with or w/o command line
 * arguments).
 *
 *
 */
