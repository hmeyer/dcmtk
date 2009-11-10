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
 *    classes: DSRTypes
 *
 *  Last Update:      $Author: meichel $
 *  Update Date:      $Date: 2005/12/08 15:48:19 $
 *  CVS/RCS Revision: $Revision: 1.48 $
 *  Status:           $State: Exp $
 *
 *  CVS/RCS Log at end of file
 *
 */


#include "dcmtk/config/osconfig.h"    /* make sure OS specific configuration is included first */

#include "dcmtk/dcmsr/dsrtypes.h"
#include "dcmtk/dcmsr/dsrtextn.h"
#include "dcmtk/dcmsr/dsrcodtn.h"
#include "dcmtk/dcmsr/dsrnumtn.h"
#include "dcmtk/dcmsr/dsrdtitn.h"
#include "dcmtk/dcmsr/dsrdattn.h"
#include "dcmtk/dcmsr/dsrtimtn.h"
#include "dcmtk/dcmsr/dsruidtn.h"
#include "dcmtk/dcmsr/dsrpnmtn.h"
#include "dcmtk/dcmsr/dsrscotn.h"
#include "dcmtk/dcmsr/dsrtcotn.h"
#include "dcmtk/dcmsr/dsrcomtn.h"
#include "dcmtk/dcmsr/dsrimgtn.h"
#include "dcmtk/dcmsr/dsrwavtn.h"
#include "dcmtk/dcmsr/dsrcontn.h"
#include "dcmtk/dcmsr/dsrreftn.h"
#include "dcmtk/dcmsr/dsrbascc.h"
#include "dcmtk/dcmsr/dsrenhcc.h"
#include "dcmtk/dcmsr/dsrcomcc.h"
#include "dcmtk/dcmsr/dsrkeycc.h"
#include "dcmtk/dcmsr/dsrmamcc.h"
#include "dcmtk/dcmsr/dsrchecc.h"
#include "dcmtk/dcmsr/dsrprocc.h"
#include "dcmtk/dcmsr/dsrxrdcc.h"

#include "dcmtk/ofstd/ofstd.h"

#define INCLUDE_CSTDIO
#define INCLUDE_CCTYPE
#include "dcmtk/ofstd/ofstdinc.h"


/*---------------------------------*
 *  constant definitions (part 1)  *
 *---------------------------------*/

/* read flags */
const size_t DSRTypes::RF_readDigitalSignatures          = 1 <<  0;
const size_t DSRTypes::RF_ignoreRelationshipConstraints  = 1 <<  1;
const size_t DSRTypes::RF_ignoreContentItemErrors        = 1 <<  2;
const size_t DSRTypes::RF_skipInvalidContentItems        = 1 <<  3;
const size_t DSRTypes::RF_verboseDebugMode               = 1 <<  4;
const size_t DSRTypes::RF_showCurrentlyProcessedItem     = 1 <<  5;

/* renderHTML flags */
const size_t DSRTypes::HF_neverExpandChildrenInline      = 1 <<  0;
const size_t DSRTypes::HF_renderInlineCodes              = 1 <<  1;
const size_t DSRTypes::HF_renderConceptNameCodes         = 1 <<  2;
const size_t DSRTypes::HF_renderNumericUnitCodes         = 1 <<  3;
const size_t DSRTypes::HF_useCodeMeaningAsUnit           = 1 <<  4;
const size_t DSRTypes::HF_renderPatientTitle             = 1 <<  5;
const size_t DSRTypes::HF_renderNoDocumentHeader         = 1 <<  6;
const size_t DSRTypes::HF_renderDcmtkFootnote            = 1 <<  7;
const size_t DSRTypes::HF_renderFullData                 = 1 <<  8;
const size_t DSRTypes::HF_copyStyleSheetContent          = 1 <<  9;
const size_t DSRTypes::HF_version32Compatibility         = 1 << 10;
const size_t DSRTypes::HF_addDocumentTypeReference       = 1 << 11;
/* internal */
const size_t DSRTypes::HF_renderItemsSeparately          = 1 << 12;
const size_t DSRTypes::HF_renderItemInline               = 1 << 13;
const size_t DSRTypes::HF_currentlyInsideAnnex           = 1 << 14;
const size_t DSRTypes::HF_createFootnoteReferences       = 1 << 15;
const size_t DSRTypes::HF_convertNonASCIICharacters      = 1 << 16;
/* shortcuts */
const size_t DSRTypes::HF_renderAllCodes                 = DSRTypes::HF_renderInlineCodes |
                                                           DSRTypes::HF_renderConceptNameCodes |
                                                           DSRTypes::HF_renderNumericUnitCodes;
const size_t DSRTypes::HF_internalUseOnly                = DSRTypes::HF_renderItemsSeparately |
                                                           DSRTypes::HF_renderItemInline |
                                                           DSRTypes::HF_currentlyInsideAnnex |
                                                           DSRTypes::HF_createFootnoteReferences |
                                                           DSRTypes::HF_convertNonASCIICharacters;

/* read/writeXML flags */
const size_t DSRTypes::XF_writeEmptyTags                 = 1 << 0;
const size_t DSRTypes::XF_writeTemplateIdentification    = 1 << 1;
const size_t DSRTypes::XF_alwaysWriteItemIdentifier      = 1 << 2;
const size_t DSRTypes::XF_codeComponentsAsAttribute      = 1 << 3;
const size_t DSRTypes::XF_relationshipTypeAsAttribute    = 1 << 4;
const size_t DSRTypes::XF_valueTypeAsAttribute           = 1 << 5;
const size_t DSRTypes::XF_templateIdentifierAsAttribute  = 1 << 6;
const size_t DSRTypes::XF_useDcmsrNamespace              = 1 << 7;
const size_t DSRTypes::XF_addSchemaReference             = 1 << 8;
const size_t DSRTypes::XF_validateSchema                 = 1 << 9;
const size_t DSRTypes::XF_enableLibxmlErrorOutput        = 1 << 10;
const size_t DSRTypes::XF_templateElementEnclosesItems   = 1 << 11;
/* shortcuts */
const size_t DSRTypes::XF_encodeEverythingAsAttribute    = DSRTypes::XF_codeComponentsAsAttribute |
                                                           DSRTypes::XF_relationshipTypeAsAttribute |
                                                           DSRTypes::XF_valueTypeAsAttribute |
                                                           DSRTypes::XF_templateIdentifierAsAttribute;

/* print flags */
const size_t DSRTypes::PF_printItemPosition              = 1 << 0;
const size_t DSRTypes::PF_shortenLongItemValues          = 1 << 1;
const size_t DSRTypes::PF_printSOPInstanceUID            = 1 << 2;
const size_t DSRTypes::PF_printConceptNameCodes          = 1 << 3;
const size_t DSRTypes::PF_printNoDocumentHeader          = 1 << 4;
const size_t DSRTypes::PF_printTemplateIdentification    = 1 << 5;
const size_t DSRTypes::PF_printAllCodes                  = DSRTypes::PF_printConceptNameCodes;


/*---------------------*
 *  type declarations  *
 *---------------------*/

struct S_DocumentTypeNameMap
{
    DSRTypes::E_DocumentType Type;
    const char *SOPClassUID;
    const char *Modality;
    const char *ReadableName;
};


struct S_RelationshipTypeNameMap
{
    DSRTypes::E_RelationshipType Type;
    const char *DefinedTerm;
    const char *ReadableName;
};


struct S_ValueTypeNameMap
{
    DSRTypes::E_ValueType Type;
    const char *DefinedTerm;
    const char *XMLTagName;
    const char *ReadableName;
};


struct S_GraphicTypeNameMap
{
    DSRTypes::E_GraphicType Type;
    const char *EnumeratedValue;
    const char *ReadableName;
};


struct S_TemporalRangeTypeNameMap
{
    DSRTypes::E_TemporalRangeType Type;
    const char *EnumeratedValue;
    const char *ReadableName;
};


struct S_ContinuityOfContentNameMap
{
    DSRTypes::E_ContinuityOfContent Type;
    const char *EnumeratedValue;
};


