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
 *    classes: DVPSCurve_PList
 *
 *  Last Update:      $Author: meichel $
 *  Update Date:      $Date: 2005/12/08 16:03:38 $
 *  CVS/RCS Revision: $Revision: 1.7 $
 *  Status:           $State: Exp $
 *
 *  CVS/RCS Log at end of file
 *
 */

#ifndef __DVPSCUL_H__
#define __DVPSCUL_H__

#include "dcmtk/config/osconfig.h"    /* make sure OS specific configuration is included first */
#include "dcmtk/ofstd/oflist.h"
#include "dcmtk/dcmdata/dctk.h"

class DVPSCurve;

/** the list of curves contained in image which is attached to a presentation state.
 *  This class manages the data structures comprising the list of curves
 *  (all instances of the Curve Module repeating elements)
 *  contained in an image object.
 */

class DVPSCurve_PList
{
public:
  /// default constructor
  DVPSCurve_PList();
  
  /// copy constructor
  DVPSCurve_PList(const DVPSCurve_PList& copy);

  /** clone method.
   *  @return a pointer to a new DVPSCurve_PList object containing
   *  a deep copy of this object.
   */
  DVPSCurve_PList *clone() { return new DVPSCurve_PList(*this); }

  /// destructor
  virtual ~DVPSCurve_PList();

  /** reads all curve groups which can be displayed in a presentation state
   *  from a DICOM dataset.
   *  This method checks which curves are contained in the DICOM dataset.
   *  All curves complying with the presentation state requirements
   *  (i.e. 2-dimensional POLY or ROI curves without curve descriptor) are copied
   *  into the "list of curves" structure managed by this object.
   *  The completeness of the curves (presence of all required elements,
   *  value multiplicity) is checked.
   *  If this method returns an error code, the object is in undefined state afterwards.
   *  @param dset the DICOM dataset from which the curves are read
   *  @return EC_Normal if successful, an error code otherwise.
   */
  OFCondition read(DcmItem &dset);

  /** reset the object to initial state.
   *  After this call, the object is in the same state as after
   *  creation with the default constructor.
   */
  void clear();
  
  /** check presence of curve group
   *  @param group curve repeating group to be checked
   *  @return OFTrue if the specified curve group is present in the
   *     list of curves managed by this object.
   */
  OFBool haveCurveGroup(Uint16 group);

  /** get curve by group
   *  @param group curve repeating group to be checked
   *  @return a pointer to the matching DVPSCurve object if found,
   *    NULL otherwise.
   */
  DVPSCurve *getCurveGroup(Uint16 group);

  /** get curve by index
   *  @param idx index, must be < size()
   *  @return a pointer to the matching DVPSCurve object if it exists,
   *    NULL otherwise.
   */
  DVPSCurve *getCurve(size_t idx);

  /** get number of curves in this list.
   *  @return the number of curves.
   */
  size_t size() const { return list_.size(); }  

  /** sets a new log stream
   *  @param stream new log stream, NULL for default logstream
   *  @param verbMode verbose mode flag
   *  @param dbgMode debug mode flag
   */
  void setLog(OFConsole *stream, OFBool verbMode, OFBool dbgMode);

private:

  /// private undefined assignment operator
  DVPSCurve_PList& operator=(const DVPSCurve_PList&);

  /** the list maintained by this object
   */
  OFList<DVPSCurve *> list_;

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
 *  $Log: dvpscul.h,v $
 *  Revision 1.7  2005/12/08 16:03:38  meichel
 *  Changed include path schema for all DCMTK header files
 *
 *  Revision 1.6  2003/06/04 10:18:06  meichel
 *  Replaced private inheritance from template with aggregation
 *
 *  Revision 1.5  2001/09/26 15:36:09  meichel
 *  Adapted dcmpstat to class OFCondition
 *
 *  Revision 1.4  2001/06/01 15:50:14  meichel
 *  Updated copyright header
 *
 *  Revision 1.3  2000/06/02 16:00:44  meichel
 *  Adapted all dcmpstat classes to use OFConsole for log and error output
 *
 *  Revision 1.2  2000/03/08 16:28:50  meichel
 *  Updated copyright header.
 *
 *  Revision 1.1  1998/12/22 17:57:05  meichel
 *  Implemented Presentation State interface for overlays,
 *    VOI LUTs, VOI windows, curves. Added test program that
 *    allows to add curve data to DICOM images.
 *
 *
 */

