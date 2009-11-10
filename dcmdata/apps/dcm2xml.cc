/*
 *
 *  Copyright (C) 2002-2005, OFFIS
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
 *  Purpose: Convert the contents of a DICOM file to XML format
 *
 *  Last Update:      $Author: meichel $
 *  Update Date:      $Date: 2005/12/08 15:40:42 $
 *  CVS/RCS Revision: $Revision: 1.22 $
 *  Status:           $State: Exp $
 *
 *  CVS/RCS Log at end of file
 *
 */


#include "dcmtk/config/osconfig.h"    /* make sure OS specific configuration is included first */

#include "dcmtk/dcmdata/dctk.h"
#include "dcmtk/dcmdata/cmdlnarg.h"
#include "dcmtk/ofstd/ofstd.h"
#include "dcmtk/ofstd/ofstream.h"
#include "dcmtk/ofstd/ofconapp.h"
#include "dcmtk/dcmdata/dcdebug.h"

#ifdef WITH_ZLIB
#include <zlib.h>        /* for zlibVersion() */
#endif

#define OFFIS_CONSOLE_APPLICATION "dcm2xml"
#define OFFIS_CONSOLE_DESCRIPTION "Convert DICOM file and data set to XML"

#define DOCUMENT_TYPE_DEFINITION_FILE "dcm2xml.dtd"

static char rcsid[] = "$dcmtk: " OFFIS_CONSOLE_APPLICATION " v"
  OFFIS_DCMTK_VERSION " " OFFIS_DCMTK_RELEASEDATE " $";

// ********************************************

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

static OFBool checkForNonASCIICharacters(DcmItem& dataset)
{
  DcmStack stack;
  while (dataset.nextObject(stack, OFTrue).good())
  {
    if (stack.top()->isaString())
    {
      if (checkForNonASCIICharacters(* OFstatic_cast(DcmElement *, stack.top())))
        return OFTrue;
    }
  }
  return OFFalse;
}