struct S_CompletionFlagNameMap
{
    DSRTypes::E_CompletionFlag Type;
    const char *EnumeratedValue;
};


struct S_VerificationFlagNameMap
{
    DSRTypes::E_VerificationFlag Type;
    const char *EnumeratedValue;
};


struct S_CharacterSetNameMap
{
    DSRTypes::E_CharacterSet Type;
    const char *DefinedTerm;
    const char *HTMLName;
    const char *XMLName;
};


/*---------------------------------*
 *  constant definitions (part 2)  *
 *---------------------------------*/

// conditions
const OFConditionConst ECC_UnknownDocumentType              (OFM_dcmsr,  1, OF_error, "Unknown Document Type");
const OFConditionConst ECC_InvalidDocument                  (OFM_dcmsr,  2, OF_error, "Invalid Document");
const OFConditionConst ECC_InvalidDocumentTree              (OFM_dcmsr,  3, OF_error, "Invalid Document Tree");
const OFConditionConst ECC_MandatoryAttributeMissing        (OFM_dcmsr,  4, OF_error, "Mandatory Attribute missing");
const OFConditionConst ECC_InvalidValue                     (OFM_dcmsr,  5, OF_error, "Invalid Value");
const OFConditionConst ECC_UnsupportedValue                 (OFM_dcmsr,  6, OF_error, "Unsupported Value");
const OFConditionConst ECC_UnknownValueType                 (OFM_dcmsr,  7, OF_error, "Unknown Value Type");
const OFConditionConst ECC_UnknownRelationshipType          (OFM_dcmsr,  8, OF_error, "Unknown Relationship Type");
const OFConditionConst ECC_InvalidByValueRelationship       (OFM_dcmsr,  9, OF_error, "Invalid by-value Relationship");
const OFConditionConst ECC_InvalidByReferenceRelationship   (OFM_dcmsr, 10, OF_error, "Invalid by-reference Relationship");
const OFConditionConst ECC_SOPInstanceNotFound              (OFM_dcmsr, 11, OF_error, "SOP Instance not found");
const OFConditionConst ECC_DifferentSOPClassesForAnInstance (OFM_dcmsr, 12, OF_error, "Different SOP Classes for an Instance");
const OFConditionConst ECC_CodingSchemeNotFound             (OFM_dcmsr, 13, OF_error, "Coding Scheme Designator not found");
const OFConditionConst ECC_CorruptedXMLStructure            (OFM_dcmsr, 14, OF_error, "Corrupted XML structure");

const OFCondition SR_EC_UnknownDocumentType                 (ECC_UnknownDocumentType);
const OFCondition SR_EC_InvalidDocument                     (ECC_InvalidDocument);
const OFCondition SR_EC_InvalidDocumentTree                 (ECC_InvalidDocumentTree);
const OFCondition SR_EC_MandatoryAttributeMissing           (ECC_MandatoryAttributeMissing);
const OFCondition SR_EC_InvalidValue                        (ECC_InvalidValue);
const OFCondition SR_EC_UnsupportedValue                    (ECC_UnsupportedValue);
const OFCondition SR_EC_UnknownValueType                    (ECC_UnknownValueType);
const OFCondition SR_EC_UnknownRelationshipType             (ECC_UnknownRelationshipType);
const OFCondition SR_EC_InvalidByValueRelationship          (ECC_InvalidByValueRelationship);
const OFCondition SR_EC_InvalidByReferenceRelationship      (ECC_InvalidByReferenceRelationship);
const OFCondition SR_EC_SOPInstanceNotFound                 (ECC_SOPInstanceNotFound);
const OFCondition SR_EC_DifferentSOPClassesForAnInstance    (ECC_DifferentSOPClassesForAnInstance);
const OFCondition SR_EC_CodingSchemeNotFound                (ECC_CodingSchemeNotFound);
const OFCondition SR_EC_CorruptedXMLStructure               (ECC_CorruptedXMLStructure);


static const S_DocumentTypeNameMap DocumentTypeNameMap[] =
{
    {DSRTypes::DT_invalid,             "",                             "",   "invalid document type"},
    {DSRTypes::DT_BasicTextSR,         UID_BasicTextSR,                "SR", "Basic Text SR"},
    {DSRTypes::DT_EnhancedSR,          UID_EnhancedSR,                 "SR", "Enhanced SR"},
    {DSRTypes::DT_ComprehensiveSR,     UID_ComprehensiveSR,            "SR", "Comprehensive SR"},
    {DSRTypes::DT_KeyObjectDoc,        UID_KeyObjectSelectionDocument, "KO", "Key Object Selection Document"},
    {DSRTypes::DT_MammographyCadSR,    UID_MammographyCADSR,           "SR", "Mammography CAD SR"},
    {DSRTypes::DT_ChestCadSR,          UID_ChestCADSR,                 "SR", "Chest CAD SR"},
    {DSRTypes::DT_ProcedureLog,        UID_ProcedureLogStorage,        "SR", "Procedure Log"},
    {DSRTypes::DT_XRayRadiationDoseSR, UID_XRayRadiationDoseSR,        "SR", "X-Ray Radiation Dose SR"}
};


static const S_RelationshipTypeNameMap RelationshipTypeNameMap[] =
{
    {DSRTypes::RT_invalid,       "",                "invalid/unknown relationship type"},
    {DSRTypes::RT_isRoot,        "",                ""},
    {DSRTypes::RT_contains,      "CONTAINS",        "contains"},
    {DSRTypes::RT_hasObsContext, "HAS OBS CONTEXT", "has obs context"},
    {DSRTypes::RT_hasAcqContext, "HAS ACQ CONTEXT", "has acq context"},
    {DSRTypes::RT_hasConceptMod, "HAS CONCEPT MOD", "has concept mod"},
    {DSRTypes::RT_hasProperties, "HAS PROPERTIES",  "has properties"},
    {DSRTypes::RT_inferredFrom,  "INFERRED FROM",   "inferred from"},
    {DSRTypes::RT_selectedFrom,  "SELECTED FROM",   "selected from"}
};


static const S_ValueTypeNameMap ValueTypeNameMap[] =
{
    {DSRTypes::VT_invalid,     "",               "item",      "invalid/unknown value type"},
    {DSRTypes::VT_Text,        "TEXT",           "text",      "Text"},
    {DSRTypes::VT_Code,        "CODE",           "code",      "Code"},
    {DSRTypes::VT_Num,         "NUM",            "num",       "Number"},
    {DSRTypes::VT_DateTime,    "DATETIME",       "datetime",  "Date/Time"},
    {DSRTypes::VT_Date,        "DATE",           "date",      "Date"},
    {DSRTypes::VT_Time,        "TIME",           "time",      "Time"},
    {DSRTypes::VT_UIDRef,      "UIDREF",         "uidref",    "UID Reference"},
    {DSRTypes::VT_PName,       "PNAME",          "pname",     "Person Name"},
    {DSRTypes::VT_SCoord,      "SCOORD",         "scoord",    "Spatial Coordinates"},
    {DSRTypes::VT_TCoord,      "TCOORD",         "tcoord",    "Temporal Coordinates"},
    {DSRTypes::VT_Composite,   "COMPOSITE",      "composite", "Composite Object"},
    {DSRTypes::VT_Image,       "IMAGE",          "image",     "Image"},
    {DSRTypes::VT_Waveform,    "WAVEFORM",       "waveform",  "Waveform"},
    {DSRTypes::VT_Container,   "CONTAINER",      "container", "Container"},
    {DSRTypes::VT_byReference, "(by-reference)", "reference", "(by-reference)"}
};


static const S_GraphicTypeNameMap GraphicTypeNameMap[] =
{
    {DSRTypes::GT_invalid,    "",           "invalid/unknown graphic type"},
    {DSRTypes::GT_Point,      "POINT",      "Point"},
    {DSRTypes::GT_Multipoint, "MULTIPOINT", "Multiple Points"},
    {DSRTypes::GT_Polyline,   "POLYLINE",   "Polyline"},
    {DSRTypes::GT_Circle,     "CIRCLE",     "Circle"},
    {DSRTypes::GT_Ellipse,    "ELLIPSE",    "Ellipse"}
};


