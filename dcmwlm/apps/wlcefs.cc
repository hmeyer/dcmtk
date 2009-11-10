/*
 *
 *  Copyright (C) 1996-2005, OFFIS
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
 *  Module:  dcmwlm
 *
 *  Author:  Thomas Wilkens
 *
 *  Purpose: Class representing a console engine for basic worklist
 *           management service class providers based on the file system.
 *
 *  Last Update:      $Author: meichel $
 *  Update Date:      $Date: 2005/12/08 15:48:30 $
 *  Source File:      $Source: /share/dicom/cvs-depot/dcmtk/dcmwlm/apps/wlcefs.cc,v $
 *  CVS/RCS Revision: $Revision: 1.10 $
 *  Status:           $State: Exp $
 *
 *  CVS/RCS Log at end of file
 *
 */

// ----------------------------------------------------------------------------

#include "dcmtk/config/osconfig.h"
#include "dcmtk/dcmnet/dicom.h"
#include "dcmtk/ofstd/ofcmdln.h"
#include "dcmtk/dcmwlm/wltypdef.h"
#include "dcmtk/dcmdata/dcxfer.h"
#include "dcmtk/ofstd/ofconapp.h"
#include "dcmtk/dcmnet/assoc.h"
#include "dcmtk/dcmdata/cmdlnarg.h"
#include "dcmtk/dcmnet/dimse.h"
#include "dcmtk/dcmdata/dcdebug.h"
#include "dcmtk/dcmdata/dcvrat.h"
#include "dcmtk/dcmdata/dcvrlo.h"
#include "dcmtk/dcmwlm/wlds.h"
#include "dcmtk/dcmdata/dcsequen.h"
#include "dcmtk/dcmwlm/wldsfs.h"
#include "dcmtk/dcmwlm/wlmactmg.h"
#include "dcmtk/dcmnet/dimse.h"

#include "wlcefs.h"

#ifdef WITH_ZLIB
#include <zlib.h>        /* for zlibVersion() */
#endif

// ----------------------------------------------------------------------------

#define SHORTCOL 4
#define LONGCOL 21

// ----------------------------------------------------------------------------

