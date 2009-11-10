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
 *    classes: DSRTimeTreeNode
 *
 *  Last Update:      $Author: meichel $
 *  Update Date:      $Date: 2005/12/08 15:48:16 $
 *  CVS/RCS Revision: $Revision: 1.19 $
 *  Status:           $State: Exp $
 *
 *  CVS/RCS Log at end of file
 *
 */


#include "dcmtk/config/osconfig.h"    /* make sure OS specific configuration is included first */

#include "dcmtk/dcmsr/dsrtypes.h"
#include "dcmtk/dcmsr/dsrtimtn.h"
#include "dcmtk/dcmsr/dsrxmld.h"


DSRTimeTreeNode::DSRTimeTreeNode(const E_RelationshipType relationshipType)
 : DSRDocumentTreeNode(relationshipType, VT_Time),
   DSRStringValue()
{
}


DSRTimeTreeNode::DSRTimeTreeNode(const E_RelationshipType relationshipType,
                                 const OFString &stringValue)
 : DSRDocumentTreeNode(relationshipType, VT_Time),
   DSRStringValue(stringValue)
{
}


DSRTimeTreeNode::~DSRTimeTreeNode()
{
}


void DSRTimeTreeNode::clear()
{
    DSRDocumentTreeNode::clear();
    DSRStringValue::clear();
}


OFBool DSRTimeTreeNode::isValid() const
{
    /* ConceptNameCodeSequence required */
    return DSRDocumentTreeNode::isValid() && DSRStringValue::isValid() && getConceptName().isValid();
}


OFCondition DSRTimeTreeNode::print(ostream &stream,
                                   const size_t flags) const
{
    OFCondition result = DSRDocumentTreeNode::print(stream, flags);
    if (result.good())
    {
        stream << "=";
        DSRStringValue::print(stream);
    }
    return result;
}


OFCondition DSRTimeTreeNode::writeXML(ostream &stream,
                                      const size_t flags,
                                      OFConsole *logStream) const
{
    OFString tmpString;
    OFCondition result = EC_Normal;
    writeXMLItemStart(stream, flags);
    result = DSRDocumentTreeNode::writeXML(stream, flags, logStream);
    /* output time in ISO 8601 format */
    DcmTime::getISOFormattedTimeFromString(getValue(), tmpString);
    writeStringValueToXML(stream, tmpString, "value", (flags & XF_writeEmptyTags) > 0);
    writeXMLItemEnd(stream, flags);
    return result;
}


OFCondition DSRTimeTreeNode::readContentItem(DcmItem &dataset,
                                             OFConsole *logStream)
{
    /* read Time */
    return DSRStringValue::read(dataset, DCM_Time, logStream);
}


OFCondition DSRTimeTreeNode::writeContentItem(DcmItem &dataset,
                                              OFConsole *logStream) const
{
    /* write Time */
    return DSRStringValue::write(dataset, DCM_Time, logStream);
}


OFCondition DSRTimeTreeNode::readXMLContentItem(const DSRXMLDocument &doc,
                                                DSRXMLCursor cursor)
{
    OFString tmpString;
    /* retrieve value from XML element "value" */
    OFCondition result = setValue(getValueFromXMLNodeContent(doc, doc.getNamedNode(cursor.gotoChild(), "value"), tmpString));
    if (result == EC_IllegalParameter)
        result = SR_EC_InvalidValue;
    return result;
}


OFString &DSRTimeTreeNode::getValueFromXMLNodeContent(const DSRXMLDocument &doc,
                                                      DSRXMLCursor cursor,
                                                      OFString &timeValue,
                                                      const OFBool clearString)
{
    if (clearString)
        timeValue.clear();
    /* check whether node is valid */
    if (cursor.valid())
    {
        OFString tmpString;
        /* retrieve value from XML element */
        if (!doc.getStringFromNodeContent(cursor, tmpString).empty())
        {
            OFTime tmpTime;
            /* convert ISO to DICOM format */
            if (tmpTime.setISOFormattedTime(tmpString))
                DcmTime::getDicomTimeFromOFTime(tmpTime, timeValue);
        }
    }    
    return timeValue;
}


