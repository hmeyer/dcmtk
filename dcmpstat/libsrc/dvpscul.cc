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
 *    classes: DVPSCurve_PList
 *
 *  Last Update:      $Author: meichel $
 *  Update Date:      $Date: 2005/12/08 15:46:22 $
 *  CVS/RCS Revision: $Revision: 1.10 $
 *  Status:           $State: Exp $
 *
 *  CVS/RCS Log at end of file
 *
 */

#include "dcmtk/config/osconfig.h"    /* make sure OS specific configuration is included first */
#include "dcmtk/dcmpstat/dvpscul.h"
#include "dcmtk/dcmpstat/dvpscu.h"      /* for DVPSCurve */


DVPSCurve_PList::DVPSCurve_PList()
: list_()
, logstream(&ofConsole)
, verboseMode(OFFalse)
, debugMode(OFFalse)
{
}

DVPSCurve_PList::DVPSCurve_PList(const DVPSCurve_PList &arg)
: list_()
, logstream(arg.logstream)
, verboseMode(arg.verboseMode)
, debugMode(arg.debugMode)
{
  OFListConstIterator(DVPSCurve *) first = arg.list_.begin();
  OFListConstIterator(DVPSCurve *) last = arg.list_.end();
  while (first != last)
  {
    list_.push_back((*first)->clone());
    ++first;
  }
}

DVPSCurve_PList::~DVPSCurve_PList()
{
  clear();
}

void DVPSCurve_PList::clear()
{
  OFListIterator(DVPSCurve *) first = list_.begin();
  OFListIterator(DVPSCurve *) last = list_.end();
  while (first != last)
  {
    delete (*first);
    first = list_.erase(first);
  }
}

OFCondition DVPSCurve_PList::read(DcmItem &dset)
{
  OFCondition result = EC_Normal;
  DVPSCurve *newCurve = NULL;

  for (Uint8 i=0; i<16; i+=2)
  {
    newCurve = new DVPSCurve();
    if (newCurve)
    {
      newCurve->setLog(logstream, verboseMode, debugMode);
      result = newCurve->read(dset,i);
      if (result==EC_Normal) list_.push_back(newCurve); else delete newCurve;
    } else return EC_MemoryExhausted;
  }
  return EC_Normal;
}


OFBool DVPSCurve_PList::haveCurveGroup(Uint16 group)
{
  if (getCurveGroup(group)) return OFTrue; else return OFFalse;
}


DVPSCurve *DVPSCurve_PList::getCurveGroup(Uint16 group)
{
  Uint8  lowergroup = (Uint8)(group & 0x00FF);
  OFListIterator(DVPSCurve *) first = list_.begin();
  OFListIterator(DVPSCurve *) last = list_.end();
  while (first != last)
  {
    if ((*first)->getCurveGroup() == lowergroup) return *first;
    ++first;
  }
  return NULL;
}

DVPSCurve *DVPSCurve_PList::getCurve(size_t idx)
{
  OFListIterator(DVPSCurve *) first = list_.begin();
  OFListIterator(DVPSCurve *) last = list_.end();
  while (first != last)
  {
    if (idx==0) return *first;
    idx--;
    ++first;
  }
  return NULL;
}

void DVPSCurve_PList::setLog(OFConsole *stream, OFBool verbMode, OFBool dbgMode)
{
  if (stream) logstream = stream; else logstream = &ofConsole;
  verboseMode = verbMode;
  debugMode = dbgMode;
  OFListIterator(DVPSCurve *) first = list_.begin();
  OFListIterator(DVPSCurve *) last = list_.end();
  while (first != last)
  {
    (*first)->setLog(logstream, verbMode, dbgMode);
    ++first;
  }
}

/*
 *  $Log: dvpscul.cc,v $
 *  Revision 1.10  2005/12/08 15:46:22  meichel
 *  Changed include path schema for all DCMTK header files
 *
 *  Revision 1.9  2004/02/04 15:57:49  joergr
 *  Removed acknowledgements with e-mail addresses from CVS log.
 *
 *  Revision 1.8  2003/09/05 08:37:46  meichel
 *  Fixed minor issue that caused certain error messages during the
 *    parse process on a GSPS object to be "swallowed".
 *
 *  Revision 1.7  2003/06/12 18:23:11  joergr
 *  Modified code to use const_iterators where appropriate (required for STL).
 *
 *  Revision 1.6  2003/06/04 10:18:07  meichel
 *  Replaced private inheritance from template with aggregation
 *
 *  Revision 1.5  2001/09/26 15:36:24  meichel
 *  Adapted dcmpstat to class OFCondition
 *
 *  Revision 1.4  2001/06/01 15:50:29  meichel
 *  Updated copyright header
 *
 *  Revision 1.3  2000/06/02 16:00:58  meichel
 *  Adapted all dcmpstat classes to use OFConsole for log and error output
 *
 *  Revision 1.2  2000/03/08 16:29:03  meichel
 *  Updated copyright header.
 *
 *  Revision 1.1  1998/12/22 17:57:15  meichel
 *  Implemented Presentation State interface for overlays,
 *    VOI LUTs, VOI windows, curves. Added test program that
 *    allows to add curve data to DICOM images.
 *
 *
 */