static const S_TemporalRangeTypeNameMap TemporalRangeTypeNameMap[] =
{
    {DSRTypes::TRT_invalid,      "",             ""},
    {DSRTypes::TRT_Point,        "POINT",        "Point"},
    {DSRTypes::TRT_Multipoint,   "MULTIPOINT",   "Multiple Points"},
    {DSRTypes::TRT_Segment,      "SEGMENT",      "Segment"},
    {DSRTypes::TRT_Multisegment, "MULTISEGMENT", "Multiple Segments"},
    {DSRTypes::TRT_Begin,        "BEGIN",        "Begin"},
    {DSRTypes::TRT_End,          "END",          "End"}
};


static const S_ContinuityOfContentNameMap ContinuityOfContentNameMap[] =
{
    {DSRTypes::COC_invalid,    ""},
    {DSRTypes::COC_Separate,   "SEPARATE"},
    {DSRTypes::COC_Continuous, "CONTINUOUS"}
};


static const S_CompletionFlagNameMap CompletionFlagNameMap[] =
{
    {DSRTypes::CF_invalid,  ""},
    {DSRTypes::CF_Partial,  "PARTIAL"},
    {DSRTypes::CF_Complete, "COMPLETE"}
};


static const S_VerificationFlagNameMap VerificationFlagNameMap[] =
{
    {DSRTypes::VF_invalid,    ""},
    {DSRTypes::VF_Unverified, "UNVERIFIED"},
    {DSRTypes::VF_Verified,   "VERIFIED"}
};


static const S_CharacterSetNameMap CharacterSetNameMap[] =
{
    // columns: enum, DICOM, HTML, XML (if "?" a warning is reported)
    {DSRTypes::CS_invalid,  "",           "",           ""},
    {DSRTypes::CS_ASCII,    "ISO_IR 6",   "",           "UTF-8"},
    {DSRTypes::CS_Latin1,   "ISO_IR 100", "ISO-8859-1", "ISO-8859-1"},
    {DSRTypes::CS_Latin2,   "ISO_IR 101", "ISO-8859-2", "ISO-8859-2"},
    {DSRTypes::CS_Latin3,   "ISO_IR 109", "ISO-8859-3", "ISO-8859-3"},
    {DSRTypes::CS_Latin4,   "ISO_IR 110", "ISO-8859-4", "ISO-8859-4"},
    {DSRTypes::CS_Cyrillic, "ISO_IR 144", "ISO-8859-5", "ISO-8859-5"},
    {DSRTypes::CS_Arabic,   "ISO_IR 127", "ISO-8859-6", "ISO-8859-6"},
    {DSRTypes::CS_Greek,    "ISO_IR 126", "ISO-8859-7", "ISO-8859-7"},
    {DSRTypes::CS_Hebrew,   "ISO_IR 138", "ISO-8859-8", "ISO-8859-8"},
    {DSRTypes::CS_Latin5,   "ISO_IR 148", "ISO-8859-9", "ISO-8859-9"},
    {DSRTypes::CS_Japanese, "ISO_IR 13",  "?",          "?"},  /* JIS_X0201 ? */
    {DSRTypes::CS_Thai,     "ISO_IR 166", "?",          "?"},  /* TIS-620 ? */
    {DSRTypes::CS_UTF8,     "ISO_IR 192", "UTF-8",      "UTF-8"}
};


/*------------------*
 *  implementation  *
 *------------------*/

const char *DSRTypes::documentTypeToSOPClassUID(const E_DocumentType documentType)
{
    const S_DocumentTypeNameMap *iterator = DocumentTypeNameMap;
    while ((iterator->Type != DT_last) && (iterator->Type != documentType))
        iterator++;
    return iterator->SOPClassUID;
}


const char *DSRTypes::documentTypeToModality(const E_DocumentType documentType)
{
    const S_DocumentTypeNameMap *iterator = DocumentTypeNameMap;
    while ((iterator->Type != DT_last) && (iterator->Type != documentType))
        iterator++;
    return iterator->Modality;
}


const char *DSRTypes::documentTypeToReadableName(const E_DocumentType documentType)
{
    const S_DocumentTypeNameMap *iterator = DocumentTypeNameMap;
    while ((iterator->Type != DT_last) && (iterator->Type != documentType))
        iterator++;
    return iterator->ReadableName;
}


const char *DSRTypes::documentTypeToDocumentTitle(const E_DocumentType documentType,
                                                  OFString &documentTitle)
{
    documentTitle = documentTypeToReadableName(documentType);
    if (!documentTitle.empty() && (documentType != DT_KeyObjectDoc))  // avoid doubling of term "Document"
        documentTitle += " Document";
    return documentTitle.c_str();
}


const char *DSRTypes::relationshipTypeToDefinedTerm(const E_RelationshipType relationshipType)
{
    const S_RelationshipTypeNameMap *iterator = RelationshipTypeNameMap;
    while ((iterator->Type != RT_last) && (iterator->Type != relationshipType))
        iterator++;
    return iterator->DefinedTerm;
}


const char *DSRTypes::relationshipTypeToReadableName(const E_RelationshipType relationshipType)
{
    const S_RelationshipTypeNameMap *iterator = RelationshipTypeNameMap;
    while ((iterator->Type != RT_last) && (iterator->Type != relationshipType))
        iterator++;
    return iterator->ReadableName;
}


const char *DSRTypes::valueTypeToDefinedTerm(const E_ValueType valueType)
{
    const S_ValueTypeNameMap *iterator = ValueTypeNameMap;
    while ((iterator->Type != VT_last) && (iterator->Type != valueType))
        iterator++;
    return iterator->DefinedTerm;
}


const char *DSRTypes::valueTypeToXMLTagName(const E_ValueType valueType)
{
    const S_ValueTypeNameMap *iterator = ValueTypeNameMap;
    while ((iterator->Type != VT_last) && (iterator->Type != valueType))
        iterator++;
    return iterator->XMLTagName;
}


const char *DSRTypes::valueTypeToReadableName(const E_ValueType valueType)
{
    const S_ValueTypeNameMap *iterator = ValueTypeNameMap;
    while ((iterator->Type != VT_last) && (iterator->Type != valueType))
        iterator++;
    return iterator->ReadableName;
}


const char *DSRTypes::graphicTypeToEnumeratedValue(const E_GraphicType graphicType)
{
    const S_GraphicTypeNameMap *iterator = GraphicTypeNameMap;
    while ((iterator->Type != GT_last) && (iterator->Type != graphicType))
        iterator++;
    return iterator->EnumeratedValue;
}


const char *DSRTypes::graphicTypeToReadableName(const E_GraphicType graphicType)
{
    const S_GraphicTypeNameMap *iterator = GraphicTypeNameMap;
    while ((iterator->Type != GT_last) && (iterator->Type != graphicType))
        iterator++;
    return iterator->ReadableName;
}


const char *DSRTypes::temporalRangeTypeToEnumeratedValue(const E_TemporalRangeType temporalRangeType)
{
    const S_TemporalRangeTypeNameMap *iterator = TemporalRangeTypeNameMap;
    while ((iterator->Type != TRT_last) && (iterator->Type != temporalRangeType))
        iterator++;
    return iterator->EnumeratedValue;
}


const char *DSRTypes::temporalRangeTypeToReadableName(const E_TemporalRangeType temporalRangeType)
{
    const S_TemporalRangeTypeNameMap *iterator = TemporalRangeTypeNameMap;
    while ((iterator->Type != TRT_last) && (iterator->Type != temporalRangeType))
        iterator++;
    return iterator->ReadableName;
}


const char *DSRTypes::continuityOfContentToEnumeratedValue(const E_ContinuityOfContent continuityOfContent)
{
    const S_ContinuityOfContentNameMap *iterator = ContinuityOfContentNameMap;
    while ((iterator->Type != COC_last) && (iterator->Type != continuityOfContent))
        iterator++;
    return iterator->EnumeratedValue;
}


