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
 *  Author:  Marco Eichelberg
 *
 *  Purpose: class DcmQueryRetrieveOptions
 *
 *  Last Update:      $Author: meichel $
 *  Update Date:      $Date: 2005/12/16 13:14:28 $
 *  Source File:      $Source: /share/dicom/cvs-depot/dcmtk/dcmqrdb/libsrc/dcmqrtis.cc,v $
 *  CVS/RCS Revision: $Revision: 1.9 $
 *  Status:           $State: Exp $
 *
 *  CVS/RCS Log at end of file
 *
 */

#include "dcmtk/config/osconfig.h"    /* make sure OS specific configuration is included first */
#include "dcmtk/dcmqrdb/dcmqrtis.h"

#include "dcmtk/dcmnet/diutil.h"
#include "dcmtk/dcmdata/dcdeftag.h"

#ifdef WITH_LUCENE
#include "dcmtk/dcmqrdb/dcmqrdbl.h"
#elif WITH_SQL_DATABASE
#include "dcmtk/dcmqrdbx/dcmqrdbq.h"
#else
#include "dcmtk/dcmqrdb/dcmqrdbi.h"
#endif

#include "dcmtk/dcmqrdb/dcmqrdbs.h"
#include "dcmtk/dcmdata/dcfilefo.h"
#include "dcmtk/dcmqrdb/dcmqropt.h"


#include <algorithm>

BEGIN_EXTERN_C
#ifdef HAVE_SYS_STAT_H
#include <sys/stat.h>    /* needed for stat() */
#endif
#ifdef HAVE_FCNTL_H
#include <fcntl.h>       /* needed on Solaris for O_RDONLY */
#endif
END_EXTERN_C

const unsigned int MAX_STUDIES = 20;
const int NumStoreRetries = 10;

#include <sstream>
#include <iostream>
#include <boost/format.hpp>
#include <boost/scoped_ptr.hpp>

typedef boost::scoped_ptr<DcmDataset> DcmDatasetPtr;

/* ========================================== helper functions ======================================== */
OFBool TI_addStudyEntry(TI_DBEntry &db, DcmDataset &reply);
OFBool TI_addSeriesEntry(TI_StudyEntry &study, DcmDataset &reply);
OFBool TI_addImageEntry(TI_SeriesEntry &series, DcmDataset &reply);
bool operator<(const TI_ImageEntry &i1, const TI_ImageEntry &i2);
bool operator<(const TI_SeriesEntry &s1, const TI_SeriesEntry &s2);

void
TI_getInfoFromDataset(DcmDataset &dset, DIC_PN patientsName, DIC_CS studyId,
    DIC_IS seriesNumber, DIC_CS modality, DIC_IS imageNumber)
{
    DU_getStringDOElement(&dset, DCM_PatientsName, patientsName);
    DU_stripLeadingAndTrailingSpaces(patientsName);
    DU_getStringDOElement(&dset, DCM_StudyID, studyId);
    DU_stripLeadingAndTrailingSpaces(studyId);
    DU_getStringDOElement(&dset, DCM_SeriesNumber, seriesNumber);
    DU_stripLeadingAndTrailingSpaces(seriesNumber);
    DU_getStringDOElement(&dset, DCM_Modality, modality);
    DU_stripLeadingAndTrailingSpaces(modality);
    DU_getStringDOElement(&dset, DCM_InstanceNumber, imageNumber);
    DU_stripLeadingAndTrailingSpaces(imageNumber);
}

void
TI_getInfoFromImage(char *imgFile, DIC_PN patientsName, DIC_CS studyId,
    DIC_IS seriesNumber, DIC_CS modality, DIC_IS imageNumber)
{
    DcmFileFormat dcmff;
    if (dcmff.loadFile(imgFile).bad())
    {
        DcmQueryRetrieveOptions::errmsg("Help!, cannot open image file: %s", imgFile);
        return;
    }

    DcmDataset *obj = dcmff.getDataset();

    TI_getInfoFromDataset(*obj, patientsName, studyId, seriesNumber,
        modality, imageNumber);
}

void
TI_destroyImageEntries(TI_SeriesEntry &series)
{
  series.images.clear();
}

void
TI_destroySeriesEntries(TI_StudyEntry &study)
{
  study.series.clear();
}

void
TI_destroyStudyEntries(TI_DBEntry &db)
{
  db.studies.clear();
}

void splitCommand( const string &cmdString, string &cmd, string &param, int &paramAsInt ) {
  param.clear(); paramAsInt = -1;
  string::size_type p1 = cmdString.find_first_of(' ');
  if (p1 != string::npos) { 
    string::size_type p2 = cmdString.find_first_not_of(' ',p1);
    if (p2 != string::npos) {
      param = cmdString.substr(p2);
      istringstream paramStream(param);
      paramStream >> paramAsInt;
    }
    cmd = cmdString.substr(0,p1);
  } else cmd = cmdString;
}

class PrefixMatchingString : public string {
  public:
    bool startsWith( const string &o ){
      return this->compare(0, o.length(), o) == 0;
    }
};

static void storeProgressCallback(void * /*callbackData*/,
    T_DIMSE_StoreProgress *progress,
    T_DIMSE_C_StoreRQ * /*req*/)
{
    int percent;
    static int dotsSoFar = 0;
    int dotsToPrint;
    int i;

    switch (progress->state) {
    case DIMSE_StoreBegin:
        printf("  0%%________25%%_________50%%__________75%%________100%%\n");
        printf("  ");
        dotsSoFar = 0;
        break;
    case DIMSE_StoreEnd:
        printf("\n");
        break;
    default:
        if (progress->totalBytes == 0) {
            percent = 100;
        } else {
            percent = (int)(((float)progress->progressBytes /
                (float)progress->totalBytes) * 100.0);
        }
        dotsToPrint = (percent/2) - dotsSoFar;
        for (i=0; i<dotsToPrint; i++) {
            putchar('-');
            fflush(stdout);
            dotsSoFar++;
        }
        break;
    }
}

typedef struct {
    TI_GenericEntryCallbackFunction cbf;
    TI_GenericCallbackStruct *cbs;
    OFBool verbose;
} TI_LocalFindCallbackData;

static void findCallback(
  /* in */
  void *callbackData,
  T_DIMSE_C_FindRQ * /*request*/ , /* original find request */
  int responseCount,
  T_DIMSE_C_FindRSP *response,  /* pending response received */
  DcmDataset *responseIdentifiers /* pending response identifiers */
  )
{
    TI_LocalFindCallbackData *cbd;

    cbd = (TI_LocalFindCallbackData*)callbackData;

    if (cbd->verbose) {
        printf("Find Response %d:\n", responseCount);
        DIMSE_printCFindRSP(stdout, response);
        printf("Identifiers %d:\n", responseCount);
        responseIdentifiers->print(COUT);
    }

    /* call the callback function */
    cbd->cbf(*cbd->cbs, *responseIdentifiers);

    /* responseIdentifers will be deleted in DIMSE_findUser() */
}

static OFBool TI_welcome()
{
    printf("\n");
    printf("Welcome to the Image CTN Telnet Initiator\n");
    printf("\n");
    printf("This program allows you to list the contents of the CTN databases, send\n");
    printf("images to peer Application Entities (AEs), and to verify connectivity with\n");
    printf("peer AEs..\n");
    printf("The databases can only be viewed using a Study/Series/Image\n");
    printf("information model.\n");
    printf("\n");
    printf("Network associations will be started when you try to send a\n");
    printf("study/series/image or perform an echo.\n");
    printf("\n");
    printf("The prompt shows the current database title and the current peer AE title.\n");
    printf("\n");
    printf("Type help for help\n");

    return OFTrue;
}

static OFBool TI_detachDB(TI_DBEntry& db)
{
    TI_destroyStudyEntries(db);
    db.dbHandle.reset();
    return OFTrue;
}

boost::format StudyFormat("%|-20.20s| %|8.8s| %|4.4s| %|8.8s| %|-20.20s| %|15.15s| %|15.15s|");

static void printStudyEntry(TI_StudyEntry &study)
{
  cout << StudyFormat % study.patientsName % study.patientsDoB % study.numInstances % study.studyDate % study.studyDescr % study.patientID % study.studyID << endl;
}

boost::format SeriesFormat("%|5.5| %|-20.20s| %|8.8s| %|6.6s| %|-8.8s| %|-20.20s|");

static void printSeriesEntry(TI_SeriesEntry &series)
{
  cout << SeriesFormat % series.numInstances % series.descr % series.time % series.seriesNumber % series.modality % series.seriesInstanceUID << endl;
}

#define IMAGEFORMAT "%-5s %-s\n"

static void printImageEntry(TI_ImageEntry &image)
{
    printf(IMAGEFORMAT, image.imageNumber, image.sopInstanceUID);
}

static void TI_buildStudyQuery(DcmDatasetPtr &query, const string &queryparam)
{
    query.reset( new DcmDataset );
    if (!query) {
        DcmQueryRetrieveOptions::errmsg("Help, out of memory");
        return;
    }

    DU_putStringDOElement(query.get(), DCM_QueryRetrieveLevel, "STUDY");
    DU_putStringDOElement(query.get(), DCM_StudyInstanceUID, NULL);
    DU_putStringDOElement(query.get(), DCM_StudyID, NULL);
    DU_putStringDOElement(query.get(), DCM_PatientsBirthDate, NULL);
    DU_putStringDOElement(query.get(), DCM_StudyDate, NULL);
    DU_putStringDOElement(query.get(), DCM_StudyDescription, NULL);
    DU_putStringDOElement(query.get(), DCM_NumberOfStudyRelatedInstances, NULL);
    DU_putStringDOElement(query.get(), DCM_PatientsName, queryparam.length()?queryparam.c_str():NULL);
    DU_putStringDOElement(query.get(), DCM_PatientID, NULL);
}

