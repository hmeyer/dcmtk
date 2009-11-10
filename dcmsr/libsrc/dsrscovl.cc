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
 *    classes: DSRSpatialCoordinatesValue
 *
 *  Last Update:      $Author: meichel $
 *  Update Date:      $Date: 2005/12/08 15:48:06 $
 *  CVS/RCS Revision: $Revision: 1.16 $
 *  Status:           $State: Exp $
 *
 *  CVS/RCS Log at end of file
 *
 */


#include "dcmtk/config/osconfig.h"    /* make sure OS specific configuration is included first */

#include "dcmtk/dcmsr/dsrscovl.h"
#include "dcmtk/dcmsr/dsrxmld.h"


DSRSpatialCoordinatesValue::DSRSpatialCoordinatesValue()
  : GraphicType(DSRTypes::GT_invalid),
    GraphicDataList()
{
}


DSRSpatialCoordinatesValue::DSRSpatialCoordinatesValue(const DSRTypes::E_GraphicType graphicType)
  : GraphicType(graphicType),
    GraphicDataList()
{
}


DSRSpatialCoordinatesValue::DSRSpatialCoordinatesValue(const DSRSpatialCoordinatesValue &coordinatesValue)
  : GraphicType(coordinatesValue.GraphicType),
    GraphicDataList(coordinatesValue.GraphicDataList)
{
}


DSRSpatialCoordinatesValue::~DSRSpatialCoordinatesValue()
{
}


DSRSpatialCoordinatesValue &DSRSpatialCoordinatesValue::operator=(const DSRSpatialCoordinatesValue &coordinatesValue)
{
    GraphicType = coordinatesValue.GraphicType;
    GraphicDataList = coordinatesValue.GraphicDataList;
    return *this;
}


void DSRSpatialCoordinatesValue::clear()
{
    GraphicType = DSRTypes::GT_invalid;
    GraphicDataList.clear();
}


OFBool DSRSpatialCoordinatesValue::isValid() const
{
    return checkData(GraphicType, GraphicDataList);
}


OFBool DSRSpatialCoordinatesValue::isShort(const size_t flags) const
{
    return GraphicDataList.isEmpty() || ((flags & DSRTypes::HF_renderFullData) == 0);
}


OFCondition DSRSpatialCoordinatesValue::print(ostream &stream,
                                              const size_t flags) const
{
    /* GraphicType */
    stream << "(" << DSRTypes::graphicTypeToEnumeratedValue(GraphicType);
    /* GraphicData */
    if (!GraphicDataList.isEmpty())
    {
        stream << ",";
        GraphicDataList.print(stream, flags);
    }
    stream << ")";
    return EC_Normal;
}


OFCondition DSRSpatialCoordinatesValue::readXML(const DSRXMLDocument &doc,
                                                DSRXMLCursor cursor)
{
    OFCondition result = SR_EC_CorruptedXMLStructure;
    if (cursor.valid())
    {
        /* graphic data (required) */
        cursor = doc.getNamedNode(cursor.getChild(), "data");
        if (cursor.valid())
        {
            OFString tmpString;
            /* put value to the graphic data list */
            result = GraphicDataList.putString(doc.getStringFromNodeContent(cursor, tmpString).c_str());
        }
    }
    return result;
}


OFCondition DSRSpatialCoordinatesValue::writeXML(ostream &stream,
                                                 const size_t flags,
                                                 OFConsole * /*logStream*/) const
{
    /* GraphicType is written in TreeNode class */
    if ((flags & DSRTypes::XF_writeEmptyTags) || !GraphicDataList.isEmpty())
    {
        stream << "<data>";
        GraphicDataList.print(stream);
        stream << "</data>" << endl;
    }
    return EC_Normal;
}


