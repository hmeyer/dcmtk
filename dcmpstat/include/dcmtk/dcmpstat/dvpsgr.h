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
 *    classes: DVPSGraphicObject
 *
 *  Last Update:      $Author: meichel $
 *  Update Date:      $Date: 2005/12/08 16:03:47 $
 *  CVS/RCS Revision: $Revision: 1.7 $
 *  Status:           $State: Exp $
 *
 *  CVS/RCS Log at end of file
 *
 */

#ifndef __DVPSGR_H__
#define __DVPSGR_H__

#include "dcmtk/config/osconfig.h"    /* make sure OS specific configuration is included first */
#include "dcmtk/dcmdata/dctk.h"
#include "dcmtk/dcmpstat/dvpstyp.h"

/** an item of the graphic object sequence in a presentation state (internal use only).
 *  This class manages the data structures comprising one item
 *  of the Graphic Object Sequence which is contained
 *  in the Graphic Annotation Sequence in a Presentation State object.
 */

class DVPSGraphicObject
{
public:
  /// default constructor
  DVPSGraphicObject();
  
  /// copy constructor
  DVPSGraphicObject(const DVPSGraphicObject& copy);

  /** clone method.
   *  @return a pointer to a new DVPSGraphicObject object containing
   *  a copy of this object.
   */
  DVPSGraphicObject *clone() { return new DVPSGraphicObject(*this); }

  /// destructor
  virtual ~DVPSGraphicObject();

  /** reads a graphic object from a DICOM dataset.
   *  The DICOM elements of the Graphic Object item are copied
   *  from the dataset to this object.
   *  The completeness of the item (presence of all required elements,
   *  value multiplicity) is checked.
   *  If this method returns an error code, the object is in undefined state afterwards.
   *  @param dset the item of the GraphicObjectSequence from which the data is to be read
   *  @return EC_Normal if successful, an error code otherwise.
   */
  OFCondition read(DcmItem &dset);

  /** writes the graphic object managed by this object to a DICOM dataset.
   *  Copies of the DICOM element managed by this object are inserted into
   *  the DICOM dataset.
   *  @param dset the the item of the GraphicObjectSequence to which the data is written
   *  @return EC_Normal if successful, an error code otherwise.
   */
  OFCondition write(DcmItem &dset);

  /** gets the graphic annotation units.
   *  @return annotation units
   */
  DVPSannotationUnit getAnnotationUnits();

  /** gets the number of graphic points.
   *  @return number of graphic points
   */
  size_t getNumberOfPoints();

  /** gets one point from the graphic data.
   *  @param idx index of the graphic point, must be < getNumberOfPoints();
   *  @param x upon success the x value of the point is returned in this parameter
   *  @param y upon success the y value of the point is returned in this parameter   
   *  @return EC_Normal if successful, an error code otherwise.
   */
  OFCondition getPoint(size_t idx, Float32& x, Float32& y);

  /** gets the graphic type of this graphic object.
   *  @return graphic type
   */
  DVPSGraphicType getGraphicType();

  /** checks if the graphic is filled
   *  @return OFTrue if graphic is filled.
   */
  OFBool isFilled();
  
  /** sets the graphic data for this graphic object.
   *  @param number number of graphic points in parameter "data"
   *  @param data pointer to an array of Float32 values with a size of (at least)
   *  2*number. The values are copied into the graphic object.
   *  @param unit the graphic annotation units for this data.
   *  @return EC_Normal if successful, an error code otherwise.
   */
  OFCondition setData(size_t number, const Float32 *data, DVPSannotationUnit unit);

  /** sets the graphic type for the graphic object
   *  @param gtype the graphic type
   *  @return EC_Normal if successful, an error code otherwise.
   */
  OFCondition setGraphicType(DVPSGraphicType gtype);

  /** sets the graphic filled status for the graphic object
   *  @param isFilled OFTrue if graphic is filled, OFFalse otherwise.
   *  @return EC_Normal if successful, an error code otherwise.
   */
  OFCondition setFilled(OFBool filled);

  /** sets a new log stream
   *  @param stream new log stream, NULL for default logstream
   *  @param verbMode verbose mode flag
   *  @param dbgMode debug mode flag
   */
  void setLog(OFConsole *stream, OFBool verbMode, OFBool dbgMode);

private:

  /// private undefined assignment operator
  DVPSGraphicObject& operator=(const DVPSGraphicObject&);

  /// VR=CS, VM=1, Type 1 
  DcmCodeString            graphicAnnotationUnits;
  /// VR=US, VM=1, Type 1 
  DcmUnsignedShort         numberOfGraphicPoints;
  /// VR=FL, VM=2-n, Type 1 
  DcmFloatingPointSingle   graphicData;
  /// VR=CS, VM=1, Type 1 
  DcmCodeString            graphicType;
  /// VR=CS, VM=1, Type 1c 
  DcmCodeString            graphicFilled;

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
 *  $Log: dvpsgr.h,v $
 *  Revision 1.7  2005/12/08 16:03:47  meichel
 *  Changed include path schema for all DCMTK header files
 *
 *  Revision 1.6  2001/09/26 15:36:11  meichel
 *  Adapted dcmpstat to class OFCondition
 *
 *  Revision 1.5  2001/06/01 15:50:16  meichel
 *  Updated copyright header
 *
 *  Revision 1.4  2000/06/02 16:00:47  meichel
 *  Adapted all dcmpstat classes to use OFConsole for log and error output
 *
 *  Revision 1.3  2000/03/08 16:28:52  meichel
 *  Updated copyright header.
 *
 *  Revision 1.2  1998/12/14 16:10:30  meichel
 *  Implemented Presentation State interface for graphic layers,
 *    text and graphic annotations, presentation LUTs.
 *
 *  Revision 1.1  1998/11/27 14:50:28  meichel
 *  Initial Release.
 *
 *
 */