const char *DSRTypes::completionFlagToEnumeratedValue(const E_CompletionFlag completionFlag)
{
    const S_CompletionFlagNameMap *iterator = CompletionFlagNameMap;
    while ((iterator->Type != CF_last) && (iterator->Type != completionFlag))
        iterator++;
    return iterator->EnumeratedValue;
}


const char *DSRTypes::verificationFlagToEnumeratedValue(const E_VerificationFlag verificationFlag)
{
    const S_VerificationFlagNameMap *iterator = VerificationFlagNameMap;
    while ((iterator->Type != VF_last) && (iterator->Type != verificationFlag))
        iterator++;
    return iterator->EnumeratedValue;
}


const char *DSRTypes::characterSetToDefinedTerm(const E_CharacterSet characterSet)
{
    const S_CharacterSetNameMap *iterator = CharacterSetNameMap;
    while ((iterator->Type != CS_last) && (iterator->Type != characterSet))
        iterator++;
    return iterator->DefinedTerm;
}


const char *DSRTypes::characterSetToHTMLName(const E_CharacterSet characterSet)
{
    const S_CharacterSetNameMap *iterator = CharacterSetNameMap;
    while ((iterator->Type != CS_last) && (iterator->Type != characterSet))
        iterator++;
    return iterator->HTMLName;
}


const char *DSRTypes::characterSetToXMLName(const E_CharacterSet characterSet)
{
    const S_CharacterSetNameMap *iterator = CharacterSetNameMap;
    while ((iterator->Type != CS_last) && (iterator->Type != characterSet))
        iterator++;
    return iterator->XMLName;
}


DSRTypes::E_DocumentType DSRTypes::sopClassUIDToDocumentType(const OFString &sopClassUID)
{
    E_DocumentType type = DT_invalid;
    const S_DocumentTypeNameMap *iterator = DocumentTypeNameMap;
    while ((iterator->Type != DT_last) && (sopClassUID != iterator->SOPClassUID))
        iterator++;
    if (sopClassUID == iterator->SOPClassUID)
        type = iterator->Type;
    return type;
}


DSRTypes::E_RelationshipType DSRTypes::definedTermToRelationshipType(const OFString &definedTerm)
{
    E_RelationshipType type = RT_invalid;
    const S_RelationshipTypeNameMap *iterator = RelationshipTypeNameMap;
    while ((iterator->Type != RT_last) && (definedTerm != iterator->DefinedTerm))
        iterator++;
    if (definedTerm == iterator->DefinedTerm)
        type = iterator->Type;
    return type;
}


DSRTypes::E_ValueType DSRTypes::definedTermToValueType(const OFString &definedTerm)
{
    E_ValueType type = VT_invalid;
    const S_ValueTypeNameMap *iterator = ValueTypeNameMap;
    while ((iterator->Type != VT_last) && (definedTerm != iterator->DefinedTerm))
        iterator++;
    if (definedTerm == iterator->DefinedTerm)
        type = iterator->Type;
    return type;
}


DSRTypes::E_ValueType DSRTypes::xmlTagNameToValueType(const OFString &xmlTagName)
{
    E_ValueType type = VT_invalid;
    const S_ValueTypeNameMap *iterator = ValueTypeNameMap;
    while ((iterator->Type != VT_last) && (xmlTagName != iterator->XMLTagName))
        iterator++;
    if (xmlTagName == iterator->XMLTagName)
        type = iterator->Type;
    return type;
}


DSRTypes::E_GraphicType DSRTypes::enumeratedValueToGraphicType(const OFString &enumeratedValue)
{
    E_GraphicType type = GT_invalid;
    const S_GraphicTypeNameMap *iterator = GraphicTypeNameMap;
    while ((iterator->Type != GT_last) && (enumeratedValue != iterator->EnumeratedValue))
        iterator++;
    if (enumeratedValue == iterator->EnumeratedValue)
        type = iterator->Type;
    return type;
}


DSRTypes::E_TemporalRangeType DSRTypes::enumeratedValueToTemporalRangeType(const OFString &enumeratedValue)
{
    E_TemporalRangeType type = TRT_invalid;
    const S_TemporalRangeTypeNameMap *iterator = TemporalRangeTypeNameMap;
    while ((iterator->Type != TRT_last) && (enumeratedValue != iterator->EnumeratedValue))
        iterator++;
    if (enumeratedValue == iterator->EnumeratedValue)
        type = iterator->Type;
    return type;
}


DSRTypes::E_ContinuityOfContent DSRTypes::enumeratedValueToContinuityOfContent(const OFString &enumeratedValue)
{
    E_ContinuityOfContent type = COC_invalid;
    const S_ContinuityOfContentNameMap *iterator = ContinuityOfContentNameMap;
    while ((iterator->Type != COC_last) && (enumeratedValue != iterator->EnumeratedValue))
        iterator++;
    if (enumeratedValue == iterator->EnumeratedValue)
        type = iterator->Type;
    return type;
}


DSRTypes::E_CompletionFlag DSRTypes::enumeratedValueToCompletionFlag(const OFString &enumeratedValue)
{
    E_CompletionFlag type = CF_invalid;
    const S_CompletionFlagNameMap *iterator = CompletionFlagNameMap;
    while ((iterator->Type != CF_last) && (enumeratedValue != iterator->EnumeratedValue))
        iterator++;
    if (enumeratedValue == iterator->EnumeratedValue)
        type = iterator->Type;
    return type;
}


DSRTypes::E_VerificationFlag DSRTypes::enumeratedValueToVerificationFlag(const OFString &enumeratedValue)
{
    E_VerificationFlag type = VF_invalid;
    const S_VerificationFlagNameMap *iterator = VerificationFlagNameMap;
    while ((iterator->Type != VF_last) && (enumeratedValue != iterator->EnumeratedValue))
        iterator++;
    if (enumeratedValue == iterator->EnumeratedValue)
        type = iterator->Type;
    return type;
}


DSRTypes::E_CharacterSet DSRTypes::definedTermToCharacterSet(const OFString &definedTerm)
{
    E_CharacterSet type = CS_invalid;
    const S_CharacterSetNameMap *iterator = CharacterSetNameMap;
    while ((iterator->Type != CS_last) && (definedTerm != iterator->DefinedTerm))
        iterator++;
    if (definedTerm == iterator->DefinedTerm)
        type = iterator->Type;
    return type;
}


OFBool DSRTypes::isDocumentTypeSupported(const E_DocumentType documentType)
{
    return (documentType != DT_invalid);
}


OFCondition DSRTypes::addElementToDataset(OFCondition &result,
                                          DcmItem &dataset,
                                          DcmElement *delem)
{
    if (result.good())
    {
        if (delem != NULL)
            result = dataset.insert(delem, OFTrue /*replaceOld*/);
        else
            result = EC_MemoryExhausted;
    }
    return result;
}


void DSRTypes::removeAttributeFromSequence(DcmSequenceOfItems &sequence,
                                           const DcmTagKey &tagKey)
{
    DcmStack stack;
    DcmItem *item = NULL;
    const size_t count = OFstatic_cast(size_t, sequence.card());
    for (size_t i = 0; i < count; i++)
    {
        /* not very efficient, should be replaced by nextObject() */
        item = sequence.getItem(i);
        if (item != NULL)
        {
            /* should not be necessary, but is more secure */
            stack.clear();
            if (item->search(tagKey, stack, ESM_fromHere, OFTrue /*searchIntoSub*/).good())
            {
                while (!stack.empty())
                    delete item->remove(stack.pop());
            }
        }
    }
}


OFCondition DSRTypes::getElementFromDataset(DcmItem &dataset,
                                            DcmElement &delem)
{
    DcmStack stack;
    OFCondition result = dataset.search(delem.getTag(), stack, ESM_fromHere, OFFalse /*searchIntoSub*/);
    if (result.good())
        delem = *OFstatic_cast(DcmElement *, stack.top());
    return result;
}


