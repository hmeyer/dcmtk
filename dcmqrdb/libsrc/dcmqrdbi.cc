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
 *  Purpose: classes DcmQueryRetrieveIndexDatabaseHandle, DcmQueryRetrieveIndexDatabaseHandleFactory
 *
 *  Last Update:      $Author: onken $
 *  Update Date:      $Date: 2005/12/16 09:16:08 $
 *  Source File:      $Source: /share/dicom/cvs-depot/dcmtk/dcmqrdb/libsrc/dcmqrdbi.cc,v $
 *  CVS/RCS Revision: $Revision: 1.9 $
 *  Status:           $State: Exp $
 *
 *  CVS/RCS Log at end of file
 *
 */

#include "dcmtk/config/osconfig.h"    /* make sure OS specific configuration is included first */

BEGIN_EXTERN_C
#ifdef HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif
#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif
#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif
END_EXTERN_C

#define INCLUDE_CCTYPE
#define INCLUDE_CSTDARG
#include "dcmtk/ofstd/ofstdinc.h"

#include "dcmtk/dcmqrdb/dcmqrdbs.h"
#include "dcmtk/dcmqrdb/dcmqrdbi.h"
#include "dcmtk/dcmqrdb/dcmqrcnf.h"

#include "dcmtk/dcmqrdb/dcmqridx.h"
#include "dcmtk/dcmnet/diutil.h"
#include "dcmtk/dcmdata/dcfilefo.h"
#include "dcmtk/ofstd/ofstd.h"

const OFConditionConst DcmQRIndexDatabaseErrorC(OFM_imagectn, 0x001, OF_error, "DcmQR Index Database Error");
const OFCondition DcmQRIndexDatabaseError(DcmQRIndexDatabaseErrorC);

/* ========================= static data ========================= */

/**** The TbFindAttr table contains the description of tags (keys) supported
 **** by the DB Module.
 **** Tags described here have to be present in the Index Record file.
 **** The order is unsignificant.
 ****
 **** Each element of this table is described by
 ****           The tag value
 ****           The level of this tag (from patient to image)
 ****           The Key Type (only UNIQUE_KEY values is used)
 ****           The key matching type, specifiing which type of
 ****                   matching should be performed. The OTHER_CLASS
 ****                   value specifies that only strict comparison is applied.
 ****
 **** This table and the IndexRecord structure should contain at least
 **** all Unique and Required keys.
 ***/

static const DB_FindAttr TbFindAttr [] = {
        DB_FindAttr( DCM_PatientsBirthDate ,                    PATIENT_LEVEL,  OPTIONAL_KEY,   DATE_CLASS      ),
        DB_FindAttr( DCM_PatientsSex,                           PATIENT_LEVEL,  OPTIONAL_KEY,   STRING_CLASS    ),
        DB_FindAttr( DCM_PatientsName,                          PATIENT_LEVEL,  REQUIRED_KEY,   STRING_CLASS    ),
        DB_FindAttr( DCM_PatientID,                             PATIENT_LEVEL,  UNIQUE_KEY,     STRING_CLASS    ),
        DB_FindAttr( DCM_PatientsBirthTime,                     PATIENT_LEVEL,  OPTIONAL_KEY,   TIME_CLASS      ),
        DB_FindAttr( DCM_OtherPatientIDs,                       PATIENT_LEVEL,  OPTIONAL_KEY,   STRING_CLASS    ),
        DB_FindAttr( DCM_OtherPatientNames,                     PATIENT_LEVEL,  OPTIONAL_KEY,   STRING_CLASS    ),
        DB_FindAttr( DCM_EthnicGroup,                           PATIENT_LEVEL,  OPTIONAL_KEY,   STRING_CLASS    ),
        DB_FindAttr( DCM_PatientComments,                       PATIENT_LEVEL,  OPTIONAL_KEY,   STRING_CLASS    ),
        DB_FindAttr( DCM_NumberOfPatientRelatedStudies,         PATIENT_LEVEL,  OPTIONAL_KEY,   STRING_CLASS    ),
        DB_FindAttr( DCM_NumberOfPatientRelatedSeries,          PATIENT_LEVEL,  OPTIONAL_KEY,   STRING_CLASS    ),
        DB_FindAttr( DCM_NumberOfPatientRelatedInstances,       PATIENT_LEVEL,  OPTIONAL_KEY,   STRING_CLASS    ),
        DB_FindAttr( DCM_StudyDate,                             STUDY_LEVEL,    REQUIRED_KEY,   DATE_CLASS      ),
        DB_FindAttr( DCM_StudyTime,                             STUDY_LEVEL,    REQUIRED_KEY,   TIME_CLASS      ),
        DB_FindAttr( DCM_StudyID,                               STUDY_LEVEL,    REQUIRED_KEY,   STRING_CLASS    ),
        DB_FindAttr( DCM_AccessionNumber,                       STUDY_LEVEL,    REQUIRED_KEY,   STRING_CLASS    ),
        DB_FindAttr( DCM_ReferringPhysiciansName,               STUDY_LEVEL,    OPTIONAL_KEY,   STRING_CLASS    ),
        DB_FindAttr( DCM_StudyDescription,                      STUDY_LEVEL,    OPTIONAL_KEY,   STRING_CLASS    ),
        DB_FindAttr( DCM_NameOfPhysiciansReadingStudy,          STUDY_LEVEL,    OPTIONAL_KEY,   STRING_CLASS    ),
        DB_FindAttr( DCM_StudyInstanceUID,                      STUDY_LEVEL,    UNIQUE_KEY,     UID_CLASS       ),
        DB_FindAttr( DCM_OtherStudyNumbers,                     STUDY_LEVEL,    OPTIONAL_KEY,   OTHER_CLASS     ),
        DB_FindAttr( DCM_AdmittingDiagnosesDescription,         STUDY_LEVEL,    OPTIONAL_KEY,   STRING_CLASS    ),
        DB_FindAttr( DCM_PatientsAge,                           STUDY_LEVEL,    OPTIONAL_KEY,   STRING_CLASS    ),
        DB_FindAttr( DCM_PatientsSize,                          STUDY_LEVEL,    OPTIONAL_KEY,   OTHER_CLASS     ),
        DB_FindAttr( DCM_PatientsWeight,                        STUDY_LEVEL,    OPTIONAL_KEY,   OTHER_CLASS     ),
        DB_FindAttr( DCM_Occupation,                            STUDY_LEVEL,    OPTIONAL_KEY,   STRING_CLASS    ),
        DB_FindAttr( DCM_AdditionalPatientHistory,              STUDY_LEVEL,    OPTIONAL_KEY,   STRING_CLASS    ),
        DB_FindAttr( DCM_NumberOfStudyRelatedSeries,            STUDY_LEVEL,    OPTIONAL_KEY,   OTHER_CLASS     ),
        DB_FindAttr( DCM_NumberOfStudyRelatedInstances,         STUDY_LEVEL,    OPTIONAL_KEY,   OTHER_CLASS     ),
        DB_FindAttr( DCM_SeriesNumber,                          SERIE_LEVEL,    REQUIRED_KEY,   OTHER_CLASS     ),
        DB_FindAttr( DCM_SeriesInstanceUID,                     SERIE_LEVEL,    UNIQUE_KEY,     UID_CLASS       ),
        DB_FindAttr( DCM_Modality,                              SERIE_LEVEL,    OPTIONAL_KEY,   STRING_CLASS    ),
        DB_FindAttr( DCM_InstanceNumber,                        IMAGE_LEVEL,    REQUIRED_KEY,   OTHER_CLASS     ),
        DB_FindAttr( DCM_SOPInstanceUID,                        IMAGE_LEVEL,    UNIQUE_KEY,     UID_CLASS       )
  };

/**** The NbFindAttr variable contains the length of the TbFindAttr table
 ***/

static int NbFindAttr = ((sizeof (TbFindAttr)) / (sizeof (TbFindAttr [0])));

/* ========================= static functions ========================= */

static char *DB_strdup(const char* str)
{
    if (str == NULL) return NULL;
    char* s = (char*)malloc(strlen(str)+1);
    strcpy(s, str);
    return s;
}

/************
**      Add UID in Index Record to the UID found list
 */

static void DB_UIDAddFound (
                DB_Private_Handle       *phandle,
                IdxRecord               *idxRec
                )
{
    DB_UidList *plist ;

    plist = (DB_UidList *) malloc (sizeof (DB_UidList)) ;
    if (plist == NULL) {
        CERR << "DB_UIDAddFound: out of memory" << endl;
        return;
    }
    plist->next = phandle->uidList ;
    plist->patient = NULL ;
    plist->study = NULL ;
    plist->serie = NULL ;
    plist->image = NULL ;

    if ((int)phandle->queryLevel >= PATIENT_LEVEL)
        plist->patient = DB_strdup ((char *) idxRec->PatientID) ;
    if ((int)phandle->queryLevel >= STUDY_LEVEL)
        plist->study = DB_strdup ((char *) idxRec->StudyInstanceUID) ;
    if ((int)phandle->queryLevel >= SERIE_LEVEL)
        plist->serie = DB_strdup ((char *) idxRec->SeriesInstanceUID) ;
    if ((int)phandle->queryLevel >= IMAGE_LEVEL)
        plist->image = DB_strdup ((char *) idxRec->SOPInstanceUID) ;

    phandle->uidList = plist ;
}


/************
**      Search if an Index Record has already been found
 */

static int DB_UIDAlreadyFound (
                DB_Private_Handle       *phandle,
                IdxRecord               *idxRec
                )
{
    DB_UidList *plist ;

    for (plist = phandle->uidList ; plist ; plist = plist->next) {
        if (  ((int)phandle->queryLevel >= PATIENT_LEVEL)
              && (strcmp (plist->patient, (char *) idxRec->PatientID) != 0)
            )
            continue ;
        if (  ((int)phandle->queryLevel >= STUDY_LEVEL)
              && (strcmp (plist->study, (char *) idxRec->StudyInstanceUID) != 0)
            )
            continue ;
        if (  ((int)phandle->queryLevel >= SERIE_LEVEL)
              && (strcmp (plist->serie, (char *) idxRec->SeriesInstanceUID) != 0)
            )
            continue ;
        if (  ((int)phandle->queryLevel >= IMAGE_LEVEL)
              && (strcmp (plist->image, (char *) idxRec->SOPInstanceUID) != 0)
            )
            continue ;
        return (OFTrue) ;
    }
    return (OFFalse) ;
}

/************
 *      Initializes addresses in an IdxRecord
 */