OFBool TI_genericEntryCallback(TI_GenericCallbackStruct &cbs, DcmDataset &reply)
{
    if (cbs.db) return TI_addStudyEntry(*cbs.db, reply);
    if (cbs.study) return TI_addSeriesEntry(*cbs.study, reply);
    if (cbs.series) return TI_addImageEntry(*cbs.series, reply);
    return OFFalse;
}

OFBool
TI_addSeriesEntry(TI_StudyEntry &study, DcmDataset &reply)
{
    OFBool ok = OFTrue;
    TI_SeriesEntry series;
    /* extract info from reply */
    ok = DU_getStringDOElement(&reply, DCM_SeriesInstanceUID, series.seriesInstanceUID);
    if (ok) ok = DU_getStringDOElement(&reply, DCM_SeriesNumber, series.seriesNumber);
    if (ok) ok = DU_getStringDOElement(&reply, DCM_Modality, series.modality);

    if (!ok) {
        DcmQueryRetrieveOptions::errmsg("TI_addSeriesEntry: missing data in DB reply");
        return OFFalse;
    }
    
    reply.findAndGetOFString(DCM_NumberOfSeriesRelatedInstances, series.numInstances);
    reply.findAndGetOFString(DCM_SeriesDescription, series.descr);
    reply.findAndGetOFString(DCM_SeriesTime, series.time);

    DU_stripLeadingAndTrailingSpaces(series.seriesInstanceUID);
    DU_stripLeadingAndTrailingSpaces(series.seriesNumber);
    DU_stripLeadingAndTrailingSpaces(series.modality);

    series.intSeriesNumber = atoi(series.seriesNumber);

    /* add to array */
    study.series.push_back( series );
    return OFTrue;
}

static void TI_buildSeriesQuery(DcmDatasetPtr &query, const TI_StudyEntry &study)
{
    query.reset( new DcmDataset );
    if (!query) {
        DcmQueryRetrieveOptions::errmsg("Help, out of memory");
        return;
    }

    DU_putStringDOElement(query.get(), DCM_QueryRetrieveLevel, "SERIES");
    DU_putStringDOElement(query.get(), DCM_StudyInstanceUID,
        study.studyInstanceUID);
    DU_putStringDOElement(query.get(), DCM_SeriesInstanceUID, NULL);
    DU_putStringDOElement(query.get(), DCM_Modality, NULL);
    DU_putStringDOElement(query.get(), DCM_SeriesNumber, NULL);
    DU_putStringDOElement(query.get(), DCM_NumberOfSeriesRelatedInstances, NULL);
    DU_putStringDOElement(query.get(), DCM_SeriesDescription, NULL);
    DU_putStringDOElement(query.get(), DCM_SeriesTime, NULL);
}

OFBool
TI_addImageEntry(TI_SeriesEntry &series, DcmDataset &reply)
{
    OFBool ok = OFTrue;
    TI_ImageEntry image;
    
    /* extract info from reply */
    ok = DU_getStringDOElement(&reply, DCM_SOPInstanceUID,
        image.sopInstanceUID);
    if (ok) ok = DU_getStringDOElement(&reply, DCM_InstanceNumber, image.imageNumber);

    if (!ok) {
        DcmQueryRetrieveOptions::errmsg("TI_addImageEntry: missing data in DB reply");
        return OFFalse;
    }

    DU_stripLeadingAndTrailingSpaces(image.sopInstanceUID);
    DU_stripLeadingAndTrailingSpaces(image.imageNumber);

    image.intImageNumber = atoi(image.imageNumber);

    /* add to array */
    series.images.push_back( image );

    return OFTrue;
}

bool operator<(const TI_ImageEntry &i1, const TI_ImageEntry &i2) {
    return i1.intImageNumber < i2.intImageNumber;
}

bool operator<(const TI_SeriesEntry &s1, const TI_SeriesEntry &s2) {
    return s1.intSeriesNumber < s2.intSeriesNumber;
}


static void TI_buildImageQuery(DcmDatasetPtr &query, TI_StudyEntry &study,
    TI_SeriesEntry &series)
{
    query.reset( new DcmDataset);
    if (!query) {
        DcmQueryRetrieveOptions::errmsg("Help, out of memory!");
        return;
    }

    DU_putStringDOElement(query.get(), DCM_QueryRetrieveLevel, "IMAGE");
    DU_putStringDOElement(query.get(), DCM_StudyInstanceUID,
        study.studyInstanceUID);
    DU_putStringDOElement(query.get(), DCM_SeriesInstanceUID,
        series.seriesInstanceUID);
    DU_putStringDOElement(query.get(), DCM_InstanceNumber, NULL);
    DU_putStringDOElement(query.get(), DCM_SOPInstanceUID, NULL);
}


OFBool
TI_addStudyEntry(TI_DBEntry &db, DcmDataset &reply)
{
    OFBool ok = OFTrue;
    TI_StudyEntry se;
    /* extract info from reply */
    ok = DU_getStringDOElement(&reply, DCM_StudyInstanceUID, se.studyInstanceUID);
    if (ok) ok = DU_getStringDOElement(&reply, DCM_StudyID, se.studyID);
    if (ok) ok = DU_getStringDOElement(&reply, DCM_PatientsName, se.patientsName);
    if (ok) ok = DU_getStringDOElement(&reply, DCM_PatientID, se.patientID);

    reply.findAndGetOFString(DCM_PatientsBirthDate, se.patientsDoB);
    reply.findAndGetOFString(DCM_StudyDate, se.studyDate);
    reply.findAndGetOFString(DCM_StudyDescription, se.studyDescr);
    reply.findAndGetOFString(DCM_NumberOfStudyRelatedInstances, se.numInstances);

    if (!ok) {
        DcmQueryRetrieveOptions::errmsg("TI_addStudyEntry: missing data in DB reply");
        return OFFalse;
    }

    DU_stripLeadingAndTrailingSpaces(se.studyInstanceUID);
    DU_stripLeadingAndTrailingSpaces(se.studyID);
    DU_stripLeadingAndTrailingSpaces(se.patientsName);
    DU_stripLeadingAndTrailingSpaces(se.patientID);

    /* add to array */
    db.studies.push_back( se );

    return OFTrue;
}

/* ========================================== DcmQueryRetrieveTelnetInitiator ======================================== */

DcmQueryRetrieveTelnetInitiator::DcmQueryRetrieveTelnetInitiator(
      DcmQueryRetrieveConfig &cfg)
:net(NULL)
, assoc(NULL)
, maxReceivePDULength(0)
, config(cfg)
, networkTransferSyntax(EXS_Unknown)
, verbose(OFFalse)
, debug(OFFalse)
, blockMode_(DIMSE_BLOCKING)
, dimse_timeout_(0)
{
}

OFBool DcmQueryRetrieveTelnetInitiator::TI_detachAssociation(OFBool abortFlag)
{
    OFCondition cond = EC_Normal;
    DIC_NODENAME presentationAddress;
    DIC_AE peerTitle;

    if (assoc == NULL) {
      return OFTrue;  /* nothing to do */
    }

    ASC_getPresentationAddresses(assoc->params, NULL,
        presentationAddress);
    ASC_getAPTitles(assoc->params, NULL, peerTitle, NULL);

    if (abortFlag) {
        /* abort association */
        if (verbose)
            printf("Aborting Association (%s)\n", peerTitle);
        cond = ASC_abortAssociation(assoc);
        if (cond.bad()) {
            DcmQueryRetrieveOptions::errmsg("Association Abort Failed:");
            DimseCondition::dump(cond);
        }
    } else {
        /* release association */
        if (verbose)
            printf("Releasing Association (%s)\n", peerTitle);
        cond = ASC_releaseAssociation(assoc);
        if (cond.bad()) {
            DcmQueryRetrieveOptions::errmsg("Association Release Failed:");
            DimseCondition::dump(cond);
        }
    }
    ASC_dropAssociation(assoc);
    ASC_destroyAssociation(&assoc);

    if (abortFlag) {
        printf("Aborted Association (%s,%s)\n",
              presentationAddress, peerTitle);
    } else {
        printf("Released Association (%s,%s)\n",
              presentationAddress, peerTitle);
    }

    return OFTrue;
}

