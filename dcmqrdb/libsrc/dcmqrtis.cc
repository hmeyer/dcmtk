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
#include "dcmtk/dcmqrdb/dcmqrdbi.h"
#include "dcmtk/dcmqrdb/dcmqrdbs.h"
#include "dcmtk/dcmdata/dcfilefo.h"
#include "dcmtk/dcmqrdb/dcmqropt.h"

BEGIN_EXTERN_C
#ifdef HAVE_SYS_STAT_H
#include <sys/stat.h>    /* needed for stat() */
#endif
#ifdef HAVE_FCNTL_H
#include <fcntl.h>       /* needed on Solaris for O_RDONLY */
#endif
END_EXTERN_C

/* ========================================== helper functions ======================================== */

OFBool TI_addStudyEntry(TI_DBEntry *db, DcmDataset *reply);
OFBool TI_addSeriesEntry(TI_StudyEntry *study, DcmDataset *reply);
OFBool TI_addImageEntry(TI_SeriesEntry *series, DcmDataset *reply);
int TI_seriesCompare(const void *a, const void *b);
int TI_imageCompare(const void *a, const void *b);

void
TI_getInfoFromDataset(DcmDataset *dset, DIC_PN patientsName, DIC_CS studyId,
    DIC_IS seriesNumber, DIC_CS modality, DIC_IS imageNumber)
{
    DU_getStringDOElement(dset, DCM_PatientsName, patientsName);
    DU_stripLeadingAndTrailingSpaces(patientsName);
    DU_getStringDOElement(dset, DCM_StudyID, studyId);
    DU_stripLeadingAndTrailingSpaces(studyId);
    DU_getStringDOElement(dset, DCM_SeriesNumber, seriesNumber);
    DU_stripLeadingAndTrailingSpaces(seriesNumber);
    DU_getStringDOElement(dset, DCM_Modality, modality);
    DU_stripLeadingAndTrailingSpaces(modality);
    DU_getStringDOElement(dset, DCM_InstanceNumber, imageNumber);
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

    TI_getInfoFromDataset(obj, patientsName, studyId, seriesNumber,
        modality, imageNumber);
}

void
TI_destroyImageEntries(TI_SeriesEntry *series)
{
    int i;

    if (series == NULL) return;

    for (i=0; i<series->imageCount; i++) {
        free(series->images[i]);
        series->images[i] = NULL;
    }
    series->imageCount = 0;
}

void
TI_destroySeriesEntries(TI_StudyEntry *study)
{
    int i;

    if (study == NULL) return;

    for (i=0; i<study->seriesCount; i++) {
        TI_destroyImageEntries(study->series[i]);
        free(study->series[i]);
        study->series[i] = NULL;
    }
    study->seriesCount = 0;
}

void
TI_destroyStudyEntries(TI_DBEntry *db)
{
    int i;

    if (db == NULL) return;

    for (i=0; i<db->studyCount; i++) {
        TI_destroySeriesEntries(db->studies[i]);
        free(db->studies[i]);
        db->studies[i] = NULL;
    }

    db->studyCount = 0;
}

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
    cbd->cbf(cbd->cbs, responseIdentifiers);

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

static OFBool TI_detachDB(TI_DBEntry *db)
{
    if (db == NULL) return OFTrue;

    TI_destroyStudyEntries(db);

    if (!db->isRemoteDB && db->dbHandle != NULL) {
    	delete db->dbHandle;
        db->dbHandle = NULL;
    } else {

    }

    return OFTrue;
}

#define STUDYFORMAT "%-30s %-12s %-12s\n"

static void printStudyEntry(TI_StudyEntry *study)
{
    printf(STUDYFORMAT, study->patientsName, study->patientID,
      study->studyID);
}

#define SERIESFORMAT "%-6s %-8s %-s\n"

static void printSeriesEntry(TI_SeriesEntry *series)
{
    printf(SERIESFORMAT, series->seriesNumber,
        series->modality, series->seriesInstanceUID);
}

#define IMAGEFORMAT "%-5s %-s\n"

static void printImageEntry(TI_ImageEntry *image)
{
    printf(IMAGEFORMAT, image->imageNumber, image->sopInstanceUID);
}

static void TI_buildStudyQuery(DcmDataset **query)
{
    if (*query != NULL) delete *query;
    *query = new DcmDataset;
    if (*query == NULL) {
        DcmQueryRetrieveOptions::errmsg("Help, out of memory");
        return;
    }

    DU_putStringDOElement(*query, DCM_QueryRetrieveLevel, "STUDY");
    DU_putStringDOElement(*query, DCM_StudyInstanceUID, NULL);
    DU_putStringDOElement(*query, DCM_StudyID, NULL);
    DU_putStringDOElement(*query, DCM_PatientsName, NULL);
    DU_putStringDOElement(*query, DCM_PatientID, NULL);
}

OFBool TI_genericEntryCallback(TI_GenericCallbackStruct *cbs, DcmDataset *reply)
{
    if (cbs->db) return TI_addStudyEntry(cbs->db, reply);
    if (cbs->study) return TI_addSeriesEntry(cbs->study, reply);
    if (cbs->series) return TI_addImageEntry(cbs->series, reply);
    return OFFalse;
}

OFBool
TI_addSeriesEntry(TI_StudyEntry *study, DcmDataset *reply)
{
    TI_SeriesEntry *series;
    OFBool ok = OFTrue;

    if (study->seriesCount >= TI_MAXSERIES) {
        DcmQueryRetrieveOptions::errmsg("TI_addSeriesEntry: too many series");
        return OFFalse;
    }

    series = (TI_SeriesEntry*) malloc(sizeof(TI_SeriesEntry));
    if (series == NULL) return OFFalse;

    bzero((char*)series, sizeof(TI_SeriesEntry)); /* make sure its clean */

    /* extract info from reply */
    ok = DU_getStringDOElement(reply, DCM_SeriesInstanceUID, series->seriesInstanceUID);
    if (ok) ok = DU_getStringDOElement(reply, DCM_SeriesNumber, series->seriesNumber);
    if (ok) ok = DU_getStringDOElement(reply, DCM_Modality, series->modality);

    if (!ok) {
        DcmQueryRetrieveOptions::errmsg("TI_addSeriesEntry: missing data in DB reply");
        return OFFalse;
    }

    DU_stripLeadingAndTrailingSpaces(series->seriesInstanceUID);
    DU_stripLeadingAndTrailingSpaces(series->seriesNumber);
    DU_stripLeadingAndTrailingSpaces(series->modality);

    series->intSeriesNumber = atoi(series->seriesNumber);

    /* add to array */
    study->series[study->seriesCount] = series;
    study->seriesCount++;

    return OFTrue;
}

int TI_seriesCompare(const void *a, const void *b)
{
    TI_SeriesEntry **sa, **sb;
    int cmp = 0;

    /* compare function for qsort, a and b are pointers to
     * the images array elements.  The routine must return an
     * integer less than, equal to, or greater than 0 according as
     * the first argument is to be considered less than, equal to,
     * or greater than the second.
     */
    sa = (TI_SeriesEntry **)a;
    sb = (TI_SeriesEntry **)b;

    cmp = (*sa)->intSeriesNumber - (*sb)->intSeriesNumber;

    return cmp;
}