static void DB_IdxInitRecord (IdxRecord *idx, int linksOnly)
{
    if (! linksOnly)
    {
        idx -> param[RECORDIDX_PatientsBirthDate]. XTag = DCM_PatientsBirthDate  ;
        idx -> param[RECORDIDX_PatientsBirthDate]. ValueLength = DA_MAX_LENGTH  ;
        idx -> PatientsBirthDate[0] = '\0' ;
        idx -> param[RECORDIDX_PatientsSex]. XTag = DCM_PatientsSex  ;
        idx -> param[RECORDIDX_PatientsSex]. ValueLength = CS_MAX_LENGTH  ;
        idx -> PatientsSex[0] = '\0' ;
        idx -> param[RECORDIDX_PatientsName]. XTag = DCM_PatientsName  ;
        idx -> param[RECORDIDX_PatientsName]. ValueLength = PN_MAX_LENGTH  ;
        idx -> PatientsName[0] = '\0' ;
        idx -> param[RECORDIDX_PatientID]. XTag = DCM_PatientID  ;
        idx -> param[RECORDIDX_PatientID]. ValueLength = LO_MAX_LENGTH  ;
        idx -> PatientID[0] = '\0' ;
        idx -> param[RECORDIDX_PatientsBirthTime]. XTag = DCM_PatientsBirthTime  ;
        idx -> param[RECORDIDX_PatientsBirthTime]. ValueLength = TM_MAX_LENGTH  ;
        idx -> PatientsBirthTime[0] = '\0' ;
        idx -> param[RECORDIDX_OtherPatientIDs]. XTag = DCM_OtherPatientIDs  ;
        idx -> param[RECORDIDX_OtherPatientIDs]. ValueLength = LO_MAX_LENGTH  ;
        idx -> OtherPatientIDs[0] = '\0' ;
        idx -> param[RECORDIDX_OtherPatientNames]. XTag = DCM_OtherPatientNames  ;
        idx -> param[RECORDIDX_OtherPatientNames]. ValueLength = PN_MAX_LENGTH  ;
        idx -> OtherPatientNames[0] = '\0' ;
        idx -> param[RECORDIDX_EthnicGroup]. XTag = DCM_EthnicGroup  ;
        idx -> param[RECORDIDX_EthnicGroup]. ValueLength = SH_MAX_LENGTH  ;
        idx -> EthnicGroup[0] = '\0' ;
        idx -> param[RECORDIDX_NumberofPatientRelatedStudies]. XTag = DCM_NumberOfPatientRelatedStudies  ;
        idx -> param[RECORDIDX_NumberofPatientRelatedStudies]. ValueLength = IS_MAX_LENGTH  ;
        idx -> NumberofPatientRelatedStudies[0] = '\0' ;
        idx -> param[RECORDIDX_NumberofPatientRelatedSeries]. XTag = DCM_NumberOfPatientRelatedSeries  ;
        idx -> param[RECORDIDX_NumberofPatientRelatedSeries]. ValueLength = IS_MAX_LENGTH  ;
        idx -> NumberofPatientRelatedSeries[0] = '\0' ;
        idx -> param[RECORDIDX_NumberofPatientRelatedInstances]. XTag = DCM_NumberOfPatientRelatedInstances  ;
        idx -> param[RECORDIDX_NumberofPatientRelatedInstances]. ValueLength = IS_MAX_LENGTH  ;
        idx -> NumberofPatientRelatedInstances[0] = '\0' ;
        idx -> param[RECORDIDX_StudyDate]. XTag = DCM_StudyDate  ;
        idx -> param[RECORDIDX_StudyDate]. ValueLength = DA_MAX_LENGTH  ;
        idx -> StudyDate[0] = '\0' ;
        idx -> param[RECORDIDX_StudyTime]. XTag = DCM_StudyTime  ;
        idx -> param[RECORDIDX_StudyTime]. ValueLength = TM_MAX_LENGTH  ;
        idx -> StudyTime[0] = '\0' ;
        idx -> param[RECORDIDX_StudyID]. XTag = DCM_StudyID  ;
        idx -> param[RECORDIDX_StudyID]. ValueLength = CS_MAX_LENGTH  ;
        idx -> StudyID[0] = '\0' ;
        idx -> param[RECORDIDX_StudyDescription]. XTag = DCM_StudyDescription  ;
        idx -> param[RECORDIDX_StudyDescription]. ValueLength = LO_MAX_LENGTH  ;
        idx -> StudyDescription[0] = '\0' ;
        idx -> param[RECORDIDX_NameOfPhysiciansReadingStudy]. XTag = DCM_NameOfPhysiciansReadingStudy  ;
        idx -> param[RECORDIDX_NameOfPhysiciansReadingStudy]. ValueLength = PN_MAX_LENGTH  ;
        idx -> NameOfPhysiciansReadingStudy[0] = '\0' ;
        idx -> param[RECORDIDX_AccessionNumber]. XTag = DCM_AccessionNumber;
        idx -> param[RECORDIDX_AccessionNumber]. ValueLength = CS_MAX_LENGTH ;
        idx -> AccessionNumber[0] = '\0' ;
        idx -> param[RECORDIDX_ReferringPhysiciansName]. XTag = DCM_ReferringPhysiciansName  ;
        idx -> param[RECORDIDX_ReferringPhysiciansName]. ValueLength = PN_MAX_LENGTH ;
        idx -> ReferringPhysiciansName[0] = '\0' ;
        idx -> param[RECORDIDX_ProcedureDescription]. XTag = DCM_StudyDescription  ;
        idx -> param[RECORDIDX_ProcedureDescription]. ValueLength = LO_MAX_LENGTH ;
        idx -> ProcedureDescription[0] = '\0' ;
        idx -> param[RECORDIDX_AttendingPhysiciansName]. XTag = DCM_NameOfPhysiciansReadingStudy  ;
        idx -> param[RECORDIDX_AttendingPhysiciansName]. ValueLength = PN_MAX_LENGTH ;
        idx -> AttendingPhysiciansName[0] = '\0' ;
        idx -> param[RECORDIDX_StudyInstanceUID]. XTag = DCM_StudyInstanceUID  ;
        idx -> param[RECORDIDX_StudyInstanceUID]. ValueLength = UI_MAX_LENGTH ;
        idx -> StudyInstanceUID[0] = '\0' ;
        idx -> param[RECORDIDX_OtherStudyNumbers]. XTag = DCM_OtherStudyNumbers  ;
        idx -> param[RECORDIDX_OtherStudyNumbers]. ValueLength = IS_MAX_LENGTH ;
        idx -> OtherStudyNumbers[0] = '\0' ;
        idx -> param[RECORDIDX_AdmittingDiagnosesDescription]. XTag = DCM_AdmittingDiagnosesDescription  ;
        idx -> param[RECORDIDX_AdmittingDiagnosesDescription]. ValueLength = LO_MAX_LENGTH ;
        idx -> AdmittingDiagnosesDescription[0] = '\0' ;
        idx -> param[RECORDIDX_PatientsAge]. XTag = DCM_PatientsAge  ;
        idx -> param[RECORDIDX_PatientsAge]. ValueLength = AS_MAX_LENGTH ;
        idx -> PatientsAge[0] = '\0' ;
        idx -> param[RECORDIDX_PatientsSize]. XTag = DCM_PatientsSize  ;
        idx -> param[RECORDIDX_PatientsSize]. ValueLength = DS_MAX_LENGTH ;
        idx -> PatientsSize[0] = '\0' ;
        idx -> param[RECORDIDX_PatientsWeight]. XTag = DCM_PatientsWeight  ;
        idx -> param[RECORDIDX_PatientsWeight]. ValueLength = DS_MAX_LENGTH ;
        idx -> PatientsWeight[0] = '\0' ;
        idx -> param[RECORDIDX_Occupation]. XTag = DCM_Occupation  ;
        idx -> param[RECORDIDX_Occupation]. ValueLength = SH_MAX_LENGTH ;
        idx -> Occupation[0] = '\0' ;
        idx -> param[RECORDIDX_NumberofStudyRelatedSeries]. XTag = DCM_NumberOfStudyRelatedSeries  ;
        idx -> param[RECORDIDX_NumberofStudyRelatedSeries]. ValueLength = IS_MAX_LENGTH ;
        idx -> NumberofStudyRelatedSeries[0] = '\0' ;
        idx -> param[RECORDIDX_NumberofStudyRelatedInstances]. XTag = DCM_NumberOfStudyRelatedInstances  ;
        idx -> param[RECORDIDX_NumberofStudyRelatedInstances]. ValueLength = IS_MAX_LENGTH ;
        idx -> NumberofStudyRelatedInstances[0] = '\0' ;
        idx -> param[RECORDIDX_SeriesNumber]. XTag = DCM_SeriesNumber  ;
        idx -> param[RECORDIDX_SeriesNumber]. ValueLength = IS_MAX_LENGTH ;
        idx -> SeriesNumber[0] = '\0' ;
        idx -> param[RECORDIDX_SeriesInstanceUID]. XTag = DCM_SeriesInstanceUID  ;
        idx -> param[RECORDIDX_SeriesInstanceUID]. ValueLength = UI_MAX_LENGTH ;
        idx -> SeriesInstanceUID[0] = '\0' ;
        idx -> param[RECORDIDX_Modality]. XTag = DCM_Modality  ;
        idx -> param[RECORDIDX_Modality]. ValueLength = CS_MAX_LENGTH ;
        idx -> ImageNumber[0] = '\0' ;
        idx -> param[RECORDIDX_ImageNumber]. XTag = DCM_InstanceNumber  ;
        idx -> param[RECORDIDX_ImageNumber]. ValueLength = IS_MAX_LENGTH ;
        idx -> ImageNumber[0] = '\0' ;
        idx -> param[RECORDIDX_SOPInstanceUID]. XTag = DCM_SOPInstanceUID  ;
        idx -> param[RECORDIDX_SOPInstanceUID]. ValueLength = UI_MAX_LENGTH ;
        idx -> SOPInstanceUID[0] = '\0' ;
        idx -> param[RECORDIDX_SeriesDate]. XTag = DCM_SeriesDate;
        idx -> param[RECORDIDX_SeriesDate]. ValueLength = DA_MAX_LENGTH ;
        idx -> SeriesDate[0] = '\0'     ;
        idx -> param[RECORDIDX_SeriesTime]. XTag = DCM_SeriesTime;
        idx -> param[RECORDIDX_SeriesTime]. ValueLength = TM_MAX_LENGTH ;
        idx -> SeriesTime[0] = '\0'     ;
        idx -> param[RECORDIDX_SeriesDescription]. XTag = DCM_SeriesDescription  ;
        idx -> param[RECORDIDX_SeriesDescription]. ValueLength = LO_MAX_LENGTH ;
        idx -> SeriesDescription[0] = '\0'      ;
        idx -> param[RECORDIDX_ProtocolName]. XTag = DCM_ProtocolName  ;
        idx -> param[RECORDIDX_ProtocolName]. ValueLength = LO_MAX_LENGTH ;
        idx -> ProtocolName[0] = '\0'   ;
        idx -> param[RECORDIDX_OperatorsName ]. XTag = DCM_OperatorsName  ;
        idx -> param[RECORDIDX_OperatorsName ]. ValueLength = PN_MAX_LENGTH ;
        idx -> OperatorsName[0] = '\0';
        idx -> param[RECORDIDX_PerformingPhysiciansName]. XTag = DCM_PerformingPhysiciansName  ;
        idx -> param[RECORDIDX_PerformingPhysiciansName]. ValueLength = PN_MAX_LENGTH ;
        idx -> PerformingPhysiciansName[0] = '\0';
        idx -> param[RECORDIDX_PresentationLabel]. XTag = DCM_ContentLabel  ;
        idx -> param[RECORDIDX_PresentationLabel]. ValueLength = CS_LABEL_MAX_LENGTH ;
        idx -> PresentationLabel[0] = '\0';
    }
    idx -> param[RECORDIDX_PatientsBirthDate]. PValueField = (char *)idx -> PatientsBirthDate ;
    idx -> param[RECORDIDX_PatientsSex]. PValueField = (char *)idx -> PatientsSex ;
    idx -> param[RECORDIDX_PatientsName]. PValueField = (char *)idx -> PatientsName ;
    idx -> param[RECORDIDX_PatientID]. PValueField = (char *)idx -> PatientID ;
    idx -> param[RECORDIDX_PatientsBirthTime]. PValueField = (char *)idx -> PatientsBirthTime ;
    idx -> param[RECORDIDX_OtherPatientIDs]. PValueField = (char *)idx -> OtherPatientIDs ;
    idx -> param[RECORDIDX_OtherPatientNames]. PValueField = (char *)idx -> OtherPatientNames ;
    idx -> param[RECORDIDX_EthnicGroup]. PValueField = (char *)idx -> EthnicGroup ;
    idx -> param[RECORDIDX_NumberofPatientRelatedStudies]. PValueField = (char *)idx -> NumberofPatientRelatedStudies ;
    idx -> param[RECORDIDX_NumberofPatientRelatedSeries]. PValueField = (char *) idx -> NumberofPatientRelatedSeries ;
    idx -> param[RECORDIDX_NumberofPatientRelatedInstances]. PValueField = (char *) idx -> NumberofPatientRelatedInstances ;
    idx -> param[RECORDIDX_StudyDate]. PValueField = (char *) idx -> StudyDate ;
    idx -> param[RECORDIDX_StudyTime]. PValueField = (char *) idx -> StudyTime ;
    idx -> param[RECORDIDX_StudyID]. PValueField = (char *) idx -> StudyID ;
    idx -> param[RECORDIDX_StudyDescription]. PValueField = (char *) idx -> StudyDescription ;
    idx -> param[RECORDIDX_NameOfPhysiciansReadingStudy]. PValueField = (char *) idx ->NameOfPhysiciansReadingStudy;
    idx -> param[RECORDIDX_AccessionNumber]. PValueField = (char *) idx -> AccessionNumber ;
    idx -> param[RECORDIDX_ReferringPhysiciansName]. PValueField = (char *) idx -> ReferringPhysiciansName ;
    idx -> param[RECORDIDX_ProcedureDescription]. PValueField = (char *) idx -> ProcedureDescription ;
    idx -> param[RECORDIDX_AttendingPhysiciansName]. PValueField = (char *) idx -> AttendingPhysiciansName ;
    idx -> param[RECORDIDX_StudyInstanceUID]. PValueField = (char *) idx -> StudyInstanceUID ;
    idx -> param[RECORDIDX_OtherStudyNumbers]. PValueField = (char *) idx -> OtherStudyNumbers ;
    idx -> param[RECORDIDX_AdmittingDiagnosesDescription]. PValueField = (char *) idx -> AdmittingDiagnosesDescription ;
    idx -> param[RECORDIDX_PatientsAge]. PValueField = (char *) idx -> PatientsAge ;
    idx -> param[RECORDIDX_PatientsSize]. PValueField = (char *) idx -> PatientsSize ;
    idx -> param[RECORDIDX_PatientsWeight]. PValueField = (char *) idx -> PatientsWeight ;
    idx -> param[RECORDIDX_Occupation]. PValueField = (char *) idx -> Occupation ;
    idx -> param[RECORDIDX_NumberofStudyRelatedSeries]. PValueField = (char *) idx -> NumberofStudyRelatedSeries ;
    idx -> param[RECORDIDX_NumberofStudyRelatedInstances]. PValueField = (char *) idx -> NumberofStudyRelatedInstances ;
    idx -> param[RECORDIDX_SeriesNumber]. PValueField = (char *) idx -> SeriesNumber ;
    idx -> param[RECORDIDX_SeriesInstanceUID]. PValueField = (char *) idx -> SeriesInstanceUID ;
    idx -> param[RECORDIDX_Modality]. PValueField = (char *) idx -> Modality ;
    idx -> param[RECORDIDX_ImageNumber]. PValueField = (char *) idx -> ImageNumber ;
    idx -> param[RECORDIDX_SOPInstanceUID]. PValueField = (char *) idx -> SOPInstanceUID ;
    idx -> param[RECORDIDX_SeriesDate]. PValueField = (char *) idx -> SeriesDate ;
    idx -> param[RECORDIDX_SeriesTime]. PValueField = (char *) idx -> SeriesTime ;
    idx -> param[RECORDIDX_SeriesDescription]. PValueField = (char *) idx -> SeriesDescription ;
    idx -> param[RECORDIDX_ProtocolName]. PValueField = (char *) idx -> ProtocolName ;
    idx -> param[RECORDIDX_OperatorsName ]. PValueField = (char *) idx -> OperatorsName ;
    idx -> param[RECORDIDX_PerformingPhysiciansName]. PValueField = (char *) idx -> PerformingPhysiciansName ;
    idx -> param[RECORDIDX_PresentationLabel]. PValueField = (char *) idx -> PresentationLabel ;
}

/******************************
 *      Seek to a file position and do error checking
 *
 * Motivation:
 * We have had situations during demonstrations where size of the DB index file
 * has exploded.  It seems that a record is being written to a position
 * way past the end of file.
 * This seek function does some sanity error checking to try to identify
 * the problem.
 */
static long DB_lseek(int fildes, long offset, int whence)
{
    long pos;
    long curpos;
    long endpos;


    /*
    ** we should not be seeking to an offset < 0
    */
    if (offset < 0) {
        CERR << "*** DB ALERT: attempt to seek before begining of file" << endl;
    }

    /* get the current position */
    curpos = lseek(fildes, 0, SEEK_CUR);
    if (curpos < 0) {
        CERR << "DB_lseek: cannot get current position: " << strerror(errno) << endl;
        return curpos;
    }
    /* get the end of file position */
    endpos = lseek(fildes, 0, SEEK_END);
    if (endpos < 0) {
        CERR << "DB_lseek: cannot get end of file position: " << strerror(errno) << endl;
        return endpos;
    }

    /* return to current position */
    curpos = lseek(fildes, curpos, SEEK_SET);
    if (curpos < 0) {
        CERR << "DB_lseek: cannot reset current position: " << strerror(errno) << endl;
        return curpos;
    }

    /* do the requested seek */
    pos = lseek(fildes, offset, whence);
    if (pos < 0) {
        char msg[1024];
        sprintf(msg, "DB_lseek: cannot seek to %ld", offset);
        CERR << msg << ": " << strerror(errno) << endl;
        return pos;
    }

    /*
    ** print an alert if we are seeking to far
    ** what is the limit? We don't expect the index file to be
    ** larger than 32Mb
    */
    const long maxFileSize = 33554432;
    if (pos > maxFileSize) {
        CERR << "*** DB ALERT: attempt to seek beyond " << maxFileSize << " bytes" << endl;
    }

    /* print an alert if we are seeking beyond the end of file.
     * ignore when file is empty
     */
    if ((endpos > 0) && (pos > endpos)) {
        CERR << "*** DB ALERT: attempt to seek beyond end of file" << endl
            << "              offset=" << offset << " filesize=" << endpos << endl;
    }

    return pos;
}

/******************************
 *      Read an Index record
 */

OFCondition DcmQueryRetrieveIndexDatabaseHandle::DB_IdxRead (int idx, IdxRecord *idxRec)
{

    /*** Goto the right index in file
    **/

    DB_lseek (handle -> pidx, (long) (SIZEOF_STUDYDESC + idx * SIZEOF_IDXRECORD), SEEK_SET) ;

    /*** Read the record
    **/

    if (read (handle -> pidx, (char *) idxRec, SIZEOF_IDXRECORD) != SIZEOF_IDXRECORD)
        return (DcmQRIndexDatabaseError) ;

    DB_lseek (handle -> pidx, 0L, SEEK_SET) ;

    /*** Initialize record links
    **/

    DB_IdxInitRecord (idxRec, 1) ;
    return EC_Normal ;
}


/******************************
 *      Add an Index record
 *      Returns the index allocated for this record
 */

static OFCondition DB_IdxAdd (DB_Private_Handle *phandle, int *idx, IdxRecord *idxRec)
{
    IdxRecord   rec ;
    OFCondition cond = EC_Normal;

    /*** Find free place for the record
    *** A place is free if filename is empty
    **/

    *idx = 0 ;

    DB_lseek (phandle -> pidx, (long) SIZEOF_STUDYDESC, SEEK_SET) ;
    while (read (phandle -> pidx, (char *) &rec, SIZEOF_IDXRECORD) == SIZEOF_IDXRECORD) {
        if (rec. filename [0] == '\0')
            break ;
        (*idx)++ ;
    }

    /*** We have either found a free place or we are at the end of file. **/


    DB_lseek (phandle -> pidx, (long) (SIZEOF_STUDYDESC + (*idx) * SIZEOF_IDXRECORD), SEEK_SET) ;

    if (write (phandle -> pidx, (char *) idxRec, SIZEOF_IDXRECORD) != SIZEOF_IDXRECORD)
        cond = DcmQRIndexDatabaseError ;
    else
        cond = EC_Normal ;

    DB_lseek (phandle -> pidx, 0L, SEEK_SET) ;

    return cond ;
}


