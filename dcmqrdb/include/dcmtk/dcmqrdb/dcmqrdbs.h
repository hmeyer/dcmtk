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
 *  Purpose: class DcmQueryRetrieveDatabaseStatus
 *
 *  Last Update:      $Author: meichel $
 *  Update Date:      $Date: 2005/12/08 16:04:23 $
 *  Source File:      $Source: /share/dicom/cvs-depot/dcmtk/dcmqrdb/include/dcmtk/dcmqrdb/dcmqrdbs.h,v $
 *  CVS/RCS Revision: $Revision: 1.2 $
 *  Status:           $State: Exp $
 *
 *  CVS/RCS Log at end of file
 *
 */

#ifndef DCMQRDBS_H
#define DCMQRDBS_H

#include "dcmtk/config/osconfig.h"    /* make sure OS specific configuration is included first */
#include "dcmtk/ofstd/oftypes.h"

class DcmDataset;

/** this class describes the result of a database operation (for an 
 *  incoming C-FIND, C-MOVE, C-GET or C-STORE request) in a format
 *  that can directly be communicated to the remote SCU in a DIMSE RSP message.
 *  An object of this class comprises a status field along with an
 *  optional dataset containing status detail information.
 */
class DcmQueryRetrieveDatabaseStatus
{
public:
  /** default constructor
   *  @param s initial DIMSE status, default is 0 (success).
   */
  DcmQueryRetrieveDatabaseStatus(Uint16 s = 0);

  /// copy constructor
  DcmQueryRetrieveDatabaseStatus(const DcmQueryRetrieveDatabaseStatus& org);

  /// destructor
  ~DcmQueryRetrieveDatabaseStatus();

  /// copy assignment operator
  DcmQueryRetrieveDatabaseStatus& operator=(const DcmQueryRetrieveDatabaseStatus& org);
  
  /// delete status detail, if any
  void deleteStatusDetail();
  
  /** return pointer to status detail object, then set status detail to NULL
   *  @return status detail, may be NULL
   */
  DcmDataset *extractStatusDetail();

  /// return status value
  Uint16 status() const { return status_; }

  /** set new status value
   *  @param s new status
   */
  void setStatus(Uint16 s) { status_ = s; }
    
private:
  /// the status flag as defined for the various DIMSE services
  Uint16 status_;

  /// an optional DICOM dataset with status detail, may be NULL.
  DcmDataset *statusDetail_;
};

#endif

/*
 * CVS Log
 * $Log: dcmqrdbs.h,v $
 * Revision 1.2  2005/12/08 16:04:23  meichel
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
