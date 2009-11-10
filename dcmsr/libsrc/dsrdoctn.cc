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
 *    classes: DSRDocumentTreeNode
 *
 *  Last Update:      $Author: meichel $
 *  Update Date:      $Date: 2005/12/08 15:47:49 $
 *  CVS/RCS Revision: $Revision: 1.41 $
 *  Status:           $State: Exp $
 *
 *  CVS/RCS Log at end of file
 *
 */


#include "dcmtk/config/osconfig.h"    /* make sure OS specific configuration is included first */

#include "dcmtk/dcmsr/dsrdoctn.h"
#include "dcmtk/dcmsr/dsrdtitn.h"
#include "dcmtk/dcmsr/dsrxmld.h"
#include "dcmtk/dcmsr/dsriodcc.h"

#include "dcmtk/ofstd/ofstring.h"
#include "dcmtk/ofstd/ofstream.h"


DSRDocumentTreeNode::DSRDocumentTreeNode(const E_RelationshipType relationshipType,
                                         const E_ValueType valueType)
  : DSRTreeNode(),
    MarkFlag(OFFalse),
    ReferenceTarget(OFFalse),
    RelationshipType(relationshipType),
    ValueType(valueType),
    ConceptName(),
    ObservationDateTime(),
    TemplateIdentifier(),
    MappingResource(),
    MACParameters(DCM_MACParametersSequence),
    DigitalSignatures(DCM_DigitalSignaturesSequence)
{
}


DSRDocumentTreeNode::~DSRDocumentTreeNode()
{
}


void DSRDocumentTreeNode::clear()
{
    MarkFlag = OFFalse;
    ReferenceTarget = OFFalse;
    ConceptName.clear();
    ObservationDateTime.clear();
    TemplateIdentifier.clear();
    MappingResource.clear();
    MACParameters.clear();
    DigitalSignatures.clear();
}


OFBool DSRDocumentTreeNode::isValid() const
{
    return (RelationshipType != RT_invalid) && (ValueType != VT_invalid);
}


OFBool DSRDocumentTreeNode::isShort(const size_t /*flags*/) const
{
    return OFTrue;
}


OFCondition DSRDocumentTreeNode::print(ostream &stream,
                                       const size_t flags) const
{
    if (RelationshipType != RT_isRoot)
        stream << relationshipTypeToReadableName(RelationshipType) << " ";
    stream << valueTypeToDefinedTerm(ValueType) << ":";
    /* only print valid concept name codes */
    if (ConceptName.isValid())
        ConceptName.print(stream, (flags & PF_printConceptNameCodes) > 0);
    return EC_Normal;
}


OFCondition DSRDocumentTreeNode::read(DcmItem &dataset,
                                      const DSRIODConstraintChecker *constraintChecker,
                                      const size_t flags,
                                      OFConsole *logStream)
{
    return readSRDocumentContentModule(dataset, constraintChecker, flags, logStream);
}


OFCondition DSRDocumentTreeNode::write(DcmItem &dataset,
                                       DcmStack *markedItems,
                                       OFConsole *logStream)
{
    return writeSRDocumentContentModule(dataset, markedItems, logStream);
}


OFCondition DSRDocumentTreeNode::readXML(const DSRXMLDocument &doc,
                                         DSRXMLCursor cursor,
                                         const E_DocumentType documentType,
                                         const size_t flags)
{
    OFCondition result = SR_EC_InvalidDocument;
    if (cursor.valid())
    {
        OFString idAttr;
        OFString templateIdentifier, mappingResource;
        /* important: NULL indicates first child node */
        DSRDocumentTreeNode *node = NULL;
        /* read "id" attribute (optional) and compare with expected value */
        if (!doc.getStringFromAttribute(cursor, idAttr, "id", OFFalse /*encoding*/, OFFalse /*required*/).empty() &&
            (stringToNumber(idAttr.c_str()) != Ident))
        {
            /* create warning message */
            OFOStringStream oss;
            oss << "XML attribute 'id' (" << idAttr << ") deviates from current node number (" << Ident << ")";
            OFSTRINGSTREAM_GETSTR(oss, tmpString)
            printWarningMessage(doc.getLogStream(), tmpString);
            OFSTRINGSTREAM_FREESTR(tmpString)
        }
        /* template identification information expected "inside" content item */
        if (!(flags & XF_templateElementEnclosesItems))
        {
            /* check for optional template identification */
            const DSRXMLCursor childCursor = doc.getNamedNode(cursor.getChild(), "template", OFFalse /*required*/);
            if (childCursor.valid())
            {
                /* check whether information is stored as XML attributes */
                if (doc.hasAttribute(childCursor, "tid"))
                {
                    doc.getStringFromAttribute(childCursor, mappingResource, "resource");
                    doc.getStringFromAttribute(childCursor, templateIdentifier, "tid");
                } else {
                    doc.getStringFromNodeContent(doc.getNamedNode(childCursor.getChild(), "resource"), mappingResource);
                    doc.getStringFromNodeContent(doc.getNamedNode(childCursor.getChild(), "id"), templateIdentifier);
                }
                if (setTemplateIdentification(templateIdentifier, mappingResource).bad())
                    printWarningMessage(doc.getLogStream(), "Content item has invalid/incomplete template identification");
            }
        }
        /* read concept name (not required in some cases) */
        ConceptName.readXML(doc, doc.getNamedNode(cursor.getChild(), "concept", OFFalse /*required*/));
        /* read observation datetime (optional) */
        const DSRXMLCursor childCursor = doc.getNamedNode(cursor.getChild(), "observation", OFFalse /*required*/);
        if (childCursor.valid())
            DSRDateTimeTreeNode::getValueFromXMLNodeContent(doc, doc.getNamedNode(childCursor.getChild(), "datetime"), ObservationDateTime);
        /* read node content (depends on value type) */
        result = readXMLContentItem(doc, cursor);
        /* goto first child node */
        cursor.gotoChild();
        /* iterate over all child content items */
        while (cursor.valid() && result.good())
        {
            /* template identification information expected "outside" content item */
            if (flags & XF_templateElementEnclosesItems)
            {
                /* check for optional template identification */
                if (doc.matchNode(cursor, "template"))
                {
                    doc.getStringFromAttribute(cursor, mappingResource, "resource");
                    doc.getStringFromAttribute(cursor, templateIdentifier, "tid");
                    /* goto first child of the "template" element */
                    cursor.gotoChild();
                }
            }
            /* get SR value type from current XML node, also supports "by-reference" detection */
            E_ValueType valueType = doc.getValueTypeFromNode(cursor);
            /* invalid types are silently ignored */
            if (valueType != VT_invalid)
            {
                /* get SR relationship type */
                E_RelationshipType relationshipType = doc.getRelationshipTypeFromNode(cursor);
                /* create new node (by-value or by-reference), do not check constraints */
                result = createAndAppendNewNode(node, relationshipType, valueType);
                if (result.good())
                {
                    if ((flags & XF_templateElementEnclosesItems) && (valueType != VT_byReference))
                    {
                        /* set template identification (if any) */
                        if (node->setTemplateIdentification(templateIdentifier, mappingResource).bad())
                            printWarningMessage(doc.getLogStream(), "Content item has invalid/incomplete template identification");
                    }
                    /* proceed with reading child nodes */
                    result = node->readXML(doc, cursor, documentType, flags);
                    /* print node error message (if any) */
                    doc.printGeneralNodeError(cursor, result);
                } else {
                    /* create new node failed */
                    OFString message = "Cannot add \"";
                    message += relationshipTypeToReadableName(relationshipType);
                    message += " ";
                    message += valueTypeToDefinedTerm(valueType /*target item*/);
                    message += "\" to ";
                    message += valueTypeToDefinedTerm(ValueType /*source item*/);
                    message += " in ";
                    message += documentTypeToReadableName(documentType);
                    printErrorMessage(doc.getLogStream(), message.c_str());
                }
            }
            /* proceed with next node */
            cursor.gotoNext();
        }
    }
    return result;
}


