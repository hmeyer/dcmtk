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
 *    classes: DVPSReferencedSeries_PList
 *
 *  Last Update:      $Author: meichel $
 *  Update Date:      $Date: 2005/12/08 16:04:02 $
 *  CVS/RCS Revision: $Revision: 1.10 $
 *  Status:           $State: Exp $
 *
 *  CVS/RCS Log at end of file
 *
 */

#ifndef __DVPSRSL_H__
#define __DVPSRSL_H__

#include "dcmtk/config/osconfig.h"    /* make sure OS specific configuration is included first */
#include "dcmtk/ofstd/oflist.h"
#include "dcmtk/dcmdata/dctk.h"


class DVPSReferencedSeries;
class DVPSReferencedImage;

/** the list of referenced series contained in a presentation state (internal use only).
 *  This class manages the data structures comprising the complete
 *  Referenced Series Sequence in a Presentation State object.
 */

class DVPSReferencedSeries_PList
{
public:
  /// default constructor
  DVPSReferencedSeries_PList();
  
  /// copy constructor
  DVPSReferencedSeries_PList(const DVPSReferencedSeries_PList& copy);

  /** clone method.
   *  @return a pointer to a new DVPSReferencedSeries_PList object containing
   *  a deep copy of this object.
   */
  DVPSReferencedSeries_PList *clone() { return new DVPSReferencedSeries_PList(*this); }

  /// destructor
  virtual ~DVPSReferencedSeries_PList();

  /** reads a list of series references (ReferencedSeriesSequence) from a DICOM dataset.
   *  The DICOM elements of the image references item are copied from the dataset to this object.
   *  The completeness of all items (presence of all required elements,
   *  value multiplicity) is checked.
   *  If this method returns an error code, the object is in undefined state afterwards.
   *  @param dset the DICOM dataset from which the sequence is to be read
   *  @return EC_Normal if successful, an error code otherwise.
   */
  OFCondition read(DcmItem &dset);
  
  /** writes the list of series references managed by this object to a DICOM dataset.
   *  Copies of the DICOM element managed by this object are inserted into
   *  the DICOM dataset.
   *  @param dset the DICOM dataset to which the ReferencedSeriesSequence is written
   *  @return EC_Normal if successful, an error code otherwise.
   */
  OFCondition write(DcmItem &dset);

  /** reset the object to initial state.
   *  After this call, the object is in the same state as after
   *  creation with the default constructor.
   */
  void clear();
  
  /** checks whether at least one series is referenced, whether all referenced
    * series contain image references and whether all references images use the same
    * SOP class.
    */
  OFBool isValid();
  
  /** checks whether all images referenced in this series use the passed SOP class UID.
   *  @param uid the SOP class UID to be checked
   *  @return result of the test
   */
  OFBool checkSOPClass(const char *uid);
 
  /** find series reference in this list.
   *  This method checks if a series reference with the given Series Instance UID exists
   *  in this ReferencedSeriesSequence.
   *  @param seriesUID the Series Instance UID of the searched series reference
   *  @return a pointer to the matching DVPSReferencedSeries if found, NULL otherwise.
   */
  DVPSReferencedSeries *findSeriesReference(const char *seriesUID);

  /** find image reference in this list.
   *  This method checks if an image reference with the given Series Instance UID and 
   *  SOP instance UID exists in this ReferencedImageSequence.
   *  @param seriesUID the Series Instance UID of the searched image reference
   *  @param instanceUID the SOP Instance UID of the searched image reference
   *  @return a pointer to the matching DVPSReferencedImage if found, NULL otherwise.
   */
  DVPSReferencedImage *findImageReference(const char *seriesUID, const char *instanceUID);
  
  /** removes series reference.
   *  This method checks if an series reference with the given Series Instance UID exists
   *  in this ReferencedSeriesSequence and deletes it (including its image references).
   *  @param seriesUID the Series Instance UID of the series reference to be removed.
   */
  void removeSeriesReference(const char *seriesUID);

  /** removes image reference.
   *  This method checks if an image reference with the given SOP instance UID exists
   *  in the series with the given Series Instance UID and deletes it.
   *  @param seriesUID the Series Instance UID of the image reference to be removed.
   *  @param instanceUID the SOP instance UID of the image reference to be removed.
   */
  void removeImageReference(const char *seriesUID, const char *instanceUID);
  
