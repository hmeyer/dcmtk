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
 *  Author:  Marco Eichelberg, Norbert Olges
 *
 *  Purpose: Codec class for encoding JPEG Lossless (8/12/16-bit)
 *
 *  Last Update:      $Author: meichel $
 *  Update Date:      $Date: 2005/12/08 15:43:44 $
 *  Source File:      $Source: /share/dicom/cvs-depot/dcmtk/dcmjpeg/libsrc/djenclol.cc,v $
 *  CVS/RCS Revision: $Revision: 1.2 $
 *  Status:           $State: Exp $
 *
 *  CVS/RCS Log at end of file
 *
 */

#include "dcmtk/config/osconfig.h"
#include "dcmtk/dcmjpeg/djenclol.h"
#include "dcmtk/dcmjpeg/djcparam.h"
#include "dcmtk/dcmjpeg/djrplol.h"
#include "dcmtk/dcmjpeg/djeijg8.h"
#include "dcmtk/dcmjpeg/djeijg12.h"
#include "dcmtk/dcmjpeg/djeijg16.h"


DJEncoderLossless::DJEncoderLossless()
: DJCodecEncoder()
{
}


DJEncoderLossless::~DJEncoderLossless()
{
}


E_TransferSyntax DJEncoderLossless::supportedTransferSyntax() const
{
  return EXS_JPEGProcess14TransferSyntax;
}


OFBool DJEncoderLossless::isLosslessProcess() const
{
  return OFTrue;
}


void DJEncoderLossless::createDerivationDescription(
  const DcmRepresentationParameter * toRepParam,
  const DJCodecParameter * /* cp */ ,
  Uint8 /* bitsPerSample */ ,
  double ratio,
  OFString& derivationDescription) const
{
  DJ_RPLossless defaultRP;
  const DJ_RPLossless *rp = toRepParam ? (const DJ_RPLossless *)toRepParam : &defaultRP ;
  char buf[64];
 
  derivationDescription =  "Lossless JPEG compression, selection value ";
  sprintf(buf, "%u", rp->getPrediction());
  derivationDescription += buf;
  derivationDescription += ", point transform ";
  sprintf(buf, "%u", rp->getPointTransformation());
  derivationDescription += buf;
  derivationDescription += ", compression ratio ";
  appendCompressionRatio(derivationDescription, ratio);
}


DJEncoder *DJEncoderLossless::createEncoderInstance(
    const DcmRepresentationParameter * toRepParam,
    const DJCodecParameter *cp,
    Uint8 bitsPerSample) const
{
  DJ_RPLossless defaultRP;
  const DJ_RPLossless *rp = toRepParam ? (const DJ_RPLossless *)toRepParam : &defaultRP ;
  DJEncoder * result = NULL;
  if (bitsPerSample > 12)
    result = new DJCompressIJG16Bit(*cp, EJM_lossless, rp->getPrediction(), rp->getPointTransformation());
  else if (bitsPerSample > 8)
    result = new DJCompressIJG12Bit(*cp, EJM_lossless, rp->getPrediction(), rp->getPointTransformation());
  else 
    result = new DJCompressIJG8Bit(*cp, EJM_lossless, rp->getPrediction(), rp->getPointTransformation());
  return result;  
}


/*
 * CVS/RCS Log
 * $Log: djenclol.cc,v $
 * Revision 1.2  2005/12/08 15:43:44  meichel
 * Changed include path schema for all DCMTK header files
 *
 * Revision 1.1  2001/11/13 15:58:32  meichel
 * Initial release of module dcmjpeg
 *
 *
 */
