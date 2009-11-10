/*
 *
 *  Copyright (C) 1994-2005, OFFIS
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
 *  Author:  Andrew Hewett
 *
 *  Purpose: Convert dicom file encoding
 *
 *  Last Update:      $Author: meichel $
 *  Update Date:      $Date: 2005/12/08 15:40:43 $
 *  CVS/RCS Revision: $Revision: 1.48 $
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
#include "dcmtk/dcmdata/dcostrmz.h"    /* for dcmZlibCompressionLevel */
#include "dcmtk/dcmdata/dcistrmz.h"    /* for dcmZlibExpectRFC1950Encoding */

#ifdef WITH_ZLIB
#include <zlib.h>        /* for zlibVersion() */
#endif

#define OFFIS_CONSOLE_APPLICATION "dcmconv"

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
  E_TransferSyntax opt_ixfer = EXS_Unknown;
  E_TransferSyntax opt_oxfer = EXS_Unknown;
  E_GrpLenEncoding opt_oglenc = EGL_recalcGL;
  E_EncodingType opt_oenctype = EET_ExplicitLength;
  E_PaddingEncoding opt_opadenc = EPD_noChange;
  OFCmdUnsignedInt opt_filepad = 0;
  OFCmdUnsignedInt opt_itempad = 0;
#ifdef WITH_ZLIB
  OFCmdUnsignedInt opt_compressionLevel = 0;
#endif

  OFConsoleApplication app(OFFIS_CONSOLE_APPLICATION , "Convert DICOM file encoding", rcsid);
  OFCommandLine cmd;
  cmd.setOptionColumns(LONGCOL, SHORTCOL);
  cmd.setParamColumn(LONGCOL + SHORTCOL + 4);

  cmd.addParam("dcmfile-in",  "DICOM input filename to be converted");
  cmd.addParam("dcmfile-out", "DICOM output filename");

  cmd.addGroup("general options:", LONGCOL, SHORTCOL + 2);
   cmd.addOption("--help",                   "-h",     "print this help text and exit");
   cmd.addOption("--version",                          "print version information and exit", OFTrue /* exclusive */);
   cmd.addOption("--verbose",                "-v",     "verbose mode, print processing details");
   cmd.addOption("--debug",                  "-d",     "debug mode, print debug information");

  cmd.addGroup("input options:");
    cmd.addSubGroup("input file format:");
     cmd.addOption("--read-file",            "+f",     "read file format or data set (default)");
     cmd.addOption("--read-file-only",       "+fo",    "read file format only");
     cmd.addOption("--read-dataset",         "-f",     "read data set without file meta information");
    cmd.addSubGroup("input transfer syntax:", LONGCOL, SHORTCOL);
     cmd.addOption("--read-xfer-auto",       "-t=",    "use TS recognition (default)");
     cmd.addOption("--read-xfer-detect",     "-td",    "ignore TS specified in the file meta header");
     cmd.addOption("--read-xfer-little",     "-te",    "read with explicit VR little endian TS");
     cmd.addOption("--read-xfer-big",        "-tb",    "read with explicit VR big endian TS");
     cmd.addOption("--read-xfer-implicit",   "-ti",    "read with implicit VR little endian TS");
    cmd.addSubGroup("parsing of odd-length attributes:");
     cmd.addOption("--accept-odd-length",    "+ao",    "accept odd length attributes (default)");
     cmd.addOption("--assume-even-length",   "+ae",    "assume real length is one byte larger");
    cmd.addSubGroup("handling of undefined length UN elements:");
     cmd.addOption("--enable-cp246",         "+ui",    "read undefined len UN as implicit VR (default)");
     cmd.addOption("--disable-cp246",        "-ui",    "read undefined len UN as explicit VR");
    cmd.addSubGroup("handling of defined length UN elements:");
     cmd.addOption("--retain-un",            "-uc",    "retain elements as UN (default)");
     cmd.addOption("--convert-un",           "+uc",    "convert to real VR if known");
    cmd.addSubGroup("automatic data correction:");
     cmd.addOption("--enable-correction",    "+dc",    "enable automatic data correction (default)");
     cmd.addOption("--disable-correction",   "-dc",    "disable automatic data correction");
#ifdef WITH_ZLIB
    cmd.addSubGroup("bitstream format of deflated input:");
     cmd.addOption("--bitstream-deflated",   "+bd",    "expect deflated bitstream (default)");
     cmd.addOption("--bitstream-zlib",       "+bz",    "expect deflated zlib bitstream");