/******************************
 *      Change the StudyDescRecord
 */

OFCondition DcmQueryRetrieveIndexDatabaseHandle::DB_StudyDescChange(StudyDescRecord *pStudyDesc)
{
    OFCondition cond = EC_Normal;
    DB_lseek (handle -> pidx, 0L, SEEK_SET) ;
    if (write (handle -> pidx, (char *) pStudyDesc, SIZEOF_STUDYDESC) != SIZEOF_STUDYDESC) cond = DcmQRIndexDatabaseError;
    DB_lseek (handle -> pidx, 0L, SEEK_SET) ;
    return cond ;
}

/******************************
 *      Init an Index record loop
 */

OFCondition DcmQueryRetrieveIndexDatabaseHandle::DB_IdxInitLoop(int *idx)
{
    DB_lseek (handle -> pidx, SIZEOF_STUDYDESC, SEEK_SET) ;
    *idx = -1 ;
    return EC_Normal ;
}

/******************************
 *      Get next Index record
 *      On return, idx is initialized with the index of the record read
 */

OFCondition DcmQueryRetrieveIndexDatabaseHandle::DB_IdxGetNext(int *idx, IdxRecord *idxRec)
{

    (*idx)++ ;
    DB_lseek (handle -> pidx, SIZEOF_STUDYDESC + (long)(*idx) * SIZEOF_IDXRECORD, SEEK_SET) ;
    while (read (handle -> pidx, (char *) idxRec, SIZEOF_IDXRECORD) == SIZEOF_IDXRECORD) {
        if (idxRec -> filename [0] != '\0') {
            DB_IdxInitRecord (idxRec, 1) ;

            return EC_Normal ;
        }
        (*idx)++ ;
    }

    DB_lseek (handle -> pidx, 0L, SEEK_SET) ;

    return DcmQRIndexDatabaseError ;
}


/******************************
 *      Get next Index record
 *      On return, idx is initialized with the index of the record read
 */

OFCondition DcmQueryRetrieveIndexDatabaseHandle::DB_GetStudyDesc (StudyDescRecord *pStudyDesc)
{

    DB_lseek (handle -> pidx, 0L, SEEK_SET) ;
    if ( read (handle -> pidx, (char *) pStudyDesc, SIZEOF_STUDYDESC) == SIZEOF_STUDYDESC )
        return EC_Normal ;

    DB_lseek (handle -> pidx, 0L, SEEK_SET) ;

    return DcmQRIndexDatabaseError ;
}


/******************************
 *      Remove an Index record
 *      Just put a record with filename == ""
 */

OFCondition DcmQueryRetrieveIndexDatabaseHandle::DB_IdxRemove(int idx)
{
    IdxRecord   rec ;
    OFCondition cond = EC_Normal;

    DB_lseek (handle -> pidx, SIZEOF_STUDYDESC + (long)idx * SIZEOF_IDXRECORD, SEEK_SET) ;
    DB_IdxInitRecord (&rec, 0) ;

    rec. filename [0] = '\0' ;
    if (write (handle -> pidx, (char *) &rec, SIZEOF_IDXRECORD) == SIZEOF_IDXRECORD)
        cond = EC_Normal ;

    else
        cond = DcmQRIndexDatabaseError ;

    DB_lseek (handle -> pidx, 0L, SEEK_SET) ;

    return cond ;
}

OFCondition DcmQueryRetrieveIndexDatabaseHandle::DB_lock(OFBool exclusive)
{
    int lockmode;

    if (exclusive) {
        lockmode = LOCK_EX;     /* exclusive lock */
    } else {
        lockmode = LOCK_SH;     /* shared lock */
    }
    if (dcmtk_flock(handle->pidx, lockmode) < 0) {
        dcmtk_plockerr("DB_lock");
        return DcmQRIndexDatabaseError;
    }
    return EC_Normal;
}

OFCondition DcmQueryRetrieveIndexDatabaseHandle::DB_unlock()
{
    if (dcmtk_flock(handle->pidx, LOCK_UN) < 0) {
        dcmtk_plockerr("DB_unlock");
        return DcmQRIndexDatabaseError;
    }
    return EC_Normal;
}

/*******************
 *    Free an element List
 */

static OFCondition DB_FreeUidList (DB_UidList *lst)
{
    if (lst == NULL) return EC_Normal;

    OFCondition cond = DB_FreeUidList (lst -> next);
    if (lst -> patient)
    free (lst -> patient);
    if (lst -> study)
    free (lst -> study);
    if (lst -> serie)
    free (lst -> serie);
    if (lst -> image)
    free (lst -> image);
    free (lst);
    return (cond);
}


/*******************
 *    Free a UID List
 */

static OFCondition DB_FreeElementList (DB_ElementList *lst)
{
    if (lst == NULL) return EC_Normal;

    OFCondition cond = DB_FreeElementList (lst -> next);
    if (lst->elem.PValueField != NULL) {
    free ((char *) lst -> elem. PValueField);
    }
    free (lst);
    return (cond);
}


/*******************
 *    Matches two strings
 */

static int DB_StringUnify  (char *pmod, char *pstr)
{
    int uni;


    if (*pmod == '\0')
    return (*pstr == '\0');

    if (  *pmod == *pstr
      || (*pmod == '?' && *pstr != '\0')
    )
    return (DB_StringUnify (pmod + 1, pstr + 1));

    if (*pmod == '*') {
    if ( *(pmod + 1) == '\0' )
        return (OFTrue);
    while (  ( (uni = DB_StringUnify (pmod + 1, pstr)) == OFFalse )
         && (*pstr != '\0')
        )
        pstr++;
    return (uni);
    }
    else if (*pmod != *pstr)
    return (OFFalse);
    return OFFalse;
}

/*******************
 *    Is the specified tag supported
 */

static int DB_TagSupported (DcmTagKey tag)
{
    int i;

    for (i = 0; i < NbFindAttr; i++)
    if (TbFindAttr[i]. tag == tag)
        return (OFTrue);

    return (OFFalse);

}


/*******************
 *    Get UID tag of a specified level
 */

static OFCondition DB_GetUIDTag (DB_LEVEL level, DcmTagKey *tag)
{
    int i;

    for (i = 0; i < NbFindAttr; i++)
    if ((TbFindAttr[i]. level == level) && (TbFindAttr[i]. keyAttr == UNIQUE_KEY))
        break;

    if (i < NbFindAttr) {
    *tag = TbFindAttr[i].tag;
    return (EC_Normal);
    }
    else
    return (DcmQRIndexDatabaseError);

}

/*******************
 *    Get tag level of a specified tag
 */

static OFCondition DB_GetTagLevel (DcmTagKey tag, DB_LEVEL *level)
{
    int i;

    for (i = 0; i < NbFindAttr; i++)
    if (TbFindAttr[i]. tag == tag)
        break;

    if (i < NbFindAttr) {
    *level = TbFindAttr[i]. level;
    return (EC_Normal);
    }
    else
    return (DcmQRIndexDatabaseError);
}

/*******************
 *    Get tag key attribute of a specified tag
 */

static OFCondition DB_GetTagKeyAttr (DcmTagKey tag, DB_KEY_TYPE *keyAttr)
{
    int i;

    for (i = 0; i < NbFindAttr; i++)
    if (TbFindAttr[i]. tag == tag)
        break;

    if (i < NbFindAttr) {
    *keyAttr = TbFindAttr[i]. keyAttr;
    return (EC_Normal);
    }
    else
    return (DcmQRIndexDatabaseError);
}

/*******************
 *    Get tag key attribute of a specified tag
 */

static OFCondition DB_GetTagKeyClass (DcmTagKey tag, DB_KEY_CLASS *keyAttr)
{
    int i;

    for (i = 0; i < NbFindAttr; i++)
    if (TbFindAttr[i]. tag == tag)
        break;

    if (i < NbFindAttr) {
    *keyAttr = TbFindAttr[i]. keyClass;
    return (EC_Normal);
    }
    else
    return (DcmQRIndexDatabaseError);
}


/*******************
 *    Remove spaces in a string
 */

static void DB_RemoveSpaces (char *string)
{
    char *pc1, *pc2;

    for (pc1 = pc2 = string; *pc2; pc2++) {
    if (*pc2 != ' ') {
        *pc1 = *pc2;
        pc1++;
    }
    }
    *pc1 = '\0';
}

/*******************
 *    Remove leading and trailing spaces in a string
 */

static void DB_RemoveEnclosingSpaces (char *string)
{
    char *pc1, *pc2;

    /** Find in pc2 the first non space character
    ** If not found, string is empty
    */

    for (pc2 = string; (*pc2 != '\0') && (*pc2 == ' '); pc2++);
    if (*pc2 == '\0') {
    string [0] = '\0';
    return;
    }

    /** Shift the string if necessary
     */

    if (pc2 != string) {
    for (pc1 = string; *pc2; pc1++, pc2++)
        *pc1 = *pc2;
    *pc1 = '\0';
    }

    /** Ship trailing spaces
     */

    for (pc2 = string + strlen (string) - 1; *pc2 == ' '; pc2--);
    pc2++;
    *pc2 = '\0';
}


/*******************
 *    Convert a date YYYYMMDD in a long
 */

static long DB_DateToLong (char *date)
{
    char year [5];
    char month[3];
    char day  [3];

    strncpy (year, date, 4);
    year [4] = '\0';
    strncpy (month, date + 4, 2);
    month [2] = '\0';
    strncpy (day, date + 6, 2);
    day [2] = '\0';

    return ((atol(year) * 10000) + (atol(month) * 100) + atol(day));
}


/*******************
 *    Convert a time in a double
 */

static double DB_TimeToDouble (char *thetime)
{
    char t [20];
    char tmp [4];

    double result = 0.;
    char *pc;

    /*** Get fractionnal part if exists
    **/

    strcpy (t, thetime);
    if ((pc = strchr (t, '.')) != NULL) {
    double f;

    *pc = '\0';
    for (pc++, f = 1.; (*pc) && (isdigit (*pc)); pc++) {
        f /= 10.;
        result += (*pc - '0') * f;
    }
    }

    /*** Add default values (mm ss) if necessary
    **/

    strcat (t, "0000");
    t [6] = '\0';

    /*** Get Hours, Minutes and Seconds
    **/

    strncpy (tmp, t, 2);
    tmp [3] = '\0';
    result += 3600. * OFStandard::atof(tmp);

    strncpy (tmp, t + 2, 2);
    tmp [3] = '\0';
    result += 60. * OFStandard::atof(tmp);

    strncpy (tmp, t + 4, 2);
    tmp [3] = '\0';
    result += OFStandard::atof(tmp);

    return result;
}



/***********************
 *    Duplicate a dicom element
 *    dst space is supposed provided by the caller
 */

static void DB_DuplicateElement (DB_SmallDcmElmt *src, DB_SmallDcmElmt *dst)
{
    bzero( (char*)dst, sizeof (DB_SmallDcmElmt));
    dst -> XTag = src -> XTag;
    dst -> ValueLength = src -> ValueLength;

    if (src -> ValueLength == 0)
    dst -> PValueField = NULL;
    else {
    dst -> PValueField = (char *)malloc ((int) src -> ValueLength+1);
    bzero(dst->PValueField, (size_t)(src->ValueLength+1));
    if (dst->PValueField != NULL) {
        memcpy (dst -> PValueField,  src -> PValueField,
            (size_t) src -> ValueLength);
    } else {
        CERR << "DB_DuplicateElement: out of memory" << endl;
    }
    }
}


/***********************
 *    Compare two ImagesofStudyArray elements
 */

static int DB_Compare(const void *ve1, const void *ve2)
{
    ImagesofStudyArray *e1 = (ImagesofStudyArray *)ve1;
    ImagesofStudyArray *e2 = (ImagesofStudyArray *)ve2;
    if ( e1 -> RecordedDate > e2 -> RecordedDate )
    return (1);
    else
    if ( e1 -> RecordedDate == e2 -> RecordedDate )
        return (0);
    else
        return (-1);

}


/* ==================================================================== */

DcmQueryRetrieveDatabaseHandle::~DcmQueryRetrieveDatabaseHandle()
{
}

/* ========================= FIND ========================= */

/************
**      Try to match Two Dates
**      The first one is the "model", the second one an element
**      Returns OFTrue if matching is ok, else returns OFFalse
 */

int DcmQueryRetrieveIndexDatabaseHandle::matchDate (DB_SmallDcmElmt *mod, DB_SmallDcmElmt *elt)
{
    char date [DBC_MAXSTRING] ;
    char modl [DBC_MAXSTRING] ;

    /*** Get elt and model data in strings
    **/

    memcpy (date, elt->PValueField, (size_t)(elt->ValueLength)) ;
    date [elt->ValueLength] = '\0' ;
    DB_RemoveSpaces (date) ;

    memcpy (modl, mod->PValueField, (size_t)(mod->ValueLength)) ;
    modl [mod->ValueLength] = '\0' ;
    DB_RemoveSpaces (modl) ;

    /*** If no '-' in date
    *** return strict comparison result
    **/

    if (strchr (modl, '-') == NULL)
        return (strcmp (modl, date) == 0) ;

    /*** If first char is -
    **/

    if (modl [0] == '-') {
        return DB_DateToLong (date) <= DB_DateToLong (modl+1) ;
    }

    /*** If last char is -
    **/

    else if (modl [strlen (modl) - 1] == '-') {
        modl [strlen (modl) - 1] = '\0' ;
        return DB_DateToLong (date) >= DB_DateToLong (modl) ;
    }
    else {
        char *pc ;
        long d ;

        d = DB_DateToLong (date) ;
        pc = strchr (modl, '-') ;
        *pc = '\0' ;

        return (d >= DB_DateToLong (modl)) && (d <= DB_DateToLong (pc+1)) ;

    }
}

/************
**      Try to match Two Times
**      The first one is the "model", the second one an element
**      Returns OFTrue if matching is ok, else returns OFFalse
 */

int DcmQueryRetrieveIndexDatabaseHandle::matchTime (DB_SmallDcmElmt *mod, DB_SmallDcmElmt *elt)
{
    char aTime [DBC_MAXSTRING] ;
    char modl [DBC_MAXSTRING] ;

    /*** Get elt and model data in strings
    **/

    memcpy (aTime, elt->PValueField, (size_t)(elt->ValueLength)) ;
    aTime [elt->ValueLength] = '\0' ;
    DB_RemoveSpaces (aTime) ;

    memcpy (modl, mod->PValueField, (size_t)(mod->ValueLength)) ;
    modl [mod->ValueLength] = '\0' ;
    DB_RemoveSpaces (modl) ;

    /*** If no '-' in time
    *** return strict comparison result
    **/

    if (strchr (modl, '-') == NULL)
        return (strcmp (modl, aTime) == 0) ;

    /*** If first char is -
    **/

    if (modl [0] == '-') {
        return DB_TimeToDouble (aTime) <= DB_TimeToDouble (modl+1) ;
    }

    /*** If last char is -
    **/

    else if (modl [strlen (modl) - 1] == '-') {
        modl [strlen (modl) - 1] = '\0' ;
        return DB_TimeToDouble (aTime) >= DB_TimeToDouble (modl) ;
    }
    else {
        char *pc ;
        double t ;

        t = DB_TimeToDouble (aTime) ;
        pc = strchr (modl, '-') ;
        *pc = '\0' ;

        return (t >= DB_TimeToDouble (modl)) && (t <= DB_TimeToDouble (pc+1)) ;

    }
}

/************
**      Try to match Two UID
**      The first one is the "model", the second one an element
**      Returns OFTrue if matching is ok, else returns OFFalse
 */

