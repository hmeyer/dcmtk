/*
 *
 *  Copyright (C) 1993-2005, OFFIS
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
 *  Module:  dcmqrdb
 *
 *  Author:  Andrew Hewett
 *
 *  Purpose: Telnet Initiator (ti) Main Program
 *
 *  Last Update:      $Author: meichel $
 *  Update Date:      $Date: 2005/12/08 15:47:03 $
 *  Source File:      $Source: /share/dicom/cvs-depot/dcmtk/dcmqrdb/apps/dcmqrti.cc,v $
 *  CVS/RCS Revision: $Revision: 1.5 $
 *  Status:           $State: Exp $
 *
 *  CVS/RCS Log at end of file
 *
 */

#include "dcmtk/config/osconfig.h"

#define INCLUDE_CSTDLIB
#define INCLUDE_CSTDIO
#define INCLUDE_CSTRING
#define INCLUDE_CSTDARG
#define INCLUDE_CERRNO
#define INCLUDE_CTIME
#define INCLUDE_CSIGNAL
#include "dcmtk/ofstd/ofstdinc.h"
BEGIN_EXTERN_C
#ifdef HAVE_SYS_FILE_H
#include <sys/file.h>
#endif
END_EXTERN_C
#include "dcmtk/dcmqrdb/dcmqrtis.h"
#include "dcmtk/dcmnet/assoc.h"
#include "dcmtk/dcmnet/dimse.h"
#include "dcmtk/ofstd/ofcmdln.h"
#include "dcmtk/dcmqrdb/dcmqrcnf.h"
#include "dcmtk/dcmdata/dcdict.h"
#include "dcmtk/dcmdata/cmdlnarg.h"
#include "dcmtk/dcmdata/dcdebug.h"
#include "dcmtk/dcmdata/dcuid.h"
#include "dcmtk/ofstd/ofconapp.h"
#ifdef WITH_ZLIB
#include <zlib.h>          /* for zlibVersion() */
#endif

#define OFFIS_CONSOLE_APPLICATION "dcmqrti"
#define MAXREMOTEDBTITLES 20
#define APPLICATIONTITLE "TELNET_INITIATOR"
#define SHORTCOL 4
#define LONGCOL 21

static char rcsid[] = "$dcmtk: " OFFIS_CONSOLE_APPLICATION " v" OFFIS_DCMTK_VERSION " " OFFIS_DCMTK_RELEASEDATE " $";
DcmQueryRetrieveConfig config;
DcmQueryRetrieveTelnetInitiator conf(config);

/*
 * Handle interrupt signals.
 * We only really need to make sure that the display is clear
 * before quiting.
 */
#ifdef SIGNAL_HANDLER_WITH_ELLIPSE
extern "C" void TI_signalHandler(...)
#else
extern "C" void TI_signalHandler(int)
#endif
{
  conf.TI_detachAssociation(OFTrue);
  exit( 1 );
}

