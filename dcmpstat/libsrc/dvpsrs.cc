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
 *    classes: DVPSReferencedSeries
 *
 *  Last Update:      $Author: meichel $
 *  Update Date:      $Date: 2005/12/08 15:46:45 $
 *  CVS/RCS Revision: $Revision: 1.13 $
 *  Status:           $State: Exp $
 *
 *  CVS/RCS Log at end of file
 *
 */

#include "dcmtk/config/osconfig.h"    /* make sure OS specific configuration is included first */
#include "dcmtk/ofstd/ofstring.h"
#include "dcmtk/dcmpstat/dvpsrs.h"
#include "dcmtk/dcmpstat/dvpsdef.h"     /* for constants and macros */
#include "dcmtk/dcmpstat/dvpsri.h"      /* for DVPSReferencedImage, needed by MSVC5 with STL */

/* --------------- class DVPSReferencedSeries --------------- */

DVPSReferencedSeries::DVPSReferencedSeries()
: referencedImageList()
, seriesInstanceUID(DCM_SeriesInstanceUID)
, retrieveAETitle(DCM_RetrieveAETitle)
, storageMediaFileSetID(DCM_StorageMediaFileSetID)
, storageMediaFileSetUID(DCM_StorageMediaFileSetUID)
, logstream(&ofConsole)
, verboseMode(OFFalse)
, debugMode(OFFalse)
{
}

DVPSReferencedSeries::DVPSReferencedSeries(const DVPSReferencedSeries& copy)
: referencedImageList(copy.referencedImageList)
, seriesInstanceUID(copy.seriesInstanceUID)
, retrieveAETitle(copy.retrieveAETitle)
, storageMediaFileSetID(copy.storageMediaFileSetID)
, storageMediaFileSetUID(copy.storageMediaFileSetUID)
, logstream(copy.logstream)
, verboseMode(copy.verboseMode)
, debugMode(copy.debugMode)
{
}

DVPSReferencedSeries::~DVPSReferencedSeries()
{
}

OFCondition DVPSReferencedSeries::read(DcmItem &dset)
{
  OFCondition result = EC_Normal;
  DcmStack stack;

  READ_FROM_DATASET(DcmUniqueIdentifier, seriesInstanceUID)
  READ_FROM_DATASET(DcmApplicationEntity, retrieveAETitle)
  READ_FROM_DATASET(DcmShortString, storageMediaFileSetID)
  READ_FROM_DATASET(DcmUniqueIdentifier, storageMediaFileSetUID)
  if (result==EC_Normal) result = referencedImageList.read(dset);
  
  /* Now perform basic sanity checks */

  if (seriesInstanceUID.getLength() == 0)
  {
    result=EC_IllegalCall;
    if (verboseMode)
    {
      logstream->lockCerr() << "Error: presentation state contains a referenced series SQ item with seriesInstanceUID absent or empty" << endl;
      logstream->unlockCerr();
    }
  }
  else if (seriesInstanceUID.getVM() != 1)
  {
    result=EC_IllegalCall;
    if (verboseMode)
    {
      logstream->lockCerr() << "Error: presentation state contains a referenced series SQ item with seriesInstanceUID VM != 1" << endl;
      logstream->unlockCerr();
    }
  }
  else if (retrieveAETitle.getVM() > 1)
  {
    result=EC_IllegalCall;
    if (verboseMode)
    {
      logstream->lockCerr() << "Error: presentation state contains a referenced series SQ item with retrieveAETitle VM > 1" << endl;
      logstream->unlockCerr();
    }
  }
  else if (storageMediaFileSetID.getVM() > 1)
  {
    result=EC_IllegalCall;
    if (verboseMode)
    {
      logstream->lockCerr() << "Error: presentation state contains a referenced series SQ item with storageMediaFileSetID VM > 1" << endl;
      logstream->unlockCerr();
    }
  }
  else if (storageMediaFileSetUID.getVM() > 1)
  {
    result=EC_IllegalCall;
    if (verboseMode)
    {
      logstream->lockCerr() << "Error: presentation state contains a referenced series SQ item with storageMediaFileSetUID VM > 1" << endl;
      logstream->unlockCerr();
    }
  }

  return result;
}

OFCondition DVPSReferencedSeries::write(DcmItem &dset)
{
  OFCondition result = EC_Normal;
  DcmElement *delem=NULL;
  
  ADD_TO_DATASET(DcmUniqueIdentifier, seriesInstanceUID)
  if (retrieveAETitle.getLength() > 0)
  {
    ADD_TO_DATASET(DcmApplicationEntity, retrieveAETitle)
  }
  if (storageMediaFileSetID.getLength() > 0)
  {
    ADD_TO_DATASET(DcmShortString, storageMediaFileSetID)
  }
  if (storageMediaFileSetUID.getLength() > 0)
  {
    ADD_TO_DATASET(DcmUniqueIdentifier, storageMediaFileSetUID)
  }
  if (result==EC_Normal) result = referencedImageList.write(dset);

  return result;
}

OFBool DVPSReferencedSeries::isValid(OFString& sopclassuid)
{
  return referencedImageList.isValid(sopclassuid);
}

OFBool DVPSReferencedSeries::isSeriesUID(const char *uid)
{
  OFString aString;
  if (uid && (EC_Normal == seriesInstanceUID.getOFString(aString,0)))
  {
    if (aString == uid) return OFTrue;
  }
  return OFFalse;
}

