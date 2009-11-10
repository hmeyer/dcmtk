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
 *    classes: DVPSSoftcopyVOI
 *
 *  Last Update:      $Author: meichel $
 *  Update Date:      $Date: 2005/12/08 15:46:49 $
 *  CVS/RCS Revision: $Revision: 1.12 $
 *  Status:           $State: Exp $
 *
 *  CVS/RCS Log at end of file
 *
 */

#include "dcmtk/config/osconfig.h"    /* make sure OS specific configuration is included first */
#include "dcmtk/ofstd/ofstring.h"
#include "dcmtk/dcmpstat/dvpssv.h"
#include "dcmtk/dcmpstat/dvpsri.h"      /* for DVPSReferencedImage */
#include "dcmtk/dcmpstat/dvpsrsl.h"     /* DVPSReferencedSeries_PList */
#include "dcmtk/dcmpstat/dvpsdef.h"     /* for constants and macros */
#include "dcmtk/ofstd/ofstd.h"
#include "dcmtk/dcmpstat/dvpsrs.h"      /* for DVPSReferencedSeries, needed by MSVC5 with STL */

/* --------------- class DVPSSoftcopyVOI --------------- */

DVPSSoftcopyVOI::DVPSSoftcopyVOI()
: referencedImageList()
, useLUT(OFFalse)
, voiLUTDescriptor(DCM_LUTDescriptor)
, voiLUTExplanation(DCM_LUTExplanation)
, voiLUTData(DCM_LUTData)
, windowCenter(DCM_WindowCenter)
, windowWidth(DCM_WindowWidth)
, windowCenterWidthExplanation(DCM_WindowCenterWidthExplanation)
, logstream(&ofConsole)
, verboseMode(OFFalse)
, debugMode(OFFalse)
{
}

DVPSSoftcopyVOI::DVPSSoftcopyVOI(const DVPSSoftcopyVOI& copy)
: referencedImageList(copy.referencedImageList)
, useLUT(copy.useLUT)
, voiLUTDescriptor(copy.voiLUTDescriptor)
, voiLUTExplanation(copy.voiLUTExplanation)
, voiLUTData(copy.voiLUTData)
, windowCenter(copy.windowCenter)
, windowWidth(copy.windowWidth)
, windowCenterWidthExplanation(copy.windowCenterWidthExplanation)
, logstream(copy.logstream)
, verboseMode(copy.verboseMode)
, debugMode(copy.debugMode)
{
}

DVPSSoftcopyVOI::~DVPSSoftcopyVOI()
{
}

OFCondition DVPSSoftcopyVOI::read(DcmItem &dset)
{
  OFCondition result = EC_Normal;
  DcmStack stack;
  DcmSequenceOfItems *seq;
  DcmItem *item;
  
  READ_FROM_DATASET(DcmDecimalString, windowCenter)
  READ_FROM_DATASET(DcmDecimalString, windowWidth)
  READ_FROM_DATASET(DcmLongString, windowCenterWidthExplanation)

  /* read VOI LUT Sequence */
  if (result==EC_Normal)
  {
    stack.clear();
    if (EC_Normal == dset.search(DCM_VOILUTSequence, stack, ESM_fromHere, OFFalse))
    {
      seq=(DcmSequenceOfItems *)stack.top();
      if (seq->card() ==1)
      {
         item = seq->getItem(0);
         stack.clear();
         if (EC_Normal == item->search((DcmTagKey &)voiLUTDescriptor.getTag(), 
           stack, ESM_fromHere, OFFalse))
         {
           voiLUTDescriptor = *((DcmUnsignedShort *)(stack.top()));
         }
         stack.clear();
         if (EC_Normal == item->search((DcmTagKey &)voiLUTExplanation.getTag(), 
           stack, ESM_fromHere, OFFalse))
         {
           voiLUTExplanation = *((DcmLongString *)(stack.top()));
         }
         stack.clear();
         if (EC_Normal == item->search((DcmTagKey &)voiLUTData.getTag(), 
           stack, ESM_fromHere, OFFalse))
         {
           voiLUTData = *((DcmUnsignedShort *)(stack.top()));
         }
      } else {
        result=EC_TagNotFound;
        if (verboseMode)
        {
          logstream->lockCerr() << "Error: VOI LUT SQ does not have exactly one item in presentation state" << endl;
          logstream->unlockCerr();
        }
      } 
    }
  }

  if (result==EC_Normal) result = referencedImageList.read(dset);

  /* Now perform basic sanity checks */

  if (result==EC_Normal)
  {
    if (windowCenter.getLength() > 0)
    {
      useLUT = OFFalse;
      
      if (windowWidth.getLength() == 0)
      {
        result=EC_IllegalCall;
        if (verboseMode)
        {
          logstream->lockCerr() << "Error: windowCenter present but windowWidth absent or empty in presentation state" << endl;
          logstream->unlockCerr();
        }
      }
      else if (windowWidth.getVM() != 1)
      {
        result=EC_IllegalCall;
        if (verboseMode)
        {
          logstream->lockCerr() << "Error: windowCenter present but windowWidth VM != 1 in presentation state" << endl;
          logstream->unlockCerr();
        }
      }
      if (windowCenter.getVM() != 1)
      {
        result=EC_IllegalCall;
        if (verboseMode)
        {
          logstream->lockCerr() << "Error: windowCenter present but VM != 1 in presentation state" << endl;
          logstream->unlockCerr();
        }
      }
    } else useLUT = OFTrue;
    
    if (voiLUTData.getLength() > 0)
    {
    	
      if (! useLUT)
      {
        result=EC_IllegalCall;
        if (verboseMode)
        {
          logstream->lockCerr() << "Error: both VOI window and LUT present in presentation state" << endl;
          logstream->unlockCerr();
        }
      }

      if (voiLUTDescriptor.getLength() == 0)
      {
        result=EC_IllegalCall;
        if (verboseMode)
        {
          logstream->lockCerr() << "Error: voiLUTData present but voiLUTDescriptor absent or empty in presentation state" << endl;
          logstream->unlockCerr();
        }
      }
      else if (voiLUTDescriptor.getVM() != 3)
      {
        result=EC_IllegalCall;
        if (verboseMode)
        {
          logstream->lockCerr() << "Error: voiLUTData present but voiLUTDescriptor VM != 3 in presentation state" << endl;
          logstream->unlockCerr();
        }
      }
    } 
    else if (useLUT)
    {
        result=EC_IllegalCall;
        if (verboseMode)
        {
          logstream->lockCerr() << "Error: neither VOI window nor LUT present in presentation state" << endl;
          logstream->unlockCerr();
        }
    }
  }
  return result;
}

