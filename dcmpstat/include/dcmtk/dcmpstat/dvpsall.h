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
 *    classes: DVPSOverlayCurveActivationLayer
 *
 *  Last Update:      $Author: meichel $
 *  Update Date:      $Date: 2005/12/08 16:03:35 $
 *  CVS/RCS Revision: $Revision: 1.9 $
 *  Status:           $State: Exp $
 *
 *  CVS/RCS Log at end of file
 *
 */

#ifndef __DVPSALL_H__
#define __DVPSALL_H__

#include "dcmtk/config/osconfig.h"    /* make sure OS specific configuration is included first */
#include "dcmtk/ofstd/oflist.h"
#include "dcmtk/dcmdata/dctk.h"

#include "dcmtk/dcmpstat/dvpstyp.h"     /* for enum types */

class DVPSGraphicLayer_PList;
class DVPSOverlay_PList;
class DVPSOverlayCurveActivationLayer;

/** the list of curve and overlay activation layers contained in a presentation state (internal use only).
 *  This class manages the data structures comprising the list of curve
 *  activation layers and overlay activation layers
 *  (all instances of the Curve Activation Layer Module and 
 *  Overlay Activation Layer Module repeating elements)
 *  contained in a Presentation State object.
 */

class DVPSOverlayCurveActivationLayer_PList
{
public:
  /// default constructor
  DVPSOverlayCurveActivationLayer_PList();
  
  /// copy constructor
  DVPSOverlayCurveActivationLayer_PList(const DVPSOverlayCurveActivationLayer_PList& copy);

  /** clone method.
   *  @return a pointer to a new DVPSOverlayCurveActivationLayer_PList object containing
   *  a deep copy of this object.
   */
  DVPSOverlayCurveActivationLayer_PList *clone() { return new DVPSOverlayCurveActivationLayer_PList(*this); }

  /// destructor
  virtual ~DVPSOverlayCurveActivationLayer_PList();

  /** reads the curve and overlay activations from a DICOM dataset.
   *  The DICOM elements of the activations are copied from the dataset to this object.
   *  If this method returns an error code, the object is in undefined state afterwards.
   *  @param dset the DICOM dataset from which the activations are to be read
   *  @return EC_Normal if successful, an error code otherwise.
   */
  OFCondition read(DcmItem &dset);
  
  /** writes the curve and overlay activations managed by this object to a DICOM dataset.
   *  Copies of the DICOM elements managed by this object are inserted into
   *  the DICOM dataset.
   *  @param dset the DICOM dataset to which the activations are written
   *  @return EC_Normal if successful, an error code otherwise.
   */
   OFCondition write(DcmItem &dset);

  /** reset the object to initial state.
   *  After this call, the object is in the same state as after
   *  creation with the default constructor.
   */
  void clear();
  
  /** create graphic layers and activations for DICOM image.
   *  This method is used when a default presentation state
   *  for a DICOM image is created. Depending on the given flags,
   *  graphic layer and curve/overlay activations for the curves
   *  and overlays present in the DICOM dataset are created.
   *  If this method returns an error code, the object is in undefined state afterwards.
   *  @param dset the DICOM dataset containing the image IOD
   *  @param gLayerList the list of graphic layers to be created
   *  @param overlayList the list of overlays internal to the presentation state
   *  @param overlayActivation flag defining how overlays should be handled
   *  @param curveActivation flag defining how curves should be handled
   *  @param layering flag defining how graphic layers should be created
   *  @return EC_Normal upon success, an error code otherwise.
   */
  OFCondition createFromImage(DcmItem &dset,
    DVPSGraphicLayer_PList &gLayerList,
    DVPSOverlay_PList &overlayList,
    DVPSoverlayActivation overlayActivation,
    OFBool                curveActivation,
    DVPSGraphicLayering   layering);
    