static void TI_buildSeriesQuery(DcmDataset **query, TI_StudyEntry *study)
{
    if (*query != NULL) delete *query;
    *query = new DcmDataset;
    if (*query == NULL) {
        DcmQueryRetrieveOptions::errmsg("Help, out of memory");
        return;
    }

    DU_putStringDOElement(*query, DCM_QueryRetrieveLevel, "SERIES");
    DU_putStringDOElement(*query, DCM_StudyInstanceUID,
        study->studyInstanceUID);
    DU_putStringDOElement(*query, DCM_SeriesInstanceUID, NULL);
    DU_putStringDOElement(*query, DCM_Modality, NULL);
    DU_putStringDOElement(*query, DCM_SeriesNumber, NULL);
}

OFBool
TI_addImageEntry(TI_SeriesEntry *series, DcmDataset *reply)
{
    TI_ImageEntry *image;
    OFBool ok = OFTrue;
    DIC_CS studyID;

    if (series->imageCount >= TI_MAXIMAGES) {
        DcmQueryRetrieveOptions::errmsg("TI_addImageEntry: too many images");
        return OFFalse;
    }

    image = (TI_ImageEntry*) malloc(sizeof(TI_ImageEntry));
    if (image == NULL) return OFFalse;

    bzero((char*)image, sizeof(TI_ImageEntry)); /* make sure its clean */
    bzero((char*)studyID, sizeof(DIC_CS));

    /* extract info from reply */
    ok = DU_getStringDOElement(reply, DCM_SOPInstanceUID,
        image->sopInstanceUID);
    if (ok) ok = DU_getStringDOElement(reply, DCM_InstanceNumber, image->imageNumber);

    if (!ok) {
        DcmQueryRetrieveOptions::errmsg("TI_addImageEntry: missing data in DB reply");
        return OFFalse;
    }

    DU_stripLeadingAndTrailingSpaces(image->sopInstanceUID);
    DU_stripLeadingAndTrailingSpaces(image->imageNumber);

    image->intImageNumber = atoi(image->imageNumber);

    /* add to array */
    series->images[series->imageCount] = image;
    series->imageCount++;

    return OFTrue;
}

int TI_imageCompare(const void *a, const void *b)
{
    TI_ImageEntry **ia, **ib;
    int cmp = 0;

    /* compare function for qsort, a and b are pointers to
     * the images array elements.  The routine must return an
     * integer less than, equal to, or greater than 0 according as
     * the first argument is to be considered less than, equal to,
     * or greater than the second.
     */
    ia = (TI_ImageEntry **)a;
    ib = (TI_ImageEntry **)b;

    /* compare image numbers */
    cmp = (*ia)->intImageNumber - (*ib)->intImageNumber;

    return cmp;
}

static void TI_buildImageQuery(DcmDataset **query, TI_StudyEntry *study,
    TI_SeriesEntry *series)
{
    if (*query != NULL) delete *query;
    *query = new DcmDataset;
    if (*query == NULL) {
        DcmQueryRetrieveOptions::errmsg("Help, out of memory!");
        return;
    }

    DU_putStringDOElement(*query, DCM_QueryRetrieveLevel, "IMAGE");
    DU_putStringDOElement(*query, DCM_StudyInstanceUID,
        study->studyInstanceUID);
    DU_putStringDOElement(*query, DCM_SeriesInstanceUID,
        series->seriesInstanceUID);
    DU_putStringDOElement(*query, DCM_InstanceNumber, NULL);
    DU_putStringDOElement(*query, DCM_SOPInstanceUID, NULL);
}


OFBool
TI_addStudyEntry(TI_DBEntry *db, DcmDataset *reply)
{
    TI_StudyEntry *se;
    OFBool ok = OFTrue;

    if (db->studyCount >= TI_MAXSTUDIES) {
        DcmQueryRetrieveOptions::errmsg("TI_addStudyEntry: too many studies");
        return OFFalse;
    }

    se = (TI_StudyEntry*) malloc(sizeof(TI_StudyEntry));
    if (se == NULL) return OFFalse;

    bzero((char*)se, sizeof(TI_StudyEntry));  /* make sure its clean */

    /* extract info from reply */
    ok = DU_getStringDOElement(reply, DCM_StudyInstanceUID, se->studyInstanceUID);
    if (ok) ok = DU_getStringDOElement(reply, DCM_StudyID, se->studyID);
    if (ok) ok = DU_getStringDOElement(reply, DCM_PatientsName, se->patientsName);
    if (ok) ok = DU_getStringDOElement(reply, DCM_PatientID, se->patientID);

    if (!ok) {
        DcmQueryRetrieveOptions::errmsg("TI_addStudyEntry: missing data in DB reply");
        return OFFalse;
    }

    DU_stripLeadingAndTrailingSpaces(se->studyInstanceUID);
    DU_stripLeadingAndTrailingSpaces(se->studyID);
    DU_stripLeadingAndTrailingSpaces(se->patientsName);
    DU_stripLeadingAndTrailingSpaces(se->patientID);

    /* add to array */
    db->studies[db->studyCount] = se;
    db->studyCount++;

    return OFTrue;
}

/* ========================================== DcmQueryRetrieveTelnetInitiator ======================================== */

DcmQueryRetrieveTelnetInitiator::DcmQueryRetrieveTelnetInitiator(
      DcmQueryRetrieveConfig &cfg)
: dbEntries(NULL)
, dbCount(0)
, peerHostName(NULL)
, peerNamesCount(0)
, myAETitle(NULL)
, net(NULL)
, assoc(NULL)
, maxReceivePDULength(0)
, currentdb(0)
, currentPeerTitle(NULL)
, config(cfg)
, networkTransferSyntax(EXS_Unknown)
, verbose(OFFalse)
, debug(OFFalse)
, blockMode_(DIMSE_BLOCKING)
, dimse_timeout_(0)
{
  bzero((char*)peerNames, sizeof(peerNames));
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

OFCondition DcmQueryRetrieveTelnetInitiator::addPresentationContexts(T_ASC_Parameters *params)
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
        cond = ASC_addPresentationContext( params, pid, abstractSyntaxes[i], transferSyntaxes, numTransferSyntaxes);
        pid += 2; /* only odd presentation context id's */
    }

    /* and then for all storage SOP classes */
    for (i=0; i<numberOfDcmLongSCUStorageSOPClassUIDs && cond.good(); i++)
    {
      cond = ASC_addPresentationContext( params, pid, dcmLongSCUStorageSOPClassUIDs[i], transferSyntaxes, numTransferSyntaxes);
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

    if (dbEntries[currentdb]->isRemoteDB) {
        strcpy(currentAETitle, myAETitle);
    } else {
        strcpy(currentAETitle, dbEntries[currentdb]->title);
    }

    cond = ASC_createAssociationParameters(&params, maxReceivePDULength);
    if (cond.bad()) {
        DcmQueryRetrieveOptions::errmsg("Help, cannot create association parameters:");
        DimseCondition::dump(cond);
        return OFFalse;
    }
    ASC_setAPTitles(params, currentAETitle, currentPeerTitle, NULL);

    gethostname(localHost, sizeof(localHost) - 1);
    if (!config.peerForAETitle(currentPeerTitle, &peer, &port)) {
        DcmQueryRetrieveOptions::errmsg("Help, AE title (%s) no longer in config",
            currentPeerTitle);
        ASC_destroyAssociationParameters(&params);
        return OFFalse;
    }
    sprintf(presentationAddress, "%s:%d", peer, port);
    ASC_setPresentationAddresses(params, localHost, presentationAddress);

    cond = addPresentationContexts(params);
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
                presentationAddress, currentPeerTitle);
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
                presentationAddress, currentPeerTitle);
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
        currentPeerTitle);

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
        if (strcmp(actualPeerAETitle, currentPeerTitle) == 0) {
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
    DcmDataset *stDetail = NULL;

    msgId = assoc->nextMsgID++;
    printf("[MsgID %d] Echo, ", msgId);
    fflush(stdout);

    cond = DIMSE_echoUser(assoc, msgId, blockMode_, dimse_timeout_,
      &status, &stDetail);

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
    if (stDetail != NULL) {
        printf("  Status Detail (should never be any):\n");
        stDetail->print(COUT);
        delete stDetail;
    }
    return (cond.good());
}


