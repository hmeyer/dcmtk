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
 *    classes: DSRDocumentTree
 *
 *  Last Update:      $Author: meichel $
 *  Update Date:      $Date: 2005/12/08 15:47:50 $
 *  CVS/RCS Revision: $Revision: 1.26 $
 *  Status:           $State: Exp $
 *
 *  CVS/RCS Log at end of file
 *
 */


#include "dcmtk/config/osconfig.h"    /* make sure OS specific configuration is included first */

#include "dcmtk/dcmsr/dsrdoctr.h"
#include "dcmtk/dcmsr/dsrcontn.h"
#include "dcmtk/dcmsr/dsrreftn.h"
#include "dcmtk/dcmsr/dsrxmld.h"
#include "dcmtk/dcmsr/dsriodcc.h"


DSRDocumentTree::DSRDocumentTree(const E_DocumentType documentType)
  : DSRTree(),
    DocumentType(DT_invalid),
    LogStream(NULL),
    CurrentContentItem(),
    ConstraintChecker(NULL)
{
    /* check & set document type, create constraint checker object */
    changeDocumentType(documentType);
}


DSRDocumentTree::~DSRDocumentTree()
{
    delete ConstraintChecker;
}


void DSRDocumentTree::clear()
{
    DSRTree::clear();
}


OFBool DSRDocumentTree::isValid() const
{
    OFBool result = OFFalse;
    if (isDocumentTypeSupported(DocumentType))
    {
        /* check root node */
        const DSRDocumentTreeNode *node = OFstatic_cast(DSRDocumentTreeNode *, getRoot());
        if (node != NULL)
        {
            if ((node->getRelationshipType() == RT_isRoot) && (node->getValueType() == VT_Container))
                result = OFTrue;
        }
    }
    return result;
}


void DSRDocumentTree::setLogStream(OFConsole *stream)
{
    LogStream = stream;
}


OFCondition DSRDocumentTree::print(ostream &stream,
                                   const size_t flags)
{
    OFCondition result = EC_Normal;
    DSRTreeNodeCursor cursor(getRoot());
    if (cursor.isValid())
    {
        /* check and update by-reference relationships (if applicable) */
        checkByReferenceRelationships(OFTrue /*updateString*/,  OFFalse /*updateNodeID*/);
        OFString tmpString;
        size_t level = 0;
        const DSRDocumentTreeNode *node = NULL;
        /* iterate over all nodes */
        do {
            node = OFstatic_cast(DSRDocumentTreeNode *, cursor.getNode());
            if (node != NULL)
            {
                /* print node position */
                if (flags & PF_printItemPosition)
                    stream << cursor.getPosition(tmpString) << "  ";
                else
                {
                    /* use line identation */
                    level = cursor.getLevel();
                    if (level > 0)  // valid ?
                        stream << OFString((level - 1) * 2, ' ');
                }
                /* print node content */
                stream << "<";
                result = node->print(stream, flags);
                stream << ">";
                if (flags & PF_printTemplateIdentification)
                {
                    /* check for template identification */
                    OFString templateIdentifier, mappingResource;
                    if (node->getTemplateIdentification(templateIdentifier, mappingResource).good())
                    {
                        if (!templateIdentifier.empty() && !mappingResource.empty())
                            stream << "  # TID " << templateIdentifier << " (" << mappingResource << ")";
                    }
                }
                stream << endl;
            } else
                result = SR_EC_InvalidDocumentTree;
        } while (result.good() && cursor.iterate());
    }
    return result;
}