OFCondition DSRTypes::getSequenceFromDataset(DcmItem &dataset,
                                             DcmSequenceOfItems &dseq)
{
    DcmStack stack;
    OFCondition result = dataset.search(dseq.getTag(), stack, ESM_fromHere, OFFalse /*searchIntoSub*/);
    if (result.good())
        dseq = *OFstatic_cast(DcmSequenceOfItems *, stack.top());
    return result;
}


const char *DSRTypes::getStringValueFromElement(const DcmElement &delem)
{
    char *stringValue = NULL;
    if (OFconst_cast(DcmElement &, delem).getString(stringValue).bad())
        stringValue = NULL;
    return stringValue;
}


const OFString &DSRTypes::getStringValueFromElement(const DcmElement &delem,
                                                    OFString &stringValue)
{
    if (OFconst_cast(DcmElement &, delem).getOFString(stringValue, 0).bad())
        stringValue.clear();
    return stringValue;
}


const OFString &DSRTypes::getPrintStringFromElement(const DcmElement &delem,
                                                    OFString &stringValue)
{
    OFString tempString;
    return convertToPrintString(getStringValueFromElement(delem, tempString), stringValue);
}


const OFString &DSRTypes::getMarkupStringFromElement(const DcmElement &delem,
                                                     OFString &stringValue,
                                                     const OFBool convertNonASCII)
{
    OFString tempString;
    return convertToMarkupString(getStringValueFromElement(delem, tempString), stringValue, convertNonASCII);
}


OFCondition DSRTypes::getStringValueFromDataset(DcmItem &dataset,
                                                const DcmTagKey &tagKey,
                                                OFString &stringValue)
{
    return dataset.findAndGetOFString(tagKey, stringValue, 0, OFFalse /*searchIntoSub*/);
}


OFCondition DSRTypes::putStringValueToDataset(DcmItem &dataset,
                                              const DcmTag &tag,
                                              const OFString &stringValue,
                                              const OFBool allowEmpty)
{
    OFCondition result = EC_Normal;
    if (allowEmpty || !stringValue.empty())
        result = dataset.putAndInsertString(tag, stringValue.c_str(), OFTrue /*replaceOld*/);
    return result;
}


OFBool DSRTypes::checkElementValue(DcmElement &delem,
                                   const OFString &vm,
                                   const OFString &type,
                                   OFConsole *stream,
                                   const OFCondition &searchCond,
                                   const char *moduleName)
{
    OFBool result = OFTrue;
    OFBool print = OFTrue;
    DcmTag tag = delem.getTag();
    OFString message = tag.getTagName();
    OFString module = (moduleName == NULL) ? "SR document" : moduleName;
    Uint32 lenNum;
    unsigned long vmNum;
    OFString vmText;
    /* special case: sequence of items */
    if (delem.getVR() == EVR_SQ)
    {
        lenNum = vmNum = OFstatic_cast(DcmSequenceOfItems &, delem).card();
        vmText = " #items";
    } else {
        lenNum = delem.getLength();
        vmNum = delem.getVM();
        vmText = " VM";
    }
    /* NB: type 1C and 2C cannot be checked, assuming to be optional = type 3 */
    if (((type == "1") || (type == "2")) && (searchCond.bad()))
    {
        message += " absent in ";
        message += module;
        message += " (type ";
        message += type;
        message += ")";
        result = OFFalse;
    }
    else if ((type == "1") && (lenNum == 0))
    {
        message += " empty in ";
        message += module;
        message += " (type 1)";
        result = OFFalse;
    }
    else if ((vm == "1") && (vmNum > 1))
    {
        message += vmText;
        message += " != 1 in ";
        message += module;
        result = OFFalse;
    }
    else if ((type == "1") && (vm == "1-n") && (vmNum < 1))
    {
        message += vmText;
        message += " != 1-n in ";
        message += module;
        result = OFFalse;
    }
    else if ((vm == "2") && (vmNum != 2))
    {
        message += vmText;
        message += " != 2 in ";
        message += module;
        result = OFFalse;
    }
    else if ((vm == "2-2n") && ((vmNum % 2) != 0))
    {
        message += vmText;
        message += " != 2-2n in ";
        message += module;
        result = (vmNum >= 2);
    } else
        print = OFFalse;
    if (print && (stream != NULL) && !message.empty())
        printWarningMessage(stream, message.c_str());
    return result;
}


OFCondition DSRTypes::getAndCheckElementFromDataset(DcmItem &dataset,
                                                    DcmElement &delem,
                                                    const OFString &vm,
                                                    const OFString &type,
                                                    OFConsole *stream,
                                                    const char *moduleName)
{
    OFCondition result = getElementFromDataset(dataset, delem);
    if (!checkElementValue(delem, vm, type, stream, result, moduleName))
        result = SR_EC_InvalidValue;
    return result;
}


OFCondition DSRTypes::getAndCheckStringValueFromDataset(DcmItem &dataset,
                                                        const DcmTagKey &tagKey,
                                                        OFString &stringValue,
                                                        const OFString &vm,
                                                        const OFString &type,
                                                        OFConsole *stream,
                                                        const char *moduleName)
{
    DcmStack stack;
    OFCondition result = dataset.search(tagKey, stack, ESM_fromHere, OFFalse /*searchIntoSub*/);
    if (result.good())
    {
        DcmElement *delem = OFstatic_cast(DcmElement *, stack.top());
        if (delem != NULL)
        {
            if (checkElementValue(*delem, vm, type, stream, result, moduleName))
                result = delem->getOFString(stringValue, 0);
            else
                result = SR_EC_InvalidValue;
        } else
            result = EC_CorruptedData;
    } else {
        if ((stream != NULL) && ((type == "1") || (type == "2")))
        {
            OFString message = DcmTag(tagKey).getTagName();
            message += " absent in ";
            message += (moduleName == NULL) ? "SR document" : moduleName;
            message += " (type ";
            message += type;
            message += ")";
            printWarningMessage(stream, message.c_str());
        }
    }
    if (result.bad())
        stringValue.clear();
    return result;
}


// --- misc helper functions ---

const OFString &DSRTypes::currentDate(OFString &dateString)
{
    DcmDate::getCurrentDate(dateString);
    return dateString;
}


const OFString &DSRTypes::currentTime(OFString &timeString)
{
    DcmTime::getCurrentTime(timeString, OFTrue /*seconds*/, OFFalse /*fraction*/);
    return timeString;
}


const OFString &DSRTypes::currentDateTime(OFString &dateTimeString)
{
    DcmDateTime::getCurrentDateTime(dateTimeString, OFTrue /*seconds*/, OFFalse /*fraction*/, OFFalse /*timeZone*/);
    return dateTimeString;
}


const OFString &DSRTypes::dicomToReadableDate(const OFString &dicomDate,
                                              OFString &readableDate)
{
    DcmDate::getISOFormattedDateFromString(dicomDate, readableDate);
    return readableDate;
}


const OFString &DSRTypes::dicomToReadableTime(const OFString &dicomTime,
                                              OFString &readableTime)
{
    DcmTime::getISOFormattedTimeFromString(dicomTime, readableTime, OFTrue /*seconds*/, OFFalse /*fraction*/, OFFalse /*createMissingPart*/);
    return readableTime;
}


const OFString &DSRTypes::dicomToReadableDateTime(const OFString &dicomDateTime,
                                                  OFString &readableDateTime)
{
    DcmDateTime::getISOFormattedDateTimeFromString(dicomDateTime, readableDateTime, OFTrue /*seconds*/, OFFalse /*fraction*/, OFTrue /*timeZone*/, OFFalse /*createMissingPart*/);
    return readableDateTime;
}


const OFString &DSRTypes::dicomToReadablePersonName(const OFString &dicomPersonName,
                                                    OFString &readablePersonName)
{
    if (DcmPersonName::getFormattedNameFromString(dicomPersonName, readablePersonName, 0 /*componentGroup*/).bad())
        readablePersonName = dicomPersonName;
    return readablePersonName;
}