OFBool DcmQueryRetrieveTelnetInitiator::TI_storeImage(char *sopClass, char *sopInstance, char * imgFile)
{
    OFCondition cond = EC_Normal;
    DIC_US msgId;
    DcmDataset *stDetail = NULL;
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

    cond = DIMSE_storeUser(assoc, presId, &req,
       imgFile, NULL, storeProgressCallback, NULL,
        blockMode_, dimse_timeout_,
        &rsp, &stDetail);

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
    if (stDetail != NULL) {
        printf("  Status Detail:\n");
        stDetail->print(COUT);
        delete stDetail;
    }

    return (cond.good());
}


/*
 * Find for remote DBs
 */

OFBool DcmQueryRetrieveTelnetInitiator::TI_remoteFindQuery(TI_DBEntry *db, DcmDataset *query,
    TI_GenericEntryCallbackFunction callbackFunction,
    TI_GenericCallbackStruct *callbackData)
{
    OFBool ok = OFTrue;
    TI_LocalFindCallbackData cbd;
    OFCondition           cond = EC_Normal;
    T_ASC_PresentationContextID presId;
    DIC_US        msgId;
    T_DIMSE_C_FindRQ  req;
    T_DIMSE_C_FindRSP rsp;
    DcmDataset    *stDetail = NULL;

    currentPeerTitle = db->title;

    /* make sure we have an association */
    ok = TI_changeAssociation();
    if (!ok) return OFFalse;

    cbd.cbf = callbackFunction;
    cbd.cbs = callbackData;
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
        query->print(COUT);
    }

    req.MessageID = msgId;
    strcpy(req.AffectedSOPClassUID,
     UID_FINDStudyRootQueryRetrieveInformationModel);
    req.Priority = DIMSE_PRIORITY_LOW;

    cond = DIMSE_findUser(assoc, presId, &req, query,
      findCallback, &cbd, blockMode_, dimse_timeout_, &rsp, &stDetail);

    if (cond.good()) {
        if (verbose) {
            DIMSE_printCFindRSP(stdout, &rsp);
        }
    } else {
        DcmQueryRetrieveOptions::errmsg("Find Failed:");
        DimseCondition::dump(cond);
    }
    if (stDetail != NULL) {
        printf("  Status Detail:\n");
        stDetail->print(COUT);
        delete stDetail;
    }

    return cond.good();

}


/* ====================================== TI USER INTERFACE ===================================== */


OFBool DcmQueryRetrieveTelnetInitiator::TI_shortHelp(int /*arg*/ , const char * /*cmdbuf*/ )
{
    printf("h)elp, t)itle, da)tabase, st)udy, ser)ies i)mage, di)splay, sen)d, e)cho, q)uit\n");
    return OFTrue;
}