OFCondition DSRDocumentTree::read(DcmItem &dataset,
                                  const E_DocumentType documentType,
                                  const size_t flags)
{
    /* clear current document tree, check & change document type */
    OFCondition result = changeDocumentType(documentType);
    if (result.good())
    {
        if (ConstraintChecker == NULL)
            printWarningMessage(LogStream, "Check for relationship content constraints not yet supported");
        else if (ConstraintChecker->isTemplateSupportRequired())
            printWarningMessage(LogStream, "Check for template constraints not yet supported");
        if ((LogStream != NULL) && (flags & RF_showCurrentlyProcessedItem))
        {
            LogStream->lockCerr() << "Processing content item 1" << endl;
            LogStream->unlockCerr();
        }
        /* first try to read value type */
        OFString tmpString;
        if (getAndCheckStringValueFromDataset(dataset, DCM_ValueType, tmpString, "1", "1", LogStream).good())
        {
            /* root node should always be a container */
            if (definedTermToValueType(tmpString) == VT_Container)
            {
                /* ... then create corresponding document tree node */
                DSRDocumentTreeNode *node = new DSRContainerTreeNode(RT_isRoot);
                if (node != NULL)
                {
                    /* ... insert it into the (empty) tree - checking is not required here */
                    if (addNode(node))
                    {
                        /* ... and let the node read the rest of the document */
                        result = node->read(dataset, ConstraintChecker, flags, LogStream);
                        /* check and update by-reference relationships (if applicable) */
                        checkByReferenceRelationships(OFFalse /*updateString*/, OFTrue /*updateNodeID*/);
                    } else
                        result = SR_EC_InvalidDocumentTree;
                } else
                    result = EC_MemoryExhausted;
            } else {
                printErrorMessage(LogStream, "Root content item should always be a CONTAINER");
                result = SR_EC_InvalidDocumentTree;
            }
        } else {
            printErrorMessage(LogStream, "ValueType attribute for root content item is missing");
            result = SR_EC_MandatoryAttributeMissing;
        }
    }
    return result;
}


OFCondition DSRDocumentTree::readXML(const DSRXMLDocument &doc,
                                     DSRXMLCursor cursor,
                                     const size_t flags)
{
    OFCondition result = SR_EC_CorruptedXMLStructure;
    if (ConstraintChecker == NULL)
        printWarningMessage(LogStream, "Check for relationship content constraints not yet supported");
    else if (ConstraintChecker->isTemplateSupportRequired())
        printWarningMessage(LogStream, "Check for template constraints not yet supported");
    /* we assume that 'cursor' points to the "content" element */
    if (cursor.valid())
    {
        OFString templateIdentifier, mappingResource;
        /* template identification information expected "outside" content item */
        if (flags & XF_templateElementEnclosesItems)
        {
            /* check for optional root template identification */
            const DSRXMLCursor childCursor = doc.getNamedNode(cursor, "template", OFFalse /*required*/);
            if (childCursor.valid())
            {
                doc.getStringFromAttribute(childCursor, mappingResource, "resource");
                doc.getStringFromAttribute(childCursor, templateIdentifier, "tid");
                /* get first child of the "template" element */
                cursor = childCursor.getChild();
            }
        }
        E_ValueType valueType = doc.getValueTypeFromNode(cursor);
        /* proceed to first valid container (if any) */
        while (cursor.getNext().valid() && (valueType != VT_Container))
            valueType = doc.getValueTypeFromNode(cursor.gotoNext());
        /* root node should always be a container */
        if (valueType == VT_Container)
        {
            /* ... then create corresponding document tree node */
            DSRDocumentTreeNode *node = new DSRContainerTreeNode(RT_isRoot);
            if (node != NULL)
            {
                /* ... insert it into the (empty) tree - checking is not required here */
                if (addNode(node))
                {
                    if (flags & XF_templateElementEnclosesItems)
                    {
                        /* set template identification (if any) */
                        if (node->setTemplateIdentification(templateIdentifier, mappingResource).bad())
                            printWarningMessage(LogStream, "Root content item has invalid/incomplete template identification");
                    }
                    /* ... and let the node read the rest of the document */
                    result = node->readXML(doc, cursor, DocumentType, flags);
                    /* check and update by-reference relationships (if applicable) */
                    checkByReferenceRelationships(OFTrue /*updateString*/,  OFFalse /*updateNodeID*/);
                } else
                    result = SR_EC_InvalidDocumentTree;
            } else
                result = EC_MemoryExhausted;
        } else {
            printErrorMessage(LogStream, "Root content item should always be a CONTAINER");
            result = SR_EC_InvalidDocumentTree;
        }
    }
    return result;
}


OFCondition DSRDocumentTree::write(DcmItem &dataset,
                                   DcmStack *markedItems)
{
    OFCondition result = SR_EC_InvalidDocumentTree;
    /* check whether root node has correct relationship and value type */
    if (isValid())
    {
        DSRDocumentTreeNode *node = OFstatic_cast(DSRDocumentTreeNode *, getRoot());
        if (node != NULL)
        {
            /* check and update by-reference relationships (if applicable) */
            checkByReferenceRelationships(OFTrue /*updateString*/,  OFFalse /*updateNodeID*/);
            /* start writing from root node */
            result = node->write(dataset, markedItems, LogStream);
        }
    }
    return result;
}


