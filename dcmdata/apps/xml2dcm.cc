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
 *  Module:  dcmdata
 *
 *  Author:  Joerg Riesmeier
 *
 *  Purpose: Convert XML document to DICOM file or data set
 *
 *  Last Update:      $Author: meichel $
 *  Update Date:      $Date: 2005/12/16 15:46:41 $
 *  CVS/RCS Revision: $Revision: 1.16 $
 *  Status:           $State: Exp $
 *
 *  CVS/RCS Log at end of file
 *
 */


#include "dcmtk/config/osconfig.h"    /* make sure OS specific configuration is included first */

#include "dcmtk/dcmdata/dctk.h"
#include "dcmtk/dcmdata/dcpxitem.h"    /* for class DcmPixelItem */
#include "dcmtk/dcmdata/cmdlnarg.h"
#include "dcmtk/dcmdata/dcdebug.h"
#include "dcmtk/ofstd/ofstd.h"
#include "dcmtk/ofstd/ofconapp.h"
#include "dcmtk/dcmdata/dcdebug.h"

#define INCLUDE_CSTDARG
#include "dcmtk/ofstd/ofstdinc.h"

#ifdef WITH_ZLIB
#include <zlib.h>        /* for zlibVersion() */
#endif

#define OFFIS_CONSOLE_APPLICATION "xml2dcm"
#define OFFIS_CONSOLE_DESCRIPTION "Convert XML document to DICOM file or data set"

// currently not used since DTD is always retrieved from XML document
//#define DOCUMENT_TYPE_DEFINITION_FILE "dcm2xml.dtd"

static char rcsid[] = "$dcmtk: " OFFIS_CONSOLE_APPLICATION " v"
  OFFIS_DCMTK_VERSION " " OFFIS_DCMTK_RELEASEDATE " $";

// ********************************************

#ifdef WITH_LIBXML

#include <libxml/parser.h>

// stores pointer to character encoding handler
static xmlCharEncodingHandlerPtr EncodingHandler = NULL;


#ifdef HAVE_VPRINTF
// function required to avoid issue with 'std' namespace
extern "C" void errorFunction(void *ctx, const char *msg, ...)
{
    va_list ap;
    va_start(ap, msg);
#ifdef HAVE_PROTOTYPE_STD__VFPRINTF
    std::vfprintf(OFstatic_cast(FILE *, ctx), msg, ap);
#else
    vfprintf(OFstatic_cast(FILE *, ctx), msg, ap);
#endif
    va_end(ap);
}
#endif

// libxml shall be quiet in non-debug mode
extern "C" void noErrorFunction(void * /*ctx*/, const char * /*msg*/, ...)
{
    /* do nothing */
}


static OFBool convertUtf8ToCharset(const xmlChar *fromString,
                                   OFString &toString)
{
    OFBool result = OFFalse;
    if (EncodingHandler != NULL)
    {
        /* prepare input/output buffers */
        xmlBufferPtr fromBuffer = xmlBufferCreate();
        xmlBufferPtr toBuffer = xmlBufferCreate();
        xmlBufferCat(fromBuffer, fromString);
        /* convert character encoding of given string */
        result = (xmlCharEncOutFunc(EncodingHandler, toBuffer, fromBuffer) >= 0);
        if (result)
            toString = OFreinterpret_cast(const char *, xmlBufferContent(toBuffer));
        /* free allocated memory */
        xmlBufferFree(toBuffer);
        xmlBufferFree(fromBuffer);
    }
    return result;
}


static OFCondition checkNode(xmlNodePtr current,
                             const char *name)
{
    OFCondition result = EC_Normal;
    /* check whether node is valid at all */
    if (current != NULL)
    {
        /* check whether node has expected name */
        if (xmlStrcmp(current->name, OFreinterpret_cast(const xmlChar *, name)) != 0)
        {
            CERR << "Error: document of the wrong type, was '" << current->name << "', '" << name << "' expected" << endl;
            result = EC_IllegalCall;
        }
    } else {
        CERR << "Error: document of the wrong type, '" << name << "' expected" << endl;
        result = EC_IllegalCall;
    }
    return result;
}


