/*
 *
 *  Copyright (C) 2003-2005, OFFIS
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
 *    classes: DSRMammographyCadSRConstraintChecker
 *
 *  Last Update:      $Author: meichel $
 *  Update Date:      $Date: 2005/12/08 15:47:58 $
 *  CVS/RCS Revision: $Revision: 1.5 $
 *  Status:           $State: Exp $
 *
 *  CVS/RCS Log at end of file
 *
 */


#include "dcmtk/config/osconfig.h"    /* make sure OS specific configuration is included first */

#include "dcmtk/dcmsr/dsrmamcc.h"


DSRMammographyCadSRConstraintChecker::DSRMammographyCadSRConstraintChecker()
  : DSRIODConstraintChecker()
{
}


DSRMammographyCadSRConstraintChecker::~DSRMammographyCadSRConstraintChecker()
{
}


OFBool DSRMammographyCadSRConstraintChecker::isByReferenceAllowed() const
{
    return OFTrue;
}


OFBool DSRMammographyCadSRConstraintChecker::isTemplateSupportRequired() const
{
    return OFTrue;
}


const char *DSRMammographyCadSRConstraintChecker::getRootTemplateIdentifier() const
{
    return "4000";
}


DSRTypes::E_DocumentType DSRMammographyCadSRConstraintChecker::getDocumentType() const
{
    return DT_MammographyCadSR;
}


OFBool DSRMammographyCadSRConstraintChecker::checkContentRelationship(const E_ValueType sourceValueType,
                                                                      const E_RelationshipType relationshipType,
                                                                      const E_ValueType targetValueType,
                                                                      const OFBool byReference) const
{
    /* the following code implements the constraints of table A.35.5-2 in DICOM PS3.3 */
    OFBool result = OFFalse;
    /* row 1 of the table */
    if ((relationshipType == RT_contains) && !byReference && (sourceValueType == VT_Container))
    {
        result = (targetValueType == VT_Code)  || (targetValueType == VT_Num) || (targetValueType == VT_SCoord) ||
                 (targetValueType == VT_Image) || (targetValueType == VT_Container);
    }
    /* row 2 of the table */
    else if ((relationshipType == RT_hasObsContext) && !byReference && ((sourceValueType == VT_Container) ||
        (sourceValueType == VT_Text) || (sourceValueType == VT_Code) || (sourceValueType == VT_Num)))
    {
        result = (targetValueType == VT_Text) || (targetValueType == VT_Code) || (targetValueType == VT_Num)   ||
                 (targetValueType == VT_Date) || (targetValueType == VT_Time) || (targetValueType == VT_PName) ||
                 (targetValueType == VT_Composite);
    }
    /* row 3 of the table */
    else if ((relationshipType == RT_hasAcqContext) && !byReference && (sourceValueType == VT_Image))
    {
        result = (targetValueType == VT_Text) || (targetValueType == VT_Code) || (targetValueType == VT_Date) ||
                 (targetValueType == VT_Time) || (targetValueType == VT_Num /* see CP 545 */);
    }
    /* row 4 of the table */
    else if ((relationshipType == RT_hasConceptMod) && !byReference &&
        ((sourceValueType == VT_Container) || (sourceValueType == VT_Code)))
    {
        result = (targetValueType == VT_Text) || (targetValueType == VT_Code);
    }
    /* row 5 the table */
    else if ((relationshipType == RT_hasProperties) && !byReference &&
        ((sourceValueType == VT_Text) || (sourceValueType == VT_Code)))
    {
        result = (targetValueType == VT_Text) || (targetValueType == VT_Code)  || (targetValueType == VT_Num) ||
                 (targetValueType == VT_Date) || (targetValueType == VT_Image) || (targetValueType == VT_SCoord);
    }
    else if ((relationshipType == RT_hasProperties) && byReference && (sourceValueType == VT_Code))
    {
        /* WORKAROUND: allow by-reference relationship since it is needed for some Mammography CAD
         * SR templates (PS 3.16); however, this conflicts with the definitions in PS 3.3-2004.
         */
        result = (targetValueType == VT_Image);
    }
    /* row 6 of the table */
    else if ((relationshipType == RT_inferredFrom) && ((sourceValueType == VT_Code) || (sourceValueType == VT_Num)))
    {
        /* by-reference allowed */
        result = (targetValueType == VT_Code) || (targetValueType == VT_Num) || (targetValueType == VT_SCoord) ||
                 (targetValueType == VT_Container);
    }
    /* row 7 of the table */
    else if ((relationshipType == RT_selectedFrom) && (sourceValueType == VT_SCoord))
    {
        /* by-reference allowed */
        result = (targetValueType == VT_Image);
    }
    return result;
}


/*
 *  CVS/RCS Log:
 *  $Log: dsrmamcc.cc,v $
 *  Revision 1.5  2005/12/08 15:47:58  meichel
 *  Changed include path schema for all DCMTK header files
 *
 *  Revision 1.4  2005/07/27 16:55:04  joergr
 *  Added workaround to allow by-reference relationships as needed for some
 *  Mammography CAD SR templates.
 *
 *  Revision 1.3  2004/11/18 13:55:52  joergr
 *  Added workaround to fix issue with inconsistent definitions in DICOM part 3
 *  and part 16 regarding Mammography CAD SR.
 *
 *  Revision 1.2  2003/10/09 13:00:41  joergr
 *  Added check for root template identifier when reading an SR document.
 *
 *  Revision 1.1  2003/09/15 14:15:36  joergr
 *  Added content relationship constraint checking support for Mammography CAD
 *  SR and Chest CAD SR.
 *
 */