static OFCondition writeFile(ostream &out,
                             const char *ifname,
                             const E_FileReadMode readMode,
                             const E_TransferSyntax xfer,
                             const OFBool loadIntoMemory,
                             const Uint32 maxReadLength,
                             const char *defaultCharset,
                             const size_t writeFlags)
{
    OFCondition result = EC_Normal;

    if ((ifname == NULL) || (strlen(ifname) == 0))
    {
        CERR << OFFIS_CONSOLE_APPLICATION << ": invalid filename: <empty string>" << endl;
        return EC_IllegalParameter;
    }

    /* read DICOM file or data set */
    DcmFileFormat dfile;
    result = dfile.loadFile(ifname, xfer, EGL_noChange, maxReadLength, readMode);

    if (result.bad())
    {
        CERR << OFFIS_CONSOLE_APPLICATION << ": error (" << result.text()
             << ") reading file: "<< ifname << endl;
    } else {
        /* write content to XML format */
        if (loadIntoMemory)
            dfile.getDataset()->loadAllDataIntoMemory();
        /* determine dataset character encoding */
        OFString encString;
        OFString csetString;
        if (dfile.getDataset()->findAndGetOFString(DCM_SpecificCharacterSet, csetString).good())
        {
            if (csetString == "ISO_IR 6")
                encString = "UTF-8";
            else if (csetString == "ISO_IR 192")
                encString = "UTF-8";
            else if (csetString == "ISO_IR 100")
                encString = "ISO-8859-1";
            else if (csetString == "ISO_IR 101")
                encString = "ISO-8859-2";
            else if (csetString == "ISO_IR 109")
                encString = "ISO-8859-3";
            else if (csetString == "ISO_IR 110")
                encString = "ISO-8859-4";
            else if (csetString == "ISO_IR 148")
                encString = "ISO-8859-9";
            else if (csetString == "ISO_IR 144")
                encString = "ISO-8859-5";
            else if (csetString == "ISO_IR 127")
                encString = "ISO-8859-6";
            else if (csetString == "ISO_IR 126")
                encString = "ISO-8859-7";
            else if (csetString == "ISO_IR 138")
                encString = "ISO-8859-8";
            else if (!csetString.empty())
                CERR << "Warning: (0008,0005) Specific Character Set '" << csetString << "' not supported" << endl;
        } else {
          /* SpecificCharacterSet is not present in the dataset */
          if (checkForNonASCIICharacters(*dfile.getDataset()))
          {
            if (defaultCharset == NULL)
            {
              /* the dataset contains non-ASCII characters that really should not be there */
              CERR << OFFIS_CONSOLE_APPLICATION << ": error: (0008,0005) Specific Character Set absent "
                   << "but extended characters used in file: " << ifname << endl;
              return EC_IllegalCall;
            } else {
              OFString charset(defaultCharset);
              if (charset == "latin-1")
              {
                csetString = "ISO_IR 100";
                encString = "ISO-8859-1";
              }
              else if (charset == "latin-2")
              {
                csetString = "ISO_IR 101";
                encString = "ISO-8859-2";
              }
              else if (charset == "latin-3")
              {
                csetString = "ISO_IR 109";
                encString = "ISO-8859-3";
              }
              else if (charset == "latin-4")
              {
                csetString = "ISO_IR 110";
                encString = "ISO-8859-4";
              }
              else if (charset == "latin-5")
              {
                csetString = "ISO_IR 148";
                encString = "ISO-8859-9";
              }
              else if (charset == "cyrillic")
              {
                csetString = "ISO_IR 144";
                encString = "ISO-8859-5";
              }
              else if (charset == "arabic")
              {
                csetString = "ISO_IR 127";
                encString = "ISO-8859-6";
              }
              else if (charset == "greek")
              {
                csetString = "ISO_IR 126";
                encString = "ISO-8859-7";
              }
              else if (charset == "hebrew")
              {
                csetString = "ISO_IR 138";
                encString = "ISO-8859-8";
              }
              dfile.getDataset()->putAndInsertString(DCM_SpecificCharacterSet, csetString.c_str());
            }
          }
        }

        /* write XML document header */
        out << "<?xml version=\"1.0\"";
        /* optional character set */
        if (encString.length() > 0)
            out << " encoding=\"" << encString << "\"";
        out << "?>" << endl;
        /* add document type definition (DTD) */
        if (writeFlags & DCMTypes::XF_addDocumentType)
        {
            out << "<!DOCTYPE ";
            if (readMode == ERM_dataset)
               out << "data-set";
            else
               out << "file-format";
            /* embed DTD */
            if (writeFlags & DCMTypes::XF_embedDocumentType)
            {
                out << " [" << endl;
                /* copy content from DTD file */
#ifdef HAVE_IOS_NOCREATE
                ifstream dtdFile(DOCUMENT_TYPE_DEFINITION_FILE, ios::in|ios::nocreate);
#else
                ifstream dtdFile(DOCUMENT_TYPE_DEFINITION_FILE, ios::in);
#endif
                if (dtdFile)
                {
                    char c;
                    /* copy all characters */
                    while (dtdFile.get(c))
                        out << c;
                }
                out << "]";
            } else { /* reference DTD */
                out << " SYSTEM \"" << DOCUMENT_TYPE_DEFINITION_FILE << "\"";
            }
            out << ">" << endl;
        }
        /* write XML document content */
        if (readMode == ERM_dataset)
            result = dfile.getDataset()->writeXML(out, writeFlags);
        else
            result = dfile.writeXML(out, writeFlags);
    }
    return result;
}


#define SHORTCOL 3
#define LONGCOL 20