int DcmQueryRetrieveIndexDatabaseHandle::matchUID (DB_SmallDcmElmt *mod, DB_SmallDcmElmt *elt)
{
    int match ;
    char *uid  ;
    char *modl  ;
    char *pc ;
    unsigned int length ;

    /*** Get elt and model data in strings
    **/

    uid = (char *) malloc ((size_t)(elt->ValueLength + 1)) ;
    if (uid == NULL) {
        return 0 ;
    }
    memcpy (uid, elt->PValueField, (size_t)(elt->ValueLength)) ;
    uid [elt->ValueLength] = '\0' ;

    modl = (char *) malloc ((size_t)(mod->ValueLength + 1)) ;
    if (modl == NULL) {
        free (uid) ;
        return 0 ;
    }
    memcpy (modl, mod->PValueField, (size_t)(mod->ValueLength)) ;
    modl [mod->ValueLength] = '\0' ;


    /*** If no '\' in model
    *** return strict comparison result
    **/

#ifdef STRICT_COMPARE
#else
    /*** Suppress Leading and Trailing spaces in
    *** model and string
    **/

    DB_RemoveEnclosingSpaces (uid) ;
    DB_RemoveEnclosingSpaces (modl) ;
#endif

    if (strchr (modl, '\\') == NULL) {
        match = (strcmp (modl, uid) == 0) ;
        free (uid) ;
        free (modl) ;
        return (match) ;
    }

    /*** UID List comparison.
    *** Match is successful if uid is found in model
    **/

    match = OFFalse ;
    for (pc = modl ; *pc ; ) {

        /*** Calculate the length to the next '\' sign (if any).
        *** Otherwise the length of pc is returned.
        **/
        length = strcspn(pc, "\\") ;

        if ((length == strlen(uid)) && (strncmp (pc, uid, length) == 0)) {
            match = OFTrue ;
            break ;
        }
        else {
            pc = strchr (pc, '\\') ;
            if (pc == NULL)
                break ;
            else
                pc++ ;
        }
    }

    free (uid) ;
    free (modl) ;
    return (match) ;

}

/************
**      Try to match Two Strings
**      The first one is the "model", the second one an element
**      Returns OFTrue if matching is ok, else returns OFFalse
 */

int DcmQueryRetrieveIndexDatabaseHandle::matchStrings (DB_SmallDcmElmt *mod, DB_SmallDcmElmt *elt)
{
    int match ;
    char *string  ;
    char *modl  ;

    /*** Get elt and model data in strings
    **/

    string = (char *) malloc ((size_t)(elt->ValueLength + 1)) ;
    if (string == NULL) {
        return 0 ;
    }
    memcpy (string, elt->PValueField, (size_t)(elt->ValueLength)) ;
    string [elt->ValueLength] = '\0' ;

    modl = (char *) malloc ((size_t)(mod->ValueLength + 1)) ;
    if (modl == NULL) {
        free (string) ;
        return 0 ;
    }
    memcpy (modl, mod->PValueField, (size_t)(mod->ValueLength)) ;
    modl [mod->ValueLength] = '\0' ;

#ifdef STRICT_COMPARE
#else
    /*** Suppress Leading and Trailing spaces in
    *** model and string
    **/

    DB_RemoveEnclosingSpaces (string) ;
    DB_RemoveEnclosingSpaces (modl) ;
#endif

    /*** If no '*' and no '?' in model
    *** return strict comparison result
    **/

    if ((strchr (modl, '*') == NULL) && (strchr (modl, '?') == NULL))
        return (strcmp (modl, string) == 0) ;

    match = DB_StringUnify (modl, string) ;

    free (string) ;
    free (modl) ;
    return (match) ;

}

/************
**      Try to match Two Unknown elements
**      Strict comparaison is applied
**      The first one is the "model", the second one an element
**      Returns OFTrue if matching is ok, else returns OFFalse
 */

int DcmQueryRetrieveIndexDatabaseHandle::matchOther (DB_SmallDcmElmt *mod, DB_SmallDcmElmt *elt)
{
    if (mod->ValueLength != elt->ValueLength)
        return OFFalse ;

    return (memcmp (mod->PValueField, elt->PValueField, (size_t)(elt->ValueLength)) == 0) ;
}

/************
**      Try to match Two DB_SmallDcmElmts
**      The first one is the "model", the second one an element
**      Returns OFTrue if matching is ok, else returns OFFalse
 */

int DcmQueryRetrieveIndexDatabaseHandle::dbmatch (DB_SmallDcmElmt *mod, DB_SmallDcmElmt *elt)
{
    DB_KEY_CLASS keyClass = OTHER_CLASS;

    /*** If model length is 0
    *** Universal matching is applied : return always OFTrue
    **/

    if (mod->ValueLength == 0)
        return (OFTrue) ;

    /*** Get the key class of the element
    **/

    DB_GetTagKeyClass (elt->XTag, &keyClass) ;

    switch (keyClass) {

    case DATE_CLASS :
        return matchDate (mod, elt)  ;

    case TIME_CLASS :
        return matchTime (mod, elt)  ;

    case UID_CLASS :
        return matchUID  (mod, elt) ;

    case STRING_CLASS :
        return matchStrings (mod, elt) ;

    case OTHER_CLASS :
        return matchOther (mod, elt) ;

    }
    return OFFalse;
}

/************
**      Create the response list in specified handle,
**      using informations found in an index record.
**      Old response list is supposed freed
**/

void DcmQueryRetrieveIndexDatabaseHandle::makeResponseList (
                DB_Private_Handle       *phandle,
                IdxRecord               *idxRec
                )
{
    int i ;
    DB_ElementList *pRequestList = NULL;
    DB_ElementList *plist = NULL;
    DB_ElementList *last = NULL;

    phandle->findResponseList = NULL ;

    /*** For each element in Request identifier
    **/

    for (pRequestList = phandle->findRequestList ; pRequestList ; pRequestList = pRequestList->next) {

        /*** Find Corresponding Tag in index record
        **/

        for (i = 0 ; i < NBPARAMETERS ; i++)
            if (idxRec->param [i]. XTag == pRequestList->elem. XTag)
                break ;

        /*** If Tag not found, skip the element
        **/

        if (i >= NBPARAMETERS)
            continue ;

        /*** Append index record element to response list
        **/

        plist = (DB_ElementList *) malloc (sizeof (DB_ElementList)) ;
        if (plist == NULL) {
            CERR << "makeResponseList: out of memory" << endl;
            return;
        }
        plist->next = NULL ;

        DB_DuplicateElement(&idxRec->param[i], &plist->elem);

        if (phandle->findResponseList == NULL) {
            phandle->findResponseList = last = plist ;
        }
        else {
            last->next = plist ;
            last = plist ;
        }

    }
}



/************
**      Test a Find Request List
**      Returns EC_Normal if ok, else returns DcmQRIndexDatabaseError
 */

OFCondition DcmQueryRetrieveIndexDatabaseHandle::testFindRequestList (
                DB_ElementList  *findRequestList,
                DB_LEVEL        queryLevel,
                DB_LEVEL        infLevel,
                DB_LEVEL        lowestLevel
                )
{
    DB_ElementList *plist ;
    DB_LEVEL    XTagLevel ;
    DB_KEY_TYPE         XTagType ;
    int level ;

    /**** Query level must be at least the infLevel
    ***/

    if (queryLevel < infLevel) {
        dbdebug(1,"Level incompatible with Information Model (level %d)\n", queryLevel) ;
        return DcmQRIndexDatabaseError ;
    }

    if (queryLevel > lowestLevel) {
        dbdebug(1,"Level incompatible with Information Model (level %d)\n", queryLevel) ;
        return DcmQRIndexDatabaseError ;
    }

    for (level = PATIENT_LEVEL ; level <= IMAGE_LEVEL ; level++) {

        /**** Manage exception due to StudyRoot Information Model :
        **** In this information model, queries may include Patient attributes
        **** but only if they are made at the study level
        ***/

        if ((level == PATIENT_LEVEL) && (infLevel == STUDY_LEVEL)) {
            /** In Study Root Information Model, accept only Patient Tags
            ** if the Query Level is the Study level
            */

            int atLeastOneKeyFound = OFFalse ;
            for (plist = findRequestList ; plist ; plist = plist->next) {
                DB_GetTagLevel (plist->elem. XTag, &XTagLevel) ;
                if (XTagLevel != level)
                    continue ;
                atLeastOneKeyFound = OFTrue ;
            }
            if (atLeastOneKeyFound && (queryLevel != STUDY_LEVEL)) {
                dbdebug(1,"Key found in Study Root Information Model (level %d)\n", level) ;
                return DcmQRIndexDatabaseError ;
            }
        }

        /**** If current level is above the QueryLevel
        ***/

        else if (level < queryLevel) {

            /** For this level, only unique keys are allowed
            ** Parse the request list elements reffering to
            ** this level.
            ** Check that only unique key attr are provided
            */

            int uniqueKeyFound = OFFalse ;
            for (plist = findRequestList ; plist ; plist = plist->next) {
                DB_GetTagLevel (plist->elem. XTag, &XTagLevel) ;
                if (XTagLevel != level)
                    continue ;
                DB_GetTagKeyAttr (plist->elem. XTag, &XTagType) ;
                if (XTagType != UNIQUE_KEY) {
                    dbdebug(1,"Non Unique Key found (level %d)\n", level) ;
                    return DcmQRIndexDatabaseError ;
                }
                else if (uniqueKeyFound) {
                    dbdebug(1,"More than one Unique Key found (level %d)\n", level) ;
                    return DcmQRIndexDatabaseError ;
                }
                else
                    uniqueKeyFound = OFTrue ;
            }
        }

        /**** If current level is the QueryLevel
        ***/

        else if (level == queryLevel) {

            /** For this level, all keys are allowed
            ** Parse the request list elements reffering to
            ** this level.
            ** Check that at least one key is provided
            */

            int atLeastOneKeyFound = OFFalse ;
            for (plist = findRequestList ; plist ; plist = plist->next) {
                DB_GetTagLevel (plist->elem. XTag, &XTagLevel) ;
                if (XTagLevel != level)
                    continue ;
                atLeastOneKeyFound = OFTrue ;
            }
            if (! atLeastOneKeyFound) {
                dbdebug(1,"No Key found at query level (level %d)\n", level) ;
                return DcmQRIndexDatabaseError ;
            }
        }

        /**** If current level beyond the QueryLevel
        ***/

        else if (level > queryLevel) {

            /** For this level, no key is allowed
            ** Parse the request list elements reffering to
            ** this level.
            ** Check that no key is provided
            */

            int atLeastOneKeyFound = OFFalse ;
            for (plist = findRequestList ; plist ; plist = plist->next) {
                DB_GetTagLevel (plist->elem. XTag, &XTagLevel) ;
                if (XTagLevel != level)
                    continue ;
                atLeastOneKeyFound = OFTrue ;
            }
            if (atLeastOneKeyFound) {
                dbdebug(1,"Key found beyond query level (level %d)\n", level) ;
                return DcmQRIndexDatabaseError ;
            }
        }

    }
    return EC_Normal ;
}


/************
**      Hierarchical Search Algorithm
**      Returns OFTrue if matching is ok, else returns OFFalse
 */

OFCondition DcmQueryRetrieveIndexDatabaseHandle::hierarchicalCompare (
                DB_Private_Handle       *phandle,
                IdxRecord               *idxRec,
                DB_LEVEL                level,
                DB_LEVEL                infLevel,
                int                     *match)
{
    int                 i ;
    DcmTagKey   XTag ;
    DB_ElementList *plist ;
    DB_LEVEL    XTagLevel ;

    /**** If current level is above the QueryLevel
    ***/

    if (level < phandle->queryLevel) {

        /** Get UID Tag for current level
         */

        DB_GetUIDTag (level, &XTag) ;

        /** Find Element with this XTag in Identifier list
         */

        for (plist = phandle->findRequestList ; plist ; plist = plist->next)
            if (plist->elem. XTag == XTag)
                break ;

        /** Element not found
         */

        if (plist == NULL) {
            *match = OFFalse ;
            CERR << "hierarchicalCompare : No UID Key found at level " << (int) level << endl;
            return DcmQRIndexDatabaseError ;
        }

        /** Find element with the same XTag in index record
         */

        for (i = 0 ; i < NBPARAMETERS ; i++)
            if (idxRec->param [i]. XTag == XTag)
                break ;

        /** Compare with Single value matching
        ** If Match fails, return OFFalse
        */

        if (! dbmatch (&(plist->elem), &idxRec->param[i])) {
            *match = OFFalse ;
            return EC_Normal ;
        }

        /** Match succeeded.
        ** Try at next level
        */

        return hierarchicalCompare (phandle, idxRec, (DB_LEVEL)(level + 1), infLevel, match) ;
    }

    /**** If current level is the QueryLevel
    ***/

    else if (level == phandle->queryLevel) {

        /*** For each element in Identifier list
        **/

        for (plist = phandle->findRequestList ; plist ; plist = plist->next) {

            /** Get the Tag level of this element
             */

            DB_GetTagLevel (plist->elem. XTag, &XTagLevel) ;

            /** If we are in the Study Root Information Model exception
            ** we must accept patients keys at the study level
            */

            if (  (XTagLevel == PATIENT_LEVEL)
                  && (phandle->queryLevel == STUDY_LEVEL)
                  && (infLevel == STUDY_LEVEL)
                ) ;

            /** In other cases, only keys at the current level are
            ** taken into account. So skip this element.
            */

            else if (XTagLevel != level)
                continue ;

            /** Find element with the same XTag in index record
             */

            for (i = 0 ; i < NBPARAMETERS ; i++)
                if (idxRec->param [i]. XTag == plist->elem. XTag)
                    break ;

            /** Compare with appropriate Matching.
            ** If Match fails, return OFFalse
            */


            if (! dbmatch (&(plist->elem), &idxRec->param[i])) {
                *match = OFFalse ;
                return EC_Normal ;
            }
        }

        /*** If we are here, all matches succeeded at the current level.
        *** Perhaps check that we have tried at least one match ??
        **/

        *match = OFTrue ;
        return EC_Normal ;

    }
    return DcmQRIndexDatabaseError;
}

/********************
**      Start find in Database
**/

