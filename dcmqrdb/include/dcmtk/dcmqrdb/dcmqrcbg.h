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
 *  Purpose: class DcmQueryRetrieveGetContext
 *
 *  Last Update:      $Author: joergr $
 *  Update Date:      $Date: 2005/12/15 08:32:49 $
 *  Source File:      $Source: /share/dicom/cvs-depot/dcmtk/dcmqrdb/include/dcmtk/dcmqrdb/dcmqrcbg.h,v $
 *  CVS/RCS Revision: $Revision: 1.3 $
 *  Status:           $State: Exp $
 *
 *  CVS/RCS Log at end of file
 *
 */

#ifndef DCMQRCBG_H
#define DCMQRCBG_H

#include "dcmtk/config/osconfig.h"    /* make sure OS specific configuration is included first */
#include "dcmtk/dcmnet/dimse.h"

class DcmQueryRetrieveDatabaseHandle;
class DcmQueryRetrieveOptions;
class DcmQueryRetrieveDatabaseStatus;

/** this class maintains the context information that is passed to the 
 *  callback function called by DIMSE_getProvider.
 */
class DcmQueryRetrieveGetContext
{
public:
    /** constructor
     *  @param handle reference to database handle
     *  @param options options for the Q/R service
     *  @param priorstatus prior DIMSE status
     *  @param origassoc pointer to DIMSE association
     *  @param origmsgid DIMSE message ID
     *  @param prior DIMSE priority
     *  @param origpresid presentation context ID
     */
    DcmQueryRetrieveGetContext(DcmQueryRetrieveDatabaseHandle& handle,
      const DcmQueryRetrieveOptions& options,
      DIC_US priorstatus,
      T_ASC_Association *origassoc,
      DIC_US origmsgid,
      T_DIMSE_Priority prior,
      T_ASC_PresentationContextID origpresid)
    : dbHandle(handle)
    , options_(options)
    , priorStatus(priorstatus)
    , origAssoc(origassoc)
    , assocStarted(OFFalse)
    , origPresId(origpresid)
    , origMsgId(origmsgid)
//    , origAETitle()
//    , origHostName()
    , priority(prior)
//    , ourAETitle()
    , failedUIDs(NULL)
    , nRemaining(0)
    , nCompleted(0)
    , nFailed(0)
    , nWarning(0)
    , getCancelled(OFFalse)
    {
      origAETitle[0] = '\0';
      origHostName[0] = '\0';
      ourAETitle[0] = '\0';
    }

    /// check whether verbose mode is enabled
    OFBool isVerbose() const;

    /** set the AEtitle under which this application operates
     *  @param ae AEtitle, is copied into this object.
     */
    void setOurAETitle(const char *ae)
    {
      if (ae) ourAETitle = ae; else ourAETitle.clear();
    }
    
    /** callback handler called by the DIMSE_storeProvider callback function.
     *  @param cancelled (in) flag indicating whether a C-CANCEL was received
     *  @param request original get request (in) 
     *  @param requestIdentifiers original get request identifiers (in)
     *  @param responseCount get response count (in)
     *  @param response get response (out)
     *  @param stDetail status detail for get response (out)
     *  @param responseIdentifiers get response identifiers (out)
     */
    void callbackHandler(
	/* in */ 
	OFBool cancelled, T_DIMSE_C_GetRQ *request, 
	DcmDataset *requestIdentifiers, int responseCount,
	/* out */
	T_DIMSE_C_GetRSP *response, DcmDataset **stDetail,	
	DcmDataset **responseIdentifiers);

private:

    void addFailedUIDInstance(const char *sopInstance);
    OFCondition performGetSubOp(DIC_UI sopClass, DIC_UI sopInstance, char *fname);
    void getNextImage(DcmQueryRetrieveDatabaseStatus * dbStatus);
    void buildFailedInstanceList(DcmDataset ** rspIds);

    /// reference to database handle
    DcmQueryRetrieveDatabaseHandle& dbHandle;

    /// reference to Q/R service options
    const DcmQueryRetrieveOptions& options_;

    /// prior DIMSE status
    DIC_US	priorStatus;

    /// pointer to association on which the C-GET-RQ was received
    T_ASC_Association   *origAssoc;

    /// true if the association was started 
    OFBool assocStarted;
    
    /// presentation context id of request
    T_ASC_PresentationContextID origPresId;

    /// message id of request
    DIC_US origMsgId;

    /// title of requestor
    OFString origAETitle;

    /// hostname of requestor
    DIC_NODENAME origHostName;


    /// priority of request
    T_DIMSE_Priority priority;

    /// our current title
    OFString ourAETitle; 

    /// instance UIDs of failed store sub-ops
    char *failedUIDs;

    /// number of remaining sub-operations
    DIC_US nRemaining; 

    /// number of completed sub-operations
    DIC_US nCompleted; 

    /// number of failed sub-operations
    DIC_US nFailed; 

    /// number of completed sub-operations that causes warnings
    DIC_US nWarning;

    /// true if the get sub-operations have been cancelled
    OFBool getCancelled;

};

#endif

/*
 * CVS Log
 * $Log: dcmqrcbg.h,v $
 * Revision 1.3  2005/12/15 08:32:49  joergr
 * Fixed issue with initialization of array member variables, reported by egcs
 * on Solaris. Fixed missing/wrong initialization of member variables.
 *
 * Revision 1.2  2005/12/08 16:04:17  meichel
 * Changed include path schema for all DCMTK header files
 *
 * Revision 1.1  2005/03/30 13:34:50  meichel
 * Initial release of module dcmqrdb that will replace module imagectn.
 *   It provides a clear interface between the Q/R DICOM front-end and the
 *   database back-end. The imagectn code has been re-factored into a minimal
 *   class structure.
 *
 *
 */