const OFString &DSRTypes::dicomToXMLPersonName(const OFString &dicomPersonName,
                                               OFString &xmlPersonName,
                                               const OFBool writeEmptyValue)
{
    OFString str1, str2, str3, str4, str5;
    if (DcmPersonName::getNameComponentsFromString(dicomPersonName, str1, str2, str3, str4, str5, 0 /*componentGroup*/).good())
    {
        OFBool newLine = OFFalse;
        OFString xmlString;
        xmlPersonName.clear();
        /* prefix */
        if (writeEmptyValue || !str4.empty())
        {
            xmlPersonName += "<prefix>";
            xmlPersonName += convertToMarkupString(str4, xmlString);
            xmlPersonName += "</prefix>";
            newLine = OFTrue;
        }
        /* first name */
        if (writeEmptyValue || !str2.empty())
        {
            if (newLine)
            {
                xmlPersonName += '\n';
                newLine = OFFalse;
            }
            xmlPersonName += "<first>";
            xmlPersonName += convertToMarkupString(str2, xmlString);
            xmlPersonName += "</first>";
            newLine = OFTrue;
        }
        /* middle name */
        if (writeEmptyValue || !str3.empty())
        {
            if (newLine)
            {
                xmlPersonName += '\n';
                newLine = OFFalse;
            }
            xmlPersonName += "<middle>";
            xmlPersonName += convertToMarkupString(str3, xmlString);
            xmlPersonName += "</middle>";
            newLine = OFTrue;
        }
        /* last name */
        if (writeEmptyValue || !str1.empty())
        {
            if (newLine)
            {
                xmlPersonName += '\n';
                newLine = OFFalse;
            }
            xmlPersonName += "<last>";
            xmlPersonName += convertToMarkupString(str1, xmlString);
            xmlPersonName += "</last>";
            newLine = OFTrue;
        }
        /* suffix */
        if (writeEmptyValue || !str5.empty())
        {
            if (newLine)
            {
                xmlPersonName += '\n';
                newLine = OFFalse;
            }
            xmlPersonName += "<suffix>";
            xmlPersonName += convertToMarkupString(str5, xmlString);
            xmlPersonName += "</suffix>";
            newLine = OFTrue;
        }
    } else
        xmlPersonName = dicomPersonName;
    return xmlPersonName;
}


const char *DSRTypes::numberToString(const size_t number,
                                     char *stringValue)
{
    if (stringValue != NULL)
    {
        /* unsigned long */
        sprintf(stringValue, "%lu", OFstatic_cast(unsigned long, number));
    }
    return stringValue;
}


size_t DSRTypes::stringToNumber(const char *stringValue)
{
    size_t result = 0;
    if (stringValue != NULL)
    {
        unsigned long lu_value = 0;
        /* unsigned long */
        if (sscanf(stringValue, "%lu", &lu_value) == 1)
            result = OFstatic_cast(size_t, lu_value);
    }
    return result;
}


const OFString &DSRTypes::convertToPrintString(const OFString &sourceString,
                                               OFString &printString)
{
    /* char ptr allows fastest access to the string */
    const char *str = sourceString.c_str();
    const size_t count = strlen(str);
    /* start with empty string */
    printString.clear();
    /* avoid to resize the string too often */
    printString.reserve(count);
    for (size_t i = 0; i < count; i++)
    {
        /* newline: depends on OS */
        if (*str == '\n')
            printString += "\\n";
        /* line feed: LF */
        else if (*str == '\012')
            printString += "\\012";
        /* return: CR */
        else if (*str == '\r')
            printString += "\\r";
        /* other character: just append */
        else
            printString += *str;
        str++;
    }
    return printString;
}


const OFString &DSRTypes::convertToMarkupString(const OFString &sourceString,
                                                OFString &markupString,
                                                const OFBool convertNonASCII,
                                                const OFBool newlineAllowed,
                                                const OFBool xmlMode)
{
    /* NB: the order of the parameters 'newlineAllowed' and 'xmlMode' is interchanged! */
    return OFStandard::convertToMarkupString(sourceString, markupString, convertNonASCII, xmlMode, newlineAllowed);
}


OFBool DSRTypes::checkForValidUIDFormat(const OFString &stringValue)
{
    OFBool result = OFFalse;
    /* empty strings are invalid */
    if (!stringValue.empty())
    {
        const char *p = stringValue.c_str();
        if (p != NULL)
        {
            /* check for leading number */
            while (isdigit(*p))
            {
                result = OFTrue;
                p++;
            }
            /* check for separator */
            while ((*p == '.') && result)
            {
                /* trailing '.' is invalid */
                result = OFFalse;
                p++;
                /* check for trailing number */
                while (isdigit(*p))
                {
                    result = OFTrue;
                    p++;
                }
            }
            /* all characters checked? */
            if (*p != 0)
                result = OFFalse;
        }
    }
    return result;
}


DSRIODConstraintChecker *DSRTypes::createIODConstraintChecker(const E_DocumentType documentType)
{
    DSRIODConstraintChecker *checker = NULL;
    switch (documentType)
    {
        case DT_BasicTextSR:
            checker = new DSRBasicTextSRConstraintChecker();
            break;
        case DT_EnhancedSR:
            checker = new DSREnhancedSRConstraintChecker();
            break;
        case DT_ComprehensiveSR:
            checker = new DSRComprehensiveSRConstraintChecker();
            break;
        case DT_KeyObjectDoc:
            checker = new DSRKeyObjectDocConstraintChecker();
            break;
        case DT_MammographyCadSR:
            checker = new DSRMammographyCadSRConstraintChecker();
            break;
        case DT_ChestCadSR:
            checker = new DSRChestCadSRConstraintChecker();
            break;
        case DT_ProcedureLog:
            checker = new DSRProcedureLogConstraintChecker();
            break;
        case DT_XRayRadiationDoseSR:
            checker = new DSRXRayRadiationDoseSRConstraintChecker();
            break;
        default:
            break;
    }
    return checker;
}


DSRDocumentTreeNode *DSRTypes::createDocumentTreeNode(const E_RelationshipType relationshipType,
                                                      const E_ValueType valueType)
{
    DSRDocumentTreeNode *node = NULL;
    switch (valueType)
    {
        case VT_Text:
            node = new DSRTextTreeNode(relationshipType);
            break;
        case VT_Code:
            node = new DSRCodeTreeNode(relationshipType);
            break;
        case VT_Num:
            node = new DSRNumTreeNode(relationshipType);
            break;
        case VT_DateTime:
            node = new DSRDateTimeTreeNode(relationshipType);
            break;
        case VT_Date:
            node = new DSRDateTreeNode(relationshipType);
            break;
        case VT_Time:
            node = new DSRTimeTreeNode(relationshipType);
            break;
        case VT_UIDRef:
            node = new DSRUIDRefTreeNode(relationshipType);
            break;
        case VT_PName:
            node = new DSRPNameTreeNode(relationshipType);
            break;
        case VT_SCoord:
            node = new DSRSCoordTreeNode(relationshipType);
            break;
        case VT_TCoord:
            node = new DSRTCoordTreeNode(relationshipType);
            break;
        case VT_Composite:
            node = new DSRCompositeTreeNode(relationshipType);
            break;
        case VT_Image:
            node = new DSRImageTreeNode(relationshipType);
            break;
        case VT_Waveform:
            node = new DSRWaveformTreeNode(relationshipType);
            break;
        case VT_Container:
            node = new DSRContainerTreeNode(relationshipType);
            break;
        case VT_byReference:
            node = new DSRByReferenceTreeNode(relationshipType);
            break;
        default:
            break;
    }
    return node;
}


void DSRTypes::printMessage(OFConsole *stream,
                            const char *message)
{
    if ((stream != NULL) && (message != NULL))
    {
        stream->lockCerr() << message << endl;
        stream->unlockCerr();
    }
}


void DSRTypes::printWarningMessage(OFConsole *stream,
                                   const char *message)
{
    if ((stream != NULL) && (message != NULL))
    {
        stream->lockCerr() << "DCMSR - Warning: " << message << endl;
        stream->unlockCerr();
    }
}


