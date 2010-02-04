/*
 *
 *  Module:  dcmjpeg
 *
 *  Author:  Henning Meyer
 *
 *  Purpose: Codec class for decoding JPEG2000 Lossless
 *
 */

#include "dcmtk/config/osconfig.h"
#include "dcmtk/dcmjpeg/djdecjp2lol.h"
#include "dcmtk/dcmjpeg/djcparam.h"
#include "dcmtk/dcmjpeg/djdjp2.h"

#include <iostream>
using namespace std;

DJDecoderJPEG2000Lossless::DJDecoderJPEG2000Lossless()
: DJCodecDecoder()
{
  cerr << __FUNCTION__ << endl;
}


DJDecoderJPEG2000Lossless::~DJDecoderJPEG2000Lossless()
{
  cerr << __FUNCTION__ << endl;
}


E_TransferSyntax DJDecoderJPEG2000Lossless::supportedTransferSyntax() const
{
  cerr << __FUNCTION__ << endl;
  return EXS_JPEG2000LosslessOnly;
}


DJDecoder *DJDecoderJPEG2000Lossless::createDecoderInstance(
    const DcmRepresentationParameter * /* toRepParam */,
    const DJCodecParameter *cp,
    Uint8 bitsPerSample,
    OFBool isYBR) const
{
  cerr << __FUNCTION__ << endl;
  return new DJDecompressJPEG2000( *cp, isYBR, bitsPerSample );
}