static OFCondition createNewElement(xmlNodePtr current,
                                    DcmElement *&newElem)
{
    OFCondition result = EC_IllegalCall;
    /* check whether node is valid */
    if (current != NULL)
    {
        /* get required information from XML element */
        xmlChar *elemTag = xmlGetProp(current, OFreinterpret_cast(const xmlChar *, "tag"));
        xmlChar *elemVR = xmlGetProp(current, OFreinterpret_cast(const xmlChar *, "vr"));
        /* convert tag string */
        DcmTagKey dcmTagKey;
        unsigned int group = 0xffff;
        unsigned int elem = 0xffff;
        if (sscanf(OFreinterpret_cast(char *, elemTag), "%x,%x", &group, &elem ) == 2)
        {
            dcmTagKey.set(group, elem);
            DcmTag dcmTag(dcmTagKey);
            /* convert vr string */
            DcmVR dcmVR(OFreinterpret_cast(char *, elemVR));
            DcmEVR dcmEVR = dcmVR.getEVR();
            if (dcmEVR == EVR_UNKNOWN)
            {
                CERR << "Warning: invalid 'vr' attribute (" << elemVR
                     << ") for " << dcmTag << ", using unknown VR" << endl;
            }
            /* check for correct vr */
            const DcmEVR tagEVR = dcmTag.getEVR();
            if ((tagEVR != dcmEVR) && (dcmEVR != EVR_UNKNOWN) && (tagEVR != EVR_UNKNOWN) &&
                ((dcmTagKey != DCM_LUTData) || ((dcmEVR != EVR_US) && (dcmEVR != EVR_SS) && (dcmEVR != EVR_OW))) &&
                ((tagEVR != EVR_xs) || ((dcmEVR != EVR_US) && (dcmEVR != EVR_SS))) &&
                ((tagEVR != EVR_ox) || ((dcmEVR != EVR_OB) && (dcmEVR != EVR_OW))))
            {
                CERR << "Warning: tag " << dcmTag << " has wrong VR (" << dcmVR.getVRName()
                     << "), correct is " << dcmTag.getVR().getVRName() << endl;
            }
            if (dcmEVR != EVR_UNKNOWN)
                dcmTag.setVR(dcmVR);
            /* create DICOM element */
            result = newDicomElement(newElem, dcmTag);
        } else {
            CERR << "Warning: invalid 'tag' attribute (" << elemTag << "), ignoring node" << endl;
            result = EC_InvalidTag;
        }
        if (result.bad())
        {
            /* delete new element if an error occured */
            delete newElem;
            newElem = NULL;
        }
        /* free allocated memory */
        xmlFree(elemTag);
        xmlFree(elemVR);
    }
    return result;
}


static OFCondition putElementContent(xmlNodePtr current,
                                     DcmElement *element)
{
    OFCondition result = EC_IllegalCall;
    /* check whether node and element are valid */
    if ((current != NULL) && (element != NULL))
    {
        DcmEVR dcmEVR = element->getVR();
        /* get the XML node content */
        xmlChar *elemVal = xmlNodeGetContent(current);
        xmlChar *attrVal = xmlGetProp(current, OFreinterpret_cast(const xmlChar *, "binary"));
        /* check whether node content is present */
        if (xmlStrcmp(attrVal, OFreinterpret_cast(const xmlChar *, "hidden")) == 0)
            CERR << "Warning: content of node " << element->getTag() << " is 'hidden', empty element inserted" << endl;
        /* check whether node content is base64 encoded */
        else if (xmlStrcmp(attrVal, OFreinterpret_cast(const xmlChar *, "base64")) == 0)
        {
            /* tbd: byte ordering?? */
            Uint8 *data = NULL;
            const size_t length = OFStandard::decodeBase64(OFreinterpret_cast(char *, elemVal), data);
            if (length > 0)
            {
                if (dcmEVR == EVR_OW)
                {
                    /* Base64 decoder produces big endian output data, convert to local byte order */
                    swapIfNecessary(gLocalByteOrder, EBO_BigEndian, data, length, sizeof(Uint16));
                }
                result = element->putUint8Array(data, length);
                if (result.bad())
                    delete[] data;
            }
        } else {
            OFString dicomVal;
            /* convert character set from UTF8 (for specific VRs only) */
            if (((dcmEVR == EVR_PN) || (dcmEVR == EVR_SH) || (dcmEVR == EVR_LO) ||
                 (dcmEVR == EVR_ST) || (dcmEVR == EVR_LT) || (dcmEVR == EVR_UT)) &&
                (xmlStrlen(elemVal) > 0) && convertUtf8ToCharset(elemVal, dicomVal))
            {
                result = element->putString(dicomVal.c_str());
            } else {
                /* set the value of the newly created element */
                result = element->putString(OFreinterpret_cast(char *, elemVal));
            }
        }
        /* free allocated memory */
        xmlFree(elemVal);
        xmlFree(attrVal);
    }
    return result;
}