OFCondition DcmQueryRetrieveTelnetInitiator::addPresentationContexts(T_ASC_Parameters &params)
{
    OFCondition cond = EC_Normal;

    /* abstract syntaxes for storage SOP classes are taken from dcmdata */
    const char *abstractSyntaxes[] = {
      UID_VerificationSOPClass,
      UID_FINDStudyRootQueryRetrieveInformationModel
    };

    int i;
    int pid = 1;

    /*
    ** We prefer to accept Explicitly encoded transfer syntaxes.
    ** If we are running on a Little Endian machine we prefer
    ** LittleEndianExplicitTransferSyntax to BigEndianTransferSyntax.
    ** Some SCP implementations will just select the first transfer
    ** syntax they support (this is not part of the standard) so
    ** organise the proposed transfer syntaxes to take advantage
    ** of such behaviour.
    */

    unsigned int numTransferSyntaxes = 0;
    const char* transferSyntaxes[] = { NULL, NULL, NULL };

    if (networkTransferSyntax == EXS_LittleEndianImplicit)
    {
        transferSyntaxes[0] = UID_LittleEndianImplicitTransferSyntax;
        numTransferSyntaxes = 1;
    }
    else
    {
        /* gLocalByteOrder is defined in dcxfer.h */
        if (gLocalByteOrder == EBO_LittleEndian)
        {
            /* we are on a little endian machine */
            transferSyntaxes[0] = UID_LittleEndianExplicitTransferSyntax;
            transferSyntaxes[1] = UID_BigEndianExplicitTransferSyntax;
            transferSyntaxes[2] = UID_LittleEndianImplicitTransferSyntax;
            numTransferSyntaxes = 3;
        } else {
            /* we are on a big endian machine */
            transferSyntaxes[0] = UID_BigEndianExplicitTransferSyntax;
            transferSyntaxes[1] = UID_LittleEndianExplicitTransferSyntax;
            transferSyntaxes[2] = UID_LittleEndianImplicitTransferSyntax;
            numTransferSyntaxes = 3;
        }
    }

    /* first add presentation contexts for find and verification */
    for (i=0; i<(int)DIM_OF(abstractSyntaxes) && cond.good(); i++)
    {
        cond = ASC_addPresentationContext( &params, pid, abstractSyntaxes[i], transferSyntaxes, numTransferSyntaxes);
        pid += 2; /* only odd presentation context id's */
    }

    /* and then for all storage SOP classes */
    for (i=0; i<numberOfDcmLongSCUStorageSOPClassUIDs && cond.good(); i++)
    {
      cond = ASC_addPresentationContext( &params, pid, dcmLongSCUStorageSOPClassUIDs[i], transferSyntaxes, numTransferSyntaxes);
      pid += 2;/* only odd presentation context id's */
    }

    return cond;
}


OFBool DcmQueryRetrieveTelnetInitiator::TI_attachAssociation()
{
    OFCondition cond = EC_Normal;
    int port;
    const char *peer;
    DIC_NODENAME presentationAddress;
    T_ASC_Parameters *params;
    DIC_NODENAME localHost;
    DIC_AE currentAETitle;
    if (assoc != NULL) {
        TI_detachAssociation(OFFalse);
    }
    
    TI_DBEntry &currentdb = dbEntries.at( currentdbIdx );

    if (currentdb.isRemoteDB) {
        strcpy(currentAETitle, myAETitle.c_str());
    } else {
        strcpy(currentAETitle, currentdb.title.c_str());
    }

    cond = ASC_createAssociationParameters(&params, maxReceivePDULength);
    if (cond.bad()) {
        DcmQueryRetrieveOptions::errmsg("Help, cannot create association parameters:");
        DimseCondition::dump(cond);
        return OFFalse;
    }
    ASC_setAPTitles(params, currentAETitle, currentPeerTitle.c_str(), NULL);

    gethostname(localHost, sizeof(localHost) - 1);
    if (!config.peerForAETitle(currentPeerTitle.c_str(), &peer, &port)) {
        DcmQueryRetrieveOptions::errmsg("Help, AE title (%s) no longer in config",
            currentPeerTitle.c_str());
        ASC_destroyAssociationParameters(&params);
        return OFFalse;
    }
    sprintf(presentationAddress, "%s:%d", peer, port);
    ASC_setPresentationAddresses(params, localHost, presentationAddress);

    cond = addPresentationContexts(*params);
    if (cond.bad()) {
        DcmQueryRetrieveOptions::errmsg("Help, cannot add presentation contexts:");
        DimseCondition::dump(cond);
        ASC_destroyAssociationParameters(&params);

        return OFFalse;
    }
    if (debug) {
        printf("Request Parameters:\n");
        ASC_dumpParameters(params, COUT);
    }

    /* create association */
    if (verbose)
        printf("Requesting Association\n");
    cond = ASC_requestAssociation(net, params, &assoc);
    if (cond.bad()) {
        if (cond == DUL_ASSOCIATIONREJECTED) {
            T_ASC_RejectParameters rej;
      
            ASC_getRejectParameters(params, &rej);
            DcmQueryRetrieveOptions::errmsg("Association Rejected:");
            ASC_printRejectParameters(stderr, &rej);
            fprintf(stderr, "\n");
            ASC_dropAssociation(assoc);
            ASC_destroyAssociation(&assoc);
      
            return OFFalse;
        } else {
            DcmQueryRetrieveOptions::errmsg("Association Request Failed: Peer (%s, %s)",
                presentationAddress, currentPeerTitle.c_str());
            DimseCondition::dump(cond);
            ASC_dropAssociation(assoc);
            ASC_destroyAssociation(&assoc);
      
            return OFFalse;
        }
    }
    /* what has been accepted/refused ? */
    if (debug) {
        printf("Association Parameters Negotiated:\n");
        ASC_dumpParameters(params, COUT);
    }

    if (ASC_countAcceptedPresentationContexts(params) == 0) {
        DcmQueryRetrieveOptions::errmsg("All Presentation Contexts Refused: Peer (%s, %s)",
                presentationAddress, currentPeerTitle.c_str());
        ASC_abortAssociation(assoc);
        ASC_dropAssociation(assoc);
        ASC_destroyAssociation(&assoc);
      
        return OFFalse;
    }

    if (verbose) {
        printf("Association Accepted (Max Send PDV: %lu)\n",
          assoc->sendPDVLength);
    }

    printf("New Association Started (%s,%s)\n", presentationAddress,
        currentPeerTitle.c_str());

    return OFTrue;
}

/*
 * Change Association
 */

OFBool DcmQueryRetrieveTelnetInitiator::TI_changeAssociation()
{
    DIC_AE actualPeerAETitle;
    OFBool ok = OFTrue;

    if (assoc != NULL) {
        /* do we really need to change the association */
        ASC_getAPTitles(assoc->params, NULL, actualPeerAETitle, NULL);
        if (strcmp(actualPeerAETitle, currentPeerTitle.c_str()) == 0) {
            /* no need to change */
            return OFTrue;
        }
    }

    ok = TI_detachAssociation(OFFalse);
    if (!ok) return ok;

    ok = TI_attachAssociation();
    return ok;
}

OFBool DcmQueryRetrieveTelnetInitiator::TI_sendEcho()
{
    OFCondition cond = EC_Normal;
    DIC_US msgId;
    DIC_US status;
    DcmDatasetPtr stDetail;

    msgId = assoc->nextMsgID++;
    printf("[MsgID %d] Echo, ", msgId);
    fflush(stdout);

    DcmDataset *tstd;
    cond = DIMSE_echoUser(assoc, msgId, blockMode_, dimse_timeout_,
      &status, &tstd);
    stDetail.reset(tstd);

    if (cond.good()) {
        printf("Complete [Status: %s]\n",
            DU_cstoreStatusString(status));
    } else {
        DcmQueryRetrieveOptions::errmsg("Failed:");
        DimseCondition::dump(cond);
        ASC_abortAssociation(assoc);
        ASC_dropAssociation(assoc);
        ASC_destroyAssociation(&assoc);

    }
    if (stDetail) {
        printf("  Status Detail (should never be any):\n");
        stDetail->print(COUT);
    }
    return (cond.good());
}