OFBool DcmQueryRetrieveTelnetInitiator::TI_help(int arg, const char * /*cmdbuf*/ )
{
    if (verbose) {
        printf("TI_help: arg=%d\n", arg);
    }
    printf("Command Summary:\n");
    printf("help                list this summary\n");
    printf("?                   short help\n");
    printf("title [#]           list [set] current peer AE title\n");
    printf("database [#]        list [set] current database\n");
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

OFBool DcmQueryRetrieveTelnetInitiator::TI_title(int arg, const char * /*cmdbuf*/ )
{
    int i;
    TI_DBEntry *db;
    const char *peer;
    int port;
    DIC_AE peerTitle;

    if (verbose) {
        printf("TI_title: arg=%d\n", arg);
    }

    bzero(peerTitle, sizeof(peerTitle));
    if (assoc) {
        ASC_getAPTitles(assoc->params, NULL, peerTitle, NULL);
    }

    db = dbEntries[currentdb];
    if (arg < 0) {
        /* print list of peer AE titles we know */
        printf("Peer AE Titles:\n");
        printf("     %-16s %s\n", "Peer AE", "HostName:PortNumber");
        for (i=0; i<db->peerTitleCount; i++) {
            if (strcmp(currentPeerTitle, db->peerTitles[i]) == 0) {
                printf("*");
            } else {
                printf(" ");
            }
            /* active = (strcmp(peerTitle, db->peerTitles[i]) == 0); */
            config.peerForAETitle(db->peerTitles[i], &peer, &port);
            printf(" %d) %-16s (%s:%d)\n", i, db->peerTitles[i],
                peer, port);
        }
    } else {
        /* choosing new peer AE title */
        if (arg >= db->peerTitleCount) {
            printf("ERROR: Peer AE Title Choice: 0 - %d\n",
                db->peerTitleCount - 1);
        } else {
            currentPeerTitle = db->peerTitles[arg];
        }
    }
    return OFTrue;
}

OFBool DcmQueryRetrieveTelnetInitiator::TI_attachDB(TI_DBEntry *db)
{
    OFCondition dbcond = EC_Normal;

    db->studyCount = 0;
    db->currentStudy = 0;
    db->currentImage = 0;

    if (!db->isRemoteDB && db->dbHandle == NULL) {
        /* Create a database handle */
        db->dbHandle = new DcmQueryRetrieveIndexDatabaseHandle(
            config.getStorageArea(db->title), 
            config.getMaxStudies(db->title),
            config.getMaxBytesPerStudy(db->title), dbcond);
        if (dbcond.bad()) {
            DcmQueryRetrieveOptions::errmsg("TI_attachDB: cannot create DB Handle");
            return OFFalse;
        }
    } else {

    }

    return OFTrue;
}

OFBool DcmQueryRetrieveTelnetInitiator::TI_database(int arg, const char * /*cmdbuf*/ )
{
    int i;
    TI_DBEntry *db = NULL;
    OFBool found = OFFalse;

    if (verbose) {
        printf("TI_database: arg=%d\n", arg);
    }

    if (arg < 0) {
        /* print list of database titles we know */
        printf("Database Titles:\n");
        printf("     %s\n", "Database");
        for (i=0; i<dbCount; i++) {
            if (currentdb == i) {
                printf("*");
            } else {
                printf(" ");
            }
            if (dbEntries[i]->isRemoteDB) {
                printf("!");
            } else {
                printf(" ");
            }
            printf("%2d) %s\n", i, dbEntries[i]->title);
        }
    } else {
        /* choosing new title */
        if (arg >= dbCount) {
            printf("ERROR: Database Title Choice: 0 - %d\n",
                dbCount - 1);
        } else {
            /* release old dbHandle */
            TI_detachDB(dbEntries[currentdb]);

            currentdb = arg;
            /* check to make sure that current peer AE title is
             * available for this database, if not must choose
             * another and tell user about the change.
             */
            db = dbEntries[currentdb];
            for (i=0; !found && i<db->peerTitleCount; i++) {
                found = (strcmp(currentPeerTitle,
                          db->peerTitles[i]) == 0);
            }
            if (!found) {
                printf("WARNING: Actual Peer AE Title (%s) has no access to database: %s\n", currentPeerTitle, db->title);
                printf("         Setting Default Peer AE Title: %s\n",
                    db->peerTitles[0]);
                currentPeerTitle = db->peerTitles[0];
            }

            if (!TI_attachDB(dbEntries[currentdb]))
            {
                     DcmQueryRetrieveOptions::errmsg("ERROR: unable to open database, bailing out.\n");
                     exit(10);
            }
        }
    }

    return OFTrue;
}

OFBool DcmQueryRetrieveTelnetInitiator::TI_echo(int arg, const char * /*cmdbuf*/ )
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

OFBool DcmQueryRetrieveTelnetInitiator::TI_quit(int arg, const char * /*cmdbuf*/ )
{
    if (verbose) {
        printf("TI_quit: arg=%d\n", arg);
    }
    TI_detachAssociation(OFFalse);
    printf("Good Bye, Auf Wiedersehen, Au Revoir\n");
    exit(0);
    return OFTrue;
}

OFBool DcmQueryRetrieveTelnetInitiator::TI_actualizeStudies()
{
    TI_DBEntry *db;

    db = dbEntries[currentdb];

    /* get a list of all the available studies in the current database */
    if (!TI_buildStudies(db))
        return OFFalse;

    if (db->studyCount == 0) {
        printf("No Studies in Database: %s\n", db->title);
        return OFFalse;
    }

    if (db->currentStudy < 0 || db->currentStudy >= db->studyCount)
        db->currentStudy = 0;

    return OFTrue;
}

OFBool DcmQueryRetrieveTelnetInitiator::TI_study(int arg, const char * /*cmdbuf*/ )
{
    TI_DBEntry *db;
    TI_StudyEntry *se;
    int i;

    if (verbose) {
        printf("TI_study: arg=%d\n", arg);
    }

    db = dbEntries[currentdb];

    if (db->isRemoteDB) {
        currentPeerTitle = db->title;
        /* make sure we have an association */
        OFBool ok = TI_changeAssociation();
        if (!ok) return OFFalse;
    }

    if (!TI_actualizeStudies())
        return OFFalse;

#ifndef RETAIN_ASSOCIATION
    if (dbEntries[currentdb]->isRemoteDB) {
        TI_detachAssociation(OFFalse);
    }
#endif

    if (arg >= 0) {
        /* set current study */
        if (arg >= db->studyCount) {
            printf("ERROR: Study Choice: 0 - %d\n",
                db->studyCount - 1);
            return OFFalse;
        }
        db->currentStudy = arg;
        return OFTrue;
    }

    /* list studies to user */
    printf("      ");
    printf(STUDYFORMAT, "Patient", "PatientID", "StudyID");
    for (i=0; i<db->studyCount; i++) {
        if (db->currentStudy == i) {
            printf("*");
        } else {
            printf(" ");
        }
        printf(" %2d) ", i);
        se = db->studies[i];
        printStudyEntry(se);
    }

    printf("\n");
    printf("%d Studies in Database: %s\n", db->studyCount, db->title);
    return OFTrue;
}

OFBool DcmQueryRetrieveTelnetInitiator::TI_actualizeSeries()
{
    TI_DBEntry *db;
    TI_StudyEntry *study;

    db = dbEntries[currentdb];

    if (db->studyCount == 0)
        if (!TI_actualizeStudies())
            return OFFalse;

    study = db->studies[db->currentStudy];

    /* get a list of all the available series in the current study */
    if (!TI_buildSeries(db, study))
        return OFFalse;

    if (study->seriesCount == 0) {
        printf("No Series in Study %s (Database: %s)\n",
            study->studyID, db->title);
        return OFFalse;
    }
    if (db->currentSeries < 0 || db->currentSeries >= study->seriesCount)
        db->currentSeries = 0;

    return OFTrue;
}

OFBool DcmQueryRetrieveTelnetInitiator::TI_series(int arg, const char * /*cmdbuf*/ )
{
    TI_DBEntry *db;
    TI_StudyEntry *study;
    TI_SeriesEntry *series;
    int i;

    if (verbose) {
        printf("TI_sseries: arg=%d\n", arg);
    }

    db = dbEntries[currentdb];

    if (db->isRemoteDB) {
        currentPeerTitle = db->title;
        /* make sure we have an association */
        OFBool ok = TI_changeAssociation();
        if (!ok) return OFFalse;
    }

    if (!TI_actualizeSeries())
        return OFFalse;

#ifndef RETAIN_ASSOCIATION
    if (dbEntries[currentdb]->isRemoteDB) {
        TI_detachAssociation(OFFalse);
    }
#endif

    study = db->studies[db->currentStudy];

    if (arg >= 0) {
        /* set current series */
        if (arg >= study->seriesCount) {
            printf("ERROR: Series Choice: 0 - %d\n",
                study->seriesCount - 1);
            return OFFalse;
        }
        db->currentSeries = arg;
        return OFTrue;
    }

    /* list series to user */
    printf("      ");
    printf(SERIESFORMAT, "Series", "Modality", "SeriesInstanceUID");
    for (i=0; i<study->seriesCount; i++) {
        if (db->currentSeries == i) {
            printf("*");
        } else {
            printf(" ");
        }
        printf(" %2d) ", i);
        series = study->series[i];
        printSeriesEntry(series);
    }

    printf("\n");
    printf("%d Series in StudyID %s,\n",
        study->seriesCount, study->studyID);
    printf("  Patient: %s (Database: %s)\n",
        study->patientsName, db->title);
    return OFTrue;
}

OFBool DcmQueryRetrieveTelnetInitiator::TI_actualizeImages()
{
    TI_DBEntry *db;
    TI_StudyEntry *study;
    TI_SeriesEntry *series;

    db = dbEntries[currentdb];

    if (db->studyCount == 0) {
        if (!TI_actualizeStudies())
            return OFFalse;
    }

    study = db->studies[db->currentStudy];
    if (study->seriesCount == 0) {
        if (!TI_actualizeSeries())
            return OFFalse;
    }

    series = study->series[db->currentSeries];

    /* get a list of all the available images in the current series */
    if (!TI_buildImages(db, study, series))
        return OFFalse;

    if (series->imageCount == 0) {
        printf("No Images in Series %s, Study %s (Database: %s)\n",
            series->seriesNumber, study->studyID, db->title);
        return OFFalse;
    }
    return OFTrue;
}

OFBool DcmQueryRetrieveTelnetInitiator::TI_image(int arg, const char * /*cmdbuf*/ )
{
    TI_DBEntry *db;
    TI_StudyEntry *study;
    TI_SeriesEntry *series;
    TI_ImageEntry *image;
    int i;

    if (verbose) {
        printf("TI_image: arg=%d\n", arg);
    }

    db = dbEntries[currentdb];

    if (db->isRemoteDB) {
        currentPeerTitle = db->title;
        /* make sure we have an association */
        OFBool ok = TI_changeAssociation();
        if (!ok) return OFFalse;
    }

    if (!TI_actualizeImages())
        return OFFalse;

#ifndef RETAIN_ASSOCIATION
    if (dbEntries[currentdb]->isRemoteDB) {
        TI_detachAssociation(OFFalse);
    }
#endif

    study = db->studies[db->currentStudy];
    series = study->series[db->currentSeries];

    if (arg >= 0) {
        /* set current image */
        if (arg >= series->imageCount) {
            printf("ERROR: Image Choice: 0 - %d\n",
                series->imageCount - 1);
            return OFFalse;
        }
        db->currentImage = arg;
        return OFTrue;
    }

    /* list images to user */
    printf("      ");
    printf(IMAGEFORMAT, "Image", "ImageInstanceUID");
    for (i=0; i<series->imageCount; i++) {
        if (db->currentImage == i) {
            printf("*");
        } else {
            printf(" ");
        }
        printf(" %2d) ", i);
        image = series->images[i];
        printImageEntry(image);
    }

    printf("\n");
    printf("%d Images in %s Series, StudyID %s,\n",
        series->imageCount, series->modality, study->studyID);
    printf("  Patient: %s (Database: %s)\n",
        study->patientsName, db->title);
    return OFTrue;
}

OFBool DcmQueryRetrieveTelnetInitiator::TI_sendStudy(int arg, const char * /*cmdbuf*/ )
{
    OFBool ok = OFTrue;
    DcmDataset *query = NULL;
    TI_DBEntry *db;
    TI_StudyEntry *study;
    OFCondition dbcond = EC_Normal;
    DcmQueryRetrieveDatabaseStatus dbStatus(STATUS_Pending);
    DIC_UI sopClass;
    DIC_UI sopInstance;
    char imgFile[MAXPATHLEN+1];
    DIC_US nRemaining = 0;

    if (verbose) {
        printf("TI_sendStudy: arg=%d\n", arg);
    }

    db = dbEntries[currentdb];

    /*
    ** We cannot read images from a DB and send images to the same DB
    ** over the network because of deadlock.  The DB move routines
    ** lock the index file.  When we send over the network to the same
    ** DB it tries to lock the index file exclusively to insert the image
    ** in the database.  We end up waiting for a response from the remote
    ** peer which never comes.
    */

    if (strcmp(db->title, currentPeerTitle) == 0) {
        printf("Sorry, cannot send images from a DB to itself, possible deadlock\n");
        return OFFalse;
    }

    /* make sure study info is actual */
    ok = TI_actualizeStudies();
    if (!ok) return OFFalse;

    /* set arg as current study */
    if (arg < 0) {
        arg = db->currentStudy;
    }

    if (arg >= db->studyCount) {
        printf("ERROR: Study Choice: 0 - %d\n",
            db->studyCount - 1);
        return OFFalse;
    }
    study = db->studies[arg];

    /* make sure we have an association */
    ok = TI_changeAssociation();
    if (!ok) return OFFalse;

    /* fabricate query */
    query = new DcmDataset;
    if (query == NULL) {
        DcmQueryRetrieveOptions::errmsg("Help, out of memory");
        return OFFalse;
    }
    DU_putStringDOElement(query, DCM_QueryRetrieveLevel, "STUDY");
    DU_putStringDOElement(query, DCM_StudyInstanceUID, study->studyInstanceUID);

    dbcond = db->dbHandle->startMoveRequest(
        UID_MOVEStudyRootQueryRetrieveInformationModel, query, &dbStatus);
    delete query; query = NULL;
    if (dbcond.bad()) {
        DcmQueryRetrieveOptions::errmsg("TI_sendStudy: cannot query database");
        return OFFalse;
    }

    while (ok && dbStatus.status() == STATUS_Pending) {
        dbcond = db->dbHandle->nextMoveResponse(sopClass, sopInstance,
            imgFile, &nRemaining, &dbStatus);
        if (dbcond.bad()) {
            DcmQueryRetrieveOptions::errmsg("TI_sendStudy: database error");
            return OFFalse;
        }
        if (dbStatus.status() == STATUS_Pending) {

            ok = TI_storeImage(sopClass, sopInstance, imgFile);
            if (!ok) {
                db->dbHandle->cancelMoveRequest(&dbStatus);
            }
        }
    }

    ok = TI_detachAssociation(OFFalse);

    return ok;
}


OFBool DcmQueryRetrieveTelnetInitiator::TI_sendSeries(int arg, const char * /*cmdbuf*/ )
{
    OFBool ok = OFTrue;
    DcmDataset *query = NULL;
    TI_DBEntry *db;
    TI_StudyEntry *study;
    TI_SeriesEntry *series;
    OFCondition dbcond = EC_Normal;
    DcmQueryRetrieveDatabaseStatus dbStatus(STATUS_Pending);
    DIC_UI sopClass;
    DIC_UI sopInstance;
    char imgFile[MAXPATHLEN+1];
    DIC_US nRemaining = 0;

    if (verbose) {
        printf("TI_sendSeries: arg=%d\n", arg);
    }

    db = dbEntries[currentdb];

    /* make sure study/series info is actual */
    ok = TI_actualizeSeries();
    if (!ok) return OFFalse;

    study = db->studies[db->currentStudy];

    /* set arg as current series */
    if (arg < 0) {
        arg = db->currentSeries;
    }

    if (arg >= study->seriesCount) {
        printf("ERROR: Series Choice: 0 - %d\n",
            study->seriesCount - 1);
        return OFFalse;
    }
    series = study->series[arg];

    /* make sure we have an association */
    ok = TI_changeAssociation();
    if (!ok) return OFFalse;

    /* fabricate query */
    query = new DcmDataset;
    if (query == NULL) {
        DcmQueryRetrieveOptions::errmsg("Help, out of memory");
        return OFFalse;
    }
    DU_putStringDOElement(query, DCM_QueryRetrieveLevel, "SERIES");
    DU_putStringDOElement(query, DCM_StudyInstanceUID, study->studyInstanceUID);
    DU_putStringDOElement(query, DCM_SeriesInstanceUID,
        series->seriesInstanceUID);

    dbcond = db->dbHandle->startMoveRequest(
        UID_MOVEStudyRootQueryRetrieveInformationModel, query, &dbStatus);
    delete query; query = NULL;
    if (dbcond.bad()) {
        DcmQueryRetrieveOptions::errmsg("TI_sendSeries: cannot query database");
        return OFFalse;
    }

    while (ok && dbStatus.status() == STATUS_Pending) {
        dbcond = db->dbHandle->nextMoveResponse(sopClass, sopInstance,
            imgFile, &nRemaining, &dbStatus);
        if (dbcond.bad()) {
            DcmQueryRetrieveOptions::errmsg("TI_sendSeries: database error");
            return OFFalse;
        }
        if (dbStatus.status() == STATUS_Pending) {

            ok = TI_storeImage(sopClass, sopInstance, imgFile);
            if (!ok) {
                db->dbHandle->cancelMoveRequest(&dbStatus);
            }
        }
    }

    ok = TI_detachAssociation(OFFalse);

    return ok;
}

OFBool DcmQueryRetrieveTelnetInitiator::TI_sendImage(int arg, const char * /*cmdbuf*/ )
{
    OFBool ok = OFTrue;
    DcmDataset *query = NULL;
    TI_DBEntry *db;
    TI_StudyEntry *study;
    TI_SeriesEntry *series;
    TI_ImageEntry *image;
    OFCondition dbcond = EC_Normal;
    DcmQueryRetrieveDatabaseStatus dbStatus(STATUS_Pending);
    DIC_UI sopClass;
    DIC_UI sopInstance;
    char imgFile[MAXPATHLEN+1];
    DIC_US nRemaining = 0;

    if (verbose) {
        printf("TI_sendImage: arg=%d\n", arg);
    }

    db = dbEntries[currentdb];

    /* make sure study/series/image info is actual */
    ok = TI_actualizeImages();
    if (!ok) return OFFalse;

    study = db->studies[db->currentStudy];
    series = study->series[db->currentSeries];

    /* set arg as current image */
    if (arg < 0) {
        arg = db->currentImage;
    }

    if (arg >= series->imageCount) {
        printf("ERROR: Image Choice: 0 - %d\n",
            series->imageCount - 1);
        return OFFalse;
    }

    image = series->images[arg];

    /* make sure we have an association */
    ok = TI_changeAssociation();
    if (!ok) return OFFalse;

    /* fabricate query */
    query = new DcmDataset;
    if (query == NULL) {
        DcmQueryRetrieveOptions::errmsg("Help, out of memory");
        return OFFalse;
    }
    DU_putStringDOElement(query, DCM_QueryRetrieveLevel, "IMAGE");
    DU_putStringDOElement(query, DCM_StudyInstanceUID, study->studyInstanceUID);
    DU_putStringDOElement(query, DCM_SeriesInstanceUID,
        series->seriesInstanceUID);
    DU_putStringDOElement(query, DCM_SOPInstanceUID,
        image->sopInstanceUID);

    dbcond = db->dbHandle->startMoveRequest(
        UID_MOVEStudyRootQueryRetrieveInformationModel, query, &dbStatus);
    delete query; query = NULL;
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
        dbcond = db->dbHandle->nextMoveResponse(sopClass, sopInstance,
      imgFile, &nRemaining, &dbStatus);
        if (dbcond.bad()) {
            DcmQueryRetrieveOptions::errmsg("TI_sendImage: database error");
            return OFFalse;
        }
        if (dbStatus.status() == STATUS_Pending) {

            ok = TI_storeImage(sopClass, sopInstance, imgFile);
            if (!ok) {
                db->dbHandle->cancelMoveRequest(&dbStatus);
            }
        }
    }

    ok = TI_detachAssociation(OFFalse);

    return ok;
}

