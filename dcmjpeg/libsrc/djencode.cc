/*
 *
 *  Copyright (C) 1997-2005, OFFIS
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
 *  Purpose: singleton class that registers encoders for all supported JPEG processes.
 *
 *  Last Update:      $Author: meichel $
 *  Update Date:      $Date: 2005/12/08 15:43:45 $
 *  Source File:      $Source: /share/dicom/cvs-depot/dcmtk/dcmjpeg/libsrc/djencode.cc,v $
 *  CVS/RCS Revision: $Revision: 1.7 $
 *  Status:           $State: Exp $
 *
 *  CVS/RCS Log at end of file
 *
 */

#include "dcmtk/config/osconfig.h"
#include "dcmtk/dcmjpeg/djencode.h"

#include "dcmtk/dcmdata/dccodec.h"  /* for DcmCodecStruct */
#include "dcmtk/dcmjpeg/djencbas.h"
#include "dcmtk/dcmjpeg/djencext.h"
#include "dcmtk/dcmjpeg/djencsps.h"
#include "dcmtk/dcmjpeg/djencpro.h"
#include "dcmtk/dcmjpeg/djencsv1.h"
#include "dcmtk/dcmjpeg/djenclol.h"
#include "dcmtk/dcmjpeg/djcparam.h"

// initialization of static members
OFBool DJEncoderRegistration::registered                  = OFFalse;
DJCodecParameter *DJEncoderRegistration::cp               = NULL;
DJEncoderBaseline *DJEncoderRegistration::encbas          = NULL;
DJEncoderExtended *DJEncoderRegistration::encext          = NULL;
DJEncoderSpectralSelection *DJEncoderRegistration::encsps = NULL;
DJEncoderProgressive *DJEncoderRegistration::encpro       = NULL;
DJEncoderP14SV1 *DJEncoderRegistration::encsv1            = NULL;
DJEncoderLossless *DJEncoderRegistration::enclol          = NULL;

void DJEncoderRegistration::registerCodecs(
    E_CompressionColorSpaceConversion pCompressionCSConversion,
    E_UIDCreation pCreateSOPInstanceUID,
    OFBool pVerbose,
    OFBool pOptimizeHuffman,
    int pSmoothingFactor,
    int pForcedBitDepth,
    Uint32 pFragmentSize,
    OFBool pCreateOffsetTable,
    E_SubSampling pSampleFactors,
    OFBool pWriteYBR422,
    OFBool pConvertToSC,
    unsigned long pWindowType,
    unsigned long pWindowParameter,
    double pVoiCenter,
    double pVoiWidth,
    unsigned long pRoiLeft,
    unsigned long pRoiTop,
    unsigned long pRoiWidth,
    unsigned long pRoiHeight,
    OFBool pUsePixelValues,
    OFBool pUseModalityRescale,
    OFBool pAcceptWrongPaletteTags,
    OFBool pAcrNemaCompatibility,
    OFBool pRealLossless)
{
  if (! registered)
  {
    cp = new DJCodecParameter(
      pCompressionCSConversion,
      EDC_photometricInterpretation,  // not relevant, used for decompression only
      pCreateSOPInstanceUID,
      EPC_default, // not relevant, used for decompression only
      pVerbose,
      pOptimizeHuffman,
      pSmoothingFactor,
      pForcedBitDepth,
      pFragmentSize,
      pCreateOffsetTable,
      pSampleFactors,
      pWriteYBR422,
      pConvertToSC,
      pWindowType,
      pWindowParameter,
      pVoiCenter,
      pVoiWidth,
      pRoiLeft,
      pRoiTop,
      pRoiWidth,
      pRoiHeight,
      pUsePixelValues,
      pUseModalityRescale,
      pAcceptWrongPaletteTags,
      pAcrNemaCompatibility,
      pRealLossless);
    if (cp)
    {
      // baseline JPEG
      encbas = new DJEncoderBaseline();
      if (encbas) DcmCodecList::registerCodec(encbas, NULL, cp);

      // extended JPEG
      encext = new DJEncoderExtended();
      if (encext) DcmCodecList::registerCodec(encext, NULL, cp);

      // spectral selection JPEG
      encsps = new DJEncoderSpectralSelection();
      if (encsps) DcmCodecList::registerCodec(encsps, NULL, cp);

      // progressive JPEG
      encpro = new DJEncoderProgressive();
      if (encpro) DcmCodecList::registerCodec(encpro, NULL, cp);

      // lossless SV1 JPEG
      encsv1 = new DJEncoderP14SV1();
      if (encsv1) DcmCodecList::registerCodec(encsv1, NULL, cp);

      // lossless JPEG
      enclol = new DJEncoderLossless();
      if (enclol) DcmCodecList::registerCodec(enclol, NULL, cp);

      registered = OFTrue;
    }
  }
}

void DJEncoderRegistration::cleanup()
{
  if (registered)
  {
    DcmCodecList::deregisterCodec(encbas);
    delete encbas;
    DcmCodecList::deregisterCodec(encext);
    delete encext;
    DcmCodecList::deregisterCodec(encsps);
    delete encsps;
    DcmCodecList::deregisterCodec(encpro);
    delete encpro;
    DcmCodecList::deregisterCodec(encsv1);
    delete encsv1;
    DcmCodecList::deregisterCodec(enclol);
    delete enclol;
    delete cp;
    registered = OFFalse;
#ifdef DEBUG
    // not needed but useful for debugging purposes
    encbas = NULL;
    encext = NULL;
    encsps = NULL;
    encpro = NULL;
    encsv1 = NULL;
    enclol = NULL;
    cp     = NULL;
#endif

  }
}


/*
 * CVS/RCS Log
 * $Log: djencode.cc,v $
 * Revision 1.7  2005/12/08 15:43:45  meichel
 * Changed include path schema for all DCMTK header files
 *
 * Revision 1.6  2005/11/29 15:56:55  onken
 * Added commandline options --accept-acr-nema and --accept-palettes
 * (same as in dcm2pnm) to dcmcjpeg and extended dcmjpeg to support
 * these options. Thanks to Gilles Mevel for suggestion.
 *
 * Revision 1.4  2005/11/29 08:48:45  onken
 * Added support for "true" lossless compression in dcmjpeg, that doesn't
 *   use dcmimage classes, but compresses raw pixel data (8 and 16 bit) to
 *   avoid losses in quality caused by color space conversions or modality
 *   transformations etc.
 * Corresponding commandline option in dcmcjpeg (new default)
 *
 * Revision 1.3  2001/12/04 17:10:20  meichel
 * Fixed codec registration: flag registered was never set to true
 *
 * Revision 1.2  2001/11/19 15:13:32  meichel
 * Introduced verbose mode in module dcmjpeg. If enabled, warning
 *   messages from the IJG library are printed on ofConsole, otherwise
 *   the library remains quiet.
 *
 * Revision 1.1  2001/11/13 15:58:32  meichel
 * Initial release of module dcmjpeg
 *
 *
 */