int main( int argc, char *argv[] )
{

  OFBool verbose = OFFalse;
  OFBool debug = OFFalse;
  const char *remoteDBTitles[ MAXREMOTEDBTITLES ];
  int remoteDBTitlesCount = 0;
  const char *configFileName = "dcmqrdb.cfg";
  E_TransferSyntax networkTransferSyntax = EXS_Unknown;
  int opt_acse_timeout = 30;

  const char *currentPeer = NULL, **vendorHosts, **aeTitleList;
  OFBool noCommandLineValueForMaxReceivePDULength = OFTrue;
  int peerCount, j, n, returnValue = 0;
  OFCondition cond = EC_Normal;
  char tempstr[20];

#ifdef HAVE_GUSI_H
  // needed for Macintosh
  GUSISetup( GUSIwithSIOUXSockets );
  GUSISetup( GUSIwithInternetSockets );
#endif

#ifdef HAVE_WINSOCK_H
  WSAData winSockData;
  // we need at least version 1.1
  WORD winSockVersionNeeded = MAKEWORD( 1, 1 );
  WSAStartup( winSockVersionNeeded, &winSockData );
#endif

  // initialize conf structure
  conf.setAETitle(APPLICATIONTITLE);

  // setup console application and command line objects
  OFConsoleApplication app( OFFIS_CONSOLE_APPLICATION , "Telnet initiator", rcsid );
  OFCommandLine cmd;

  cmd.setParamColumn( LONGCOL + SHORTCOL + 2 );
  cmd.addParam( "peer", "peer host name or symbolic name from cfg file", OFCmdParam::PM_MultiMandatory );

  cmd.setOptionColumns( LONGCOL, SHORTCOL );
  cmd.addGroup( "general options:");
    cmd.addOption( "--help",                      "-h",      "print this help text and exit" );
    cmd.addOption( "--version",                              "print version information and exit", OFTrue );
    cmd.addOption( "--verbose",                   "-v",      "verbose mode, print processing details" );
    cmd.addOption( "--debug",                     "-d",      "debug mode, print debug information" );
    OFString opt0 = "use configuration file f (default: ";
    opt0 += configFileName;
    opt0 += ")";
    cmd.addOption( "--config",                    "-c",   1, "[f]ilename: string", opt0.c_str() );

  cmd.addGroup( "network options:" );
    cmd.addOption( "--timeout",                   "-to",  1, "[s]econds: integer (default: unlimited)", "timeout for connection requests");
      cmd.addOption("--acse-timeout",  "-ta", 1, "[s]econds: integer (default: 30)", "timeout for ACSE messages");
      cmd.addOption("--dimse-timeout", "-td", 1, "[s]econds: integer (default: unlimited)", "timeout for DIMSE messages");

    cmd.addOption( "--propose-implicit",          "-xi",     "propose implicit VR little endian TS only" );
    OFString opt1 = "set my AE title (default: ";
    opt1 += APPLICATIONTITLE;
    opt1 += ")";
    cmd.addOption( "--aetitle",                   "-aet", 1, "aetitle: string", opt1.c_str() );
    OFString opt2 = "[n]umber of bytes: integer [";
    sprintf(tempstr, "%ld", (long)ASC_MINIMUMPDUSIZE);
    opt2 += tempstr;
    opt2 += "..";
    sprintf(tempstr, "%ld", (long)ASC_MAXIMUMPDUSIZE);
    opt2 += tempstr;
    opt2 += "]";
    cmd.addOption( "--max-pdu",                   "-pdu", 1,  opt2.c_str(), "set max receive pdu to n bytes\n(default: use value from configuration file)" );

  cmd.addGroup( "other options:" );
    cmd.addOption( "--disable-new-vr",            "-u",       "disable support for new VRs, convert to OB" );
    cmd.addOption( "--remote",                    "-rmt", 1,  "[t]itle: string", "connect to remote database defined in cfg file" );

  // evaluate command line
  prepareCmdLineArgs( argc, argv, OFFIS_CONSOLE_APPLICATION );
  if( app.parseCommandLine( cmd, argc, argv, OFCommandLine::ExpandWildcards ) )
  {
    // check exclusive options first
    if( cmd.getParamCount() == 0 )
    {
      if( cmd.findOption("--version") )
      {
        app.printHeader( OFTrue );
        CERR << endl << "External libraries used:";
#if !defined(WITH_ZLIB)
        CERR << " none" << endl;
#else
        CERR << endl;
#endif
#ifdef WITH_ZLIB
        CERR << "- ZLIB, Version " << zlibVersion() << endl;
#endif
        return( 0 );
      }
    }

    // command line parameters
    if( cmd.findOption("--verbose") ) verbose = OFTrue;
    if( cmd.findOption("--debug") )
    {
      debug = OFTrue;
      verbose = OFTrue;
      DUL_Debug(OFTrue);
      DIMSE_debug(OFTrue);
      SetDebugLevel(3);
    }
    if( cmd.findOption("--config") ) app.checkValue( cmd.getValue( configFileName ) );
    if( cmd.findOption("--propose-implicit") ) networkTransferSyntax = EXS_LittleEndianImplicit;

    conf.setDebug(verbose, debug);
    conf.setXferSyntax(networkTransferSyntax);
    
    const char *myAE = NULL;
    if( cmd.findOption("--aetitle") ) 
    {
    	app.checkValue( cmd.getValue( myAE ) );
        conf.setAETitle(myAE);
    }
    if( cmd.findOption("--max-pdu") )
    {
      OFCmdUnsignedInt pdu=0;
      app.checkValue( cmd.getValueAndCheckMinMax( pdu, ASC_MINIMUMPDUSIZE, ASC_MAXIMUMPDUSIZE ) );
      conf.setMaxPDU(pdu);
      noCommandLineValueForMaxReceivePDULength = OFFalse;
    }

    if (cmd.findOption("--timeout"))
    {
      OFCmdSignedInt opt_timeout = 0;
      app.checkValue(cmd.getValueAndCheckMin(opt_timeout, 1));
      dcmConnectionTimeout.set((Sint32) opt_timeout);
    }

    if (cmd.findOption("--acse-timeout"))
    {
      OFCmdSignedInt opt_timeout = 0;
      app.checkValue(cmd.getValueAndCheckMin(opt_timeout, 1));
      opt_acse_timeout = OFstatic_cast(int, opt_timeout);
    }

    if (cmd.findOption("--dimse-timeout"))
    {
      OFCmdSignedInt opt_timeout = 0;
      app.checkValue(cmd.getValueAndCheckMin(opt_timeout, 1));
      conf.setBlockMode(DIMSE_NONBLOCKING, OFstatic_cast(int, opt_timeout));
    }

    if( cmd.findOption("--disable-new-vr") )
    {
      dcmEnableUnknownVRGeneration.set( OFFalse );
      dcmEnableUnlimitedTextVRGeneration.set( OFFalse );
    }

    if (cmd.findOption("--remote", 0, OFCommandLine::FOM_First))
    {
      do
      {
        if( remoteDBTitlesCount < MAXREMOTEDBTITLES )
        {
          app.checkValue( cmd.getValue( remoteDBTitles[remoteDBTitlesCount] ) );
          remoteDBTitlesCount++;
        }
        else CERR << "ti: Too many remote database titles." << endl;
      } while (cmd.findOption("--remote", 0, OFCommandLine::FOM_Next));
    }

  }

  // in case accessing the configuration file for reading is successful
  if( access( configFileName, R_OK ) != -1 )
  {
    // in case reading values from configuration file is successful
    if( config.init( configFileName ) == 1 )
    {
      // dump information
      if( verbose )
        config.printConfig();

      // determine max pdu size from configuration file
      OFCmdUnsignedInt maxPDU = config.getMaxPDUSize();

      // in case the max pdu size was not set in the configuration file, or
      // in case its value is not in a certain range, use the default value
      if( maxPDU == 0 || maxPDU < ASC_MINIMUMPDUSIZE || maxPDU > ASC_MAXIMUMPDUSIZE )
      {
        COUT << "ti: no or invalid max pdu size found in configuration file." << endl;
        maxPDU = ASC_DEFAULTMAXPDU;
      }

      // if no max pdu size was set on the command line then use config file value
      if( noCommandLineValueForMaxReceivePDULength ) conf.setMaxPDU(maxPDU);

      // go through all peers that were specified on the command line
      peerCount = cmd.getParamCount();
      for( int i=1 ; i<=peerCount ; i++ )
      {
        // determine current peer
        cmd.getParam( i, currentPeer );

        // in general, we now want to add host names to the conf structure; it might be
        // though that currentPeer is a symbolic name that stands for a number of hosts;
        // hence we need to check first if peer can stands for a symbolic name
        if( ( n = config.aeTitlesForSymbolicName( currentPeer, &aeTitleList ) ) > 0 )
        {
          // in case peer is a symbolic name and can be found in the host table,
          // determine corresponding host names and add them to conf structure
          const char *peerName = NULL;
          int portNumber;
          for( j=0 ; j<n ; j++ )
          {
            if( config.peerForAETitle( aeTitleList[j], &peerName, &portNumber ) )
              conf.addPeerName(peerName, configFileName);
          }

          // free memory
          if( aeTitleList )
            free( aeTitleList );
          aeTitleList = NULL;
        }
        else if( ( n = config.HostNamesForVendor( currentPeer, &vendorHosts ) ) > 0 )
        {
          // in case peer is a symbolic name and can be interpreted as a vendor name, add the
          // corresponding host names are known for for this vendor to the conf structure
          for( j=0 ; j<n ; j++ )
            conf.addPeerName(vendorHosts[j], configFileName);

          // free memory
          if( vendorHosts )
            free( vendorHosts );
          vendorHosts = NULL;
        }
        else
        {
          // in case peer is not a symbolic name but the name of a
          // specific host, add this host name to the conf structure
          conf.addPeerName(currentPeer, configFileName);
        }
      }

      // set "peer to talk to" to the first host
      // name in the array (this is the default)
      conf.activateFirstPeer();

      // load up configuration info
      conf.createConfigEntries(configFileName, remoteDBTitlesCount, remoteDBTitles);

      // only go ahead in case there is at least one database we know of
      if( conf.getdbCount() > 0 )
      {
        // dump information
        if( verbose )
          conf.printConfig();

        // make sure data dictionary is loaded
        if( !dcmDataDict.isDictionaryLoaded() )
          CERR << "Warning: no data dictionary loaded, check environment variable: " << DCM_DICT_ENVIRONMENT_VARIABLE << endl;

        // if starting up network is successful
        cond = ASC_initializeNetwork( NET_REQUESTOR, 0, opt_acse_timeout, conf.accessNet() );
        if( cond.good() )
        {
          // set interrupts for signal handling
#ifdef SIGHUP
          signal( SIGHUP, TI_signalHandler );
#endif
#ifdef SIGINT
          signal( SIGINT, TI_signalHandler );
#endif
#ifdef SIGQUIT
          signal( SIGQUIT, TI_signalHandler );
#endif
#ifdef SIGTERM
          signal( SIGTERM, TI_signalHandler );
#endif

          // do the real work
          conf.TI_userInput();

          // clean up network
          cond = ASC_dropNetwork( conf.accessNet() );
          if( cond.bad() )
          {
            CERR << "ti: error dropping network: ";
            DimseCondition::dump( cond );
            returnValue = 1;
          }
        }
        else
        {
          CERR << "ti: error initialising network: ";
          DimseCondition::dump( cond );
          returnValue = 1;
        }
      }
      else
      {
        CERR << "ti: no accessable databases." << endl;
        returnValue = 1;
      }
    }
    else
    {
      CERR << "ti: error while reading configuration file '" << configFileName << "'." << endl;
      returnValue = 1;
    }
  }
  else
  {
    CERR << "ti: cannot access configuration file '" << configFileName << "'." << endl;
    returnValue = 1;
  }

#ifdef HAVE_WINSOCK_H
  WSACleanup();
#endif

  // return result
  return( returnValue );
}

//-------------------------------------------------------------------------------------------------


/*
 * CVS Log
 * $Log: dcmqrti.cc,v $
 * Revision 1.5  2005/12/08 15:47:03  meichel
 * Changed include path schema for all DCMTK header files
 *
 * Revision 1.4  2005/11/17 13:44:59  meichel
 * Added command line options for DIMSE and ACSE timeouts
 *
 * Revision 1.3  2005/11/16 14:59:00  meichel
 * Set association timeout in ASC_initializeNetwork to 30 seconds. This improves
 *   the responsiveness of the tools if the peer blocks during assoc negotiation.
 *
 * Revision 1.2  2005/06/16 08:05:48  meichel
 * Fixed typo in method name
 *
 * Revision 1.1  2005/03/30 13:34:44  meichel
 * Initial release of module dcmqrdb that will replace module imagectn.
 *   It provides a clear interface between the Q/R DICOM front-end and the
 *   database back-end. The imagectn code has been re-factored into a minimal
 *   class structure.
 *
 *
 */