int main(int argc, char *argv[])
{
    int opt_debugMode = 0;
    size_t opt_writeFlags = 0;
    OFBool loadIntoMemory = OFFalse;
    const char *opt_defaultCharset = NULL;
    E_FileReadMode opt_readMode = ERM_autoDetect;
    E_TransferSyntax opt_ixfer = EXS_Unknown;
    OFCmdUnsignedInt maxReadLength = 4096; // default is 4 KB

    SetDebugLevel(( 0 ));

    OFConsoleApplication app(OFFIS_CONSOLE_APPLICATION, OFFIS_CONSOLE_DESCRIPTION, rcsid);
    OFCommandLine cmd;
    cmd.setOptionColumns(LONGCOL, SHORTCOL);
    cmd.setParamColumn(LONGCOL + SHORTCOL + 4);

    cmd.addParam("dcmfile-in",   "DICOM input filename to be converted", OFCmdParam::PM_Mandatory);
    cmd.addParam("xmlfile-out",  "XML output filename (default: stdout)", OFCmdParam::PM_Optional);

    cmd.addGroup("general options:", LONGCOL, SHORTCOL + 2);
      cmd.addOption("--help",                  "-h",     "print this help text and exit");
      cmd.addOption("--version",                         "print version information and exit", OFTrue /* exclusive */);
      cmd.addOption("--debug",                 "-d",     "debug mode, print debug information");

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
      cmd.addSubGroup("long tag values:");
        cmd.addOption("--load-all",            "+M",     "load very long tag values (e.g. pixel data)");
        cmd.addOption("--load-short",          "-M",     "do not load very long values (default)");
        cmd.addOption("--max-read-length",     "+R",  1, "[k]bytes: integer [4..4194302] (default: 4)",
                                                         "set threshold for long values to k kbytes");
    cmd.addGroup("processing options:");
      cmd.addSubGroup("character set:");
        cmd.addOption("--charset-require",     "+Cr",    "require declaration of extended charset (default)");
        cmd.addOption("--charset-assume",      "+Ca", 1, "charset: string constant",
                                                         "(latin-1 to -5, cyrillic, arabic, greek, hebrew)\n"
                                                         "assume charset if undeclared ext. charset found");
    cmd.addGroup("output options:");
      cmd.addSubGroup("XML structure:");
        cmd.addOption("--add-dtd-reference",   "+Xd",    "add reference to document type definition (DTD)");
        cmd.addOption("--embed-dtd-content",   "+Xe",    "embed document type definition into XML document");
        cmd.addOption("--use-xml-namespace",   "+Xn",    "add XML namespace declaration to root element");
      cmd.addSubGroup("DICOM elements:");
        cmd.addOption("--write-binary-data",   "+Wb",    "write binary data of OB and OW elements\n(default: off, be careful with --load-all)");
        cmd.addOption("--encode-base64",       "+Eb",    "encode binary data as Base64 (RFC 2045, MIME)");

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

        /* options */

        if (cmd.findOption("--debug"))
            opt_debugMode = 5;

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

        if (cmd.findOption("--max-read-length"))
        {
            app.checkValue(cmd.getValueAndCheckMinMax(maxReadLength, 4, 4194302));
            maxReadLength *= 1024; // convert kbytes to bytes
        }
        cmd.beginOptionBlock();
        if (cmd.findOption("--load-all"))
            loadIntoMemory = OFTrue;
        if (cmd.findOption("--load-short"))
            loadIntoMemory = OFFalse;
        cmd.endOptionBlock();

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

        cmd.beginOptionBlock();
        if (cmd.findOption("--add-dtd-reference"))
            opt_writeFlags |= DCMTypes::XF_addDocumentType;
        if (cmd.findOption("--embed-dtd-content"))
            opt_writeFlags |= DCMTypes::XF_addDocumentType | DCMTypes::XF_embedDocumentType;
        cmd.endOptionBlock();

        if (cmd.findOption("--use-xml-namespace"))
            opt_writeFlags |= DCMTypes::XF_useDcmtkNamespace;

        if (cmd.findOption("--write-binary-data"))
            opt_writeFlags |= DCMTypes::XF_writeBinaryData;
        if (cmd.findOption("--encode-base64"))
        {
            app.checkDependence("--encode-base64", "--write-binary-data", (opt_writeFlags & DCMTypes::XF_writeBinaryData) > 0);
            opt_writeFlags |= DCMTypes::XF_encodeBase64;
        }
    }

    SetDebugLevel((opt_debugMode));

    /* make sure data dictionary is loaded */
    if (!dcmDataDict.isDictionaryLoaded())
    {
        CERR << "Warning: no data dictionary loaded, "
             << "check environment variable: "
             << DCM_DICT_ENVIRONMENT_VARIABLE << endl;
    }

    /* make sure document type definition file exists */
    if ((opt_writeFlags & DCMTypes::XF_embedDocumentType) &&
        !OFStandard::fileExists(DOCUMENT_TYPE_DEFINITION_FILE))
    {
        CERR << "Warning: DTD file \"" << DOCUMENT_TYPE_DEFINITION_FILE
             << "\" does not exist ... adding reference instead" << endl;
        opt_writeFlags &= ~DCMTypes::XF_embedDocumentType;
    }

    int result = 0;
    /* first parameter is treated as the input filename */
    const char *ifname = NULL;
    cmd.getParam(1, ifname);
    /* if second parameter is present, it is treated as the output filename ("stdout" otherwise) */
    if (cmd.getParamCount() == 2)
    {
        const char *ofname = NULL;
        cmd.getParam(2, ofname);
        ofstream stream(ofname);
        if (stream.good())
        {
            if (writeFile(stream, ifname, opt_readMode, opt_ixfer, loadIntoMemory, maxReadLength, opt_defaultCharset, opt_writeFlags).bad())
                result = 2;
        } else
            result = 1;
    } else {
        if (writeFile(COUT, ifname, opt_readMode, opt_ixfer, loadIntoMemory, maxReadLength, opt_defaultCharset, opt_writeFlags).bad())
            result = 3;
    }

    return result;
}