OFCondition DSRDocumentTree::writeXML(ostream &stream,
                                      const size_t flags)
{
    OFCondition result = SR_EC_InvalidDocumentTree;
    /* check whether root node has correct relationship and value type */
    if (isValid())
    {
        DSRDocumentTreeNode *node = OFstatic_cast(DSRDocumentTreeNode *, getRoot());
        /* start writing from root node */
        if (node != NULL)
        {
            /* check by-reference relationships (if applicable) */
            checkByReferenceRelationships(OFFalse /*updateString*/,  OFFalse /*updateNodeID*/);
            /* start writing from root node */
            result = node->writeXML(stream, flags, LogStream);
        }
    }
    return result;
}


OFCondition DSRDocumentTree::renderHTML(ostream &docStream,
                                        ostream &annexStream,
                                        const size_t flags)
{
    OFCondition result = SR_EC_InvalidDocumentTree;
    /* check whether root node has correct relationship and value type */
    if (isValid())
    {
        DSRDocumentTreeNode *node = OFstatic_cast(DSRDocumentTreeNode *, getRoot());
        /* start rendering from root node */
        if (node != NULL)
        {
            /* check by-reference relationships (if applicable) */
            checkByReferenceRelationships(OFFalse /*updateString*/,  OFFalse /*updateNodeID*/);
            size_t annexNumber = 1;
            /* start rendering from root node */
            result = node->renderHTML(docStream, annexStream, 1 /*nestingLevel*/, annexNumber, flags & ~HF_internalUseOnly, LogStream);
        }
    }
    return result;
}


OFCondition DSRDocumentTree::changeDocumentType(const E_DocumentType documentType)
{
    OFCondition result = SR_EC_UnsupportedValue;
    /* first, check whether new document type is supported at all */
    if (isDocumentTypeSupported(documentType))
    {
        /* clear object */
        clear();
        /* store new document type */
        DocumentType = documentType;
        /* create appropriate IOD constraint checker */
        delete ConstraintChecker;
        ConstraintChecker = createIODConstraintChecker(documentType);
        result = EC_Normal;
    }
    return result;
}


OFBool DSRDocumentTree::canAddContentItem(const E_RelationshipType relationshipType,
                                          const E_ValueType valueType,
                                          const E_AddMode addMode)
{
    OFBool result = OFFalse;
    const DSRDocumentTreeNode *node = OFstatic_cast(const DSRDocumentTreeNode *, getNode());
    if (node != NULL)
    {
        if (ConstraintChecker != NULL)
        {
            if ((addMode == AM_beforeCurrent) || (addMode == AM_afterCurrent))
            {
                /* check parent node */
                node = OFstatic_cast(const DSRDocumentTreeNode *, getParentNode());
                if (node != NULL)
                    result = ConstraintChecker->checkContentRelationship(node->getValueType(), relationshipType, valueType);
            } else
                result = ConstraintChecker->checkContentRelationship(node->getValueType(), relationshipType, valueType);
        } else
            result = OFTrue;    /* cannot check, therefore, allow everything */
    } else {
        /* root node has to be a Container */
        result = (relationshipType == RT_isRoot) && (valueType == VT_Container);
    }
    return result;
}


OFBool DSRDocumentTree::canAddByReferenceRelationship(const E_RelationshipType relationshipType,
                                                      const E_ValueType targetValueType)
{
    OFBool result = OFFalse;
    if (ConstraintChecker != NULL)
    {
        const DSRDocumentTreeNode *node = OFstatic_cast(const DSRDocumentTreeNode *, getNode());
        if (node != NULL)
            result = ConstraintChecker->checkContentRelationship(node->getValueType(), relationshipType, targetValueType, OFTrue /*byReference*/);
    } else
        result = OFTrue;    /* cannot check, therefore, allow everything */
    return result;
}


size_t DSRDocumentTree::addContentItem(const E_RelationshipType relationshipType,
                                       const E_ValueType valueType,
                                       const E_AddMode addMode)
{
    size_t nodeID = 0;
    if (canAddContentItem(relationshipType, valueType, addMode))
    {
        /* create a new node ... */
        DSRDocumentTreeNode *node = createDocumentTreeNode(relationshipType, valueType);
        if (node != NULL)
        {
            /* ... and add it to the current node */
            nodeID = addNode(node, addMode);
        }
    }
    return nodeID;
}