static OFCondition parseElement(DcmItem *dataset,
                                xmlNodePtr current)
{
    DcmElement *newElem = NULL;
    /* create new DICOM element from XML element */
    OFCondition result = createNewElement(current, newElem);
    if (result.good())
    {
        /* retrieve specific character set */
        if (newElem->getTag() == DCM_SpecificCharacterSet)
        {
            if (EncodingHandler == NULL)
            {
                const char *encString = NULL;
                xmlChar *elemVal = xmlNodeGetContent(current);
                /* check for known character set */
                if (xmlStrcmp(elemVal, OFreinterpret_cast(const xmlChar *, "ISO_IR 6")) == 0)
                    encString = "UTF-8";
                else if (xmlStrcmp(elemVal, OFreinterpret_cast(const xmlChar *, "ISO_IR 192")) == 0)
                    encString = "UTF-8";
                else if (xmlStrcmp(elemVal, OFreinterpret_cast(const xmlChar *, "ISO_IR 100")) == 0)
                    encString = "ISO-8859-1";
                else if (xmlStrcmp(elemVal, OFreinterpret_cast(const xmlChar *, "ISO_IR 101")) == 0)
                    encString = "ISO-8859-2";
                else if (xmlStrcmp(elemVal, OFreinterpret_cast(const xmlChar *, "ISO_IR 109")) == 0)
                    encString = "ISO-8859-3";
                else if (xmlStrcmp(elemVal, OFreinterpret_cast(const xmlChar *, "ISO_IR 110")) == 0)
                    encString = "ISO-8859-4";
                else if (xmlStrcmp(elemVal, OFreinterpret_cast(const xmlChar *, "ISO_IR 148")) == 0)
                    encString = "ISO-8859-9";
                else if (xmlStrcmp(elemVal, OFreinterpret_cast(const xmlChar *, "ISO_IR 144")) == 0)
                    encString = "ISO-8859-5";
                else if (xmlStrcmp(elemVal, OFreinterpret_cast(const xmlChar *, "ISO_IR 127")) == 0)
                    encString = "ISO-8859-6";
                else if (xmlStrcmp(elemVal, OFreinterpret_cast(const xmlChar *, "ISO_IR 126")) == 0)
                    encString = "ISO-8859-7";
                else if (xmlStrcmp(elemVal, OFreinterpret_cast(const xmlChar *, "ISO_IR 138")) == 0)
                    encString = "ISO-8859-8";
                else if (xmlStrlen(elemVal) > 0)
                    CERR << "Warning: character set '" << elemVal << "' not supported" << endl;
                if (encString != NULL)
                {
                    /* find appropriate encoding handler */
                    EncodingHandler = xmlFindCharEncodingHandler(encString);
                }
                xmlFree(elemVal);
           } else
                CERR << "Warning: encoding handler already set ... ignoring 'SpecificCharacterSet'" << endl;
        }
        /* set the element value */
        result = putElementContent(current, newElem);
        /* insert the new element into the dataset */
        if (result.good())
            result = dataset->insert(newElem, OFTrue /*replaceOld*/);
        if (result.bad())
        {
            /* delete element if insertion or putting the value failed */
            delete newElem;
        }
    }
    return result;
}


// forward declaration
static OFCondition parseDataSet(DcmItem *dataset,
                                xmlNodePtr current,
                                E_TransferSyntax xfer);


static OFCondition parseSequence(DcmSequenceOfItems *sequence,
                                 xmlNodePtr current,
                                 E_TransferSyntax xfer)
{
    OFCondition result = EC_IllegalCall;
    if (sequence != NULL)
    {
        /* ignore blank (empty or whitespace only) nodes */
        while ((current != NULL) && xmlIsBlankNode(current))
            current = current->next;
        while (current != NULL)
        {
            /* ignore non-item nodes */
            if (xmlStrcmp(current->name, OFreinterpret_cast(const xmlChar *, "item")) == 0)
            {
                /* create new sequence item */
                DcmItem *newItem = new DcmItem();
                if (newItem != NULL)
                {
                    sequence->insert(newItem);
                    /* proceed parsing the item content */
                    parseDataSet(newItem, current->xmlChildrenNode, xfer);
                }
            } else if (!xmlIsBlankNode(current))
                CERR << "Warning: unexpected node '" << current->name << "', 'item' expected, skipping" << endl;
            /* proceed with next node */
            current = current->next;
        }
        result = EC_Normal;
    }
    return result;
}


