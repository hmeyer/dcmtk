/*
 *
 *  Copyright (C) 2001-2005, OFFIS
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
 *  Module:  dcmjpeg
 *
 *  Author:  Marco Eichelberg
 *
 *  Purpose: Decompress DICOM file
 *
 *  Last Update:      $Author: meichel $
 *  Update Date:      $Date: 2005/12/08 15:43:21 $
 *  CVS/RCS Revision: $Revision: 1.13 $
 *  Status:           $State: Exp $
 *
 *  CVS/RCS Log at end of file
 *
 */

#include "dcmtk/config/osconfig.h"    /* make sure OS specific configuration is included first */

#define INCLUDE_CSTDLIB
#define INCLUDE_CSTDIO
#define INCLUDE_CSTRING
#include "dcmtk/ofstd/ofstdinc.h"

#ifdef HAVE_GUSI_H
#include <GUSI.h>
#endif

#include "dcmtk/dcmdata/dctk.h"
#include "dcmtk/dcmdata/dcdebug.h"
#include "dcmtk/dcmdata/cmdlnarg.h"
#include "dcmtk/ofstd/ofconapp.h"
#include "dcmtk/dcmdata/dcuid.h"       /* for dcmtk version name */
#include "dcmtk/dcmjpeg/djdecode.h"    /* for dcmjpeg decoders */
#include "dcmtk/dcmjpeg/dipijpeg.h"    /* for dcmimage JPEG plugin */

#ifdef WITH_ZLIB
#include <zlib.h>      /* for zlibVersion() */
#endif

#define OFFIS_CONSOLE_APPLICATION "dcmdjpeg"

static char rcsid[] = "$dcmtk: " OFFIS_CONSOLE_APPLICATION " v"
  OFFIS_DCMTK_VERSION " " OFFIS_DCMTK_RELEASEDATE " $";

// ********************************************


#define SHORTCOL 4
#define LONGCOL 21

