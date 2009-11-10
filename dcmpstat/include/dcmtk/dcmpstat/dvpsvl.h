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
 *    classes: DVPSVOILUT
 *
 *  Last Update:      $Author: meichel $
 *  Update Date:      $Date: 2005/12/08 16:04:11 $
 *  CVS/RCS Revision: $Revision: 1.7 $
 *  Status:           $State: Exp $
 *
 *  CVS/RCS Log at end of file
 *
 */

#ifndef __DVPSVL_H__
#define __DVPSVL_H__

#include "dcmtk/config/osconfig.h"    /* make sure OS specific configuration is included first */
#include "dcmtk/dcmpstat/dvpstyp.h"
#include "dcmtk/ofstd/ofstring.h"
#include "dcmtk/dcmdata/dctk.h"

class DVPSSoftcopyVOI;

/** the representation of one VOI LUT in a DICOM image.
 */  

class DVPSVOILUT
{
public:
  /// default constructor
  DVPSVOILUT();
  
  /// copy constructor
  DVPSVOILUT(const DVPSVOILUT& copy);

  /** clone method.
   *  @return a pointer to a new DVPSVOILUT object containing
   *  a copy of this object.
   */
  DVPSVOILUT *clone() { return new DVPSVOILUT(*this); }

  /// destructor
  virtual ~DVPSVOILUT();

  /** reads a VOI LUT from a DICOM dataset.
   *  The DICOM elements of the VOI LUT item are copied
   *  from the dataset to this object.
   *  The completeness of the item (presence of all required elements,
   *  value multiplicity) is checked.
   *  If this method returns an error code, the object is in undefined state afterwards.
   *  @param dset the item of the VOI LUT Sequence from which the data is to be read
   *  @return EC_Normal if successful, an error code otherwise.
   */
  OFCondition read(DcmItem &dset);
  
  /** resets the object to initial state.
   *  After this call, the object is in the same state as after
   *  creation with the default constructor.
   */
  void clear();
  
  /** gets the LUT explanation for this VOI LUT.
   *  If no explanation exists, NULL is returned.
   *  @return LUT explanation or NULL
   */
  const char *getExplanation();

  /** assigns the contents of this VOI LUT to the
   *  references passed as parameters.
   *  @param reference to the Softcopy VOI in which the LUT is stored.
   */
  OFCondition assign(DVPSSoftcopyVOI& voi);

  /** sets a new log stream
   *  @param stream new log stream, NULL for default logstream
   *  @param verbMode verbose mode flag
   *  @param dbgMode debug mode flag
   */
  void setLog(OFConsole *stream, OFBool verbMode, OFBool dbgMode);

private:

  /// private undefined assignment operator
  DVPSVOILUT& operator=(const DVPSVOILUT&);

  /// Module=VOI_LUT, VR=xs, VM=3, Type 1c 
  DcmUnsignedShort         voiLUTDescriptor;
  /// Module=VOI_LUT, VR=LO, VM=1, Type 3 
  DcmLongString            voiLUTExplanation;
  /// Module=VOI_LUT, VR=xs, VM=1-n, Type 1c 
  DcmUnsignedShort         voiLUTData;
  
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
 *  $Log: dvpsvl.h,v $
 *  Revision 1.7  2005/12/08 16:04:11  meichel
 *  Changed include path schema for all DCMTK header files
 *
 *  Revision 1.6  2001/09/26 15:36:18  meichel
 *  Adapted dcmpstat to class OFCondition
 *
 *  Revision 1.5  2001/06/01 15:50:25  meichel
 *  Updated copyright header
 *
 *  Revision 1.4  2000/06/02 16:00:54  meichel
 *  Adapted all dcmpstat classes to use OFConsole for log and error output
 *
 *  Revision 1.3  2000/03/08 16:28:59  meichel
 *  Updated copyright header.
 *
 *  Revision 1.2  1999/07/22 16:39:15  meichel
 *  Adapted dcmpstat data structures and API to supplement 33 letter ballot text.
 *
 *  Revision 1.1  1998/12/22 17:57:09  meichel
 *  Implemented Presentation State interface for overlays,
 *    VOI LUTs, VOI windows, curves. Added test program that
 *    allows to add curve data to DICOM images.
 *
 *
 */

