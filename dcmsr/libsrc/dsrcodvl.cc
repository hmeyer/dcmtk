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
 *    classes: DSRCodedEntryValue
 *
 *  Last Update:      $Author: meichel $
 *  Update Date:      $Date: 2005/12/08 15:47:41 $
 *  CVS/RCS Revision: $Revision: 1.20 $
 *  Status:           $State: Exp $
 *
 *  CVS/RCS Log at end of file
 *
 */


#include "dcmtk/config/osconfig.h"    /* make sure OS specific configuration is included first */

#include "dcmtk/dcmsr/dsrtypes.h"
#include "dcmtk/dcmsr/dsrcodvl.h"
#include "dcmtk/dcmsr/dsrxmld.h"


DSRCodedEntryValue::DSRCodedEntryValue()
  : CodeValue(),
    CodingSchemeDesignator(),
    CodingSchemeVersion(),
    CodeMeaning()
{
}


DSRCodedEntryValue::DSRCodedEntryValue(const OFString &codeValue,
                                       const OFString &codingSchemeDesignator,
                                       const OFString &codeMeaning)
  : CodeValue(codeValue),
    CodingSchemeDesignator(codingSchemeDesignator),
    CodingSchemeVersion(),
    CodeMeaning(codeMeaning)
{
    /* check code */
    setCode(codeValue, codingSchemeDesignator, codeMeaning);
}


DSRCodedEntryValue::DSRCodedEntryValue(const OFString &codeValue,
                                       const OFString &codingSchemeDesignator,
                                       const OFString &codingSchemeVersion,
                                       const OFString &codeMeaning)
  : CodeValue(),
    CodingSchemeDesignator(),
    CodingSchemeVersion(),
    CodeMeaning()
{
    /* check code */
    setCode(codeValue, codingSchemeDesignator, codingSchemeVersion, codeMeaning);
}


DSRCodedEntryValue::DSRCodedEntryValue(const DSRCodedEntryValue &codedEntryValue)
  : CodeValue(codedEntryValue.CodeValue),
    CodingSchemeDesignator(codedEntryValue.CodingSchemeDesignator),
    CodingSchemeVersion(codedEntryValue.CodingSchemeVersion),
    CodeMeaning(codedEntryValue.CodeMeaning)
{
    /* do not check since this would unexpected to the user */
}


DSRCodedEntryValue::~DSRCodedEntryValue()
{
}


DSRCodedEntryValue &DSRCodedEntryValue::operator=(const DSRCodedEntryValue &codedEntryValue)
{
    /* do not check since this would unexpected to the user */
    CodeValue = codedEntryValue.CodeValue;
    CodingSchemeDesignator = codedEntryValue.CodingSchemeDesignator;
    CodingSchemeVersion = codedEntryValue.CodingSchemeVersion;
    CodeMeaning = codedEntryValue.CodeMeaning;
    return *this;
}


OFBool DSRCodedEntryValue::operator==(const DSRCodedEntryValue &codedEntryValue) const
{
    return (CodeValue == codedEntryValue.CodeValue) &&
           (CodingSchemeDesignator == codedEntryValue.CodingSchemeDesignator) &&
           (CodingSchemeVersion == codedEntryValue.CodingSchemeVersion);
}


void DSRCodedEntryValue::clear()
{
    CodeValue.clear();
    CodingSchemeDesignator.clear();
    CodingSchemeVersion.clear();
    CodeMeaning.clear();
}


OFBool DSRCodedEntryValue::isValid() const
{
    return checkCode(CodeValue, CodingSchemeDesignator, CodeMeaning);
}


OFBool DSRCodedEntryValue::isEmpty() const
{
    return CodeValue.empty() && CodingSchemeDesignator.empty() && CodingSchemeVersion.empty() && CodeMeaning.empty();
}


void DSRCodedEntryValue::print(ostream &stream,
                               const OFBool printCodeValue,
                               const OFBool printInvalid) const
{
    if (isValid())
    {
        OFString printString;
        stream << "(";
        if (printCodeValue)
        {
            stream << DSRTypes::convertToPrintString(CodeValue, printString) << ",";
            stream << DSRTypes::convertToPrintString(CodingSchemeDesignator, printString);
            if (!CodingSchemeVersion.empty())
                stream << "[" << DSRTypes::convertToPrintString(CodingSchemeVersion, printString) << "]";
        } else
            stream << ",";
        stream << ",\"" << DSRTypes::convertToPrintString(CodeMeaning, printString) << "\")";
    }
    else if (printInvalid)
        stream << "invalid code";
}