  /** set activation layer for given repeating group.
   *  The activation is created if necessary and the layer name
   *  is assigned. This method check if a valid repeating group
   *  number is passed and returns an error code otherwise.
   *  @return EC_Normal if successful, an error code otherwise.
   */
  OFCondition setActivation(Uint16 group, const char *layer);

  /** remove activation for given repeating group.
   */
  void removeActivation(Uint16 group);

  /** get activation layer name of the given repeating group.
   *  @return a pointer to the activation layer name if found, NULL otherwise.
   */
  const char *getActivationLayer(Uint16 group);
  
  /** renames the activation layer name in all activations
   *  with a matching old activation layer name.
   *  Required to keep the presentation consistent when a
   *  graphic layer is renamed.
   *  @param oldName the old activation layer name
   *  @param newName the new activation layer name
   */
  void renameLayer(const char *oldName, const char *newName);

  /** deletes all activation belonging to the given activation
   *  layer name.
   *  @param name name of the deleted activation layer
   */
  void removeLayer(const char *name);

  /** checks if the given layer name is used for any of the
   *  activation layers managed by this object.
   *  @param name name of the layer
   *  @return OFTrue if name is used
   */
  OFBool usesLayerName(const char *name);

  /** returns the number of activations for the given
   *  graphic layer and object type (curve or overlay).
   *  This method does not check whether an image object is attached to the
   *  presentation state and if all activations really have a matching 
   *  curve structure in the attached image.
   *  @param layer name of the graphic layer
   *  @param isCurve if OFTrue, curves are counted, otherwise overlays are counted.
   *  @return number of curves or overlays on the given layer.
   */   
  size_t getNumberOfActivations(const char *layer, OFBool isCurve);

  /** gets the repeating group of the overlay/curve activation with the given index
   *  on the given layer. If the activation or the graphic layer does
   *  not exist, 0 is returned.
   *  @param layer name of the graphic layer
   *  @param idx index of the object, must be < getNumberOfActivations(layer, isCurve)
   *  @param isCurve if OFTrue, curves are searched, otherwise overlays are searched.
   *  @return the repeating group number of the activation
   */   
  Uint16 getActivationGroup(const char *layer, size_t idx, OFBool isCurve);

  /** sets a new log stream
   *  @param stream new log stream, NULL for default logstream
   *  @param verbMode verbose mode flag
   *  @param dbgMode debug mode flag
   */
  void setLog(OFConsole *stream, OFBool verbMode, OFBool dbgMode);

private:

  /// private undefined assignment operator
  DVPSOverlayCurveActivationLayer_PList& operator=(const DVPSOverlayCurveActivationLayer_PList&);

  /** the list maintained by this object
   */
  OFList<DVPSOverlayCurveActivationLayer *> list_;

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
 *  $Log: dvpsall.h,v $
 *  Revision 1.9  2005/12/08 16:03:35  meichel
 *  Changed include path schema for all DCMTK header files
 *
 *  Revision 1.8  2003/06/04 10:18:06  meichel
 *  Replaced private inheritance from template with aggregation
 *
 *  Revision 1.7  2001/09/26 15:36:08  meichel
 *  Adapted dcmpstat to class OFCondition
 *
 *  Revision 1.6  2001/06/01 15:50:12  meichel
 *  Updated copyright header
 *
 *  Revision 1.5  2000/06/02 16:00:43  meichel
 *  Adapted all dcmpstat classes to use OFConsole for log and error output
 *
 *  Revision 1.4  2000/03/08 16:28:49  meichel
 *  Updated copyright header.
 *
 *  Revision 1.3  1998/12/22 17:57:04  meichel
 *  Implemented Presentation State interface for overlays,
 *    VOI LUTs, VOI windows, curves. Added test program that
 *    allows to add curve data to DICOM images.
 *
 *  Revision 1.2  1998/12/14 16:10:26  meichel
 *  Implemented Presentation State interface for graphic layers,
 *    text and graphic annotations, presentation LUTs.
 *
 *  Revision 1.1  1998/11/27 14:50:25  meichel
 *  Initial Release.
 *
 *
 */