OFBool DcmQueryRetrieveTelnetInitiator::TI_storeImage(char *sopClass, char *sopInstance, char * imgFile)
{
    OFCondition cond = EC_Normal;
    DIC_US msgId;
    DcmDatasetPtr stDetail;
    T_ASC_PresentationContextID presId;
    T_DIMSE_C_StoreRQ req;
    T_DIMSE_C_StoreRSP rsp;
    DIC_PN patientsName;
    DIC_CS studyId;
    DIC_IS seriesNumber;
    DIC_CS modality;
    DIC_IS imageNumber;

    if (strlen(sopClass) == 0) {
        printf("WARNING: CTN has deleted image, giving up (no sopClass): %s\n",
         (imgFile)?(imgFile):("(nil)"));
        /* give up because if this image is gone, then others are also
         * very likely to have disappeared.  The user should restart
         * the operation when other activities have finished.
         */
        return OFFalse;
    }

#ifdef LOCK_IMAGE_FILES
     /* shared lock image file */
    int lockfd;
#ifdef O_BINARY
    lockfd = open(imgFile, O_RDONLY | O_BINARY, 0666);
#else
    lockfd = open(imgFile, O_RDONLY, 0666);
#endif
    if (lockfd < 0) {
        printf("WARNING: CTN has deleted image, giving up (no imgFile): %s\n",
         (imgFile)?(imgFile):("(nil)"));
        /* give up because if this image is gone, then others are also
         * very likely to have disappeared.  The user should restart
         * the operation when other activities have finished.
         */
        return OFFalse;
    }
    dcmtk_flock(lockfd, LOCK_SH);
#endif

    /* which presentation context should be used */
    presId = ASC_findAcceptedPresentationContextID(assoc, sopClass);
    if (presId == 0) {
        DcmQueryRetrieveOptions::errmsg("No presentation context for: (%s) %s",
            dcmSOPClassUIDToModality(sopClass), sopClass);
        return OFFalse;
    }

    TI_getInfoFromImage(imgFile, patientsName, studyId,
        seriesNumber, modality, imageNumber);

    /* start store */
    msgId = assoc->nextMsgID++;
    printf("[MsgID %d] Store,\n", msgId);
    printf("  PatientsName: %s, StudyID: %s,\n",
        patientsName, studyId);
    printf("  Series: %s, Modality: %s, Image: %s,\n",
        seriesNumber, modality, imageNumber);
    printf("  Image UID: %s\n", sopInstance);
    fflush(stdout);
    bzero((char*)&req, sizeof(req));
    req.MessageID = msgId;
    strcpy(req.AffectedSOPClassUID, sopClass);
    strcpy(req.AffectedSOPInstanceUID, sopInstance);
    req.DataSetType = DIMSE_DATASET_PRESENT;
    req.Priority = DIMSE_PRIORITY_MEDIUM;
    
    DcmFileFormat dcmff;
    cond = dcmff.loadFile(imgFile);
    /* figure out if an error occured while the file was read*/
    if (cond.bad()) {
        printf("Bad DICOM file: %s: %s", imgFile, cond.text());
        return cond.good();
    }

#ifdef ON_THE_FLY_COMPRESSION
    T_ASC_PresentationContext pc;
    ASC_findAcceptedPresentationContext(assoc->params, presId, &pc);
    DcmXfer netTransfer(pc.acceptedTransferSyntax);
    dcmff.getDataset()->chooseRepresentation(netTransfer.getXfer(), NULL);
#endif    
    
    DcmDataset *tstd = NULL;
    cond = DIMSE_storeUser(assoc, presId, &req,
        NULL, dcmff.getDataset(), storeProgressCallback, NULL,
        blockMode_, dimse_timeout_,
        &rsp, &tstd, NULL, DU_fileSize(imgFile));    

    stDetail.reset(tstd);

#ifdef LOCK_IMAGE_FILES
     /* unlock image file */
    dcmtk_flock(lockfd, LOCK_UN);
    close(lockfd);
#endif

    if (cond.good()) {
        printf("[MsgID %d] Complete [Status: %s]\n", msgId,
            DU_cstoreStatusString(rsp.DimseStatus));
    } else {
        DcmQueryRetrieveOptions::errmsg("[MsgID %d] Failed:", msgId);
        DimseCondition::dump(cond);
        ASC_abortAssociation(assoc);
        ASC_dropAssociation(assoc);
        ASC_destroyAssociation(&assoc);
    }
    if (stDetail) {
        printf("  Status Detail:\n");
        stDetail->print(COUT);
    }
    return (cond.good());
}


/*
 * Find for remote DBs
 */

OFBool DcmQueryRetrieveTelnetInitiator::TI_remoteFindQuery(TI_DBEntry &db, DcmDataset &query,
    TI_GenericEntryCallbackFunction callbackFunction,
    TI_GenericCallbackStruct &callbackData)
{
    OFBool ok = OFTrue;
    TI_LocalFindCallbackData cbd;
    OFCondition           cond = EC_Normal;
    T_ASC_PresentationContextID presId;
    DIC_US        msgId;
    T_DIMSE_C_FindRQ  req;
    T_DIMSE_C_FindRSP rsp;
    DcmDatasetPtr    stDetail;

    currentPeerTitle = db.title;

    /* make sure we have an association */
    ok = TI_changeAssociation();
    if (!ok) return OFFalse;

    cbd.cbf = callbackFunction;
    cbd.cbs = &callbackData;
    cbd.verbose = verbose;

    /* which presentation context should be used */
    presId = ASC_findAcceptedPresentationContextID(assoc,
        UID_FINDStudyRootQueryRetrieveInformationModel);
    if (presId == 0) {
        DcmQueryRetrieveOptions::errmsg("No Presentation Context for Find Operation");
        return OFFalse;
    }

    msgId =  assoc->nextMsgID++;

    if (verbose) {
        printf("Sending Find SCU RQ: MsgID %d:\n", msgId);
        query.print(COUT);
    }

    req.MessageID = msgId;
    strcpy(req.AffectedSOPClassUID,
     UID_FINDStudyRootQueryRetrieveInformationModel);
    req.Priority = DIMSE_PRIORITY_LOW;

    DcmDataset *tstd;
    cond = DIMSE_findUser(assoc, presId, &req, &query,
      findCallback, &cbd, blockMode_, dimse_timeout_, &rsp, &tstd);
    stDetail.reset(tstd);

    if (cond.good()) {
        if (verbose) {
            DIMSE_printCFindRSP(stdout, &rsp);
        }
    } else {
        DcmQueryRetrieveOptions::errmsg("Find Failed:");
        DimseCondition::dump(cond);
    }
    if (stDetail) {
        printf("  Status Detail:\n");
        stDetail->print(COUT);
    }

    return cond.good();

}


/* ====================================== TI USER INTERFACE ===================================== */


OFBool DcmQueryRetrieveTelnetInitiator::TI_shortHelp()
{
    printf("h)elp, t)itle, da)tabase, f)ilter, st)udy, ser)ies i)mage, di)splay, sen)d, e)cho, q)uit\n");
    return OFTrue;
}

OFBool DcmQueryRetrieveTelnetInitiator::TI_help()
{
    if (verbose) {
        printf("TI_help:\n");
    }
    printf("Command Summary:\n");
    printf("help                list this summary\n");
    printf("?                   short help\n");
    printf("title [#]           list [set] current peer AE title\n");
    printf("database [#]        list [set] current database\n");
    printf("filter [#]          show [set] current filter\n");
    printf("study [#]           list [set] current study\n");
    printf("series [#]          list [set] current series\n");
    printf("image [#]           list [set] current image\n");
    printf("send study [#]      send current [specific] study\n");
    printf("send series [#]     send current [specific] series\n");
    printf("send image [#]      send current [specific] image\n");
    printf("echo [#]            verify connectivity [# times]\n");
    printf("quit                quit program\n");
    printf("exit                synonym for quit\n");

    return OFTrue;
}

OFBool DcmQueryRetrieveTelnetInitiator::TI_title(int arg)
{
    const char *peer;
    int port;
    DIC_AE peerTitle;
    TI_DBEntry &currentdb = dbEntries.at( currentdbIdx );

    if (verbose) {
        printf("TI_title: arg=%d\n", arg);
    }

    bzero(peerTitle, sizeof(peerTitle));
    if (assoc) {
        ASC_getAPTitles(assoc->params, NULL, peerTitle, NULL);
    }

    if (arg < 0) {
        /* print list of peer AE titles we know */
        printf("Peer AE Titles:\n");
        printf("     %-16s %s\n", "Peer AE", "HostName:PortNumber");
	int c = 0;
        for (TI_String_List::iterator i = currentdb.peerTitles.begin(); i != currentdb.peerTitles.end(); i++) {
            if (currentPeerTitle == *i) {
                printf("*");
            } else {
                printf(" ");
            }
            /* active = (strcmp(peerTitle, db->peerTitles[i]) == 0); */
            config.peerForAETitle(i->c_str(), &peer, &port);
            printf(" %d) %-16s (%s:%d)\n", c++, i->c_str(),
                peer, port);
        }
    } else {
        /* choosing new peer AE title */
        if (arg >= (int)currentdb.peerTitles.size()) {
            printf("ERROR: Peer AE Title Choice: 0 - %d\n",
                (unsigned int)currentdb.peerTitles.size() - 1);
        } else {
            currentPeerTitle = currentdb.peerTitles[arg];
        }
    }
    return OFTrue;
}

OFBool DcmQueryRetrieveTelnetInitiator::TI_attachDB(TI_DBEntry &db)
{
    OFCondition dbcond = EC_Normal;
    db.studies.clear();
    db.currentImageIdx = 0;
    db.currentStudyIdx = 0;
    db.currentSeriesIdx = 0;

    if (!db.isRemoteDB && db.dbHandle == NULL) {
        /* Create a database handle */
#ifdef WITH_LUCENE
        db.dbHandle.reset( new DcmQueryRetrieveLuceneIndexHandle(
            config.getStorageArea(db.title.c_str()), 
            DcmQRLuceneReader,
            dbcond) );
#elif WITH_SQL_DATABASE
	WTF
#else
        db.dbHandle.reset( new DcmQueryRetrieveIndexDatabaseHandle(
            config.getStorageArea(db.title.c_str()), 
            config.getMaxStudies(db.title.c_str()),
            config.getMaxBytesPerStudy(db.title.c_str()), dbcond) );
#endif	
        if (dbcond.bad()) {
            DcmQueryRetrieveOptions::errmsg("TI_attachDB: cannot create DB Handle");
            return OFFalse;
        }
    } else {

    }

    return OFTrue;
}