OFCondition DSRCodedEntryValue::readItem(DcmItem &dataset,
                                         OFConsole *logStream,
                                         const char *moduleName)
{
    /* read BasicCodedEntryAttributes only */
    OFCondition result = DSRTypes::getAndCheckStringValueFromDataset(dataset, DCM_CodeValue, CodeValue, "1", "1", logStream, moduleName);
    if (result.good())
        result = DSRTypes::getAndCheckStringValueFromDataset(dataset, DCM_CodingSchemeDesignator, CodingSchemeDesignator, "1", "1", logStream, moduleName);
    if (result.good())                                             /* conditional (type 1C) */
        DSRTypes::getAndCheckStringValueFromDataset(dataset, DCM_CodingSchemeVersion, CodingSchemeVersion, "1", "1C", logStream, moduleName);
    if (result.good())
        result = DSRTypes::getAndCheckStringValueFromDataset(dataset, DCM_CodeMeaning, CodeMeaning, "1", "1", logStream, moduleName);
    /* tbd: might add check for correct code */

    return result;
}


OFCondition DSRCodedEntryValue::writeItem(DcmItem &dataset,
                                          OFConsole * /*logStream*/) const
{
    /* write BasicCodedEntryAttributes only */
    OFCondition result = DSRTypes::putStringValueToDataset(dataset, DCM_CodeValue, CodeValue);
    if (result.good())
        result = DSRTypes::putStringValueToDataset(dataset, DCM_CodingSchemeDesignator, CodingSchemeDesignator);
    if (result.good() && !CodingSchemeVersion.empty())                /* conditional (type 1C) */
        result = DSRTypes::putStringValueToDataset(dataset, DCM_CodingSchemeVersion, CodingSchemeVersion);
    if (result.good())
        result = DSRTypes::putStringValueToDataset(dataset, DCM_CodeMeaning, CodeMeaning);
    return result;
}


OFCondition DSRCodedEntryValue::readSequence(DcmItem &dataset,
                                             const DcmTagKey &tagKey,
                                             const OFString &type,
                                             OFConsole *logStream)
{
    /* read CodeSequence */
    DcmSequenceOfItems dseq(tagKey);
    OFCondition result = DSRTypes::getSequenceFromDataset(dataset, dseq);
    DSRTypes::checkElementValue(dseq, "1", type, logStream, result, "content item");
    if (result.good())
    {
        DcmItem *ditem = dseq.getItem(0);
        if (ditem != NULL)
        {
            /* read Code */
            result = readItem(*ditem, logStream, DcmTag(tagKey).getTagName());
        } else
            result = SR_EC_InvalidDocumentTree;
    }
    return result;
}


OFCondition DSRCodedEntryValue::writeSequence(DcmItem &dataset,
                                              const DcmTagKey &tagKey,
                                              OFConsole *logStream) const
{
    OFCondition result = EC_MemoryExhausted;
    /* write CodeSequence */
    DcmSequenceOfItems *dseq = new DcmSequenceOfItems(tagKey);
    if (dseq != NULL)
    {
        /* check for empty value */
        if (isEmpty())
            result = EC_Normal;
        else
        {
            DcmItem *ditem = new DcmItem();
            if (ditem != NULL)
            {
                /* write Code */
                if (isValid())
                    result = writeItem(*ditem, logStream);
                if (result.good())
                    dseq->insert(ditem);
                else
                    delete ditem;
            } else
                result = EC_MemoryExhausted;
        }
        if (result.good())
            result= dataset.insert(dseq, OFTrue /*replaceOld*/);
        if (result.bad())
            delete dseq;
    }
    return result;
}


OFCondition DSRCodedEntryValue::readXML(const DSRXMLDocument &doc,
                                        DSRXMLCursor cursor)
{
    OFCondition result = SR_EC_CorruptedXMLStructure;
    if (cursor.valid())
    {
        /* check whether code is stored as XML attributes */
        if (doc.hasAttribute(cursor, "codValue"))
        {
            doc.getStringFromAttribute(cursor, CodeValue, "codValue", OFTrue /*encoding*/);
            doc.getStringFromAttribute(cursor, CodingSchemeDesignator, "codScheme", OFTrue /*encoding*/);
            doc.getStringFromAttribute(cursor, CodingSchemeVersion, "codVersion", OFTrue /*encoding*/, OFFalse /*required*/);
            doc.getStringFromNodeContent(cursor, CodeMeaning, NULL /*name*/, OFTrue /*encoding*/);
        } else {
            /* goto first child node */
            cursor.gotoChild();
            /* iterate over all content items */
            while (cursor.valid())
            {
                /* check for known element tags */
                if (doc.matchNode(cursor, "scheme"))
                {
                    doc.getStringFromNodeContent(doc.getNamedNode(cursor.getChild(), "designator"), CodingSchemeDesignator, NULL /*name*/, OFTrue /*encoding*/, OFFalse /*clearString*/);
                    doc.getStringFromNodeContent(doc.getNamedNode(cursor.getChild(), "version", OFFalse /*required*/), CodingSchemeVersion, NULL /*name*/, OFTrue /*encoding*/, OFFalse /*clearString*/);
                } else {
                    doc.getStringFromNodeContent(cursor, CodeValue, "value", OFTrue /*encoding*/, OFFalse /*clearString*/);
                    doc.getStringFromNodeContent(cursor, CodeMeaning, "meaning", OFTrue /*encoding*/, OFFalse /*clearString*/);
                }
                /* proceed with next node */
                cursor.gotoNext();
            }
        }
        /* check whether code is valid */
        result = (isValid() ? EC_Normal : SR_EC_InvalidValue);
    }
    return result;
}