DVPSReferencedImage *DVPSReferencedSeries::findImageReference(const char *sopinstanceuid)
{
  return referencedImageList.findImageReference(sopinstanceuid);
}

void DVPSReferencedSeries::removeImageReference(const char *sopinstanceuid)
{
  referencedImageList.removeImageReference(sopinstanceuid);
  return;
}

OFCondition DVPSReferencedSeries::addImageReference(
    const char *sopclassUID,
    const char *instanceUID, 
    const char *frames)
{
  return referencedImageList.addImageReference(sopclassUID, instanceUID, frames);
}

void DVPSReferencedSeries::setSeriesInstanceUID(const char *uid)
{
  if (uid) seriesInstanceUID.putString(uid); else seriesInstanceUID.clear();
  return;
}

void DVPSReferencedSeries::setRetrieveLocation(const char *aetitle, const char *filesetID, const char *filesetUID)
{
  if (aetitle) retrieveAETitle.putString(aetitle); else retrieveAETitle.clear();
  if (filesetID) storageMediaFileSetID.putString(filesetID); else storageMediaFileSetID.clear();
  if (filesetUID) storageMediaFileSetUID.putString(filesetUID); else storageMediaFileSetUID.clear();
}

const char *DVPSReferencedSeries::getRetrieveAETitle()
{
  char *c = NULL;
  if (EC_Normal == retrieveAETitle.getString(c)) return c; else return NULL;
}

const char *DVPSReferencedSeries::getStorageMediaFileSetID()
{
  char *c = NULL;
  if (EC_Normal == storageMediaFileSetID.getString(c)) return c; else return NULL;
}

const char *DVPSReferencedSeries::getStorageMediaFileSetUID()
{
  char *c = NULL;
  if (EC_Normal == storageMediaFileSetUID.getString(c)) return c; else return NULL;
}


OFCondition DVPSReferencedSeries::getImageReference(
    size_t idx,
    OFString& seriesUID,
    OFString& sopclassUID,
    OFString& instanceUID, 
    OFString& frames,
    OFString& aetitle,
    OFString& filesetID,
    OFString& filesetUID)
{
  OFCondition result = referencedImageList.getImageReference(idx, sopclassUID, instanceUID, frames);
  if (EC_Normal == result) result = seriesInstanceUID.getOFString(seriesUID,0); // must not be empty string
  if (EC_Normal == result) if (retrieveAETitle.getLength() == 0) aetitle.clear(); else result = retrieveAETitle.getOFString(aetitle,0);
  if (EC_Normal == result) if (storageMediaFileSetID.getLength() == 0) filesetID.clear(); else result = storageMediaFileSetID.getOFString(filesetID,0);
  if (EC_Normal == result) if (storageMediaFileSetUID.getLength() == 0) filesetUID.clear(); else result = storageMediaFileSetUID.getOFString(filesetUID,0);
  return result;
}

void DVPSReferencedSeries::setLog(OFConsole *stream, OFBool verbMode, OFBool dbgMode)
{
  if (stream) logstream = stream; else logstream = &ofConsole;
  verboseMode = verbMode;
  debugMode = dbgMode;
  referencedImageList.setLog(logstream, verbMode, dbgMode);
}

/*
 *  $Log: dvpsrs.cc,v $
 *  Revision 1.13  2005/12/08 15:46:45  meichel
 *  Changed include path schema for all DCMTK header files
 *
 *  Revision 1.12  2003/06/04 12:30:28  meichel
 *  Added various includes needed by MSVC5 with STL
 *
 *  Revision 1.11  2001/09/26 15:36:31  meichel
 *  Adapted dcmpstat to class OFCondition
 *
 *  Revision 1.10  2001/06/01 15:50:36  meichel
 *  Updated copyright header
 *
 *  Revision 1.9  2000/06/02 16:01:06  meichel
 *  Adapted all dcmpstat classes to use OFConsole for log and error output
 *
 *  Revision 1.8  2000/05/31 13:02:38  meichel
 *  Moved dcmpstat macros and constants into a common header file
 *
 *  Revision 1.7  2000/03/08 16:29:09  meichel
 *  Updated copyright header.
 *
 *  Revision 1.6  2000/03/03 14:14:04  meichel
 *  Implemented library support for redirecting error messages into memory
 *    instead of printing them to stdout/stderr for GUI applications.
 *
 *  Revision 1.5  1999/09/24 11:13:52  meichel
 *  Fixed problems related to DcmElement::getOFString on empty strings.
 *
 *  Revision 1.4  1999/07/22 16:40:01  meichel
 *  Adapted dcmpstat data structures and API to supplement 33 letter ballot text.
 *
 *  Revision 1.3  1999/01/15 17:32:57  meichel
 *  added methods to DVPresentationState allowing to access the image
 *    references in the presentation state.  Also added methods allowing to
 *    get the width and height of the attached image.
 *
 *  Revision 1.2  1998/12/14 16:10:47  meichel
 *  Implemented Presentation State interface for graphic layers,
 *    text and graphic annotations, presentation LUTs.
 *
 *  Revision 1.1  1998/11/27 14:50:46  meichel
 *  Initial Release.
 *
 *
 */