void DSRTypes::printErrorMessage(OFConsole *stream,
                                 const char *message)
{
    if ((stream != NULL) && (message != NULL))
    {
        stream->lockCerr() << "DCMSR - Error: " << message << endl;
        stream->unlockCerr();
    }
}


void DSRTypes::printInvalidContentItemMessage(OFConsole *stream,
                                              const char *action,
                                              const DSRDocumentTreeNode *node,
                                              const char *location)
{
    if (stream != NULL)
    {
        OFString message;
        if (action != NULL)
            message += action;
        else
            message += "Processing";
        message += " invalid/incomplete content item";
        if (node != NULL)
        {
            message += " ";
            message += valueTypeToDefinedTerm(node->getValueType());
        }
        if (location != NULL)
        {
            message += " \"";
            message += location;
            message += "\"";
        }
        printWarningMessage(stream, message.c_str());
    }
}


void DSRTypes::printContentItemErrorMessage(OFConsole *stream,
                                            const char *action,
                                            const OFCondition &result,
                                            const DSRDocumentTreeNode *node,
                                            const char *location)
{
    if ((stream != NULL) && result.bad())
    {
        OFString message;
        if (action != NULL)
            message += action;
        else
            message += "Processing";
        message += " content item";
        if (node != NULL)
        {
            message += " ";
            message += valueTypeToDefinedTerm(node->getValueType());
        }
        if (location != NULL)
        {
            message += " \"";
            message += location;
            message += "\"";
        }
        message += " (";
        message += result.text();
        message += ")";
        printErrorMessage(stream, message.c_str());
    }
}


void DSRTypes::printUnknownValueWarningMessage(OFConsole *stream,
                                               const char *valueName,
                                               const char *readValue,
                                               const char *action)
{
    if ((stream != NULL) && (valueName != NULL))
    {
        OFString message;
        if (action != NULL)
            message += action;
        else
            message += "Processing";
        message += " unknown/unsupported ";
        message += valueName;
        if ((readValue != NULL) && (strlen(readValue) > 0))
        {
            message += " (";
            message += readValue;
            message += ")";
        }
        printWarningMessage(stream, message.c_str());
    }
}


OFBool DSRTypes::writeStringValueToXML(ostream &stream,
                                       const OFString &stringValue,
                                       const OFString &tagName,
                                       const OFBool writeEmptyValue)
{
    OFBool result = OFFalse;
    if (!stringValue.empty() || writeEmptyValue)
    {
        OFString tmpString;
        stream << "<" << tagName << ">";
        stream << convertToMarkupString(stringValue, tmpString, OFFalse /*convertNonASCII*/, OFFalse /*newlineAllowed*/, OFTrue /*xmlMode*/);
        stream << "</" << tagName << ">" << endl;
        result = OFTrue;
    }
    return result;
}


OFBool DSRTypes::writeStringFromElementToXML(ostream &stream,
                                             DcmElement &delem,
                                             const OFString &tagName,
                                             const OFBool writeEmptyValue)
{
    OFBool result = OFFalse;
    if ((delem.getLength() > 0) || writeEmptyValue)
    {
        OFString tmpString;
        stream << "<" << tagName << ">";
        if (delem.getVR() == EVR_PN)        // special formatting for person names
        {
            OFString xmlString;
            stream << endl << dicomToXMLPersonName(getStringValueFromElement(delem, tmpString), xmlString, writeEmptyValue) << endl;
        } else
            stream << getMarkupStringFromElement(delem, tmpString);
        stream << "</" << tagName << ">" << endl;
        result = OFTrue;
    }
    return result;
}


size_t DSRTypes::createHTMLAnnexEntry(ostream &docStream,
                                      ostream &annexStream,
                                      const OFString &referenceText,
                                      size_t &annexNumber)
{
    /* hyperlink to corresponding annex */
    docStream << "[";
    if (!referenceText.empty())
        docStream << referenceText << " ";
    docStream << "<a name=\"annex_src_" << annexNumber << "\" href=\"#annex_dst_" << annexNumber << "\">Annex " << annexNumber << "</a>]" << endl;
    /* create new annex */
    annexStream << "<h2><a name=\"annex_dst_" << annexNumber << "\" href=\"#annex_src_" << annexNumber << "\">Annex " << annexNumber << "</a></h2>" << endl;
    /* increase annex number, return previous number */
    return annexNumber++;
}


size_t DSRTypes::createHTMLFootnote(ostream &docStream,
                                    ostream &footnoteStream,
                                    size_t &footnoteNumber,
                                    const size_t nodeID)
{
    /* hyperlink to corresponding footnote */
    docStream << "<sup><small><a name=\"footnote_src_" << nodeID << "_" << footnoteNumber << "\" ";
    docStream << "href=\"#footnote_dst_" << nodeID << "_" << footnoteNumber << "\">" << footnoteNumber << "</a></small></sup>" << endl;
    /* create new footnote */
    footnoteStream << "<b><a name=\"footnote_dst_" << nodeID << "_" << footnoteNumber << "\" ";
    footnoteStream << "href=\"#footnote_src_" << nodeID << "_" << footnoteNumber << "\">Footnote " << footnoteNumber << "</a></b>" << endl;
    /* increase footnote number, return previous number */
    return footnoteNumber++;
}


OFCondition DSRTypes::appendStream(ostream &mainStream,
                                   OFOStringStream &tempStream,
                                   const char *heading)
{
    OFCondition result = EC_InvalidStream;
    /* add final 0 byte (if required) */
    tempStream << OFStringStream_ends;
    /* freeze/get string (now we have full control over the array) */
    OFSTRINGSTREAM_GETSTR(tempStream, tempString)
    /* should never be NULL */
    if (tempString != NULL)
    {
        if (strlen(tempString) > 0)
        {
            /* append optional heading */
            if (heading != NULL)
                mainStream << heading << endl;
            /* append temporal document to main document */
            mainStream << tempString;
        }
        /* very important! since we have full control we are responsible for deleting the array */
        OFSTRINGSTREAM_FREESTR(tempString)
        result = EC_Normal;
    }
    return result;
}

static OFBool checkForNonASCIICharacters(DcmElement& elem)
{
  char *c = NULL;
  if (elem.getString(c).good() && c)
  {
    while (*c)
    {
      if (OFstatic_cast(unsigned char, *c) > 127) return OFTrue;
      ++c;
    }
  }
  return OFFalse;
}

OFBool DSRTypes::stringContainsExtendedCharacters(const OFString &s)
{
  const char *c = s.c_str();
  if (c)
  {
    while (*c)
    {
      if (OFstatic_cast(unsigned char, *c) > 127) return OFTrue;
      ++c;
    }
  }
  return OFFalse;  
}

OFBool DSRTypes::elementContainsExtendedCharacters(DcmElement &elem)
{
  if (elem.isaString())
  {
    return checkForNonASCIICharacters(elem);
  }
  else if (! elem.isLeaf()) // element is a sequence
  {  
    DcmStack stack;
    while (elem.nextObject(stack, OFTrue).good())
    {
      if (stack.top()->isaString())
      {
        if (checkForNonASCIICharacters(* OFstatic_cast(DcmElement *, stack.top()))) 
          return OFTrue;
      }
    }  
    return OFFalse;
  }
  return OFFalse;
}