static OFCondition parsePixelSequence(DcmPixelSequence *sequence,
                                      xmlNodePtr current)
{
    OFCondition result = EC_IllegalCall;
    if (sequence != NULL)
    {
        /* ignore blank (empty or whitespace only) nodes */
        while ((current != NULL) && xmlIsBlankNode(current))
            current = current->next;
        while (current != NULL)
        {
            /* ignore non-pixel-item nodes */
            if (xmlStrcmp(current->name, OFreinterpret_cast(const xmlChar *, "pixel-item")) == 0)
            {
                /* create new pixel item */
                DcmPixelItem *newItem = new DcmPixelItem(DcmTag(DCM_Item, EVR_OB));
                if (newItem != NULL)
                {
                    sequence->insert(newItem);
                    /* put pixel data into the item */
                    putElementContent(current, newItem);
                }
            } else if (!xmlIsBlankNode(current))
                CERR << "Warning: unexpected node '" << current->name << "', 'pixel-item' expected, skipping" << endl;
            /* proceed with next node */
            current = current->next;
        }
        result = EC_Normal;
    }
    return result;
}


static OFCondition parseMetaHeader(DcmMetaInfo *metainfo,
                                   xmlNodePtr current,
                                   const OFBool parse)
{
    /* check for valid node and correct name */
    OFCondition result = checkNode(current, "meta-header");
    if (result.good() && parse)
    {
        /* get child nodes */
        current = current->xmlChildrenNode;
        while (current != NULL)
        {
            /* ignore non-element nodes */
            if (xmlStrcmp(current->name, OFreinterpret_cast(const xmlChar *, "element")) == 0)
                parseElement(metainfo, current);
            else if (!xmlIsBlankNode(current))
                CERR << "Warning: unexpected node '" << current->name << "', 'element' expected, skipping" << endl;
            /* proceed with next node */
            current = current->next;
        }
    }
    return result;
}


static OFCondition parseDataSet(DcmItem *dataset,
                                xmlNodePtr current,
                                E_TransferSyntax xfer)
{
    OFCondition result = EC_Normal;
    /* ignore blank (empty or whitespace only) nodes */
    while ((current != NULL) && xmlIsBlankNode(current))
        current = current->next;
    while (current != NULL)
    {
        /* ignore non-element/sequence nodes */
        if (xmlStrcmp(current->name, OFreinterpret_cast(const xmlChar *, "element")) == 0)
            parseElement(dataset, current);
        else if (xmlStrcmp(current->name, OFreinterpret_cast(const xmlChar *, "sequence")) == 0)
        {
            DcmElement *newElem = NULL;
            /* create new sequence element */
            if (createNewElement(current, newElem).good())
            {
                /* insert new sequence element into the dataset */
                result = dataset->insert(newElem, OFTrue /*replaceOld*/);
                if (result.good())
                {
                    /* special handling for compressed pixel data */
                    if (newElem->getTag() == DCM_PixelData)
                    {
                        /* create new pixel sequence */
                        DcmPixelSequence *sequence = new DcmPixelSequence(DcmTag(DCM_PixelData, EVR_OB));
                        if (sequence != NULL)
                        {
                            /* ... insert it into the dataset and proceed with the pixel items */
                            OFstatic_cast(DcmPixelData *, newElem)->putOriginalRepresentation(xfer, NULL, sequence);
                            parsePixelSequence(sequence, current->xmlChildrenNode);
                        }
                    } else {
                        /* proceed parsing the items of the sequence */
                        parseSequence(OFstatic_cast(DcmSequenceOfItems *, newElem), current->xmlChildrenNode, xfer);
                    }
                } else {
                    /* delete element if insertion failed */
                    delete newElem;
                }
            }
        } else if (!xmlIsBlankNode(current))
            CERR << "Warning: unexpected node '" << current->name << "', skipping" << endl;
        /* proceed with next node */
        current = current->next;
    }
    return result;
}


static OFCondition validateXmlDocument(xmlDocPtr doc,
                                       const OFBool verbose,
                                       const OFBool debug)
{
    OFCondition result = EC_Normal;
    if (verbose)
        COUT << "validating XML document ..." << endl;
    xmlGenericError(xmlGenericErrorContext, "--- libxml validating ---\n");
    /* create context for document validation */
    xmlValidCtxt cvp;
    if (debug)
    {
        cvp.userData = OFstatic_cast(void *, stderr);
#ifdef HAVE_VPRINTF
        cvp.error = errorFunction;
        cvp.warning = errorFunction;
#else
        cvp.error = OFreinterpret_cast(xmlValidityErrorFunc, fprintf);
        cvp.warning = OFreinterpret_cast(xmlValidityWarningFunc, fprintf);
#endif
    } else {
        cvp.userData = NULL;
        cvp.error = NULL;
        cvp.warning = NULL;
    }
    /* validate the document */
    const int valid = xmlValidateDocument(&cvp, doc);
    xmlGenericError(xmlGenericErrorContext, "-------------------------\n");
    if (!valid)
    {
        CERR << "Error: document does not validate" << endl;
        result = EC_IllegalCall;
    }
    return result;
}