OFCondition DSRDocumentTreeNode::readXMLContentItem(const DSRXMLDocument & /*doc*/,
                                                    DSRXMLCursor /*cursor*/)
{
    return EC_IllegalCall;
}


OFCondition DSRDocumentTreeNode::writeXML(ostream &stream,
                                          const size_t flags,
                                          OFConsole *logStream) const
{
    OFCondition result = EC_Normal;
    /* check for validity */
    if (!isValid())
        printInvalidContentItemMessage(logStream, "Writing to XML", this);
    /* write optional template identification */
    if ((flags & XF_writeTemplateIdentification) && !(flags & XF_templateElementEnclosesItems))
    {
        if (!TemplateIdentifier.empty() && !MappingResource.empty())
        {
            if (flags & XF_templateIdentifierAsAttribute)
                stream << "<template resource=\"" << MappingResource << "\" tid=\"" << TemplateIdentifier << "\"/>" << endl;
            else
            {
                stream << "<template>" << endl;
                writeStringValueToXML(stream, MappingResource, "resource");
                writeStringValueToXML(stream, TemplateIdentifier, "id");
                stream << "</template>" << endl;
            }
        }
    }
    /* relationship type */
    if ((RelationshipType != RT_isRoot) && !(flags & XF_relationshipTypeAsAttribute))
        writeStringValueToXML(stream, relationshipTypeToDefinedTerm(RelationshipType), "relationship", (flags & XF_writeEmptyTags) > 0);
    /* concept name */
    if (ConceptName.isValid())
    {
        if (flags & XF_codeComponentsAsAttribute)
            stream << "<concept";     // bracket ">" is closed in the next writeXML() routine
        else
            stream << "<concept>" << endl;
        ConceptName.writeXML(stream, flags, logStream);
        stream << "</concept>" << endl;
    }
    /* observation datetime (optional) */
    if (!ObservationDateTime.empty())
    {
        OFString tmpString;
        stream << "<observation>" << endl;
        DcmDateTime::getISOFormattedDateTimeFromString(ObservationDateTime, tmpString, OFTrue /*seconds*/,
            OFFalse /*fraction*/, OFFalse /*timeZone*/, OFFalse /*createMissingPart*/, "T" /*dateTimeSeparator*/);
        writeStringValueToXML(stream, tmpString, "datetime");
        stream << "</observation>" << endl;
    }
    /* write child nodes (if any) */
    DSRTreeNodeCursor cursor(Down);
    if (cursor.isValid())
    {
        DSRDocumentTreeNode *node = NULL;
        do {    /* for all child nodes */
            node = OFstatic_cast(DSRDocumentTreeNode *, cursor.getNode());
            if (node != NULL)
                result = node->writeXML(stream, flags, logStream);
            else
                result = SR_EC_InvalidDocumentTree;
        } while (result.good() && cursor.gotoNext());
    }
    return result;
}


void DSRDocumentTreeNode::writeXMLItemStart(ostream &stream,
                                            const size_t flags,
                                            const OFBool closingBracket) const
{
    /* write optional template identification */
    if ((flags & XF_writeTemplateIdentification) && (flags & XF_templateElementEnclosesItems))
    {
        if (!TemplateIdentifier.empty() && !MappingResource.empty())
            stream << "<template resource=\"" << MappingResource << "\" tid=\"" << TemplateIdentifier << "\">" << endl;
    }
    /* write content item */
    if (flags & XF_valueTypeAsAttribute)
    {
        stream << "<item";
        if (ValueType != VT_byReference)
            stream << " valType=\"" << valueTypeToDefinedTerm(ValueType) << "\"";
    } else
        stream << "<" << valueTypeToXMLTagName(ValueType);
    if ((RelationshipType != RT_isRoot) && (flags & XF_relationshipTypeAsAttribute))
        stream << " relType=\"" << relationshipTypeToDefinedTerm(RelationshipType) << "\"";
    if (ReferenceTarget || (flags & XF_alwaysWriteItemIdentifier))
        stream << " id=\"" << getNodeID() << "\"";
    if (closingBracket)
        stream << ">" << endl;
}