OFCondition DcmQueryRetrieveIndexDatabaseHandle::startFindRequest(
                const char      *SOPClassUID,
                DcmDataset      *findRequestIdentifiers,
                DcmQueryRetrieveDatabaseStatus  *status)
{
    DB_SmallDcmElmt     elem ;
    DB_ElementList      *plist = NULL;
    DB_ElementList      *last = NULL;
    int                 MatchFound ;
    IdxRecord           idxRec ;
    DB_LEVEL            qLevel = PATIENT_LEVEL; // highest legal level for a query in the current model
    DB_LEVEL            lLevel = IMAGE_LEVEL;   // lowest legal level for a query in the current model

    OFCondition         cond = EC_Normal;
    OFBool qrLevelFound = OFFalse;

    /**** Is SOPClassUID supported ?
    ***/

    if (strcmp( SOPClassUID, UID_FINDPatientRootQueryRetrieveInformationModel) == 0)
        handle->rootLevel = PATIENT_ROOT ;
    else if (strcmp( SOPClassUID, UID_FINDStudyRootQueryRetrieveInformationModel) == 0)
        handle->rootLevel = STUDY_ROOT ;
#ifndef NO_PATIENTSTUDYONLY_SUPPORT
    else if (strcmp( SOPClassUID, UID_FINDPatientStudyOnlyQueryRetrieveInformationModel) == 0)
        handle->rootLevel = PATIENT_STUDY ;
#endif
    else {
        status->setStatus(STATUS_FIND_Refused_SOPClassNotSupported);
        return (DcmQRIndexDatabaseError) ;
    }


    /**** Parse Identifiers in the Dicom Object
    **** Find Query Level and contruct a list
    **** of query identifiers
    ***/

    handle->findRequestList = NULL ;

    int elemCount = (int)(findRequestIdentifiers->card());
    for (int elemIndex=0; elemIndex<elemCount; elemIndex++) {

        DcmElement* dcelem = findRequestIdentifiers->getElement(elemIndex);

        elem.XTag = dcelem->getTag().getXTag();
        if (elem.XTag == DCM_QueryRetrieveLevel || DB_TagSupported(elem.XTag)) {
            elem.ValueLength = dcelem->getLength();
            if (elem.ValueLength == 0) {
                elem.PValueField = NULL ;
            } else if ((elem.PValueField = (char*)malloc((size_t)(elem.ValueLength+1))) == NULL) {
                status->setStatus(STATUS_FIND_Refused_OutOfResources);
                return (DcmQRIndexDatabaseError) ;
            } else {
                /* only char string type tags are supported at the moment */
                char *s = NULL;
                dcelem->getString(s);
                strcpy(elem.PValueField, s);
            }
            /** If element is the Query Level, store it in handle
             */

            if (elem. XTag == DCM_QueryRetrieveLevel) {
                char *pc ;
                char level [50] ;

                strncpy(level, (char*)elem.PValueField,
                        (elem.ValueLength<50)? (size_t)(elem.ValueLength) : 49) ;

                /*** Skip this two lines if you want strict comparison
                **/

                for (pc = level ; *pc ; pc++)
                    *pc = ((*pc >= 'a') && (*pc <= 'z')) ? 'A' - 'a' + *pc : *pc ;

                if (strncmp (level, PATIENT_LEVEL_STRING,
                             strlen (PATIENT_LEVEL_STRING)) == 0)
                    handle->queryLevel = PATIENT_LEVEL ;
                else if (strncmp (level, STUDY_LEVEL_STRING,
                                  strlen (STUDY_LEVEL_STRING)) == 0)
                    handle->queryLevel = STUDY_LEVEL ;
                else if (strncmp (level, SERIE_LEVEL_STRING,
                                  strlen (SERIE_LEVEL_STRING)) == 0)
                    handle->queryLevel = SERIE_LEVEL ;
                else if (strncmp (level, IMAGE_LEVEL_STRING,
                                  strlen (IMAGE_LEVEL_STRING)) == 0)
                    handle->queryLevel = IMAGE_LEVEL ;
                else {
                    if (elem. PValueField)
                        free (elem. PValueField) ;
#ifdef DEBUG
                    dbdebug(1, "DB_startFindRequest () : Illegal query level (%s)\n", level) ;
#endif
                    status->setStatus(STATUS_FIND_Failed_UnableToProcess);
                    return (DcmQRIndexDatabaseError) ;
                }
                qrLevelFound = OFTrue;
            } else {
                /** Else it is a query identifier.
                ** Append it to our RequestList if it is supported
                */
                if (DB_TagSupported (elem. XTag)) {

                    plist = (DB_ElementList *) malloc (sizeof (DB_ElementList)) ;
                    if (plist == NULL) {
                        status->setStatus(STATUS_FIND_Refused_OutOfResources);
                        return (DcmQRIndexDatabaseError) ;
                    }
                    plist->next = NULL ;
                    DB_DuplicateElement (&elem, &(plist->elem)) ;
                    if (handle->findRequestList == NULL) {
                        handle->findRequestList = last = plist ;
                    } else {
                        last->next = plist ;
                        last = plist ;
                    }
                }
            }

            if ( elem. PValueField ) {
                free (elem. PValueField) ;
            }
        }
    }

    if (!qrLevelFound) {
        /* The Query/Retrieve Level is missing */
        status->setStatus(STATUS_FIND_Failed_IdentifierDoesNotMatchSOPClass);
        CERR << "DB_startFindRequest(): missing Query/Retrieve Level" << endl;
        handle->idxCounter = -1 ;
        DB_FreeElementList (handle->findRequestList) ;
        handle->findRequestList = NULL ;
        return (DcmQRIndexDatabaseError) ;
    }

    switch (handle->rootLevel)
    {
      case PATIENT_ROOT :
        qLevel = PATIENT_LEVEL ;
        lLevel = IMAGE_LEVEL ;
        break ;
      case STUDY_ROOT :
        qLevel = STUDY_LEVEL ;
        lLevel = IMAGE_LEVEL ;
        break ;
      case PATIENT_STUDY:
        qLevel = PATIENT_LEVEL ;
        lLevel = STUDY_LEVEL ;
        break ;
    }

    /**** Test the consistency of the request list
    ***/

    if (doCheckFindIdentifier) {
        cond = testFindRequestList (handle->findRequestList, handle->queryLevel, qLevel, lLevel) ;
        if (cond != EC_Normal) {
            handle->idxCounter = -1 ;
            DB_FreeElementList (handle->findRequestList) ;
            handle->findRequestList = NULL ;
#ifdef DEBUG
            dbdebug(1, "DB_startFindRequest () : STATUS_FIND_Failed_IdentifierDoesNotMatchSOPClass - Invalid RequestList\n") ;
#endif
            status->setStatus(STATUS_FIND_Failed_IdentifierDoesNotMatchSOPClass);
            return (cond) ;
        }
    }

    /**** Goto the beginning of Index File
    **** Then find the first matching image
    ***/

    DB_lock(OFFalse);

    DB_IdxInitLoop (&(handle->idxCounter)) ;
    MatchFound = OFFalse ;
    cond = EC_Normal ;

    while (1) {

        /*** Exit loop if read error (or end of file)
        **/

        if (DB_IdxGetNext (&(handle->idxCounter), &idxRec) != EC_Normal)
            break ;

        /*** Exit loop if error or matching OK
        **/

        cond = hierarchicalCompare (handle, &idxRec, qLevel, qLevel, &MatchFound) ;
        if (cond != EC_Normal)
            break ;
        if (MatchFound)
            break ;
    }

    /**** If an error occured in Matching function
    ****    return a failed status
    ***/

    if (cond != EC_Normal) {
        handle->idxCounter = -1 ;
        DB_FreeElementList (handle->findRequestList) ;
        handle->findRequestList = NULL ;
#ifdef DEBUG
        dbdebug(1, "DB_startFindRequest () : STATUS_FIND_Failed_UnableToProcess\n") ;
#endif
        status->setStatus(STATUS_FIND_Failed_UnableToProcess);

        DB_unlock();

        return (cond) ;
    }


    /**** If a matching image has been found,
    ****         add index record to UID found list
    ****    prepare Response List in handle
    ****    return status is pending
    ***/

    if (MatchFound) {
        DB_UIDAddFound (handle, &idxRec) ;
        makeResponseList (handle, &idxRec) ;
#ifdef DEBUG
        dbdebug(1, "DB_startFindRequest () : STATUS_Pending\n") ;
#endif
        status->setStatus(STATUS_Pending);
        return (EC_Normal) ;
    }

    /**** else no matching image has been found,
    ****    free query identifiers list
    ****    status is success
    ***/

    else {
        handle->idxCounter = -1 ;
        DB_FreeElementList (handle->findRequestList) ;
        handle->findRequestList = NULL ;
#ifdef DEBUG
        dbdebug(1, "DB_startFindRequest () : STATUS_Success\n") ;
#endif
        status->setStatus(STATUS_Success);

        DB_unlock();

        return (EC_Normal) ;
    }

}

/********************
**      Get next find response in Database
 */

OFCondition DcmQueryRetrieveIndexDatabaseHandle::nextFindResponse (
                DcmDataset      **findResponseIdentifiers,
                DcmQueryRetrieveDatabaseStatus  *status)
{

    DB_ElementList      *plist = NULL;
    int                 MatchFound = OFFalse;
    IdxRecord           idxRec ;
    DB_LEVEL            qLevel = PATIENT_LEVEL;
    const char          *queryLevelString = NULL;
    OFCondition         cond = EC_Normal;

    if (handle->findResponseList == NULL) {
#ifdef DEBUG
        dbdebug(1, "DB_nextFindResponse () : STATUS_Success\n") ;
#endif
        *findResponseIdentifiers = NULL ;
        status->setStatus(STATUS_Success);

        DB_unlock();

        return (EC_Normal) ;
    }

    /***** Create the response (findResponseIdentifiers) using
    ***** the last find done and saved in handle findResponseList
    ****/

    *findResponseIdentifiers = new DcmDataset ;
    if ( *findResponseIdentifiers != NULL ) {

        /*** Put responses
        **/

        for ( plist = handle->findResponseList ; plist != NULL ; plist = plist->next ) {
            DcmTag t(plist->elem.XTag);
            DcmElement *dce = newDicomElement(t);
            if (dce == NULL) {
                status->setStatus(STATUS_FIND_Refused_OutOfResources);
                return DcmQRIndexDatabaseError;
            }
            if (plist->elem.PValueField != NULL &&
                strlen(plist->elem.PValueField) > 0) {
                OFCondition ec = dce->putString(plist->elem.PValueField);
                if (ec != EC_Normal) {
                    CERR << "dbfind: DB_nextFindResponse: cannot put()" << endl;
                    status->setStatus(STATUS_FIND_Failed_UnableToProcess);
                    return DcmQRIndexDatabaseError;
                }
            }
            OFCondition ec = (*findResponseIdentifiers)->insert(dce, OFTrue /*replaceOld*/);
            if (ec != EC_Normal) {
                CERR << "dbfind: DB_nextFindResponse: cannot insert()" << endl;
                status->setStatus(STATUS_FIND_Failed_UnableToProcess);
                return DcmQRIndexDatabaseError;
            }
        }

        /*** Append the Query level
        **/

        switch (handle->queryLevel) {
        case PATIENT_LEVEL :
            queryLevelString = PATIENT_LEVEL_STRING ;
            break ;
        case STUDY_LEVEL :
            queryLevelString = STUDY_LEVEL_STRING ;
            break ;
        case SERIE_LEVEL :
            queryLevelString = SERIE_LEVEL_STRING ;
            break ;
        case IMAGE_LEVEL :
            queryLevelString = IMAGE_LEVEL_STRING ;
            break ;
        }
        DU_putStringDOElement(*findResponseIdentifiers,
                              DCM_QueryRetrieveLevel, queryLevelString);
#ifdef DEBUG
        if (debugLevel > 0) {
            COUT << "DB: findResponseIdentifiers:" << endl;
            (*findResponseIdentifiers)->print(COUT);
        }
#endif
    }
    else {

        DB_unlock();

        return (DcmQRIndexDatabaseError) ;
    }

    switch (handle->rootLevel) {
    case PATIENT_ROOT : qLevel = PATIENT_LEVEL ;        break ;
    case STUDY_ROOT :   qLevel = STUDY_LEVEL ;          break ;
    case PATIENT_STUDY: qLevel = PATIENT_LEVEL ;        break ;
    }

    /***** Free the last response...
    ****/

    DB_FreeElementList (handle->findResponseList) ;
    handle->findResponseList = NULL ;

    /***** ... and find the next one
    ****/

    MatchFound = OFFalse ;
    cond = EC_Normal ;

    while (1) {

        /*** Exit loop if read error (or end of file)
        **/

        if (DB_IdxGetNext (&(handle->idxCounter), &idxRec) != EC_Normal)
            break ;

        /*** If Response already found
        **/

        if (DB_UIDAlreadyFound (handle, &idxRec))
            continue ;

        /*** Exit loop if error or matching OK
        **/

        cond = hierarchicalCompare (handle, &idxRec, qLevel, qLevel, &MatchFound) ;
        if (cond != EC_Normal)
            break ;
        if (MatchFound)
            break ;

    }

    /**** If an error occured in Matching function
    ****    return status is pending
    ***/

    if (cond != EC_Normal) {
        handle->idxCounter = -1 ;
        DB_FreeElementList (handle->findRequestList) ;
        handle->findRequestList = NULL ;
#ifdef DEBUG
        dbdebug(1, "DB_nextFindResponse () : STATUS_FIND_Failed_UnableToProcess\n") ;
#endif
        status->setStatus(STATUS_FIND_Failed_UnableToProcess);

        DB_unlock();

        return (cond) ;
    }


    /**** If a matching image has been found
    ****    add index records UIDs in found UID list
    ****    prepare Response List in handle
    ***/

    if (MatchFound) {
        DB_UIDAddFound (handle, &idxRec) ;
        makeResponseList (handle, &idxRec) ;
#ifdef DEBUG
        dbdebug(1, "DB_nextFindResponse () : STATUS_Pending\n") ;
#endif
        status->setStatus(STATUS_Pending);
        return (EC_Normal) ;
    }

    /**** else no matching image has been found,
    ****    free query identifiers list
    **** Response list is null, so next call will return STATUS_Success
    ***/

    else {
        handle->idxCounter = -1 ;
        DB_FreeElementList (handle->findRequestList) ;
        handle->findRequestList = NULL ;
        DB_FreeUidList (handle->uidList) ;
        handle->uidList = NULL ;
    }


#ifdef DEBUG
    dbdebug(1, "DB_nextFindResponse () : STATUS_Pending\n") ;
#endif
    status->setStatus(STATUS_Pending);
    return (EC_Normal) ;
}

/********************
**      Cancel find request
 */

OFCondition DcmQueryRetrieveIndexDatabaseHandle::cancelFindRequest (DcmQueryRetrieveDatabaseStatus *status)
{

    handle->idxCounter = -1 ;
    DB_FreeElementList (handle->findRequestList) ;
    handle->findRequestList = NULL ;
    DB_FreeElementList (handle->findResponseList) ;
    handle->findResponseList = NULL ;
    DB_FreeUidList (handle->uidList) ;
    handle->uidList = NULL ;

    status->setStatus(STATUS_FIND_Cancel_MatchingTerminatedDueToCancelRequest);

    DB_unlock();

    return (EC_Normal) ;
}

/* ========================= MOVE ========================= */

/************
 *      Test a Move Request List
 *      Returns EC_Normal if ok, else returns DcmQRIndexDatabaseError
 */

OFCondition DcmQueryRetrieveIndexDatabaseHandle::testMoveRequestList (
                DB_ElementList  *findRequestList,
                DB_LEVEL        queryLevel,
                DB_LEVEL        infLevel,
                DB_LEVEL        lowestLevel
                )
{
    DB_ElementList *plist ;
    DB_LEVEL    XTagLevel ;
    DB_KEY_TYPE         XTagType ;
    int level ;

    /**** Query level must be at least the infLevel
    ***/

    if (queryLevel < infLevel) {
        dbdebug(1,"Level incompatible with Information Model (level %d)\n", (int)queryLevel) ;
        return DcmQRIndexDatabaseError ;
    }

    if (queryLevel > lowestLevel) {
        dbdebug(1,"Level incompatible with Information Model (level %d)\n", (int)queryLevel) ;
        return DcmQRIndexDatabaseError ;
    }

    for (level = PATIENT_LEVEL ; level <= IMAGE_LEVEL ; level++) {

        /**** Manage exception due to StudyRoot Information Model :
        **** In this information model, move may not include any
        **** Patient attributes.
        ***/

        if ((level == PATIENT_LEVEL) && (infLevel == STUDY_LEVEL)) {

            /** In Study Root Information Model, do not accept any
            ** Patient Tag
            */

            int atLeastOneKeyFound = OFFalse ;
            for (plist = findRequestList ; plist ; plist = plist->next) {
                DB_GetTagLevel (plist->elem. XTag, &XTagLevel) ;
                if (XTagLevel != level)
                    continue ;
                atLeastOneKeyFound = OFTrue ;
            }
            if (atLeastOneKeyFound) {
                dbdebug(1,"Key found in Study Root Information Model (level %d)\n", level) ;
                return DcmQRIndexDatabaseError ;
            }
        }

        /**** If current level is above or equal to the QueryLevel
        ***/

        else if (level <= queryLevel) {

            /** For these levels, only unique keys are allowed
            ** Parse the request list elements reffering to
            ** this level.
            ** Check that only unique key attr are provided
            */

            int uniqueKeyFound = OFFalse ;
            for (plist = findRequestList ; plist ; plist = plist->next) {
                DB_GetTagLevel (plist->elem. XTag, &XTagLevel) ;
                if (XTagLevel != level)
                    continue ;
                DB_GetTagKeyAttr (plist->elem. XTag, &XTagType) ;
                if (XTagType != UNIQUE_KEY) {
                    dbdebug(1,"Non Unique Key found (level %d)\n", level) ;
                    return DcmQRIndexDatabaseError ;
                }
                else if (uniqueKeyFound) {
                    dbdebug(1,"More than one Unique Key found (level %d)\n", level) ;
                    return DcmQRIndexDatabaseError ;
                }
                else
                    uniqueKeyFound = OFTrue ;
            }
            if (! uniqueKeyFound) {
                dbdebug(1,"No Unique Key found (level %d)\n", level) ;
                return DcmQRIndexDatabaseError ;
            }
        }

        /**** If current level beyond the QueryLevel
        ***/

        else if (level > queryLevel) {

            /** For this level, no key is allowed
            ** Parse the request list elements reffering to
            ** this level.
            ** Check that no key is provided
            */

            int atLeastOneKeyFound = OFFalse ;
            for (plist = findRequestList ; plist ; plist = plist->next) {
                DB_GetTagLevel (plist->elem. XTag, &XTagLevel) ;
                if (XTagLevel != level)
                    continue ;
                atLeastOneKeyFound = OFTrue ;
            }
            if (atLeastOneKeyFound) {
                dbdebug(1,"Key found beyond query level (level %d)\n", level) ;
                return DcmQRIndexDatabaseError ;
            }
        }

    }
    return EC_Normal ;
}



