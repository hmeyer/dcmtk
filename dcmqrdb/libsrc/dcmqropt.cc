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
 *  Update Date:      $Date: 2005/12/08 15:47:11 $
 *  Source File:      $Source: /share/dicom/cvs-depot/dcmtk/dcmqrdb/libsrc/dcmqropt.cc,v $
 *  CVS/RCS Revision: $Revision: 1.5 $
 *  Status:           $State: Exp $
 *
 *  CVS/RCS Log at end of file
 *
 */

#include "dcmtk/config/osconfig.h"    /* make sure OS specific configuration is included first */
#include "dcmtk/dcmqrdb/dcmqropt.h"

#define INCLUDE_CSTDARG
#include "dcmtk/ofstd/ofstdinc.h"

const OFConditionConst APPE_INVALIDPEER(1024, 1, OF_error, "dcmqrdb: invalid peer for move operation");
const OFCondition APP_INVALIDPEER(APPE_INVALIDPEER);

DcmQueryRetrieveOptions::DcmQueryRetrieveOptions()
: allowShutdown_(OFFalse)
, bitPreserving_(OFFalse)
, correctUIDPadding_(OFFalse)
, debug_(OFFalse)
, disableGetSupport_(OFFalse)
, filepad_(0)
, groupLength_(EGL_recalcGL)
, ignoreStoreData_(OFFalse)
, itempad_(0) 
, maxAssociations_(20)
, maxPDU_(ASC_DEFAULTMAXPDU)
, net_(NULL)
, networkTransferSyntax_(EXS_Unknown)
#ifndef DISABLE_COMPRESSION_EXTENSION
,  networkTransferSyntaxOut_(EXS_Unknown)
#endif
, paddingType_(EPD_withoutPadding)
, refuseMultipleStorageAssociations_(OFFalse)
, refuse_(OFFalse)
, rejectWhenNoImplementationClassUID_(OFFalse)
, requireFindForMove_(OFFalse)
, restrictMoveToSameAE_(OFFalse)
, restrictMoveToSameHost_(OFFalse)
, restrictMoveToSameVendor_(OFFalse)
, sequenceType_(EET_ExplicitLength)
#ifdef HAVE_FORK
, singleProcess_(OFFalse)
#else
, singleProcess_(OFTrue)
#endif
, supportPatientRoot_(OFTrue)
#ifdef NO_PATIENTSTUDYONLY_SUPPORT
, supportPatientStudyOnly_(OFFalse)
#else
, supportPatientStudyOnly_(OFTrue)
#endif
, supportStudyRoot_(OFTrue)
, useMetaheader_(OFTrue)
, keepDBHandleDuringAssociation_(OFTrue)
, verbose_(0)
, writeTransferSyntax_(EXS_Unknown)
, blockMode_(DIMSE_BLOCKING)
, dimse_timeout_(0)
, acse_timeout_(30)
{
}

DcmQueryRetrieveOptions::~DcmQueryRetrieveOptions()
{
}

void DcmQueryRetrieveOptions::errmsg(const char* msg, ...)
{
    va_list args;

    fprintf(stderr, "DcmQueryRetrieve: ");
    va_start(args, msg);
    vfprintf(stderr, msg, args);
    va_end(args);
    fprintf(stderr, "\n");
}

/*
 * CVS Log
 * $Log: dcmqropt.cc,v $
 * Revision 1.5  2005/12/08 15:47:11  meichel
 * Changed include path schema for all DCMTK header files
 *
 * Revision 1.4  2005/11/29 11:27:20  meichel
 * Added new flag keepDBHandleDuringAssociation_ which allows to determine
 *   whether a DB handle is kept open for a complete association or a single
 *   DIMSE message only. Also improved error handling of file locking.
 *
 * Revision 1.3  2005/11/29 10:54:52  meichel
 * Added minimal support for compressed transfer syntaxes to dcmqrscp.
 *   No on-the-fly decompression is performed, but compressed images can
 *   be stored and retrieved.
 *
 * Revision 1.2  2005/11/17 13:44:40  meichel
 * Added command line options for DIMSE and ACSE timeouts
 *
 * Revision 1.1  2005/03/30 13:34:53  meichel
 * Initial release of module dcmqrdb that will replace module imagectn.
 *   It provides a clear interface between the Q/R DICOM front-end and the
 *   database back-end. The imagectn code has been re-factored into a minimal
 *   class structure.
 *
 *
 */
