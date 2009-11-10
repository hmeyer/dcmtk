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
 *    classes: DSRImageReferenceValue
 *
 *  Last Update:      $Author: meichel $
 *  Update Date:      $Date: 2005/12/08 15:47:55 $
 *  CVS/RCS Revision: $Revision: 1.17 $
 *  Status:           $State: Exp $
 *
 *  CVS/RCS Log at end of file
 *
 */


#include "dcmtk/config/osconfig.h"    /* make sure OS specific configuration is included first */

#include "dcmtk/dcmsr/dsrimgvl.h"
#include "dcmtk/dcmsr/dsrxmld.h"


DSRImageReferenceValue::DSRImageReferenceValue()
  : DSRCompositeReferenceValue(),
    PresentationState(),
    FrameList()
{
}


DSRImageReferenceValue::DSRImageReferenceValue(const OFString &sopClassUID,
                                               const OFString &sopInstanceUID)
  : DSRCompositeReferenceValue(),
    PresentationState(),
    FrameList()
{
    /* check for appropriate SOP class UID */
    setReference(sopClassUID, sopInstanceUID);
}


DSRImageReferenceValue::DSRImageReferenceValue(const OFString &imageSOPClassUID,
                                               const OFString &imageSOPInstanceUID,
                                               const OFString &pstateSOPClassUID,
                                               const OFString &pstateSOPInstanceUID)
  : DSRCompositeReferenceValue(),
    PresentationState(),
    FrameList()
{
    /* check for appropriate SOP class UID */
    setReference(imageSOPClassUID, imageSOPInstanceUID);
    setPresentationState(DSRCompositeReferenceValue(pstateSOPClassUID, pstateSOPInstanceUID));
}


DSRImageReferenceValue::DSRImageReferenceValue(const DSRImageReferenceValue &referenceValue)
  : DSRCompositeReferenceValue(referenceValue),
    PresentationState(referenceValue.PresentationState),
    FrameList(referenceValue.FrameList)
{
    /* do not check since this would unexpected to the user */
}


DSRImageReferenceValue::DSRImageReferenceValue(const DSRCompositeReferenceValue &imageReferenceValue,
                                               const DSRCompositeReferenceValue &pstateReferenceValue)
  : DSRCompositeReferenceValue(),
    PresentationState(),
    FrameList()
{
    /* check for appropriate SOP class UID */
    DSRCompositeReferenceValue::setValue(imageReferenceValue);
    setPresentationState(pstateReferenceValue);
}


DSRImageReferenceValue::~DSRImageReferenceValue()
{
}


DSRImageReferenceValue &DSRImageReferenceValue::operator=(const DSRImageReferenceValue &referenceValue)
{
    DSRCompositeReferenceValue::operator=(referenceValue);
    /* do not check since this would unexpected to the user */
    PresentationState = referenceValue.PresentationState;
    FrameList = referenceValue.FrameList;
    return *this;
}


void DSRImageReferenceValue::clear()
{
    DSRCompositeReferenceValue::clear();
    PresentationState.clear();
    FrameList.clear();
}


OFBool DSRImageReferenceValue::isValid() const
{
    return DSRCompositeReferenceValue::isValid() && checkPresentationState(PresentationState);
}


OFBool DSRImageReferenceValue::isShort(const size_t flags) const
{
    return (FrameList.isEmpty()) || !(flags & DSRTypes::HF_renderFullData);
}


OFCondition DSRImageReferenceValue::print(ostream &stream,
                                          const size_t flags) const
{
    const char *modality = dcmSOPClassUIDToModality(SOPClassUID.c_str());
    stream << "(";
    if (modality != NULL)
        stream << modality << " image";
    else
        stream << "\"" << SOPClassUID << "\"";
    stream << ",";
    if (flags & DSRTypes::PF_printSOPInstanceUID)
        stream << "\"" << SOPInstanceUID << "\"";
    if (!FrameList.isEmpty())
    {
        stream << ",";
        FrameList.print(stream, flags);
    }
    stream << ")";
    if (PresentationState.isValid())
    {
        stream << ",(GSPS,";
        if (flags & DSRTypes::PF_printSOPInstanceUID)
            stream << "\"" << PresentationState.getSOPInstanceUID() << "\"";
        stream << ")";
    }
    return EC_Normal;
}