#endif

  cmd.addGroup("output options:");
    cmd.addSubGroup("output file format:");
      cmd.addOption("--write-file",          "+F",     "write file format (default)");
      cmd.addOption("--write-dataset",       "-F",     "write data set without file meta information");
    cmd.addSubGroup("output transfer syntax:");
      cmd.addOption("--write-xfer-same",     "+t=",    "write with same TS as input (default)");
      cmd.addOption("--write-xfer-little",   "+te",    "write with explicit VR little endian TS");
      cmd.addOption("--write-xfer-big",      "+tb",    "write with explicit VR big endian TS");
      cmd.addOption("--write-xfer-implicit", "+ti",    "write with implicit VR little endian TS");
#ifdef WITH_ZLIB
      cmd.addOption("--write-xfer-deflated", "+td",    "write with deflated expl. VR little endian TS");
#endif
    cmd.addSubGroup("post-1993 value representations:");
      cmd.addOption("--enable-new-vr",       "+u",     "enable support for new VRs (UN/UT) (default)");
      cmd.addOption("--disable-new-vr",      "-u",     "disable support for new VRs, convert to OB");
    cmd.addSubGroup("group length encoding:");
      cmd.addOption("--group-length-recalc", "+g=",    "recalculate group lengths if present (default)");
      cmd.addOption("--group-length-create", "+g",     "always write with group length elements");
      cmd.addOption("--group-length-remove", "-g",     "always write without group length elements");
    cmd.addSubGroup("length encoding in sequences and items:");
      cmd.addOption("--length-explicit",     "+e",     "write with explicit lengths (default)");
      cmd.addOption("--length-undefined",    "-e",     "write with undefined lengths");
    cmd.addSubGroup("data set trailing padding (not with --write-dataset):");
      cmd.addOption("--padding-retain",      "-p=",    "do not change padding\n(default if not --write-dataset)");
      cmd.addOption("--padding-off",         "-p",     "no padding (implicit if --write-dataset)");
      cmd.addOption("--padding-create",      "+p",  2, "[f]ile-pad [i]tem-pad: integer",
                                                       "align file on multiple of f bytes\nand items on multiple of i bytes");
#ifdef WITH_ZLIB
    cmd.addSubGroup("deflate compression level (only with --write-xfer-deflated):");
      cmd.addOption("--compression-level",   "+cl", 1, "compression level: 0-9 (default 6)",
                                                       "0=uncompressed, 1=fastest, 9=best compression");
#endif

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

      /* command line parameters */

      cmd.getParam(1, opt_ifname);
      cmd.getParam(2, opt_ofname);

      if (cmd.findOption("--verbose")) opt_verbose = OFTrue;
      if (cmd.findOption("--debug")) opt_debugMode = 5;

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

      cmd.beginOptionBlock();
      if (cmd.findOption("--accept-odd-length"))
      {
        dcmAcceptOddAttributeLength.set(OFTrue);
      }
      if (cmd.findOption("--assume-even-length"))
      {
        dcmAcceptOddAttributeLength.set(OFFalse);
      }
      cmd.endOptionBlock();

      cmd.beginOptionBlock();
      if (cmd.findOption("--enable-cp246"))
      {
        dcmEnableCP246Support.set(OFTrue);
      }
      if (cmd.findOption("--disable-cp246"))
      {
        dcmEnableCP246Support.set(OFFalse);
      }
      cmd.endOptionBlock();

      cmd.beginOptionBlock();
      if (cmd.findOption("--retain-un"))
      {
        dcmEnableUnknownVRConversion.set(OFFalse);
      }
      if (cmd.findOption("--convert-un"))
      {
        dcmEnableUnknownVRConversion.set(OFTrue);
      }
      cmd.endOptionBlock();

      cmd.beginOptionBlock();
      if (cmd.findOption("--enable-correction"))
      {
        dcmEnableAutomaticInputDataCorrection.set(OFTrue);
      }
      if (cmd.findOption("--disable-correction"))
      {
        dcmEnableAutomaticInputDataCorrection.set(OFFalse);
      }
      cmd.endOptionBlock();

#ifdef WITH_ZLIB
      cmd.beginOptionBlock();
      if (cmd.findOption("--bitstream-deflated"))
      {
        dcmZlibExpectRFC1950Encoding.set(OFFalse);
      }
      if (cmd.findOption("--bitstream-zlib"))
      {
        dcmZlibExpectRFC1950Encoding.set(OFTrue);
      }
      cmd.endOptionBlock();
