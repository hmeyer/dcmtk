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
 *  Purpose: Activity manager class for basic worklist management service
 *           class provider engines.
 *
 *  Last Update:      $Author: meichel $
 *  Update Date:      $Date: 2005/12/08 16:05:43 $
 *  Source File:      $Source: /share/dicom/cvs-depot/dcmtk/dcmwlm/include/dcmtk/dcmwlm/wlmactmg.h,v $
 *  CVS/RCS Revision: $Revision: 1.13 $
 *  Status:           $State: Exp $
 *
 *  CVS/RCS Log at end of file
 *
 */

#ifndef WlmActivityManager_h
#define WlmActivityManager_h

#include "dcmtk/config/osconfig.h"

class WlmDataSource;
class OFCondition;
class OFConsole;

/** This class encapsulates data structures and operations for basic worklist management service
 *  class providers.
 */
class WlmActivityManager
{
  protected:
    /// data source connection object
    WlmDataSource *dataSource;
    /// port on which the application is listening
    OFCmdUnsignedInt opt_port;
    /// indicates if the application shall refuse any association
    OFBool opt_refuseAssociation;
    /// indicates if the application shall reject associations without implementation class uids
    OFBool opt_rejectWithoutImplementationUID;
    /// indicates how long the application shall sleep after a find
    OFCmdUnsignedInt opt_sleepAfterFind;
    /// indicates how long the application shall sleep during a find
    OFCmdUnsignedInt opt_sleepDuringFind;
    /// max pdu size
    OFCmdUnsignedInt opt_maxPDU;
    /// preferred network transfer syntax
    E_TransferSyntax opt_networkTransferSyntax;
    /// indicates if the application is run in verbose mode or not
    OFBool opt_verbose;
    /// indicates if the application is run in debug mode or not
    OFBool opt_debug;
    /// indicates if the application shall fail on an invalid C-Find RQ message
    OFBool opt_failInvalidQuery;
    /// indicates if the application is run in single process mode or not
    OFBool opt_singleProcess;
    /// maximum number of association for non-single process mode
    int opt_maxAssociations;
    /// blocking mode for DIMSE operations
    T_DIMSE_BlockingMode opt_blockMode;
    /// timeout for DIMSE operations
    int opt_dimse_timeout;
    /// timeout for ACSE operations
    int opt_acse_timeout;
    /// array of supported abstract syntaxes
    char **supportedAbstractSyntaxes;
    /// number of array fields
    int numberOfSupportedAbstractSyntaxes;
    /// stream log information will be dumped to
    OFConsole *logStream;
    /// table of processes for non-single process mode
    WlmProcessTableType processTable;

      /** This function dumps the given information on a stream. Used for dumping information in normal, debug and verbose mode.
       *  @param message The message to dump.
       */
    void DumpMessage( const char *message );

      /** This function takes care of receiving, negotiating and accepting/refusing an
       *  association request. Additionally, it handles the request the association
       *  requesting application transmits after a connection isd established.
       *  @param net Contains network parameters.
       */
    OFCondition WaitForAssociation( T_ASC_Network *net );

      /** This function takes care of removing items referring to (terminated) subprocess
       *  from the table which stores all subprocess information. Three different versions
       *  for three different platforms are implemented.
       */
    void CleanChildren();

      /** This function negotiates a presentation context which will be used by this application
       *  and the other DICOM appliation that requests an association.
       *  @param assoc The association (network connection to another DICOM application).
       */
    OFCondition NegotiateAssociation( T_ASC_Association *assoc );

      /** This function adds a process to the table that stores process information.
       *  @param pid   the process id of the sub-process which was just started.
       *  @param assoc The association (network connection to another DICOM application).
       */
    void AddProcessToTable( int pid, T_ASC_Association *assoc );

      /** This function counts all child processes which are still referenced in the process table.
       *  @return The current amount of child processes.
       */
    int CountChildProcesses();

      /** This function removes one particular item from the table which stores all subprocess
       *  information. The item which shall be deleted will be identified by its process id.
       *  @param pid process id.
       */
    void RemoveProcessFromTable( int pid );

      /** This function takes care of refusing an assocation request.
       *  @param assoc  The association (network connection to another DICOM application).
       *  @param reason The reason why the association request will be refused.
       */
    void RefuseAssociation( T_ASC_Association **assoc, WlmRefuseReasonType reason );

      /** This function takes care of handling the other DICOM application's request. After
       *  having accomplished all necessary steps, the association will be dropped and destroyed.
       *  @param assoc The association (network connection to another DICOM application).
       */
    void HandleAssociation( T_ASC_Association *assoc );

      /** This function takes care of handling the other DICOM application's request.
       *  @param assoc The association (network connection to another DICOM application).
       *  @return An OFCondition value 'cond' for which 'cond.bad()' will always be set
       *          indicating that either some kind of error occurred, or that the peer aborted
       *          the association (DUL_PEERABORTEDASSOCIATION), or that the peer requested the
       *          release of the association (DUL_PEERREQUESTEDRELEASE).
       */
    OFCondition ReceiveAndHandleCommands( T_ASC_Association *assoc );

      /** Having received a DIMSE C-ECHO-RQ message, this function takes care of sending a
       *  DIMSE C-ECHO-RSP message over the network connection.
       *  @param assoc  The association (network connection to another DICOM application).
       *  @param req    The DIMSE C-ECHO-RQ message that was received.
       *  @param presId The ID of the presentation context which was specified in the PDV
       *                which contained the DIMSE command.
       *  @return OFCondition value denoting success or error.
       */
    OFCondition HandleEchoSCP( T_ASC_Association *assoc, T_DIMSE_C_EchoRQ *req, T_ASC_PresentationContextID presId );

