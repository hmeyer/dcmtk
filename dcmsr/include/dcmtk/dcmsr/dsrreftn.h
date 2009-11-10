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
 *    classes: DSRByReferenceTreeNode
 *
 *  Last Update:      $Author: meichel $
 *  Update Date:      $Date: 2005/12/08 16:05:14 $
 *  CVS/RCS Revision: $Revision: 1.10 $
 *  Status:           $State: Exp $
 *
 *  CVS/RCS Log at end of file
 *
 */


#ifndef DSRREFTN_H
#define DSRREFTN_H

#include "dcmtk/config/osconfig.h"   /* make sure OS specific configuration is included first */

#include "dcmtk/dcmsr/dsrdoctr.h"
#include "dcmtk/dcmsr/dsrdoctn.h"


/*---------------------*
 *  class declaration  *
 *---------------------*/

/** Class for by-reference relationships
 */
class DSRByReferenceTreeNode
  : public DSRDocumentTreeNode
{
    // allow access to private member variables
    friend class DSRDocumentTree;

  public:

    /** constructor
     ** @param  relationshipType  type of relationship to the parent/source tree node.
     *                            Should not be RT_invalid or RT_isRoot.
     */
    DSRByReferenceTreeNode(const E_RelationshipType relationshipType);

    /** constructor
     ** @param  relationshipType  type of relationship to the parent/source tree node.
     *                            Should not be RT_invalid or RT_isRoot.
     *  @param  referencedNodeID  ID of the node to be referenced
     */
    DSRByReferenceTreeNode(const E_RelationshipType relationshipType,
                           const size_t referencedNodeID);

    /** destructor
     */
    virtual ~DSRByReferenceTreeNode();

    /** clear all member variables.
     *  Please note that the content item becomes invalid afterwards.
     */
    virtual void clear();

    /** check whether the content item is valid.
     *  The content item is valid if the base class is valid, the concept name is
     *  empty and the reference (checked from outside this class) is valid.
     ** @return OFTrue if tree node is valid, OFFalse otherwise
     */
    virtual OFBool isValid() const;

    /** print content item.
     *  A typical output looks like this: inferred from 1.2.3
     ** @param  stream  output stream to which the content item should be printed
     *  @param  flags   flag used to customize the output (see DSRTypes::PF_xxx)
     ** @return status, EC_Normal if successful, an error code otherwise
     */
    virtual OFCondition print(ostream &stream,
                              const size_t flags) const;

    /** write content item in XML format
     ** @param  stream     output stream to which the XML document is written
     *  @param  flags      flag used to customize the output (see DSRTypes::XF_xxx)
     *  @param  logStream  pointer to error/warning output stream (output disabled if NULL)
     ** @return status, EC_Normal if successful, an error code otherwise
     */
    virtual OFCondition writeXML(ostream &stream,
                                 const size_t flags,
                                 OFConsole *logStream) const;

    /** set the concept name
     ** @param  conceptName  dummy parameter
     ** @return always returns EC_IllegalCall, since this content item has no concept name
     */
    virtual OFCondition setConceptName(const DSRCodedEntryValue &conceptName);

    /** set observation date time
     ** @param  observationDateTime  dummy parameter
     ** @return always returns EC_IllegalCall, since this content item has no observation
     *          date and time (part of Document Relationship Macro)
     */
    virtual OFCondition setObservationDateTime(const OFString &observationDateTime);

    /** set template identifier and mapping resource
     ** @param  templateIdentifier  dummy parameter
     *  @param  mappingResource     dummy parameter
     ** @return always returns EC_IllegalCall, since this content item has no template
     *          identification (part of Document Relationship Macro)
     */
    virtual OFCondition setTemplateIdentification(const OFString &templateIdentifier,
                                                  const OFString &mappingResource);

    /** get ID of the referenced node
     ** @return ID of the referenced node if valid, 0 otherwise
     */
    size_t getReferencedNodeID() const
    {
        return ReferencedNodeID;
    }


  protected:

    /** read content item (value) from dataset
     ** @param  dataset    DICOM dataset from which the content item should be read
     *  @param  logStream  pointer to error/warning output stream (output disabled if NULL)
     ** @return status, EC_Normal if successful, an error code otherwise
     */
    virtual OFCondition readContentItem(DcmItem &dataset,
                                        OFConsole *logStream);

    /** write content item (value) to dataset
     ** @param  dataset    DICOM dataset to which the content item should be written
     *  @param  logStream  pointer to error/warning output stream (output disabled if NULL)
     ** @return status, EC_Normal if successful, an error code otherwise
     */
    virtual OFCondition writeContentItem(DcmItem &dataset,
                                         OFConsole *logStream) const;

    /** read content item specific XML data
     ** @param  doc     document containing the XML file content
     *  @param  cursor  cursor pointing to the starting node
     ** @return status, EC_Normal if successful, an error code otherwise
     */
    virtual OFCondition readXMLContentItem(const DSRXMLDocument &doc,
                                           DSRXMLCursor cursor);

    /** render content item (value) in HTML format
     ** @param  docStream     output stream to which the main HTML document is written
     *  @param  annexStream   output stream to which the HTML document annex is written
     *  @param  nestingLevel  current nesting level.  Used to render section headings.
     *  @param  annexNumber   reference to the variable where the current annex number is stored.
     *                        Value is increased automatically by 1 after a new entry has been added.
     *  @param  flags         flag used to customize the output (see DSRTypes::HF_xxx)
     *  @param  logStream     pointer to error/warning output stream (output disabled if NULL)
     ** @return status, EC_Normal if successful, an error code otherwise
     */
    virtual OFCondition renderHTMLContentItem(ostream &docStream,
                                              ostream &annexStream,
                                              const size_t nestingLevel,
                                              size_t &annexNumber,
                                              const size_t flags,
                                              OFConsole *logStream) const;


  private:

    /// flag indicating whether the reference is valid or not (i.e. checked)
    OFBool   ValidReference;
    /// position string of the referenced content item (target)
    OFString ReferencedContentItem;
    /// node ID of the referenced content item (target)
    size_t   ReferencedNodeID;


 // --- declaration of default/copy constructor and assignment operator

    DSRByReferenceTreeNode();
    DSRByReferenceTreeNode(const DSRByReferenceTreeNode &);
    DSRByReferenceTreeNode &operator=(const DSRByReferenceTreeNode &);
};