#endif

      cmd.beginOptionBlock();
      if (cmd.findOption("--write-file")) opt_oDataset = OFFalse;
      if (cmd.findOption("--write-dataset")) opt_oDataset = OFTrue;
      cmd.endOptionBlock();

      cmd.beginOptionBlock();
      if (cmd.findOption("--write-xfer-same")) opt_oxfer = EXS_Unknown;
      if (cmd.findOption("--write-xfer-little")) opt_oxfer = EXS_LittleEndianExplicit;
      if (cmd.findOption("--write-xfer-big")) opt_oxfer = EXS_BigEndianExplicit;
      if (cmd.findOption("--write-xfer-implicit")) opt_oxfer = EXS_LittleEndianImplicit;
#ifdef WITH_ZLIB
      if (cmd.findOption("--write-xfer-deflated")) opt_oxfer = EXS_DeflatedLittleEndianExplicit;
#endif
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
          app.checkConflict("--padding-retain", "--write-dataset", opt_oDataset);
          opt_opadenc = EPD_noChange;
      }
      if (cmd.findOption("--padding-off")) opt_opadenc = EPD_withoutPadding;
      if (cmd.findOption("--padding-create"))
      {
          app.checkConflict("--padding-create", "--write-dataset", opt_oDataset);
          app.checkValue(cmd.getValueAndCheckMin(opt_filepad, 0));
          app.checkValue(cmd.getValueAndCheckMin(opt_itempad, 0));
          opt_opadenc = EPD_withPadding;
      }
      cmd.endOptionBlock();

#ifdef WITH_ZLIB
      cmd.beginOptionBlock();
      if (cmd.findOption("--compression-level"))
      {
          app.checkDependence("--compression-level", "--write-xfer-deflated", opt_oxfer == EXS_DeflatedLittleEndianExplicit);
          app.checkValue(cmd.getValueAndCheckMinMax(opt_compressionLevel, 0, 9));
          dcmZlibCompressionLevel.set(OFstatic_cast(int, opt_compressionLevel));
      }
      cmd.endOptionBlock();
#endif

    }

    SetDebugLevel((opt_debugMode));

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

    DcmFileFormat fileformat;
    DcmDataset * dataset = fileformat.getDataset();

    if (opt_verbose)
        COUT << "open input file " << opt_ifname << endl;

    OFCondition error = fileformat.loadFile(opt_ifname, opt_ixfer, EGL_noChange, DCM_MaxReadLength, opt_readMode);

    if (error.bad())
    {
        CERR << "Error: "
             << error.text()
             << ": reading file: " <<  opt_ifname << endl;
        return 1;
    }

    if (opt_verbose)
        COUT << "load all data into memory" << endl;
    /* make sure that pixel data is loaded before output file is created */
    dataset->loadAllDataIntoMemory();

    if (opt_oxfer == EXS_Unknown)
    {
        if (opt_verbose)
            COUT << "set output transfer syntax to input transfer syntax" << endl;
        opt_oxfer = dataset->getOriginalXfer();
    }

    if (opt_verbose)
        COUT << "check if new output transfer syntax is possible" << endl;

    DcmXfer opt_oxferSyn(opt_oxfer);

    dataset->chooseRepresentation(opt_oxfer, NULL);

    if (dataset->canWriteXfer(opt_oxfer))
    {
        if (opt_verbose)
            COUT << "output transfer syntax " << opt_oxferSyn.getXferName()
                 << " can be written" << endl;
    } else {
        CERR << "Error: no conversion to transfer syntax " << opt_oxferSyn.getXferName()
             << " possible!" << endl;
        return 1;
    }

    if (opt_verbose)
        COUT << "create output file " << opt_ofname << endl;

    error = fileformat.saveFile(opt_ofname, opt_oxfer, opt_oenctype, opt_oglenc, opt_opadenc,
        OFstatic_cast(Uint32, opt_filepad), OFstatic_cast(Uint32, opt_itempad), opt_oDataset);

    if (error.bad())
    {
        CERR << "Error: "
             << error.text()
             << ": writing file: " <<  opt_ofname << endl;
        return 1;
    }

    if (opt_verbose)
        COUT << "conversion successful" << endl;

    return 0;
}