  /** add new image reference.
   *  This method checks if the given SOP class matches the SOP class of all
   *  other existing image references and whether the image reference exists.
   *  In this case an error code is returned. Otherwise a new image reference
   *  (in a new series if necessary) is created.
   *  @param seriesUID the Series Instance UID of the image reference to be added.
   *  @param sopclassUID the SOP class UID of the image reference to be added.
   *  @param instanceUID the SOP instance UID of the image reference to be added.
   *  @param frames a list of frame numbers in DICOM IS format
   *    (integer numbers separated by '\' characters). Default: frame numbers absent.
   *    The frame numbers are required if the referenced image is a multiframe image.
   *  @param aetitle the series retrieveAETitle. Default: value absent.
   *  @param filesetID the series storageMediaFileSetID. Default: value absent.
   *  @param filesetUID the series storageMediaFileSetUID. Default: value absent.
   *  @return EC_Normal if successful, an error code otherwise.
   */
  OFCondition addImageReference(
    const char *seriesUID,
    const char *sopclassUID,
    const char *instanceUID, 
    const char *frames=NULL,
    const char *aetitle=NULL, 
    const char *filesetID=NULL, 
    const char *filesetUID=NULL);

  /** gets the number of image references in all series managed by this list.
   *  @return number of image references
   */
  size_t numberOfImageReferences();
  
  /** gets an image reference with the given index.
   *  @param idx index, must be < numberOfImageReferences().
   *  @param seriesUID the Series Instance UID is returned in this string
   *  @param sopclassUID the SOP Class UID is returned in this string
   *  @param instanceUID the SOP Instance UID is returned in this string
   *  @param frames the list of frames is returned in this string
   *  @param aetitle the series retrieveAETitle is returned in this string
   *  @param filesetID the series storageMediaFileSetID is returned in this string
   *  @param filesetUID the series storageMediaFileSetUID is returned in this string
   *  @return EC_Normal if successful, an error code otherwise.
   */
  OFCondition getImageReference(
    size_t idx,
    OFString& seriesUID,
    OFString& sopclassUID,
    OFString& instanceUID, 
    OFString& frames,
    OFString& aetitle,
    OFString& filesetID,
    OFString& filesetUID);

  /** sets a new log stream
   *  @param stream new log stream, NULL for default logstream
   *  @param verbMode verbose mode flag
   *  @param dbgMode debug mode flag
   */
  void setLog(OFConsole *stream, OFBool verbMode, OFBool dbgMode);

private:

  /// private undefined assignment operator
  DVPSReferencedSeries_PList& operator=(const DVPSReferencedSeries_PList&);

  /** the list maintained by this object
   */
  OFList<DVPSReferencedSeries *> list_;

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
 *  $Log: dvpsrsl.h,v $
 *  Revision 1.10  2005/12/08 16:04:02  meichel
 *  Changed include path schema for all DCMTK header files
 *
 *  Revision 1.9  2003/06/04 10:18:06  meichel
 *  Replaced private inheritance from template with aggregation
 *
 *  Revision 1.8  2001/09/26 15:36:15  meichel
 *  Adapted dcmpstat to class OFCondition
 *
 *  Revision 1.7  2001/06/01 15:50:21  meichel
 *  Updated copyright header
 *
 *  Revision 1.6  2000/06/02 16:00:51  meichel
 *  Adapted all dcmpstat classes to use OFConsole for log and error output
 *
 *  Revision 1.5  2000/03/08 16:28:56  meichel
 *  Updated copyright header.
 *
 *  Revision 1.4  1999/07/22 16:39:11  meichel
 *  Adapted dcmpstat data structures and API to supplement 33 letter ballot text.
 *
 *  Revision 1.3  1999/01/15 17:33:04  meichel
 *  added methods to DVPresentationState allowing to access the image
 *    references in the presentation state.  Also added methods allowing to
 *    get the width and height of the attached image.
 *
 *  Revision 1.2  1998/12/14 16:10:34  meichel
 *  Implemented Presentation State interface for graphic layers,
 *    text and graphic annotations, presentation LUTs.
 *
 *  Revision 1.1  1998/11/27 14:50:33  meichel
 *  Initial Release.
 *
 *
 */