static OFCondition readXmlFile(const char *ifname,
                               DcmFileFormat &fileformat,
                               E_TransferSyntax &xfer,
                               const OFBool metaInfo,
                               const OFBool checkNamespace,
                               const OFBool validateDocument,
                               const OFBool verbose,
                               const OFBool debug)
{
    OFCondition result = EC_Normal;
    xfer = EXS_Unknown;
    xmlGenericError(xmlGenericErrorContext, "--- libxml parsing ------\n");
    /* build an XML tree from the file */
    xmlDocPtr doc = xmlParseFile(ifname);
    xmlGenericError(xmlGenericErrorContext, "-------------------------\n");
    if (doc != NULL)
    {
        /* validate document */
        if (validateDocument)
            result = validateXmlDocument(doc, verbose, debug);
        if (result.good())
        {
            /* check whether the document is of the right kind */
            xmlNodePtr current = xmlDocGetRootElement(doc);
            if (current != NULL)
            {
                /* check namespace declaration (if required) */
                if (!checkNamespace || (xmlSearchNsByHref(doc, current, OFreinterpret_cast(const xmlChar *, DCMTK_XML_NAMESPACE_URI)) != NULL))
                {
                    /* check whether to parse a "file-format" or "data-set" */
                    if (xmlStrcmp(current->name, OFreinterpret_cast(const xmlChar *, "file-format")) == 0)
                    {
                        if (verbose)
                        {
                            COUT << "parsing file-format ..." << endl;
                            if (metaInfo)
                                COUT << "parsing meta-header ..." << endl;
                            else
                                COUT << "skipping meta-header ..." << endl;
                        }
                        current = current->xmlChildrenNode;
                        /* ignore blank (empty or whitespace only) nodes */
                        while ((current != NULL) && xmlIsBlankNode(current))
                            current = current->next;
                        /* parse/skip "meta-header" */
                        result = parseMetaHeader(fileformat.getMetaInfo(), current, metaInfo /*parse*/);
                        if (result.good())
                        {
                            current = current->next;
                            /* ignore blank (empty or whitespace only) nodes */
                            while ((current != NULL) && xmlIsBlankNode(current))
                                current = current->next;
                        }
                    }
                    /* there should always be a "data-set" node */
                    if (result.good())
                    {
                        if (verbose)
                            COUT << "parsing data-set ..." << endl;
                        /* parse "data-set" */
                        result = checkNode(current, "data-set");
                        if (result.good())
                        {
                            DcmDataset *dataset = fileformat.getDataset();
                            /* determine stored transfer syntax */
                            xmlChar *xferUID = xmlGetProp(current, OFreinterpret_cast(const xmlChar *, "xfer"));
                            if (xferUID != NULL)
                                xfer = DcmXfer(OFreinterpret_cast(char *, xferUID)).getXfer();
                            result = parseDataSet(dataset, current->xmlChildrenNode, xfer);
                            /* free allocated memory */
                            xmlFree(xferUID);
                        }
                    }
                    if (result.bad())
                    {
                        /* dump XML document for debugging purposes */
                        if (debug)
                            xmlDocDump(stderr, doc);
                    }
                } else {
                    CERR << "Error: document has wrong type, dcmtk namespace not found" << endl;
                    result = EC_IllegalCall;
                }
            } else {
                CERR << "Error: document is empty: " << ifname << endl;
                result = EC_IllegalCall;
            }
        }
    } else {
        CERR << "Error: could not parse document: " << ifname << endl;
        result = EC_IllegalCall;
    }
    /* free allocated memory */
    xmlFreeDoc(doc);
    return result;
}


#define SHORTCOL 3
#define LONGCOL 21


