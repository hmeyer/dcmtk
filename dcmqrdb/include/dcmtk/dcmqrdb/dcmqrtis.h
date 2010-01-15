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
 *  Purpose: TI Common Constants, Types, Globals and Functions
 *
 *  Last Update:      $Author: meichel $
 *  Update Date:      $Date: 2005/12/08 16:04:28 $
 *  Source File:      $Source: /share/dicom/cvs-depot/dcmtk/dcmqrdb/include/dcmtk/dcmqrdb/dcmqrtis.h,v $
 *  CVS/RCS Revision: $Revision: 1.4 $
 *  Status:           $State: Exp $
 *
 *  CVS/RCS Log at end of file
 *
 */

#ifndef DCMQRTIS_H
#define DCMQRTIS_H

#include "dcmtk/config/osconfig.h"    /* make sure OS specific configuration is included first */

#include "dcmtk/dcmnet/dicom.h"
#include "dcmtk/dcmnet/cond.h"
#include "dcmtk/dcmnet/assoc.h"
#include "dcmtk/dcmnet/dimse.h"
#include "dcmtk/ofstd/ofcmdln.h"
#include "dcmtk/dcmqrdb/dcmqrcnf.h"

#include <boost/shared_ptr.hpp>
#include <vector>
#include <string>


using namespace std;

class DcmQueryRetrieveDatabaseHandle;

/*
 * Constants
 */

#define TI_MAXPEERS       100
#define TI_MAXDATABASES   100
#define TI_MAXSTUDIES    1000
#define TI_MAXSERIES      500
#define TI_MAXIMAGES     1000

/*
 * Type definitions
 */
typedef vector< string > TI_String_List;

struct TI_ImageEntry
{
    DIC_UI  sopInstanceUID;
    DIC_IS  imageNumber;
    int   intImageNumber;
};

typedef vector< TI_ImageEntry > TI_ImageEntry_List;

struct TI_SeriesEntry
{
    DIC_UI  seriesInstanceUID;
    DIC_IS  seriesNumber;
    int   intSeriesNumber;
    DIC_CS  modality;
    OFString numInstances;
    OFString descr;
    OFString time;
    TI_ImageEntry_List images;

    time_t lastQueryTime; /* time we last queried db */
};

typedef vector< TI_SeriesEntry > TI_SeriesEntry_List;

struct TI_StudyEntry
{
    DIC_UI  studyInstanceUID;
    DIC_CS  studyID;
    OFString  studyDate;
    OFString  studyDescr;
    DIC_PN  patientsName;
    OFString  patientsDoB;
    OFString numInstances;
    DIC_LO  patientID;
    TI_SeriesEntry_List series;
    time_t lastQueryTime; /* time we last queried db */
};

typedef vector< TI_StudyEntry > TI_StudyEntry_List;

struct TI_DBEntry
{
  string title;
  TI_String_List peerTitles; /* the CTN AE Title associated with this DB */

    boost::shared_ptr<DcmQueryRetrieveDatabaseHandle> dbHandle;  /* handle to current db */

    TI_StudyEntry_List studies; /* array of study pointers */

    unsigned int currentStudyIdx; /* index of current study */
    unsigned int currentSeriesIdx;  /* index of current series in current study */
    unsigned int currentImageIdx; /* index of current image in current study */

    time_t lastQueryTime; /* time we last queried db */

    OFBool isRemoteDB;  /* true if DB is remote */
};

typedef vector< TI_DBEntry > TI_DBEntry_List;


struct TI_GenericCallbackStruct
{
    TI_DBEntry *db;
    TI_StudyEntry *study;
    TI_SeriesEntry *series;
};

typedef OFBool (*TI_GenericEntryCallbackFunction)(TI_GenericCallbackStruct &cbstruct, DcmDataset &reply);

/** this class provides the functionality of the telnet initiator application
 */
class DcmQueryRetrieveTelnetInitiator
{
public:
    typedef TI_DBEntry_List::iterator TI_DBEntry_it;

    /** constructor
     *  @param cfg configuration facility
     */
    DcmQueryRetrieveTelnetInitiator(DcmQueryRetrieveConfig &cfg);

    /** main entry point for console-based user interface
     */
    void TI_userInput();

    /** add remote peer to list of peers
     *  @param peerName name of peer
     *  @param configFileName name of configuration file from which peer was read
     */
    OFBool addPeerName(const string &peerName, const string &configFileName);

    /** print TI configuration to stdout 
     */
    void printConfig();

    /** detach current association
     *  @param abortFlag if true, abort association instead of releasing it 
     */
    OFBool TI_detachAssociation(OFBool abortFlag);
    
    /** set local aetitle
     *  @param ae aetitle
     */
    void setAETitle(const string &ae)
    {
      myAETitle = ae;
    }
    
    /** set max receive PDU
     *  @param pdu max receive PDU size
     */
    void setMaxPDU(OFCmdUnsignedInt pdu)
    {
      maxReceivePDULength = pdu;
    }

    /** activate first peer in list of peers
     */
    void activateFirstPeer()
    {
      peerHostName = peerNames[0];
    }
    
    /** provide read/write access to network structure maintained by this object.
     *  Yes, this is ugly.
     *  @return pointer to pointer to network structure
     */
    T_ASC_Network **accessNet()
    {
      return &net;
    }
    