#endif


/*
 *  CVS/RCS Log:
 *  $Log: dsrreftn.h,v $
 *  Revision 1.10  2005/12/08 16:05:14  meichel
 *  Changed include path schema for all DCMTK header files
 *
 *  Revision 1.9  2003/10/30 17:53:02  joergr
 *  Added full support for the ContentTemplateSequence (read/write, get/set
 *  template identification). Template constraints are not checked yet.
 *
 *  Revision 1.8  2003/09/15 14:18:54  joergr
 *  Introduced new class to facilitate checking of SR IOD relationship content
 *  constraints. Replaced old implementation distributed over numerous classes.
 *
 *  Revision 1.7  2003/08/07 12:45:14  joergr
 *  Added readXML functionality.
 *
 *  Revision 1.6  2001/09/28 14:07:02  joergr
 *  Added term "class" to friend declaration to keep gcc 3.0 quiet.
 *
 *  Revision 1.5  2001/09/26 13:04:10  meichel
 *  Adapted dcmsr to class OFCondition
 *
 *  Revision 1.4  2001/06/01 15:51:03  meichel
 *  Updated copyright header
 *
 *  Revision 1.3  2000/11/07 18:14:30  joergr
 *  Enhanced support for by-reference relationships.
 *
 *  Revision 1.2  2000/11/01 16:23:23  joergr
 *  Added support for conversion to XML.
 *
 *  Revision 1.1  2000/10/26 14:22:42  joergr
 *  Added support for "Comprehensive SR".
 *
 *
 *
 */