size_t DSRDocumentTree::addByReferenceRelationship(const E_RelationshipType relationshipType,
                                                   const size_t referencedNodeID)
{
    size_t nodeID = 0;
    if (referencedNodeID > 0)
    {
        DSRTreeNodeCursor cursor(getRoot());
        if (cursor.isValid())
        {
            /* goto specified target node (might be improved later on) */
            if (cursor.gotoNode(referencedNodeID))
            {
                OFString sourceString;
                OFString targetString;
                getPosition(sourceString);
                cursor.getPosition(targetString);
                /* check whether target node is an ancestor of source node (prevent loops) */
                if (sourceString.substr(0, targetString.length()) != targetString)
                {
                    const DSRDocumentTreeNode *targetNode = OFstatic_cast(DSRDocumentTreeNode *, cursor.getNode());
                    if (targetNode != NULL)
                    {
                        /* check whether relationship is valid/allowed */
                        if (canAddByReferenceRelationship(relationshipType, targetNode->getValueType()))
                        {
                            DSRDocumentTreeNode *node = new DSRByReferenceTreeNode(relationshipType, referencedNodeID);
                            if (node != NULL)
                            {
                                nodeID = addNode(node, AM_belowCurrent);
                                /* go back to current node */
                                if (nodeID > 0)
                                    goUp();
                            }
                        }
                    }
                }
            }
        }
    }
    return nodeID;
}


size_t DSRDocumentTree::removeCurrentContentItem()
{
    return removeNode();
}


DSRContentItem &DSRDocumentTree::getCurrentContentItem()
{
    CurrentContentItem.setTreeNode(OFstatic_cast(DSRDocumentTreeNode *, getNode()));
    return CurrentContentItem;
}


size_t DSRDocumentTree::gotoNamedNode(const DSRCodedEntryValue &conceptName,
                                      const OFBool startFromRoot,
                                      const OFBool searchIntoSub)
{
    size_t nodeID = 0;
    if (conceptName.isValid())
    {
        if (startFromRoot)
            gotoRoot();
        DSRDocumentTreeNode *node = NULL;
        clearNodeCursorStack();
        /* iterate over all nodes */
        do {
            node = OFstatic_cast(DSRDocumentTreeNode *, getNode());
            if ((node != NULL) && (node->getConceptName() == conceptName))
                nodeID = node->getNodeID();
        } while ((nodeID == 0) && iterate(searchIntoSub));
    }
    return nodeID;
}


size_t DSRDocumentTree::gotoNextNamedNode(const DSRCodedEntryValue &conceptName,
                                          const OFBool searchIntoSub)
{
    /* first, goto "next" node */
    size_t nodeID = iterate(searchIntoSub);
    if (nodeID > 0)
        nodeID = gotoNamedNode(conceptName, OFFalse /*startFromRoot*/, searchIntoSub);
    return nodeID;
}


void DSRDocumentTree::unmarkAllContentItems()
{
    DSRTreeNodeCursor cursor(getRoot());
    if (cursor.isValid())
    {
        DSRDocumentTreeNode *node = NULL;
        /* iterate over all nodes */
        do {
            node = OFstatic_cast(DSRDocumentTreeNode *, cursor.getNode());
            if (node != NULL)
                node->setMark(OFFalse);
        } while (cursor.iterate());
    }
}


void DSRDocumentTree::removeSignatures()
{
    DSRTreeNodeCursor cursor(getRoot());
    if (cursor.isValid())
    {
        DSRDocumentTreeNode *node = NULL;
        /* iterate over all nodes */
        do {
            node = OFstatic_cast(DSRDocumentTreeNode *, cursor.getNode());
            if (node != NULL)
                node->removeSignatures();
        } while (cursor.iterate());
    }
}


size_t DSRDocumentTree::addNode(DSRTreeNode * /*node*/,
                                const E_AddMode /*addMode*/)
{
    /* invalid for this class */
    return 0;
}


size_t DSRDocumentTree::addNode(DSRDocumentTreeNode *node,
                                const E_AddMode addMode)
{
    /* might add a check for templates later on */
    return DSRTree::addNode(node, addMode);
}


size_t DSRDocumentTree::removeNode()
{
    /* might add a check for templates later on */
    return DSRTree::removeNode();
}