void DSRDocumentTreeNode::writeXMLItemEnd(ostream &stream,
                                          const size_t flags) const
{
    /* close content item */
    if (flags & XF_valueTypeAsAttribute)
        stream << "</item>" << endl;
    else
        stream << "</" << valueTypeToXMLTagName(ValueType) <<  ">" << endl;
    /* close optional template identification */
    if ((flags & XF_writeTemplateIdentification) && (flags & XF_templateElementEnclosesItems))
    {
        if (!TemplateIdentifier.empty() && !MappingResource.empty())
            stream << "</template>" << endl;
    }
}


OFCondition DSRDocumentTreeNode::renderHTML(ostream &docStream,
                                            ostream &annexStream,
                                            const size_t nestingLevel,
                                            size_t &annexNumber,
                                            const size_t flags,
                                            OFConsole *logStream) const
{
    /* check for validity */
    if (!isValid())
        printInvalidContentItemMessage(logStream, "Rendering", this);
    /* declare hyperlink target */
    if (ReferenceTarget)
        docStream << "<a name=\"content_item_" << getNodeID() << "\">" << endl;
    /* render content item */
    OFCondition result = renderHTMLContentItem(docStream, annexStream, nestingLevel, annexNumber, flags, logStream);
    if (ReferenceTarget)
        docStream << "</a>" << endl;
    /* render child nodes */
    if (result.good())
        result = renderHTMLChildNodes(docStream, annexStream, nestingLevel, annexNumber, flags | HF_renderItemsSeparately, logStream);
    else
        printContentItemErrorMessage(logStream, "Rendering", result, this);
    return result;
}


OFCondition DSRDocumentTreeNode::getConceptName(DSRCodedEntryValue &conceptName) const
{
    conceptName = ConceptName;
    return EC_Normal;
}


OFCondition DSRDocumentTreeNode::setConceptName(const DSRCodedEntryValue &conceptName)
{
    OFCondition result = EC_IllegalParameter;
    /* check for valid code */
    if (conceptName.isValid() || conceptName.isEmpty())
    {
        ConceptName = conceptName;
        result = EC_Normal;
    }
    return result;
}


OFCondition DSRDocumentTreeNode::setObservationDateTime(const OFString &observationDateTime)
{
    /* might add a check for proper DateTime format */
    ObservationDateTime = observationDateTime;
    return EC_Normal;
}


OFCondition DSRDocumentTreeNode::getTemplateIdentification(OFString &templateIdentifier,
                                                           OFString &mappingResource) const
{
    OFCondition result = SR_EC_InvalidValue;
    /* check for valid value pair */
    if ((TemplateIdentifier.empty() && MappingResource.empty()) ||
        (!TemplateIdentifier.empty() && !MappingResource.empty()))
    {
        templateIdentifier = TemplateIdentifier;
        mappingResource = MappingResource;
        result = EC_Normal;
    }
    return result;
}


OFCondition DSRDocumentTreeNode::setTemplateIdentification(const OFString &templateIdentifier,
                                                           const OFString &mappingResource)
{
    OFCondition result = EC_IllegalParameter;
    /* check for valid value pair */
    if ((templateIdentifier.empty() && mappingResource.empty()) ||
        (!templateIdentifier.empty() && !mappingResource.empty()))
    {
        TemplateIdentifier = templateIdentifier;
        MappingResource = mappingResource;
        result = EC_Normal;
    }
    return result;
}


void DSRDocumentTreeNode::removeSignatures()
{
    MACParameters.clear();
    DigitalSignatures.clear();
}


OFCondition DSRDocumentTreeNode::readContentItem(DcmItem & /*dataset*/,
                                                 OFConsole * /*logStream*/)
{
    /* no content to read */
    return EC_Normal;
}


OFCondition DSRDocumentTreeNode::writeContentItem(DcmItem & /*dataset*/,
                                                  OFConsole * /*logStream*/) const
{
    /* no content to insert */
    return EC_Normal;
}


OFCondition DSRDocumentTreeNode::renderHTMLContentItem(ostream & /*docStream*/,
                                                       ostream & /*annexStream*/,
                                                       const size_t /*nestingLevel*/,
                                                       size_t & /*annexNumber*/,
                                                       const size_t /*flags*/,
                                                       OFConsole * /*logStream*/) const
{
    /* no content to render */
    return EC_Normal;
}


OFCondition DSRDocumentTreeNode::readSRDocumentContentModule(DcmItem &dataset,
                                                             const DSRIODConstraintChecker *constraintChecker,
                                                             const size_t flags,
                                                             OFConsole *logStream)
{
    OFCondition result = EC_Normal;
    /* read DocumentRelationshipMacro */
    result = readDocumentRelationshipMacro(dataset, constraintChecker, "1" /*posString*/, flags, logStream);
    /* read DocumentContentMacro */
    if (result.good())
        result = readDocumentContentMacro(dataset, "1" /*posString*/, flags, logStream);
    return result;
}


OFCondition DSRDocumentTreeNode::writeSRDocumentContentModule(DcmItem &dataset,
                                                              DcmStack *markedItems,
                                                              OFConsole *logStream)
{
    OFCondition result = EC_Normal;
    /* write DocumentRelationshipMacro */
    result = writeDocumentRelationshipMacro(dataset, markedItems, logStream);
    /* write DocumentContentMacro */
    if (result.good())
        result = writeDocumentContentMacro(dataset, logStream);
    return result;
}