OFCondition DSRSpatialCoordinatesValue::read(DcmItem &dataset,
                                             OFConsole *logStream)
{
    /* read GraphicType */
    OFString tmpString;
    OFCondition result = DSRTypes::getAndCheckStringValueFromDataset(dataset, DCM_GraphicType, tmpString, "1", "1", logStream, "SCOORD content item");
    if (result.good())
    {
        GraphicType = DSRTypes::enumeratedValueToGraphicType(tmpString);
        /* check GraphicType */
        if (GraphicType == DSRTypes::GT_invalid)
            DSRTypes::printUnknownValueWarningMessage(logStream, "GraphicType", tmpString.c_str());
        /* read GraphicData */
        result = GraphicDataList.read(dataset, logStream);
        /* check GraphicData and report warnings if any */
        checkData(GraphicType, GraphicDataList, logStream);
    }
    return result;
}


OFCondition DSRSpatialCoordinatesValue::write(DcmItem &dataset,
                                              OFConsole *logStream) const
{
    /* write GraphicType */
    OFCondition result = DSRTypes::putStringValueToDataset(dataset, DCM_GraphicType, DSRTypes::graphicTypeToEnumeratedValue(GraphicType));
    /* write GraphicData */
    if (result.good())
    {
        if (!GraphicDataList.isEmpty())
            result = GraphicDataList.write(dataset, logStream);
    }
    /* check GraphicData and report warnings if any */
    checkData(GraphicType, GraphicDataList, logStream);
    return result;
}


OFCondition DSRSpatialCoordinatesValue::renderHTML(ostream &docStream,
                                                   ostream &annexStream,
                                                   size_t &annexNumber,
                                                   const size_t flags,
                                                   OFConsole * /*logStream*/) const
{
    /* render GraphicType */
    docStream << DSRTypes::graphicTypeToReadableName(GraphicType);
    /* render GraphicData */
    if (!isShort(flags))
    {
        if (flags & DSRTypes::HF_currentlyInsideAnnex)
        {
            docStream << endl << "<p>" << endl;
            /* render graphic data list (= print)*/
            docStream << "<b>Graphic Data:</b><br>";
            GraphicDataList.print(docStream);
            docStream << "</p>";
        } else {
            DSRTypes::createHTMLAnnexEntry(docStream, annexStream, "for more details see", annexNumber);
            annexStream << "<p>" << endl;
            /* render graphic data list (= print)*/
            annexStream << "<b>Graphic Data:</b><br>";
            GraphicDataList.print(annexStream);
            annexStream << "</p>" << endl;
        }
    }
    return EC_Normal;
}


OFCondition DSRSpatialCoordinatesValue::setGraphicType(const DSRTypes::E_GraphicType graphicType)
{
    OFCondition result = EC_IllegalParameter;
    if (graphicType != DSRTypes::GT_invalid)
    {
        GraphicType = graphicType;
        result = EC_Normal;
    }
    return result;
}


OFCondition DSRSpatialCoordinatesValue::getValue(DSRSpatialCoordinatesValue &coordinatesValue) const
{
    coordinatesValue = *this;
    return EC_Normal;
}


OFCondition DSRSpatialCoordinatesValue::setValue(const DSRSpatialCoordinatesValue &coordinatesValue)
{
    OFCondition result = EC_IllegalParameter;
    if (checkData(coordinatesValue.GraphicType, coordinatesValue.GraphicDataList))
    {
        GraphicType = coordinatesValue.GraphicType;
        GraphicDataList = coordinatesValue.GraphicDataList;
        result = EC_Normal;
    }
    return result;
}


