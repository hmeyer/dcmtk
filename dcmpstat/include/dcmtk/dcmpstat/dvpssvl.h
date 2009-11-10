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
 *    classes: DVPSSoftcopyVOI_PList
 *
 *  Last Update:      $Author: meichel $
 *  Update Date:      $Date: 2005/12/08 16:04:06 $
 *  CVS/RCS Revision: $Revision: 1.8 $
 *  Status:           $State: Exp $
 *
 *  CVS/RCS Log at end of file
 *
 */

#ifndef __DVPSSVL_H__
#define __DVPSSVL_H__

#include "dcmtk/config/osconfig.h"    /* make sure OS specific configuration is included first */
#include "dcmtk/ofstd/oflist.h"
#include "dcmtk/dcmdata/dctk.h"
#include "dcmtk/dcmpstat/dvpstyp.h"     /* for enum types */

class DVPSSoftcopyVOI;
class DVPSReferencedSeries_PList;

/** the list of softcopy VOI LUT items contained in a presentation state (internal use only).
 *  This class manages the data structures comprising one complete Softcopy VOI LUT sequence
 *  contained in a presentation state object.
 */

class DVPSSoftcopyVOI_PList
{
public:
  /// default constructor
  DVPSSoftcopyVOI_PList();
  
  /// copy constructor
  DVPSSoftcopyVOI_PList(const DVPSSoftcopyVOI_PList& copy);

  /** clone method.
   *  @return a pointer to a new DVPSSoftcopyVOI_PList object containing
   *  a deep copy of this object.
   */
  DVPSSoftcopyVOI_PList *clone() { return new DVPSSoftcopyVOI_PList(*this); }

  /// destructor
  virtual ~DVPSSoftcopyVOI_PList();

  /** reads a list of softcopy VOI LUTs (SoftcopyVOILUTSequence) from a DICOM dataset.
   *  The DICOM elements of the softcopy VOI LUT item are copied from the dataset to this object.
   *  The completeness of all items (presence of all required elements,
   *  value multiplicity) is checked.
   *  If this method returns an error code, the object is in undefined state afterwards.
   *  @param dset the DICOM dataset from which the sequence is to be read
   *  @return EC_Normal if successful, an error code otherwise.
   */
  OFCondition read(DcmItem &dset);
  
  /** writes the list of softcopy VOI LUTs managed by this object to a DICOM dataset.
   *  Copies of the DICOM elements managed by this object are inserted into
   *  the DICOM dataset.
   *  @param dset the DICOM dataset to which the SoftcopyVOILUTSequence is written
   *  @return EC_Normal if successful, an error code otherwise.
   */
  OFCondition write(DcmItem &dset);

  /** reset the object to initial state.
   *  After this call, the object is in the same state as after
   *  creation with the default constructor.
   */
  void clear();
      
  /** gets the number of softcopy VOI LUTs in this list.
   *  @return the number of softcopy VOI LUTs.
   */
  size_t size() const { return list_.size(); }

  /** creates a default softcopy VOI LUT sequence for a presentation state from a DICOM image.
   *  If this method returns an error code, the object is in undefined state afterwards.
   *  @param dset the DICOM dataset containing the image IOD
   *  @param allReferences list of series/instance references registered for the
   *    presentation state.
   *  @param sopclassUID SOP class UID of the current image
   *  @param instanceUID SOP instance UID of the current image
   *  @param voiActivation flag defining how VOI LUTs or VOI window width/center should
   *    be handled. Default: Use VOI and prefer VOI LUT from VOI window.
   *  @return EC_Normal upon success, an error code otherwise.
   */
  OFCondition createFromImage(
    DcmItem &dset, 
    DVPSReferencedSeries_PList& allReferences,
    const char *sopclassUID, 
    const char *instanceUID, 
    DVPSVOIActivation voiActivation);

  /** checks if a softcopy VOI LUT item exists for the given image and frame.
   *  @param instanceUID SOP instance UID of the current image
   *  @param frame number of the current frame
   *  @return pointer to the softcopy VOI LUT item if it exists, NULL otherwise.
   */
  DVPSSoftcopyVOI *findSoftcopyVOI(const char *instanceUID, unsigned long frame);

  /** finds or creates a softcopy VOI LUT SQ item
   *  with an applicability controlled by the applicability, instanceUID and frame
   *  parameters. The softcopy VOI LUT sequence is rearranged such that
   *  all other referenced images/frames keep their old settings.
   *  @param allReferences list of series/instance references registered for the
   *    presentation state.
   *  @param sopclassUID SOP class UID of the current image
   *  @param instanceUID SOP instance UID of the current image
   *  @param frame number of the current frame
   *  @param numberOfFrames number of frames of the current image
   *  @param applicability applicability of the new softcopy VOI LUT
   *  @return pointer to a softcopy VOI LUT object from the list
   *    that matches the applicability parameters. NULL is returned if
   *    out of memory.
   */
  DVPSSoftcopyVOI *createSoftcopyVOI(
    DVPSReferencedSeries_PList& allReferences,
    const char *sopclassUID, 
    const char *instanceUID, 
    unsigned long frame, 
    unsigned long numberOfFrames, 
    DVPSObjectApplicability applicability);

  /** removes the softcopy VOI for a set of references
   *  controlled by the applicability, instanceUID and frame
   *  parameters. The softcopy VOI LUT sequence is rearranged such that
   *  all other referenced images/frames keep their old settings.
   *  @param allReferences list of series/instance references registered for the
   *    presentation state.
   *  @param instanceUID SOP instance UID of the current image
   *  @param frame number of the current frame
   *  @param numberOfFrames number of frames of the current image
   *  @param applicability applicability of the VOI LUT removal
   */
  void removeSoftcopyVOI(
    DVPSReferencedSeries_PList& allReferences,
    const char *instanceUID, 
    unsigned long frame, 
    unsigned long numberOfFrames, 
    DVPSObjectApplicability applicability);

  /** sets a new log stream
   *  @param stream new log stream, NULL for default logstream
   *  @param verbMode verbose mode flag
   *  @param dbgMode debug mode flag
   */
  void setLog(OFConsole *stream, OFBool verbMode, OFBool dbgMode);

private:

  /// private undefined assignment operator
  DVPSSoftcopyVOI_PList& operator=(const DVPSSoftcopyVOI_PList&);

  /** the list maintained by this object
   */
  OFList<DVPSSoftcopyVOI *> list_;

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
 *  $Log: dvpssvl.h,v $
 *  Revision 1.8  2005/12/08 16:04:06  meichel
 *  Changed include path schema for all DCMTK header files
 *
 *  Revision 1.7  2003/06/04 10:18:06  meichel
 *  Replaced private inheritance from template with aggregation
 *
 *  Revision 1.6  2001/09/26 15:36:16  meichel
 *  Adapted dcmpstat to class OFCondition
 *
 *  Revision 1.5  2001/06/01 15:50:23  meichel
 *  Updated copyright header
 *
 *  Revision 1.4  2000/06/02 16:00:52  meichel
 *  Adapted all dcmpstat classes to use OFConsole for log and error output
 *
 *  Revision 1.3  2000/03/08 16:28:57  meichel
 *  Updated copyright header.
 *
 *  Revision 1.2  1999/07/30 13:34:51  meichel
 *  Added new classes managing Stored Print objects
 *
 *  Revision 1.1  1999/07/22 16:39:12  meichel
 *  Adapted dcmpstat data structures and API to supplement 33 letter ballot text.
 *
 *
 */