OFCondition DcmQueryRetrieveIndexDatabaseHandle::startMoveRequest(
        const char      *SOPClassUID,
        DcmDataset      *moveRequestIdentifiers,
        DcmQueryRetrieveDatabaseStatus  *status)
{

    DB_SmallDcmElmt     elem ;
    DB_ElementList      *plist = NULL;
    DB_ElementList      *last = NULL;
    DB_CounterList      *pidxlist = NULL;
    DB_CounterList      *lastidxlist = NULL;
    int                 MatchFound = OFFalse;
    IdxRecord           idxRec ;
    DB_LEVEL            qLevel = PATIENT_LEVEL; // highest legal level for a query in the current model
    DB_LEVEL            lLevel = IMAGE_LEVEL;   // lowest legal level for a query in the current model
    OFCondition         cond = EC_Normal;
    OFBool qrLevelFound = OFFalse;

    /**** Is SOPClassUID supported ?
    ***/

    if (strcmp( SOPClassUID, UID_MOVEPatientRootQueryRetrieveInformationModel) == 0)
        handle->rootLevel = PATIENT_ROOT ;
    else if (strcmp( SOPClassUID, UID_MOVEStudyRootQueryRetrieveInformationModel) == 0)
        handle->rootLevel = STUDY_ROOT ;
#ifndef NO_PATIENTSTUDYONLY_SUPPORT
    else if (strcmp( SOPClassUID, UID_MOVEPatientStudyOnlyQueryRetrieveInformationModel) == 0)
        handle->rootLevel = PATIENT_STUDY ;
#endif
#ifndef NO_GET_SUPPORT
    /* experimental support for GET */
    else if (strcmp( SOPClassUID, UID_GETPatientRootQueryRetrieveInformationModel) == 0)
        handle->rootLevel = PATIENT_ROOT ;
    else if (strcmp( SOPClassUID, UID_GETStudyRootQueryRetrieveInformationModel) == 0)
        handle->rootLevel = STUDY_ROOT ;
#ifndef NO_PATIENTSTUDYONLY_SUPPORT
    else if (strcmp( SOPClassUID, UID_GETPatientStudyOnlyQueryRetrieveInformationModel) == 0)
        handle->rootLevel = PATIENT_STUDY ;
#endif
#endif

    else {
        status->setStatus(STATUS_MOVE_Failed_SOPClassNotSupported);
        return (DcmQRIndexDatabaseError) ;
    }


    /**** Parse Identifiers in the Dicom Object
    **** Find Query Level and contruct a list
    **** of query identifiers
    ***/

    int elemCount = (int)(moveRequestIdentifiers->card());
    for (int elemIndex=0; elemIndex<elemCount; elemIndex++) {

        DcmElement* dcelem = moveRequestIdentifiers->getElement(elemIndex);

        elem.XTag = dcelem->getTag().getXTag();
        if (elem.XTag == DCM_QueryRetrieveLevel || DB_TagSupported(elem.XTag)) {
            elem.ValueLength = dcelem->getLength();
            if (elem.ValueLength == 0) {
                elem.PValueField = NULL ;
            } else if ((elem.PValueField = (char*)malloc((size_t)(elem.ValueLength+1))) == NULL) {
                status->setStatus(STATUS_MOVE_Failed_UnableToProcess);
                return (DcmQRIndexDatabaseError) ;
            } else {
                /* only char string type tags are supported at the moment */
                char *s = NULL;
                dcelem->getString(s);
                strcpy(elem.PValueField, s);
            }

            /** If element is the Query Level, store it in handle
             */

            if (elem. XTag == DCM_QueryRetrieveLevel) {
                char *pc ;
                char level [50] ;

                strncpy (level, (char *) elem. PValueField, (size_t)((elem. ValueLength < 50) ? elem. ValueLength : 49)) ;

                /*** Skip this two lines if you want strict comparison
                **/

                for (pc = level ; *pc ; pc++)
                    *pc = ((*pc >= 'a') && (*pc <= 'z')) ? 'A' - 'a' + *pc : *pc ;

                if (strncmp (level, PATIENT_LEVEL_STRING,
                             strlen (PATIENT_LEVEL_STRING)) == 0)
                    handle->queryLevel = PATIENT_LEVEL ;
                else if (strncmp (level, STUDY_LEVEL_STRING,
                                  strlen (STUDY_LEVEL_STRING)) == 0)
                    handle->queryLevel = STUDY_LEVEL ;
                else if (strncmp (level, SERIE_LEVEL_STRING,
                                  strlen (SERIE_LEVEL_STRING)) == 0)
                    handle->queryLevel = SERIE_LEVEL ;
                else if (strncmp (level, IMAGE_LEVEL_STRING,
                                  strlen (IMAGE_LEVEL_STRING)) == 0)
                    handle->queryLevel = IMAGE_LEVEL ;
                else {
#ifdef DEBUG
                    dbdebug(1,"DB_startMoveRequest : STATUS_MOVE_Failed_UnableToProcess\n") ;
#endif
                    status->setStatus(STATUS_MOVE_Failed_UnableToProcess);
                    return (DcmQRIndexDatabaseError) ;
                }
                qrLevelFound = OFTrue;
            } else {
                /** Else it is a query identifier
                ** Append it to our RequestList
                */
                if (! DB_TagSupported (elem. XTag))
                    continue ;

                plist = (DB_ElementList *) malloc (sizeof( DB_ElementList ) ) ;
                if (plist == NULL) {
                    status->setStatus(STATUS_FIND_Refused_OutOfResources);
                    return (DcmQRIndexDatabaseError) ;
                }
                plist->next = NULL ;
                DB_DuplicateElement (&elem, & (plist->elem)) ;
                if (handle->findRequestList == NULL) {
                    handle->findRequestList = last = plist ;
                } else {
                    last->next = plist ;
                    last = plist ;
                }
            }

            if ( elem. PValueField ) {
                free (elem. PValueField) ;
            }
        }
    }

    if (!qrLevelFound) {
        /* The Query/Retrieve Level is missing */
        status->setStatus(STATUS_MOVE_Failed_IdentifierDoesNotMatchSOPClass);
        CERR << "DB_startMoveRequest(): missing Query/Retrieve Level" << endl;
        handle->idxCounter = -1 ;
        DB_FreeElementList (handle->findRequestList) ;
        handle->findRequestList = NULL ;
        return (DcmQRIndexDatabaseError) ;
    }

    switch (handle->rootLevel)
    {
      case PATIENT_ROOT :
        qLevel = PATIENT_LEVEL ;
        lLevel = IMAGE_LEVEL ;
        break ;
      case STUDY_ROOT :
        qLevel = STUDY_LEVEL ;
        lLevel = IMAGE_LEVEL ;
        break ;
      case PATIENT_STUDY:
        qLevel = PATIENT_LEVEL ;
        lLevel = STUDY_LEVEL ;
        break ;
    }

    /**** Test the consistency of the request list
    ***/

    if (doCheckMoveIdentifier) {
        cond = testMoveRequestList (handle->findRequestList,
                                       handle->queryLevel, qLevel, lLevel) ;
        if (cond != EC_Normal) {
            handle->idxCounter = -1 ;
            DB_FreeElementList (handle->findRequestList) ;
            handle->findRequestList = NULL ;
#ifdef DEBUG
            dbdebug(1,"DB_startMoveRequest () : STATUS_MOVE_Failed_IdentifierDoesNotMatchSOPClass - Invalid RequestList\n") ;
#endif
            status->setStatus(STATUS_MOVE_Failed_IdentifierDoesNotMatchSOPClass);
            return (cond) ;
        }
    }

    /**** Goto the beginning of Index File
    **** Then find all matching images
    ***/

    MatchFound = OFFalse ;
    handle->moveCounterList = NULL ;
    handle->NumberRemainOperations = 0 ;

    /**** Find matching images
    ***/

    DB_lock(OFFalse);

    DB_IdxInitLoop (&(handle->idxCounter)) ;
    while (1) {

        /*** Exit loop if read error (or end of file)
        **/

        if (DB_IdxGetNext (&(handle->idxCounter), &idxRec) != EC_Normal)
            break ;

        /*** If matching found
        **/

        cond = hierarchicalCompare (handle, &idxRec, qLevel, qLevel, &MatchFound) ;
        if (MatchFound) {
            pidxlist = (DB_CounterList *) malloc (sizeof( DB_CounterList ) ) ;
            if (pidxlist == NULL) {
                status->setStatus(STATUS_FIND_Refused_OutOfResources);
                return (DcmQRIndexDatabaseError) ;
            }

            pidxlist->next = NULL ;
            pidxlist->idxCounter = handle->idxCounter ;
            handle->NumberRemainOperations++ ;
            if ( handle->moveCounterList == NULL )
                handle->moveCounterList = lastidxlist = pidxlist ;
            else {
                lastidxlist->next = pidxlist ;
                lastidxlist = pidxlist ;
            }

        }
    }

    DB_FreeElementList (handle->findRequestList) ;
    handle->findRequestList = NULL ;

    /**** If a matching image has been found,
    ****    status is pending
    ***/

    if ( handle->NumberRemainOperations > 0 ) {
#ifdef DEBUG
        dbdebug(1,"DB_startMoveRequest : STATUS_Pending\n") ;
#endif
        status->setStatus(STATUS_Pending);
        return (EC_Normal) ;
    }

    /**** else no matching image has been found,
    ****    free query identifiers list
    ****    status is success
    ***/

    else {
        handle->idxCounter = -1 ;
#ifdef DEBUG
        dbdebug(1,"DB_startMoveRequest : STATUS_Success\n") ;
#endif
        status->setStatus(STATUS_Success);

        DB_unlock();

        return (EC_Normal) ;
    }


}

OFCondition DcmQueryRetrieveIndexDatabaseHandle::nextMoveResponse(
                char            *SOPClassUID,
                char            *SOPInstanceUID,
                char            *imageFileName,
                unsigned short  *numberOfRemainingSubOperations,
                DcmQueryRetrieveDatabaseStatus  *status)
{
    IdxRecord           idxRec ;
    DB_CounterList              *nextlist ;

    /**** If all matching images have been retrieved,
    ****    status is success
    ***/

    if ( handle->NumberRemainOperations <= 0 ) {
        status->setStatus(STATUS_Success);

        DB_unlock();

        return (EC_Normal) ;
    }

    /**** Goto the next matching image number of Index File
    ***/

    if (DB_IdxRead (handle->moveCounterList->idxCounter, &idxRec) != EC_Normal) {
#ifdef DEBUG
        dbdebug(1,"DB_nextMoveResponse : STATUS_MOVE_Failed_UnableToProcess\n") ;
#endif
        status->setStatus(STATUS_MOVE_Failed_UnableToProcess);

        DB_unlock();

        return (DcmQRIndexDatabaseError) ;
    }

    strcpy (SOPClassUID, (char *) idxRec. SOPClassUID) ;
    strcpy (SOPInstanceUID, (char *) idxRec. SOPInstanceUID) ;
    strcpy (imageFileName, (char *) idxRec. filename) ;

    *numberOfRemainingSubOperations = --handle->NumberRemainOperations ;


    nextlist = handle->moveCounterList->next ;
    free (handle->moveCounterList) ;
    handle->moveCounterList = nextlist ;
    status->setStatus(STATUS_Pending);
#ifdef DEBUG
    dbdebug(1,"DB_nextMoveResponse : STATUS_Pending\n") ;
#endif
    return (EC_Normal) ;
}



OFCondition DcmQueryRetrieveIndexDatabaseHandle::cancelMoveRequest (DcmQueryRetrieveDatabaseStatus *status)
{
    DB_CounterList              *plist ;

    while (handle->moveCounterList) {
        plist  = handle->moveCounterList ;
        handle->moveCounterList = handle->moveCounterList->next ;
        free (plist) ;
    }

    status->setStatus(STATUS_MOVE_Cancel_SubOperationsTerminatedDueToCancelIndication);

    DB_unlock();

    return (EC_Normal) ;
}


/* ========================= STORE ========================= */


void DcmQueryRetrieveIndexDatabaseHandle::enableQuotaSystem(OFBool enable)
{
    quotaSystemEnabled = enable;
}


/*
** Image file deleting
*/


OFCondition DcmQueryRetrieveIndexDatabaseHandle::deleteImageFile(char* imgFile)
{
    if (!quotaSystemEnabled) {
    CERR << "DB QUOTA: disabled: retaining: " << imgFile << endl;
    return EC_Normal;
    } else {
    CERR << "DB QUOTA: enabled: deleting: " << imgFile << endl;
    }

#ifdef LOCK_IMAGE_FILES
    int lockfd;
#ifdef O_BINARY
    lockfd = open(imgFile, O_RDWR | O_BINARY, 0666);    /* obtain file descriptor */
#else
    lockfd = open(imgFile, O_RDWR, 0666);   /* obtain file descriptor */
#endif
    if (lockfd < 0) {
    CERR << "DB ERROR: cannot open image file for deleting: " << imgFile << endl;
    return DcmQRIndexDatabaseError;
    }
    if (dcmtk_flock(lockfd, LOCK_EX) < 0) { /* exclusive lock (blocking) */
    CERR << "DB ERROR: cannot lock image file  for deleting: " << imgFile << endl;
        dcmtk_plockerr("DB ERROR");
    }
#endif

    if (unlink(imgFile) < 0) {
        /* delete file */
    CERR << "DB ERROR: cannot delete image file: " << imgFile << endl
        << "DcmQRIndexDatabaseError: " << strerror(errno) << endl;
   }

#ifdef LOCK_IMAGE_FILES
    if (dcmtk_flock(lockfd, LOCK_UN) < 0) { /* unlock */
    CERR << "DB ERROR: cannot unlock image file  for deleting: " << imgFile << endl;
        dcmtk_plockerr("DB ERROR");
     }
    close(lockfd);              /* release file descriptor */
#endif

    return EC_Normal;
}


/*************************
**   Delete oldest study in database
 */