OFCondition DSRDocumentTreeNode::readDocumentRelationshipMacro(DcmItem &dataset,
                                                               const DSRIODConstraintChecker *constraintChecker,
                                                               const OFString &posString,
                                                               const size_t flags,
                                                               OFConsole *logStream)
{
    OFCondition result = EC_Normal;
    /* read digital signatures sequences (optional) */
    if (flags & RF_readDigitalSignatures)
    {
        getSequenceFromDataset(dataset, MACParameters);
        getSequenceFromDataset(dataset, DigitalSignatures);
    }
    /* read ObservationDateTime (conditional) */
    getAndCheckStringValueFromDataset(dataset, DCM_ObservationDateTime, ObservationDateTime, "1", "1C", logStream);
    /* determine template identifier expected for this document */
    const OFString expectedTemplateIdentifier = (constraintChecker != NULL) ? constraintChecker->getRootTemplateIdentifier() : "";
    /* read ContentTemplateSequence (conditional) */
    DcmItem *ditem = NULL;
    if (dataset.findAndGetSequenceItem(DCM_ContentTemplateSequence, ditem, 0 /*itemNum*/).good())
    {
        getAndCheckStringValueFromDataset(*ditem, DCM_MappingResource, MappingResource, "1", "1", logStream, "ContentTemplateSequence");
        getAndCheckStringValueFromDataset(*ditem, DCM_TemplateIdentifier, TemplateIdentifier, "1", "1", logStream, "ContentTemplateSequence");
        if (!expectedTemplateIdentifier.empty())
        {
            /* check for DICOM Content Mapping Resource */
            if (MappingResource == "DCMR")
            {
                /* compare with expected TID */
                if (TemplateIdentifier != expectedTemplateIdentifier)
                {
                    OFString message = "Incorrect value for TemplateIdentifier (";
                    if (TemplateIdentifier.empty())
                        message += "<empty>";
                    else
                        message += TemplateIdentifier;
                    message += "), ";
                    message += expectedTemplateIdentifier;
                    message += " expected";
                    printWarningMessage(logStream, message.c_str());
                }
            } else if (!MappingResource.empty())
                printUnknownValueWarningMessage(logStream, "MappingResource", MappingResource.c_str());
        }
    }
    /* only check template identifier on dataset level (root node) */
    else if ((dataset.ident() == EVR_dataset) && !expectedTemplateIdentifier.empty())
    {
        OFString message = "ContentTemplateSequence missing or empty, TemplateIdentifier ";
        message += expectedTemplateIdentifier;
        /* DICOM Content Mapping Resource is currently hard-coded (see above) */
        message += " (DCMR) expected";
        printWarningMessage(logStream, message.c_str());
    }
    /* read ContentSequence */
    if (result.good())
        result = readContentSequence(dataset, constraintChecker, posString, flags, logStream);
    return result;
}


OFCondition DSRDocumentTreeNode::writeDocumentRelationshipMacro(DcmItem &dataset,
                                                                DcmStack *markedItems,
                                                                OFConsole *logStream)
{
    OFCondition result = EC_Normal;
    /* write digital signatures sequences (optional) */
    if (MACParameters.card() > 0)
        addElementToDataset(result, dataset, new DcmSequenceOfItems(MACParameters));
    if (DigitalSignatures.card() > 0)
    {
        addElementToDataset(result, dataset, new DcmSequenceOfItems(DigitalSignatures));
        printWarningMessage(logStream, "Writing possibly incorrect digital signature - same as read from dataset.");
    }
    /* add to mark stack */
    if (MarkFlag && (markedItems != NULL))
        markedItems->push(&dataset);
    /* write ObservationDateTime (conditional) */
    result = putStringValueToDataset(dataset, DCM_ObservationDateTime, ObservationDateTime, OFFalse /*allowEmpty*/);
    /* write ContentTemplateSequence (conditional) */
    if (result.good())
    {
        if (!TemplateIdentifier.empty() && !MappingResource.empty())
        {
            DcmItem *ditem = NULL;
            /* create sequence with a single item */
            result = dataset.findOrCreateSequenceItem(DCM_ContentTemplateSequence, ditem, 0 /*position*/);
            if (result.good())
            {
                /* write item data */
                putStringValueToDataset(*ditem, DCM_TemplateIdentifier, TemplateIdentifier);
                putStringValueToDataset(*ditem, DCM_MappingResource, MappingResource);
            }
        }
    }
    /* write ContentSequence */
    if (result.good())
        result = writeContentSequence(dataset, markedItems, logStream);
    return result;
}


OFCondition DSRDocumentTreeNode::readDocumentContentMacro(DcmItem &dataset,
                                                          const OFString &posString,
                                                          const size_t flags,
                                                          OFConsole *logStream)
{
    OFCondition result = EC_Normal;
    /* skip reading ValueType, already done somewhere else */

    /* read ConceptNameCodeSequence (might be empty) */
    ConceptName.readSequence(dataset, DCM_ConceptNameCodeSequence, "1C" /*type*/, logStream);
    /* read ContentItem (depending on ValueType) */
    result = readContentItem(dataset, logStream);
    /* check for validity, after reading */
    if (result.bad() || !isValid())
    {
        printInvalidContentItemMessage(logStream, "Reading", this, posString.c_str());
        /* ignore content item reading/parsing error if flag is set */
        if (flags & RF_ignoreContentItemErrors)
           result = EC_Normal;
        /* content item is not valid */
        else if (result.good())
           result = SR_EC_InvalidValue;
    }
    return result;
}


OFCondition DSRDocumentTreeNode::writeDocumentContentMacro(DcmItem &dataset,
                                                           OFConsole *logStream) const
{
    OFCondition result = EC_Normal;
    /* write ValueType */
    result = putStringValueToDataset(dataset, DCM_ValueType, valueTypeToDefinedTerm(ValueType));
    /* write ConceptNameCodeSequence */
    if (result.good())
    {
        if (ConceptName.isValid())
            result = ConceptName.writeSequence(dataset, DCM_ConceptNameCodeSequence, logStream);
    }
    if (result.good())
    {
        /* check for validity, before writing */
        if (!isValid())
            printInvalidContentItemMessage(logStream, "Writing", this);
        /* write ContentItem (depending on ValueType) */
        result = writeContentItem(dataset, logStream);
    }
    return result;
}