OFBool DcmQueryRetrieveTelnetInitiator::TI_database(int arg)
{
    OFBool found = OFFalse;

    if (verbose) {
        printf("TI_database: arg=%d\n", arg);
    }
    TI_DBEntry &currentdb = dbEntries.at( currentdbIdx );

    if (arg < 0) {
        /* print list of database titles we know */
        printf("Database Titles:\n");
        printf("     %s\n", "Database");
	unsigned int c = 0;
        for (TI_DBEntry_it i=dbEntries.begin(); i!=dbEntries.end(); i++) {
            if (currentdbIdx == c) {
                printf("*");
            } else {
                printf(" ");
            }
            if (i->isRemoteDB) {
                printf("!");
            } else {
                printf(" ");
            }
            printf("%2d) %s\n", c++, i->title.c_str());
        }
    } else {
        /* choosing new title */
        if (arg >= (int)dbEntries.size()) {
            printf("ERROR: Database Title Choice: 0 - %d\n",
                (unsigned int)dbEntries.size() - 1);
        } else {
            /* release old dbHandle */
            TI_detachDB(currentdb);

            currentdbIdx = arg;
	    currentdb = dbEntries.at( currentdbIdx );
            /* check to make sure that current peer AE title is
             * available for this database, if not must choose
             * another and tell user about the change.
             */
            for (TI_String_List::iterator i = currentdb.peerTitles.begin(); !found && i != currentdb.peerTitles.end(); i++) {
                found = (currentPeerTitle == *i);
            }
            if (!found) {
                printf("WARNING: Actual Peer AE Title (%s) has no access to database: %s\n", currentPeerTitle.c_str(), currentdb.title.c_str());
                printf("         Setting Default Peer AE Title: %s\n",
                    currentdb.peerTitles.begin()->c_str());
                currentPeerTitle = *currentdb.peerTitles.begin();
            }

            if (!TI_attachDB(currentdb))
            {
                     DcmQueryRetrieveOptions::errmsg("ERROR: unable to open database, bailing out.\n");
                     exit(10);
            }
        }
    }
    return OFTrue;
}

OFBool DcmQueryRetrieveTelnetInitiator::TI_echo(int arg)
{
    OFBool ok = OFTrue;

    if (verbose) {
        printf("TI_echo: arg=%d\n", arg);
    }

    ok = TI_changeAssociation();
    if (!ok) return OFFalse;

    if (arg <= 0) arg = 1;  /* send 1 echo message */

    /* send echo message to peer AE title */

    while ( arg-- > 0 && ok) {
        ok = TI_sendEcho();
    }

    ok = TI_detachAssociation(OFFalse);

    return ok;
}

OFBool DcmQueryRetrieveTelnetInitiator::TI_quit()
{
    if (verbose) {
        printf("TI_quit:\n");
    }
    TI_detachAssociation(OFFalse);
    printf("Good Bye, Auf Wiedersehen, Au Revoir\n");
    exit(0);
    return OFTrue;
}

OFBool DcmQueryRetrieveTelnetInitiator::TI_actualizeStudies()
{
    /* get a list of all the available studies in the current database */
    TI_DBEntry &currentdb = dbEntries.at( currentdbIdx );
    if (!TI_buildStudies(currentdb))
        return OFFalse;

    if (currentdb.studies.size() == 0) {
        printf("No Studies in Database: %s", currentdb.title.c_str());
        if (patientFilter.length()) printf(" (with filter \"%s\")", patientFilter.c_str());
        printf("\n");
        return OFFalse;
    }
    if (currentdb.currentStudyIdx >= currentdb.studies.size())
      currentdb.currentStudyIdx = 0;
    return OFTrue;
}

OFBool DcmQueryRetrieveTelnetInitiator::TI_filter( const string &cmdString )
{
    if (verbose) {
        printf("TI_filter: arg=%s\n", cmdString.c_str());
    }
    patientFilter = cmdString;
    if (!TI_actualizeStudies())
        return OFFalse;
    return OFTrue;
}


OFBool DcmQueryRetrieveTelnetInitiator::TI_study(int arg)
{
    if (verbose) {
        cout << "TI_study: arg=" << arg << endl;
    }

    TI_DBEntry &currentdb = dbEntries.at( currentdbIdx );
    if (currentdb.isRemoteDB) {
        currentPeerTitle = currentdb.title;
        /* make sure we have an association */
        OFBool ok = TI_changeAssociation();
        if (!ok) return OFFalse;
    }

    if (!TI_actualizeStudies())
        return OFFalse;

#ifndef RETAIN_ASSOCIATION
    if (currentdb.isRemoteDB) {
        TI_detachAssociation(OFFalse);
    }
#endif

    if (arg >= 0) {
        /* set current study */
        if (arg >= (int)currentdb.studies.size()) {
            cout << "ERROR: Study Choice: 0 - " << currentdb.studies.size()- 1 << endl;
            return OFFalse;
        }
        currentdb.currentStudyIdx = arg;
        return OFTrue;
    }

    /* list studies to user */
    boost::format prefix("%|1| %|3|%|1| ");
    cout << prefix % ' ' % ' ' % ' ';
    cout << StudyFormat % "Patient" % "DoB" % "#Im" % "Date" % "Description" % "PatientID" % "StudyID" << endl;
    unsigned int c = 0;
    for (TI_StudyEntry_List::iterator i = currentdb.studies.begin(); i != currentdb.studies.end(); i++) {
	cout << prefix % ((currentdb.currentStudyIdx == c)?"*":"") % c % ')';
	c++;
        printStudyEntry(*i);
    }
    cout << endl;
    cout << "Studies in Database: " << currentdb.title;
    if (patientFilter.length()) 
      cout << " (with filter " << patientFilter << ")";
    if (currentdb.studies.size()>=MAX_STUDIES)
      cout << "(showing only first " << MAX_STUDIES << ")";
    cout << endl;
    return OFTrue;
}

OFBool DcmQueryRetrieveTelnetInitiator::TI_actualizeSeries()
{
    TI_DBEntry &currentdb = dbEntries.at( currentdbIdx );
    if (currentdb.studies.size() == 0)
        if (!TI_actualizeStudies())
            return OFFalse;

    TI_StudyEntry &study = currentdb.studies.at( currentdb.currentStudyIdx );

    /* get a list of all the available series in the current study */
    if (!TI_buildSeries(currentdb, study))
        return OFFalse;

    if (study.series.size() == 0) {
        printf("No Series in Study %s (Database: %s)\n",
            study.studyID, currentdb.title.c_str());
        return OFFalse;
    }
    
    if (currentdb.currentSeriesIdx >= study.series.size())
      currentdb.currentSeriesIdx = 0;
    
    return OFTrue;
}

OFBool DcmQueryRetrieveTelnetInitiator::TI_series(int arg)
{
    TI_DBEntry &currentdb = dbEntries.at( currentdbIdx );
    if (verbose) {
        cout << "TI_sseries: arg=" << arg << endl;
    }

    if (currentdb.isRemoteDB) {
        currentPeerTitle = currentdb.title;
        /* make sure we have an association */
        OFBool ok = TI_changeAssociation();
        if (!ok) return OFFalse;
    }

    if (!TI_actualizeSeries())
        return OFFalse;

#ifndef RETAIN_ASSOCIATION
    if (currentdb.isRemoteDB) {
        TI_detachAssociation(OFFalse);
    }
#endif

    TI_StudyEntry &study = currentdb.studies.at( currentdb.currentStudyIdx );

    if (arg >= 0) {
        /* set current series */
        if (arg >= (int)study.series.size()) {
            printf("ERROR: Series Choice: 0 - %d\n",
                (unsigned int)study.series.size() - 1);
            return OFFalse;
        }
        currentdb.currentSeriesIdx = arg;
        return OFTrue;
    }
    
    /* list series to user */
    boost::format prefix("%|1| %|3|%|1| ");
    cout << prefix % ' ' % ' ' % ' ';
    cout << SeriesFormat % "#Im" % "Description" % "Time" % "Number" % "Modality" % "SeriesInstanceUID" << endl;
    unsigned int c = 0;
    for (TI_SeriesEntry_List::iterator i = study.series.begin(); i != study.series.end(); i++) {
	cout << prefix % ((currentdb.currentSeriesIdx == c)?"*":"") % c % ')';
	c++;
	printSeriesEntry(*i);
    }

    cout << endl;
    cout << study.series.size() << " Series in StudyID " << study.studyID << endl;
    cout << "  Patient: " << study.patientsName << " (Database: " << currentdb.title << ")" << endl;
    return OFTrue;
}

OFBool DcmQueryRetrieveTelnetInitiator::TI_actualizeImages()
{
    TI_DBEntry &currentdb = dbEntries.at( currentdbIdx );

    if (currentdb.studies.size() == 0) {
        if (!TI_actualizeStudies())
            return OFFalse;
    }

    TI_StudyEntry &study = currentdb.studies.at( currentdb.currentStudyIdx );
    if (study.series.size() == 0) {
        if (!TI_actualizeSeries())
            return OFFalse;
    }

    TI_SeriesEntry &series = study.series.at(currentdb.currentSeriesIdx);

    /* get a list of all the available images in the current series */
    if (!TI_buildImages(currentdb, study, series))
        return OFFalse;

    if (series.images.size() == 0) {
        printf("No Images in Series %s, Study %s (Database: %s)\n",
            series.seriesNumber, study.studyID, currentdb.title.c_str());
        return OFFalse;
    }
    
    if (currentdb.currentImageIdx >= series.images.size())
      currentdb.currentImageIdx = 0;
    
    return OFTrue;
}