OFBool DcmQueryRetrieveTelnetInitiator::TI_send(int /*arg*/, const char *cmdbuf)
{
    OFBool ok = OFTrue;
    char cmdarg[128];
    int iarg;
    int narg;

    if (dbEntries[currentdb]->isRemoteDB) {
        printf("Sorry, cannot send from remote DB\n");
        return OFTrue;
    }

    bzero(cmdarg, sizeof(cmdarg));

    narg = sscanf(cmdbuf, "send %s %d", cmdarg, &iarg);
    if (narg == 1)
        iarg = -1;

    if      (strncmp("st", cmdarg, strlen("st")) == 0) TI_sendStudy(iarg, cmdbuf);
    else if (strncmp("se", cmdarg, strlen("se")) == 0) TI_sendSeries(iarg, cmdbuf);
    else if (strncmp("i",  cmdarg, strlen("i"))  == 0) TI_sendImage(iarg, cmdbuf);
    else {
        printf("What do you want to send? Type help for help\n");
    }

    return ok;
}

void DcmQueryRetrieveTelnetInitiator::TI_userInput()
{
    char cmdBuf[1024];  /* can't have lines longer than this */
    int arg;

    /* make the first database current */
    currentdb = 0;
    /* make the first peer title for this database current */
    currentPeerTitle = dbEntries[currentdb]->peerTitles[0];
    /* open db */
    TI_database(currentdb, cmdBuf);

    TI_welcome();
    printf("\n");

    while (1) {
        printf("%s->%s> ", dbEntries[currentdb]->title,
            currentPeerTitle);
        if (fgets(cmdBuf, 1024, stdin) == NULL) {
            DcmQueryRetrieveOptions::errmsg("unexpected end of input\n");
            return; /* give up */
        }

        DU_stripLeadingSpaces(cmdBuf);
        if (strlen(cmdBuf) == 0)
            continue; /* no input */

        if (sscanf(cmdBuf, "%*s %d", &arg) != 1) {
            arg = -1;
        }

        /* find command parser */
        if      (strncmp("h",  cmdBuf, strlen("h"))  == 0) TI_help(arg, cmdBuf);
        else if (strncmp("?",  cmdBuf, strlen("?"))  == 0) TI_shortHelp(arg, cmdBuf);
        else if (strncmp("t",  cmdBuf, strlen("t"))  == 0) TI_title(arg, cmdBuf);
        else if (strncmp("da", cmdBuf, strlen("da")) == 0) TI_database(arg, cmdBuf);
        else if (strncmp("st", cmdBuf, strlen("st")) == 0) TI_study(arg, cmdBuf);
        else if (strncmp("ser",cmdBuf, strlen("ser"))== 0) TI_series(arg, cmdBuf);
        else if (strncmp("i",  cmdBuf, strlen("i"))  == 0) TI_image(arg, cmdBuf);
        else if (strncmp("send", cmdBuf, strlen("send")) == 0) TI_send(arg, cmdBuf);
        else if (strncmp("ec", cmdBuf, strlen("ec")) == 0) TI_echo(arg, cmdBuf);
        else if (strncmp("q",  cmdBuf, strlen("q"))  == 0) TI_quit(arg, cmdBuf);
        else if (strncmp("exit", cmdBuf, strlen("exit")) == 0) TI_quit(arg, cmdBuf);
        else {
            printf("What do you want to do? Type help for help\n");
        }
    }
}