OFCondition DSRDocumentTreeNode::createAndAppendNewNode(DSRDocumentTreeNode *&previousNode,
                                                        const E_RelationshipType relationshipType,
                                                        const E_ValueType valueType,
                                                        const DSRIODConstraintChecker *constraintChecker)
{
    OFCondition result = EC_Normal;
    /* do not check by-reference relationships here, will be done later (after complete reading) */
    if (((valueType == VT_byReference) && (relationshipType != RT_unknown)) || (constraintChecker == NULL) ||
        constraintChecker->checkContentRelationship(ValueType, relationshipType, valueType))
    {
        DSRDocumentTreeNode *node = createDocumentTreeNode(relationshipType, valueType);
        if (node != NULL)
        {
            /* first child node */
            if (previousNode == NULL)
                Down = node;
            else
            {
                /* new sibling */
                previousNode->Next = node;
                node->Prev = previousNode;
            }
            /* store new node for the next time */
            previousNode = node;
        } else {
            if (valueType == VT_unknown)
                result = SR_EC_UnknownValueType;
            else
                result = EC_MemoryExhausted;
        }
    } else {
        /* summarize what went wrong */
        if (valueType == VT_unknown)
            result = SR_EC_UnknownValueType;
        else if (relationshipType == RT_unknown)
            result = SR_EC_UnknownRelationshipType;
        else
            result = SR_EC_InvalidByValueRelationship;
    }
    return result;
}


OFCondition DSRDocumentTreeNode::readContentSequence(DcmItem &dataset,
                                                     const DSRIODConstraintChecker *constraintChecker,
                                                     const OFString &posString,
                                                     const size_t flags,
                                                     OFConsole *logStream)
{
    OFCondition result = EC_Normal;
    DcmStack stack;
    /* read ContentSequence (might be absent or empty) */
    if (dataset.search(DCM_ContentSequence, stack, ESM_fromHere, OFFalse).good())
    {
        DcmSequenceOfItems *dseq = OFstatic_cast(DcmSequenceOfItems *, stack.top());
        if (dseq != NULL)
        {
            OFString tmpString;
            E_ValueType valueType = VT_invalid;
            E_RelationshipType relationshipType = RT_invalid;
            /* important: NULL indicates first child node */
            DSRDocumentTreeNode *node = NULL;
            DcmItem *ditem = NULL;
            const unsigned long count = dseq->card();
            /* for all items in the sequence */
            unsigned long i = 0;
            while ((i < count) && result.good())
            {
                ditem = dseq->getItem(i);
                if (ditem != NULL)
                {
                    /* create current location string */
                    char buffer[20];
                    OFString location = posString;
                    if (!location.empty())
                        location += ".";
                    location += numberToString(OFstatic_cast(size_t, i + 1), buffer);
                    if ((logStream != NULL) && (flags & RF_showCurrentlyProcessedItem))
                    {
                        logStream->lockCerr() << "Processing content item " << location << endl;
                        logStream->unlockCerr();
                    }
                    /* read RelationshipType */
                    result = getAndCheckStringValueFromDataset(*ditem, DCM_RelationshipType, tmpString, "1", "1", logStream);
                    if (result.good())
                    {
                        relationshipType = definedTermToRelationshipType(tmpString);
                        /* check relationship type */
                        if (relationshipType == RT_unknown)
                            printUnknownValueWarningMessage(logStream, "RelationshipType", tmpString.c_str());
                        /* check for by-reference relationship */
                        DcmUnsignedLong referencedContentItemIdentifier(DCM_ReferencedContentItemIdentifier);
                        if (getAndCheckElementFromDataset(*ditem, referencedContentItemIdentifier, "1-n", "1C", logStream).good())
                        {
                            /* create new node (by-reference) */
                            result = createAndAppendNewNode(node, relationshipType, VT_byReference, constraintChecker);
                            /* read ReferencedContentItemIdentifier (again) */
                            if (result.good())
                                result = node->readContentItem(*ditem, logStream);
                        } else {
                            /* read ValueType (from DocumentContentMacro) - required to create new node */
                            result = getAndCheckStringValueFromDataset(*ditem, DCM_ValueType, tmpString, "1", "1", logStream);
                            if (result.good())
                            {
                                /* read by-value relationship */
                                valueType = definedTermToValueType(tmpString);
                                /* check value type */
                                if (valueType != VT_unknown)
                                {
                                    /* create new node (by-value) */
                                    result = createAndAppendNewNode(node, relationshipType, valueType, (flags & RF_ignoreRelationshipConstraints) ? NULL : constraintChecker);
                                    /* read RelationshipMacro */
                                    if (result.good())
                                    {
                                        result = node->readDocumentRelationshipMacro(*ditem, constraintChecker, location, flags, logStream);
                                        /* read DocumentContentMacro */
                                        if (result.good())
                                            result = node->readDocumentContentMacro(*ditem, location.c_str(), flags, logStream);
                                    } else {
                                        /* create new node failed */
                                        OFString message = "Cannot add \"";
                                        message += relationshipTypeToReadableName(relationshipType);
                                        message += " ";
                                        message += valueTypeToDefinedTerm(valueType /*target item*/);
                                        message += "\" to ";
                                        message += valueTypeToDefinedTerm(ValueType /*source item*/);
                                        message += " in ";
                                        /* determine document type */
                                        const E_DocumentType documentType = (constraintChecker != NULL) ? constraintChecker->getDocumentType() : DT_invalid;
                                        message += documentTypeToReadableName(documentType);
                                        printErrorMessage(logStream, message.c_str());
                                    }
                                } else {
                                    /* unknown/unsupported value type */
                                    printUnknownValueWarningMessage(logStream, "ValueType", tmpString.c_str());
                                    result = SR_EC_UnknownValueType;
                                }
                            }
                        }
                    }
                    /* check for any errors */
                    if (result.bad())
                    {
                        printContentItemErrorMessage(logStream, "Reading", result, node, location.c_str());
                        /* print current data set (item) that caused the error */
                        if ((logStream != NULL) && (flags & RF_verboseDebugMode))
                        {
                            logStream->lockCerr() << OFString(31, '-') << " DICOM DATA SET " << OFString(31, '-') << endl;
                            ditem->print(logStream->getCerr(), OFFalse /*showFullData*/, 1 /*level*/);
                            logStream->getCerr() << OFString(78, '-') << endl;
                            logStream->unlockCerr();
                        }
                    }
                } else
                    result = SR_EC_InvalidDocumentTree;
                i++;
            }
            /* skipping complete sub-tree if flag is set */
            if (result.bad() && (flags & RF_skipInvalidContentItems))
            {
                printInvalidContentItemMessage(logStream, "Skipping", node);
                result = EC_Normal;
            }
        }
    }
    return result;
}