int DcmQueryRetrieveIndexDatabaseHandle::deleteOldestStudy(StudyDescRecord *pStudyDesc)
{

    int oldestStudy ;
    double OldestDate ;
    int s ;
    int n ;
    int idx = 0 ;
    IdxRecord idxRec ;

    oldestStudy = 0 ;
    OldestDate = 0.0 ;

#ifdef DEBUG
    dbdebug(1, "deleteOldestStudy\n") ;
#endif

    for ( s = 0 ; s < handle -> maxStudiesAllowed ; s++ ) {
    if ( ( pStudyDesc[s]. NumberofRegistratedImages != 0 ) &&
        ( ( OldestDate == 0.0 ) || ( pStudyDesc[s]. LastRecordedDate < OldestDate ) ) ) {
        OldestDate = pStudyDesc[s]. LastRecordedDate ;
        oldestStudy = s ;
    }
    }

#ifdef DEBUG
    dbdebug(1, "deleteOldestStudy oldestStudy = %d\n",oldestStudy) ;
#endif

    n = strlen(pStudyDesc[oldestStudy].StudyInstanceUID) ;
    while ( DB_IdxRead (idx, &idxRec) == EC_Normal ) {

    if ( ! ( strncmp(idxRec. StudyInstanceUID, pStudyDesc[oldestStudy].StudyInstanceUID, n) ) ) {
        DB_IdxRemove (idx) ;
        deleteImageFile(idxRec.filename);
    }
    idx++ ;
    }

    pStudyDesc[oldestStudy].NumberofRegistratedImages = 0 ;
    pStudyDesc[oldestStudy].StudySize = 0 ;
    return(oldestStudy) ;
}




/*************************
**   Delete oldest images in database
 */

OFCondition DcmQueryRetrieveIndexDatabaseHandle::deleteOldestImages(StudyDescRecord *pStudyDesc, int StudyNum, char *StudyUID, long RequiredSize)
{

    ImagesofStudyArray *StudyArray ;
    IdxRecord idxRec ;
    int nbimages = 0 , s = 0 , n ;
    long DeletedSize ;

#ifdef DEBUG
    dbdebug(1, "deleteOldestImages RequiredSize = %ld\n",RequiredSize) ;
#endif
    n = strlen(StudyUID) ;
    StudyArray = (ImagesofStudyArray *)malloc(MAX_NUMBER_OF_IMAGES * sizeof(ImagesofStudyArray)) ;

    if (StudyArray == NULL) {
    CERR << "deleteOldestImages: out of memory" << endl;
    return DcmQRIndexDatabaseError;
    }

    /** Find all images having the same StudyUID
     */

    DB_IdxInitLoop (&(handle -> idxCounter)) ;
    while ( DB_IdxGetNext(&(handle -> idxCounter), &idxRec) == EC_Normal ) {
    if ( ! ( strncmp(idxRec. StudyInstanceUID, StudyUID, n) ) ) {

        StudyArray[nbimages]. idxCounter = handle -> idxCounter ;
        StudyArray[nbimages]. RecordedDate = idxRec. RecordedDate ;
        StudyArray[nbimages++]. ImageSize = idxRec. ImageSize ;
    }
    }

    /** Sort the StudyArray in order to have the oldest images first
     */
    qsort((char *)StudyArray, nbimages, sizeof(ImagesofStudyArray), DB_Compare) ;

#ifdef DEBUG
    {
    int i ;
    dbdebug(1, "deleteOldestImages : Sorted images ref array\n") ;
    for (i = 0 ; i < nbimages ; i++)
        dbdebug(1, "[%2d] :   Size %ld   Date %20.3f   Ref %d \n",
            i, StudyArray[i]. ImageSize, StudyArray[i]. RecordedDate, StudyArray[i]. idxCounter) ;
    dbdebug(1, "deleteOldestImages : end of ref array\n") ;
    }
#endif

    s = 0 ;
    DeletedSize = 0 ;

    while ( DeletedSize < RequiredSize ) {

    IdxRecord idxRemoveRec ;
    DB_IdxRead (StudyArray[s]. idxCounter, &idxRemoveRec) ;
#ifdef DEBUG
    dbdebug(1, "Removing file : %s\n", idxRemoveRec. filename) ;
#endif
    deleteImageFile(idxRemoveRec.filename);

    DB_IdxRemove (StudyArray[s]. idxCounter) ;
    pStudyDesc[StudyNum].NumberofRegistratedImages -= 1 ;
    pStudyDesc[StudyNum].StudySize -= StudyArray[s]. ImageSize ;
    DeletedSize += StudyArray[s++]. ImageSize ;
    }

#ifdef DEBUG
    dbdebug(1, "deleteOldestImages DeletedSize = %d\n",
         (int)DeletedSize) ;
#endif
    free(StudyArray) ;
    return( EC_Normal ) ;

}



/*************************
 *   Verify if study UID already exists
 *   If the study UID exists, its index in the study descriptor is returned.
 *   If the study UID does not exist, the index of the first unused descriptor entry is returned.
 *   If no entries are free, maxStudiesAllowed is returned.
 */

int DcmQueryRetrieveIndexDatabaseHandle::matchStudyUIDInStudyDesc (StudyDescRecord *pStudyDesc, char *StudyUID, int maxStudiesAllowed)
{
    int s = 0 ;
    while  (s < maxStudiesAllowed)
    {
      if ((pStudyDesc[s].NumberofRegistratedImages > 0) && (0 == strcmp(pStudyDesc[s].StudyInstanceUID, StudyUID))) break;
      s++ ;
    }
    if (s==maxStudiesAllowed) // study uid does not exist, look for free descriptor
    {
      s=0;
      while  (s < maxStudiesAllowed)
      {
        if (pStudyDesc[s].NumberofRegistratedImages == 0) break;
        s++ ;
      }
    }
    return s;
}


/*************************
**  Check up storage rights in Study Desk record
 */

OFCondition DcmQueryRetrieveIndexDatabaseHandle::checkupinStudyDesc(StudyDescRecord *pStudyDesc, char *StudyUID, long imageSize)
{
    int         s ;
    long        RequiredSize ;

    s = matchStudyUIDInStudyDesc (pStudyDesc, StudyUID,
                     (int)(handle -> maxStudiesAllowed)) ;

    /** If Study already exists
     */

    if ( pStudyDesc[s]. NumberofRegistratedImages != 0 ) {

#ifdef DEBUG
    dbdebug(1, "checkupinStudyDesc: study already exists : %d\n",s) ;
#endif
    if ( ( pStudyDesc[s]. StudySize + imageSize )
         > handle -> maxBytesPerStudy ) {
        if ( imageSize > handle -> maxBytesPerStudy ) {
#ifdef DEBUG
        dbdebug(1,
             "checkupinStudyDesc: imageSize = %ld too large\n",
             imageSize) ;
#endif
        return ( DcmQRIndexDatabaseError ) ;
        }

        RequiredSize = imageSize -
        ( handle -> maxBytesPerStudy - pStudyDesc[s]. StudySize ) ;
        deleteOldestImages(pStudyDesc, s,
                  StudyUID, RequiredSize) ;
    }


    }
    else {
#ifdef DEBUG
    dbdebug(1, "checkupinStudyDesc: study doesn't already exist\n") ;
#endif
    if ( imageSize > handle -> maxBytesPerStudy ) {
#ifdef DEBUG
        dbdebug(1, "checkupinStudyDesc: imageSize = %ld too large\n",
             imageSize) ;
#endif
        return ( DcmQRIndexDatabaseError ) ;
    }
    if ( s > ( handle -> maxStudiesAllowed - 1 ) )
        s = deleteOldestStudy(pStudyDesc) ;

    }

    pStudyDesc[s]. StudySize += imageSize ;
#ifdef DEBUG
    dbdebug(1, "checkupinStudyDesc: ~~~~~~~~ StudySize = %ld\n",
         pStudyDesc[s]. StudySize) ;
#endif

    /* we only have second accuracy */
    pStudyDesc[s]. LastRecordedDate =  (double) time(NULL);

    pStudyDesc[s]. NumberofRegistratedImages++ ;
    strcpy(pStudyDesc[s].StudyInstanceUID,StudyUID) ;

    if ( DB_StudyDescChange (pStudyDesc) == EC_Normal)
    return ( EC_Normal ) ;
    else
    return ( DcmQRIndexDatabaseError ) ;


}

/*
 * If the image is already stored remove it from the database.
 * hewett - Nov. 1, 93
 */
OFCondition DcmQueryRetrieveIndexDatabaseHandle::removeDuplicateImage(
    const char *SOPInstanceUID, const char *StudyInstanceUID,
    StudyDescRecord *pStudyDesc, const char *newImageFileName)
{

    int idx = 0;
    IdxRecord       idxRec ;
    int studyIdx = 0;

    studyIdx = matchStudyUIDInStudyDesc (pStudyDesc, (char*)StudyInstanceUID,
                        (int)(handle -> maxStudiesAllowed)) ;

    if ( pStudyDesc[studyIdx].NumberofRegistratedImages == 0 ) {
    /* no study images, cannot be any old images */
    return EC_Normal;
    }

    while (DB_IdxRead(idx, &idxRec) == EC_Normal) {

    if (strcmp(idxRec.SOPInstanceUID, SOPInstanceUID) == 0) {

#ifdef DEBUG
        dbdebug(1,"--- Removing Existing DB Image Record: %s\n",
           idxRec.filename) ;
#endif
        /* remove the idx record  */
        DB_IdxRemove (idx);
        /* only remove the image file if it is different than that
         * being entered into the database.
         */
        if (strcmp(idxRec.filename, newImageFileName) != 0) {
        deleteImageFile(idxRec.filename);
        }
        /* update the study info */
        pStudyDesc[studyIdx].NumberofRegistratedImages--;
        pStudyDesc[studyIdx].StudySize -= idxRec.ImageSize;
    }
    idx++;
    }
    /* the study record should be written to file later */
    return EC_Normal;
}


/*************************
**  Add data from imageFileName to database
 */

OFCondition DcmQueryRetrieveIndexDatabaseHandle::storeRequest (
    const char  *SOPClassUID,
    const char  * /*SOPInstanceUID*/,
    const char  *imageFileName,
    DcmQueryRetrieveDatabaseStatus   *status,
    OFBool      isNew)
{
    IdxRecord           idxRec ;
    StudyDescRecord     *pStudyDesc ;
    int                 i ;
    struct stat         buf ;


    /**** Initialize an IdxRecord
    ***/

    bzero((char*)&idxRec, sizeof(idxRec));

    DB_IdxInitRecord (&idxRec, 0) ;

    strncpy(idxRec.filename, imageFileName, DBC_MAXSTRING);
#ifdef DEBUG
    dbdebug(1,"DB_storeRequest () : storage request of file : %s\n",idxRec . filename) ;
#endif
    strncpy (idxRec.SOPClassUID, SOPClassUID, UI_MAX_LENGTH);

    /**** Get IdxRec values from ImageFile
    ***/

    DcmFileFormat dcmff;
    if (dcmff.loadFile(imageFileName).bad())
    {
      CERR << "DB: Cannot open file: " << imageFileName << ": "
           << strerror(errno) << endl;
      status->setStatus(STATUS_STORE_Error_CannotUnderstand);
      return (DcmQRIndexDatabaseError) ;
    }

    DcmDataset *dset = dcmff.getDataset();

    for (i = 0 ; i < NBPARAMETERS ; i++ ) {
        OFCondition ec = EC_Normal;
        DB_SmallDcmElmt *se = idxRec.param + i;

        const char *strPtr = NULL;
        ec = dset->findAndGetString(se->XTag, strPtr);
        if ((ec != EC_Normal) || (strPtr == NULL)) {
            /* not found or empty */
            se->PValueField[0] = '\0';
            se->ValueLength = 0;
        } else {
            /* found and non-empty */
            strncpy(se->PValueField, strPtr, (size_t)(se->ValueLength));
            /* important: do not change the ValueLength field before the string is copied! */
            se->ValueLength = strlen(se->PValueField);
        }
    }

    /* InstanceStatus */
    idxRec.hstat = (isNew) ? DVIF_objectIsNew : DVIF_objectIsNotNew;

    /* InstanceDescription */
    OFBool useDescrTag = OFTrue;
    DcmTagKey descrTag = DCM_ImageComments;
    if (SOPClassUID != NULL)
    {
        /* fill in value depending on SOP class UID (content might be improved) */
        if (strcmp(SOPClassUID, UID_GrayscaleSoftcopyPresentationStateStorage) == 0)
        {
            descrTag = DCM_ContentDescription;
        } else if (strcmp(SOPClassUID, UID_HardcopyGrayscaleImageStorage) == 0)
        {
            strcpy(idxRec.InstanceDescription, "Hardcopy Grayscale Image");
            useDescrTag = OFFalse;
        } else if ((strcmp(SOPClassUID, UID_BasicTextSR) == 0) ||
                   (strcmp(SOPClassUID, UID_EnhancedSR) == 0) ||
                   (strcmp(SOPClassUID, UID_ComprehensiveSR) == 0))
        {
            OFString string;
            OFString description = "unknown SR";
            const char *name = dcmFindNameOfUID(SOPClassUID);
            if (name != NULL)
                description = name;
            if (dset->findAndGetOFString(DCM_VerificationFlag, string) == EC_Normal)
            {
                description += ", ";
                description += string;
            }
            if (dset->findAndGetOFString(DCM_CompletionFlag, string) == EC_Normal)
            {
                description += ", ";
                description += string;
            }
            if (dset->findAndGetOFString(DCM_CompletionFlagDescription, string) == EC_Normal)
            {
                description += ", ";
                description += string;
            }
            strncpy(idxRec.InstanceDescription, description.c_str(), DESCRIPTION_MAX_LENGTH);
            useDescrTag = OFFalse;
        } else if (strcmp(SOPClassUID, UID_StoredPrintStorage) == 0)
        {
            strcpy(idxRec.InstanceDescription, "Stored Print");
            useDescrTag = OFFalse;
        }
    }
    /* get description from attribute specified above */
    if (useDescrTag)
    {
        OFString string;
        /* return value is irrelevant */
        dset->findAndGetOFString(descrTag, string);
        strncpy(idxRec.InstanceDescription, string.c_str(), DESCRIPTION_MAX_LENGTH);
    }
    /* is dataset digitally signed? */
    if (strlen(idxRec.InstanceDescription) + 9 < DESCRIPTION_MAX_LENGTH)
    {
        DcmStack stack;
        if (dset->search(DCM_DigitalSignaturesSequence, stack, ESM_fromHere, OFTrue /* searchIntoSub */) == EC_Normal)
        {
            /* in principle it should be checked whether there is _any_ non-empty digital signatures sequence, but ... */
            if (((DcmSequenceOfItems *)stack.top())->card() > 0)
            {
                if (strlen(idxRec.InstanceDescription) > 0)
                    strcat(idxRec.InstanceDescription, " (Signed)");
                else
                    strcpy(idxRec.InstanceDescription, "Signed Instance");
            }
        }
    }

    /**** Print Elements
    ***/

#ifdef DEBUG
    dbdebug(1,"-- BEGIN Parameters to Register in DB\n") ;
    for (i = 0 ; i < NBPARAMETERS ; i++) {  /* new definition */
    DB_SmallDcmElmt *se = idxRec.param + i;
    const char* value = "";
    if (se->PValueField != NULL) value = se->PValueField;
    DcmTag tag(se->XTag);
    dbdebug(1," %s: \"%s\"\n", tag.getTagName(), value);
    }
    dbdebug(1,"-- END Parameters to Register in DB\n") ;
#endif

    /**** Goto the end of IndexFile, and write the record
    ***/

    DB_lock(OFTrue);

    pStudyDesc = (StudyDescRecord *)malloc (SIZEOF_STUDYDESC) ;
    if (pStudyDesc == NULL) {
      CERR << "DB_storeRequest: out of memory" << endl;
      status->setStatus(STATUS_STORE_Refused_OutOfResources);
      DB_unlock();
      return (DcmQRIndexDatabaseError) ;
    }

    bzero((char *)pStudyDesc, SIZEOF_STUDYDESC);
    DB_GetStudyDesc(pStudyDesc) ;

    stat(imageFileName, &buf) ;
    idxRec. ImageSize = (int)(buf. st_size) ;

    /* we only have second accuracy */
    idxRec. RecordedDate =  (double) time(NULL);

    /*
     * If the image is already stored remove it from the database.
     * hewett - Nov. 1, 93
     */

    removeDuplicateImage(idxRec.SOPInstanceUID,
                idxRec.StudyInstanceUID, pStudyDesc,
                imageFileName);


    if ( checkupinStudyDesc(pStudyDesc, idxRec. StudyInstanceUID, idxRec. ImageSize) != EC_Normal ) {
    free (pStudyDesc) ;
    status->setStatus(STATUS_STORE_Refused_OutOfResources);

    DB_unlock();

    return (DcmQRIndexDatabaseError) ;
    }

    free (pStudyDesc) ;

    if (DB_IdxAdd (handle, &i, &idxRec) == EC_Normal)
    {
    status->setStatus(STATUS_Success);
    DB_unlock();
    return (EC_Normal) ;
    }
    else
    {
    status->setStatus(STATUS_STORE_Refused_OutOfResources);
    DB_unlock();
    }
    return DcmQRIndexDatabaseError;
}