OFCondition DSRCodedEntryValue::writeXML(ostream &stream,
                                         const size_t flags,
                                         OFConsole * /*logStream*/) const
{
    OFString tmpString;
    if (flags & DSRTypes::XF_codeComponentsAsAttribute)
    {
        stream << " codValue=\"" << DSRTypes::convertToMarkupString(CodeValue, tmpString, OFFalse /*convertNonASCII*/, OFFalse /*newlineAllowed*/, OFTrue /*xmlMode*/) << "\"";
        stream << " codScheme=\"" << DSRTypes::convertToMarkupString(CodingSchemeDesignator, tmpString, OFFalse /*convertNonASCII*/, OFFalse /*newlineAllowed*/, OFTrue /*xmlMode*/) << "\"";
        if (!CodingSchemeVersion.empty() || (flags & DSRTypes::XF_writeEmptyTags))
            stream << " codVersion=\"" << DSRTypes::convertToMarkupString(CodingSchemeVersion, tmpString, OFFalse /*convertNonASCII*/, OFFalse /*newlineAllowed*/, OFTrue /*xmlMode*/) << "\"";
        stream << ">";      // close open bracket from calling routine
        stream << DSRTypes::convertToMarkupString(CodeMeaning, tmpString, OFFalse /*convertNonASCII*/, OFFalse /*newlineAllowed*/, OFTrue /*xmlMode*/);
    } else {
        DSRTypes::writeStringValueToXML(stream, CodeValue, "value", (flags & DSRTypes::XF_writeEmptyTags) > 0);
        stream << "<scheme>" << endl;
        DSRTypes::writeStringValueToXML(stream, CodingSchemeDesignator, "designator", (flags & DSRTypes::XF_writeEmptyTags) > 0);
        DSRTypes::writeStringValueToXML(stream, CodingSchemeVersion, "version", (flags & DSRTypes::XF_writeEmptyTags) > 0);
        stream << "</scheme>" << endl;
        DSRTypes::writeStringValueToXML(stream, CodeMeaning, "meaning", (flags & DSRTypes::XF_writeEmptyTags) > 0);
    }
    return EC_Normal;
}


OFCondition DSRCodedEntryValue::renderHTML(ostream &stream,
                                           const size_t flags,
                                           OFConsole * /*logStream*/,
                                           const OFBool fullCode,
                                           const OFBool valueFirst) const
{
    OFString htmlString;
    const OFBool convertNonASCII = (flags & DSRTypes::HF_convertNonASCIICharacters) > 0;
    if (valueFirst)
        stream << DSRTypes::convertToMarkupString(CodeValue, htmlString, convertNonASCII);
    else
        stream << DSRTypes::convertToMarkupString(CodeMeaning, htmlString, convertNonASCII);
    if (fullCode)
    {
        stream << " (";
        if (!valueFirst)
            stream << DSRTypes::convertToMarkupString(CodeValue, htmlString, convertNonASCII) << ", ";
        stream << DSRTypes::convertToMarkupString(CodingSchemeDesignator, htmlString, convertNonASCII);
        if (!CodingSchemeVersion.empty())
            stream << " [" << DSRTypes::convertToMarkupString(CodingSchemeVersion, htmlString, convertNonASCII) << "]";
        if (valueFirst)
            stream << ", " << DSRTypes::convertToMarkupString(CodeMeaning, htmlString, convertNonASCII);
        stream << ")";
    }
    return EC_Normal;
}


OFCondition DSRCodedEntryValue::getValue(DSRCodedEntryValue &codedEntryValue) const
{
    codedEntryValue = *this;
    return EC_Normal;
}


OFCondition DSRCodedEntryValue::setValue(const DSRCodedEntryValue &codedEntryValue)
{
    return setCode(codedEntryValue.CodeValue, codedEntryValue.CodingSchemeDesignator,
        codedEntryValue.CodingSchemeVersion, codedEntryValue.CodeMeaning);
}


OFCondition DSRCodedEntryValue::setCode(const OFString &codeValue,
                                        const OFString &codingSchemeDesignator,
                                        const OFString &codeMeaning)
{
    return setCode(codeValue, codingSchemeDesignator, "", codeMeaning);
}


