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
 *  Module:  dcmsr
 *
 *  Author:  Joerg Riesmeier
 *
 *  Purpose:
 *    classes: DSRStringValue
 *
 *  Last Update:      $Author: meichel $
 *  Update Date:      $Date: 2005/12/08 15:48:09 $
 *  CVS/RCS Revision: $Revision: 1.13 $
 *  Status:           $State: Exp $
 *
 *  CVS/RCS Log at end of file
 *
 */


#include "dcmtk/config/osconfig.h"    /* make sure OS specific configuration is included first */

#include "dcmtk/dcmsr/dsrstrvl.h"
#include "dcmtk/dcmsr/dsrxmld.h"


DSRStringValue::DSRStringValue()
  : Value()
{
}


DSRStringValue::DSRStringValue(const OFString &stringValue)
  : Value()
{
    /* use the set methods for checking purposes */
    setValue(stringValue);
}


DSRStringValue::DSRStringValue(const DSRStringValue &stringValue)
  : Value(stringValue.Value)
{
    /* do not check since this would unexpected to the user */
}


DSRStringValue::~DSRStringValue()
{
}


DSRStringValue &DSRStringValue::operator=(const DSRStringValue &stringValue)
{
    /* do not check since this would unexpected to the user */
    Value = stringValue.Value;
    return *this;
}


void DSRStringValue::clear()
{
    Value.clear();
}


OFBool DSRStringValue::isValid() const
{
    return checkValue(Value);
}


void DSRStringValue::print(ostream &stream,
                           const size_t maxLength) const
{
    OFString printString;
    if ((maxLength > 3) && (Value.length() > maxLength))
        stream << "\"" << DSRTypes::convertToPrintString(Value.substr(0, maxLength - 3), printString) << "...\"";
    else
        stream << "\"" << DSRTypes::convertToPrintString(Value, printString) << "\"";
}


OFCondition DSRStringValue::read(DcmItem &dataset,
                                 const DcmTagKey &tagKey,
                                 OFConsole *logStream)
{
    /* tbd: check value */
    return DSRTypes::getAndCheckStringValueFromDataset(dataset, tagKey, Value, "1", "1", logStream, "content item");
}


OFCondition DSRStringValue::write(DcmItem &dataset,
                                  const DcmTagKey &tagKey,
                                  OFConsole * /*logStream*/) const
{
    /* write Value */
    return DSRTypes::putStringValueToDataset(dataset, tagKey, Value);
}


OFCondition DSRStringValue::readXML(const DSRXMLDocument &doc,
                                    DSRXMLCursor cursor,
                                    const OFBool encoding)
{
    OFCondition result = SR_EC_CorruptedXMLStructure;
    if (cursor.valid())
    {
        /* retrieve value from XML element */
        doc.getStringFromNodeContent(cursor, Value, NULL /*name*/, encoding);
        /* check whether string value is valid */
        result = (isValid() ? EC_Normal : SR_EC_InvalidValue);
    }
    return result;
}


OFCondition DSRStringValue::renderHTML(ostream &docStream,
                                       const size_t flags,
                                       OFConsole * /*logStream*/) const
{
    OFString htmlString;
    if (!(flags & DSRTypes::HF_renderItemsSeparately))
        docStream << "<u>";
    docStream << DSRTypes::convertToMarkupString(Value, htmlString, (flags & DSRTypes::HF_convertNonASCIICharacters) > 0);
    if (!(flags & DSRTypes::HF_renderItemsSeparately))
        docStream << "</u>";
    return EC_Normal;
}


OFCondition DSRStringValue::setValue(const OFString &stringValue)
{
    OFCondition result = EC_IllegalParameter;
    if (checkValue(stringValue))
    {
        Value = stringValue;
        result = EC_Normal;
    }
    return result;
}


OFBool DSRStringValue::checkValue(const OFString &stringValue) const
{
    return !stringValue.empty();
}

OFBool DSRStringValue::valueContainsExtendedCharacters() const
{
  return DSRTypes::stringContainsExtendedCharacters(Value);
}

/*
 *  CVS/RCS Log:
 *  $Log: dsrstrvl.cc,v $
 *  Revision 1.13  2005/12/08 15:48:09  meichel
 *  Changed include path schema for all DCMTK header files
 *
 *  Revision 1.12  2004/11/22 16:39:12  meichel
 *  Added method that checks if the SR document contains non-ASCII characters
 *    in any of the strings affected by SpecificCharacterSet.
 *
 *  Revision 1.11  2003/08/07 15:21:53  joergr
 *  Added brackets around "bitwise and" operator/operands to avoid warnings
 *  reported by MSVC5.
 *
 *  Revision 1.10  2003/08/07 13:53:01  joergr
 *  Added readXML functionality.
 *  Distinguish more strictly between OFBool and int (required when HAVE_CXX_BOOL
 *  is defined).
 *
 *  Revision 1.9  2001/10/10 15:30:01  joergr
 *  Additonal adjustments for new OFCondition class.
 *
 *  Revision 1.8  2001/09/26 13:04:24  meichel
 *  Adapted dcmsr to class OFCondition
 *
 *  Revision 1.7  2001/06/01 15:51:10  meichel
 *  Updated copyright header
 *
 *  Revision 1.6  2000/11/09 20:34:01  joergr
 *  Added support for non-ASCII characters in HTML 3.2 (use numeric value).
 *
 *  Revision 1.5  2000/11/01 16:37:04  joergr
 *  Added support for conversion to XML. Optimized HTML rendering.
 *
 *  Revision 1.4  2000/10/23 15:04:13  joergr
 *  Enhanced implementation of method isValid().
 *
 *  Revision 1.3  2000/10/19 16:07:14  joergr
 *  Added optional module name to read method to provide more detailed warning
 *  messages.
 *
 *  Revision 1.2  2000/10/18 17:22:44  joergr
 *  Added read and write methods.
 *  Added check for read methods (VM and type).
 *
 *  Revision 1.1  2000/10/13 07:52:25  joergr
 *  Added new module 'dcmsr' providing access to DICOM structured reporting
 *  documents (supplement 23).  Doc++ documentation not yet completed.
 *
 *
 */
