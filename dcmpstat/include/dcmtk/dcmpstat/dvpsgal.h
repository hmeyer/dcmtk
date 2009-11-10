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
 *    classes: DVPSGraphicAnnotation_PList
 *
 *  Last Update:      $Author: meichel $
 *  Update Date:      $Date: 2005/12/08 16:03:44 $
 *  CVS/RCS Revision: $Revision: 1.9 $
 *  Status:           $State: Exp $
 *
 *  CVS/RCS Log at end of file
 *
 */

#ifndef __DVPSGAL_H__
#define __DVPSGAL_H__

#include "dcmtk/config/osconfig.h"    /* make sure OS specific configuration is included first */
#include "dcmtk/ofstd/oflist.h"
#include "dcmtk/dcmdata/dctk.h"
#include "dcmtk/dcmpstat/dvpstyp.h"     /* for enum types */

class DVPSGraphicAnnotation;
class DVPSTextObject;
class DVPSGraphicObject;

/** the list of graphic annotations contained in a presentation state (internal use only).
 *  This class manages the data structures comprising the complete
 *  Graphic Annotation Sequence in a Presentation State object.
 */

class DVPSGraphicAnnotation_PList
{
public:
  /// default constructor
  DVPSGraphicAnnotation_PList();
  
  /// copy constructor
  DVPSGraphicAnnotation_PList(const DVPSGraphicAnnotation_PList& copy);

  /** clone method.
   *  @return a pointer to a new DVPSGraphicAnnotation_PList object containing
   *  a deep copy of this object.
   */
  DVPSGraphicAnnotation_PList *clone() { return new DVPSGraphicAnnotation_PList(*this); }

  /// destructor
  virtual ~DVPSGraphicAnnotation_PList();

  /** reads a list of graphic annotations from a DICOM dataset.
   *  The DICOM elements of the Graphic Annotation Sequence are copied
   *  from the dataset to this object.
   *  The completeness of the item (presence of all required elements,
   *  value multiplicity) is checked.
   *  If this method returns an error code, the object is in undefined state afterwards.
   *  @param dset the dataset from which the GraphicAnnotationSequence is to be read
   *  @return EC_Normal if successful, an error code otherwise.
   */
  OFCondition read(DcmItem &dset);

  /** writes the list of graphic annotations managed by this object to a DICOM dataset.
   *  Copies of the DICOM elements managed by this object are inserted into
   *  the DICOM dataset.
   *  @param dset the dataset to which the GraphicAnnotationSequence is written
   *  @return EC_Normal if successful, an error code otherwise.
   */
  OFCondition write(DcmItem &dset);
  
  /** reset the object to initial state.
   *  After this call, the object is in the same state as after
   *  creation with the default constructor.
   */
  void clear();

  /** renames the graphic annotation layer name in all activations
   *  with a matching old graphic annotation layer name.
   *  Required to keep the presentation consistent when a
   *  graphic layer is renamed.
   *  @param oldName the old graphic annotation layer name
   *  @param newName the new graphic annotation layer name
   */
  void renameLayer(const char *oldName, const char *newName);

  /** deletes all graphic annotation layers belonging to the given 
   *  graphic annotation layer name.
   *  @param name name of the graphic annotation layers to be deleted
   */
  void removeLayer(const char *name);

  /** deletes all graphic annotation sequence items containing
   *  no text and no graphic object. Called before writing a presentation state.
   */
  void cleanupLayers();
  
  /** checks if the given layer name is used for any of the
   *  graphic annotation layers managed by this object.
   *  @param name name of the layer
   *  @return OFTrue if name is used
   */
  OFBool usesLayerName(const char *name);

  /** returns the number of text objects for the given graphic layer
   *  that apply to the given image and frame.
   *  @param layer name of the graphic layer
   *  @param instanceUID SOP instance UID of the current image
   *  @param frame number of the current frame
   *  @return number of text objects
   */   
  size_t getNumberOfTextObjects(const char *layer, const char *instanceUID, unsigned long frame);

