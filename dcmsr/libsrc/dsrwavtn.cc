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
 *    classes: DSRWaveformTreeNode
 *
 *  Last Update:      $Author: meichel $
 *  Update Date:      $Date: 2005/12/08 15:48:22 $
 *  CVS/RCS Revision: $Revision: 1.17 $
 *  Status:           $State: Exp $
 *
 *  CVS/RCS Log at end of file
 *
 */


#include "dcmtk/config/osconfig.h"    /* make sure OS specific configuration is included first */

#include "dcmtk/dcmsr/dsrtypes.h"
#include "dcmtk/dcmsr/dsrwavtn.h"
#include "dcmtk/dcmsr/dsrxmld.h"


DSRWaveformTreeNode::DSRWaveformTreeNode(const E_RelationshipType relationshipType)
 : DSRDocumentTreeNode(relationshipType, VT_Waveform),
   DSRWaveformReferenceValue()
{
}


DSRWaveformTreeNode::~DSRWaveformTreeNode()
{
}


void DSRWaveformTreeNode::clear()
{
    DSRDocumentTreeNode::clear();
    DSRWaveformReferenceValue::clear();
}


OFBool DSRWaveformTreeNode::isValid() const
{
    return DSRDocumentTreeNode::isValid() && DSRWaveformReferenceValue::isValid();
}


OFBool DSRWaveformTreeNode::isShort(const size_t flags) const
{
    return DSRWaveformReferenceValue::isShort(flags);
}


OFCondition DSRWaveformTreeNode::print(ostream &stream,
                                       const size_t flags) const
{
    OFCondition result = DSRDocumentTreeNode::print(stream, flags);
    if (result.good())
    {
        stream << "=";
        result = DSRWaveformReferenceValue::print(stream, flags);
    }
    return result;
}


OFCondition DSRWaveformTreeNode::writeXML(ostream &stream,
                                          const size_t flags,
                                          OFConsole *logStream) const
{
    OFCondition result = EC_Normal;
    writeXMLItemStart(stream, flags);
    result = DSRDocumentTreeNode::writeXML(stream, flags, logStream);
    stream << "<value>" << endl;
    DSRWaveformReferenceValue::writeXML(stream, flags, logStream);
    stream << "</value>" << endl;
    writeXMLItemEnd(stream, flags);
    return result;
}


OFCondition DSRWaveformTreeNode::readContentItem(DcmItem &dataset,
                                                 OFConsole *logStream)
{
    /* read ReferencedSOPSequence */
    return DSRWaveformReferenceValue::readSequence(dataset, "1" /*type*/, logStream);
}


OFCondition DSRWaveformTreeNode::writeContentItem(DcmItem &dataset,
                                                  OFConsole *logStream) const
{
    /* write ReferencedSOPSequence */
    return DSRWaveformReferenceValue::writeSequence(dataset, logStream);
}


OFCondition DSRWaveformTreeNode::readXMLContentItem(const DSRXMLDocument &doc,
                                                    DSRXMLCursor cursor)
{
    /* retrieve value from XML element "value" */
    return DSRWaveformReferenceValue::readXML(doc, doc.getNamedNode(cursor.gotoChild(), "value"));
}


OFCondition DSRWaveformTreeNode::renderHTMLContentItem(ostream &docStream,
                                                       ostream &annexStream,
                                                       const size_t /*nestingLevel*/,
                                                       size_t &annexNumber,
                                                       const size_t flags,
                                                       OFConsole *logStream) const
{
    /* render ConceptName */
    OFCondition result = renderHTMLConceptName(docStream, flags, logStream);
    /* render Reference */
    if (result.good())
    {
        result = DSRWaveformReferenceValue::renderHTML(docStream, annexStream, annexNumber, flags, logStream);
        docStream << endl;
    }
    return result;
}


/*
 *  CVS/RCS Log:
 *  $Log: dsrwavtn.cc,v $
 *  Revision 1.17  2005/12/08 15:48:22  meichel
 *  Changed include path schema for all DCMTK header files
 *
 *  Revision 1.16  2003/09/15 14:13:42  joergr
 *  Introduced new class to facilitate checking of SR IOD relationship content
 *  constraints. Replaced old implementation distributed over numerous classes.
 *
 *  Revision 1.15  2003/08/07 17:29:13  joergr
 *  Removed libxml dependency from header files. Simplifies linking (MSVC).
 *
 *  Revision 1.14  2003/08/07 14:17:18  joergr
 *  Added readXML functionality.
 *  Modified writeXML() output (introduced new "<value>...</value>" element).
 *
 *  Revision 1.13  2003/06/04 14:26:54  meichel
 *  Simplified include structure to avoid preprocessor limitation
 *    (max 32 #if levels) on MSVC5 with STL.
 *
 *  Revision 1.12  2001/11/09 16:20:48  joergr
 *  Added preliminary support for Mammography CAD SR.
 *
 *  Revision 1.11  2001/10/10 15:30:07  joergr
 *  Additonal adjustments for new OFCondition class.
 *
 *  Revision 1.10  2001/09/26 13:04:30  meichel
 *  Adapted dcmsr to class OFCondition
 *
 *  Revision 1.9  2001/05/07 16:14:26  joergr
 *  Updated CVS header.
 *
 *  Revision 1.8  2001/02/02 14:41:50  joergr
 *  Added new option to dsr2xml allowing to specify whether value and/or
 *  relationship type are to be encoded as XML attributes or elements.
 *
 *  Revision 1.7  2000/11/07 18:33:33  joergr
 *  Enhanced support for by-reference relationships.
 *
 *  Revision 1.6  2000/11/01 16:37:07  joergr
 *  Added support for conversion to XML. Optimized HTML rendering.
 *
 *  Revision 1.5  2000/10/26 14:37:48  joergr
 *  Added support for "Comprehensive SR".
 *
 *  Revision 1.4  2000/10/20 10:14:59  joergr
 *  Renamed class DSRReferenceValue to DSRCompositeReferenceValue.
 *
 *  Revision 1.3  2000/10/18 17:25:34  joergr
 *  Added check for read methods (VM and type).
 *
 *  Revision 1.2  2000/10/16 12:00:02  joergr
 *  Reformatted print output.
 *
 *  Revision 1.1  2000/10/13 07:52:29  joergr
 *  Added new module 'dcmsr' providing access to DICOM structured reporting
 *  documents (supplement 23).  Doc++ documentation not yet completed.
 *
 *
 */