    /// return number of databases
    int getdbCount() const
    {
      return dbEntries.size();
    }

    /** create configuration entries for remote databases
     *  @param configFileName name of configuration file
     *  @param remoteDBTitlesCount number of remote DB titles
     *  @param remoteDBTitles list of remote DB titles
     */
    void createConfigEntries(
      const string &configFileName,
      int remoteDBTitlesCount,
      const char **remoteDBTitles);

    /** set the network transfer syntax
     *  @param xfer new network transfer syntax
     */
    void setXferSyntax(E_TransferSyntax xfer) { networkTransferSyntax = xfer; }
   
    /** set verbose and debug mode
     *  @param is_verbose verbose mode flag
     *  @param is_debug debug mode flag
     */
    void setDebug (OFBool is_verbose, OFBool is_debug)
    {
      verbose = is_verbose;
      debug = is_debug;
    }

    /** set blocking mode and timeout for DIMSE operations
     *  @param blockMode blocking mode for DIMSE operations
     *  @param timeout timeout for DIMSE operations
     */
    void setBlockMode(T_DIMSE_BlockingMode blockMode, int timeout)
    {
      blockMode_ = blockMode;
      dimse_timeout_ = timeout;
    }
    
private:
    
    OFBool TI_attachAssociation();
    OFBool TI_changeAssociation();
    OFBool TI_sendEcho();
    OFBool TI_storeImage(char *sopClass, char *sopInstance, char * imgFile);
    OFBool TI_remoteFindQuery(
        TI_DBEntry &db, DcmDataset &query,
        TI_GenericEntryCallbackFunction callbackFunction,
        TI_GenericCallbackStruct &callbackData);
    OFBool TI_title(int arg);
    OFBool TI_attachDB(TI_DBEntry &db);
    OFBool TI_database(int arg);
    OFBool TI_filter( const string &cmdString );
    OFBool TI_echo(int arg);
    OFBool TI_quit();
    OFBool TI_actualizeStudies();
    OFBool TI_study(int arg);
    OFBool TI_actualizeSeries();
    OFBool TI_series(int arg);
    OFBool TI_actualizeImages();
    OFBool TI_image(int arg);
    OFBool TI_buildStudies(TI_DBEntry &db);
    OFBool TI_buildSeries(TI_DBEntry &db, TI_StudyEntry &study);
    OFBool TI_buildRemoteImages(TI_DBEntry &db, TI_StudyEntry &study, TI_SeriesEntry &series);
    OFBool TI_buildImages(TI_DBEntry &db, TI_StudyEntry &study, TI_SeriesEntry &series);
    OFBool TI_sendStudy(int arg);
    OFBool TI_sendSeries(int arg);
    OFBool TI_sendImage(int arg);
    OFBool TI_send(const string &cmdString);
    OFBool TI_shortHelp();
    OFBool TI_help();
    OFBool TI_buildRemoteStudies(TI_DBEntry &db);
    OFBool TI_buildRemoteSeries(TI_DBEntry &db, TI_StudyEntry &study);
    OFBool TI_dbReadable(const string &dbTitle);
    time_t TI_dbModifyTime(const string &dbTitle);
    OFCondition addPresentationContexts(T_ASC_Parameters &params);

    OFBool findDBPeerTitles(
      const string &configFileName,
      TI_DBEntry &dbEntry, 
      const string &peer);
    
    /// the CTN databases we know
    TI_DBEntry_List dbEntries;

    /// current patient filter
    string patientFilter;
    
    /// current peer to talk to
    string peerHostName;
    
    /// list of peer names
    TI_String_List peerNames;


    /// my application entity title
    string myAETitle;

    /// active network
    T_ASC_Network *net;

    /// currently active association
    T_ASC_Association *assoc;

    /// number of bytes per PDU we can receive
    OFCmdUnsignedInt maxReceivePDULength;

    /// current database index
    unsigned int currentdbIdx;
    
    /// current peer title
    string currentPeerTitle;

    /// configuration facility
    DcmQueryRetrieveConfig& config;

    /// network transfer syntax
    E_TransferSyntax networkTransferSyntax;

    /// verbose mode flag
    OFBool verbose;

    /// debug mode flag
    OFBool debug;

    /// blocking mode for DIMSE operations
    T_DIMSE_BlockingMode blockMode_;
    
    /// timeout for DIMSE operations
    int dimse_timeout_;
    
};


#endif

/*
 * CVS Log
 * $Log: dcmqrtis.h,v $
 * Revision 1.4  2005/12/08 16:04:28  meichel
 * Changed include path schema for all DCMTK header files
 *
 * Revision 1.3  2005/11/17 13:44:37  meichel
 * Added command line options for DIMSE and ACSE timeouts
 *
 * Revision 1.2  2005/06/16 08:03:51  meichel
 * Fixed typo in method name
 *
 * Revision 1.1  2005/03/30 13:34:50  meichel
 * Initial release of module dcmqrdb that will replace module imagectn.
 *   It provides a clear interface between the Q/R DICOM front-end and the
 *   database back-end. The imagectn code has been re-factored into a minimal
 *   class structure.
 *
 *
 */