OFBool DSRSpatialCoordinatesValue::checkData(const DSRTypes::E_GraphicType graphicType,
                                             const DSRGraphicDataList &graphicDataList,
                                             OFConsole *logStream) const
{
    OFBool result = OFFalse;
    if (graphicType == DSRTypes::GT_invalid)
        DSRTypes::printWarningMessage(logStream, "Invalid GraphicType for SCOORD content item");
    else if (graphicDataList.isEmpty())
        DSRTypes::printWarningMessage(logStream, "No GraphicData for SCOORD content item");
    else
    {
        const size_t count = graphicDataList.getNumberOfItems();
        switch (graphicType)
        {
            case DSRTypes::GT_Point:
                if (count > 1)
                    DSRTypes::printWarningMessage(logStream, "GraphicData has too many entries, only a single entry expected");
                result = OFTrue;
                break;
            case DSRTypes::GT_Multipoint:
                if (count < 1)
                    DSRTypes::printWarningMessage(logStream, "GraphicData has too few entries, at least one entry expected");
                result = OFTrue;
                break;
            case DSRTypes::GT_Polyline:
/*              // not required any more according to CP-233
                if (!(graphicDataList.getItem(1) == graphicDataList.getItem(count)))
                    DSRTypes::printWarningMessage(logStream, "First and last entry in GraphicData are not equal (POLYLINE)");
*/
                result = OFTrue;
                break;
            case DSRTypes::GT_Circle:
                if (count < 2)
                    DSRTypes::printWarningMessage(logStream, "GraphicData has too few entries, exactly two entries expected");
                else
                {
                    if (count > 2)
                        DSRTypes::printWarningMessage(logStream, "GraphicData has too many entries, exactly two entries expected");
                    result = OFTrue;
                }
                break;
            case DSRTypes::GT_Ellipse:
                if (count < 4)
                    DSRTypes::printWarningMessage(logStream, "GraphicData has too few entries, exactly four entries expected");
                else
                {
                    if (count > 4)
                        DSRTypes::printWarningMessage(logStream, "GraphicData has too many entries, exactly four entries expected");
                    result = OFTrue;
                }
                break;
            default:
                /* GT_invalid */
                break;
        }
    }
    return result;
}


/*
 *  CVS/RCS Log:
 *  $Log: dsrscovl.cc,v $
 *  Revision 1.16  2005/12/08 15:48:06  meichel
 *  Changed include path schema for all DCMTK header files
 *
 *  Revision 1.15  2003/10/17 16:10:43  joergr
 *  Fixed wrong wording in a warning message ("too less" -> "too few").
 *
 *  Revision 1.14  2003/08/07 15:21:53  joergr
 *  Added brackets around "bitwise and" operator/operands to avoid warnings
 *  reported by MSVC5.
 *
 *  Revision 1.13  2003/08/07 13:46:04  joergr
 *  Added readXML functionality.
 *  Distinguish more strictly between OFBool and int (required when HAVE_CXX_BOOL
 *  is defined).
 *
 *  Revision 1.12  2002/12/05 13:53:30  joergr
 *  Added further checks when reading SR documents (e.g. value of VerificationFlag,
 *  CompletionsFlag, ContinuityOfContent and SpecificCharacterSet).
 *
 *  Revision 1.11  2001/10/10 15:30:01  joergr
 *  Additonal adjustments for new OFCondition class.
 *
 *  Revision 1.10  2001/09/26 13:04:24  meichel
 *  Adapted dcmsr to class OFCondition
 *
 *  Revision 1.9  2001/05/07 16:14:25  joergr
 *  Updated CVS header.
 *
 *  Revision 1.8  2001/02/13 16:35:28  joergr
 *  Minor corrections in XML output (newlines, etc.).
 *
 *  Revision 1.7  2000/11/06 11:33:45  joergr
 *  Removed additional check (according to CP).
 *
 *  Revision 1.6  2000/11/01 16:37:03  joergr
 *  Added support for conversion to XML. Optimized HTML rendering.
 *
 *  Revision 1.5  2000/10/26 14:34:39  joergr
 *  Use method isShort() to decide whether a content item can be rendered
 *  "inline" or not.
 *
 *  Revision 1.4  2000/10/19 16:06:42  joergr
 *  Added optional module name to read method to provide more detailed warning
 *  messages.
 *
 *  Revision 1.3  2000/10/18 17:22:09  joergr
 *  Added check for read methods (VM and type).
 *
 *  Revision 1.2  2000/10/16 12:08:29  joergr
 *  Reformatted print output.
 *
 *  Revision 1.1  2000/10/13 07:52:25  joergr
 *  Added new module 'dcmsr' providing access to DICOM structured reporting
 *  documents (supplement 23).  Doc++ documentation not yet completed.
 *
 *
 */
