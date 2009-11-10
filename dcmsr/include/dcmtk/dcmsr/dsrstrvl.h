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
 *    classes: DSRStringValue
 *
 *  Last Update:      $Author: meichel $
 *  Update Date:      $Date: 2005/12/08 16:05:19 $
 *  CVS/RCS Revision: $Revision: 1.11 $
 *  Status:           $State: Exp $
 *
 *  CVS/RCS Log at end of file
 *
 */


#ifndef DSRSTRVL_H
#define DSRSTRVL_H

#include "dcmtk/config/osconfig.h"   /* make sure OS specific configuration is included first */

#include "dcmtk/dcmsr/dsrtypes.h"

#include "dcmtk/ofstd/ofstring.h"


/*---------------------*
 *  class declaration  *
 *---------------------*/

/** Class for string values
 */
class DSRStringValue
{

  public:

    /** default contructor
     */
    DSRStringValue();

    /** constructor
     *  The string value is only set if it passed the validity check (see setValue()).
     ** @param  stringValue  string value to be set
     */
    DSRStringValue(const OFString &stringValue);

    /** copy constructor
     ** @param  stringValue  string value to be copied (not checked !)
     */
    DSRStringValue(const DSRStringValue &stringValue);

    /** destructor
     */
    virtual ~DSRStringValue();

    /** assignment operator
     ** @param  stringValue  string value to be copied (not checked !)
     ** @return reference to this string value after 'stringValue' has been copied
     */
    DSRStringValue &operator=(const DSRStringValue &stringValue);

    /** clear all internal variables.
     *  Please note that the string value might become invalid afterwards.
     */
    virtual void clear();

    /** check whether the current code is valid.
     *  See checkValue() for details.
     ** @return OFTrue if code is valid, OFFalse otherwise
     */
    virtual OFBool isValid() const;

    /** print string value.
     *  The output of a typical string value looks like this: "Short text" or "Very long t..."
     *  (incl. the quotation marks).
     ** @param  stream     output stream to which the string value should be printed
     *  @param  maxLength  maximum number of characters to be printed.  If the string value is
     *                     longer the output is shortened automatically and three dots "..." are
     *                     added.  The value of 'maxLength' includes these three trailing char's.
     *                     A value of 0 turns this meachanism off (default), i.e. the full string
     *                     value is printed.
     */
    void print(ostream &stream,
               const size_t maxLength = 0) const;

    /** read string value from dataset.
     *  If error/warning output is enabled a warning message is printed if the string value does
     *  not conform with the type (= 1) and value multiplicity (= 1).
     ** @param  dataset    DICOM dataset from which the string value should be read
     *  @param  tagKey     DICOM tag specifying the attribute which should be read
     *  @param  logStream  pointer to error/warning output stream (output disabled if NULL)
     ** @return status, EC_Normal if successful, an error code otherwise
     */
    OFCondition read(DcmItem &dataset,
                     const DcmTagKey &tagKey,
                     OFConsole *logStream);

    /** write string value to dataset
     ** @param  dataset    DICOM dataset to which the string value should be written
     *  @param  tagKey     DICOM tag specifying the attribute which should be written
     *  @param  logStream  pointer to error/warning output stream (output disabled if NULL)
     ** @return status, EC_Normal if successful, an error code otherwise
     */
    OFCondition write(DcmItem &dataset,
                      const DcmTagKey &tagKey,
                      OFConsole *logStream) const;

    /** read string value from XML document
     ** @param  doc       document containing the XML file content
     *  @param  cursor    cursor pointing to the starting node
     *  @param  encoding  use encoding handler if OFTrue, ignore character set otherwise
     ** @return status, EC_Normal if successful, an error code otherwise
     */
    OFCondition readXML(const DSRXMLDocument &doc,
                        DSRXMLCursor cursor,
                        const OFBool encoding = OFFalse);

    /** render string value in HTML format
     ** @param  docStream  output stream to which the main HTML document is written
     *  @param  flags      flag used to customize the output (see DSRTypes::HF_xxx)
     *  @param  logStream  pointer to error/warning output stream (output disabled if NULL)
     ** @return status, EC_Normal if successful, an error code otherwise
     */
    OFCondition renderHTML(ostream &docStream,
                           const size_t flags,
                           OFConsole *logStream) const;

    /** get string value
     ** @return reference to string value
     */
    inline const OFString &getValue() const
    {
        return Value;
    }

    /** set string value.
     *  Before setting the string value it is checked (see checkValue()).  If the value is
     *  invalid the current value is not replaced and remains unchanged.  Use clear() to
     *  empty the string value (which becomes invalid afterwards).
     ** @param  stringValue  value to be set
     ** @return status, EC_Normal if successful, an error code otherwise
     */
    OFCondition setValue(const OFString &stringValue);

    /** check if this value contains non-ASCII characters.
     *  @return true if value contains non-ASCII characters, false otherwise
     */
    OFBool valueContainsExtendedCharacters() const;

  protected:

    /** check the specified string value for validity.
     *  This base class just checks that the string value is not empty (since all corresponding
     *  DICOM attributes are type 1).  Derived classes might overwrite this method to perform
     *  more sophisticated tests.
     ** @param  stringValue  string value to be checked
     ** @return OFTrue if code is valid, OFFalse otherwise
     */
    virtual OFBool checkValue(const OFString &stringValue) const;


  private:

    /// string value (various VRs, mandatory)
    OFString Value;
};


#endif


/*
 *  CVS/RCS Log:
 *  $Log: dsrstrvl.h,v $
 *  Revision 1.11  2005/12/08 16:05:19  meichel
 *  Changed include path schema for all DCMTK header files
 *
 *  Revision 1.10  2004/11/22 16:39:09  meichel
 *  Added method that checks if the SR document contains non-ASCII characters
 *    in any of the strings affected by SpecificCharacterSet.
 *
 *  Revision 1.9  2003/08/07 18:01:42  joergr
 *  Removed libxml dependency from header files.
 *
 *  Revision 1.8  2003/08/07 12:50:44  joergr
 *  Added readXML functionality.
 *
 *  Revision 1.7  2001/09/26 13:04:11  meichel
 *  Adapted dcmsr to class OFCondition
 *
 *  Revision 1.6  2001/06/01 15:51:03  meichel
 *  Updated copyright header
 *
 *  Revision 1.5  2000/11/06 11:18:48  joergr
 *  Moved some protected methods to public part.
 *
 *  Revision 1.4  2000/11/01 16:23:25  joergr
 *  Added support for conversion to XML.
 *
 *  Revision 1.3  2000/10/23 15:12:55  joergr
 *  Added/updated doc++ comments.
 *
 *  Revision 1.2  2000/10/18 17:08:11  joergr
 *  Added read and write methods.
 *
 *  Revision 1.1  2000/10/13 07:49:32  joergr
 *  Added new module 'dcmsr' providing access to DICOM structured reporting
 *  documents (supplement 23).  Doc++ documentation not yet completed.
 *
 *
 */
