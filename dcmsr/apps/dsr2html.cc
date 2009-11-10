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
 *  Purpose: Renders the contents of a DICOM structured reporting file in
 *           HTML format
 *
 *  Last Update:      $Author: meichel $
 *  Update Date:      $Date: 2005/12/08 15:47:33 $
 *  CVS/RCS Revision: $Revision: 1.23 $
 *  Status:           $State: Exp $
 *
 *  CVS/RCS Log at end of file
 *
 */


#include "dcmtk/config/osconfig.h"    /* make sure OS specific configuration is included first */

#include "dcmtk/dcmsr/dsrdoc.h"
#include "dcmtk/dcmdata/dcdebug.h"
#include "dcmtk/dcmdata/cmdlnarg.h"
#include "dcmtk/ofstd/ofstream.h"
#include "dcmtk/ofstd/ofconapp.h"
#include "dcmtk/dcmdata/dcuid.h"      /* for dcmtk version name */

#ifdef WITH_ZLIB
#include <zlib.h>       /* for zlibVersion() */
#endif

#define OFFIS_CONSOLE_APPLICATION "dsr2html"

static char rcsid[] = "$dcmtk: " OFFIS_CONSOLE_APPLICATION " v"
  OFFIS_DCMTK_VERSION " " OFFIS_DCMTK_RELEASEDATE " $";


// ********************************************


static OFCondition renderFile(ostream &out,
                              const char *ifname,
                              const char *cssName,
                              const char *defaultCharset,
                              const E_FileReadMode readMode,
                              const E_TransferSyntax xfer,
                              const size_t readFlags,
                              const size_t renderFlags,
                              const OFBool debugMode)
{
    OFCondition result = EC_Normal;

    if ((ifname == NULL) || (strlen(ifname) == 0))
    {
        CERR << OFFIS_CONSOLE_APPLICATION << ": invalid filename: <empty string>" << endl;
        return EC_IllegalParameter;
    }

    DcmFileFormat *dfile = new DcmFileFormat();
    if (dfile != NULL)
    {
        if (readMode == ERM_dataset)
            result = dfile->getDataset()->loadFile(ifname, xfer);
        else
            result = dfile->loadFile(ifname, xfer);
        if (result.bad())
        {
            CERR << OFFIS_CONSOLE_APPLICATION << ": error (" << result.text()
                 << ") reading file: "<< ifname << endl;
        }
    } else
        result = EC_MemoryExhausted;

    if (result.good())
    {
        result = EC_CorruptedData;
        DSRDocument *dsrdoc = new DSRDocument();
        if (dsrdoc != NULL)
        {
            if (debugMode)
                dsrdoc->setLogStream(&ofConsole);
            result = dsrdoc->read(*dfile->getDataset(), readFlags);
            if (result.good())
            {
                // check extended character set
                const char *charset = dsrdoc->getSpecificCharacterSet();
                if ((charset == NULL || strlen(charset) == 0) && dsrdoc->containsExtendedCharacters())
                {
                  // we have an unspecified extended character set
                  if (defaultCharset == NULL)
                  {
                    /* the dataset contains non-ASCII characters that really should not be there */
                    CERR << OFFIS_CONSOLE_APPLICATION << ": error: (0008,0005) Specific Character Set absent but extended characters used in file: "<< ifname << endl;
                    result = EC_IllegalCall;
                  }
                  else
                  {
                    OFString charset(defaultCharset);
                    if (charset == "latin-1") dsrdoc->setSpecificCharacterSetType(DSRTypes::CS_Latin1);
                    else if (charset == "latin-2") dsrdoc->setSpecificCharacterSetType(DSRTypes::CS_Latin2);
                    else if (charset == "latin-3") dsrdoc->setSpecificCharacterSetType(DSRTypes::CS_Latin3);
                    else if (charset == "latin-4") dsrdoc->setSpecificCharacterSetType(DSRTypes::CS_Latin4);
                    else if (charset == "latin-5") dsrdoc->setSpecificCharacterSetType(DSRTypes::CS_Latin5);
                    else if (charset == "cyrillic") dsrdoc->setSpecificCharacterSetType(DSRTypes::CS_Cyrillic);
                    else if (charset == "arabic") dsrdoc->setSpecificCharacterSetType(DSRTypes::CS_Arabic);
                    else if (charset == "greek") dsrdoc->setSpecificCharacterSetType(DSRTypes::CS_Greek);
                    else if (charset == "hebrew") dsrdoc->setSpecificCharacterSetType(DSRTypes::CS_Hebrew);
                  }
                }
                if (result.good()) result = dsrdoc->renderHTML(out, renderFlags, cssName);
            }
            else
            {
                CERR << OFFIS_CONSOLE_APPLICATION << ": error (" << result.text()
                     << ") parsing file: "<< ifname << endl;
            }
        }
        delete dsrdoc;
    }
    delete dfile;

    return result;
}


