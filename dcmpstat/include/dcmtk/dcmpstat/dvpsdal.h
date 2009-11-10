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
 *    classes: DVPSDisplayedArea_PList
 *
 *  Last Update:      $Author: meichel $
 *  Update Date:      $Date: 2005/12/08 16:03:40 $
 *  CVS/RCS Revision: $Revision: 1.8 $
 *  Status:           $State: Exp $
 *
 *  CVS/RCS Log at end of file
 *
 */

#ifndef __DVPSDAL_H__
#define __DVPSDAL_H__

#include "dcmtk/config/osconfig.h"    /* make sure OS specific configuration is included first */
#include "dcmtk/ofstd/oflist.h"
#include "dcmtk/dcmdata/dctk.h"
#include "dcmtk/dcmpstat/dvpstyp.h"     /* for enum types */

class DVPSDisplayedArea;
class DVPSReferencedSeries_PList;

/** the list of displayed area selections contained in a presentation state (internal use only).
 *  This class manages the data structures comprising one complete displayed area selection sequence
 *  contained in a presentation state object.
 */

class DVPSDisplayedArea_PList
{
public:
  /// default constructor
  DVPSDisplayedArea_PList();
  
  /// copy constructor
  DVPSDisplayedArea_PList(const DVPSDisplayedArea_PList& copy);

  /** clone method.
   *  @return a pointer to a new DVPSDisplayedArea_PList object containing
   *  a deep copy of this object.
   */
  DVPSDisplayedArea_PList *clone() { return new DVPSDisplayedArea_PList(*this); }

  /// destructor
  virtual ~DVPSDisplayedArea_PList();

  /** reads a list of displayed area selections (DisplayedAreaSelectionSequence) from a DICOM dataset.
   *  The DICOM elements of the displayed area selection item are copied from the dataset to this object.
   *  The completeness of all items (presence of all required elements,
   *  value multiplicity) is checked.
   *  If this method returns an error code, the object is in undefined state afterwards.
   *  @param dset the DICOM dataset from which the sequence is to be read
   *  @return EC_Normal if successful, an error code otherwise.
   */
  OFCondition read(DcmItem &dset);
  
  /** writes the list of displayed area selections managed by this object to a DICOM dataset.
   *  Copies of the DICOM elements managed by this object are inserted into
   *  the DICOM dataset.
   *  @param dset the DICOM dataset to which the DisplayedAreaSelectionSequence is written
   *  @return EC_Normal if successful, an error code otherwise.
   */
  OFCondition write(DcmItem &dset);

  /** reset the object to initial state.
   *  After this call, the object is in the same state as after
   *  creation with the default constructor.
   */
  void clear();
      
  /** gets the number of displayed area selections in this list.
   *  @return the number of displayed area selections.
   */
  size_t size() const { return list_.size(); }

  /** checks if an displayed area selection exists for the given image and frame.
   *  @param instanceUID SOP instance UID of the current image
   *  @param frame number of the current frame
   *  @return pointer to the displayed area if it exists, NULL otherwise.
   */
  DVPSDisplayedArea *findDisplayedArea(const char *instanceUID, unsigned long frame);

  /** finds or creates a displayed area selection SQ item
   *  with an applicability controlled by the applicability, instanceUID and frame
   *  parameters. The displayed area selection sequence is rearranged such that
   *  all other referenced images/frames keep their old displayed area settings.
   *  @param allReferences list of series/instance references registered for the
   *    presentation state.
   *  @param sopclassUID SOP class UID of the current image
   *  @param instanceUID SOP instance UID of the current image
   *  @param frame number of the current frame
   *  @param numberOfFrames number of frames of the current image
   *  @param applicability applicability of the new displayed area selection
   *  @return pointer to a displayed area selection object from the list
   *    that matches the applicability parameters. NULL is returned if
   *    out of memory.
   */
  DVPSDisplayedArea *createDisplayedArea(
    DVPSReferencedSeries_PList& allReferences,
    const char *sopclassUID, 
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

  /** adjusts all displayed area coordinates for the rotation and flipping
   *  status of the image.
   *  @param rotationFrom previous rotation
   *  @param isFlippedFrom previous flip status
   *  @param rotationTo new rotation
   *  @param isFlippedTo new flip status
   */
  void rotateAndFlip(
    DVPSRotationType rotationFrom, 
    OFBool isFlippedFrom,
    DVPSRotationType rotationTo, 
    OFBool isFlippedTo);

private:

  /** private undefined assignment operator
   */
  DVPSDisplayedArea_PList& operator=(const DVPSDisplayedArea_PList&);

  /** the list maintained by this object
   */
  OFList<DVPSDisplayedArea *> list_;

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
 *  $Log: dvpsdal.h,v $
 *  Revision 1.8  2005/12/08 16:03:40  meichel
 *  Changed include path schema for all DCMTK header files
 *
 *  Revision 1.7  2003/09/05 14:30:06  meichel
 *  Introduced new API methods that allow Displayed Areas to be queried
 *    and set either relative to the image (ignoring rotation and flip) or
 *    in absolute values as defined in the standard.  Rotate and flip methods
 *    now adjust displayed areas in the presentation state.
 *
 *  Revision 1.6  2003/06/04 10:18:06  meichel
 *  Replaced private inheritance from template with aggregation
 *
 *  Revision 1.5  2001/09/26 15:36:10  meichel
 *  Adapted dcmpstat to class OFCondition
 *
 *  Revision 1.4  2001/06/01 15:50:14  meichel
 *  Updated copyright header
 *
 *  Revision 1.3  2000/06/02 16:00:45  meichel
 *  Adapted all dcmpstat classes to use OFConsole for log and error output
 *
 *  Revision 1.2  2000/03/08 16:28:51  meichel
 *  Updated copyright header.
 *
 *  Revision 1.1  1999/07/22 16:39:06  meichel
 *  Adapted dcmpstat data structures and API to supplement 33 letter ballot text.
 *
 *
 */

