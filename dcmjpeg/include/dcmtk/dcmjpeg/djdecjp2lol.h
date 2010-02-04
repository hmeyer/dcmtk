/*
 *
 *  Module:  dcmjpeg
 *
 *  Author:  Henning Meyer
 *
 *  Purpose: Codec class for decoding JPEG2000 Lossless
 *
 */

#ifndef DJDECJP2LOL_H
#define DJDECJP2LOL_H

#include "dcmtk/config/osconfig.h"
#include "dcmtk/dcmjpeg/djcodecd.h" /* for class DJCodecDecoder */


/** Decoder class for JPEG2000 Lossless
 */
class DJDecoderJPEG2000Lossless : public DJCodecDecoder
{
public: 

  /// default constructor
  DJDecoderJPEG2000Lossless();

  /// destructor
  virtual ~DJDecoderJPEG2000Lossless();

  /** returns the transfer syntax that this particular codec
   *  is able to encode and decode.
   *  @return supported transfer syntax
   */
  virtual E_TransferSyntax supportedTransferSyntax() const;

private:

  /** creates an instance of the compression library to be used for decoding.
   *  @param toRepParam representation parameter passed to decode()
   *  @param cp codec parameter passed to decode()
   *  @param bitsPerSample bits per sample for the image data
   *  @param isYBR flag indicating whether DICOM photometric interpretation is YCbCr
   *  @return pointer to newly allocated decoder object
   */
  virtual DJDecoder *createDecoderInstance(
    const DcmRepresentationParameter * toRepParam,
    const DJCodecParameter *cp,
    Uint8 bitsPerSample,
    OFBool isYBR) const;
    
};

#endif