/* ========================================== TI QUERY ========================================== */


OFBool DcmQueryRetrieveTelnetInitiator::TI_dbReadable(const char *dbTitle)
{
    char path[MAXPATHLEN+1];
    sprintf(path, "%s%c%s", config.getStorageArea(dbTitle), PATH_SEPARATOR, DBINDEXFILE);

    return (access(path, R_OK) == 0);
}

time_t DcmQueryRetrieveTelnetInitiator::TI_dbModifyTime(const char *dbTitle)
{
    char path[MAXPATHLEN+1];
    struct stat s;

    sprintf(path, "%s%c%s", config.getStorageArea(dbTitle), PATH_SEPARATOR, DBINDEXFILE);

    if (stat(path, &s) < 0) {
        DcmQueryRetrieveOptions::errmsg("cannot stat: %s", path);
        return 0;
    }
    return s.st_mtime;
}

/*
 * Study Level
 */

OFBool DcmQueryRetrieveTelnetInitiator::TI_buildRemoteStudies(TI_DBEntry *db)
{
    TI_GenericCallbackStruct cbs;
    DcmDataset *query = NULL;
    OFBool ok = OFTrue;

    cbs.db = db;
    cbs.study = NULL; cbs.series = NULL;

    TI_destroyStudyEntries(db);

    /* get all known studies */
    TI_buildStudyQuery(&query);

    ok = TI_remoteFindQuery(db, query, TI_genericEntryCallback, &cbs);

    delete query;

    return ok;
}