WlmConsoleEngineFileSystem::WlmConsoleEngineFileSystem( int argc, char *argv[], const char *applicationName, WlmDataSource *dataSourcev )
// Date         : December 17, 2001
// Author       : Thomas Wilkens
// Task         : Constructor.
// Parameters   : argc            - [in] Specifies how many arguments were passed to the program from the
//                                  command line.
//                argv            - [in] An array of null-terminated strings containing the arguments that
//                                  were passed to the program from the command line.
//                applicationName - [in] The name of this console application.
//                dataSourcev     - [in] Pointer to the dataSource object.
// Return Value : none.
  : opt_returnedCharacterSet( RETURN_NO_CHARACTER_SET ),
    opt_dfPath( NULL ), opt_port( 0 ), opt_refuseAssociation( OFFalse ),
    opt_rejectWithoutImplementationUID( OFFalse ), opt_sleepAfterFind( 0 ), opt_sleepDuringFind( 0 ),
    opt_maxPDU( ASC_DEFAULTMAXPDU ), opt_networkTransferSyntax( EXS_Unknown ),
    opt_verbose( OFFalse ), opt_debug( OFFalse ), opt_failInvalidQuery( OFTrue ), opt_singleProcess( OFTrue ),
    opt_maxAssociations( 50 ), opt_noSequenceExpansion( OFFalse ), opt_enableRejectionOfIncompleteWlFiles( OFTrue ),
    opt_blockMode(DIMSE_BLOCKING), opt_dimse_timeout(0), opt_acse_timeout(30),
    app( NULL ), cmd( NULL ), dataSource( dataSourcev )
{
  // Initialize application identification string.
  sprintf( rcsid, "$dcmtk: %s v%s %s $", applicationName, OFFIS_DCMTK_VERSION, OFFIS_DCMTK_RELEASEDATE );

  // Initialize starting values for variables pertaining to program options.
  opt_dfPath = "/home/www/wlist";

#ifdef HAVE_FORK
  opt_singleProcess = OFFalse;
#else
  opt_singleProcess = OFTrue;
#endif

  // Initialize program options and parameters.
  char tempstr[20];
  app = new OFConsoleApplication( applicationName , "DICOM Basic Worklist Management SCP (based on data files)", rcsid );

  cmd = new OFCommandLine();

  cmd->setParamColumn(LONGCOL+SHORTCOL+4);
  cmd->addParam("port", "tcp/ip port number to listen on");

  cmd->setOptionColumns(LONGCOL, SHORTCOL);
  cmd->addGroup("general options:", LONGCOL, SHORTCOL+2);
    cmd->addOption("--help",                      "-h",        "print this help text and exit");
    cmd->addOption("--version",                                "print version information and exit", OFTrue /* exclusive */);
    cmd->addOption("--verbose",                   "-v",        "verbose mode, print processing details");
    cmd->addOption("--debug",                     "-d",        "debug mode, print debug information");
#ifdef HAVE_FORK
    cmd->addOption("--single-process",            "-s",        "single process mode");
#endif
    cmd->addOption("--no-sq-expansion",           "-nse",        "disable expansion of empty sequences\nin C-FIND request messages");
    OFString opt5 = "path to worklist data files\n(default: ";
    opt5 += opt_dfPath;
    opt5 += ")";
    cmd->addOption("--data-files-path",           "-dfp",    1, "[p]ath: string", opt5.c_str() );
    cmd->addOption("--enable-file-reject",        "-efr",       "enable rejection of incomplete worklist-files\n(default)");
    cmd->addOption("--disable-file-reject",       "-dfr",       "disable rejection of incomplete worklist-files");

  cmd->addGroup("returned character set options:", LONGCOL, SHORTCOL+2);
    cmd->addOption("--return-no-char-set",        "-cs0",       "return no specific character set (default)");
    cmd->addOption("--return-iso-ir-100",         "-cs1",       "return specific character set ISO IR 100");

  cmd->addGroup("network options:");
    cmd->addSubGroup("preferred network transfer syntaxes:");
      cmd->addOption("--prefer-uncompr",        "+x=",       "prefer explicit VR local byte order (default)");
      cmd->addOption("--prefer-little",         "+xe",       "prefer explicit VR little endian TS");
      cmd->addOption("--prefer-big",            "+xb",       "prefer explicit VR big endian TS");
      cmd->addOption("--implicit",              "+xi",       "accept implicit VR little endian TS only");

#ifdef WITH_TCPWRAPPER
    cmd->addSubGroup("network host access control (tcp wrapper) options:");
      cmd->addOption("--access-full",            "-ac",       "accept connections from any host (default)");
      cmd->addOption("--access-control",         "+ac",       "enforce host access control rules");
#endif

    cmd->addSubGroup("other network options:");
      cmd->addOption("--acse-timeout",           "-ta", 1, "[s]econds: integer (default: 30)", "timeout for ACSE messages");
      cmd->addOption("--dimse-timeout",          "-td", 1, "[s]econds: integer (default: unlimited)", "timeout for DIMSE messages");

      OFString opt6 = "[a]ssocs: integer (default: ";
      sprintf(tempstr, "%ld", (long)opt_maxAssociations);
      opt6 += tempstr;
      opt6 += ")";
      cmd->addOption("--max-associations",                1, opt6.c_str(), "limit maximum number of parallel associations");
      cmd->addOption("--refuse",                             "refuse association");
      cmd->addOption("--reject",                             "reject association if no implement. class UID");
      cmd->addOption("--no-fail",                            "don't fail on an invalid query");
      cmd->addOption("--sleep-after",                     1, "[s]econds: integer", "sleep s seconds after find (default: 0)");
      cmd->addOption("--sleep-during",                    1, "[s]econds: integer", "sleep s seconds during find (default: 0)");
      OFString opt3 = "set max receive pdu to n bytes (default: ";
      sprintf(tempstr, "%ld", (long)ASC_DEFAULTMAXPDU);
      opt3 += tempstr;
      opt3 += ")";
      OFString opt4 = "[n]umber of bytes: integer [";
      sprintf(tempstr, "%ld", (long)ASC_MINIMUMPDUSIZE);
      opt4 += tempstr;
      opt4 += "..";
      sprintf(tempstr, "%ld", (long)ASC_MAXIMUMPDUSIZE);
      opt4 += tempstr;
      opt4 += "]";
      cmd->addOption("--max-pdu",                "-pdu",   1,  opt4.c_str(), opt3.c_str());
      cmd->addOption("--disable-host-lookup",    "-dhl",      "disable hostname lookup");
    cmd->addSubGroup("group length encoding (when sending C-FIND response data):");
      cmd->addOption("--group-length-recalc",    "+g=",       "recalculate group lengths if present (default)");
      cmd->addOption("--group-length-create",    "+g",        "always write with group length elements");
      cmd->addOption("--group-length-remove",    "-g",        "always write without group length elements");
    cmd->addSubGroup("length encoding in sequences and items (when sending C-FIND response data):");
      cmd->addOption("--length-explicit",        "+e",        "write with explicit lengths (default)");
      cmd->addOption("--length-undefined",       "-e",        "write with undefined lengths");

  cmd->addGroup("encoding options:");
    cmd->addSubGroup("post-1993 value representations:");
      cmd->addOption("--enable-new-vr",          "+u",        "enable support for new VRs (UN/UT) (default)");
      cmd->addOption("--disable-new-vr",         "-u",        "disable support for new VRs, convert to OB");

  // Evaluate command line.
  prepareCmdLineArgs( argc, argv, applicationName );
  if( app->parseCommandLine( *cmd, argc, argv, OFCommandLine::ExpandWildcards ) )
  {
    /* check exclusive options first */
    if (cmd->getParamCount() == 0)
    {
      if (cmd->findOption("--version"))
      {
        app->printHeader(OFTrue /*print host identifier*/);          // uses ofConsole.lockCerr()
        CERR << endl << "External libraries used:";
#ifdef WITH_ZLIB
        CERR << endl << "- ZLIB, Version " << zlibVersion() << endl;
#else
        CERR << " none" << endl;
#endif
        exit(0);
      }
    }
    /* command line parameters and options */
    app->checkParam(cmd->getParamAndCheckMinMax(1, opt_port, 1, 65535));
    if( cmd->findOption("--verbose") ) opt_verbose = OFTrue;
    if( cmd->findOption("--debug") )
    {
      opt_debug = OFTrue;
      DUL_Debug(OFTrue);
      DIMSE_debug(OFTrue);
      SetDebugLevel(3);
    }
#ifdef HAVE_FORK
    if( cmd->findOption("--single-process") ) opt_singleProcess = OFTrue;
#endif
    if( cmd->findOption("--no-sq-expansion") ) opt_noSequenceExpansion = OFTrue;
    if( cmd->findOption("--data-files-path") ) app->checkValue(cmd->getValue(opt_dfPath));
    cmd->beginOptionBlock();
    if( cmd->findOption("--enable-file-reject") )  opt_enableRejectionOfIncompleteWlFiles = OFTrue;
    if( cmd->findOption("--disable-file-reject") )  opt_enableRejectionOfIncompleteWlFiles = OFFalse;
    cmd->endOptionBlock();
    cmd->beginOptionBlock();
    if( cmd->findOption("--return-no-char-set") )  opt_returnedCharacterSet = RETURN_NO_CHARACTER_SET;
    if( cmd->findOption("--return-iso-ir-100") )  opt_returnedCharacterSet = RETURN_CHARACTER_SET_ISO_IR_100;
    cmd->endOptionBlock();
    cmd->beginOptionBlock();
    if( cmd->findOption("--prefer-uncompr") )  opt_networkTransferSyntax = EXS_Unknown;
    if( cmd->findOption("--prefer-little") )   opt_networkTransferSyntax = EXS_LittleEndianExplicit;
    if( cmd->findOption("--prefer-big") )      opt_networkTransferSyntax = EXS_BigEndianExplicit;
    if( cmd->findOption("--implicit") )        opt_networkTransferSyntax = EXS_LittleEndianImplicit;
    cmd->endOptionBlock();
#ifdef WITH_TCPWRAPPER
    cmd->beginOptionBlock();
    if (cmd->findOption("--access-full")) dcmTCPWrapperDaemonName.set(NULL);
    if (cmd->findOption("--access-control")) dcmTCPWrapperDaemonName.set(applicationName);
    cmd->endOptionBlock();
#endif

    if (cmd->findOption("--acse-timeout"))
    {
      OFCmdSignedInt opt_timeout = 0;
      app->checkValue(cmd->getValueAndCheckMin(opt_timeout, 1));
      opt_acse_timeout = OFstatic_cast(int, opt_timeout);
    }

    if (cmd->findOption("--dimse-timeout"))
    {
      OFCmdSignedInt opt_timeout = 0;
      app->checkValue(cmd->getValueAndCheckMin(opt_timeout, 1));
      opt_dimse_timeout = OFstatic_cast(int, opt_timeout);
      opt_blockMode = DIMSE_NONBLOCKING;
    }

    if( cmd->findOption("--max-associations") ) 
    {
        OFCmdSignedInt maxAssoc = 1;
        app->checkValue(cmd->getValueAndCheckMin(maxAssoc, 1));
        opt_maxAssociations = OFstatic_cast(int, maxAssoc);
    }
    if( cmd->findOption("--refuse") ) opt_refuseAssociation = OFTrue;
    if( cmd->findOption("--reject") ) opt_rejectWithoutImplementationUID = OFTrue;
    if( cmd->findOption("--no-fail") ) opt_failInvalidQuery = OFFalse;
    if( cmd->findOption("--sleep-after") ) app->checkValue(cmd->getValueAndCheckMin(opt_sleepAfterFind, 0));
    if( cmd->findOption("--sleep-during") ) app->checkValue(cmd->getValueAndCheckMin(opt_sleepDuringFind, 0));
    if( cmd->findOption("--max-pdu") ) app->checkValue(cmd->getValueAndCheckMinMax(opt_maxPDU, ASC_MINIMUMPDUSIZE, ASC_MAXIMUMPDUSIZE));
    if( cmd->findOption("--disable-host-lookup") ) dcmDisableGethostbyaddr.set(OFTrue);
    cmd->beginOptionBlock();
    if( cmd->findOption("--enable-new-vr") )
    {
      dcmEnableUnknownVRGeneration.set(OFTrue);
      dcmEnableUnlimitedTextVRGeneration.set(OFTrue);
    }
    if( cmd->findOption("--disable-new-vr") )
    {
      dcmEnableUnknownVRGeneration.set(OFFalse);
      dcmEnableUnlimitedTextVRGeneration.set(OFFalse);
    }
    cmd->endOptionBlock();
  }

  // dump application information
  DumpMessage( rcsid );
  DumpMessage( "" );

  // set general parameters in data source object
  dataSource->SetLogStream( &ofConsole );
  dataSource->SetVerbose( opt_verbose );
  dataSource->SetDebug( opt_debug );
  dataSource->SetNoSequenceExpansion( opt_noSequenceExpansion );
  dataSource->SetReturnedCharacterSet( opt_returnedCharacterSet );

  // set specific parameters in data source object
  dataSource->SetDfPath( opt_dfPath );
  dataSource->SetEnableRejectionOfIncompleteWlFiles( opt_enableRejectionOfIncompleteWlFiles );
}

