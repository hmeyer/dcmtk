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
 *    classes: DVPSVOIWindow_PList
 *
 *  Last Update:      $Author: meichel $
 *  Update Date:      $Date: 2005/12/08 16:04:14 $
 *  CVS/RCS Revision: $Revision: 1.7 $
 *  Status:           $State: Exp $
 *
 *  CVS/RCS Log at end of file
 *
 */

#ifndef __DVPSVWL_H__
#define __DVPSVWL_H__

#include "dcmtk/config/osconfig.h"    /* make sure OS specific configuration is included first */
#include "dcmtk/ofstd/oflist.h"
#include "dcmtk/dcmdata/dctk.h"

class DVPSVOIWindow;


/** the list of VOI Windows contained in an image attached to a presentation state.
 *  This class manages the data structures comprising the VOI Windows
 *  of one image attached to a presentation state.
 */

class DVPSVOIWindow_PList
{
public:
  /// default constructor
  DVPSVOIWindow_PList();
  
  /// copy constructor
  DVPSVOIWindow_PList(const DVPSVOIWindow_PList& copy);

  /** clone method.
   *  @return a pointer to a new DVPSVOIWindow_PList object containing
   *  a deep copy of this object.
   */
  DVPSVOIWindow_PList *clone() { return new DVPSVOIWindow_PList(*this); }

  /// destructor
  virtual ~DVPSVOIWindow_PList();

  /** reads a list of VOI Windows from a DICOM dataset.
   *  The DICOM elements of the VOI Window are copied
   *  from the dataset to this object.
   *  If this method returns an error code, the object is in undefined state afterwards.
   *  @param dset the dataset from which the VOI Windows are to be read
   *  @return EC_Normal if successful, an error code otherwise.
   */
  OFCondition read(DcmItem &dset);

  /** reset the object to initial state.
   *  After this call, the object is in the same state as after
   *  creation with the default constructor.
   */
  void clear();
  
  /** get number of VOI Windows in this list.
   *  @return the number of VOI Windows.
   */
  size_t size() const { return list_.size(); }  

  /** returns a pointer to the VOI Window with the given
   *  index or NULL if it does not exist.
   *  @param idx index, must be < size()
   *  @return pointer to VOI Window or NULL
   */
  DVPSVOIWindow *getVOIWindow(size_t idx);

  /** sets a new log stream
   *  @param stream new log stream, NULL for default logstream
   *  @param verbMode verbose mode flag
   *  @param dbgMode debug mode flag
   */
  void setLog(OFConsole *stream, OFBool verbMode, OFBool dbgMode);

private:

  /// private undefined assignment operator
  DVPSVOIWindow_PList& operator=(const DVPSVOIWindow_PList&);

  /** the list maintained by this object
   */
  OFList<DVPSVOIWindow *> list_;

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
 *  $Log: dvpsvwl.h,v $
 *  Revision 1.7  2005/12/08 16:04:14  meichel
 *  Changed include path schema for all DCMTK header files
 *
 *  Revision 1.6  2003/06/04 10:18:06  meichel
 *  Replaced private inheritance from template with aggregation
 *
 *  Revision 1.5  2001/09/26 15:36:19  meichel
 *  Adapted dcmpstat to class OFCondition
 *
 *  Revision 1.4  2001/06/01 15:50:25  meichel
 *  Updated copyright header
 *
 *  Revision 1.3  2000/06/02 16:00:55  meichel
 *  Adapted all dcmpstat classes to use OFConsole for log and error output
 *
 *  Revision 1.2  2000/03/08 16:28:59  meichel
 *  Updated copyright header.
 *
 *  Revision 1.1  1998/12/22 17:57:10  meichel
 *  Implemented Presentation State interface for overlays,
 *    VOI LUTs, VOI windows, curves. Added test program that
 *    allows to add curve data to DICOM images.
 *
 *
 */

