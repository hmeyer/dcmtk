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
 *    classes: DVPSGraphicObject_PList
 *
 *  Last Update:      $Author: meichel $
 *  Update Date:      $Date: 2005/12/08 15:46:31 $
 *  CVS/RCS Revision: $Revision: 1.12 $
 *  Status:           $State: Exp $
 *
 *  CVS/RCS Log at end of file
 *
 */

#include "dcmtk/config/osconfig.h"    /* make sure OS specific configuration is included first */
#include "dcmtk/dcmpstat/dvpsgrl.h"
#include "dcmtk/dcmpstat/dvpsgr.h"      /* for DVPSGraphicObject */


DVPSGraphicObject_PList::DVPSGraphicObject_PList()
: list_()
, logstream(&ofConsole)
, verboseMode(OFFalse)
, debugMode(OFFalse)
{
}

DVPSGraphicObject_PList::DVPSGraphicObject_PList(const DVPSGraphicObject_PList &arg)
: list_()
, logstream(arg.logstream)
, verboseMode(arg.verboseMode)
, debugMode(arg.debugMode)
{
  OFListConstIterator(DVPSGraphicObject *) first = arg.list_.begin();
  OFListConstIterator(DVPSGraphicObject *) last = arg.list_.end();
  while (first != last)
  {
    list_.push_back((*first)->clone());
    ++first;
  }
}

DVPSGraphicObject_PList::~DVPSGraphicObject_PList()
{
  clear();
}

void DVPSGraphicObject_PList::clear()
{
  OFListIterator(DVPSGraphicObject *) first = list_.begin();
  OFListIterator(DVPSGraphicObject *) last = list_.end();
  while (first != last)
  {
    delete (*first);
    first = list_.erase(first);
  }
}

OFCondition DVPSGraphicObject_PList::read(DcmItem &dset)
{
  OFCondition result = EC_Normal;
  DcmStack stack;
  DVPSGraphicObject *newObject = NULL;
  DcmSequenceOfItems *dseq=NULL;
  DcmItem *ditem=NULL;

  if (EC_Normal == dset.search(DCM_GraphicObjectSequence, stack, ESM_fromHere, OFFalse))
  {
    dseq=(DcmSequenceOfItems *)stack.top();
    if (dseq)
    {
      unsigned long numItems = dseq->card();
      for (unsigned int i=0; i<numItems; i++)
      {
        ditem = dseq->getItem(i);
        newObject = new DVPSGraphicObject();
        if (newObject && ditem)
        {
          newObject->setLog(logstream, verboseMode, debugMode);
          result = newObject->read(*ditem);
          list_.push_back(newObject);
        } else result = EC_MemoryExhausted;
      }
    }
  }

  return result;
}

OFCondition DVPSGraphicObject_PList::write(DcmItem &dset)
{
  if (size()==0) return EC_Normal; // don't write empty Sequence

  OFCondition result = EC_Normal;
  DcmSequenceOfItems *dseq=NULL;
  DcmItem *ditem=NULL;

  dseq = new DcmSequenceOfItems(DCM_GraphicObjectSequence);
  if (dseq)
  {
    OFListIterator(DVPSGraphicObject *) first = list_.begin();
    OFListIterator(DVPSGraphicObject *) last = list_.end();
    while (first != last)
    {
      if (result==EC_Normal)
      {
        ditem = new DcmItem();
        if (ditem)
        {
          result = (*first)->write(*ditem);
          if (result==EC_Normal) dseq->insert(ditem); else delete ditem;
        } else result = EC_MemoryExhausted;
      }
      ++first;
    }
    if (result==EC_Normal) dset.insert(dseq, OFTrue /*replaceOld*/); else delete dseq;
  } else result = EC_MemoryExhausted;
  return result;
}


DVPSGraphicObject *DVPSGraphicObject_PList::getGraphicObject(size_t idx)
{
  OFListIterator(DVPSGraphicObject *) first = list_.begin();
  OFListIterator(DVPSGraphicObject *) last = list_.end();
  while (first != last)
  {
    if (idx==0) return *first;
    idx--;
    ++first;
  }
  return NULL;
}

void DVPSGraphicObject_PList::addGraphicObject(DVPSGraphicObject *graphic)
{
  if (graphic) list_.push_back(graphic);
}

DVPSGraphicObject *DVPSGraphicObject_PList::removeGraphicObject(size_t idx)
{
  OFListIterator(DVPSGraphicObject *) first = list_.begin();
  OFListIterator(DVPSGraphicObject *) last = list_.end();
  while (first != last)
  {
    if (idx==0)
    {
      DVPSGraphicObject *result = *first;
      list_.erase(first);
      return result;
    }
    idx--;
    ++first;
  }
  return NULL;
}

void DVPSGraphicObject_PList::setLog(OFConsole *stream, OFBool verbMode, OFBool dbgMode)
{
  if (stream) logstream = stream; else logstream = &ofConsole;
  verboseMode = verbMode;
  debugMode = dbgMode;
  OFListIterator(DVPSGraphicObject *) first = list_.begin();
  OFListIterator(DVPSGraphicObject *) last = list_.end();
  while (first != last)
  {
    (*first)->setLog(logstream, verbMode, dbgMode);
    ++first;
  }
}

/*
 *  $Log: dvpsgrl.cc,v $
 *  Revision 1.12  2005/12/08 15:46:31  meichel
 *  Changed include path schema for all DCMTK header files
 *
 *  Revision 1.11  2004/02/04 15:57:49  joergr
 *  Removed acknowledgements with e-mail addresses from CVS log.
 *
 *  Revision 1.10  2003/09/05 08:37:46  meichel
 *  Fixed minor issue that caused certain error messages during the
 *    parse process on a GSPS object to be "swallowed".
 *
 *  Revision 1.9  2003/06/12 18:23:11  joergr
 *  Modified code to use const_iterators where appropriate (required for STL).
 *
 *  Revision 1.8  2003/06/04 10:18:07  meichel
 *  Replaced private inheritance from template with aggregation
 *
 *  Revision 1.7  2001/11/28 13:56:55  joergr
 *  Check return value of DcmItem::insert() statements where appropriate to
 *  avoid memory leaks when insert procedure fails.
 *
 *  Revision 1.6  2001/09/26 15:36:27  meichel
 *  Adapted dcmpstat to class OFCondition
 *
 *  Revision 1.5  2001/06/01 15:50:32  meichel
 *  Updated copyright header
 *
 *  Revision 1.4  2000/06/02 16:01:01  meichel
 *  Adapted all dcmpstat classes to use OFConsole for log and error output
 *
 *  Revision 1.3  2000/03/08 16:29:06  meichel
 *  Updated copyright header.
 *
 *  Revision 1.2  1998/12/14 16:10:44  meichel
 *  Implemented Presentation State interface for graphic layers,
 *    text and graphic annotations, presentation LUTs.
 *
 *  Revision 1.1  1998/11/27 14:50:43  meichel
 *  Initial Release.
 *
 *
 */
