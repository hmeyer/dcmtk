/*
 *
 *  Copyright (C) 1999-2005, OFFIS
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
 *  Module:  dcmpstat
 *
 *  Authors: Marco Eichelberg
 *
 *  Purpose
 *    sample application that reads multiple images and (optionally)
 *    presentation states and creates a print job consisting of
 *    stored print and hardcopy grayscale images.
 *    Non-grayscale transformations in the presentation state are ignored.
 *
 *  Last Update:      $Author: meichel $
 *  Update Date:      $Date: 2005/12/08 15:46:09 $
 *  Source File:      $Source: /share/dicom/cvs-depot/dcmtk/dcmpstat/apps/dcmpsprt.cc,v $
 *  CVS/RCS Revision: $Revision: 1.36 $
 *  Status:           $State: Exp $
 *
 *  CVS/RCS Log at end of file
 *
 */


#include "dcmtk/config/osconfig.h"    /* make sure OS specific configuration is included first */

#define INCLUDE_CCTYPE
#include "dcmtk/ofstd/ofstdinc.h"

#ifdef HAVE_GUSI_H
#include <GUSI.h>
#endif

#include "dcmtk/ofstd/ofstream.h"
#include "dcmtk/dcmpstat/dviface.h"
#include "dcmtk/dcmpstat/dvpssp.h"
#include "dcmtk/dcmimgle/dcmimage.h"
#include "dcmtk/dcmdata/cmdlnarg.h"
#include "dcmtk/ofstd/ofcmdln.h"
#include "dcmtk/ofstd/ofconapp.h"
#include "dcmtk/dcmdata/dcuid.h"       /* for dcmtk version name */
#include "dcmtk/ofstd/oflist.h"
#include "dcmtk/dcmdata/dcdebug.h"

#ifdef WITH_ZLIB
#include <zlib.h>        /* for zlibVersion() */
#endif

#define OFFIS_CONSOLE_APPLICATION "dcmpsprt"

static char rcsid[] = "$dcmtk: " OFFIS_CONSOLE_APPLICATION " v"
  OFFIS_DCMTK_VERSION " " OFFIS_DCMTK_RELEASEDATE " $";


int addOverlay(const char *filename,
               unsigned long xpos,
               unsigned long ypos,
               Uint16 *pixel,
               unsigned long width,
               unsigned long height,
               unsigned int gray)
{
    if ((filename != NULL) && (pixel != NULL))
    {
#ifdef HAVE_IOS_NOCREATE
        ifstream input(filename, ios::in|ios::nocreate);
#else
        ifstream input(filename);
#endif
        if (input)
        {
            char c;
            unsigned int xsize, ysize;
            if (input.get(c) && (c == 'P') && input.get(c) && (c == '1'))
            {
                /* still need to add code for skipping comments in PBM file */
                input >> xsize;
                input >> ysize;
                if ((xpos + xsize <= width) && (ypos + ysize <= height))
                {
                    unsigned int value;
                    Uint16 *p = pixel + (ypos * width) + xpos;
                    for (unsigned long ys = 0; ys < ysize; ys++)
                    {
                        for (unsigned long xs = 0; xs < xsize; xs++)
                        {
                            while (input.get(c) && !isdigit(c));                            // skip non-numeric chars
                            input.putback(c);
                            input >> value;
                            if (value)
                                *p = gray;
                            p++;
                        }
                        p += (width - xsize);
                    }
                    return 1;
                } else
                    CERR << "error: invalid position for overlay PBM file '" << filename << endl;
            } else
                CERR << "error: overlay PBM file '" << filename << "' has no magic number P1" << endl;
        } else
            CERR << "error: can't open overlay PBM file '" << filename << "'" << endl;
    }
    return 0;
}


#define SHORTCOL 2
#define LONGCOL 21