OFBool DcmQueryRetrieveTelnetInitiator::TI_image(int arg)
{
    TI_DBEntry &currentdb = dbEntries.at( currentdbIdx );
    if (verbose) {
        printf("TI_image: arg=%d\n", arg);
    }

    if (currentdb.isRemoteDB) {
        currentPeerTitle = currentdb.title;
        /* make sure we have an association */
        OFBool ok = TI_changeAssociation();
        if (!ok) return OFFalse;
    }

    if (!TI_actualizeImages())
        return OFFalse;

#ifndef RETAIN_ASSOCIATION
    if (currentdb.isRemoteDB) {
        TI_detachAssociation(OFFalse);
    }
#endif

    TI_StudyEntry &study = currentdb.studies.at( currentdb.currentStudyIdx );
    TI_SeriesEntry &series = study.series.at(currentdb.currentSeriesIdx);

    if (arg >= 0) {
        /* set current image */
        if (arg >= (int)series.images.size()) {
            printf("ERROR: Image Choice: 0 - %d\n",
                (unsigned int)series.images.size() - 1);
            return OFFalse;
        }
        currentdb.currentImageIdx = arg;
        return OFTrue;
    }

    /* list images to user */
    printf("      ");
    printf(IMAGEFORMAT, "Image", "ImageInstanceUID");
    unsigned int c = 0;
    for (TI_ImageEntry_List::iterator i = series.images.begin(); i != series.images.end(); i++) {
        if (currentdb.currentImageIdx == c) {
            printf("*");
        } else {
            printf(" ");
        }
        printf(" %2d) ", c++);
        printImageEntry(*i);
    }

    printf("\n");
    printf("%d Images in %s Series, StudyID %s,\n",
        (unsigned int)series.images.size(), series.modality, study.studyID);
    printf("  Patient: %s (Database: %s)\n",
        study.patientsName, currentdb.title.c_str());
    return OFTrue;
}

OFBool DcmQueryRetrieveTelnetInitiator::TI_sendStudy(int arg)
{
    OFBool ok = OFTrue;
    DcmDatasetPtr query;
    OFCondition dbcond = EC_Normal;
    DcmQueryRetrieveDatabaseStatus dbStatus(STATUS_Pending);
    DIC_UI sopClass;
    DIC_UI sopInstance;
    char imgFile[MAXPATHLEN+1];
    DIC_US nRemaining = 0;
    TI_DBEntry &currentdb = dbEntries.at( currentdbIdx );

    if (verbose) {
        printf("TI_sendStudy: arg=%d\n", arg);
    }

    /*
    ** We cannot read images from a DB and send images to the same DB
    ** over the network because of deadlock.  The DB move routines
    ** lock the index file.  When we send over the network to the same
    ** DB it tries to lock the index file exclusively to insert the image
    ** in the database.  We end up waiting for a response from the remote
    ** peer which never comes.
    */

    if (currentdb.title == currentPeerTitle) {
        printf("Sorry, cannot send images from a DB to itself, possible deadlock\n");
        return OFFalse;
    }

    /* make sure study info is actual */
    ok = TI_actualizeStudies();
    if (!ok) return OFFalse;

    /* set arg as current study */
    TI_StudyEntry &study = currentdb.studies.at( currentdb.currentStudyIdx );
    if (arg < 0) {
    } else if (arg >= (int)currentdb.studies.size()) {
        printf("ERROR: Study Choice: 0 - %d\n",
            (unsigned int)currentdb.studies.size() - 1);
        return OFFalse;
    } else {
      study = currentdb.studies.at( arg );
    }

    /* make sure we have an association */
    ok = TI_changeAssociation();
    if (!ok) return OFFalse;

    /* fabricate query */
    query.reset( new DcmDataset );
    if (!query) {
        DcmQueryRetrieveOptions::errmsg("Help, out of memory");
        return OFFalse;
    }
    DU_putStringDOElement(query.get(), DCM_QueryRetrieveLevel, "STUDY");
    DU_putStringDOElement(query.get(), DCM_StudyInstanceUID, study.studyInstanceUID);

    dbcond = currentdb.dbHandle->startMoveRequest(
        UID_MOVEStudyRootQueryRetrieveInformationModel, query.get(), &dbStatus);
    if (dbcond.bad()) {
        DcmQueryRetrieveOptions::errmsg("TI_sendStudy: cannot query database");
        return OFFalse;
    }

    while (ok && dbStatus.status() == STATUS_Pending) {
        dbcond = currentdb.dbHandle->nextMoveResponse(sopClass, sopInstance,
            imgFile, &nRemaining, &dbStatus);
        if (dbcond.bad()) {
            DcmQueryRetrieveOptions::errmsg("TI_sendStudy: database error");
            return OFFalse;
        }
        if (dbStatus.status() == STATUS_Pending) {
	    int retryCount = NumStoreRetries;
	    do {
            	ok = TI_storeImage(sopClass, sopInstance, imgFile);
		if (!ok) {
    			ok = TI_detachAssociation(OFFalse);
    			ok = TI_changeAssociation();
			if (!ok) return OFFalse;
		}
	    } while(!ok && --retryCount > 0);
            if (!ok) {
                currentdb.dbHandle->cancelMoveRequest(&dbStatus);
            }
        }
    }

    ok = TI_detachAssociation(OFFalse);

    return ok;
}


OFBool DcmQueryRetrieveTelnetInitiator::TI_sendSeries(int arg)
{
    OFBool ok = OFTrue;
    DcmDatasetPtr query;
    OFCondition dbcond = EC_Normal;
    DcmQueryRetrieveDatabaseStatus dbStatus(STATUS_Pending);
    DIC_UI sopClass;
    DIC_UI sopInstance;
    char imgFile[MAXPATHLEN+1];
    DIC_US nRemaining = 0;
    TI_DBEntry &currentdb = dbEntries.at( currentdbIdx );

    if (verbose) {
        printf("TI_sendSeries: arg=%d\n", arg);
    }

    /* make sure study/series info is actual */
    ok = TI_actualizeSeries();
    if (!ok) return OFFalse;

    TI_StudyEntry &study = currentdb.studies.at( currentdb.currentStudyIdx );
    TI_SeriesEntry &series = study.series.at(currentdb.currentSeriesIdx);
    
    /* set arg as current series */
    if (arg < 0) {
    } else if (arg >= (int)study.series.size()) {
        printf("ERROR: Series Choice: 0 - %d\n",
            (unsigned int)study.series.size() - 1);
        return OFFalse;
    } else {
      series = study.series.at( arg );;
    }

    /* make sure we have an association */
    ok = TI_changeAssociation();
    if (!ok) return OFFalse;

    /* fabricate query */
    query.reset( new DcmDataset);
    if (!query) {
        DcmQueryRetrieveOptions::errmsg("Help, out of memory");
        return OFFalse;
    }
    DU_putStringDOElement(query.get(), DCM_QueryRetrieveLevel, "SERIES");
    DU_putStringDOElement(query.get(), DCM_StudyInstanceUID, study.studyInstanceUID);
    DU_putStringDOElement(query.get(), DCM_SeriesInstanceUID,
        series.seriesInstanceUID);

    dbcond = currentdb.dbHandle->startMoveRequest(
        UID_MOVEStudyRootQueryRetrieveInformationModel, query.get(), &dbStatus);
    if (dbcond.bad()) {
        DcmQueryRetrieveOptions::errmsg("TI_sendSeries: cannot query database");
        return OFFalse;
    }

    while (ok && dbStatus.status() == STATUS_Pending) {
        dbcond = currentdb.dbHandle->nextMoveResponse(sopClass, sopInstance,
            imgFile, &nRemaining, &dbStatus);
        if (dbcond.bad()) {
            DcmQueryRetrieveOptions::errmsg("TI_sendSeries: database error");
            return OFFalse;
        }
        if (dbStatus.status() == STATUS_Pending) {
	    int retryCount = NumStoreRetries;
	    do {
            	ok = TI_storeImage(sopClass, sopInstance, imgFile);
		if (!ok) {
    			ok = TI_detachAssociation(OFFalse);
    			ok = TI_changeAssociation();
			if (!ok) return OFFalse;
		}
	    } while(!ok && --retryCount > 0);
            if (!ok) {
                currentdb.dbHandle->cancelMoveRequest(&dbStatus);
            }
        }
    }

    ok = TI_detachAssociation(OFFalse);

    return ok;
}