OFCondition DSRTimeTreeNode::renderHTMLContentItem(ostream &docStream,
                                                   ostream & /*annexStream*/,
                                                   const size_t /*nestingLevel*/,
                                                   size_t & /*annexNumber*/,
                                                   const size_t flags,
                                                   OFConsole *logStream) const
{
    /* render ConceptName */
    OFCondition result = renderHTMLConceptName(docStream, flags, logStream);
    /* render Time */
    if (result.good())
    {
        OFString htmlString;
        if (!(flags & DSRTypes::HF_renderItemsSeparately))
            docStream << "<u>";
        docStream << dicomToReadableTime(getValue(), htmlString);
        if (!(flags & DSRTypes::HF_renderItemsSeparately))
            docStream << "</u>";
        docStream << endl;
    }
    return result;
}


/*
 *  CVS/RCS Log:
 *  $Log: dsrtimtn.cc,v $
 *  Revision 1.19  2005/12/08 15:48:16  meichel
 *  Changed include path schema for all DCMTK header files
 *
 *  Revision 1.18  2004/01/16 10:17:04  joergr
 *  Adapted XML output format of Date, Time and Datetime to XML Schema (ISO
 *  8601) requirements.
 *
 *  Revision 1.17  2003/09/15 14:13:42  joergr
 *  Introduced new class to facilitate checking of SR IOD relationship content
 *  constraints. Replaced old implementation distributed over numerous classes.
 *
 *  Revision 1.16  2003/08/07 17:29:13  joergr
 *  Removed libxml dependency from header files. Simplifies linking (MSVC).
 *
 *  Revision 1.15  2003/08/07 15:21:53  joergr
 *  Added brackets around "bitwise and" operator/operands to avoid warnings
 *  reported by MSVC5.
 *
 *  Revision 1.14  2003/08/07 14:10:25  joergr
 *  Added readXML functionality.
 *  Distinguish more strictly between OFBool and int (required when HAVE_CXX_BOOL
 *  is defined).
 *
 *  Revision 1.13  2003/06/04 14:26:54  meichel
 *  Simplified include structure to avoid preprocessor limitation
 *    (max 32 #if levels) on MSVC5 with STL.
 *
 *  Revision 1.12  2001/11/09 16:19:36  joergr
 *  Added preliminary support for Mammography CAD SR.
 *
 *  Revision 1.11  2001/10/10 15:30:06  joergr
 *  Additonal adjustments for new OFCondition class.
 *
 *  Revision 1.10  2001/09/26 13:04:27  meichel
 *  Adapted dcmsr to class OFCondition
 *
 *  Revision 1.9  2001/05/07 16:14:26  joergr
 *  Updated CVS header.
 *
 *  Revision 1.8  2001/02/02 14:41:50  joergr
 *  Added new option to dsr2xml allowing to specify whether value and/or
 *  relationship type are to be encoded as XML attributes or elements.
 *
 *  Revision 1.7  2000/11/07 18:30:21  joergr
 *  Enhanced support for by-reference relationships.
 *  Enhanced rendered HTML output of date, time, datetime and pname.
 *
 *  Revision 1.6  2000/11/01 16:37:06  joergr
 *  Added support for conversion to XML. Optimized HTML rendering.
 *
 *  Revision 1.5  2000/10/26 14:35:09  joergr
 *  Added support for "Comprehensive SR".
 *
 *  Revision 1.4  2000/10/23 15:04:47  joergr
 *  Added clear() method.
 *
 *  Revision 1.3  2000/10/18 17:23:12  joergr
 *  Moved read and write methods to base class.
 *
 *  Revision 1.2  2000/10/16 12:08:51  joergr
 *  Reformatted print output.
 *
 *  Revision 1.1  2000/10/13 07:52:26  joergr
 *  Added new module 'dcmsr' providing access to DICOM structured reporting
 *  documents (supplement 23).  Doc++ documentation not yet completed.
 *
 *
 */