OFCondition DSRDocumentTree::checkByReferenceRelationships(const OFBool updateString,
                                                           const OFBool updateNodeID)
{
    OFCondition result = EC_IllegalParameter;
    /* the flags are mutually exclusive */
    if (!(updateString && updateNodeID))
    {
        result = EC_Normal;
        /* by-reference relationships are only allowed for particular IODs */
        if ((ConstraintChecker != NULL) && ConstraintChecker->isByReferenceAllowed())
        {
            DSRTreeNodeCursor cursor(getRoot());
            if (cursor.isValid())
            {
                const DSRDocumentTreeNode *node = NULL;
                do {    /* for all content items */
                    node = OFstatic_cast(DSRDocumentTreeNode *, cursor.getNode());
                    if (node != NULL)
                    {
                        /* only check/update by-reference relationships */
                        if (node->getValueType() == VT_byReference)
                        {
                            size_t refNodeID = 0;
                            /* type cast to directly access member variables of by-reference class */
                            DSRByReferenceTreeNode *refNode = OFconst_cast(DSRByReferenceTreeNode *, OFstatic_cast(const DSRByReferenceTreeNode *, node));
                            /* start searching from root node (be careful with large trees, might be improved later on) */
                            DSRTreeNodeCursor refCursor(getRoot());
                            if (updateNodeID)
                            {
                                /* update node ID */
                                refNodeID = refCursor.gotoNode(refNode->ReferencedContentItem);
                                if (refNodeID > 0)
                                    refNode->ReferencedNodeID = refCursor.getNodeID();
                                else
                                    refNode->ReferencedNodeID = 0;
                                refNode->ValidReference = (refNode->ReferencedNodeID > 0);
                            } else {
                                /* ReferenceNodeID contains a valid value */
                                refNodeID = refCursor.gotoNode(refNode->ReferencedNodeID);
                                if (updateString)
                                {
                                    /* update position string */
                                    if (refNodeID > 0)
                                        refCursor.getPosition(refNode->ReferencedContentItem);
                                    else
                                        refNode->ReferencedContentItem.clear();
                                    refNode->ValidReference = checkForValidUIDFormat(refNode->ReferencedContentItem);
                                } else if (refNodeID == 0)
                                    refNode->ValidReference = OFFalse;
                            }
                            if (refNodeID > 0)
                            {
                                /* source and target content items should not be identical */
                                if (refNodeID != cursor.getNodeID())
                                {
                                    OFString posString;
                                    cursor.getPosition(posString);
                                    /* check whether target node is an ancestor of source node (prevent loops) */
                                    if (posString.substr(0, refNode->ReferencedContentItem.length()) != refNode->ReferencedContentItem)
                                    {
                                        /* refCursor should now point to the reference target (refNodeID > 0) */
                                        const DSRDocumentTreeNode *parentNode = OFstatic_cast(const DSRDocumentTreeNode *, cursor.getParentNode());
                                        DSRDocumentTreeNode *targetNode = OFstatic_cast(DSRDocumentTreeNode *, refCursor.getNode());
                                        if ((parentNode != NULL) && (targetNode != NULL))
                                        {
                                            /* tbd: need to reset flag to OFFalse!? */
                                            targetNode->setReferenceTarget();
                                            /* check whether relationship is valid */
                                            if ((ConstraintChecker != NULL) && !ConstraintChecker->checkContentRelationship(parentNode->getValueType(),
                                                refNode->getRelationshipType(), targetNode->getValueType(), OFTrue /*byReference*/))
                                            {
                                                OFString message = "Invalid by-reference relationship between item \"";
                                                message += posString;
                                                message += "\" and \"";
                                                message += refNode->ReferencedContentItem;
                                                message += "\"";
                                                printWarningMessage(LogStream, message.c_str());
                                            }
                                        } else
                                            printWarningMessage(LogStream, "Corrupted data structures while checking by-reference relationships");
                                    } else
                                        printWarningMessage(LogStream, "By-reference relationship to ancestor content item (loop check)");
                                } else
                                    printWarningMessage(LogStream, "Source and target content item of by-reference relationship are identical");
                            } else
                                printWarningMessage(LogStream, "Target content item of by-reference relationship does not exist");
                        }
                    } else
                        result = SR_EC_InvalidDocumentTree;
                } while (result.good() && cursor.iterate());
            }
        }
    }
    return result;
}


OFBool DSRDocumentTree::containsExtendedCharacters()
{
  DSRTreeNodeCursor cursor(getRoot());
  if (cursor.isValid())
  {
      const DSRDocumentTreeNode *node = NULL;
      /* iterate over all nodes */
      do
      {
          node = OFstatic_cast(DSRDocumentTreeNode *, cursor.getNode());
          if (node && node->containsExtendedCharacters()) return OFTrue;
      } while (cursor.iterate());
  }
  return OFFalse;
}