// ----------------------------------------------------------------------------

WlmConsoleEngineFileSystem::~WlmConsoleEngineFileSystem()
// Date         : December 17, 2001
// Author       : Thomas Wilkens
// Task         : Destructor.
// Parameters   : none.
// Return Value : none.
{
  delete cmd;
  delete app;
}

// ----------------------------------------------------------------------------

int WlmConsoleEngineFileSystem::StartProvidingService()
// Date         : December 17, 2001
// Author       : Thomas Wilkens
// Task         : Starts providing the implemented service for calling SCUs.
//                After having created an instance of this class, this function
//                shall be called in order to start the SCP.
// Parameters   : none.
// Return Value : Indicator that shows if the function was executed successfully or not.
{
  OFCondition cond;

  // connect to data source
  cond = dataSource->ConnectToDataSource();
  if( cond.bad() )
  {
    // in case something unexpected happened, dump a corresponding message
    DumpMessage( cond.text() );

    // return error
    return( 1 );
  }

  // start providing the basic worklist management service
  WlmActivityManager *activityManager = new WlmActivityManager( 
      dataSource, opt_port,
      opt_refuseAssociation,
      opt_rejectWithoutImplementationUID,
      opt_sleepAfterFind, opt_sleepDuringFind,
      opt_maxPDU, opt_networkTransferSyntax,
      opt_verbose, opt_debug, opt_failInvalidQuery,
      opt_singleProcess, opt_maxAssociations,
      opt_blockMode, opt_dimse_timeout, opt_acse_timeout,
      &ofConsole );
  cond = activityManager->StartProvidingService();
  if( cond.bad() )
  {
    // in case something unexpected happened, dump a corresponding message
    DumpMessage( cond.text() );

    // disconnect from data source
    dataSource->DisconnectFromDataSource();

    // free memory
    delete activityManager;

    // return error
    return( 1 );
  }

  // free memory
  delete activityManager;

  // disconnect from data source
  cond = dataSource->DisconnectFromDataSource();
  if( cond.bad() )
  {
    // in case something unexpected happened, dump a corresponding message
    DumpMessage( cond.text() );

    // return error
    return( 1 );
  }

  // return result
  return( 0 );
}