  /** gets the text object (applicable to the current image and frame) 
   *  with the given index on the given layer. 
   *  If the text object or the graphic layer does not exist, NULL is returned.
   *  @param layer name of the graphic layer
   *  @param instanceUID SOP instance UID of the current image
   *  @param frame number of the current frame
   *  @param idx index of the text object, must be < getNumberOfTextObjects(layer)
   *  @return a pointer to the text object
   */   
  DVPSTextObject *getTextObject(const char *layer, const char *instanceUID, unsigned long frame, size_t idx);

  /** creates a new text object on the given layer. 
   *  Returns a pointer to the new text object. 
   *  If no graphic layer with appropriate applicability exists, it is created.
   *  If the creation of the layer or text object fails, NULL is returned.
   *  @param layer name of the graphic layer
   *  @param sopclassUID SOP class UID of the current image
   *  @param instanceUID SOP instance UID of the current image
   *  @param frame number of the current frame
   *  @param applicability applicability of the new text object
   *  @param text the text object to be inserted. If NULL, a new text object
   *    is created. If a pointer to an object is passed in this parameter,
   *    it gets owned by this graphic annotation object and will be deleted
   *    upon destruction of the annotation or if this method fails (returns NULL).
   *  @return a pointer to the new text object
   */   
  DVPSTextObject *addTextObject(
    const char *layer, 
    const char *sopclassUID, 
    const char *instanceUID, 
    unsigned long frame, 
    DVPSObjectApplicability applicability, 
    DVPSTextObject *text=NULL);

  /** deletes the text object (applicable to the current image and frame) with the given index
   *  on the given layer. 
   *  @param layer name of the graphic layer
   *  @param instanceUID SOP instance UID of the current image
   *  @param frame number of the current frame
   *  @param idx index of the text object, must be < getNumberOfTextObjects(layer)
   *  @return EC_Normal upon success, an error code otherwise
   */   
  OFCondition removeTextObject(const char *layer, const char *instanceUID, unsigned long frame, size_t idx);

  /** moves the text object (applicable to the current image and frame) with the given index 
   *  on the given layer to a different layer. 
   *  @param old_layer name of the graphic layer on which the text object is
   *  @param sopclassUID SOP class UID of the current image
   *  @param instanceUID SOP instance UID of the current image
   *  @param frame number of the current frame
   *  @param idx index of the text object, must be < getNumberOfTextObjects(layer)
   *  @param applicability new applicability of the text object
   *  @param new_layer name of the graphic layer to which the text object is moved
   *  @return EC_Normal upon success, an error code otherwise
   */   
  OFCondition moveTextObject(
    const char *old_layer, 
    const char *sopclassUID, 
    const char *instanceUID,
    unsigned long frame,
    size_t idx,
    DVPSObjectApplicability applicability, 
    const char *new_layer);

  /** returns the number of graphic objects for the given
   *  graphic layer. 
   *  @param layer name of the graphic layer
   *  @param instanceUID SOP instance UID of the current image
   *  @param frame number of the current frame
   *  @return number of graphic objects
   */   
  size_t getNumberOfGraphicObjects(const char *layer, const char *instanceUID, unsigned long frame);

  /** gets the graphic object with the given index
   *  on the given layer. If the graphic object or the graphic layer does
   *  not exist, NULL is returned.
   *  @param layer name of the graphic layer
   *  @param instanceUID SOP instance UID of the current image
   *  @param frame number of the current frame
   *  @param idx index of the graphic object, must be < getNumberOfGraphicObjects(layer)
   *  @return a pointer to the graphic object
   */   
  DVPSGraphicObject *getGraphicObject(const char *layer, const char *instanceUID, unsigned long frame, size_t idx);

