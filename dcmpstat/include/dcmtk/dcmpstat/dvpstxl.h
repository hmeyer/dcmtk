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
 *    classes: DVPSTextObject_PList
 *
 *  Last Update:      $Author: meichel $
 *  Update Date:      $Date: 2005/12/08 16:04:09 $
 *  CVS/RCS Revision: $Revision: 1.8 $
 *  Status:           $State: Exp $
 *
 *  CVS/RCS Log at end of file
 *
 */

#ifndef __DVPSTXL_H__
#define __DVPSTXL_H__

#include "dcmtk/config/osconfig.h"    /* make sure OS specific configuration is included first */
#include "dcmtk/ofstd/oflist.h"
#include "dcmtk/dcmdata/dctk.h"

class DVPSTextObject;


/** the list of text objects contained in a presentation state (internal use only).
 *  This class manages the data structures comprising one complete
 *  Text Object Sequence which is contained in one item
 *  of the Graphic Annotation Sequence in a Presentation State object.
 */

class DVPSTextObject_PList
{
public:
  /// default constructor
  DVPSTextObject_PList();
  
  /// copy constructor
  DVPSTextObject_PList(const DVPSTextObject_PList& copy);

  /** clone method.
   *  @return a pointer to a new DVPSTextObject_PList object containing
   *  a deep copy of this object.
   */
  DVPSTextObject_PList *clone() { return new DVPSTextObject_PList(*this); }

  /// destructor
  virtual ~DVPSTextObject_PList();

  /** reads a list of text objects from a DICOM dataset.
   *  The DICOM elements of the Text Object Sequence are copied
   *  from the dataset to this object.
   *  The completeness of the item (presence of all required elements,
   *  value multiplicity) is checked.
   *  If this method returns an error code, the object is in undefined state afterwards.
   *  @param dset the dataset from which the TextObjectSequence is to be read
   *  @return EC_Normal if successful, an error code otherwise.
   */
  OFCondition read(DcmItem &dset);

  /** writes the list of text objects managed by this object to a DICOM dataset.
   *  Copies of the DICOM elements managed by this object are inserted into
   *  the DICOM dataset.
   *  @param dset the dataset to which the TextObjectSequence is written
   *  @return EC_Normal if successful, an error code otherwise.
   */
  OFCondition write(DcmItem &dset);

  /** reset the object to initial state.
   *  After this call, the object is in the same state as after
   *  creation with the default constructor.
   */
  void clear();
  
  /** get number of text objects in this list.
   *  @return the number of text objects.
   */
  size_t size() const { return list_.size(); }  

  /** returns a pointer to the text object with the given
   *  index or NULL if it does not exist.
   *  @param idx index, must be < size()
   *  @return pointer to text object or NULL
   */
  DVPSTextObject *getTextObject(size_t idx);

  /** adds the given text object to
   *  the list of text objects managed by this object.
   *  @param text text object to be inserted.
   */
  void addTextObject(DVPSTextObject *text);

  /** returns a pointer to the text object with the given
   *  index (or NULL if it does not exist) and removes it from the list.
   *  @param idx index, must be < size()
   *  @return pointer to text object or NULL
   */
  DVPSTextObject *removeTextObject(size_t idx);

  /** sets a new log stream
   *  @param stream new log stream, NULL for default logstream
   *  @param verbMode verbose mode flag
   *  @param dbgMode debug mode flag
   */
  void setLog(OFConsole *stream, OFBool verbMode, OFBool dbgMode);

private:
  /** private undefined assignment operator
   */
  DVPSTextObject_PList& operator=(const DVPSTextObject_PList&);

  /** the list maintained by this object
   */
  OFList<DVPSTextObject *> list_;

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
 *  $Log: dvpstxl.h,v $
 *  Revision 1.8  2005/12/08 16:04:09  meichel
 *  Changed include path schema for all DCMTK header files
 *
 *  Revision 1.7  2003/06/04 10:18:06  meichel
 *  Replaced private inheritance from template with aggregation
 *
 *  Revision 1.6  2001/09/26 15:36:18  meichel
 *  Adapted dcmpstat to class OFCondition
 *
 *  Revision 1.5  2001/06/01 15:50:24  meichel
 *  Updated copyright header
 *
 *  Revision 1.4  2000/06/02 16:00:54  meichel
 *  Adapted all dcmpstat classes to use OFConsole for log and error output
 *
 *  Revision 1.3  2000/03/08 16:28:58  meichel
 *  Updated copyright header.
 *
 *  Revision 1.2  1998/12/14 16:10:37  meichel
 *  Implemented Presentation State interface for graphic layers,
 *    text and graphic annotations, presentation LUTs.
 *
 *  Revision 1.1  1998/11/27 14:50:35  meichel
 *  Initial Release.
 *
 *
 */