OFCondition DSRImageReferenceValue::readXML(const DSRXMLDocument &doc,
                                            DSRXMLCursor cursor)
{
    /* first read general composite reference information */
    OFCondition result = DSRCompositeReferenceValue::readXML(doc, cursor);
    /* then read image related XML tags */
    if (result.good())
    {
        /* frame list (optional) */
        const DSRXMLCursor childCursor = doc.getNamedNode(cursor.getChild(), "frames", OFFalse /*required*/);
        if (childCursor.valid())
        {
            OFString tmpString;
            /* put element content to the channel list */
            result = FrameList.putString(doc.getStringFromNodeContent(childCursor, tmpString).c_str());
        }
        if (result.good())
        {
            /* presentation state (optional) */
            cursor = doc.getNamedNode(cursor.getChild(), "pstate", OFFalse /*required*/);
            if (cursor.getChild().valid())
                result = PresentationState.readXML(doc, cursor);
        }
    }
    return result;
}


OFCondition DSRImageReferenceValue::writeXML(ostream &stream,
                                             const size_t flags,
                                             OFConsole *logStream) const
{
    OFCondition result = DSRCompositeReferenceValue::writeXML(stream, flags, logStream);
    if ((flags & DSRTypes::XF_writeEmptyTags) || !FrameList.isEmpty())
    {
        stream << "<frames>";
        FrameList.print(stream);
        stream << "</frames>" << endl;
    }
    if ((flags & DSRTypes::XF_writeEmptyTags) || PresentationState.isValid())
    {
        stream << "<pstate>" << endl;
        if (PresentationState.isValid())
            PresentationState.writeXML(stream, flags, logStream);
        stream << "</pstate>" << endl;
    }
    return result;
}


OFCondition DSRImageReferenceValue::readItem(DcmItem &dataset,
                                             OFConsole *logStream)
{
    /* read ReferencedSOPClassUID and ReferencedSOPInstanceUID */
    OFCondition result = DSRCompositeReferenceValue::readItem(dataset, logStream);
    /* read ReferencedFrameNumber (conditional) */
    if (result.good())
        FrameList.read(dataset, logStream);
    /* read ReferencedSOPSequence (Presentation State, optional) */
    if (result.good())
        PresentationState.readSequence(dataset, "3" /*type*/, logStream);
    return result;
}


OFCondition DSRImageReferenceValue::writeItem(DcmItem &dataset,
                                              OFConsole *logStream) const
{
    /* write ReferencedSOPClassUID and ReferencedSOPInstanceUID */
    OFCondition result = DSRCompositeReferenceValue::writeItem(dataset, logStream);
    /* write ReferencedFrameNumber (conditional) */
    if (result.good())
    {
        if (!FrameList.isEmpty())
            result = FrameList.write(dataset, logStream);
    }
    /* write ReferencedSOPSequence (Presentation State, optional) */
    if (result.good())
    {
        if (PresentationState.isValid())
            result = PresentationState.writeSequence(dataset, logStream);
    }
    return result;
}


OFCondition DSRImageReferenceValue::renderHTML(ostream &docStream,
                                               ostream &annexStream,
                                               size_t &annexNumber,
                                               const size_t flags,
                                               OFConsole * /*logStream*/) const
{
    /* reference: image */
    docStream << "<a href=\"" << HTML_HYPERLINK_PREFIX_FOR_CGI;
    docStream << "?image=" << SOPClassUID << "+" << SOPInstanceUID;
    /* reference: pstate */
    if (PresentationState.isValid())
    {
        docStream << "&pstate=" << PresentationState.getSOPClassUID();
        docStream << "+" << PresentationState.getSOPInstanceUID();
    }
    /* reference: frames */
    if (!FrameList.isEmpty())
    {
        docStream << "&frames=";
        FrameList.print(docStream, 0 /*flags*/, '+');
    }
    docStream << "\">";
    /* text: image */
    const char *modality = dcmSOPClassUIDToModality(SOPClassUID.c_str());
    if (modality != NULL)
        docStream << modality;
    else
        docStream << "unknown";
    docStream << " image";
    /* text: pstate */
    if (PresentationState.isValid())
        docStream << " with GSPS";
    docStream << "</a>";
    if (!isShort(flags))
    {
        if (flags & DSRTypes::HF_currentlyInsideAnnex)
        {
            docStream << endl << "<p>" << endl;
            /* render frame list (= print)*/
            docStream << "<b>Referenced Frame Number:</b><br>";
            FrameList.print(docStream);
            docStream << "</p>";
        } else {
            DSRTypes::createHTMLAnnexEntry(docStream, annexStream, "for more details see", annexNumber);
            annexStream << "<p>" << endl;
            /* render frame list (= print)*/
            annexStream << "<b>Referenced Frame Number:</b><br>";
            FrameList.print(annexStream);
            annexStream << "</p>" << endl;
        }
    }
    return EC_Normal;
}


