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
 *    classes: DSRWaveformReferenceValue
 *
 *  Last Update:      $Author: meichel $
 *  Update Date:      $Date: 2005/12/08 15:48:23 $
 *  CVS/RCS Revision: $Revision: 1.16 $
 *  Status:           $State: Exp $
 *
 *  CVS/RCS Log at end of file
 *
 */


#include "dcmtk/config/osconfig.h"    /* make sure OS specific configuration is included first */

#include "dcmtk/dcmsr/dsrwavvl.h"
#include "dcmtk/dcmsr/dsrxmld.h"


DSRWaveformReferenceValue::DSRWaveformReferenceValue()
  : DSRCompositeReferenceValue(),
    ChannelList()
{
}


DSRWaveformReferenceValue::DSRWaveformReferenceValue(const OFString &sopClassUID,
                                                     const OFString &sopInstanceUID)
  : DSRCompositeReferenceValue(),
    ChannelList()
{
    /* check for appropriate SOP class UID */
    setReference(sopClassUID, sopInstanceUID);
}


DSRWaveformReferenceValue::DSRWaveformReferenceValue(const DSRWaveformReferenceValue &referenceValue)
  : DSRCompositeReferenceValue(referenceValue),
    ChannelList(referenceValue.ChannelList)
{
    /* do not check since this would unexpected to the user */
}


DSRWaveformReferenceValue::~DSRWaveformReferenceValue()
{
}


DSRWaveformReferenceValue &DSRWaveformReferenceValue::operator=(const DSRWaveformReferenceValue &referenceValue)
{
    DSRCompositeReferenceValue::operator=(referenceValue);
    /* do not check since this would unexpected to the user */
    ChannelList = referenceValue.ChannelList;
    return *this;
}


void DSRWaveformReferenceValue::clear()
{
    DSRCompositeReferenceValue::clear();
    ChannelList.clear();
}


OFBool DSRWaveformReferenceValue::isShort(const size_t flags) const
{
    return ChannelList.isEmpty() || !(flags & DSRTypes::HF_renderFullData);
}


OFCondition DSRWaveformReferenceValue::print(ostream &stream,
                                             const size_t flags) const
{
    const char *className = dcmFindNameOfUID(SOPClassUID.c_str());
    stream << "(";
    if (className != NULL)
        stream << className;
    else
        stream << "\"" << SOPClassUID << "\"";
    stream << ",";
    if (flags & DSRTypes::PF_printSOPInstanceUID)
        stream << "\"" << SOPInstanceUID << "\"";
    if (!ChannelList.isEmpty())
    {
        stream << ",";
        ChannelList.print(stream, flags);
    }
    stream << ")";
    return EC_Normal;
}


OFCondition DSRWaveformReferenceValue::readXML(const DSRXMLDocument &doc,
                                               DSRXMLCursor cursor)
{
    /* first read general composite reference information */
    OFCondition result = DSRCompositeReferenceValue::readXML(doc, cursor);
    /* then read waveform related XML tags */
    if (result.good())
    {
        /* channel list (optional) */
        cursor = doc.getNamedNode(cursor.getChild(), "channels");
        if (cursor.valid())
        {
            OFString tmpString;
            /* put element content to the channel list */
            result = ChannelList.putString(doc.getStringFromNodeContent(cursor, tmpString).c_str());
        }
    }
    return result;
}


OFCondition DSRWaveformReferenceValue::writeXML(ostream &stream,
                                                const size_t flags,
                                                OFConsole *logStream) const
{
    OFCondition result = DSRCompositeReferenceValue::writeXML(stream, flags, logStream);
    if ((flags & DSRTypes::XF_writeEmptyTags) || !ChannelList.isEmpty())
    {
        stream << "<channels>";
        ChannelList.print(stream);
        stream << "</channels>" << endl;
    }
    return result;
}


OFCondition DSRWaveformReferenceValue::readItem(DcmItem &dataset,
                                                OFConsole *logStream)
{
    /* read ReferencedSOPClassUID and ReferencedSOPInstanceUID */
    OFCondition result = DSRCompositeReferenceValue::readItem(dataset, logStream);
    /* read ReferencedWaveformChannels (conditional) */
    if (result.good())
        ChannelList.read(dataset, logStream);
    return result;
}


OFCondition DSRWaveformReferenceValue::writeItem(DcmItem &dataset,
                                                 OFConsole *logStream) const
{
    /* write ReferencedSOPClassUID and ReferencedSOPInstanceUID */
    OFCondition result = DSRCompositeReferenceValue::writeItem(dataset, logStream);
    /* write ReferencedWaveformChannels (conditional) */
    if (result.good())
    {
        if (!ChannelList.isEmpty())
            result = ChannelList.write(dataset, logStream);
    }
    return result;
}


