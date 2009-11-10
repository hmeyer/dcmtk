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
 *    classes: DVPSAnnotationContent
 *
 *  Last Update:      $Author: meichel $
 *  Update Date:      $Date: 2005/12/08 15:46:16 $
 *  CVS/RCS Revision: $Revision: 1.9 $
 *  Status:           $State: Exp $
 *
 *  CVS/RCS Log at end of file
 *
 */

#include "dcmtk/config/osconfig.h"    /* make sure OS specific configuration is included first */
#include "dcmtk/ofstd/ofstring.h"
#include "dcmtk/dcmpstat/dvpsab.h"
#include "dcmtk/dcmpstat/dvpsdef.h"     /* for constants and macros */


/* --------------- class DVPSAnnotationContent --------------- */

DVPSAnnotationContent::DVPSAnnotationContent()
: sOPInstanceUID(DCM_SOPInstanceUID)
, annotationPosition(DCM_AnnotationPosition)
, textString(DCM_TextString)
, logstream(&ofConsole)
, verboseMode(OFFalse)
, debugMode(OFFalse)
{
}

DVPSAnnotationContent::DVPSAnnotationContent(const DVPSAnnotationContent& copy)
: sOPInstanceUID(copy.sOPInstanceUID)
, annotationPosition(copy.annotationPosition)
, textString(copy.textString)
, logstream(copy.logstream)
, verboseMode(copy.verboseMode)
, debugMode(copy.debugMode)
{
}

DVPSAnnotationContent::~DVPSAnnotationContent()
{
}

void DVPSAnnotationContent::clear()
{
  sOPInstanceUID.clear();
  annotationPosition.clear();
  textString.clear();
  return;
}

OFCondition DVPSAnnotationContent::setContent(
    const char *instanceuid,
    const char *text,
    Uint16 position)
{
  OFCondition result = EC_Normal;
  if (instanceuid && text)
  {
    clear();
    result = sOPInstanceUID.putString(instanceuid);
    if (EC_Normal == result) result = textString.putString(text);
    if (EC_Normal == result) result = annotationPosition.putUint16(position,0);
  } else result = EC_IllegalCall;
  return result;
}

OFCondition DVPSAnnotationContent::read(DcmItem &dset)
{
  OFCondition result = EC_Normal;
  DcmStack stack;
  
  READ_FROM_DATASET(DcmUniqueIdentifier, sOPInstanceUID)
  READ_FROM_DATASET(DcmUnsignedShort, annotationPosition)
  READ_FROM_DATASET(DcmLongString, textString)
  
  /* Now perform basic sanity checks */

  if (result==EC_Normal)
  {
    if ((sOPInstanceUID.getLength() == 0)||(sOPInstanceUID.getVM() != 1))
    {
        result=EC_TagNotFound;
        if (verboseMode)
        {
          logstream->lockCerr() << "Error: SOPInstanceUID missing or incorrect in Stored Print Annotation" << endl;
          logstream->unlockCerr();
        }
    }
    if ((annotationPosition.getLength() == 0)||(annotationPosition.getVM() != 1))
    {
        result=EC_TagNotFound;
        if (verboseMode)
        {
          logstream->lockCerr() << "Error: AnnotationPosition missing or incorrect in Stored Print Annotation" << endl;
          logstream->unlockCerr();
        }
    }
    if ((textString.getLength() == 0)||(textString.getVM() != 1))
    {
        result=EC_TagNotFound;
        if (verboseMode)
        {
          logstream->lockCerr() << "Error: TextString missing or incorrect in Stored Print Annotation" << endl;
          logstream->unlockCerr();
        }
    }
  }

  return result;
}

OFCondition DVPSAnnotationContent::write(DcmItem &dset)
{
  OFCondition result = EC_Normal;
  DcmElement *delem=NULL;

  /* before writing anything, check that we are able to write a correct item */
  if (sOPInstanceUID.getLength() == 0)
  {
    result=EC_TagNotFound;
    if (verboseMode)
    {
      logstream->lockCerr() << "Error: cannot write Stored Print Annotation: SOPInstanceUID empty" << endl;
      logstream->unlockCerr();
    }
  }
  if (annotationPosition.getLength() == 0)
  {
    result=EC_TagNotFound;
    if (verboseMode)
    {
      logstream->lockCerr() << "Error: cannot write Stored Print Annotation: AnnotationPosition empty" << endl;
      logstream->unlockCerr();
    }
  }
  if (textString.getLength() == 0)
  {
    result=EC_TagNotFound;
    if (verboseMode)
    {
      logstream->lockCerr() << "Error: cannot write Stored Print Annotation: TextString empty" << endl;
      logstream->unlockCerr();
    }
  }

  ADD_TO_DATASET(DcmUniqueIdentifier, sOPInstanceUID)
  ADD_TO_DATASET(DcmUnsignedShort, annotationPosition)
  ADD_TO_DATASET(DcmLongString, textString)

  return result;
}

OFCondition DVPSAnnotationContent::prepareBasicAnnotationBox(DcmItem &dset)
{
  OFCondition result = EC_Normal;
  DcmElement *delem=NULL;

  ADD_TO_DATASET(DcmUnsignedShort, annotationPosition)
  ADD_TO_DATASET(DcmLongString, textString)
  return result;
}

OFCondition DVPSAnnotationContent::setSOPInstanceUID(const char *value)
{
  if ((value==NULL)||(strlen(value)==0)) 
  {
  	sOPInstanceUID.clear();
  	return EC_Normal;
  }
  return sOPInstanceUID.putString(value);
}

const char *DVPSAnnotationContent::getSOPInstanceUID()
{
  char *c = NULL;
  if (EC_Normal == sOPInstanceUID.getString(c)) return c; else return NULL;
}

void DVPSAnnotationContent::setLog(OFConsole *stream, OFBool verbMode, OFBool dbgMode)
{
  if (stream) logstream = stream; else logstream = &ofConsole;
  verboseMode = verbMode;
  debugMode = dbgMode;
}

/*
 *  $Log: dvpsab.cc,v $
 *  Revision 1.9  2005/12/08 15:46:16  meichel
 *  Changed include path schema for all DCMTK header files
 *
 *  Revision 1.8  2001/09/26 15:36:22  meichel
 *  Adapted dcmpstat to class OFCondition
 *
 *  Revision 1.7  2001/06/07 14:31:34  joergr
 *  Removed unused variable (reported by gcc 2.5.8 on NeXTSTEP).
 *
 *  Revision 1.6  2001/06/01 15:50:26  meichel
 *  Updated copyright header
 *
 *  Revision 1.5  2000/06/02 16:00:56  meichel
 *  Adapted all dcmpstat classes to use OFConsole for log and error output
 *
 *  Revision 1.4  2000/05/31 13:02:35  meichel
 *  Moved dcmpstat macros and constants into a common header file
 *
 *  Revision 1.3  2000/03/08 16:29:01  meichel
 *  Updated copyright header.
 *
 *  Revision 1.2  2000/03/03 14:13:57  meichel
 *  Implemented library support for redirecting error messages into memory
 *    instead of printing them to stdout/stderr for GUI applications.
 *
 *  Revision 1.1  1999/10/19 14:48:27  meichel
 *  added support for the Basic Annotation Box SOP Class
 *    as well as access methods for Max Density and Min Density.
 *
 *
 */