/*
 *  CVS/RCS Log:
 *  $Log: dsrtypes.cc,v $
 *  Revision 1.48  2005/12/08 15:48:19  meichel
 *  Changed include path schema for all DCMTK header files
 *
 *  Revision 1.47  2005/11/30 12:01:15  joergr
 *  Added support for X-Ray Radiation Dose SR documents.
 *
 *  Revision 1.46  2004/11/29 17:11:37  joergr
 *  Added warning message when character set is unknown, unsupported  or cannot
 *  be mapped to the output format. Added support for UTF-8 character set.
 *
 *  Revision 1.45  2004/11/22 16:35:40  meichel
 *  Added helper methods to check strings and DICOM elements for presence of
 *    extended (non-ASCII) characters
 *
 *  Revision 1.44  2004/09/09 14:02:02  joergr
 *  Added flags to control the way the template identification is encoded in
 *  writeXML() and expected in readXML().
 *
 *  Revision 1.43  2004/02/11 15:58:32  joergr
 *  Renamed UID_ProcedureLog to UID_ProcedureLogStorage.
 *
 *  Revision 1.42  2004/01/20 15:37:39  joergr
 *  Added new command line option which allows to write the item identifier "id"
 *  (XML attribute) even if it is not required (because the item is not referenced
 *  by any other item). Useful for debugging purposes.
 *
 *  Revision 1.41  2004/01/16 10:09:45  joergr
 *  Replaced OFString::resize() by ..reserve() in convertToPrintString().
 *
 *  Revision 1.40  2004/01/05 14:37:23  joergr
 *  Removed acknowledgements with e-mail addresses from CVS log.
 *
 *  Revision 1.39  2003/12/08 13:05:48  joergr
 *  Return more appropriate error codes in getAndCheckXXX() routines.
 *
 *  Revision 1.38  2003/12/01 15:47:28  joergr
 *  Changed XML encoding of by-reference relationships if flag
 *  XF_valueTypeAsAttribute is set.
 *
 *  Revision 1.37  2003/10/30 17:51:43  joergr
 *  Added new command line options which allow to print/write the template
 *  identification of a content item.
 *
 *  Revision 1.36  2003/10/09 12:58:19  joergr
 *  Added support for Procedure Log.
 *
 *  Revision 1.35  2003/10/06 09:55:35  joergr
 *  Added new flag which allows to ignore content item errors when reading an SR
 *  document (e.g. missing value type specific attributes).
 *
 *  Revision 1.34  2003/09/15 14:13:42  joergr
 *  Introduced new class to facilitate checking of SR IOD relationship content
 *  constraints. Replaced old implementation distributed over numerous classes.
 *
 *  Revision 1.33  2003/09/10 13:18:43  joergr
 *  Replaced PrivateCodingSchemeUID by new CodingSchemeIdenticationSequence as
 *  required by CP 324.
 *
 *  Revision 1.32  2003/08/07 14:14:46  joergr
 *  Added readXML functionality. Added support for Chest CAD SR.
 *  Added new option --add-schema-reference to command line tool dsr2xml.
 *  Renamed parameters/variables "string" to avoid name clash with STL class.
 *  Adapted type casts to new-style typecast operators defined in ofcast.h.
 *
 *  Revision 1.31  2003/04/01 14:59:41  joergr
 *  Added support for XML namespaces.
 *
 *  Revision 1.30  2002/11/27 14:36:18  meichel
 *  Adapted module dcmsr to use of new header file ofstdinc.h
 *
 *  Revision 1.29  2002/08/20 12:53:57  meichel
 *  Added explicit includes for header files included implicitly
 *    via dcstream before.
 *
 *  Revision 1.28  2002/08/02 12:39:07  joergr
 *  Enhanced debug output of dcmsr::read() routines (e.g. add position string
 *  of invalid content items to error messages).
 *
 *  Revision 1.27  2002/07/22 14:22:34  joergr
 *  Added new print flag to suppress the output of general document information.
 *
 *  Revision 1.26  2002/05/07 12:54:28  joergr
 *  Added support for the Current Requested Procedure Evidence Sequence and the
 *  Pertinent Other Evidence Sequence to the dcmsr module.
 *
 *  Revision 1.25  2002/05/02 14:08:36  joergr
 *  Added support for standard and non-standard string streams (which one is
 *  supported is detected automatically via the configure mechanism).
 *
 *  Revision 1.24  2002/04/25 09:15:39  joergr
 *  Moved helper function which converts a conventional character string to an
 *  HTML/XML mnenonic string (e.g. using "&lt;" instead of "<") from module
 *  dcmsr to ofstd.
 *
 *  Revision 1.23  2001/11/09 16:20:18  joergr
 *  Added new command line option allowing to encode codes as XML attributes
 *  (instead of tags).
 *  Added preliminary support for Mammography CAD SR.
 *
 *  Revision 1.22  2001/10/10 15:29:18  joergr
 *  Changed parameter DcmTagKey to DcmTag in DcmItem::putAndInsert... methods
 *  to support elements which are not in the data dictionary (e.g. private
 *  extensions).
 *  Additonal adjustments for new OFCondition class.
 *
 *  Revision 1.21  2001/10/02 12:07:11  joergr
 *  Adapted module "dcmsr" to the new class OFCondition. Introduced module
 *  specific error codes.
 *
 *  Revision 1.20  2001/10/01 15:11:37  joergr
 *  Introduced new general purpose functions to get/set person names, date, time
 *  and date/time.
 *
 *  Revision 1.19  2001/09/26 13:04:28  meichel
 *  Adapted dcmsr to class OFCondition
 *
 *  Revision 1.18  2001/06/20 15:05:22  joergr
 *  Added minimal support for new SOP class Key Object Selection Document
 *  (suppl. 59).
 *  Added new debugging features (additional flags) to examine "corrupted" SR
 *  documents.
 *
 *  Revision 1.17  2001/04/03 08:25:18  joergr
 *  Added new command line option: ignore relationship content constraints
 *  specified for each SR document class.
 *
 *  Revision 1.16  2001/02/13 16:34:09  joergr
 *  Allow newline characters (encoded as &#182;) in XML documents.
 *
 *  Revision 1.15  2001/02/02 14:41:51  joergr
 *  Added new option to dsr2xml allowing to specify whether value and/or
 *  relationship type are to be encoded as XML attributes or elements.
 *
 *  Revision 1.14  2001/01/25 11:50:10  joergr
 *  Always remove signature sequences from certain dataset sequences (e.g.
 *  VerifyingObserver or PredecessorDocuments).
 *
 *  Revision 1.13  2001/01/18 15:56:46  joergr
 *  Encode PN components in separate XML tags.
 *
 *  Revision 1.12  2000/12/12 17:21:21  joergr
 *  Added explicit typecast to keep gcc 2.7 quiet.
 *
 *  Revision 1.11  2000/12/08 13:46:00  joergr
 *  Removed optional fractional second part from time value.
 *
 *  Revision 1.10  2000/11/16 13:32:11  joergr
 *  Fixed bug in dicomToReadablePersonName().
 *
 *  Revision 1.9  2000/11/09 20:34:02  joergr
 *  Added support for non-ASCII characters in HTML 3.2 (use numeric value).
 *
 *  Revision 1.8  2000/11/09 11:36:07  joergr
 *  Minor HTML code purifications.
 *  Reordered renderHTML flags (internal flags to the end).
 *
 *  Revision 1.7  2000/11/07 18:32:01  joergr
 *  Enhanced rendered HTML output of date, time, datetime and pname.
 *  Added new command line option allowing to choose code value or meaning to be
 *  rendered as the numeric measurement unit.
 *
 *  Revision 1.6  2000/11/01 16:36:11  joergr
 *  Added support for conversion to XML.
 *  Added support for Cascading Style Sheet (CSS) used optionally for HTML
 *  rendering.
 *  Enhanced support for specific character sets.
 *
 *  Revision 1.5  2000/10/26 14:36:32  joergr
 *  Added support for "Comprehensive SR".
 *  Added support for TCOORD content item.
 *  Added new flag specifying whether to add a "dcmtk" footnote to the rendered
 *  HTML document or not.
 *  Added check routine for valid UID strings.
 *
 *  Revision 1.4  2000/10/18 17:23:58  joergr
 *  Added new method allowing to get and check string values from dataset.
 *
 *  Revision 1.3  2000/10/16 12:09:28  joergr
 *  Added new options: number nested items instead of indenting them, print SOP
 *  instance UID of referenced composite objects.
 *
 *  Revision 1.2  2000/10/13 08:53:33  joergr
 *  Removed typedef statements to keep MSVC++ quiet.
 *
 *  Revision 1.1  2000/10/13 07:52:27  joergr
 *  Added new module 'dcmsr' providing access to DICOM structured reporting
 *  documents (supplement 23).  Doc++ documentation not yet completed.
 *
 *
 */