  /** creates a new graphic object on the given layer. 
   *  Returns a pointer to the new graphic object. If the graphic layer
   *  does not exist or if the creation of the graphic object fails, NULL is returned.
   *  @param layer name of the graphic layer
   *  @param sopclassUID SOP class UID of the current image
   *  @param instanceUID SOP instance UID of the current image
   *  @param frame number of the current frame
   *  @param applicability applicability of the new text object
   *  @param graphic the graphic object to be inserted. If NULL, a new graphic object
   *    is created. If a pointer to an object is passed in this parameter,
   *    it gets owned by this graphic annotation object and will be deleted
   *    upon destruction of the annotation or if this method fails (returns NULL).
   *  @return a pointer to the new graphic object
   */   
  DVPSGraphicObject *addGraphicObject(
    const char *layer, 
    const char *sopclassUID, 
    const char *instanceUID, 
    unsigned long frame, 
    DVPSObjectApplicability applicability, 
    DVPSGraphicObject *graphic=NULL);

  /** deletes the graphic object with the given index
   *  on the given layer. 
   *  @param layer name of the graphic layer
   *  @param instanceUID SOP instance UID of the current image
   *  @param frame number of the current frame
   *  @param idx index of the graphic object, must be < getNumberOfGraphicObjects(layer)
   *  @return EC_Normal upon success, an error code otherwise
   */   
  OFCondition removeGraphicObject(const char *layer, const char *instanceUID, unsigned long frame, size_t idx);

  /** moves the graphic object with the given index on the given
   *  layer to a different layer. 
   *  @param old_layer name of the graphic layer on which the graphic object is
   *  @param sopclassUID SOP class UID of the current image
   *  @param instanceUID SOP instance UID of the current image
   *  @param frame number of the current frame
   *  @param idx index of the graphic object, must be < getNumberOfGraphicObjects(layer)
   *  @param applicability new applicability of the graphic object
   *  @param new_layer name of the graphic layer to which the graphic object is moved
   *  @return EC_Normal upon success, an error code otherwise
   */   
  OFCondition moveGraphicObject(
    const char *old_layer, 
    const char *sopclassUID, 
    const char *instanceUID,
    unsigned long frame,
    size_t idx,
    DVPSObjectApplicability applicability, 
    const char *new_layer);

  /** sets a new log stream
   *  @param stream new log stream, NULL for default logstream
   *  @param verbMode verbose mode flag
   *  @param dbgMode debug mode flag
   */
  void setLog(OFConsole *stream, OFBool verbMode, OFBool dbgMode);

private:

  /// private undefined assignment operator
  DVPSGraphicAnnotation_PList& operator=(const DVPSGraphicAnnotation_PList&);

  /** the list maintained by this object
   */
  OFList<DVPSGraphicAnnotation *> list_;

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
 *  $Log: dvpsgal.h,v $
 *  Revision 1.9  2005/12/08 16:03:44  meichel
 *  Changed include path schema for all DCMTK header files
 *
 *  Revision 1.8  2003/06/04 10:18:06  meichel
 *  Replaced private inheritance from template with aggregation
 *
 *  Revision 1.7  2001/09/26 15:36:10  meichel
 *  Adapted dcmpstat to class OFCondition
 *
 *  Revision 1.6  2001/06/01 15:50:15  meichel
 *  Updated copyright header
 *
 *  Revision 1.5  2000/06/02 16:00:46  meichel
 *  Adapted all dcmpstat classes to use OFConsole for log and error output
 *
 *  Revision 1.4  2000/03/08 16:28:51  meichel
 *  Updated copyright header.
 *
 *  Revision 1.3  1999/07/22 16:39:07  meichel
 *  Adapted dcmpstat data structures and API to supplement 33 letter ballot text.
 *
 *  Revision 1.2  1998/12/14 16:10:28  meichel
 *  Implemented Presentation State interface for graphic layers,
 *    text and graphic annotations, presentation LUTs.
 *
 *  Revision 1.1  1998/11/27 14:50:26  meichel
 *  Initial Release.
 *
 *
 */