#define SHORTCOL 3
#define LONGCOL 21


int main(int argc, char *argv[])
{
    int opt_debugMode = 0;
    size_t opt_readFlags = 0;
    size_t opt_renderFlags = DSRTypes::HF_renderDcmtkFootnote;
    const char *opt_cssName = NULL;
    const char *opt_defaultCharset = NULL;
    E_FileReadMode opt_readMode = ERM_autoDetect;
    E_TransferSyntax opt_ixfer = EXS_Unknown;

    SetDebugLevel(( 0 ));

    OFConsoleApplication app(OFFIS_CONSOLE_APPLICATION, "Render DICOM SR file and data set to HTML", rcsid);
    OFCommandLine cmd;
    cmd.setOptionColumns(LONGCOL, SHORTCOL);
    cmd.setParamColumn(LONGCOL + SHORTCOL + 4);

    cmd.addParam("dsrfile-in",   "DICOM SR input filename to be rendered", OFCmdParam::PM_Mandatory);
    cmd.addParam("htmlfile-out", "HTML output filename (default: stdout)", OFCmdParam::PM_Optional);

    cmd.addGroup("general options:", LONGCOL, SHORTCOL + 2);
      cmd.addOption("--help",                  "-h",     "print this help text and exit");
      cmd.addOption("--version",                         "print version information and exit", OFTrue /* exclusive */);
      cmd.addOption("--debug",                 "-d",     "debug mode, print debug information");
      cmd.addOption("--verbose-debug",         "-dd",    "verbose debug mode, print more details");

    cmd.addGroup("input options:");
      cmd.addSubGroup("input file format:");
        cmd.addOption("--read-file",           "+f",     "read file format or data set (default)");
        cmd.addOption("--read-file-only",      "+fo",    "read file format only");
        cmd.addOption("--read-dataset",        "-f",     "read data set without file meta information");
      cmd.addSubGroup("input transfer syntax:");
        cmd.addOption("--read-xfer-auto",      "-t=",    "use TS recognition (default)");
        cmd.addOption("--read-xfer-detect",    "-td",    "ignore TS specified in the file meta header");
        cmd.addOption("--read-xfer-little",    "-te",    "read with explicit VR little endian TS");
        cmd.addOption("--read-xfer-big",       "-tb",    "read with explicit VR big endian TS");
        cmd.addOption("--read-xfer-implicit",  "-ti",    "read with implicit VR little endian TS");

    cmd.addGroup("parsing options:");
      cmd.addSubGroup("additional information:");
        cmd.addOption("--processing-details",  "-Ip",    "show currently processed content item");
      cmd.addSubGroup("error handling:");
        cmd.addOption("--ignore-constraints",  "-Ec",    "ignore relationship content constraints");
        cmd.addOption("--ignore-item-errors",  "-Ee",    "do not abort on content item errors, just warn\n(e.g. missing value type specific attributes)");
        cmd.addOption("--skip-invalid-items",  "-Ei",    "skip invalid content items (incl. sub-tree)");
      cmd.addSubGroup("character set:");
        cmd.addOption("--charset-require",     "+Cr",    "require declaration of ext. charset (default)");
        cmd.addOption("--charset-assume",      "+Ca", 1, "charset: string constant (latin-1 to -5,",
                                                         "greek, cyrillic, arabic, hebrew)\n"
                                                         "assume charset if undeclared ext. charset found");
    cmd.addGroup("output options:");
      cmd.addSubGroup("HTML compatibility:");
        cmd.addOption("--html-3.2",            "+H3",    "use only HTML version 3.2 compatible features");
        cmd.addOption("--html-4.0",            "+H4",    "allow all HTML version 4.0 features (default)");
        cmd.addOption("--add-document-type",   "+Hd",    "add reference to SGML document type definition");
      cmd.addSubGroup("cascading style sheet (CSS), only with HTML 4.0:");
        cmd.addOption("--css-reference",       "+Sr", 1, "URL : string",
                                                         "add reference to specified CSS to HTML page");
        cmd.addOption("--css-file",            "+Sf", 1, "filename : string",
                                                         "embed content of specified CSS into HTML page");
      cmd.addSubGroup("general rendering:");
        cmd.addOption("--expand-inline",       "+Ri",    "expand short content items inline (default)");
        cmd.addOption("--never-expand-inline", "-Ri",    "never expand content items inline");
        cmd.addOption("--render-full-data",    "+Rd",    "render full data of content items");
      cmd.addSubGroup("document rendering:");
        cmd.addOption("--document-type-title", "+Dt",    "use document type as document title (default)");
        cmd.addOption("--patient-info-title",  "+Dp",    "use patient information as document title");
        cmd.addOption("--no-document-header",  "-Dh",    "do not render general document information");
      cmd.addSubGroup("code rendering:");
        cmd.addOption("--render-inline-codes", "+Ci",    "render codes in continuous text blocks");
        cmd.addOption("--concept-name-codes",  "+Cn",    "render code of concept names");
        cmd.addOption("--numeric-unit-codes",  "+Cu",    "render code of numeric measurement units");
        cmd.addOption("--code-value-unit",     "+Cv",    "use code value as measurement unit (default)");
        cmd.addOption("--code-meaning-unit",   "+Cm",    "use code meaning as measurement unit");
        cmd.addOption("--render-all-codes",    "+Ca",    "render all codes (implies +Ci, +Cn and +Cu)");

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
              CERR << endl << "External libraries used:";
#ifdef WITH_ZLIB
              CERR << endl << "- ZLIB, Version " << zlibVersion() << endl;
#else
              CERR << " none" << endl;
#endif
              return 0;
           }
        }

        /* general options */
        if (cmd.findOption("--debug"))
            opt_debugMode = 2;
        if (cmd.findOption("--verbose-debug"))
        {
            opt_debugMode = 5;
            opt_readFlags |= DSRTypes::RF_verboseDebugMode;
        }

        /* input options */
        cmd.beginOptionBlock();
        if (cmd.findOption("--read-file")) opt_readMode = ERM_autoDetect;
        if (cmd.findOption("--read-file-only")) opt_readMode = ERM_fileOnly;
        if (cmd.findOption("--read-dataset")) opt_readMode = ERM_dataset;
        cmd.endOptionBlock();

        cmd.beginOptionBlock();
        if (cmd.findOption("--read-xfer-auto"))
            opt_ixfer = EXS_Unknown;
        if (cmd.findOption("--read-xfer-detect"))
            dcmAutoDetectDatasetXfer.set(OFTrue);
        if (cmd.findOption("--read-xfer-little"))
        {
            app.checkDependence("--read-xfer-little", "--read-dataset", opt_readMode == ERM_dataset);
            opt_ixfer = EXS_LittleEndianExplicit;
        }
        if (cmd.findOption("--read-xfer-big"))
        {
            app.checkDependence("--read-xfer-big", "--read-dataset", opt_readMode == ERM_dataset);
            opt_ixfer = EXS_BigEndianExplicit;
        }
        if (cmd.findOption("--read-xfer-implicit"))
        {
            app.checkDependence("--read-xfer-implicit", "--read-dataset", opt_readMode == ERM_dataset);
            opt_ixfer = EXS_LittleEndianImplicit;
        }
        cmd.endOptionBlock();

        if (cmd.findOption("--processing-details"))
            opt_readFlags |= DSRTypes::RF_showCurrentlyProcessedItem;
        if (cmd.findOption("--ignore-constraints"))
            opt_readFlags |= DSRTypes::RF_ignoreRelationshipConstraints;
        if (cmd.findOption("--ignore-item-errors"))
            opt_readFlags |= DSRTypes::RF_ignoreContentItemErrors;
        if (cmd.findOption("--skip-invalid-items"))
            opt_readFlags |= DSRTypes::RF_skipInvalidContentItems;

        /* charset options */
        cmd.beginOptionBlock();
        if (cmd.findOption("--charset-require"))
        {
           opt_defaultCharset = NULL;
        }
        if (cmd.findOption("--charset-assume"))
        {
          app.checkValue(cmd.getValue(opt_defaultCharset));
          OFString charset(opt_defaultCharset);
          if (charset != "latin-1" && charset != "latin-2" && charset != "latin-3" &&
              charset != "latin-4" && charset != "latin-5" && charset != "cyrillic" &&
              charset != "arabic" && charset != "greek" && charset != "hebrew")
          {
            app.printError("unknown value for --charset-assume. known values are latin-1 to -5, cyrillic, arabic, greek, hebrew.");
          }
        }
        cmd.endOptionBlock();

        /* HTML compatibility */
        cmd.beginOptionBlock();
        if (cmd.findOption("--html-3.2"))
            opt_renderFlags |= DSRTypes::HF_version32Compatibility;
        if (cmd.findOption("--html-4.0"))
        {
            /* default */
        }
        cmd.endOptionBlock();

        if (cmd.findOption("--add-document-type"))
            opt_renderFlags |= DSRTypes::HF_addDocumentTypeReference;

        /* cascading style sheet */
        cmd.beginOptionBlock();
        if (cmd.findOption("--css-reference"))
        {
          	app.checkDependence("--css-reference", "--html-4.0", !(opt_renderFlags & DSRTypes::HF_version32Compatibility));
            opt_renderFlags &= ~DSRTypes::HF_copyStyleSheetContent;
            app.checkValue(cmd.getValue(opt_cssName));
        }
        if (cmd.findOption("--css-file"))
        {
          	app.checkDependence("--css-file", "--html-4.0", !(opt_renderFlags & DSRTypes::HF_version32Compatibility));
            opt_renderFlags |= DSRTypes::HF_copyStyleSheetContent;
            app.checkValue(cmd.getValue(opt_cssName));
        }
        cmd.endOptionBlock();

        /* general rendering */
        cmd.beginOptionBlock();
        if (cmd.findOption("--expand-inline"))
        {
            /* default */
        }
        if (cmd.findOption("--never-expand-inline"))
            opt_renderFlags |= DSRTypes::HF_neverExpandChildrenInline;
        cmd.endOptionBlock();

        if (cmd.findOption("--render-full-data"))
            opt_renderFlags |= DSRTypes::HF_renderFullData;

        /* document rendering */
        cmd.beginOptionBlock();
        if (cmd.findOption("--document-type-title"))
        {
            /* default */
        }
        if (cmd.findOption("--patient-info-title"))
            opt_renderFlags |= DSRTypes::HF_renderPatientTitle;
        cmd.endOptionBlock();

        if (cmd.findOption("--no-document-header"))
            opt_renderFlags |= DSRTypes::HF_renderNoDocumentHeader;

        /* code rendering */
        if (cmd.findOption("--render-inline-codes"))
            opt_renderFlags |= DSRTypes::HF_renderInlineCodes;
        if (cmd.findOption("--concept-name-codes"))
            opt_renderFlags |= DSRTypes::HF_renderConceptNameCodes;
        if (cmd.findOption("--numeric-unit-codes"))
            opt_renderFlags |= DSRTypes::HF_renderNumericUnitCodes;
        if (cmd.findOption("--code-value-unit"))
            opt_renderFlags &= ~DSRTypes::HF_useCodeMeaningAsUnit;
        if (cmd.findOption("--code-meaning-unit"))
            opt_renderFlags |= DSRTypes::HF_useCodeMeaningAsUnit;
        if (cmd.findOption("--render-all-codes"))
            opt_renderFlags |= DSRTypes::HF_renderAllCodes;
    }

    SetDebugLevel((opt_debugMode));

    /* make sure data dictionary is loaded */
    if (!dcmDataDict.isDictionaryLoaded())
    {
        CERR << "Warning: no data dictionary loaded, "
             << "check environment variable: "
             << DCM_DICT_ENVIRONMENT_VARIABLE << endl;
    }

    int result = 0;
    const char *ifname = NULL;
    cmd.getParam(1, ifname);
    if (cmd.getParamCount() == 2)
    {
        const char *ofname = NULL;
        cmd.getParam(2, ofname);
        ofstream stream(ofname);
        if (stream.good())
        {
            if (renderFile(stream, ifname, opt_cssName, opt_defaultCharset, opt_readMode, opt_ixfer, opt_readFlags, opt_renderFlags, opt_debugMode != 0).bad())
                result = 2;
        } else
            result = 1;
    } else {
        if (renderFile(COUT, ifname, opt_cssName, opt_defaultCharset, opt_readMode, opt_ixfer, opt_readFlags, opt_renderFlags, opt_debugMode != 0).bad())
            result = 3;
    }

    return result;
}