      /** This function processes a DIMSE C-FIND-RQ commmand that was
       *  received over the network connection.
       *  @param assoc   The association (network connection to another DICOM application).
       *  @param request The DIMSE C-FIND-RQ message that was received.
       *  @param presID  The ID of the presentation context which was specified in the PDV
       *                 which contained the DIMSE command.
       *  @return OFCondition value denoting success or error.
       */
    OFCondition HandleFindSCP( T_ASC_Association *assoc, T_DIMSE_C_FindRQ *request, T_ASC_PresentationContextID presID );

      /** Protected undefined copy-constructor. Shall never be called.
       *  @param Src Source object.
       */
    WlmActivityManager( const WlmActivityManager &Src );

      /** Protected undefined operator=. Shall never be called.
       *  @param Src Source object.
       *  @return Reference to this.
       */
    WlmActivityManager &operator=( const WlmActivityManager &Src );


  public:
      /** constructor.
       *  @param dataSourcev                         Pointer to the data source which shall be used.
       *  @param opt_portv                           The port on which the application is supposed to listen.
       *  @param opt_refuseAssociationv              Specifies if an association shall always be refused by the SCP.
       *  @param opt_rejectWithoutImplementationUIDv Specifies if the application shall reject an association if no implementation class UID is provided by the calling SCU.
       *  @param opt_sleepAfterFindv                 Specifies how many seconds the application is supposed to sleep after having handled a C-FIND-Rsp.
       *  @param opt_sleepDuringFindv                Specifies how many seconds the application is supposed to sleep during the handling of a C-FIND-Rsp.
       *  @param opt_maxPDUv                         Maximum length of a PDU that can be received in bytes.
       *  @param opt_networkTransferSyntaxv          Specifies the preferred network transfer syntaxes.
       *  @param opt_verbosev                        Specifies if the application shall print processing details or not.
       *  @param opt_debugv                          Specifies if the application shall print debug information.
       *  @param opt_failInvalidQueryv               Specifies if the application shall fail on an invalid query.
       *  @param opt_singleProcessv                  Specifies if the application shall run in a single process.
       *  @param opt_maxAssociationsv                Specifies many concurrent associations the application shall be able to handle.
       *  @param opt_blockModev                      Specifies the blocking mode for DIMSE operations
       *  @param opt_dimse_timeoutv                  Specifies the timeout for DIMSE operations
       *  @param opt_acse_timeoutv                   Specifies the timeout for ACSE operations
       *  @param logStreamv                          A stream information can be dumped to.
       */
    WlmActivityManager(
        WlmDataSource *dataSourcev, 
        OFCmdUnsignedInt opt_portv, 
        OFBool opt_refuseAssociationv, 
        OFBool opt_rejectWithoutImplementationUIDv, 
        OFCmdUnsignedInt opt_sleepAfterFindv, 
        OFCmdUnsignedInt opt_sleepDuringFindv, 
        OFCmdUnsignedInt opt_maxPDUv, 
        E_TransferSyntax opt_networkTransferSyntaxv, 
        OFBool opt_verbosev, 
        OFBool opt_debugv, 
        OFBool opt_failInvalidQueryv, 
        OFBool opt_singleProcessv, 
        int opt_maxAssociationsv, 
        T_DIMSE_BlockingMode opt_blockModev,
        int opt_dimse_timeoutv,
        int opt_acse_timeoutv,
        OFConsole *logStreamv );

      /** destructor
       */
    ~WlmActivityManager();

      /** Starts providing the implemented service for calling SCUs.
       *  After having created an instance of this class, this function
       *  shall be called from main.
       *  @return Value that is supposed to be returned from main().
       */
    OFCondition StartProvidingService();
};

#endif

/*
** CVS Log
** $Log: wlmactmg.h,v $
** Revision 1.13  2005/12/08 16:05:43  meichel
** Changed include path schema for all DCMTK header files
**
** Revision 1.12  2005/11/17 13:45:39  meichel
** Added command line options for DIMSE and ACSE timeouts
**
** Revision 1.11  2003/07/02 09:17:55  wilkens
** Updated documentation to get rid of doxygen warnings.
**
** Revision 1.10  2002/12/16 11:08:36  wilkens
** Added missing #include "osconfig.h" to certain files.
**
** Revision 1.9  2002/12/12 16:48:35  wilkens
** Added some code to avoid compiler warning (unreachable code) on Sun CC 2.0.1.
**
** Revision 1.8  2002/08/05 09:09:59  wilkens
** Modfified the project's structure in order to be able to create a new
** application which contains both wlmscpdb and ppsscpdb.
**
** Revision 1.7  2002/07/17 13:10:37  wilkens
** Corrected some minor logical errors in the wlmscpdb sources and completely
** updated the wlmscpfs so that it does not use the original wlistctn sources
** any more but standard wlm sources which are now used by all three variants
** of wlmscps.
**
** Revision 1.6  2002/06/10 11:25:06  wilkens
** Made some corrections to keep gcc 2.95.3 quiet.
**
** Revision 1.5  2002/04/18 14:20:09  wilkens
** Modified Makefiles. Updated latest changes again. These are the latest
** sources. Added configure file.
**
** Revision 1.4  2002/01/08 19:10:04  joergr
** Minor adaptations to keep the gcc compiler on Linux and Solaris happy.
** Currently only the "file version" of the worklist SCP is supported on
** Unix systems.
**
** Revision 1.3  2002/01/08 17:45:34  joergr
** Reformatted source files (replaced Windows newlines by Unix ones, replaced
** tabulator characters by spaces, etc.)
**
** Revision 1.2  2002/01/08 17:35:39  joergr
** Reworked database support after trials at the hospital (modfied by MC/JR on
** 2002-01-08).
**
**
*/