int main(int argc, char *argv[])
{
    int                       opt_debugMode      = 0;           /* default: no debug */
    OFBool                    opt_verbose        = OFFalse;     /* default: do not dump presentation state */
    const char *              opt_printerID = NULL;             /* printer ID */
    const char *              opt_cfgName = NULL;               /* config read file name */
    DVPSFilmOrientation       opt_filmorientation = DVPSF_default;
    DVPSTrimMode              opt_trim = DVPSH_default;
    DVPSDecimateCropBehaviour opt_decimate = DVPSI_default;
    OFCmdUnsignedInt          opt_columns = 1;
    OFCmdUnsignedInt          opt_rows = 1;
    OFCmdUnsignedInt          opt_copies = 0;
    OFCmdUnsignedInt          opt_ovl_graylevel = 4095;
    const char *              opt_filmsize = NULL;
    const char *              opt_magnification = NULL;
    const char *              opt_smoothing = NULL;
    const char *              opt_configuration = NULL;
    const char *              opt_img_polarity = NULL;
    const char *              opt_img_request_size = NULL;
    const char *              opt_img_magnification = NULL;
    const char *              opt_img_smoothing = NULL;
    const char *              opt_img_configuration = NULL;
    const char *              opt_resolution = NULL;
    const char *              opt_border = NULL;
    const char *              opt_emptyimage = NULL;
    const char *              opt_maxdensity = NULL;
    const char *              opt_mindensity = NULL;
    const char *              opt_plutname = NULL;
    OFList<char *>            opt_filenames;
    int                       opt_LUTshape = 0; // 0=use SCP default, 1=IDENTITY, 2=LIN OD.
    OFBool                    opt_inverse_plut = OFFalse;
    OFBool                    opt_spool = OFFalse;
    const char *              opt_mediumtype = NULL;
    const char *              opt_destination     = NULL;
    const char *              opt_sessionlabel    = NULL;
    const char *              opt_priority        = NULL;
    const char *              opt_ownerID         = NULL;

    OFBool                    opt_annotation = OFFalse;
    OFBool                    opt_annotationDatetime = OFTrue;
    OFBool                    opt_annotationPrinter = OFTrue;
    OFBool                    opt_annotationIllumination = OFTrue;
    const char *              opt_annotationString = NULL;

    OFCmdUnsignedInt          opt_illumination = (OFCmdUnsignedInt)-1;
    OFCmdUnsignedInt          opt_reflection = (OFCmdUnsignedInt)-1;

    SetDebugLevel(( 0 ));
    DicomImageClass::setDebugLevel(DicomImageClass::DL_NoMessages);

    OFConsoleApplication app(OFFIS_CONSOLE_APPLICATION , "Read DICOM images and presentation states and render print job", rcsid);
    OFCommandLine cmd;
    cmd.setOptionColumns(LONGCOL, SHORTCOL+2);
    cmd.setParamColumn(LONGCOL + SHORTCOL + 4);

    cmd.addParam("imagefile_in",   "DICOM image file(s) to be printed", OFCmdParam::PM_MultiOptional);

    cmd.addGroup("general options:");
     cmd.addOption("--help",                      "-h",        "print this help text and exit");
     cmd.addOption("--version",                                "print version information and exit", OFTrue /* exclusive */);
     cmd.addOption("--verbose",                   "-v",        "verbose mode, print actions");
     cmd.addOption("--debug",                     "-d",        "debug mode, print debug information");

    cmd.addGroup("processing options:");
     cmd.addOption("--pstate",      "+p", 1, "[p]state-file: string",
                                             "render the following image with pres. state p");
     cmd.addOption("--config",      "-c", 1, "[f]ilename: string",
                                             "process using settings from configuration file f");
     cmd.addOption("--printer",     "-p", 1, "[n]ame: string (default: 1st printer in cfg file)",
                                             "select printer with identifier n from cfg file");

    cmd.addGroup("spooling options:");
     cmd.addOption("--spool",       "-s",    "spool print job to DICOM printer");
     cmd.addOption("--nospool",              "do not spool print job to DICOM printer (default)");

    cmd.addGroup("film orientation options:");
     cmd.addOption("--portrait",             "set portrait orientation");
     cmd.addOption("--landscape",            "set landscape orientation");
     cmd.addOption("--default-orientation",  "use printer default (default)");

    cmd.addGroup("trim (border) options:");
     cmd.addOption("--trim",                 "set trim on");
     cmd.addOption("--no-trim",              "set trim off");
     cmd.addOption("--default-trim",         "use printer default (default)");

    cmd.addGroup("requested decimate/crop behaviour options:");
     cmd.addOption("--request-decimate",     "request decimate");
     cmd.addOption("--request-crop",         "request crop");
     cmd.addOption("--request-fail",         "request failure");
     cmd.addOption("--default-request",      "use printer default (default)");

    cmd.addGroup("print presentation LUT options:");
     cmd.addOption("--default-plut",         "do not create presentation LUT (default)");
     cmd.addOption("--identity",             "set IDENTITY presentation LUT shape");
     cmd.addOption("--lin-od",               "set LIN OD presentation LUT shape");
     cmd.addOption("--plut",              1, "[l]ut identifier: string",
                                             "add LUT [l] to print job");
     cmd.addOption("--inverse-plut",         "render the inverse presentation LUT into the\nbitmap of the hardcopy grayscale image");
     cmd.addOption("--illumination",      1, "[v]alue: integer (0..65535)",
                                             "set illumination to v (in cd/m^2)");
     cmd.addOption("--reflection",        1, "[v]alue: integer (0..65535)",
                                             "set reflected ambient light to v (in cd/m^2)");

    cmd.addGroup("basic film session options (only with --spool):");
     cmd.addOption("--copies",            1, "[v]alue: integer (1..100, default: 1)",
                                             "set number of copies to v");
     cmd.addOption("--medium-type",       1, "[v]alue: string",
                                             "set medium type to v");
     cmd.addOption("--destination",       1, "[v]alue: string",
                                             "set film destination to v");
     cmd.addOption("--label",             1, "[v]alue: string",
                                             "set film session label to v");
     cmd.addOption("--priority",          1, "[v]alue: string",
                                             "set print priority to v");
     cmd.addOption("--owner",             1, "[v]alue: string",
                                             "set film session owner ID to v");

    cmd.addGroup("annotation options:");
     cmd.addOption("--no-annotation",        "do not create annotation (default)");
     cmd.addOption("--annotation",  "-a", 1, "[t]ext: string",
                                             "create annotation with text t");
     cmd.addOption("--print-date",        "+pd",  "prepend date/time to annotation (default)");
     cmd.addOption("--print-no-date",     "-pd",  "do not prepend date/time to annotation");

     cmd.addOption("--print-name",        "+pn",  "prepend printer name to annotation (default)");
     cmd.addOption("--print-no-name",     "-pn",  "do not prepend printer name to annotation");

     cmd.addOption("--print-lighting",    "+pl",  "prepend illumination to annotation (default)");
     cmd.addOption("--print-no-lighting", "-pl",  "do not prepend illumination to annotation");

    cmd.addGroup("overlay options:");
     cmd.addOption("--overlay",       "+O" , 3, "[f]ilename : string, [x] [y] : integer",
                                                "load overlay data from PBM file f and\ndisplay at position (x,y)");
     cmd.addOption("--ovl-graylevel", "+Og", 1, "[v]alue: integer (0..4095)",
                                                "use overlay gray level v (default: 4095 = white)");

    cmd.addGroup("other print options:");
     cmd.addOption("--layout",      "-l", 2, "[c]olumns [r]ows: integer (default: 1 1)",
                                             "use 'STANDARD\\c,r' image display format");
     cmd.addOption("--filmsize",          1, "[v]alue: string",
                                             "set film size ID to v");
     cmd.addOption("--magnification",     1, "[v]alue: string",
                                             "set magnification type to v");
     cmd.addOption("--smoothing",         1, "[v]alue: string",
                                             "set smoothing type to v");
     cmd.addOption("--configinfo",        1, "[v]alue: string",
                                             "set configuration information to v");
     cmd.addOption("--resolution",        1, "[v]alue: string",
                                             "set requested resolution ID to v");
     cmd.addOption("--border",            1, "[v]alue: string",
                                             "set border density to v");
     cmd.addOption("--empty-image",       1, "[v]alue: string",
                                             "set empty image density to v");
     cmd.addOption("--max-density",       1, "[v]alue: string",
                                             "set max density to v");
     cmd.addOption("--min-density",       1, "[v]alue: string",
                                             "set min density to v");
     cmd.addOption("--img-polarity",      1, "[v]alue: string",
                                             "set image box polarity to v (NORMAL or REVERSE)");
     cmd.addOption("--img-request-size",  1, "[v]alue: string",
                                             "set requested image size to v (width in mm)");
     cmd.addOption("--img-magnification", 1, "[v]alue: string",
                                             "set image box magnification type to v");
     cmd.addOption("--img-smoothing",     1, "[v]alue: string",
                                             "set image box smoothing type to v");
     cmd.addOption("--img-configinfo",    1, "[v]alue: string",
                                             "set image box configuration information to v");

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
      if (cmd.findOption("--verbose")) opt_verbose=OFTrue;
      if (cmd.findOption("--debug"))   opt_debugMode = 3;

      cmd.beginOptionBlock();
      if (cmd.findOption("--portrait"))  opt_filmorientation = DVPSF_portrait;
      if (cmd.findOption("--landscape")) opt_filmorientation = DVPSF_landscape;
      if (cmd.findOption("--default-orientation"))  opt_filmorientation = DVPSF_default;
      cmd.endOptionBlock();

      cmd.beginOptionBlock();
      if (cmd.findOption("--trim"))         opt_trim = DVPSH_trim_on;
      if (cmd.findOption("--no-trim"))      opt_trim = DVPSH_trim_off;
      if (cmd.findOption("--default-trim")) opt_trim = DVPSH_default;
      cmd.endOptionBlock();

      cmd.beginOptionBlock();
      if (cmd.findOption("--request-decimate")) opt_decimate = DVPSI_decimate;
      if (cmd.findOption("--request-crop"))     opt_decimate = DVPSI_crop;
      if (cmd.findOption("--request-fail"))     opt_decimate = DVPSI_fail;
      if (cmd.findOption("--default-request"))  opt_decimate = DVPSI_default;
      cmd.endOptionBlock();

      cmd.beginOptionBlock();
      if (cmd.findOption("--default-plut")) opt_LUTshape = 0;
      if (cmd.findOption("--identity"))     opt_LUTshape = 1;
      if (cmd.findOption("--lin-od"))       opt_LUTshape = 2;
      if (cmd.findOption("--plut"))         app.checkValue(cmd.getValue(opt_plutname));
      cmd.endOptionBlock();
      if (cmd.findOption("--inverse-plut")) opt_inverse_plut = OFTrue;

      cmd.beginOptionBlock();
      if (cmd.findOption("--spool"))   opt_spool = OFTrue;
      if (cmd.findOption("--nospool")) opt_spool = OFFalse;
      cmd.endOptionBlock();

      cmd.beginOptionBlock();
      if (cmd.findOption("--no-annotation")) opt_annotation = OFFalse;
      if (cmd.findOption("--annotation"))
      {
        opt_annotation = OFTrue;
        app.checkValue(cmd.getValue(opt_annotationString));
      }
      cmd.endOptionBlock();

      cmd.findOption("--overlay", 0, OFCommandLine::FOM_First);      /* check at least once to avoid warnings */
      if (cmd.findOption("--ovl-graylevel"))
         app.checkValue(cmd.getValueAndCheckMinMax(opt_ovl_graylevel, 0, 4095));

      cmd.beginOptionBlock();
      if (cmd.findOption("--print-date"))    opt_annotationDatetime = OFTrue;
      if (cmd.findOption("--print-no-date")) opt_annotationDatetime = OFFalse;
      cmd.endOptionBlock();

      cmd.beginOptionBlock();
      if (cmd.findOption("--print-name"))         opt_annotationPrinter = OFTrue;
      if (cmd.findOption("--print-no-name"))      opt_annotationPrinter = OFFalse;
      cmd.endOptionBlock();

      cmd.beginOptionBlock();
      if (cmd.findOption("--print-lighting"))         opt_annotationIllumination = OFTrue;
      if (cmd.findOption("--print-no-lighting"))      opt_annotationIllumination = OFFalse;
      cmd.endOptionBlock();

      if (cmd.findOption("--filmsize"))          app.checkValue(cmd.getValue(opt_filmsize));
      if (cmd.findOption("--magnification"))     app.checkValue(cmd.getValue(opt_magnification));
      if (cmd.findOption("--smoothing"))         app.checkValue(cmd.getValue(opt_smoothing));
      if (cmd.findOption("--configinfo"))        app.checkValue(cmd.getValue(opt_configuration));
      if (cmd.findOption("--resolution"))        app.checkValue(cmd.getValue(opt_resolution));
      if (cmd.findOption("--border"))            app.checkValue(cmd.getValue(opt_border));
      if (cmd.findOption("--empty-image"))       app.checkValue(cmd.getValue(opt_emptyimage));
      if (cmd.findOption("--max-density"))       app.checkValue(cmd.getValue(opt_maxdensity));
      if (cmd.findOption("--min-density"))       app.checkValue(cmd.getValue(opt_mindensity));
      if (cmd.findOption("--config"))            app.checkValue(cmd.getValue(opt_cfgName));
      if (cmd.findOption("--printer"))           app.checkValue(cmd.getValue(opt_printerID));
      if (cmd.findOption("--img-polarity"))      app.checkValue(cmd.getValue(opt_img_polarity));
      if (cmd.findOption("--img-request-size"))  app.checkValue(cmd.getValue(opt_img_request_size));
      if (cmd.findOption("--img-magnification")) app.checkValue(cmd.getValue(opt_img_magnification));
      if (cmd.findOption("--img-smoothing"))     app.checkValue(cmd.getValue(opt_img_smoothing));
      if (cmd.findOption("--img-configinfo"))    app.checkValue(cmd.getValue(opt_img_configuration));

      /* film session options */
      if (cmd.findOption("--medium-type"))
      {
        app.checkConflict("--medium-type", "--nospool", (! opt_spool));
        app.checkValue(cmd.getValue(opt_mediumtype));
      }
      if (cmd.findOption("--illumination"))
      {
        app.checkValue(cmd.getValueAndCheckMinMax(opt_illumination, 0, 65535));
      }
      if (cmd.findOption("--reflection"))
      {
        app.checkValue(cmd.getValueAndCheckMinMax(opt_reflection, 0, 65535));
      }

      if (cmd.findOption("--destination"))
      {
        app.checkConflict("--destination", "--nospool", (! opt_spool));
        app.checkValue(cmd.getValue(opt_destination));
      }
      if (cmd.findOption("--label"))
      {
        app.checkConflict("--label", "--nospool", (! opt_spool));
        app.checkValue(cmd.getValue(opt_sessionlabel));
      }
      if (cmd.findOption("--priority"))
      {
        app.checkConflict("--priority", "--nospool", (! opt_spool));
        app.checkValue(cmd.getValue(opt_priority));
      }
      if (cmd.findOption("--owner"))
      {
        app.checkConflict("--owner", "--nospool", (! opt_spool));
        app.checkValue(cmd.getValue(opt_ownerID));
      }
      if (cmd.findOption("--copies"))
      {
        app.checkConflict("--copies", "--nospool", (! opt_spool));
        app.checkValue(cmd.getValueAndCheckMinMax(opt_copies, 1, 100));
      }

      if (cmd.findOption("--pstate")) { /* prevent warning - this option is only checked if image filenames are really specified */ }

      if (cmd.findOption("--layout"))
      {
         app.checkValue(cmd.getValueAndCheckMin(opt_columns, 1));
         app.checkValue(cmd.getValueAndCheckMin(opt_rows, 1));
      }

      const char *imageFile=NULL;
      const char *pstateFile=NULL;
      int paramCount = cmd.getParamCount();
      for (int param=1; param<=paramCount; param++)
      {
        cmd.getParam(param, imageFile);
        pstateFile = NULL;
        if (cmd.findOption("--pstate", -param)) app.checkValue(cmd.getValue(pstateFile));
        opt_filenames.push_back((char *)imageFile);
        opt_filenames.push_back((char *)pstateFile);
      }
    }

    SetDebugLevel((opt_debugMode));
    DicomImageClass::setDebugLevel(opt_debugMode);

    if (opt_cfgName)
    {
      FILE *cfgfile = fopen(opt_cfgName, "rb");
      if (cfgfile) fclose(cfgfile); else
      {
        CERR << "error: can't open configuration file '" << opt_cfgName << "'" << endl;
        return 10;
      }
    }
    DVInterface dvi(opt_cfgName);

    if (opt_printerID && (EC_Normal != dvi.setCurrentPrinter(opt_printerID)))
      CERR << "warning: unable to select printer '" << opt_printerID << "', ignoring." << endl;

    /* dump printer characteristics if requested */
    const char *currentPrinter = dvi.getCurrentPrinter();

	if ((opt_img_request_size) && (!dvi.getTargetPrinterSupportsRequestedImageSize(opt_printerID)))
      CERR << "warning: printer does not support requested image size" << endl;

    if (EC_Normal != dvi.getPrintHandler().setImageDisplayFormat(opt_columns, opt_rows))
      CERR << "warning: cannot set image display format to columns=" << opt_columns
           << ", rows=" << opt_rows << ", ignoring." << endl;
    if ((opt_filmsize)&&(EC_Normal != dvi.getPrintHandler().setFilmSizeID(opt_filmsize)))
      CERR << "warning: cannot set film size ID to '" << opt_filmsize << "', ignoring." << endl;
    if ((opt_magnification)&&(EC_Normal != dvi.getPrintHandler().setMagnificationType(opt_magnification)))
      CERR << "warning: cannot set magnification type to '" << opt_magnification << "', ignoring." << endl;
    if ((opt_smoothing)&&(EC_Normal != dvi.getPrintHandler().setSmoothingType(opt_smoothing)))
      CERR << "warning: cannot set smoothing type to '" << opt_smoothing << "', ignoring." << endl;
    if ((opt_configuration)&&(EC_Normal != dvi.getPrintHandler().setConfigurationInformation(opt_configuration)))
      CERR << "warning: cannot set configuration information to '" << opt_configuration << "', ignoring." << endl;
    if ((opt_resolution)&&(EC_Normal != dvi.getPrintHandler().setResolutionID(opt_resolution)))
      CERR << "warning: cannot set requested resolution ID to '" << opt_resolution << "', ignoring." << endl;
    if ((opt_border)&&(EC_Normal != dvi.getPrintHandler().setBorderDensity(opt_border)))
      CERR << "warning: cannot set border density to '" << opt_border << "', ignoring." << endl;
    if ((opt_emptyimage)&&(EC_Normal != dvi.getPrintHandler().setEmtpyImageDensity(opt_emptyimage)))
      CERR << "warning: cannot set empty image density to '" << opt_emptyimage << "', ignoring." << endl;
    if ((opt_maxdensity)&&(EC_Normal != dvi.getPrintHandler().setMaxDensity(opt_maxdensity)))
      CERR << "warning: cannot set max density to '" << opt_maxdensity << "', ignoring." << endl;
    if ((opt_mindensity)&&(EC_Normal != dvi.getPrintHandler().setMinDensity(opt_mindensity)))
      CERR << "warning: cannot set min density to '" << opt_mindensity << "', ignoring." << endl;
    if (EC_Normal != dvi.getPrintHandler().setFilmOrientation(opt_filmorientation))
      CERR << "warning: cannot set film orientation, ignoring." << endl;
    if (EC_Normal != dvi.getPrintHandler().setTrim(opt_trim))
      CERR << "warning: cannot set trim, ignoring." << endl;
    if (EC_Normal != dvi.getPrintHandler().setRequestedDecimateCropBehaviour(opt_decimate))
      CERR << "warning: cannot set requested decimate/crop behaviour, ignoring." << endl;
    if ((opt_illumination != (OFCmdUnsignedInt)-1)&&(EC_Normal != dvi.getPrintHandler().setPrintIllumination((Uint16)opt_illumination)))
      CERR << "warning: cannot set illumination to '" << opt_illumination << "', ignoring." << endl;
    if ((opt_reflection != (OFCmdUnsignedInt)-1)&&(EC_Normal != dvi.getPrintHandler().setPrintReflectedAmbientLight((Uint16)opt_reflection)))
      CERR << "warning: cannot set reflected ambient light to '" << opt_reflection << "', ignoring." << endl;

    if ((opt_copies > 0)&&(EC_Normal != dvi.setPrinterNumberOfCopies(opt_copies)))
      CERR << "warning: cannot set film session number of copies to '" << opt_copies << "', ignoring." << endl;
    if ((opt_mediumtype)&&(EC_Normal != dvi.setPrinterMediumType(opt_mediumtype)))
      CERR << "warning: cannot set film session medium type to '" << opt_mediumtype << "', ignoring." << endl;
    if ((opt_destination)&&(EC_Normal != dvi.setPrinterFilmDestination(opt_destination)))
      CERR << "warning: cannot set film destination to '" << opt_destination << "', ignoring." << endl;
    if ((opt_sessionlabel)&&(EC_Normal != dvi.setPrinterFilmSessionLabel(opt_sessionlabel)))
      CERR << "warning: cannot set film session label to '" << opt_sessionlabel << "', ignoring." << endl;
    if ((opt_priority)&&(EC_Normal != dvi.setPrinterPriority(opt_priority)))
      CERR << "warning: cannot set film session print priority to '" << opt_priority << "', ignoring." << endl;
    if ((opt_ownerID)&&(EC_Normal != dvi.setPrinterOwnerID(opt_ownerID)))
      CERR << "warning: cannot set film session owner ID to '" << opt_ownerID << "', ignoring." << endl;
    if ((opt_spool)&&(EC_Normal != dvi.startPrintSpooler()))
      CERR << "warning: unable to start print spooler, ignoring." << endl;

    OFListIterator(char *) first = opt_filenames.begin();
    OFListIterator(char *) last = opt_filenames.end();
    const char *currentImage = NULL;
    const char *currentPState = NULL;
    OFCondition status = EC_Normal;
    void *pixelData = NULL;
    unsigned long width = 0;
    unsigned long height = 0;
    unsigned long bitmapSize = 0;
    double pixelAspectRatio;

    while ((EC_Normal == status)&&(first != last))
    {
      currentImage = *first;
      ++first;
      if (first != last)
      {
        currentPState = *first;
        ++first;
        if (currentPState)
        {
          if (opt_verbose) CERR << "loading image file '" << currentImage << "' with presentation state '" << currentPState << "'" << endl;
          status = dvi.loadPState(currentPState, currentImage);
          if (EC_Normal != status)
          {
            CERR << "error: loading image file '" << currentImage << "' with presentation state '" << currentPState << "' failed." << endl;
            return 10;
          }
        }
        else
        {
          if (opt_verbose) CERR << "loading image file '" << currentImage << "'" << endl;
          status = dvi.loadImage(currentImage);
          if (EC_Normal != status)
          {
            CERR << "error: loading image file '" << currentImage << "' failed." << endl;
            return 10;
          }
        }

        if (opt_plutname)
        {
          if (EC_Normal != dvi.selectDisplayPresentationLUT(opt_plutname))
          CERR << "warning: cannot set requested presentation LUT '" << opt_plutname << "', ignoring." << endl;
        } else {
          // in the case of a Presentation LUT Shape, we set the shape inside
          // the GSPS object to default (corresponding to IDENTITY for MONOCHROME2
          // and INVERSE for MONOCHROME1). This will leave our image data unaltered.
          // The LIN OD shape is only activated in the print handler, not the GSPS.
          if ((opt_LUTshape == 1) || (opt_LUTshape == 2))
          {
            if (dvi.getCurrentPState().setDefaultPresentationLUTShape().bad())
               CERR << "warning: cannot set presentation LUT shape, ignoring." << endl;

            if (opt_LUTshape == 2)
            {
              if (dvi.getPrintHandler().setPresentationLUTShape(DVPSP_lin_od).bad())
              CERR << "warning: cannot set LIN OD presentation LUT shape, ignoring." << endl;
            }
          }
        }

        // save grayscale hardcopy image.
        bitmapSize = dvi.getCurrentPState().getPrintBitmapSize();
        pixelData = new char[bitmapSize];
        if (pixelData)
        {
            if (EC_Normal != dvi.getCurrentPState().getPrintBitmapWidthHeight(width, height))
            {
              CERR << "error: can't determine bitmap size" << endl;
              return 10;
            }
            if (EC_Normal != dvi.getCurrentPState().getPrintBitmap(pixelData, bitmapSize, opt_inverse_plut))
            {
              CERR << "error: can't create print bitmap" << endl;
              return 10;
            }
            pixelAspectRatio = dvi.getCurrentPState().getPrintBitmapPixelAspectRatio();

            if (cmd.findOption("--overlay", 0, OFCommandLine::FOM_First))
            {
                do {
                    const char *fn = NULL;
                    OFCmdUnsignedInt x, y;
                    app.checkValue(cmd.getValue(fn));
                    app.checkValue(cmd.getValue(x));
                    app.checkValue(cmd.getValue(y));
                    if (fn != NULL)
                        addOverlay(fn, x, y, (Uint16 *)pixelData, width, height, (unsigned int)opt_ovl_graylevel);
                } while (cmd.findOption("--overlay", 0, OFCommandLine::FOM_Next));
            }

            if (opt_verbose) CERR << "writing DICOM grayscale hardcopy image to database." << endl;
            if (EC_Normal != dvi.saveHardcopyGrayscaleImage(pixelData, width, height, pixelAspectRatio))
            {
              CERR << "error during creation of DICOM grayscale hardcopy image file" << endl;
              return 10;
            }
            delete[] (char *)pixelData;
        } else {
          CERR << "out of memory error: cannot allocate print bitmap" << endl;
          return 10;
        }
      } else {
        CERR << "internal error - odd number of filenames" << endl;
        return 10;
      }
    }

    // set annotations
    if (status == EC_Normal)
    {
      if (opt_annotation)
      {
        if (dvi.getTargetPrinterSupportsAnnotation(currentPrinter))
        {
          dvi.setActiveAnnotation(OFTrue);
          dvi.setPrependDateTime(opt_annotationDatetime);
          dvi.setPrependPrinterName(opt_annotationPrinter);
          dvi.setPrependLighting(opt_annotationIllumination);
          dvi.setAnnotationText(opt_annotationString);
        } else {
          CERR << "warning: printer '" << currentPrinter << "' does not support annotations, ignoring." << endl;
          dvi.setActiveAnnotation(OFFalse);
        }
      } else {
        dvi.setActiveAnnotation(OFFalse);
      }
    }

    if (status == EC_Normal)
    {
      size_t numImages = dvi.getPrintHandler().getNumberOfImages();
      for (size_t i=0; i<numImages; i++)
      {
        if ((opt_img_polarity)&&(EC_Normal != dvi.getPrintHandler().setImagePolarity(i, opt_img_polarity)))
          CERR << "warning: cannot set polarity for image #" << i+1 << " (of " << numImages << ") to '" << opt_img_polarity << "', ignoring." << endl;
        if ((opt_img_request_size)&&(EC_Normal != dvi.getPrintHandler().setImageRequestedSize(i, opt_img_request_size)))
          CERR << "warning: cannot set requested size for image #" << i+1 << " (of " << numImages << ") to '" << opt_img_request_size << "', ignoring." << endl;
        if ((opt_img_magnification)&&(EC_Normal != dvi.getPrintHandler().setImageMagnificationType(i, opt_img_magnification)))
          CERR << "warning: cannot set magnification type for image #" << i+1 << " (of " << numImages << ") to '" << opt_img_magnification << "', ignoring." << endl;
        if ((opt_img_smoothing)&&(EC_Normal != dvi.getPrintHandler().setImageSmoothingType(i, opt_img_smoothing)))
          CERR << "warning: cannot set smoothing type for image #" << i+1 << " (of " << numImages << ") to '" << opt_img_smoothing << "', ignoring." << endl;
        if ((opt_img_configuration)&&(EC_Normal != dvi.getPrintHandler().setImageConfigurationInformation(i, opt_img_configuration)))
          CERR << "warning: cannot set configuration information for image #" << i+1 << " (of " << numImages << ") to '" << opt_img_configuration << "', ignoring." << endl;
      }
      if ((numImages > 0)&&(! opt_spool))
      {
        // no need to do this manually if we are spooling - spoolPrintJob() will do this anyway.
        if (opt_verbose) CERR << "writing DICOM stored print object to database." << endl;
        if (EC_Normal != dvi.saveStoredPrint(dvi.getTargetPrinterSupportsRequestedImageSize(opt_printerID)))
        {
          CERR << "error during creation of DICOM stored print object" << endl;
        }
      }
    }

    if ((status == EC_Normal) && opt_spool)
    {
      if (currentPrinter)
      {
        if (opt_verbose) CERR << "spooling print job to printer '" << currentPrinter << "'" << endl;
        if (EC_Normal != dvi.spoolPrintJob())
          CERR << "warning: unable to spool print job to printer '" << currentPrinter << "', ignoring." << endl;
      } else {
          CERR << "warning: no printer (undefined in config file?), cannot spool print job." << endl;
      }
    }

    if ((opt_spool)&&(EC_Normal != dvi.terminatePrintSpooler()))
      CERR << "warning: unable to stop print spooler, ignoring." << endl;