/*
 * CVS/RCS Log:
 * $Log: dsr2html.cc,v $
 * Revision 1.23  2005/12/08 15:47:33  meichel
 * Changed include path schema for all DCMTK header files
 *
 * Revision 1.22  2005/12/02 10:37:30  joergr
 * Added new command line option that ignores the transfer syntax specified in
 * the meta header and tries to detect the transfer syntax automatically from
 * the dataset.
 * Added new command line option that checks whether a given file starts with a
 * valid DICOM meta header.
 *
 * Revision 1.21  2004/11/22 17:20:16  meichel
 * Now checking whether extended characters are present in a DICOM SR document,
 *   preventing generation of incorrect HTML if undeclared extended charset used.
 *
 * Revision 1.20  2004/01/05 14:34:59  joergr
 * Removed acknowledgements with e-mail addresses from CVS log.
 *
 * Revision 1.19  2003/10/06 09:56:10  joergr
 * Added new flag which allows to ignore content item errors when reading an SR
 * document (e.g. missing value type specific attributes).
 *
 * Revision 1.18  2002/11/26 08:45:34  meichel
 * Replaced all includes for "zlib.h" with <zlib.h>
 *   to avoid inclusion of zlib.h in the makefile dependencies.
 *
 * Revision 1.17  2002/09/23 18:16:42  joergr
 * Added new command line option "--version" which prints the name and version
 * number of external libraries used (incl. preparation for future support of
 * 'config.guess' host identifiers).
 *
 * Revision 1.16  2002/08/02 12:37:16  joergr
 * Enhanced debug output of dcmsr command line tools (e.g. add position string
 * of invalid content items to error messages).
 *
 * Revision 1.15  2002/05/07 12:47:58  joergr
 * Fixed bug in an error message.
 *
 * Revision 1.14  2002/04/16 13:49:52  joergr
 * Added configurable support for C++ ANSI standard includes (e.g. streams).
 *
 * Revision 1.13  2002/04/11 13:05:02  joergr
 * Use the new loadFile() and saveFile() routines from the dcmdata library.
 *
 * Revision 1.12  2001/10/10 15:26:31  joergr
 * Additonal adjustments for new OFCondition class.
 *
 * Revision 1.11  2001/10/02 11:55:59  joergr
 * Adapted module "dcmsr" to the new class OFCondition. Introduced module
 * specific error codes.
 *
 * Revision 1.10  2001/09/26 13:04:01  meichel
 * Adapted dcmsr to class OFCondition
 *
 * Revision 1.9  2001/06/20 15:06:38  joergr
 * Added new debugging features (additional flags) to examine "corrupted" SR
 * documents.
 *
 * Revision 1.8  2001/06/01 15:50:57  meichel
 * Updated copyright header
 *
 * Revision 1.7  2001/04/03 08:22:54  joergr
 * Added new command line option: ignore relationship content constraints
 * specified for each SR document class.
 *
 * Revision 1.6  2000/12/08 16:06:19  joergr
 * Replaced empty code lines (";") by empty command blocks ("{}") to avoid
 * compiler warnings reported by MSVC6.
 *
 * Revision 1.5  2000/11/09 20:31:08  joergr
 * Added new command line options (document type and HTML version).
 *
 * Revision 1.4  2000/11/07 18:09:48  joergr
 * Added new command line option allowing to choose code value or meaning to be
 * rendered as the numeric measurement unit.
 *
 * Revision 1.3  2000/11/01 16:08:04  joergr
 * Added support for Cascading Style Sheet (CSS) used optionally for HTML
 * rendering. Optimized HTML rendering.
 *
 * Revision 1.2  2000/10/26 14:15:33  joergr
 * Added new flag specifying whether to add a "dcmtk" footnote to the rendered
 * HTML document or not.
 *
 * Revision 1.1  2000/10/13 07:46:21  joergr
 * Added new module 'dcmsr' providing access to DICOM structured reporting
 * documents (supplement 23).  Doc++ documentation not yet completed.
 *
 *
 */