OFCondition DVPSSoftcopyVOI::write(DcmItem &dset)
{
  OFCondition result = EC_Normal;
  DcmElement *delem=NULL;
  DcmSequenceOfItems *dseq=NULL;
  DcmItem *ditem=NULL;

  if (useLUT)
  {
    ditem = new DcmItem();
    if (ditem)
    {
      dseq = new DcmSequenceOfItems(DCM_VOILUTSequence);
      if (dseq)
      {
        delem = new DcmUnsignedShort(voiLUTDescriptor);
        if (delem) ditem->insert(delem, OFTrue /*replaceOld*/); else result=EC_MemoryExhausted;
        delem = new DcmUnsignedShort(voiLUTData);
        if (delem) ditem->insert(delem, OFTrue /*replaceOld*/); else result=EC_MemoryExhausted;
        if (voiLUTExplanation.getLength() >0)
        {
          delem = new DcmLongString(voiLUTExplanation);
          if (delem) ditem->insert(delem, OFTrue /*replaceOld*/); else result=EC_MemoryExhausted;
        }
        if (result==EC_Normal)
        {
          dseq->insert(ditem);
          dset.insert(dseq, OFTrue /*replaceOld*/);
        } else {
          // out of memory during creation of sequence contents.
          delete dseq;
          delete ditem;
          result = EC_MemoryExhausted;
        }
      } else {
        // could allocate item but not sequence. Bail out.
        delete ditem;
        result = EC_MemoryExhausted;
      }
    }
    else result = EC_MemoryExhausted;
  }
  else
  {
    ADD_TO_DATASET(DcmDecimalString, windowCenter)
    ADD_TO_DATASET(DcmDecimalString, windowWidth)
    if (windowCenterWidthExplanation.getLength() > 0) { ADD_TO_DATASET(DcmLongString, windowCenterWidthExplanation) }
  }

  if ((result == EC_Normal)&&(referencedImageList.size() >0)) result = referencedImageList.write(dset);
  return result;
}

OFBool DVPSSoftcopyVOI::isApplicable(const char *instanceUID, unsigned long frame)
{
  return referencedImageList.isApplicable(instanceUID, frame);
}

OFBool DVPSSoftcopyVOI::matchesApplicability(const char *instanceUID, unsigned long frame, DVPSObjectApplicability applicability)
{
  return referencedImageList.matchesApplicability(instanceUID, frame, applicability);
}

void DVPSSoftcopyVOI::removeImageReference(
    DVPSReferencedSeries_PList& allReferences,
    const char *instanceUID,
    unsigned long frame, 
    unsigned long numberOfFrames, 
    DVPSObjectApplicability applicability)
{
  referencedImageList.removeImageReference(allReferences, instanceUID, frame, numberOfFrames, applicability);
  return;
}

OFCondition DVPSSoftcopyVOI::addImageReference(
    const char *sopclassUID,
    const char *instanceUID, 
    unsigned long frame,
    DVPSObjectApplicability applicability)
{
  return referencedImageList.addImageReference(sopclassUID, instanceUID, frame, applicability);
}

const char *DVPSSoftcopyVOI::getCurrentVOIDescription()
{
  char *c=NULL;
  if (useLUT)
  {
    if (EC_Normal == voiLUTExplanation.getString(c)) return c;
  } 
  else
  {
    if (EC_Normal == windowCenterWidthExplanation.getString(c)) return c;
  } 
  return NULL;
}