OFBool DcmQueryRetrieveTelnetInitiator::TI_buildStudies(TI_DBEntry *db)
{
    OFCondition dbcond = EC_Normal;
    DcmQueryRetrieveDatabaseStatus dbStatus(STATUS_Pending);
    DcmDataset *query = NULL;
    DcmDataset *reply = NULL;

    if (db->isRemoteDB) {
        return TI_buildRemoteStudies(db);
    }

    if (db->studyCount != 0 &&
        TI_dbModifyTime(db->title) < db->lastQueryTime) {
        /* nothing has changed */
        return OFTrue;
    }

    TI_destroyStudyEntries(db);

    /* get all known studies */
    TI_buildStudyQuery(&query);

    printf("Querying Database for Studies ...\n");
    db->lastQueryTime = time(NULL);

    dbcond = db->dbHandle->startFindRequest(
        UID_FINDStudyRootQueryRetrieveInformationModel, query, &dbStatus);
    if (dbcond.bad()) {
        DcmQueryRetrieveOptions::errmsg("TI_buildStudies: cannot query database");
        delete query;
        return OFFalse;
    }
    
    dbStatus.deleteStatusDetail();

    while (dbStatus.status() == STATUS_Pending) {
        dbcond = db->dbHandle->nextFindResponse(&reply, &dbStatus);
        if (dbcond.bad()) {
            DcmQueryRetrieveOptions::errmsg("TI_buildStudies: database error");
            return OFFalse;
        }
        if (dbStatus.status() == STATUS_Pending) {
            TI_addStudyEntry(db, reply);
            delete reply;
            reply = NULL;
        }
    }

    delete query;

    return OFTrue;
}

/*
 * Series Level
 */


OFBool DcmQueryRetrieveTelnetInitiator::TI_buildRemoteSeries(TI_DBEntry *db, TI_StudyEntry *study)
{
    TI_GenericCallbackStruct cbs;
    DcmDataset *query = NULL;
    OFBool ok = OFTrue;

    cbs.db = NULL;
    cbs.study = study; cbs.series = NULL;

    TI_destroySeriesEntries(study);

    /* get all known studies */
    TI_buildSeriesQuery(&query, study);

    ok = TI_remoteFindQuery(db, query, TI_genericEntryCallback, &cbs);

    delete query;

    return ok;
}

OFBool DcmQueryRetrieveTelnetInitiator::TI_buildSeries(TI_DBEntry *db, TI_StudyEntry *study)
{
    OFCondition dbcond = EC_Normal;
    DcmQueryRetrieveDatabaseStatus dbStatus(STATUS_Pending);
    DcmDataset *query = NULL;
    DcmDataset *reply = NULL;

    if (db->isRemoteDB) {
        return TI_buildRemoteSeries(db, study);
    }

    if (study->seriesCount != 0 &&
        TI_dbModifyTime(db->title) < db->lastQueryTime) {
        /* nothing has changed */
        return OFTrue;
    }

    TI_destroySeriesEntries(study);

    /* get all known series for this study */
    TI_buildSeriesQuery(&query, study);

    printf("Querying Database for Series ...\n");
    study->lastQueryTime = time(NULL);

    dbcond = db->dbHandle->startFindRequest(
        UID_FINDStudyRootQueryRetrieveInformationModel, query, &dbStatus);
    if (dbcond.bad()) {
        DcmQueryRetrieveOptions::errmsg("TI_buildSeries: cannot query database");
        delete query; query = NULL;
        return OFFalse;
    }
    
    dbStatus.deleteStatusDetail();

    while (dbStatus.status() == STATUS_Pending) {
        dbcond = db->dbHandle->nextFindResponse(&reply, &dbStatus);
        if (dbcond.bad()) {
            DcmQueryRetrieveOptions::errmsg("TI_buildSeries: database error");
            return OFFalse;
        }
        if (dbStatus.status() == STATUS_Pending) {
            TI_addSeriesEntry(study, reply);
            delete reply;
            reply = NULL;
        }
    }

    delete query;
    query = NULL;

    if (study->seriesCount > 0) {
        /* sort the seriesinto assending series number order */
        qsort(study->series, study->seriesCount, sizeof(study->series[0]),
              TI_seriesCompare);
    }

    return OFTrue;
}

/*
 * Image Level
 */


OFBool DcmQueryRetrieveTelnetInitiator::TI_buildRemoteImages(TI_DBEntry *db, TI_StudyEntry *study, TI_SeriesEntry *series)
{
    TI_GenericCallbackStruct cbs;
    DcmDataset *query = NULL;
    OFBool ok = OFTrue;

    cbs.db = NULL;
    cbs.study = NULL; cbs.series = series;

    TI_destroyImageEntries(series);

    /* get all known studies */
    TI_buildImageQuery(&query, study, series);

    ok = TI_remoteFindQuery(db, query, TI_genericEntryCallback, &cbs);

    delete query;

    return ok;
}

OFBool DcmQueryRetrieveTelnetInitiator::TI_buildImages(TI_DBEntry *db, TI_StudyEntry *study, TI_SeriesEntry *series)
{
    OFCondition dbcond = EC_Normal;
    DcmQueryRetrieveDatabaseStatus dbStatus(STATUS_Pending);
    DcmDataset *query = NULL;
    DcmDataset *reply = NULL;

    if (db->isRemoteDB) {
        return TI_buildRemoteImages(db, study, series);
    }

    if (series->imageCount != 0 &&
        TI_dbModifyTime(db->title) < study->lastQueryTime) {
        /* nothing has changed */
        return OFTrue;
    }

    TI_destroyImageEntries(series);

    /* get all known images in current series */
    TI_buildImageQuery(&query, study, series);

    if (verbose) {
        printf("QUERY OBJECT:\n");
        query->print(COUT);
    }

    printf("Querying Database for Images ...\n");
    series->lastQueryTime = time(NULL);

    dbcond = db->dbHandle->startFindRequest(
        UID_FINDStudyRootQueryRetrieveInformationModel, query, &dbStatus);
    delete query; query = NULL;
    if (dbcond.bad()) {
        DcmQueryRetrieveOptions::errmsg("TI_buildImages: cannot query database");
        return OFFalse;
    }

    while (dbStatus.status() == STATUS_Pending) {
        dbcond = db->dbHandle->nextFindResponse(&reply, &dbStatus);
        if (dbcond.bad()) {
            DcmQueryRetrieveOptions::errmsg("TI_buildImages: database error");
            return OFFalse;
        }
        if (dbStatus.status() == STATUS_Pending) {
            if (verbose) {
                printf("REPLY OBJECT:\n");
                reply->print(COUT);
            }
            TI_addImageEntry(series, reply);
            delete reply; reply = NULL;
        }
    }

    if (series->imageCount > 0) {
        /* sort the images into assending image number order */
        qsort(series->images, series->imageCount, sizeof(series->images[0]),
              TI_imageCompare);
    }

    return OFTrue;
}