OFBool DcmQueryRetrieveTelnetInitiator::TI_sendImage(int arg)
{
    OFBool ok = OFTrue;
    DcmDatasetPtr query;
    OFCondition dbcond = EC_Normal;
    DcmQueryRetrieveDatabaseStatus dbStatus(STATUS_Pending);
    DIC_UI sopClass;
    DIC_UI sopInstance;
    char imgFile[MAXPATHLEN+1];
    DIC_US nRemaining = 0;
    TI_DBEntry &currentdb = dbEntries.at( currentdbIdx );

    if (verbose) {
        printf("TI_sendImage: arg=%d\n", arg);
    }

    /* make sure study/series/image info is actual */
    ok = TI_actualizeImages();
    if (!ok) return OFFalse;

    TI_StudyEntry &study = currentdb.studies.at( currentdb.currentStudyIdx );
    TI_SeriesEntry &series = study.series.at(currentdb.currentSeriesIdx);
    TI_ImageEntry &image = series.images.at( currentdb.currentImageIdx );

    /* set arg as current image */
    if (arg < 0) {
    } else if (arg >= (int)series.images.size()) {
        printf("ERROR: Image Choice: 0 - %d\n",
            (unsigned int)series.images.size() - 1);
        return OFFalse;
    } else {
      image = series.images.at( arg );
    }

    /* make sure we have an association */
    ok = TI_changeAssociation();
    if (!ok) return OFFalse;

    /* fabricate query */
    query.reset( new DcmDataset );
    if (!query) {
        DcmQueryRetrieveOptions::errmsg("Help, out of memory");
        return OFFalse;
    }
    DU_putStringDOElement(query.get(), DCM_QueryRetrieveLevel, "IMAGE");
    DU_putStringDOElement(query.get(), DCM_StudyInstanceUID, study.studyInstanceUID);
    DU_putStringDOElement(query.get(), DCM_SeriesInstanceUID,
        series.seriesInstanceUID);
    DU_putStringDOElement(query.get(), DCM_SOPInstanceUID,
        image.sopInstanceUID);

    dbcond = currentdb.dbHandle->startMoveRequest(
        UID_MOVEStudyRootQueryRetrieveInformationModel, query.get(), &dbStatus);
    if (dbcond.bad()) {
        DcmQueryRetrieveOptions::errmsg("TI_sendImage: cannot query database");
        return OFFalse;
    }

    /*
     * We should only ever get 1 response to the above query,
     * but you never know (there could be non-unique UIDs in
     * the database).
     */
    while (ok && dbStatus.status() == STATUS_Pending) {
        dbcond = currentdb.dbHandle->nextMoveResponse(sopClass, sopInstance,
      imgFile, &nRemaining, &dbStatus);
        if (dbcond.bad()) {
            DcmQueryRetrieveOptions::errmsg("TI_sendImage: database error");
            return OFFalse;
        }
        if (dbStatus.status() == STATUS_Pending) {
	    int retryCount = NumStoreRetries;
	    do {
            	ok = TI_storeImage(sopClass, sopInstance, imgFile);
		if (!ok) {
    			ok = TI_detachAssociation(OFFalse);
    			ok = TI_changeAssociation();
			if (!ok) return OFFalse;
		}
	    } while(!ok && --retryCount > 0);
            if (!ok) {
                currentdb.dbHandle->cancelMoveRequest(&dbStatus);
            }
        }
    }

    ok = TI_detachAssociation(OFFalse);

    return ok;
}

OFBool DcmQueryRetrieveTelnetInitiator::TI_send(const string &cmdString)
{
    OFBool ok = OFTrue;
    if (dbEntries.at( currentdbIdx ).isRemoteDB) {
        cout << "Sorry, cannot send from remote DB" << endl;
        return OFTrue;
    }

    PrefixMatchingString cmd;
    string param;
    int paramAsInt=-1;
    splitCommand(cmdString,cmd,param,paramAsInt);
    if      (cmd.startsWith("st")) TI_sendStudy(paramAsInt);
    else if (cmd.startsWith("se")) TI_sendSeries(paramAsInt);
    else if (cmd.startsWith("i")) TI_sendImage(paramAsInt);
    else {
        cout << "What do you want to send? Type help for help" << endl;
    }

    return ok;
}

void DcmQueryRetrieveTelnetInitiator::TI_userInput()
{
    /* make the first database current */
    currentdbIdx = 0;
    TI_DBEntry &currentdb = dbEntries.at( currentdbIdx );
    
    /* make the first peer title for this database current */
    currentPeerTitle = currentdb.peerTitles[0];
    /* open db */
    TI_database(0);

    TI_welcome();
    cout << endl;

    while (1) {
	cout << currentdb.title << "->" << currentPeerTitle << " ";
	string input;
	getline(cin, input);

	PrefixMatchingString cmd;
	string parameter;
	int paramterAsInt = -1;

	/* find command parser */
	splitCommand( input, cmd, parameter, paramterAsInt );
	
        if      (cmd.startsWith("h")) TI_help();
        else if (cmd.startsWith("?")) TI_shortHelp();
        else if (cmd.startsWith("t")) TI_title(paramterAsInt);
        else if (cmd.startsWith("da")) TI_database(paramterAsInt);
        else if (cmd.startsWith("f")) TI_filter(parameter);
        else if (cmd.startsWith("st")) TI_study(paramterAsInt);
        else if (cmd.startsWith("ser")) TI_series(paramterAsInt);
        else if (cmd.startsWith("i")) TI_image(paramterAsInt);
        else if (cmd.startsWith("sen")) TI_send(parameter);
        else if (cmd.startsWith("e")) TI_echo(paramterAsInt);
        else if (cmd.startsWith("q")) TI_quit();
        else if (cmd.startsWith("exit")) TI_quit();
        else {
            cout << "What do you want to do? Type help for help" << endl;
        }
    }
}

/* ========================================== TI QUERY ========================================== */


OFBool DcmQueryRetrieveTelnetInitiator::TI_dbReadable(const string &dbTitle)
{
#ifdef WITH_LUCENE
    return DcmQueryRetrieveLuceneIndexHandle::indexExists( config.getStorageArea(dbTitle.c_str()) );
#elif WITH_SQL_DATABASE
    WTF?
#else
    char path[MAXPATHLEN+1];
    sprintf(path, "%s%c%s", config.getStorageArea(dbTitle.c_str()), PATH_SEPARATOR, DBINDEXFILE);
    return (access(path, R_OK) == 0);
#endif
}

time_t DcmQueryRetrieveTelnetInitiator::TI_dbModifyTime(const string &dbTitle)
{
#ifdef WITH_LUCENE
  return time(NULL); // to be on safe side
#elif WITH_SQL_DATABASE
    WTF?
#else
    char path[MAXPATHLEN+1];
    struct stat s;

    sprintf(path, "%s%c%s", config.getStorageArea(dbTitle.c_str()), PATH_SEPARATOR, DBINDEXFILE);

    if (stat(path, &s) < 0) {
        DcmQueryRetrieveOptions::errmsg("cannot stat: %s", path);
        return 0;
    }
    return s.st_mtime;
#endif
}

/*
 * Study Level
 */

OFBool DcmQueryRetrieveTelnetInitiator::TI_buildRemoteStudies(TI_DBEntry &db)
{
    TI_GenericCallbackStruct cbs;
    DcmDatasetPtr query;
    OFBool ok = OFTrue;

    cbs.db = &db;
    cbs.study = NULL; cbs.series = NULL;

    TI_destroyStudyEntries(db);

    /* get all known studies */
    TI_buildStudyQuery(query, patientFilter);

    ok = TI_remoteFindQuery(db, *query, TI_genericEntryCallback, cbs);

    return ok;
}

OFBool DcmQueryRetrieveTelnetInitiator::TI_buildStudies(TI_DBEntry &db)
{
    OFCondition dbcond = EC_Normal;
    DcmQueryRetrieveDatabaseStatus dbStatus(STATUS_Pending);
    DcmDatasetPtr query;
    DcmDatasetPtr reply;

    if (db.isRemoteDB) {
        return TI_buildRemoteStudies(db);
    }

    if (db.studies.size() != 0 &&
        TI_dbModifyTime(db.title) < db.lastQueryTime) {
        /* nothing has changed */
        return OFTrue;
    }

    TI_destroyStudyEntries(db);

    /* get all known studies */
    TI_buildStudyQuery(query, patientFilter);

    printf("Querying Database for Studies ...\n");
    db.lastQueryTime = time(NULL);
    dbcond = db.dbHandle->startFindRequest(
        UID_FINDStudyRootQueryRetrieveInformationModel, query.get(), &dbStatus);
    if (dbcond.bad()) {
        DcmQueryRetrieveOptions::errmsg("TI_buildStudies: cannot query database");
        return OFFalse;
    }
    
    dbStatus.deleteStatusDetail();
    unsigned int c=0;
    while (c++ < MAX_STUDIES && dbStatus.status() == STATUS_Pending) {
	DcmDataset *tr;
        dbcond = db.dbHandle->nextFindResponse(&tr, &dbStatus);
	reply.reset(tr);
        if (dbcond.bad()) {
            DcmQueryRetrieveOptions::errmsg("TI_buildStudies: database error");
            return OFFalse;
        }
        if (dbStatus.status() == STATUS_Pending) {
            TI_addStudyEntry(db, *reply);
        }
    }

    return OFTrue;
}

/*
 * Series Level
 */


OFBool DcmQueryRetrieveTelnetInitiator::TI_buildRemoteSeries(TI_DBEntry &db, TI_StudyEntry &study)
{
    TI_GenericCallbackStruct cbs;
    DcmDatasetPtr query;
    OFBool ok = OFTrue;

    cbs.db = NULL;
    cbs.study = &study; cbs.series = NULL;

    TI_destroySeriesEntries(study);

    /* get all known studies */
    TI_buildSeriesQuery(query, study);

    ok = TI_remoteFindQuery(db, *query, TI_genericEntryCallback, cbs);

    return ok;
}

OFBool DcmQueryRetrieveTelnetInitiator::TI_buildSeries(TI_DBEntry &db, TI_StudyEntry &study)
{
    OFCondition dbcond = EC_Normal;
    DcmQueryRetrieveDatabaseStatus dbStatus(STATUS_Pending);
    DcmDatasetPtr query;
    DcmDatasetPtr reply;

    if (db.isRemoteDB) {
        return TI_buildRemoteSeries(db, study);
    }

    if (study.series.size() != 0 &&
        TI_dbModifyTime(db.title) < db.lastQueryTime) {
        /* nothing has changed */
        return OFTrue;
    }

    TI_destroySeriesEntries(study);

    /* get all known series for this study */
    TI_buildSeriesQuery(query, study);

    printf("Querying Database for Series ...\n");
    study.lastQueryTime = time(NULL);

    dbcond = db.dbHandle->startFindRequest(
        UID_FINDStudyRootQueryRetrieveInformationModel, query.get(), &dbStatus);
    if (dbcond.bad()) {
        DcmQueryRetrieveOptions::errmsg("TI_buildSeries: cannot query database");
        return OFFalse;
    }
    
    dbStatus.deleteStatusDetail();

    while (dbStatus.status() == STATUS_Pending) {
	DcmDataset *tr;
        dbcond = db.dbHandle->nextFindResponse(&tr, &dbStatus);
	reply.reset(tr);
        if (dbcond.bad()) {
            DcmQueryRetrieveOptions::errmsg("TI_buildSeries: database error");
            return OFFalse;
        }
        if (dbStatus.status() == STATUS_Pending)
            TI_addSeriesEntry(study, *reply);
    }
    std::sort(study.series.begin(), study.series.end());
    return OFTrue;
}