int main(int argc, char *argv[])
{
    int opt_debug = 0;
    OFBool opt_verbose = OFFalse;
    OFBool opt_metaInfo = OFTrue;
    OFBool opt_dataset = OFFalse;
    OFBool opt_namespace = OFFalse;
    OFBool opt_validate = OFFalse;
    E_TransferSyntax opt_xfer = EXS_Unknown;
    E_EncodingType opt_enctype = EET_ExplicitLength;
    E_GrpLenEncoding opt_glenc = EGL_recalcGL;
    E_PaddingEncoding opt_padenc = EPD_withoutPadding;
    OFCmdUnsignedInt opt_filepad = 0;
    OFCmdUnsignedInt opt_itempad = 0;

    SetDebugLevel(( 0 ));

    /* set-up command line parameters and options */
    OFConsoleApplication app(OFFIS_CONSOLE_APPLICATION, OFFIS_CONSOLE_DESCRIPTION, rcsid);
    OFCommandLine cmd;
    cmd.setOptionColumns(LONGCOL, SHORTCOL);
    cmd.setParamColumn(LONGCOL + SHORTCOL + 4);

    cmd.addParam("xmlfile-in",  "XML input filename to be converted (stdin: \"-\")", OFCmdParam::PM_Mandatory);
    cmd.addParam("dcmfile-out", "DICOM output filename", OFCmdParam::PM_Mandatory);

    cmd.addGroup("general options:", LONGCOL, SHORTCOL + 2);
      cmd.addOption("--help",                  "-h",    "print this help text and exit");
      cmd.addOption("--version",                        "print version information and exit", OFTrue /* exclusive */);
      cmd.addOption("--verbose",               "-v",    "verbose mode, print processing details");
      cmd.addOption("--debug",                 "-d",    "debug mode, print debug information");

    cmd.addGroup("input options:");
      cmd.addSubGroup("input file format:");
        cmd.addOption("--read-meta-info",      "+f",    "read meta information if present (default)");
        cmd.addOption("--ignore-meta-info",    "-f",    "ignore file meta information");

    cmd.addGroup("processing options:");
      cmd.addSubGroup("validation:");
        cmd.addOption("--validate-document",   "+Vd",   "validate XML document against DTD");
        cmd.addOption("--check-namespace",     "+Vn",   "check XML namespace in document root");

    cmd.addGroup("output options:");
      cmd.addSubGroup("output file format:");
        cmd.addOption("--write-file",          "+F",    "write file format (default)");
        cmd.addOption("--write-dataset",       "-F",    "write data set without file meta information");
      cmd.addSubGroup("output transfer syntax:");
        cmd.addOption("--write-xfer-same",     "+t=",   "write with same TS as input (default)");
        cmd.addOption("--write-xfer-little",   "+te",   "write with explicit VR little endian TS");
        cmd.addOption("--write-xfer-big",      "+tb",   "write with explicit VR big endian TS");
        cmd.addOption("--write-xfer-implicit", "+ti",   "write with implicit VR little endian TS");
      cmd.addSubGroup("post-1993 value representations:");
        cmd.addOption("--enable-new-vr",       "+u",    "enable support for new VRs (UN/UT) (default)");
        cmd.addOption("--disable-new-vr",      "-u",    "disable support for new VRs, convert to OB");
      cmd.addSubGroup("group length encoding:");
        cmd.addOption("--group-length-recalc", "+g=",   "recalculate group lengths if present (default)");
        cmd.addOption("--group-length-create", "+g",    "always write with group length elements");
        cmd.addOption("--group-length-remove", "-g",    "always write without group length elements");
      cmd.addSubGroup("length encoding in sequences and items:");
        cmd.addOption("--length-explicit",     "+e",    "write with explicit lengths (default)");
        cmd.addOption("--length-undefined",    "-e",    "write with undefined lengths");
      cmd.addSubGroup("data set trailing padding (not with --write-dataset):");
        cmd.addOption("--padding-retain",      "-p=",   "do not change padding\n(default if not --write-dataset)");
        cmd.addOption("--padding-off",         "-p",    "no padding (implicit if --write-dataset)");
        cmd.addOption("--padding-create",      "+p", 2, "[f]ile-pad [i]tem-pad: integer",
                                                        "align file on multiple of f bytes\nand items on multiple of i bytes");

    /* evaluate command line */
    prepareCmdLineArgs(argc, argv, OFFIS_CONSOLE_APPLICATION);
    if (app.parseCommandLine(cmd, argc, argv, OFCommandLine::ExpandWildcards))
    {
        /* check exclusive options first */

        if (cmd.getParamCount() == 0)
        {
          if (cmd.findOption("--version"))
          {
              app.printHeader(OFTrue /*print host identifier*/);          // uses ofConsole.lockCerr()
              CERR << endl << "External libraries used:" << endl;
#ifdef WITH_ZLIB
              CERR << "- ZLIB, Version " << zlibVersion() << endl;
#endif
              CERR << "- LIBXML, Version " << LIBXML_DOTTED_VERSION << endl;
              return 0;
           }
        }

        /* general options */

        if (cmd.findOption("--verbose"))
            opt_verbose = OFTrue;
        if (cmd.findOption("--debug"))
            opt_debug = 5;

        /* input options */

        cmd.beginOptionBlock();
        if (cmd.findOption("--read-meta-info"))
            opt_metaInfo = OFTrue;
        if (cmd.findOption("--ignore-meta-info"))
            opt_metaInfo = OFFalse;
        cmd.endOptionBlock();

        /* processing options */

        if (cmd.findOption("--validate-document"))
            opt_validate = OFTrue;
        if (cmd.findOption("--check-namespace"))
            opt_namespace = OFTrue;

        /* output options */

        cmd.beginOptionBlock();
        if (cmd.findOption("--write-file"))
            opt_dataset = OFFalse;
        if (cmd.findOption("--write-dataset"))
            opt_dataset = OFTrue;
        cmd.endOptionBlock();

        cmd.beginOptionBlock();
        if (cmd.findOption("--write-xfer-same"))
            opt_xfer = EXS_Unknown;
        if (cmd.findOption("--write-xfer-little"))
            opt_xfer = EXS_LittleEndianExplicit;
        if (cmd.findOption("--write-xfer-big"))
            opt_xfer = EXS_BigEndianExplicit;
        if (cmd.findOption("--write-xfer-implicit"))
            opt_xfer = EXS_LittleEndianImplicit;
        cmd.endOptionBlock();

        cmd.beginOptionBlock();
        if (cmd.findOption("--enable-new-vr"))
        {
            dcmEnableUnknownVRGeneration.set(OFTrue);
            dcmEnableUnlimitedTextVRGeneration.set(OFTrue);
        }
        if (cmd.findOption("--disable-new-vr"))
        {
            dcmEnableUnknownVRGeneration.set(OFFalse);
            dcmEnableUnlimitedTextVRGeneration.set(OFFalse);
        }
        cmd.endOptionBlock();

        cmd.beginOptionBlock();
        if (cmd.findOption("--group-length-recalc"))
            opt_glenc = EGL_recalcGL;
        if (cmd.findOption("--group-length-create"))
            opt_glenc = EGL_withGL;
        if (cmd.findOption("--group-length-remove"))
            opt_glenc = EGL_withoutGL;
        cmd.endOptionBlock();

        cmd.beginOptionBlock();
        if (cmd.findOption("--length-explicit"))
            opt_enctype = EET_ExplicitLength;
        if (cmd.findOption("--length-undefined"))
            opt_enctype = EET_UndefinedLength;
        cmd.endOptionBlock();

        cmd.beginOptionBlock();
        if (cmd.findOption("--padding-retain"))
        {
            app.checkConflict("--padding-retain", "--write-dataset", opt_dataset);
            opt_padenc = EPD_noChange;
        }
        if (cmd.findOption("--padding-off"))
            opt_padenc = EPD_withoutPadding;
        if (cmd.findOption("--padding-create"))
        {
            app.checkConflict("--padding-create", "--write-dataset", opt_dataset);
            app.checkValue(cmd.getValueAndCheckMin(opt_filepad, 0));
            app.checkValue(cmd.getValueAndCheckMin(opt_itempad, 0));
            opt_padenc = EPD_withPadding;
        }
        cmd.endOptionBlock();
    }

    SetDebugLevel((opt_debug));

    /* make sure data dictionary is loaded */
    if (!dcmDataDict.isDictionaryLoaded())
    {
        CERR << "Warning: no data dictionary loaded, "
             << "check environment variable: "
             << DCM_DICT_ENVIRONMENT_VARIABLE << endl;
    }
    
    /* check for compatible libxml version */
    LIBXML_TEST_VERSION
    /* initialize the XML library (only required for MT-safety) */
    xmlInitParser();
    /* substitute default entities (XML mnenonics) */
    xmlSubstituteEntitiesDefault(1);
    if (opt_debug)
    {
        /* add line number to debug messages */
        xmlLineNumbersDefault(1);
        xmlGetWarningsDefaultValue = 1;
    } else {
        /* disable libxml warnings and error messages in non-debug mode */
        xmlGetWarningsDefaultValue = 0;
        xmlSetGenericErrorFunc(NULL, noErrorFunction);
    }

    OFCondition result = EC_Normal;
    const char *opt_ifname = NULL;
    const char *opt_ofname = NULL;
    cmd.getParam(1, opt_ifname);
    cmd.getParam(2, opt_ofname);

    /* check filenames */
    if ((opt_ifname == NULL) || (strlen(opt_ifname) == 0))
    {
        CERR << OFFIS_CONSOLE_APPLICATION << ": invalid input filename: <empty string>" << endl;
        result = EC_IllegalParameter;
    }
    if ((opt_ofname == NULL) || (strlen(opt_ofname) == 0))
    {
        CERR << OFFIS_CONSOLE_APPLICATION << ": invalid output filename: <empty string>" << endl;
        result = EC_IllegalParameter;
    }

    if (result.good())
    {
        DcmFileFormat fileformat;
        E_TransferSyntax xfer;
        if (opt_verbose)
            COUT << "reading XML input file: " << opt_ifname << endl;
        /* read XML file and feed data into DICOM fileformat */
        result = readXmlFile(opt_ifname, fileformat, xfer, opt_metaInfo, opt_namespace,
                             opt_validate, opt_verbose, opt_debug != 0);
        if (result.good())
        {
            if (opt_verbose)
                COUT << "writing DICOM output file: " << opt_ofname << endl;
            /* determine transfer syntax to write the file */
            if ((opt_xfer == EXS_Unknown) && (xfer != EXS_Unknown))
                opt_xfer = xfer;
            /* check whether this is possible */
            if (fileformat.canWriteXfer(opt_xfer))
            {
                /* check whether pixel data is compressed */
                if (opt_dataset && DcmXfer(xfer).isEncapsulated())
                {
                    CERR << "Warning: encapsulated pixel data require file format, ignoring --write-dataset" << endl;
                    opt_dataset = OFFalse;
                }
                /* write DICOM file */
                result = fileformat.saveFile(opt_ofname, opt_xfer, opt_enctype, opt_glenc, opt_padenc,
                    OFstatic_cast(Uint32, opt_filepad), OFstatic_cast(Uint32, opt_itempad), opt_dataset);
                if (result.bad())
                    CERR << "Error: " << result.text() << ": writing file: "  << opt_ofname << endl;
            } else {
                CERR << "Error: no conversion to transfer syntax " << DcmXfer(opt_xfer).getXferName()
                     << " possible!" << endl;
                result = EC_CannotChangeRepresentation;
            }
        }
    }

    /* clean up XML library before quitting */
    xmlCleanupParser();

    return result.status();
}