int main(int argc, char *argv[])
{

#ifdef HAVE_GUSI_H
  GUSISetup(GUSIwithSIOUXSockets);
  GUSISetup(GUSIwithInternetSockets);
#endif

  SetDebugLevel(( 0 ));

  const char *opt_ifname = NULL;
  const char *opt_ofname = NULL;

  int opt_debugMode = 0;
  OFBool opt_verbose = OFFalse;
  OFBool opt_oDataset = OFFalse;
  E_FileReadMode opt_readMode = ERM_autoDetect;
  E_TransferSyntax opt_oxfer = EXS_LittleEndianExplicit;
  E_GrpLenEncoding opt_oglenc = EGL_recalcGL;
  E_EncodingType opt_oenctype = EET_ExplicitLength;
  E_PaddingEncoding opt_opadenc = EPD_noChange;
  OFCmdUnsignedInt opt_filepad = 0;
  OFCmdUnsignedInt opt_itempad = 0;
  E_TransferSyntax opt_ixfer = EXS_Unknown;

  // JPEG parameters
  E_DecompressionColorSpaceConversion opt_decompCSconversion = EDC_photometricInterpretation;
  E_UIDCreation opt_uidcreation = EUC_default;
  E_PlanarConfiguration opt_planarconfig = EPC_default;

  OFConsoleApplication app(OFFIS_CONSOLE_APPLICATION , "Decode JPEG-compressed DICOM file", rcsid);
  OFCommandLine cmd;
  cmd.setOptionColumns(LONGCOL, SHORTCOL);
  cmd.setParamColumn(LONGCOL + SHORTCOL + 4);

  cmd.addParam("dcmfile-in",  "DICOM input filename to be converted");
  cmd.addParam("dcmfile-out", "DICOM output filename");

  cmd.addGroup("general options:", LONGCOL, SHORTCOL + 2);
   cmd.addOption("--help",                      "-h",        "print this help text and exit");
   cmd.addOption("--version",                                "print version information and exit", OFTrue /* exclusive */);
   cmd.addOption("--verbose",                   "-v",        "verbose mode, print processing details");
   cmd.addOption("--debug",                     "-d",        "debug mode, print debug information");

   cmd.addGroup("input options:");
    cmd.addSubGroup("input file format:");
     cmd.addOption("--read-file",              "+f",        "read file format or data set (default)");
     cmd.addOption("--read-file-only",         "+fo",       "read file format only");
     cmd.addOption("--read-dataset",           "-f",        "read data set without file meta information");

  cmd.addGroup("processing options:");
    cmd.addSubGroup("color space conversion options:");
      cmd.addOption("--conv-photometric",       "+cp",       "convert if YCbCr photom. interpr. (default)");
      cmd.addOption("--conv-lossy",             "+cl",       "convert YCbCr to RGB if lossy JPEG");
      cmd.addOption("--conv-always",            "+ca",       "always convert YCbCr to RGB");
      cmd.addOption("--conv-never",             "+cn",       "never convert color space");

    cmd.addSubGroup("planar configuration options:");
      cmd.addOption("--planar-auto",            "+pa",       "automatically determine planar configuration\nfrom SOP class and color space (default)");
      cmd.addOption("--color-by-pixel",         "+px",       "always store color-by-pixel");
      cmd.addOption("--color-by-plane",         "+pl",       "always store color-by-plane");

    cmd.addSubGroup("SOP Instance UID options:");
     cmd.addOption("--uid-default",        "+ud",     "keep same SOP Instance UID (default)");
     cmd.addOption("--uid-always",         "+ua",     "always assign new UID");

  cmd.addGroup("output options:");
    cmd.addSubGroup("output file format:");
      cmd.addOption("--write-file",             "+F",        "write file format (default)");
      cmd.addOption("--write-dataset",          "-F",        "write data set without file meta information");
    cmd.addSubGroup("output transfer syntax:");
      cmd.addOption("--write-xfer-little",      "+te",       "write with explicit VR little endian (default)");
      cmd.addOption("--write-xfer-big",         "+tb",       "write with explicit VR big endian TS");
      cmd.addOption("--write-xfer-implicit",    "+ti",       "write with implicit VR little endian TS");
    cmd.addSubGroup("post-1993 value representations:");
      cmd.addOption("--enable-new-vr",          "+u",        "enable support for new VRs (UN/UT) (default)");
      cmd.addOption("--disable-new-vr",         "-u",        "disable support for new VRs, convert to OB");
    cmd.addSubGroup("group length encoding:");
      cmd.addOption("--group-length-recalc",    "+g=",       "recalculate group lengths if present (default)");
      cmd.addOption("--group-length-create",    "+g",        "always write with group length elements");
      cmd.addOption("--group-length-remove",    "-g",        "always write without group length elements");
    cmd.addSubGroup("length encoding in sequences and items:");
      cmd.addOption("--length-explicit",        "+e",        "write with explicit lengths (default)");
      cmd.addOption("--length-undefined",       "-e",        "write with undefined lengths");
    cmd.addSubGroup("data set trailing padding (not with --write-dataset):");
      cmd.addOption("--padding-retain",         "-p=",       "do not change padding\n(default if not --write-dataset)");
      cmd.addOption("--padding-off",            "-p",        "no padding (implicit if --write-dataset)");
      cmd.addOption("--padding-create",         "+p",    2,  "[f]ile-pad [i]tem-pad: integer",
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
              CERR << "- " << DiJPEGPlugin::getLibraryVersionString() << endl;
              return 0;
          }
      }

      /* command line parameters */

      cmd.getParam(1, opt_ifname);
      cmd.getParam(2, opt_ofname);

      /* options */

      if (cmd.findOption("--verbose")) opt_verbose = OFTrue;
      if (cmd.findOption("--debug")) opt_debugMode = 5;

      cmd.beginOptionBlock();
      if (cmd.findOption("--planar-auto")) opt_planarconfig = EPC_default;
      if (cmd.findOption("--color-by-pixel")) opt_planarconfig = EPC_colorByPixel;
      if (cmd.findOption("--color-by-plane")) opt_planarconfig = EPC_colorByPlane;
      cmd.endOptionBlock();

      cmd.beginOptionBlock();
      if (cmd.findOption("--conv-photometric"))  opt_decompCSconversion = EDC_photometricInterpretation;
      if (cmd.findOption("--conv-lossy"))        opt_decompCSconversion = EDC_lossyOnly;
      if (cmd.findOption("--conv-always"))       opt_decompCSconversion = EDC_always;
      if (cmd.findOption("--conv-never"))        opt_decompCSconversion = EDC_never;
      cmd.endOptionBlock();

      cmd.beginOptionBlock();
      if (cmd.findOption("--uid-default")) opt_uidcreation = EUC_default;
      if (cmd.findOption("--uid-always")) opt_uidcreation = EUC_always;
      cmd.endOptionBlock();

      cmd.beginOptionBlock();
      if (cmd.findOption("--read-file"))
      {
        opt_readMode = ERM_autoDetect;
        opt_ixfer = EXS_Unknown;
      }
      if (cmd.findOption("--read-file-only"))
      {
        opt_readMode = ERM_fileOnly;
        opt_ixfer = EXS_Unknown;
      }
      if (cmd.findOption("--read-dataset"))
      {
        opt_readMode = ERM_dataset;

        // we don't know the real transfer syntax of the dataset, but this does
        // not matter. As long as the content of encapsulated pixel sequences is
        // some kind of JPEG bitstream supported by the underlying library, the
        // decompression will work. So we simply choose one of the lossless
        // transfer syntaxes, because these support all bit depths up to 16.
        opt_ixfer = EXS_JPEGProcess14TransferSyntax;
      }
      cmd.endOptionBlock();

      cmd.beginOptionBlock();
      if (cmd.findOption("--write-file")) opt_oDataset = OFFalse;
      if (cmd.findOption("--write-dataset")) opt_oDataset = OFTrue;
      cmd.endOptionBlock();

      cmd.beginOptionBlock();
      if (cmd.findOption("--write-xfer-little")) opt_oxfer = EXS_LittleEndianExplicit;
      if (cmd.findOption("--write-xfer-big")) opt_oxfer = EXS_BigEndianExplicit;
      if (cmd.findOption("--write-xfer-implicit")) opt_oxfer = EXS_LittleEndianImplicit;
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
      if (cmd.findOption("--group-length-recalc")) opt_oglenc = EGL_recalcGL;
      if (cmd.findOption("--group-length-create")) opt_oglenc = EGL_withGL;
      if (cmd.findOption("--group-length-remove")) opt_oglenc = EGL_withoutGL;
      cmd.endOptionBlock();

      cmd.beginOptionBlock();
      if (cmd.findOption("--length-explicit")) opt_oenctype = EET_ExplicitLength;
      if (cmd.findOption("--length-undefined")) opt_oenctype = EET_UndefinedLength;
      cmd.endOptionBlock();

      cmd.beginOptionBlock();
      if (cmd.findOption("--padding-retain"))
      {
        if (opt_oDataset) app.printError("--padding-retain not allowed with --write-dataset");
        opt_opadenc = EPD_noChange;
      }
      if (cmd.findOption("--padding-off")) opt_opadenc = EPD_withoutPadding;
      if (cmd.findOption("--padding-create"))
      {
          if (opt_oDataset) app.printError("--padding-create not allowed with --write-dataset");
          app.checkValue(cmd.getValueAndCheckMin(opt_filepad, 0));
          app.checkValue(cmd.getValueAndCheckMin(opt_itempad, 0));
          opt_opadenc = EPD_withPadding;
      }
      cmd.endOptionBlock();

    }

    SetDebugLevel((opt_debugMode));

    // register global decompression codecs
    DJDecoderRegistration::registerCodecs(
      opt_decompCSconversion,
      opt_uidcreation,
      opt_planarconfig,
      opt_verbose);

    /* make sure data dictionary is loaded */
    if (!dcmDataDict.isDictionaryLoaded())
    {
        CERR << "Warning: no data dictionary loaded, "
             << "check environment variable: "
             << DCM_DICT_ENVIRONMENT_VARIABLE << endl;
    }

    // open inputfile
    if ((opt_ifname == NULL) || (strlen(opt_ifname) == 0))
    {
        CERR << "Error: invalid filename: <empty string>" << endl;
        return 1;
    }

    OFCondition error = EC_Normal;

    DcmFileFormat fileformat;

    if (opt_verbose)
        COUT << "reading input file " << opt_ifname << endl;

    error = fileformat.loadFile(opt_ifname, opt_ixfer, EGL_noChange, DCM_MaxReadLength, opt_readMode);
    if (error.bad())
    {
        CERR << "Error: "
             << error.text()
             << ": reading file: " <<  opt_ifname << endl;
        return 1;
    }

    DcmDataset *dataset = fileformat.getDataset();

    if (opt_verbose)
        COUT << "decompressing file" << endl;

    DcmXfer opt_oxferSyn(opt_oxfer);

    error = dataset->chooseRepresentation(opt_oxfer, NULL);
    if (error.bad())
    {
        CERR << "Error: "
             << error.text()
             << ": decompressing file: " <<  opt_ifname << endl;
       if (error == EJ_UnsupportedColorConversion)
           CERR << "Try --conv-never to disable color space conversion" << endl;
        return 1;
    }

    if (! dataset->canWriteXfer(opt_oxfer))
    {
        CERR << "Error: no conversion to transfer syntax " << opt_oxferSyn.getXferName()
             << " possible" << endl;
        return 1;
    }

    if (opt_verbose)
        COUT << "creating output file " << opt_ofname << endl;

    fileformat.loadAllDataIntoMemory();
    error = fileformat.saveFile(opt_ofname, opt_oxfer, opt_oenctype, opt_oglenc,
              opt_opadenc, (Uint32) opt_filepad, (Uint32) opt_itempad, opt_oDataset);
    if (error != EC_Normal)
    {
        CERR << "Error: "
             << error.text()
             << ": writing file: " <<  opt_ofname << endl;
        return 1;
    }

    if (opt_verbose)
        COUT << "conversion successful" << endl;

    // deregister global decompression codecs
    DJDecoderRegistration::cleanup();

    return 0;
}


