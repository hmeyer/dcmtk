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
 *    classes: DVPSOverlay_PList
 *
 *  Last Update:      $Author: meichel $
 *  Update Date:      $Date: 2005/12/08 16:03:54 $
 *  CVS/RCS Revision: $Revision: 1.10 $
 *  Status:           $State: Exp $
 *
 *  CVS/RCS Log at end of file
 *
 */

#ifndef __DVPSOVL_H__
#define __DVPSOVL_H__

#include "dcmtk/config/osconfig.h"    /* make sure OS specific configuration is included first */
#include "dcmtk/ofstd/oflist.h"
#include "dcmtk/dcmdata/dctk.h"

class DVPSOverlay;

/** the list of overlays contained in a presentation state (internal use only).
 *  This class manages the data structures comprising the list of overlays
 *  (all instances of the Overlay Plane Module repeating elements)
 *  contained in a Presentation State object.
 */

class DVPSOverlay_PList
{
public:
  /// default constructor
  DVPSOverlay_PList();
  
  /// copy constructor
  DVPSOverlay_PList(const DVPSOverlay_PList& copy);

  /** clone method.
   *  @return a pointer to a new DVPSOverlay_PList object containing
   *  a deep copy of this object.
   */
  DVPSOverlay_PList *clone() { return new DVPSOverlay_PList(*this); }

  /// destructor
  virtual ~DVPSOverlay_PList();

  /** reads all overlay groups from a DICOM dataset.
   *  This method checks which overlays are contained in the DICOM dataset.
   *  All overlays that contain the OverlayData element are copied
   *  into the "list of overlays" structure managed by this object.
   *  The completeness of the overlays (presence of all required elements,
   *  value multiplicity) is checked.
   *  If this method returns an error code, the object is in undefined state afterwards.
   *  @param dset the DICOM dataset from which the overlays are read
   *  @return EC_Normal if successful, an error code otherwise.
   */
  OFCondition read(DcmItem &dset);
  
  /** writes the overlays managed by this object to a DICOM dataset.
   *  Copies of the DICOM elements managed by this object are inserted into
   *  the DICOM dataset.
   *  @param dset the DICOM dataset to which the overlays are written
   *  @return EC_Normal if successful, an error code otherwise.
   */
  OFCondition write(DcmItem &dset);

  /** reset the object to initial state.
   *  After this call, the object is in the same state as after
   *  creation with the default constructor.
   */
  void clear();
  
  /** check presence of overlay group
   *  @param group overlay repeating group to be checked
   *  @return OFTrue if the specified overlay group is present in the
   *     list of overlays managed by this object.
   */
  OFBool haveOverlayGroup(Uint16 group);
  
  /** gets the number of overlays in managed by this object.
   *  @return number of overlays in this list.
   */
  size_t size() const { return list_.size(); }
  
  /** gets the overlay object with the given index.
   *  @param idx index of the overlay, must be < size().
   *  @return pointer to overlay object or NULL.
   */  
  DVPSOverlay *getOverlay(size_t idx);

  /** removes the overlay object with the given index.
   *  @param idx index of the overlay, must be < size().
   *  @return EC_Normal upon success, an error code otherwise
   */ 
  OFCondition removeOverlay(size_t idx);

  /** changes the repeating group used for an overlay.
   *  @param idx index of the overlay, must be < size().
   *  @param newGroup new repeating group number 0x6000-0x601F (even)
   *  @return EC_Normal upon success, an error code otherwise.
   */
  OFCondition changeOverlayGroup(size_t idx, Uint16 newGroup);

  /** adds a new overlay bitmap.
   *  The overlay is read from a DICOM dataset which must contain the 
   *  attributes required for a graphic or ROI overlay, see class DVPSOverlay.
   *  The dataset can be an image or standalone overlay IOD.
   *  The overlay data is copied into the presentation state, i.e. the DICOM dataset
   *  can be deleted after execution of this method.
   *  @param overlayIOD the DICOM dataset from which the overlay is to be read
   *  @groupInItem the repeating group 0x6000..0x61F (even) of the overlay to be read
   *  @param newGroup repeating group number 0x6000-0x601F (even) to be used for
   *    the overlay in the presentation state.
   *  @return EC_Normal upon success, an error code otherwise.
   */
  OFCondition addOverlay(DcmItem& overlayIOD, Uint16 groupInItem, Uint16 newGroup);

  /** get overlay by group
   *  @param group overlay repeating group to be checked
   *  @return a pointer to the matching DVPSOverlay object if found,
   *    NULL otherwise.
   */
  DVPSOverlay *getOverlayGroup(Uint16 group);

  /** sets a new log stream
   *  @param stream new log stream, NULL for default logstream
   *  @param verbMode verbose mode flag
   *  @param dbgMode debug mode flag
   */
  void setLog(OFConsole *stream, OFBool verbMode, OFBool dbgMode);

private:

  /// private undefined assignment operator
  DVPSOverlay_PList& operator=(const DVPSOverlay_PList&);

  /** the list maintained by this object
   */
  OFList<DVPSOverlay *> list_;

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
 *  $Log: dvpsovl.h,v $
 *  Revision 1.10  2005/12/08 16:03:54  meichel
 *  Changed include path schema for all DCMTK header files
 *
 *  Revision 1.9  2003/06/04 10:18:06  meichel
 *  Replaced private inheritance from template with aggregation
 *
 *  Revision 1.8  2001/09/26 15:36:13  meichel
 *  Adapted dcmpstat to class OFCondition
 *
 *  Revision 1.7  2001/06/01 15:50:19  meichel
 *  Updated copyright header
 *
 *  Revision 1.6  2000/06/02 16:00:49  meichel
 *  Adapted all dcmpstat classes to use OFConsole for log and error output
 *
 *  Revision 1.5  2000/03/08 16:28:54  meichel
 *  Updated copyright header.
 *
 *  Revision 1.4  2000/03/06 18:23:15  joergr
 *  Added const type specifier to derived method (reported by Sun CC 4.2).
 *
 *  Revision 1.3  1998/12/22 17:57:06  meichel
 *  Implemented Presentation State interface for overlays,
 *    VOI LUTs, VOI windows, curves. Added test program that
 *    allows to add curve data to DICOM images.
 *
 *  Revision 1.2  1998/12/14 16:10:31  meichel
 *  Implemented Presentation State interface for graphic layers,
 *    text and graphic annotations, presentation LUTs.
 *
 *  Revision 1.1  1998/11/27 14:50:30  meichel
 *  Initial Release.
 *
 *
 */