/*
 * Image Level
 */


OFBool DcmQueryRetrieveTelnetInitiator::TI_buildRemoteImages(TI_DBEntry &db, TI_StudyEntry &study, TI_SeriesEntry &series)
{
    TI_GenericCallbackStruct cbs;
    DcmDatasetPtr query;
    OFBool ok = OFTrue;

    cbs.db = NULL;
    cbs.study = NULL; cbs.series = &series;

    TI_destroyImageEntries(series);

    /* get all known studies */
    TI_buildImageQuery(query, study, series);

    ok = TI_remoteFindQuery(db, *query, TI_genericEntryCallback, cbs);

    return ok;
}

OFBool DcmQueryRetrieveTelnetInitiator::TI_buildImages(TI_DBEntry &db, TI_StudyEntry &study, TI_SeriesEntry &series)
{
    OFCondition dbcond = EC_Normal;
    DcmQueryRetrieveDatabaseStatus dbStatus(STATUS_Pending);
    DcmDatasetPtr query;
    DcmDatasetPtr reply;

    if (db.isRemoteDB) {
        return TI_buildRemoteImages(db, study, series);
    }

    if (series.images.size() != 0 &&
        TI_dbModifyTime(db.title) < study.lastQueryTime) {
        /* nothing has changed */
        return OFTrue;
    }

    TI_destroyImageEntries(series);

    /* get all known images in current series */
    TI_buildImageQuery(query, study, series);

    if (verbose) {
        printf("QUERY OBJECT:\n");
        query->print(COUT);
    }

    printf("Querying Database for Images ...\n");
    series.lastQueryTime = time(NULL);

    dbcond = db.dbHandle->startFindRequest(
        UID_FINDStudyRootQueryRetrieveInformationModel, query.get(), &dbStatus);
    if (dbcond.bad()) {
        DcmQueryRetrieveOptions::errmsg("TI_buildImages: cannot query database");
        return OFFalse;
    }

    while (dbStatus.status() == STATUS_Pending) {
	DcmDataset *tr;
        dbcond = db.dbHandle->nextFindResponse(&tr, &dbStatus);
	reply.reset(tr);
        if (dbcond.bad()) {
            DcmQueryRetrieveOptions::errmsg("TI_buildImages: database error");
            return OFFalse;
        }
        if (dbStatus.status() == STATUS_Pending) {
            if (verbose) {
                printf("REPLY OBJECT:\n");
                reply->print(COUT);
            }
            TI_addImageEntry(series, *reply);
        }
    }
    std::sort( series.images.begin(), series.images.end());
    return OFTrue;
}


OFBool DcmQueryRetrieveTelnetInitiator::addPeerName(const string &peerName, const string &configFileName)
{
  const char **aeTitles;
  if( std::find( peerNames.begin(), peerNames.end(), peerName ) != peerNames.end() )
    return( OFTrue );

  if( config.aeTitlesForPeer( peerName.c_str(), &aeTitles ) <= 0 )
  {
    DcmQueryRetrieveOptions::errmsg("no AE titles defined (in: %s) for peer: %s", configFileName.c_str(), peerName.c_str());
    return( OFFalse );
  }
  free( aeTitles );
  peerNames.push_back( peerName );
  return( OFTrue );
}

void DcmQueryRetrieveTelnetInitiator::printConfig()
{
  printf("TI Configuration:\n");
  printf("My AE Title: %s\n", myAETitle.c_str() );
  printf("DatabaseTitles    Peer AE Titles\n");

  for(TI_DBEntry_it i = dbEntries.begin() ; i != dbEntries.end(); i++ )
  {
    printf("%-18s", i->title.c_str() );

    for(TI_String_List::iterator j = i->peerTitles.begin() ; j < i->peerTitles.end() ; j++ )
      printf("%s ", j->c_str() );

    printf("\n");
  }
}

OFBool DcmQueryRetrieveTelnetInitiator::findDBPeerTitles(
  const string &configFileName,
  TI_DBEntry &dbEntry, 
  const string &peer)
{
  const char **peerTitles = NULL;
  int peerTitleCount = 0;
  int i;

  // discover all known AETitles for peer
  peerTitleCount = config.aeTitlesForPeer( peer.c_str(),&peerTitles );
  if( peerTitleCount <= 0 )
  {
    DcmQueryRetrieveOptions::errmsg("no AE titles defined (in: %s) for peer: %s", configFileName.c_str(), peer.c_str());
    return( OFFalse );
  }

  // check to make sure peer+AEtitle has access to database areas
  for( i=0 ; i<peerTitleCount ; i++ )
  {
    if( config.peerInAETitle( dbEntry.title.c_str(), peerTitles[i], peer.c_str() ) )
    {
      // add peer title to database's peer title list
      dbEntry.peerTitles.push_back( peerTitles[i] );
    }
  }

  // throw away the old list
  free( peerTitles );

  return( dbEntry.peerTitles.size() > 0 );
}


void DcmQueryRetrieveTelnetInitiator::createConfigEntries(
  const string &configFileName,
  int remoteDBTitlesCount,
  const char **remoteDBTitles)
{
  const char **ctnTitles = NULL;
  int ctnTitleCount = 0;

  // discover all the known CTN AE titles
  ctnTitleCount = config.ctnTitles(&ctnTitles);
  for(int i=0 ; i<ctnTitleCount ; i++ )
  {
    if( !TI_dbReadable( ctnTitles[i] ) )
      DcmQueryRetrieveOptions::errmsg("Warning: db does not exist: %s", ctnTitles[i]);
    else
    {
      TI_DBEntry dbEntry;
      dbEntry.title = ctnTitles[i];
      dbEntry.isRemoteDB = OFFalse;

      for(TI_String_List::iterator j = peerNames.begin() ; j != peerNames.end(); j++ )
        findDBPeerTitles(configFileName.c_str(), dbEntry, *j);

      if( dbEntry.peerTitles.size() > 0 )
      {
	dbEntries.push_back( dbEntry );
      }
    }
  }

  // throw away the old lists
  free( ctnTitles );

  // add any remote DB entries
  for(int i=0 ; i<remoteDBTitlesCount ; i++ )
  {
    const char *peerName = NULL;
    int portNumber;
    if( config.peerForAETitle( remoteDBTitles[i], &peerName, &portNumber ) )
    {
      // add DB
      TI_DBEntry dbEntry;
      dbEntry.title = remoteDBTitles[i];
      dbEntry.isRemoteDB = OFTrue;

      for(TI_String_List::iterator j = peerNames.begin() ; j != peerNames.end(); j++ )
      {
        const char **peerTitles = NULL;
        int peerTitleCount = 0;

        peerTitleCount = config.aeTitlesForPeer( j->c_str(), &peerTitles );
        if( peerTitleCount <= 0 )
          DcmQueryRetrieveOptions::errmsg("no AE titles defined (in: %s) for peer: %s", configFileName.c_str(), j->c_str());
	

        for(int k=0 ; k<peerTitleCount ; k++ )
	  dbEntry.peerTitles.push_back( peerTitles[k] );

        // throw away the old list
        free( peerTitles );
      }
      dbEntries.push_back(dbEntry);
    }
  }
}


/*
 * CVS Log
 * $Log: dcmqrtis.cc,v $
 * Revision 1.9  2005/12/16 13:14:28  meichel
 * Simplified overly clever code producing undefined behaviour
 *
 * Revision 1.8  2005/12/15 16:13:38  joergr
 * Added char* parameter casts to bzero() calls.
 *
 * Revision 1.7  2005/12/14 17:36:28  meichel
 * Removed naming conflict
 *
 * Revision 1.6  2005/12/08 15:47:14  meichel
 * Changed include path schema for all DCMTK header files
 *
 * Revision 1.5  2005/11/17 13:44:40  meichel
 * Added command line options for DIMSE and ACSE timeouts
 *
 * Revision 1.4  2005/10/25 08:56:18  meichel
 * Updated list of UIDs and added support for new transfer syntaxes
 *   and storage SOP classes.
 *
 * Revision 1.3  2005/06/16 08:02:43  meichel
 * Added system include files needed on Solaris
 *
 * Revision 1.2  2005/04/04 14:23:21  meichel
 * Renamed application "dcmqrdb" into "dcmqrscp" to avoid name clash with
 *   dcmqrdb library, which confuses the MSVC build system.
 *
 * Revision 1.1  2005/03/30 13:34:53  meichel
 * Initial release of module dcmqrdb that will replace module imagectn.
 *   It provides a clear interface between the Q/R DICOM front-end and the
 *   database back-end. The imagectn code has been re-factored into a minimal
 *   class structure.
 *
 *
 */
