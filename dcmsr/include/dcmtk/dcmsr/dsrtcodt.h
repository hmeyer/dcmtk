/*
 *
 *  Copyright (C) 2000-2005, OFFIS
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
 *  Module: dcmsr
 *
 *  Author: Joerg Riesmeier
 *
 *  Purpose:
 *    classes: DSRReferencedDatetimeList
 *
 *  Last Update:      $Author: meichel $
 *  Update Date:      $Date: 2005/12/08 16:05:20 $
 *  CVS/RCS Revision: $Revision: 1.8 $
 *  Status:           $State: Exp $
 *
 *  CVS/RCS Log at end of file
 *
 */


#ifndef DSRTCODT_H
#define DSRTCODT_H

#include "dcmtk/config/osconfig.h"   /* make sure OS specific configuration is included first */

#include "dcmtk/dcmsr/dsrtypes.h"
#include "dcmtk/dcmsr/dsrtlist.h"

#include "dcmtk/ofstd/ofstring.h"


/*---------------------*
 *  class declaration  *
 *---------------------*/

/** Class for referenced datetime list
 */
class DSRReferencedDatetimeList
  : public DSRListOfItems<OFString>
{

  public:

    /** default constructor
     */
    DSRReferencedDatetimeList();

    /** copy constructor
     ** @param  lst  list to be copied
     */
    DSRReferencedDatetimeList(const DSRReferencedDatetimeList &lst);

    /** destructor
     */
    virtual ~DSRReferencedDatetimeList();

    /** assignment operator
     ** @param  lst  list to be copied
     ** @return reference to this list after 'lst' has been copied
     */
    DSRReferencedDatetimeList &operator=(const DSRReferencedDatetimeList &lst);

    /** print list of referenced datetime.
     *  The output of a typical list looks like this: 20001010120000, ...
     ** @param  stream     output stream to which the list should be printed
     *  @param  flags      flag used to customize the output (see DSRTypes::PF_xxx)
     *  @param  separator  character specifying the separator between the list items
     ** @return status, EC_Normal if successful, an error code otherwise
     */
    OFCondition print(ostream &stream,
                      const size_t flags = 0,
                      const char separator = ',') const;

    /** read list of referenced datetime
     ** @param  dataset    DICOM dataset from which the list should be read
     *  @param  logStream  pointer to error/warning output stream (output disabled if NULL)
     ** @return status, EC_Normal if successful, an error code otherwise
     */
    OFCondition read(DcmItem &dataset,
                     OFConsole *logStream);

    /** write list of referenced datetime
     ** @param  dataset    DICOM dataset to which the list should be written
     *  @param  logStream  pointer to error/warning output stream (output disabled if NULL)
     ** @return status, EC_Normal if successful, an error code otherwise
     */
    OFCondition write(DcmItem &dataset,
                      OFConsole *logStream) const;

    /** put list of referenced datetime as a string.
     *  This function expects the same input format as created by print(), i.e. a comma
     *  separated list of datetime values.
     ** @param  stringValue  string value to be set
     ** @return status, EC_Normal if successful, an error code otherwise
     */
    OFCondition putString(const char *stringValue);
};


#endif


/*
 *  CVS/RCS Log:
 *  $Log: dsrtcodt.h,v $
 *  Revision 1.8  2005/12/08 16:05:20  meichel
 *  Changed include path schema for all DCMTK header files
 *
 *  Revision 1.7  2003/08/07 12:51:14  joergr
 *  Added new putString() method.
 *
 *  Revision 1.6  2003/06/03 10:16:44  meichel
 *  Renamed local variables to avoid name clashes with STL
 *
 *  Revision 1.5  2001/09/26 13:04:11  meichel
 *  Adapted dcmsr to class OFCondition
 *
 *  Revision 1.4  2001/06/01 15:51:04  meichel
 *  Updated copyright header
 *
 *  Revision 1.3  2000/11/06 11:19:18  joergr
 *  Added parameter to print() method specifying the item separator character.
 *
 *  Revision 1.2  2000/11/01 16:20:24  joergr
 *  Updated comments/formatting.
 *
 *  Revision 1.1  2000/10/26 14:23:24  joergr
 *  Added support for TCOORD content item.
 *
 *
 *
 */