OFCondition DSRDocumentTreeNode::writeContentSequence(DcmItem &dataset,
                                                      DcmStack *markedItems,
                                                      OFConsole *logStream) const
{
    OFCondition result = EC_Normal;
    /* goto first child of current node */
    DSRTreeNodeCursor cursor(Down);
    if (cursor.isValid())
    {
        /* write ContentSequence */
        DcmSequenceOfItems *dseq = new DcmSequenceOfItems(DCM_ContentSequence);
        if (dseq != NULL)
        {
            DcmItem *ditem = NULL;
            DSRDocumentTreeNode *node = NULL;
            do {        /* for all child nodes */
                node = OFstatic_cast(DSRDocumentTreeNode *, cursor.getNode());
                if (node != NULL)
                {
                    ditem = new DcmItem();
                    if (ditem != NULL)
                    {
                        /* write RelationshipType */
                        result = putStringValueToDataset(*ditem, DCM_RelationshipType, relationshipTypeToDefinedTerm(node->getRelationshipType()));
                        /* check for by-reference relationship */
                        if (node->getValueType() == VT_byReference)
                        {
                            /* write ReferencedContentItemIdentifier */
                            if (result.good())
                                result = node->writeContentItem(*ditem, logStream);
                        } else {    // by-value
                            /* write RelationshipMacro */
                            if (result.good())
                                result = node->writeDocumentRelationshipMacro(*ditem, markedItems, logStream);
                            /* write DocumentContentMacro */
                            if (result.good())
                                node->writeDocumentContentMacro(*ditem, logStream);
                        }
                        /* check for any errors */
                        if (result.bad())
                            printContentItemErrorMessage(logStream, "Writing", result, node);
                        /* insert item into sequence */
                        if (result.good())
                            dseq->insert(ditem);
                        else
                            delete ditem;
                    } else
                        result = EC_MemoryExhausted;
                } else
                    result = SR_EC_InvalidDocumentTree;
            } while (result.good() && cursor.gotoNext());
            if (result.good())
                result = dataset.insert(dseq, OFTrue /*replaceOld*/);
            if (result.bad())
                delete dseq;
        } else
            result = EC_MemoryExhausted;
    }
    return result;
}


OFCondition DSRDocumentTreeNode::renderHTMLConceptName(ostream &docStream,
                                                       const size_t flags,
                                                       OFConsole *logStream) const
{
    if (!(flags & HF_renderItemInline) && (flags & HF_renderItemsSeparately))
    {
        /* flag indicating whether line is empty or not */
        OFBool writeLine = OFFalse;
        if (!ConceptName.getCodeMeaning().empty())
        {
            docStream << "<b>";
            /* render ConceptName & Code (if valid) */
            ConceptName.renderHTML(docStream, flags, logStream, (flags & HF_renderConceptNameCodes) && ConceptName.isValid() /*fullCode*/);
            docStream << ":</b>";
            writeLine = OFTrue;
        }
        else if (flags & HF_currentlyInsideAnnex)
        {
            docStream << "<b>";
            /* render ValueType only */
            docStream << valueTypeToReadableName(ValueType);
            docStream << ":</b>";
            writeLine = OFTrue;
        }
        /* render optional observation datetime */
        if (!ObservationDateTime.empty())
        {
            if (writeLine)
                docStream << " ";
            OFString tmpString;
            docStream << "<small>(observed: " << dicomToReadableDateTime(ObservationDateTime, tmpString) << ")</small>";
            writeLine = OFTrue;
        }
        if (writeLine)
            docStream << "<br>" << endl;
    }
    return EC_Normal;
}