/*
 * CVS/RCS Log:
 * $Log: dcm2xml.cc,v $
 * Revision 1.22  2005/12/08 15:40:42  meichel
 * Changed include path schema for all DCMTK header files
 *
 * Revision 1.21  2005/12/07 16:42:49  onken
 * Changed default and minimum value of --max-read-length to 4 KB
 *
 * Revision 1.20  2005/12/02 08:58:44  joergr
 * Added new command line option that ignores the transfer syntax specified in
 * the meta header and tries to detect the transfer syntax automatically from
 * the dataset.
 * Added new command line option that checks whether a given file starts with a
 * valid DICOM meta header.
 * Removed superfluous local variable. Changed type of variable "maxReadLength".
 * Made description of option --max-read-length more consistent with the other
 * command line tools.
 *
 * Revision 1.19  2005/12/01 11:25:44  joergr
 * Removed superfluous local variable. Changed type of variable "maxReadLength".
 *
 * Revision 1.18  2005/11/28 15:28:54  meichel
 * File dcdebug.h is not included by any other header file in the toolkit
 *   anymore, to minimize the risk of name clashes of macro debug().
 *
 * Revision 1.17  2005/11/17 11:26:11  onken
 * Option --max-read-length now uses OFCommandLine to check, whether option
 * value is in range
 *
 * Revision 1.16  2005/11/16 14:59:11  onken
 * *** empty log message ***
 *
 * Revision 1.15  2005/11/16 14:55:56  onken
 * Added "--max-read-length" option to dcmdump and dcm2xml to override
 * DCMTK-internal threshold (4096K) for long attribute values.
 *
 * Revision 1.14  2005/06/24 10:06:46  joergr
 * Check dependence between command line options --write-binary-data and
 * --encode-base64.
 *
 * Revision 1.13  2004/11/29 17:02:17  joergr
 * Added warning message when character set is unknown, unsupported  or cannot
 * be mapped to the output format. Added support for UTF-8 character set.
 *
 * Revision 1.12  2004/11/22 16:30:19  meichel
 * Now checking whether extended characters are present in a DICOM dataset,
 *   preventing generation of incorrect XML if undeclared extended charset used.
 *
 * Revision 1.11  2003/04/22 08:23:33  joergr
 * Added new command line option which allows to embed the content of the DTD
 * instead of referencing the DTD file.
 *
 * Revision 1.10  2003/04/01 14:56:14  joergr
 * Added support for XML namespaces.
 *
 * Revision 1.9  2002/11/26 08:42:58  meichel
 * Replaced all includes for "zlib.h" with <zlib.h>
 *   to avoid inclusion of zlib.h in the makefile dependencies.
 *
 * Revision 1.8  2002/09/23 17:52:01  joergr
 * Prepared code for future support of 'config.guess' host identifiers.
 *
 * Revision 1.7  2002/09/23 13:50:39  joergr
 * Added new command line option "--version" which prints the name and version
 * number of external libraries used.
 *
 * Revision 1.6  2002/08/21 10:14:13  meichel
 * Adapted code to new loadFile and saveFile methods, thus removing direct
 *   use of the DICOM stream classes.
 *
 * Revision 1.5  2002/06/10 17:35:47  joergr
 * Fixed inconsistency regarding spelling of the "file-format" element.
 *
 * Revision 1.4  2002/05/14 08:19:22  joergr
 * Added support for Base64 (MIME) encoded binary data.
 *
 * Revision 1.3  2002/05/07 12:47:41  joergr
 * Fixed bug in an error message.
 *
 * Revision 1.2  2002/04/25 14:56:35  joergr
 * Removed unused function parameter to keep Sun CC 2.0.1 quiet.
 *
 * Revision 1.1  2002/04/25 10:08:35  joergr
 * Added new command line tool to convert DICOM files to XML.
 *
 *
 */