OFCondition DVPSSoftcopyVOI::getCurrentWindowWidth(double &w)
{
  OFCondition result = EC_IllegalCall;
  if (!useLUT)
  {
    Float64 temp=0.0;
    result = windowWidth.getFloat64(temp,0);
    if (EC_Normal==result) w = (double)temp;
  }
  return result;
}
  
OFCondition DVPSSoftcopyVOI::getCurrentWindowCenter(double &c)
{
  OFCondition result = EC_IllegalCall;
  if (!useLUT)
  {
    Float64 temp=0.0;
    result = windowCenter.getFloat64(temp,0);
    if (EC_Normal==result) c = (double)temp;
  }
  return result;
}

OFCondition DVPSSoftcopyVOI::setVOIWindow(double wCenter, double wWidth, const char *description)
{
  if (wWidth < 1.0) 
  {
    if (verboseMode)
    {
      logstream->lockCerr() << "Error: Window Width < 1 not allowed." << endl;
      logstream->unlockCerr();
    }
    return EC_IllegalCall;
  }
  DcmDecimalString wc(DCM_WindowCenter);
  DcmDecimalString ww(DCM_WindowWidth);
  DcmLongString expl(DCM_WindowCenterWidthExplanation);
  char buf[80];

  OFStandard::ftoa(buf, sizeof(buf), wCenter, OFStandard::ftoa_uppercase);
  OFCondition result = wc.putString(buf);
  OFStandard::ftoa(buf, sizeof(buf), wWidth, OFStandard::ftoa_uppercase);
  if (EC_Normal == result) result = ww.putString(buf);
  if ((EC_Normal == result)&&(description)) result = expl.putString(description);
  if (EC_Normal == result)
  {
    // everything worked fine, now copy.
    windowCenter = wc;
    windowWidth = ww;
    windowCenterWidthExplanation = expl;
    voiLUTDescriptor.clear();
    voiLUTData.clear();
    voiLUTExplanation.clear();
    useLUT = OFFalse;
  }
  return result;
}

OFCondition DVPSSoftcopyVOI::setVOILUT( 
    DcmUnsignedShort& lutDescriptor,
    DcmUnsignedShort& lutData,
    DcmLongString& lutExplanation)
{
  if (lutData.getLength() == 0) return EC_IllegalCall;
  if (lutDescriptor.getVM() != 3) return EC_IllegalCall;
  voiLUTDescriptor = lutDescriptor;
  voiLUTData = lutData;
  voiLUTExplanation = lutExplanation;
  windowCenter.clear();
  windowWidth.clear();
  windowCenterWidthExplanation.clear();
  useLUT = OFTrue;
  return EC_Normal;
}

void DVPSSoftcopyVOI::setLog(OFConsole *stream, OFBool verbMode, OFBool dbgMode)
{
  if (stream) logstream = stream; else logstream = &ofConsole;
  verboseMode = verbMode;
  debugMode = dbgMode;
}

/*
 *  $Log: dvpssv.cc,v $
 *  Revision 1.12  2005/12/08 15:46:49  meichel
 *  Changed include path schema for all DCMTK header files
 *
 *  Revision 1.11  2003/06/04 12:30:29  meichel
 *  Added various includes needed by MSVC5 with STL
 *
 *  Revision 1.10  2002/12/04 10:41:37  meichel
 *  Changed toolkit to use OFStandard::ftoa instead of sprintf for all
 *    double to string conversions that are supposed to be locale independent
 *
 *  Revision 1.9  2001/11/28 13:57:03  joergr
 *  Check return value of DcmItem::insert() statements where appropriate to
 *  avoid memory leaks when insert procedure fails.
 *
 *  Revision 1.8  2001/09/26 15:36:33  meichel
 *  Adapted dcmpstat to class OFCondition
 *
 *  Revision 1.7  2001/06/01 15:50:39  meichel
 *  Updated copyright header
 *
 *  Revision 1.6  2000/06/02 16:01:07  meichel
 *  Adapted all dcmpstat classes to use OFConsole for log and error output
 *
 *  Revision 1.5  2000/05/31 13:02:39  meichel
 *  Moved dcmpstat macros and constants into a common header file
 *
 *  Revision 1.4  2000/03/08 16:29:11  meichel
 *  Updated copyright header.
 *
 *  Revision 1.3  2000/03/03 14:14:06  meichel
 *  Implemented library support for redirecting error messages into memory
 *    instead of printing them to stdout/stderr for GUI applications.
 *
 *  Revision 1.2  1999/10/05 12:10:59  joergr
 *  Fixed bug in DVPSSoftcopyVOI::setVOIWindow(). Window width = 1 was
 *  rejected.
 *
 *  Revision 1.1  1999/07/22 16:40:03  meichel
 *  Adapted dcmpstat data structures and API to supplement 33 letter ballot text.
 *
 *
 */