#else /* WITH_LIBXML */

int main(int, char *[])
{
  CERR << rcsid << endl << OFFIS_CONSOLE_DESCRIPTION << endl << endl
       << OFFIS_CONSOLE_APPLICATION " requires the libxml library." << endl
       << "This " OFFIS_CONSOLE_APPLICATION " has been configured and compiled without libxml." << endl
       << "Please reconfigure your system and recompile if appropriate." << endl;
  return 0;
}

#endif /* WITH_LIBXML */


/*
 * CVS/RCS Log:
 * $Log: xml2dcm.cc,v $
 * Revision 1.16  2005/12/16 15:46:41  meichel
 * Declared libxml2 callback functions as extern "C"
 *
 * Revision 1.15  2005/12/09 12:38:51  meichel
 * Added missing include for dcdebug.h
 *
 * Revision 1.14  2005/12/08 15:40:54  meichel
 * Changed include path schema for all DCMTK header files
 *
 * Revision 1.13  2005/11/30 12:51:45  joergr
 * Added missing header file "dcdebug.h".
 *
 * Revision 1.12  2005/03/22 13:55:50  joergr
 * Added call of macro LIBXML_TEST_VERSION.
 *
 * Revision 1.11  2004/11/29 17:04:08  joergr
 * Added support for UTF-8 character set.
 *
 * Revision 1.10  2004/08/03 10:06:18  joergr
 * Added new option that allows to ignore the file meta information.
 *
 * Revision 1.9  2004/03/25 17:27:36  joergr
 * Solved issue with function pointer to std::fprintf or fprintf, respectively.
 *
 * Revision 1.8  2004/03/22 16:55:11  joergr
 * Replaced tabs by spaces.
 *
 * Revision 1.7  2004/02/20 18:06:43  joergr
 * Avoid wrong warning for LUTData (0028,3006) having a VR of US or SS.
 *
 * Revision 1.6  2004/01/16 10:53:53  joergr
 * Adapted type casts to new-style typecast operators defined in ofcast.h.
 *
 * Revision 1.5  2003/08/08 14:46:24  joergr
 * Made libxml output consistent with new xml2dsr command line tool.
 *
 * Revision 1.4  2003/06/17 17:36:04  joergr
 * Distinguish more strictly between OFBool and int (required when HAVE_CXX_BOOL
 * is defined).
 *
 * Revision 1.3  2003/05/20 08:52:56  joergr
 * Minor code corrections.
 *
 * Revision 1.2  2003/04/22 08:25:48  joergr
 * Adapted code to also compile trouble-free without libxml support (report a
 * message that libxml library is required).
 *
 * Revision 1.1  2003/04/17 18:57:08  joergr
 * Added new command line tool that allows to convert an XML document to DICOM
 * file or dataset.
 *
 *
 */