OFBool DcmQueryRetrieveTelnetInitiator::addPeerName(const char *peerName, const char *configFileName)
{
  int k;
  OFBool found = OFFalse;
  const char **aeTitles;

  if (peerNamesCount == TI_MAXPEERS)
    return OFFalse;

  for( k=0; !found && k<peerNamesCount ; k++ )
    found = ( strcmp( peerNames[k], peerName ) == 0 );

  if( found )
    return( OFTrue );

  if( config.aeTitlesForPeer( peerName, &aeTitles ) <= 0 )
  {
    DcmQueryRetrieveOptions::errmsg("no AE titles defined (in: %s) for peer: %s", configFileName, peerName);
    return( OFFalse );
  }

  free( aeTitles );

  peerNames[ peerNamesCount ] = (char*)peerName;
  peerNamesCount++;

  return( OFTrue );
}

void DcmQueryRetrieveTelnetInitiator::printConfig()
{
  int i,j;

  printf("TI Configuration:\n");
  printf("My AE Title: %s\n", myAETitle );
  printf("DatabaseTitles    Peer AE Titles\n");

  for( i=0 ; i<dbCount ; i++ )
  {
    printf("%-18s", dbEntries[i]->title );

    for( j=0 ; j<dbEntries[i]->peerTitleCount ; j++ )
      printf("%s ", dbEntries[i]->peerTitles[j] );

    printf("\n");
  }
}

OFBool DcmQueryRetrieveTelnetInitiator::findDBPeerTitles(
  const char *configFileName,
  TI_DBEntry *dbEntry, 
  const char *peer)
{
  const char **peerTitles = NULL;
  int peerTitleCount = 0;
  int i;

  // discover all known AETitles for peer
  peerTitleCount = config.aeTitlesForPeer( peer,&peerTitles );
  if( peerTitleCount <= 0 )
  {
    DcmQueryRetrieveOptions::errmsg("no AE titles defined (in: %s) for peer: %s", configFileName, peer);
    return( OFFalse );
  }

  // check to make sure peer+AEtitle has access to database areas
  for( i=0 ; i<peerTitleCount ; i++ )
  {
    if( config.peerInAETitle( dbEntry->title, peerTitles[i], peer ) )
    {
      // add peer title to database's peer title list
      if( dbEntry->peerTitles == NULL )
        dbEntry->peerTitles = (const char**) malloc( sizeof( const char* ) );
      else
        dbEntry->peerTitles = (const char**) realloc( dbEntry->peerTitles, (dbEntry->peerTitleCount + 1) * sizeof(const char*) );

      dbEntry->peerTitles[ dbEntry->peerTitleCount ] = peerTitles[i];
      dbEntry->peerTitleCount++;
    }
  }

  // throw away the old list
  free( peerTitles );

  return( dbEntry->peerTitleCount > 0 );
}


void DcmQueryRetrieveTelnetInitiator::createConfigEntries(
  const char *configFileName,
  int remoteDBTitlesCount,
  const char **remoteDBTitles)
{
  const char **ctnTitles = NULL;
  int ctnTitleCount = 0;
  int i, j;
  TI_DBEntry *dbEntry = NULL;

  dbCount = 0;

  // discover all the known CTN AE titles
  ctnTitleCount = config.ctnTitles(&ctnTitles);
  for( i=0 ; i<ctnTitleCount ; i++ )
  {
    if( !TI_dbReadable( ctnTitles[i] ) )
      DcmQueryRetrieveOptions::errmsg("Warning: db does not exist: %s", ctnTitles[i]);
    else
    {
      dbEntry = (TI_DBEntry*) malloc( sizeof(TI_DBEntry) );
      bzero( (char*)dbEntry, sizeof(*dbEntry) );
      dbEntry->title = ctnTitles[i];

      for( j=0 ; j<peerNamesCount ; j++ )
        findDBPeerTitles(configFileName, dbEntry, peerNames[j]);

      if( dbEntry->peerTitleCount > 0 )
      {
        // add database to list, it is accessable by something
        if( dbEntries == NULL )
          dbEntries = (TI_DBEntry**) malloc( sizeof( TI_DBEntry* ) );
        else
          dbEntries = (TI_DBEntry**) realloc( dbEntries, (dbCount + 1) * sizeof(TI_DBEntry*) );

        dbEntries[ dbCount ] = dbEntry;
        dbCount++;
      }
      else
        free( dbEntry );
    }
  }

  // throw away the old lists
  free( ctnTitles );

  // add any remote DB entries
  for( i=0 ; i<remoteDBTitlesCount ; i++ )
  {
    const char *peerName = NULL;
    int portNumber;
    if( config.peerForAETitle( remoteDBTitles[i], &peerName, &portNumber ) )
    {
      // add DB
      dbEntry = (TI_DBEntry*) malloc( sizeof( TI_DBEntry ) );
      bzero( (char*)dbEntry, sizeof(*dbEntry) );
      dbEntry->title = remoteDBTitles[i];
      dbEntry->isRemoteDB = OFTrue;

      if( dbEntries == NULL )
        dbEntries = (TI_DBEntry**) malloc( sizeof( TI_DBEntry* ) );
      else
        dbEntries = (TI_DBEntry**) realloc( dbEntries, (dbCount + 1) * sizeof(TI_DBEntry*) );

      dbEntries[ dbCount ] = dbEntry;
      dbCount++;

      for( j=0 ; j<peerNamesCount ; j++ )
      {
        const char **peerTitles = NULL;
        int peerTitleCount = 0;
        int k;

        peerTitleCount = config.aeTitlesForPeer( peerNames[j], &peerTitles );
        if( peerTitleCount <= 0 )
          DcmQueryRetrieveOptions::errmsg("no AE titles defined (in: %s) for peer: %s", configFileName, peerNames[j]);

        for( k=0 ; k<peerTitleCount ; k++ )
        {
          // add peer title to database's peer title list
          if( dbEntry->peerTitles == NULL )
            dbEntry->peerTitles = (const char**) malloc( sizeof( const char* ) );
          else
            dbEntry->peerTitles = (const char**) realloc( dbEntry->peerTitles, (dbEntry->peerTitleCount + 1) * sizeof(const char*) );

          dbEntry->peerTitles[ dbEntry->peerTitleCount ] = peerTitles[k];
          dbEntry->peerTitleCount++;
        }

        // throw away the old list
        free( peerTitles );
      }
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
