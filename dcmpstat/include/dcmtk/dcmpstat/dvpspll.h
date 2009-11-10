/*
 *
 *  Copyright (C) 1998-2005, OFFIS
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
 *  Module: dcmpstat
 *
 *  Author: Marco Eichelberg
 *
 *  Purpose:
 *    classes: DVPSPresentationLUT_PList
 *
 *  Last Update:      $Author: meichel $
 *  Update Date:      $Date: 2005/12/08 16:03:56 $
 *  CVS/RCS Revision: $Revision: 1.9 $
 *  Status:           $State: Exp $
 *
 *  CVS/RCS Log at end of file
 *
 */

#ifndef __DVPSPLL_H__
#define __DVPSPLL_H__

#include "dcmtk/config/osconfig.h"    /* make sure OS specific configuration is included first */
#include "dcmtk/ofstd/oflist.h"
#include "dcmtk/dcmdata/dctk.h"
#include "dcmtk/dcmpstat/dvpstyp.h"     /* for enum types */
#include "dcmtk/dcmnet/dimse.h"

class DVPSPresentationLUT;
class DVPSImageBoxContent_PList;

/** the list of presentation LUTs contained in a stored print object.
 *  This class manages the data structures comprising one complete
 *  Presentation LUT Content Sequence.
 */

class DVPSPresentationLUT_PList
{
public:
  /// default constructor
  DVPSPresentationLUT_PList();
  
  /// copy constructor
  DVPSPresentationLUT_PList(const DVPSPresentationLUT_PList& copy);

  /** clone method.
   *  @return a pointer to a new DVPSPresentationLUT_PList object containing
   *  a deep copy of this object.
   */
  DVPSPresentationLUT_PList *clone() { return new DVPSPresentationLUT_PList(*this); }

  /// destructor
  virtual ~DVPSPresentationLUT_PList();

  /** reads a list of Presentation LUTs (Presentation LUT Content Sequence) from a DICOM dataset.
   *  The DICOM elements of the image references item are copied from the dataset to this object.
   *  The completeness of all items (presence of all required elements,
   *  value multiplicity) is checked.
   *  If this method returns an error code, the object is in undefined state afterwards.
   *  @param dset the DICOM dataset from which the sequence is to be read
   *  @return EC_Normal if successful, an error code otherwise.
   */
  OFCondition read(DcmItem &dset);
  
  /** writes the list of Presentation LUTs managed by this object to a DICOM dataset.
   *  Copies of the DICOM element managed by this object are inserted into
   *  the DICOM dataset.
   *  @param dset the DICOM dataset to which the ReferencedImageSequence is written
   *  @return EC_Normal if successful, an error code otherwise.
   */
  OFCondition write(DcmItem &dset);

  /** reset the object to initial state.
   *  After this call, the object is in the same state as after
   *  creation with the default constructor.
   */
  void clear();
    
  /** gets the number of Presentation LUTs in this list.
   *  @return the number of Presentation LUTs.
   */
  size_t size() const { return list_.size(); }

  /** sets a new log stream
   *  @param stream new log stream, NULL for default logstream
   *  @param verbMode verbose mode flag
   *  @param dbgMode debug mode flag
   */
  void setLog(OFConsole *stream, OFBool verbMode, OFBool dbgMode);

  /** finds a presentation LUT by its SOP instance UID.
   *  @param instanceUID SOP instance UID
   *  @return pointer to matching presentation LUT if found, NULL otherwise.
   */
  DVPSPresentationLUT *findPresentationLUT(const char *instanceUID);

  /** removes all presentation LUT entries that are not
   *  referenced from the film box or image box level.
   *  @param filmBox Presentation LUT UID reference on film box level, may be NULL.
   *  @param imageBoxes list of image boxes
   */
  void cleanup(const char *filmBox, DVPSImageBoxContent_PList& imageBoxes);

  /** adds a Presentation LUT to the list of managed LUTs.
   *  The referenced LUT is copied. If an identical LUT already exists,
   *  no duplicate is created.
   *  @param newLUT pointer to new Presentation LUT. May be NULL.
   *  @param inversePLUT true if presentation LUT is for Monochrome1 and must be inversed.
   *  @return UID of referenced Presentation LUT. May be NULL (if input was NULL).
   */
  const char *addPresentationLUT(DVPSPresentationLUT *newLUT, OFBool inversePLUT);

  /** adds a Presentation LUT to the list of managed LUT. The LUT object becomes
   *  owned by this object and is destroyed upon destruction of the list.
   *  @param newLUT LUT to be added.
   */
  void insert(DVPSPresentationLUT *newLUT) { if (newLUT) list_.push_back(newLUT); }

  /** performs a Print SCP Presentation LUT N-DELETE operation.
   *  The results of the N-DELETE operation are stored in the object passed as rsp.
   *  @param rq N-DELETE request message
   *  @param rsp N-DELETE response message
   */
  void printSCPDelete(T_DIMSE_Message& rq, T_DIMSE_Message& rsp);

private:

  /** private undefined assignment operator
   */
  DVPSPresentationLUT_PList& operator=(const DVPSPresentationLUT_PList&);

  /** the list maintained by this object
   */
  OFList<DVPSPresentationLUT *> list_;
  
  /** output stream for error messages, never NULL
   */
  OFConsole *logstream;

  /** flag indicating whether we're operating in verbose mode
   */
  OFBool verboseMode;
   
  /** flag indicating whether we're operating in debug mode
   */
  OFBool debugMode;
    
};


#endif

/*
 *  $Log: dvpspll.h,v $
 *  Revision 1.9  2005/12/08 16:03:56  meichel
 *  Changed include path schema for all DCMTK header files
 *
 *  Revision 1.8  2003/06/04 10:18:06  meichel
 *  Replaced private inheritance from template with aggregation
 *
 *  Revision 1.7  2001/09/26 15:36:14  meichel
 *  Adapted dcmpstat to class OFCondition
 *
 *  Revision 1.6  2001/06/01 15:50:19  meichel
 *  Updated copyright header
 *
 *  Revision 1.5  2000/06/02 16:00:49  meichel
 *  Adapted all dcmpstat classes to use OFConsole for log and error output
 *
 *  Revision 1.4  2000/05/31 12:56:39  meichel
 *  Added initial Print SCP support
 *
 *  Revision 1.3  2000/03/08 16:28:54  meichel
 *  Updated copyright header.
 *
 *  Revision 1.2  2000/02/29 12:16:15  meichel
 *  Fixed bug in dcmpstat library that caused Monochrome1 images
 *    to be printed inverse if a Presentation LUT was applied.
 *
 *  Revision 1.1  1999/10/07 17:21:49  meichel
 *  Reworked management of Presentation LUTs in order to create tighter
 *    coupling between Softcopy and Print.
 *
 *
 */