/*
 *  CVS/RCS Log:
 *  $Log: dsrdoctr.cc,v $
 *  Revision 1.26  2005/12/08 15:47:50  meichel
 *  Changed include path schema for all DCMTK header files
 *
 *  Revision 1.25  2005/07/27 16:39:16  joergr
 *  Added methods that allow to go to a named node, i.e. using a given concept
 *  name.
 *
 *  Revision 1.24  2004/11/22 16:39:12  meichel
 *  Added method that checks if the SR document contains non-ASCII characters
 *    in any of the strings affected by SpecificCharacterSet.
 *
 *  Revision 1.23  2004/11/18 13:53:41  joergr
 *  Enhanced warning message for invalid by-reference relationships by adding the
 *  relevant item identifiers.
 *
 *  Revision 1.22  2004/09/09 14:03:19  joergr
 *  Added flags to control the way the template identification is encoded in
 *  writeXML() and expected in readXML().
 *
 *  Revision 1.21  2003/10/30 17:59:37  joergr
 *  Added full support for the ContentTemplateSequence (read/write, get/set
 *  template identification). Template constraints are not checked yet.
 *
 *  Revision 1.20  2003/09/15 14:13:42  joergr
 *  Introduced new class to facilitate checking of SR IOD relationship content
 *  constraints. Replaced old implementation distributed over numerous classes.
 *
 *  Revision 1.19  2003/08/07 17:29:13  joergr
 *  Removed libxml dependency from header files. Simplifies linking (MSVC).
 *
 *  Revision 1.18  2003/08/07 13:31:40  joergr
 *  Added readXML functionality.
 *  Renamed parameters/variables "string" to avoid name clash with STL class.
 *  Adapted type casts to new-style typecast operators defined in ofcast.h.
 *
 *  Revision 1.17  2002/12/05 13:55:13  joergr
 *  Added missing "processing ..." message for the root content item.
 *
 *  Revision 1.16  2001/11/09 16:15:42  joergr
 *  Added preliminary support for Mammography CAD SR.
 *
 *  Revision 1.15  2001/10/10 15:29:54  joergr
 *  Additonal adjustments for new OFCondition class.
 *
 *  Revision 1.14  2001/10/02 12:07:09  joergr
 *  Adapted module "dcmsr" to the new class OFCondition. Introduced module
 *  specific error codes.
 *
 *  Revision 1.13  2001/09/26 13:04:20  meichel
 *  Adapted dcmsr to class OFCondition
 *
 *  Revision 1.12  2001/06/20 15:05:48  joergr
 *  Added minimal support for new SOP class Key Object Selection Document
 *  (suppl. 59).
 *
 *  Revision 1.11  2001/06/07 14:35:01  joergr
 *  Removed unused variable (reported by gcc 2.5.8 on NeXTSTEP).
 *
 *  Revision 1.10  2001/04/03 08:25:19  joergr
 *  Added new command line option: ignore relationship content constraints
 *  specified for each SR document class.
 *
 *  Revision 1.9  2001/02/13 16:35:05  joergr
 *  Corrected wrong implementation of getLevel() - started from 0 instead of 1.
 *
 *  Revision 1.8  2001/01/18 15:55:48  joergr
 *  Added support for digital signatures.
 *
 *  Revision 1.7  2000/11/07 18:33:30  joergr
 *  Enhanced support for by-reference relationships.
 *
 *  Revision 1.6  2000/11/01 16:34:12  joergr
 *  Added support for conversion to XML.
 *
 *  Revision 1.5  2000/10/26 14:29:49  joergr
 *  Added support for "Comprehensive SR".
 *
 *  Revision 1.4  2000/10/18 17:16:08  joergr
 *  Added check for read methods (VM and type).
 *
 *  Revision 1.3  2000/10/16 16:32:56  joergr
 *  Added const type specifier.
 *
 *  Revision 1.2  2000/10/16 12:04:14  joergr
 *  Added new options: number nested items instead of indenting them, print SOP
 *  instance UID of referenced composite objects.
 *
 *  Revision 1.1  2000/10/13 07:52:20  joergr
 *  Added new module 'dcmsr' providing access to DICOM structured reporting
 *  documents (supplement 23).  Doc++ documentation not yet completed.
 *
 *
 */