/*
 * CVS/RCS Log:
 * $Log: dcmdjpeg.cc,v $
 * Revision 1.13  2005/12/08 15:43:21  meichel
 * Changed include path schema for all DCMTK header files
 *
 * Revision 1.12  2005/12/02 09:41:40  joergr
 * Added new command line option that checks whether a given file starts with a
 * valid DICOM meta header.
 *
 * Revision 1.11  2005/11/30 14:10:43  onken
 * Added hint concerning --convert-never option when color conversion fails
 *
 * Revision 1.10  2005/11/07 17:10:21  meichel
 * All tools that both read and write a DICOM file now call loadAllDataIntoMemory()
 *   to make sure they do not destroy a file when output = input.
 *
 * Revision 1.9  2005/05/26 14:35:07  meichel
 * Added option --read-dataset to dcmdjpeg that allows to decompress JPEG
 *   compressed DICOM objects that have been stored as dataset without meta-header.
 *   Such a thing should not exist since the transfer syntax cannot be reliably
 *   determined without meta-header, but unfortunately it does.
 *
 * Revision 1.8  2004/01/16 14:28:01  joergr
 * Updated copyright header.
 *
 * Revision 1.7  2002/11/27 15:39:56  meichel
 * Adapted module dcmjpeg to use of new header file ofstdinc.h
 *
 * Revision 1.6  2002/11/26 08:44:41  meichel
 * Replaced all includes for "zlib.h" with <zlib.h>
 *   to avoid inclusion of zlib.h in the makefile dependencies.
 *
 * Revision 1.5  2002/09/23 18:14:07  joergr
 * Added new command line option "--version" which prints the name and version
 * number of external libraries used (incl. preparation for future support of
 * 'config.guess' host identifiers).
 *
 * Revision 1.4  2002/08/20 12:20:58  meichel
 * Adapted code to new loadFile and saveFile methods, thus removing direct
 *   use of the DICOM stream classes.
 *
 * Revision 1.3  2002/07/10 12:26:02  meichel
 * Fixed memory leak in command line applications
 *
 * Revision 1.2  2001/11/19 15:13:22  meichel
 * Introduced verbose mode in module dcmjpeg. If enabled, warning
 *   messages from the IJG library are printed on ofConsole, otherwise
 *   the library remains quiet.
 *
 * Revision 1.1  2001/11/13 15:56:10  meichel
 * Initial release of module dcmjpeg
 *
 *
 */