OFCondition DSRImageReferenceValue::getValue(DSRImageReferenceValue &referenceValue) const
{
    referenceValue = *this;
    return EC_Normal;
}


OFCondition DSRImageReferenceValue::setValue(const DSRImageReferenceValue &referenceValue)
{
    OFCondition result = DSRCompositeReferenceValue::setValue(referenceValue);
    if (result.good())
    {
        FrameList = referenceValue.FrameList;
        setPresentationState(referenceValue.PresentationState);
    }
    return result;
}


OFCondition DSRImageReferenceValue::setPresentationState(const DSRCompositeReferenceValue &referenceValue)
{
    OFCondition result = EC_IllegalParameter;
    if (checkPresentationState(referenceValue))
    {
        PresentationState = referenceValue;
        result = EC_Normal;
    }
    return result;
}


OFBool DSRImageReferenceValue::appliesToFrame(const Sint32 frameNumber) const
{
    OFBool result = OFTrue;
    if (!FrameList.isEmpty())
        result = FrameList.isElement(frameNumber);
    return result;
}


OFBool DSRImageReferenceValue::checkSOPClassUID(const OFString &sopClassUID) const
{
    OFBool result = OFFalse;
    if (DSRCompositeReferenceValue::checkSOPClassUID(sopClassUID))
    {
        /* tbd: might check for IMAGE storage class later on */
        result = OFTrue;
    }
    return result;
}


OFBool DSRImageReferenceValue::checkPresentationState(const DSRCompositeReferenceValue &referenceValue) const
{
    return referenceValue.isEmpty() || (referenceValue.isValid() &&
          (referenceValue.getSOPClassUID() == UID_GrayscaleSoftcopyPresentationStateStorage));
}


/*
 *  CVS/RCS Log:
 *  $Log: dsrimgvl.cc,v $
 *  Revision 1.17  2005/12/08 15:47:55  meichel
 *  Changed include path schema for all DCMTK header files
 *
 *  Revision 1.16  2004/01/16 10:14:14  joergr
 *  Made readXML() more robust with regard to expected XML structure.
 *  Only write <pstate/> XML element value when presentation state is valid.
 *
 *  Revision 1.15  2003/08/07 13:38:32  joergr
 *  Added readXML functionality.
 *  Renamed parameters/variables "string" to avoid name clash with STL class.
 *
 *  Revision 1.14  2001/10/10 15:29:56  joergr
 *  Additonal adjustments for new OFCondition class.
 *
 *  Revision 1.13  2001/09/26 13:04:22  meichel
 *  Adapted dcmsr to class OFCondition
 *
 *  Revision 1.12  2001/05/07 16:14:24  joergr
 *  Updated CVS header.
 *
 *  Revision 1.11  2001/02/13 16:35:28  joergr
 *  Minor corrections in XML output (newlines, etc.).
 *
 *  Revision 1.10  2000/11/06 11:32:51  joergr
 *  Changes structure of HTML hyperlinks to composite objects (now using pseudo
 *  CGI script).
 *
 *  Revision 1.9  2000/11/01 16:37:00  joergr
 *  Added support for conversion to XML. Optimized HTML rendering.
 *
 *  Revision 1.8  2000/10/26 14:31:44  joergr
 *  Use method isShort() to decide whether a content item can be rendered
 *  "inline" or not.
 *
 *  Revision 1.7  2000/10/24 15:04:11  joergr
 *  Changed HTML hyperlinks to referenced objects from "dicom://" to "file://"
 *  to facilitate access from Java.
 *
 *  Revision 1.6  2000/10/23 15:01:05  joergr
 *  Added SOP class UID to hyperlink in method renderHTML().
 *
 *  Revision 1.5  2000/10/20 10:14:58  joergr
 *  Renamed class DSRReferenceValue to DSRCompositeReferenceValue.
 *
 *  Revision 1.4  2000/10/19 16:04:42  joergr
 *  Renamed some set methods.
 *
 *  Revision 1.3  2000/10/18 17:19:12  joergr
 *  Added check for read methods (VM and type).
 *
 *  Revision 1.2  2000/10/16 12:05:32  joergr
 *  Reformatted print output.
 *  Added new method checking whether an image content item applies to a
 *  certain frame.
 *  Added new options: number nested items instead of indenting them, print SOP
 *  instance UID of referenced composite objects.
 *
 *  Revision 1.1  2000/10/13 07:52:21  joergr
 *  Added new module 'dcmsr' providing access to DICOM structured reporting
 *  documents (supplement 23).  Doc++ documentation not yet completed.
 *
 *
 */