// ----------------------------------------------------------------------------

void WlmConsoleEngineFileSystem::DumpMessage( const char *message )
// Date         : August 6, 2002
// Author       : Thomas Wilkens
// Task         : This function dumps the given (runtime) information on the out stream.
//                Used for dumping information in normal, debug and verbose mode.
// Parameters   : message - [in] The message to dump.
// Return Value : none.
{
  if( message != NULL )
  {
    ofConsole.lockCout();
    ofConsole.getCout() << message << endl;
    ofConsole.unlockCout();
  }
}

// ----------------------------------------------------------------------------

/*
** CVS Log
** $Log: wlcefs.cc,v $
** Revision 1.10  2005/12/08 15:48:30  meichel
** Changed include path schema for all DCMTK header files
**
** Revision 1.9  2005/11/17 13:45:34  meichel
** Added command line options for DIMSE and ACSE timeouts
**
** Revision 1.8  2005/05/04 11:33:47  wilkens
** Added two command line options --enable-file-reject (default) and
** --disable-file-reject to wlmscpfs: these options can be used to enable or
** disable a file rejection mechanism which makes sure only complete worklist files
** will be used during the matching process. A worklist file is considered to be
** complete if it contains all necessary type 1 information which the SCP might
** have to return to an SCU in a C-Find response message.
**
** Revision 1.7  2004/02/24 14:45:47  meichel
** Added max-associations command line option, changed default to 50.
**
** Revision 1.6  2003/06/10 13:54:35  meichel
** Added support for TCP wrappers based host access control
**
** Revision 1.5  2003/02/17 12:02:00  wilkens
** Made some minor modifications to be able to modify a special variant of the
** worklist SCP implementation (wlmscpki).
**
** Revision 1.4  2002/11/26 08:53:00  meichel
** Replaced all includes for "zlib.h" with <zlib.h>
**   to avoid inclusion of zlib.h in the makefile dependencies.
**
** Revision 1.3  2002/09/23 18:36:58  joergr
** Added new command line option "--version" which prints the name and version
** number of external libraries used (incl. preparation for future support of
** 'config.guess' host identifiers).
**
** Revision 1.2  2002/08/12 10:55:47  wilkens
** Made some modifications in in order to be able to create a new application
** which contains both wlmscpdb and ppsscpdb and another application which
** contains both wlmscpfs and ppsscpfs.
**
** Revision 1.1  2002/08/05 09:09:17  wilkens
** Modfified the project's structure in order to be able to create a new
** application which contains both wlmscpdb and ppsscpdb.
**
** Revision 1.7  2002/07/17 13:10:21  wilkens
** Corrected some minor logical errors in the wlmscpdb sources and completely
** updated the wlmscpfs so that it does not use the original wlistctn sources
** any more but standard wlm sources which are now used by all three variants
** of wlmscps.
**
** Revision 1.6  2002/06/10 11:24:55  wilkens
** Made some corrections to keep gcc 2.95.3 quiet.
**
** Revision 1.5  2002/05/08 13:20:40  wilkens
** Added new command line option -nse to wlmscpki and wlmscpdb.
**
** Revision 1.4  2002/04/18 14:19:55  wilkens
** Modified Makefiles. Updated latest changes again. These are the latest
** sources. Added configure file.
**
** Revision 1.3  2002/01/08 17:44:44  joergr
** Reformatted source files (replaced Windows newlines by Unix ones, replaced
** tabulator characters by spaces, etc.)
**
** Revision 1.2  2002/01/08 17:14:51  joergr
** Reworked database support after trials at the hospital (modfied by MC/JR on
** 2002-01-08).
**
**
*/
