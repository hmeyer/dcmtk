/*
 *
 *  Copyright (C) 2002-2005, OFFIS
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
 *  Module:  dcmimage
 *
 *  Author:  Marco Eichelberg
 *
 *  Purpose: class DcmQuantHistogramItemList
 *
 *  Last Update:      $Author: meichel $
 *  Update Date:      $Date: 2005/12/08 15:42:31 $
 *  CVS/RCS Revision: $Revision: 1.3 $
 *  Status:           $State: Exp $
 *
 *  CVS/RCS Log at end of file
 *
 */

#include "dcmtk/config/osconfig.h"
#include "dcmtk/dcmimage/diqthitl.h"


DcmQuantHistogramItemList::DcmQuantHistogramItemList()
: list_()
, first(list_.end())
, last(list_.end())
{
}


DcmQuantHistogramItemList::~DcmQuantHistogramItemList()
{
  first = list_.begin();
  while (first != last)
  {
    delete *first;
    first = list_.erase(first);
  }
}


void DcmQuantHistogramItemList::moveto(DcmQuantHistogramItemPointer *array, unsigned long& counter, unsigned long numcolors)
{
  first = list_.begin();
  while ((first != last) && (counter < numcolors))
  {
    array[counter++] = *first;
    first = list_.erase(first);
  }
}


/*
 *
 * CVS/RCS Log:
 * $Log: diqthitl.cc,v $
 * Revision 1.3  2005/12/08 15:42:31  meichel
 * Changed include path schema for all DCMTK header files
 *
 * Revision 1.2  2003/12/17 16:34:14  joergr
 * Renamed parameters/variables "list" to avoid name clash with STL class.
 *
 * Revision 1.1  2002/01/25 13:32:11  meichel
 * Initial release of new color quantization classes and
 *   the dcmquant tool in module dcmimage.
 *
 *
 */