OFCondition DSRCodedEntryValue::setCode(const OFString &codeValue,
                                        const OFString &codingSchemeDesignator,
                                        const OFString &codingSchemeVersion,
                                        const OFString &codeMeaning)
{
    OFCondition result = EC_Normal;
    if (checkCode(codeValue, codingSchemeDesignator, codeMeaning))
    {
        /* copy attributes */
        CodeValue = codeValue;
        CodingSchemeDesignator = codingSchemeDesignator;
        CodingSchemeVersion = codingSchemeVersion;
        CodeMeaning = codeMeaning;
    } else
        result = SR_EC_InvalidValue;
    return result;
}


OFBool DSRCodedEntryValue::checkCode(const OFString &codeValue,
                                     const OFString &codingSchemeDesignator,
                                     const OFString &codeMeaning) const
{
    /* need to check correctness of the code (code dictionary?) */
    return (!codeValue.empty() && !codingSchemeDesignator.empty() && !codeMeaning.empty());
}


OFBool DSRCodedEntryValue::valueContainsExtendedCharacters() const
{
  return DSRTypes::stringContainsExtendedCharacters(CodeValue)
    || DSRTypes::stringContainsExtendedCharacters(CodingSchemeDesignator)
    || DSRTypes::stringContainsExtendedCharacters(CodingSchemeVersion)
    || DSRTypes::stringContainsExtendedCharacters(CodeMeaning);
}


/*
 *  CVS/RCS Log:
 *  $Log: dsrcodvl.cc,v $
 *  Revision 1.20  2005/12/08 15:47:41  meichel
 *  Changed include path schema for all DCMTK header files
 *
 *  Revision 1.19  2004/11/22 16:39:12  meichel
 *  Added method that checks if the SR document contains non-ASCII characters
 *    in any of the strings affected by SpecificCharacterSet.
 *
 *  Revision 1.18  2003/11/28 16:51:44  joergr
 *  Changed output format of CodingSchemeVersion in print() and renderHTML().
 *  Now using square brackets instead of comma to separate from CodingScheme.
 *
 *  Revision 1.17  2003/09/10 13:18:43  joergr
 *  Replaced PrivateCodingSchemeUID by new CodingSchemeIdenticationSequence as
 *  required by CP 324.
 *
 *  Revision 1.16  2003/09/08 14:59:32  meichel
 *  Updated attribute names that have changed in DICOM 2003
 *
 *  Revision 1.15  2003/08/07 17:29:13  joergr
 *  Removed libxml dependency from header files. Simplifies linking (MSVC).
 *
 *  Revision 1.14  2003/08/07 15:21:53  joergr
 *  Added brackets around "bitwise and" operator/operands to avoid warnings
 *  reported by MSVC5.
 *
 *  Revision 1.13  2003/08/07 13:13:02  joergr
 *  Added readXML functionality.
 *
 *  Revision 1.12  2003/06/04 14:26:54  meichel
 *  Simplified include structure to avoid preprocessor limitation
 *    (max 32 #if levels) on MSVC5 with STL.
 *
 *  Revision 1.11  2001/11/09 16:13:16  joergr
 *  Added new command line option allowing to encode codes as XML attributes
 *  (instead of tags).
 *
 *  Revision 1.10  2001/10/10 15:29:48  joergr
 *  Additonal adjustments for new OFCondition class.
 *
 *  Revision 1.9  2001/10/02 12:07:06  joergr
 *  Adapted module "dcmsr" to the new class OFCondition. Introduced module
 *  specific error codes.
 *
 *  Revision 1.8  2001/09/28 14:09:26  joergr
 *  Check return value of DcmItem::insert() statements to avoid memory leaks
 *  when insert procedure failes.
 *
 *  Revision 1.7  2001/09/26 13:04:17  meichel
 *  Adapted dcmsr to class OFCondition
 *
 *  Revision 1.6  2001/06/01 15:51:07  meichel
 *  Updated copyright header
 *
 *  Revision 1.5  2000/11/09 20:33:59  joergr
 *  Added support for non-ASCII characters in HTML 3.2 (use numeric value).
 *
 *  Revision 1.4  2000/11/01 16:29:48  joergr
 *  Added support for conversion to XML.
 *  Enhanced support for specific character sets.
 *
 *  Revision 1.3  2000/10/19 16:03:20  joergr
 *  Renamed some set methods.
 *  Added optional module name to read method to provide more detailed warning
 *  messages.
 *
 *  Revision 1.2  2000/10/18 17:13:36  joergr
 *  Added check for read methods (VM and type).
 *
 *  Revision 1.1  2000/10/13 07:52:17  joergr
 *  Added new module 'dcmsr' providing access to DICOM structured reporting
 *  documents (supplement 23).  Doc++ documentation not yet completed.
 *
 *
 */