OFCondition DSRDocumentTreeNode::renderHTMLChildNodes(ostream &docStream,
                                                      ostream &annexStream,
                                                      const size_t nestingLevel,
                                                      size_t &annexNumber,
                                                      const size_t flags,
                                                      OFConsole *logStream) const
{
    OFCondition result = EC_Normal;
    /* goto first child of current node */
    DSRTreeNodeCursor cursor(Down);
    if (cursor.isValid())
    {
        /* flag used to format the relationship reference texts */
        OFBool paragraphFlag = (flags & HF_createFootnoteReferences) > 0;
        /* local version of flags */
        size_t newFlags = flags;
        /* footnote counter */
        size_t footnoteNumber = 1;
        /* create memory output stream for the temporal document */
        OFOStringStream tempDocStream;
        DSRDocumentTreeNode *node = NULL;
        do {        /* for all child nodes */
            node = OFstatic_cast(DSRDocumentTreeNode *, cursor.getNode());
            if (node != NULL)
            {
                /* set/reset flag for footnote creation*/
                newFlags &= ~HF_createFootnoteReferences;
                if (!(flags & HF_renderItemsSeparately) && node->hasChildNodes() && (node->getValueType() != VT_Container))
                    newFlags |= HF_createFootnoteReferences;
                /* render (optional) reference to annex */
                OFString relationshipText;
                if (!getRelationshipText(node->getRelationshipType(), relationshipText, flags).empty())
                {
                    if (paragraphFlag)
                    {
                        /* inside paragraph: line break */
                        docStream << "<br>" << endl;
                    } else {
                        /* open paragraph */
                        docStream << "<p><small>" << endl;
                        paragraphFlag = OFTrue;
                    }
                    docStream << "<u>" << relationshipText << "</u>: ";
                    /* expand short nodes with no children inline */
                    if (!(flags & HF_neverExpandChildrenInline) && !node->hasChildNodes() && node->isShort(flags))
                    {
                        if (node->getValueType() != VT_byReference)
                        {
                            /* render concept name/code or value type */
                            if (node->getConceptName().getCodeMeaning().empty())
                                docStream << valueTypeToReadableName(node->getValueType());
                            else
                                node->getConceptName().renderHTML(docStream, flags, logStream, (flags & HF_renderConceptNameCodes) && ConceptName.isValid() /*fullCode*/);
                            docStream << " = ";
                        }
                        /* render HTML code (directly to the reference text) */
                        result = node->renderHTML(docStream, annexStream, 0 /*nesting level*/, annexNumber, newFlags | HF_renderItemInline, logStream);
                    } else {
                        /* render concept name or value type */
                        if (node->getConceptName().getCodeMeaning().empty())
                            docStream << valueTypeToReadableName(node->getValueType()) << " ";
                        else
                            docStream << node->getConceptName().getCodeMeaning() << " ";
                        /* render annex heading and reference */
                        createHTMLAnnexEntry(docStream, annexStream, "" /*referenceText*/, annexNumber);
                        /* create memory output stream for the temporal annex */
                        OFOStringStream tempAnnexStream;
                        /* render HTML code (directly to the annex) */
                        result = node->renderHTML(annexStream, tempAnnexStream, 0 /*nesting level*/, annexNumber, newFlags | HF_currentlyInsideAnnex, logStream);
                        /* append temporary stream to main stream */
                        if (result.good())
                            result = appendStream(annexStream, tempAnnexStream);
                    }
                } else {
                    /* close paragraph */
                    if (paragraphFlag)
                    {
                        docStream << "</small></p>" << endl;
                        paragraphFlag = OFFalse;
                    }
                    /* begin new paragraph */
                    if (flags & HF_renderItemsSeparately)
                        docStream << "<p>" << endl;
                    /* write footnote text to temporary stream */
                    if (newFlags & HF_createFootnoteReferences)
                    {
                        /* render HTML code (without child nodes) */
                        result = node->renderHTMLContentItem(docStream, annexStream, 0 /*nestingLevel*/, annexNumber, newFlags, logStream);
                        /* create footnote numbers (individually for each child?) */
                        if (result.good())
                        {
                            /* tags are closed automatically in 'node->renderHTMLChildNodes()' */
                            tempDocStream << "<p><small>" << endl;
                            /* render footnote text and reference */
                            createHTMLFootnote(docStream, tempDocStream, footnoteNumber, node->getNodeID());
                            /* render child nodes to temporary stream */
                            result = node->renderHTMLChildNodes(tempDocStream, annexStream, 0 /*nestingLevel*/, annexNumber, newFlags, logStream);
                        }
                    } else {
                        /* render HTML code (incl. child nodes)*/
                        result = node->renderHTML(docStream, annexStream, nestingLevel + 1, annexNumber, newFlags, logStream);
                    }
                    /* end paragraph */
                    if (flags & HF_renderItemsSeparately)
                        docStream << "</p>" << endl;
                }
            } else
                result = SR_EC_InvalidDocumentTree;
        } while (result.good() && cursor.gotoNext());
        /* close last open paragraph (if any) */
        if (paragraphFlag)
            docStream << "</small></p>" << endl;
        /* append temporary stream to main stream */
        if (result.good())
            result = appendStream(docStream, tempDocStream);
    }
    return result;
}


const OFString &DSRDocumentTreeNode::getRelationshipText(const E_RelationshipType relationshipType,
                                                         OFString &relationshipText,
                                                         const size_t flags)
{
    switch (relationshipType)
    {
        case RT_contains:
            if (flags & HF_createFootnoteReferences)
                relationshipText = "Contains";
            else
                relationshipText.clear();
            break;
        case RT_hasObsContext:
            relationshipText = "Observation Context";
            break;
        case RT_hasAcqContext:
            relationshipText = "Acquisition Context";
            break;
        case RT_hasConceptMod:
            relationshipText = "Concept Modifier";
            break;
        case RT_hasProperties:
            relationshipText = "Properties";
            break;
        case RT_inferredFrom:
            relationshipText = "Inferred from";
            break;
        case RT_selectedFrom:
            relationshipText = "Selected from";
            break;
        default:
            relationshipText.clear();
            break;
    }
    return relationshipText;
}


OFBool DSRDocumentTreeNode::containsExtendedCharacters() const
{
  return ConceptName.valueContainsExtendedCharacters();
}