/*
** CVS/RCS Log:
** $Log: dcmconv.cc,v $
** Revision 1.48  2005/12/08 15:40:43  meichel
** Changed include path schema for all DCMTK header files
**
** Revision 1.47  2005/12/02 09:00:37  joergr
** Added new command line option that ignores the transfer syntax specified in
** the meta header and tries to detect the transfer syntax automatically from
** the dataset.
** Added new command line option that checks whether a given file starts with a
** valid DICOM meta header.
**
** Revision 1.46  2005/11/15 18:33:20  meichel
** Added new command line option --convert-un that enables the re-conversion of
**   defined length UN elements.
**
** Revision 1.45  2005/05/10 15:27:09  meichel
** Added support for reading UN elements with undefined length according
**   to CP 246. The global flag dcmEnableCP246Support allows to revert to the
**   prior behaviour in which UN elements with undefined length were parsed
**   like a normal explicit VR SQ element.
**
** Revision 1.44  2004/04/07 12:20:27  joergr
** Additional modifications for new-style type casts.
**
** Revision 1.43  2004/01/16 10:53:53  joergr
** Adapted type casts to new-style typecast operators defined in ofcast.h.
**
** Revision 1.42  2003/01/08 10:34:23  joergr
** Fixed typo (missing closing bracket).
**
** Revision 1.41  2002/11/27 12:07:15  meichel
** Adapted module dcmdata to use of new header file ofstdinc.h
**
** Revision 1.40  2002/11/26 08:42:59  meichel
** Replaced all includes for "zlib.h" with <zlib.h>
**   to avoid inclusion of zlib.h in the makefile dependencies.
**
** Revision 1.39  2002/11/25 10:15:07  joergr
** Used OFConsoleApplication::checkDependence() and checkConflict() routine
** wherever possible.
** Made declaration of compression level variable dependent on WITH_ZLIB define.
** Avoids warning about unused variable reported by gcc 2.95.3 on Cygwin.
**
** Revision 1.38  2002/09/23 17:52:01  joergr
** Prepared code for future support of 'config.guess' host identifiers.
**
** Revision 1.37  2002/09/23 13:50:39  joergr
** Added new command line option "--version" which prints the name and version
** number of external libraries used.
**
** Revision 1.36  2002/08/27 16:55:25  meichel
** Initial release of new DICOM I/O stream classes that add support for stream
**   compression (deflated little endian explicit VR transfer syntax)
**
** Revision 1.35  2002/08/21 10:14:14  meichel
** Adapted code to new loadFile and saveFile methods, thus removing direct
**   use of the DICOM stream classes.
**
** Revision 1.34  2002/07/08 14:44:53  meichel
** Improved dcmdata behaviour when reading odd tag length. Depending on the
**   global boolean flag dcmAcceptOddAttributeLength, the parser now either accepts
**   odd length attributes or implements the old behaviour, i.e. assumes a real
**   length larger by one.
**
** Revision 1.33  2001/11/28 12:42:30  joergr
** Fixed bug in dcmconv that caused problems in cases where the same filename
** was used for input and output (e.g. pixel data was removed without any
** warnings).
**
** Revision 1.32  2001/11/09 15:50:52  joergr
** Renamed some of the getValue/getParam methods to avoid ambiguities reported
** by certain compilers.
**
** Revision 1.31  2001/09/25 17:20:59  meichel
** Adapted dcmdata to class OFCondition
**
** Revision 1.30  2001/06/01 15:48:27  meichel
** Updated copyright header
**
** Revision 1.29  2000/04/14 15:42:53  meichel
** Global VR generation flags are now derived from OFGlobal and, thus,
**   safe for use in multi-thread applications.
**
** Revision 1.28  2000/03/08 16:26:04  meichel
** Updated copyright header.
**
** Revision 1.27  2000/03/06 18:09:36  joergr
** Avoid empty statement in the body of if-statements (MSVC6 reports warnings).
**
** Revision 1.26  2000/03/03 14:05:15  meichel
** Implemented library support for redirecting error messages into memory
**   instead of printing them to stdout/stderr for GUI applications.
**
** Revision 1.25  2000/02/29 11:48:49  meichel
** Removed support for VS value representation. This was proposed in CP 101
**   but never became part of the standard.
**
** Revision 1.24  2000/02/23 15:11:29  meichel
** Corrected macro for Borland C++ Builder 4 workaround.
**
** Revision 1.23  2000/02/01 10:11:56  meichel
** Avoiding to include <stdlib.h> as extern "C" on Borland C++ Builder 4,
**   workaround for bug in compiler header files.
**
** Revision 1.22  1999/10/07 17:27:03  meichel
** Corrected typo in dcmconv error message
**
** Revision 1.21  1999/04/27 17:50:49  joergr
** Adapted console applications to new OFCommandLine and OFConsoleApplication
** functionality.
**
** Revision 1.20  1999/04/27 12:23:22  meichel
** Prevented dcmdata applications from opening a file with empty filename,
**   leads to application crash on Win32.
**
** Revision 1.19  1999/03/31 09:24:18  meichel
** Updated copyright header in module dcmdata
**
** Revision 1.18  1999/03/29 10:14:11  meichel
** Adapted command line options of dcmdata applications to new scheme.
**
** Revision 1.17  1998/01/27 10:51:26  meichel
** Removed some unused variables, meaningless const modifiers
**   and unreached statements.
**
** Revision 1.16  1998/01/14 14:41:13  hewett
** Modified existing -u command line option to also disable generation
** of UT and VS (previously just disabled generation of UN).
**
** Revision 1.15  1997/07/21 08:12:42  andreas
** - New environment for encapsulated pixel representations. DcmPixelData
**   can contain different representations and uses codecs to convert
**   between them. Codecs are derived from the DcmCodec class. New error
**   codes are introduced for handling of representations. New internal
**   value representation (only for ident()) for PixelData
** - Replace all boolean types (BOOLEAN, CTNBOOLEAN, DICOM_BOOL, BOOL)
**   with one unique boolean type OFBool.
**
** Revision 1.14  1997/07/03 15:09:37  andreas
** - removed debugging functions Bdebug() and Edebug() since
**   they write a static array and are not very useful at all.
**   Cdebug and Vdebug are merged since they have the same semantics.
**   The debugging functions in dcmdata changed their interfaces
**   (see dcmdata/include/dcdebug.h)
**
** Revision 1.13  1997/05/30 06:44:56  andreas
** - fixed scanf format problem leading to warnings on 64 bit machines.
**
** Revision 1.12  1997/05/29 15:52:50  meichel
** Added constant for dcmtk release date in dcuid.h.
** All dcmtk applications now contain a version string
** which is displayed with the command line options ("usage" message)
** and which can be queried in the binary with the "ident" command.
**
** Revision 1.11  1997/05/27 13:47:40  andreas
** - Add method canWriteXfer to class DcmObject and all derived classes.
**   This method checks whether it is possible to convert the original
**   transfer syntax to an new transfer syntax. The check is used in the
**   dcmconv utility to prohibit the change of a compressed transfer
**   syntax to a uncompressed.
**
** Revision 1.10  1997/05/22 13:26:22  hewett
** Modified the test for presence of a data dictionary to use the
** method DcmDataDictionary::isDictionaryLoaded().
**
** Revision 1.9  1997/05/20 07:57:10  andreas
** - Removed obsolete applications file2ds and ds2file. The functionality of these
**   applications is now peformed by dcmconv. Unified calling parameters
**   are implemented in dump2dcm, dcmdump and dcmconv.
**
** Revision 1.8  1997/05/16 08:31:04  andreas
** - Revised handling of GroupLength elements and support of
**   DataSetTrailingPadding elements. The enumeratio E_GrpLenEncoding
**   got additional enumeration values (for a description see dctypes.h).
**   addGroupLength and removeGroupLength methods are replaced by
**   computeGroupLengthAndPadding. To support Padding, the parameters of
**   element and sequence write functions changed.
**
** Revision 1.7  1997/03/27 15:47:23  hewett
** Added command line switche to allow generation of UN to be
** disabled (it is enabled by default).
**
** Revision 1.6  1996/09/24 16:13:49  hewett
** Added preliminary support for the Macintosh environment (GUSI library).
**
** Revision 1.5  1996/03/12 15:11:37  hewett
** Added call to prepareCmdLineArgs to enable command line arguments
** in environments which do not provide them.
**
** Revision 1.4  1996/01/09 14:10:13  hewett
** Added check for presence of input and output files on command line.
**
** Revision 1.3  1996/01/09 11:05:59  andreas
** New Support for Visual C++
** Correct problems with inconsistent const declarations
** Correct error in reading Item Delimitation Tags
**
** Revision 1.2  1996/01/05 13:29:34  andreas
** - new streaming facilities
** - unique read/write methods for block and file transfer
** - more cleanups
**
** Revision 1.1  1995/11/23 17:10:30  hewett
** Updated for loadable data dictionary.
**
*/