/*
** Prune invalid DB records.
*/

OFCondition DcmQueryRetrieveIndexDatabaseHandle::pruneInvalidRecords()
{
    int idx = 0;
    IdxRecord idxRec ;
    StudyDescRecord *pStudyDesc;

    DB_lock(OFTrue);

    pStudyDesc = (StudyDescRecord *)malloc (SIZEOF_STUDYDESC) ;
    if (pStudyDesc == NULL) {
      CERR << "DB_pruneInvalidRecords: out of memory" << endl;
      DB_unlock();
      return (DcmQRIndexDatabaseError) ;
    }

    for (int i = 0 ; i < handle -> maxStudiesAllowed ; i++ )
      pStudyDesc[i]. NumberofRegistratedImages = 0 ;

    DB_GetStudyDesc(pStudyDesc) ;

    while (DB_IdxRead(idx, &idxRec) == EC_Normal)
    {
      if (access(idxRec.filename, R_OK) < 0)
      {
#ifdef DEBUG
        dbdebug(1,"*** Pruning Invalid DB Image Record: %s\n", idxRec.filename);
#endif
        /* update the study info */
        int studyIdx = matchStudyUIDInStudyDesc(pStudyDesc, idxRec.StudyInstanceUID, (int)(handle->maxStudiesAllowed)) ;
        if (studyIdx < handle->maxStudiesAllowed)
        {
          if (pStudyDesc[studyIdx].NumberofRegistratedImages > 0)
          {
            pStudyDesc[studyIdx].NumberofRegistratedImages--;
          } else {
            pStudyDesc[studyIdx].NumberofRegistratedImages = 0;
            pStudyDesc[studyIdx].StudySize = 0;
            pStudyDesc[studyIdx].StudyInstanceUID[0] = '\0';
          }
          if (pStudyDesc[studyIdx].StudySize > idxRec.ImageSize)
          {
            pStudyDesc[studyIdx].StudySize -= idxRec.ImageSize;
          }
        }

        /* remove the idx record  */
        DB_IdxRemove (idx);
      }
      idx++;
    }

    DB_StudyDescChange (pStudyDesc);
    DB_unlock();
    free (pStudyDesc) ;
    return EC_Normal;
}


/* ========================= INDEX ========================= */


/************************
 *      Dump an index file
 */

void DcmQueryRetrieveIndexDatabaseHandle::printIndexFile (char *storeArea)
{
    int i ;
    int j ;
    IdxRecord           idxRec ;
    StudyDescRecord     *pStudyDesc;

    OFCondition result;
    DcmQueryRetrieveIndexDatabaseHandle handle(storeArea, -1, -1, result);
    if (result.bad()) return;

    pStudyDesc = (StudyDescRecord *)malloc (SIZEOF_STUDYDESC) ;
    if (pStudyDesc == NULL) {
        CERR << "printIndexFile: out of memory" << endl;
        return;
    }

    handle.DB_lock(OFFalse);

    handle.DB_GetStudyDesc(pStudyDesc);

    for (i=0; i<handle.handle->maxStudiesAllowed; i++) {
        if (pStudyDesc[i].NumberofRegistratedImages != 0 ) {
            COUT << "******************************************************" << endl
                << "STUDY DESCRIPTOR: " << i << endl
                << "  Study UID: " << pStudyDesc[i].StudyInstanceUID << endl
                << "  StudySize: " << pStudyDesc[i].StudySize << endl
                << "  LastRecDate: " << pStudyDesc[i].LastRecordedDate << endl
                << "  NumOfImages: " << pStudyDesc[i].NumberofRegistratedImages << endl;
        }
    }

    handle.DB_IdxInitLoop (&j) ;
    while (1) {
        if (handle.DB_IdxGetNext(&j, &idxRec) != EC_Normal)
            break ;

        COUT << "*******************************************************" << endl;
        COUT << "RECORD NUMBER: " << j << endl << "  Status: ";
        if (idxRec.hstat == DVIF_objectIsNotNew)
            COUT << "is NOT new" << endl;
        else
            COUT << "is new" << endl;
        COUT << "  Filename: " << idxRec.filename << endl
             << "  ImageSize: " << idxRec.ImageSize << endl
             << "  RecordedDate: " << idxRec.RecordedDate << endl;
        for (i = 0 ; i < NBPARAMETERS ; i++) {  /* new definition */
            DB_SmallDcmElmt *se = idxRec.param + i;
            const char* value = "";
            if (se->PValueField != NULL) value = se->PValueField;
            DcmTag tag(se->XTag);
            COUT << "    " << tag.getTagName() << ": \"" << value << "\"" << endl;
        }
            COUT << "  InstanceDescription: \"" << idxRec.InstanceDescription << "\"" << endl;
    }
    COUT << "*******************************************************" << endl
         << "RECORDS IN THIS INDEXFILE: " << j << endl;

    handle.DB_unlock();

}


/* ========================= UTILS ========================= */


const char *DcmQueryRetrieveIndexDatabaseHandle::getStorageArea() const
{
  return handle->storageArea;
}

const char *DcmQueryRetrieveIndexDatabaseHandle::getIndexFilename() const
{
  return handle->indexFilename;
}

void DcmQueryRetrieveIndexDatabaseHandle::setDebugLevel(int dLevel)
{
    debugLevel = dLevel;
}

int DcmQueryRetrieveIndexDatabaseHandle::getDebugLevel() const
{
    return debugLevel;
}

void DcmQueryRetrieveIndexDatabaseHandle::dbdebug(int level, const char* format, ...) const
{
    va_list ap;
    char buf[4096]; /* we hope a message never gets larger */

    if (level <= debugLevel) {
        CERR << "DB:";
        va_start(ap, format);
        vsprintf(buf, format, ap);
        va_end(ap);
        CERR << buf << endl;
    }
}


void DcmQueryRetrieveIndexDatabaseHandle::setIdentifierChecking(OFBool checkFind, OFBool checkMove)
{
    doCheckFindIdentifier = checkFind;
    doCheckMoveIdentifier = checkMove;
}


/***********************
 *      Creates a handle
 */

DcmQueryRetrieveIndexDatabaseHandle::DcmQueryRetrieveIndexDatabaseHandle(
    const char *storageArea,
    long maxStudiesPerStorageArea,
    long maxBytesPerStudy,
    OFCondition& result)
: handle(NULL)
, quotaSystemEnabled(OFTrue)
, doCheckFindIdentifier(OFFalse)
, doCheckMoveIdentifier(OFFalse)
, fnamecreator()
, debugLevel(0)
{

    handle = (DB_Private_Handle *) malloc ( sizeof(DB_Private_Handle) );

#ifdef DEBUG
    dbdebug(1, "DB_createHandle () : Handle created for %s\n",storageArea);
    dbdebug(1, "                     maxStudiesPerStorageArea: %ld maxBytesPerStudy: %ld\n",
            maxStudiesPerStorageArea, maxBytesPerStudy);
#endif

    if (maxStudiesPerStorageArea > DB_UpperMaxStudies) {
        CERR << "WARING: maxStudiesPerStorageArea too large" << endl
             << "        setting to " << DB_UpperMaxStudies << endl;
        maxStudiesPerStorageArea = DB_UpperMaxStudies;
    }
    if (maxStudiesPerStorageArea < 0) {
        maxStudiesPerStorageArea = DB_UpperMaxStudies;
    }
    if (maxBytesPerStudy < 0 || maxBytesPerStudy > DB_UpperMaxBytesPerStudy) {
        maxBytesPerStudy = DB_UpperMaxBytesPerStudy;
    }

    if (handle) {
        sprintf (handle -> storageArea,"%s", storageArea);
        sprintf (handle -> indexFilename,"%s%c%s", storageArea, PATH_SEPARATOR, DBINDEXFILE);

        /* create index file if it does not already exist */
        FILE* f = fopen(handle->indexFilename, "ab");
        if (f == NULL) {
            CERR << handle->indexFilename << ": " << strerror(errno) << endl;
            result = DcmQRIndexDatabaseError;
            return;
        }
        fclose(f);

        /* open fd of index file */
#ifdef O_BINARY
        handle -> pidx = open(handle -> indexFilename, O_RDWR | O_BINARY );
#else
        handle -> pidx = open(handle -> indexFilename, O_RDWR );
#endif
        if ( handle -> pidx == (-1) )
        {
           result = DcmQRIndexDatabaseError;
           return;
        }
        else {
            handle -> idxCounter = -1;
            handle -> findRequestList = NULL;
            handle -> findResponseList = NULL;
            handle -> maxBytesPerStudy = maxBytesPerStudy;
            handle -> maxStudiesAllowed = maxStudiesPerStorageArea;
            handle -> uidList = NULL;
            result = EC_Normal;
            return;
        }
    }
    else
    {
        result = DcmQRIndexDatabaseError;
        return;
    }
}

/***********************
 *      Destroys a handle
 */

DcmQueryRetrieveIndexDatabaseHandle::~DcmQueryRetrieveIndexDatabaseHandle()
{
    int closeresult;

    if (handle)
    {
#ifndef _WIN32
      /* should not be necessary because we are closing the file handle anyway.
       * On Unix systems this does no harm, but on Windows the unlock fails
       * if the file was not locked before
       * and this gives an unnecessary error message on stderr.
       */
      DB_unlock();
#endif
      closeresult = close( handle -> pidx);

      /* Free lists */
      DB_FreeElementList (handle -> findRequestList);
      DB_FreeElementList (handle -> findResponseList);
      DB_FreeUidList (handle -> uidList);

      free ( (char *)(handle) );
    }
}

/**********************************
 *      Provides a storage filename
 */

OFCondition DcmQueryRetrieveIndexDatabaseHandle::makeNewStoreFileName(
                const char      *SOPClassUID,
                const char      * /* SOPInstanceUID */ ,
                char            *newImageFileName)
{

    OFString filename;
    char prefix[12];

    const char *m = dcmSOPClassUIDToModality(SOPClassUID);
    if (m==NULL) m = "XX";
    sprintf(prefix, "%s_", m);
    // unsigned int seed = fnamecreator.hashString(SOPInstanceUID);
    unsigned int seed = (unsigned int)time(NULL);
    newImageFileName[0]=0; // return empty string in case of error
    if (! fnamecreator.makeFilename(seed, handle->storageArea, prefix, ".dcm", filename)) return DcmQRIndexDatabaseError;

    strcpy(newImageFileName, filename.c_str());
    return EC_Normal;
}


OFCondition DcmQueryRetrieveIndexDatabaseHandle::instanceReviewed(int idx)
{
    // acquire shared lock and read record at index position
    OFCondition result = DB_lock(OFFalse);
    if (result.bad()) return result;
    IdxRecord record;
    result = DB_IdxRead(idx, &record);
    DB_unlock();

    if (result.good() && (record.hstat != DVIF_objectIsNotNew))
    {
      // acquire exclusive lock and update flag
      result = DB_lock(OFTrue);
      if (result.bad()) return result;

      record.hstat = DVIF_objectIsNotNew;
      DB_lseek(handle->pidx, OFstatic_cast(long, SIZEOF_STUDYDESC + idx * SIZEOF_IDXRECORD), SEEK_SET);
      write(handle->pidx, OFreinterpret_cast(char *, &record), SIZEOF_IDXRECORD);
      DB_lseek(handle->pidx, 0L, SEEK_SET);
      DB_unlock();
    }

    return result;
}


/***********************
 *    Default constructors for struct IdxRecord and DB_SSmallDcmElmt
 */

IdxRecord::IdxRecord()
: RecordedDate(0.0)
, ImageSize(0)
, hstat(DVIF_objectIsNotNew)
{
}

DB_SmallDcmElmt::DB_SmallDcmElmt()
: PValueField(NULL)
, ValueLength(0)
, XTag()
{
}


DcmQueryRetrieveIndexDatabaseHandleFactory::DcmQueryRetrieveIndexDatabaseHandleFactory(const DcmQueryRetrieveConfig *config)
: DcmQueryRetrieveDatabaseHandleFactory()
, config_(config)
{
}

DcmQueryRetrieveIndexDatabaseHandleFactory::~DcmQueryRetrieveIndexDatabaseHandleFactory()
{
}

DcmQueryRetrieveDatabaseHandle *DcmQueryRetrieveIndexDatabaseHandleFactory::createDBHandle(
    const char * /* callingAETitle */,
    const char *calledAETitle,
    OFCondition& result) const
{
  return new DcmQueryRetrieveIndexDatabaseHandle(
    config_->getStorageArea(calledAETitle),
    config_->getMaxStudies(calledAETitle),
    config_->getMaxBytesPerStudy(calledAETitle), result);
}


/*
 * CVS Log
 * $Log: dcmqrdbi.cc,v $
 * Revision 1.9  2005/12/16 09:16:08  onken
 * - Added variable initialization to avoid compiler warning
 *
 * Revision 1.8  2005/12/14 14:29:43  joergr
 * Including ctype if present, needed for Solaris.
 *
 * Revision 1.7  2005/12/14 13:46:54  meichel
 * Changed order of include files to avoid warning on FreeBSD
 *
 * Revision 1.6  2005/12/08 15:47:09  meichel
 * Changed include path schema for all DCMTK header files
 *
 * Revision 1.5  2005/12/01 09:10:06  joergr
 * Fixed bug in method matchUID (formerly known as DB_MatchUID).
 *
 * Revision 1.4  2005/04/22 15:36:32  meichel
 * Passing calling aetitle to DcmQueryRetrieveDatabaseHandleFactory::createDBHandle
 *   to allow configuration retrieval based on calling aetitle.
 *
 * Revision 1.3  2005/04/04 14:23:21  meichel
 * Renamed application "dcmqrdb" into "dcmqrscp" to avoid name clash with
 *   dcmqrdb library, which confuses the MSVC build system.
 *
 * Revision 1.2  2005/04/04 10:04:47  meichel
 * Added public declarations for index file functions that are
 *   used from module dcmpstat
 *
 * Revision 1.1  2005/03/30 13:34:53  meichel
 * Initial release of module dcmqrdb that will replace module imagectn.
 *   It provides a clear interface between the Q/R DICOM front-end and the
 *   database back-end. The imagectn code has been re-factored into a minimal
 *   class structure.
 *
 *
 */