OFCondition DSRWaveformReferenceValue::renderHTML(ostream &docStream,
                                                  ostream &annexStream,
                                                  size_t &annexNumber,
                                                  const size_t flags,
                                                  OFConsole * /*logStream*/) const
{
    /* render reference */
    docStream << "<a href=\"" << HTML_HYPERLINK_PREFIX_FOR_CGI;
    docStream << "?waveform=" << SOPClassUID << "+" << SOPInstanceUID;
    if (!ChannelList.isEmpty())
    {
        docStream << "&channels=";
        ChannelList.print(docStream, 0 /*flags*/, '+', '+');
    }
    docStream << "\">";
    const char *className = dcmFindNameOfUID(SOPClassUID.c_str());
    if (className != NULL)
        docStream << className;
    else
        docStream << "unknown waveform";
    docStream << "</a>";
    /* render (optional) channel list */
    if (!isShort(flags))
    {
        if (flags & DSRTypes::HF_currentlyInsideAnnex)
        {
            docStream  << endl << "<p>" << endl;
            /* render channel list (= print)*/
            docStream << "<b>Referenced Waveform Channels:</b><br>";
            ChannelList.print(docStream);
            docStream << "</p>";
        } else {
            DSRTypes::createHTMLAnnexEntry(docStream, annexStream, "for more details see", annexNumber);
            annexStream << "<p>" << endl;
            /* render channel list (= print)*/
            annexStream << "<b>Referenced Waveform Channels:</b><br>";
            ChannelList.print(annexStream);
            annexStream << "</p>" << endl;
        }
    }
    return EC_Normal;
}


OFCondition DSRWaveformReferenceValue::getValue(DSRWaveformReferenceValue &referenceValue) const
{
    referenceValue = *this;
    return EC_Normal;
}


OFCondition DSRWaveformReferenceValue::setValue(const DSRWaveformReferenceValue &referenceValue)
{
    OFCondition result = DSRCompositeReferenceValue::setValue(referenceValue);
    if (result.good())
        ChannelList = referenceValue.ChannelList;
    return result;
}


OFBool DSRWaveformReferenceValue::appliesToChannel(const Uint16 multiplexGroupNumber,
                                                   const Uint16 channelNumber) const
{
    OFBool result = OFTrue;
    if (!ChannelList.isEmpty())
        result = ChannelList.isElement(multiplexGroupNumber, channelNumber);
    return result;
}


OFBool DSRWaveformReferenceValue::checkSOPClassUID(const OFString &sopClassUID) const
{
    OFBool result = OFFalse;
    if (DSRCompositeReferenceValue::checkSOPClassUID(sopClassUID))
    {
        /* check for all valid/known SOP classes (according to DICOM PS 3.x 2003) */
        if ((sopClassUID == UID_TwelveLeadECGWaveformStorage) ||
            (sopClassUID == UID_GeneralECGWaveformStorage) ||
            (sopClassUID == UID_AmbulatoryECGWaveformStorage) ||
            (sopClassUID == UID_HemodynamicWaveformStorage) ||
            (sopClassUID == UID_CardiacElectrophysiologyWaveformStorage) ||
            (sopClassUID == UID_BasicVoiceAudioWaveformStorage))
        {
            result = OFTrue;
        }
    }
    return result;
}


/*
 *  CVS/RCS Log:
 *  $Log: dsrwavvl.cc,v $
 *  Revision 1.16  2005/12/08 15:48:23  meichel
 *  Changed include path schema for all DCMTK header files
 *
 *  Revision 1.15  2003/08/07 14:18:30  joergr
 *  Added readXML functionality.
 *  Renamed parameters/variables "string" to avoid name clash with STL class.
 *
 *  Revision 1.14  2001/10/10 15:30:08  joergr
 *  Additonal adjustments for new OFCondition class.
 *
 *  Revision 1.13  2001/09/26 13:04:31  meichel
 *  Adapted dcmsr to class OFCondition
 *
 *  Revision 1.12  2001/05/07 16:14:27  joergr
 *  Updated CVS header.
 *
 *  Revision 1.11  2001/02/13 16:33:40  joergr
 *  Minor corrections in XML output (newlines, etc.).
 *
 *  Revision 1.10  2000/11/06 11:32:52  joergr
 *  Changes structure of HTML hyperlinks to composite objects (now using pseudo
 *  CGI script).
 *
 *  Revision 1.9  2000/11/01 16:37:08  joergr
 *  Added support for conversion to XML. Optimized HTML rendering.
 *
 *  Revision 1.8  2000/10/26 14:38:02  joergr
 *  Use method isShort() to decide whether a content item can be rendered
 *  "inline" or not.
 *
 *  Revision 1.7  2000/10/24 15:04:12  joergr
 *  Changed HTML hyperlinks to referenced objects from "dicom://" to "file://"
 *  to facilitate access from Java.
 *
 *  Revision 1.6  2000/10/23 15:01:05  joergr
 *  Added SOP class UID to hyperlink in method renderHTML().
 *
 *  Revision 1.5  2000/10/20 10:14:59  joergr
 *  Renamed class DSRReferenceValue to DSRCompositeReferenceValue.
 *
 *  Revision 1.4  2000/10/19 16:07:42  joergr
 *  Renamed some set methods.
 *
 *  Revision 1.3  2000/10/18 17:25:34  joergr
 *  Added check for read methods (VM and type).
 *
 *  Revision 1.2  2000/10/16 12:11:41  joergr
 *  Reformatted print output.
 *  Added new method checking whether a waveform content item applies to a
 *  certain channel.
 *  Added new options: number nested items instead of indenting them, print SOP
 *  instance UID of referenced composite objects.
 *
 *  Revision 1.1  2000/10/13 07:52:30  joergr
 *  Added new module 'dcmsr' providing access to DICOM structured reporting
 *  documents (supplement 23).  Doc++ documentation not yet completed.
 *
 *
 */