#ifdef DEBUG
    dcmDataDict.clear();  /* useful for debugging with dmalloc */
#endif

    return (status != EC_Normal);
}


/*
 * CVS/RCS Log:
 * $Log: dcmpsprt.cc,v $
 * Revision 1.36  2005/12/08 15:46:09  meichel
 * Changed include path schema for all DCMTK header files
 *
 * Revision 1.35  2005/11/28 15:29:05  meichel
 * File dcdebug.h is not included by any other header file in the toolkit
 *   anymore, to minimize the risk of name clashes of macro debug().
 *
 * Revision 1.34  2004/02/04 15:44:38  joergr
 * Removed acknowledgements with e-mail addresses from CVS log.
 *
 * Revision 1.33  2003/12/01 16:54:44  meichel
 * Fixed handling of LIN OD LUT Shape
 *
 * Revision 1.32  2002/11/27 15:47:54  meichel
 * Adapted module dcmpstat to use of new header file ofstdinc.h
 *
 * Revision 1.31  2002/11/26 08:44:28  meichel
 * Replaced all includes for "zlib.h" with <zlib.h>
 *   to avoid inclusion of zlib.h in the makefile dependencies.
 *
 * Revision 1.30  2002/09/23 18:26:08  joergr
 * Added new command line option "--version" which prints the name and version
 * number of external libraries used (incl. preparation for future support of
 * 'config.guess' host identifiers).
 *
 * Revision 1.29  2002/04/16 14:01:27  joergr
 * Added configurable support for C++ ANSI standard includes (e.g. streams).
 *
 * Revision 1.28  2001/11/09 16:06:05  joergr
 * Renamed some of the getValue/getParam methods to avoid ambiguities reported
 * by certain compilers.
 *
 * Revision 1.27  2001/09/28 13:47:38  joergr
 * Added check whether ios::nocreate exists.
 *
 * Revision 1.26  2001/09/26 15:36:03  meichel
 * Adapted dcmpstat to class OFCondition
 *
 * Revision 1.25  2001/06/07 14:34:09  joergr
 * Removed comment.
 *
 * Revision 1.23  2001/06/01 15:50:09  meichel
 * Updated copyright header
 *
 * Revision 1.22  2000/06/19 16:29:05  meichel
 * Added options for session printing and LIN OD to print tools, fixed
 *   pixel aspect ratio related bug.
 *
 * Revision 1.21  2000/06/14 14:24:39  joergr
 * Added new command line option allowing to add a PBM file as an overlay to
 * the hardcopy grayscale image (very preliminary support, only "P1" files
 * without comments).
 *
 * Revision 1.20  2000/06/14 11:30:15  joergr
 * Added methods to access the attributes Polarity and Requested Image Size.
 *
 * Revision 1.19  2000/06/09 10:19:56  joergr
 * Added support for rendering inverse presentation LUT into print bitmaps.
 *
 * Revision 1.18  2000/05/30 14:01:59  joergr
 * Renamed GrayscaleHardcopy to HardcopyGrayscale (which is the correct term
 * according to the DICOM standard).
 *
 * Revision 1.17  2000/05/03 14:27:27  meichel
 * Updated dcmpstat apps for changes in dcmimgle.
 *
 * Revision 1.16  2000/03/08 16:28:42  meichel
 * Updated copyright header.
 *
 * Revision 1.15  2000/03/07 16:18:10  joergr
 * Removed type specifier 'const' to make Sun CC 2.0.1 happy.
 *
 * Revision 1.14  2000/03/06 18:21:46  joergr
 * Avoid empty statement in the body of if-statements (MSVC6 reports warnings).
 *
 * Revision 1.13  2000/03/03 14:13:27  meichel
 * Implemented library support for redirecting error messages into memory
 *   instead of printing them to stdout/stderr for GUI applications.
 *
 * Revision 1.12  2000/02/01 11:54:36  meichel
 * Avoiding to include <stdlib.h> as extern "C" on Borland C++ Builder 4,
 *   workaround for bug in compiler header files.
 *
 * Revision 1.11  1999/10/28 08:18:32  meichel
 * Added options for setting Max Density and Min Density from command line
 *
 * Revision 1.10  1999/10/19 14:45:27  meichel
 * added support for the Basic Annotation Box SOP Class
 *   as well as access methods for Max Density and Min Density.
 *
 * Revision 1.9  1999/10/07 17:21:42  meichel
 * Reworked management of Presentation LUTs in order to create tighter
 *   coupling between Softcopy and Print.
 *
 * Revision 1.8  1999/09/24 15:24:25  meichel
 * Added support for CP 173 (Presentation LUT clarifications)
 *
 * Revision 1.7  1999/09/23 17:37:09  meichel
 * Added support for Basic Film Session options to dcmpstat print code.
 *
 * Revision 1.6  1999/09/15 17:42:56  meichel
 * Implemented print job dispatcher code for dcmpstat, adapted dcmprtsv
 *   and dcmpsprt applications.
 *
 * Revision 1.5  1999/09/14 18:12:29  meichel
 * Removed unneeded debug output from dcmpsprt
 *
 * Revision 1.4  1999/09/13 15:18:45  meichel
 * Adapted dcmpsprt to print API enhancements
 *
 * Revision 1.3  1999/09/08 16:49:22  meichel
 * Added print API method declarations
 *
 * Revision 1.2  1999/09/01 16:14:11  meichel
 * Completed printer characteristics dump routine
 *
 * Revision 1.1  1999/08/31 16:54:40  meichel
 * Added new sample application that allows to create simple print jobs.
 *
 *
 */
