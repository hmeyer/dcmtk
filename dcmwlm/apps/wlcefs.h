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
 *  Update Date:      $Date: 2005/12/08 15:46:50 $
 *  Source File:      $Source: /share/dicom/cvs-depot/dcmtk/dcmwlm/apps/wlcefs.h,v $
 *  CVS/RCS Revision: $Revision: 1.7 $
 *  Status:           $State: Exp $
 *
 *  CVS/RCS Log at end of file
 *
 */

#ifndef WlmConsoleEngineFileSystem_h
#define WlmConsoleEngineFileSystem_h

#include "dcmtk/config/osconfig.h"
#include "dcmtk/dcmnet/dimse.h"

class WlmDataSource;
class OFConsoleApplication;
class OFCommandLine;

/** This class encapsulates data structures and operations for a console application that
 *  can act as a basic worklist management service class provider based on the file system.
 */
class WlmConsoleEngineFileSystem
{
  protected:
    /// contains application's id string
    char rcsid[200];
    /// returned character set type
    WlmReturnedCharacterSetType opt_returnedCharacterSet;
    /// path to database files
    const char *opt_dfPath;
    /// port on which this application is listening
    OFCmdUnsignedInt opt_port;
    /// indicates if incoming associations shall be refused or not
    OFBool opt_refuseAssociation;
    /// indicates if incoming associations shall be refused if no implementation class uid is specified
    OFBool opt_rejectWithoutImplementationUID;
    /// indicates how long the application shall sleep after a find
    OFCmdUnsignedInt opt_sleepAfterFind;
    /// indicates how long the application shall sleep during a find
    OFCmdUnsignedInt opt_sleepDuringFind;
    /// max PDU size
    OFCmdUnsignedInt opt_maxPDU;
    /// preferred network transfer syntax
    E_TransferSyntax opt_networkTransferSyntax;
    /// indicates if this application is run in verbose mode
    OFBool opt_verbose;
    /// indicates if this application is run in debug mode
    OFBool opt_debug;
    /// indicates if find shall fail on an invalid query or not
    OFBool opt_failInvalidQuery;
    /// indicates if this application is run in single process mode or not
    OFBool opt_singleProcess;
    /// indicates how many associations can be accepted at the same time
    int opt_maxAssociations;
    /// indicates if an expansion of empty sequences in C-Find RQ messages shall take place or not
    OFBool opt_noSequenceExpansion;
    /// indicates if wl-files which are lacking return type 1 attributes or information in such attributes shall be rejected or not
    OFBool opt_enableRejectionOfIncompleteWlFiles;
    /// blocking mode for DIMSE operations
    T_DIMSE_BlockingMode opt_blockMode;
    /// timeout for DIMSE operations
    int opt_dimse_timeout;
    /// timeout for ACSE operations
    int opt_acse_timeout;
    /// instance of console application class (for handling command line arguments)
    OFConsoleApplication *app;
    /// instance of command line class (for handling command line arguments)
    OFCommandLine *cmd;
    /// data source which shall be queried on incoming C-Find RQ messages
    WlmDataSource *dataSource;

      /** This function dumps the given information on a stream.
       *  Used for dumping information in normal, debug and verbose mode.
       *  @param message The message to dump.
       */
    void DumpMessage( const char *message );

      /** Protected undefined copy-constructor. Shall never be called.
       *  @param Src Source object.
       */
    WlmConsoleEngineFileSystem( const WlmConsoleEngineFileSystem &Src );

      /** Protected undefined operator=. Shall never be called.
       *  @param Src Source object.
       *  @return Reference to this.
       */
    WlmConsoleEngineFileSystem &operator=( const WlmConsoleEngineFileSystem &Src );


  public:
      /** constructor.
       *  @param argc            Number of arguments that were passed to main.
       *  @param argv            Arguments that were passed to main.
       *  @param applicationName The name of this application.
       *  @param dataSourcev     Object which provides access to the data source.
       */
    WlmConsoleEngineFileSystem( int argc, char *argv[], const char *applicationName, WlmDataSource *dataSourcev );

      /** destructor
       */
    ~WlmConsoleEngineFileSystem();

      /** Starts providing the implemented service for calling SCUs.
       *  After having created an instance of this class, this function
       *  shall be called from main.
       *  @return Return value that is supposed to be returned from main().
       */
    int StartProvidingService();
};

#endif