/*
 *  CVS/RCS Log:
 *  $Log: dsrdoctn.cc,v $
 *  Revision 1.41  2005/12/08 15:47:49  meichel
 *  Changed include path schema for all DCMTK header files
 *
 *  Revision 1.40  2004/11/22 16:39:12  meichel
 *  Added method that checks if the SR document contains non-ASCII characters
 *    in any of the strings affected by SpecificCharacterSet.
 *
 *  Revision 1.39  2004/09/09 14:03:19  joergr
 *  Added flags to control the way the template identification is encoded in
 *  writeXML() and expected in readXML().
 *
 *  Revision 1.38  2004/01/20 15:37:39  joergr
 *  Added new command line option which allows to write the item identifier "id"
 *  (XML attribute) even if it is not required (because the item is not referenced
 *  by any other item). Useful for debugging purposes.
 *
 *  Revision 1.37  2004/01/16 10:17:04  joergr
 *  Adapted XML output format of Date, Time and Datetime to XML Schema (ISO
 *  8601) requirements.
 *
 *  Revision 1.36  2004/01/05 14:37:23  joergr
 *  Removed acknowledgements with e-mail addresses from CVS log.
 *
 *  Revision 1.35  2003/12/05 14:02:36  joergr
 *  Only report warning on incorrect template identifier when actually expecting
 *  one (i.e. only for particular SOP classes).
 *
 *  Revision 1.34  2003/12/01 15:47:28  joergr
 *  Changed XML encoding of by-reference relationships if flag
 *  XF_valueTypeAsAttribute is set.
 *
 *  Revision 1.33  2003/10/30 17:58:58  joergr
 *  Added full support for the ContentTemplateSequence (read/write, get/set
 *  template identification). Template constraints are not checked yet.
 *  Fixed bug in setConceptName() that caused to return EC_IllegalParameter
 *  even for valid parameters.
 *
 *  Revision 1.32  2003/10/09 17:48:16  joergr
 *  Slightly modified warning message text in readDocumentRelationshipMacro().
 *
 *  Revision 1.31  2003/10/09 14:17:59  joergr
 *  Changed message type for incorrect/missing TemplateIdentifier from error to
 *  warning.
 *
 *  Revision 1.30  2003/10/09 13:00:41  joergr
 *  Added check for root template identifier when reading an SR document.
 *
 *  Revision 1.29  2003/10/06 09:55:35  joergr
 *  Added new flag which allows to ignore content item errors when reading an SR
 *  document (e.g. missing value type specific attributes).
 *
 *  Revision 1.28  2003/09/15 14:13:42  joergr
 *  Introduced new class to facilitate checking of SR IOD relationship content
 *  constraints. Replaced old implementation distributed over numerous classes.
 *
 *  Revision 1.27  2003/09/10 13:18:43  joergr
 *  Replaced PrivateCodingSchemeUID by new CodingSchemeIdenticationSequence as
 *  required by CP 324.
 *
 *  Revision 1.26  2003/08/07 17:29:13  joergr
 *  Removed libxml dependency from header files. Simplifies linking (MSVC).
 *
 *  Revision 1.25  2003/08/07 15:21:53  joergr
 *  Added brackets around "bitwise and" operator/operands to avoid warnings
 *  reported by MSVC5.
 *
 *  Revision 1.24  2003/08/07 13:29:43  joergr
 *  Added readXML functionality.
 *  Adapted type casts to new-style typecast operators defined in ofcast.h.
 *  Distinguish more strictly between OFBool and int (required when HAVE_CXX_BOOL
 *  is defined).
 *
 *  Revision 1.23  2002/10/23 13:21:03  joergr
 *  Fixed bug in debug output of read() routines.
 *
 *  Revision 1.22  2002/08/02 15:06:02  joergr
 *  Fixed problems reported by Sun CC 2.0.1.
 *
 *  Revision 1.21  2002/08/02 12:39:06  joergr
 *  Enhanced debug output of dcmsr::read() routines (e.g. add position string
 *  of invalid content items to error messages).
 *
 *  Revision 1.20  2002/05/02 14:08:35  joergr
 *  Added support for standard and non-standard string streams (which one is
 *  supported is detected automatically via the configure mechanism).
 *
 *  Revision 1.19  2001/11/09 16:15:19  joergr
 *  Added new command line option allowing to encode codes as XML attributes
 *  (instead of tags).
 *
 *  Revision 1.18  2001/10/10 15:29:53  joergr
 *  Additonal adjustments for new OFCondition class.
 *
 *  Revision 1.17  2001/10/02 12:07:08  joergr
 *  Adapted module "dcmsr" to the new class OFCondition. Introduced module
 *  specific error codes.
 *
 *  Revision 1.16  2001/09/28 14:10:29  joergr
 *  Check return value of DcmItem::insert() statements to avoid memory leaks
 *  when insert procedure failes.
 *
 *  Revision 1.15  2001/09/26 13:04:20  meichel
 *  Adapted dcmsr to class OFCondition
 *
 *  Revision 1.14  2001/06/20 15:04:25  joergr
 *  Added minimal support for new SOP class Key Object Selection Document
 *  (suppl. 59).
 *  Added new debugging features (additional flags) to examine "corrupted" SR
 *  documents.
 *
 *  Revision 1.13  2001/04/03 08:25:19  joergr
 *  Added new command line option: ignore relationship content constraints
 *  specified for each SR document class.
 *
 *  Revision 1.12  2001/02/02 14:41:53  joergr
 *  Added new option to dsr2xml allowing to specify whether value and/or
 *  relationship type are to be encoded as XML attributes or elements.
 *
 *  Revision 1.11  2001/01/18 15:55:20  joergr
 *  Added support for digital signatures.
 *
 *  Revision 1.10  2000/11/13 10:27:00  joergr
 *  Added output of optional observation datetime to rendered HTML page.
 *
 *  Revision 1.9  2000/11/09 20:34:00  joergr
 *  Added support for non-ASCII characters in HTML 3.2 (use numeric value).
 *
 *  Revision 1.8  2000/11/09 11:32:45  joergr
 *  Minor HTML code purifications.
 *
 *  Revision 1.7  2000/11/07 18:33:29  joergr
 *  Enhanced support for by-reference relationships.
 *
 *  Revision 1.6  2000/11/01 16:33:38  joergr
 *  Added support for conversion to XML.
 *
 *  Revision 1.5  2000/10/26 14:29:20  joergr
 *  Added support for "Comprehensive SR".
 *
 *  Revision 1.4  2000/10/23 15:03:36  joergr
 *  Allow to set empty concept name code (= clear).
 *
 *  Revision 1.3  2000/10/18 17:16:08  joergr
 *  Added check for read methods (VM and type).
 *
 *  Revision 1.2  2000/10/16 12:03:29  joergr
 *  Reformatted print output.
 *
 *  Revision 1.1  2000/10/13 07:52:19  joergr
 *  Added new module 'dcmsr' providing access to DICOM structured reporting
 *  documents (supplement 23).  Doc++ documentation not yet completed.
 *
 *
 */
