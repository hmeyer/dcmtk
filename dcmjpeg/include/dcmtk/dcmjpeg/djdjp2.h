/*
 *
 *  Module:  dcmjpeg
 *
 *  Author:  Henning Meyer
 *
 *  Purpose: decompression routines of the Jasper JPEG2000 library
 *
 */

#ifndef DJDJP2_H
#define DJDJP2_H

#include "dcmtk/config/osconfig.h"
#include "dcmtk/dcmjpeg/djdecabs.h" /* for class DJDecoder */

#include <boost/scoped_ptr.hpp>
using namespace boost;

class DJCodecParameter;

/** this class encapsulates the decompression routines of the
 *  JasPer JPEG library
 */
class DJDecompressJPEG2000 : public DJDecoder
{
public:

  /** constructor
   *  @param cp codec parameters
   *  @param isYBR flag indicating if DICOM photometric interpretation is YCbCr
   */
  DJDecompressJPEG2000(const DJCodecParameter& cp, OFBool isYBR, Uint8 bitsPerSample);

  /// destructor
  virtual ~DJDecompressJPEG2000();

  /** initializes internal object structures.
   *  Must be called before a new frame is decompressed.
   *  @return EC_Normal if successful, an error code otherwise
   */
  virtual OFCondition init();

  /** suspended decompression routine. Decompresses a JPEG frame
   *  until finished or out of data. Can be called with new data
   *  until a frame is complete.
   *  @param compressedFrameBuffer pointer to compressed input data, must not be NULL
   *  @param compressedFrameBufferSize size of buffer, in bytes
   *  @param uncompressedFrameBuffer pointer to uncompressed output data, must not be NULL.
   *     This buffer must not change between multiple decode() calls for a single frame.
   *  @param uncompressedFrameBufferSize size of buffer, in bytes (!)
   *     Buffer must be large enough to contain a complete frame.
   *  @param isSigned OFTrue, if uncompressed pixel data is signed, OFFalse otherwise
   *  @return EC_Normal if successful, EC_Suspend if more data is needed, an error code otherwise.
   */
  virtual OFCondition decode(
    Uint8 *compressedFrameBuffer,
    Uint32 compressedFrameBufferSize,
    Uint8 *uncompressedFrameBuffer,
    Uint32 uncompressedFrameBufferSize,
    OFBool isSigned);

  /** returns the number of bytes per sample that will be written when decoding.
   */
  virtual Uint16 bytesPerSample() const
  {
    return sizeof(Uint16);
  }

  /** after successful compression,
   *  returns the color model of the decompressed image
   */
  virtual EP_Interpretation getDecompressedColorModel() const
  {
    return decompressedColorModel;
  }

  /** callback function used to report warning messages and the like.
   *  Should not be called by user code directly.
   */
  virtual void outputMessage() const;

private:

  /// private undefined copy constructor
  DJDecompressJPEG2000(const DJDecompressJPEG2000&);

  /// private undefined copy assignment operator
  DJDecompressJPEG2000& operator=(const DJDecompressJPEG2000&);

  /// cleans up cinfo structure, called from destructor and error handlers
  void cleanup();
/*
  /// codec parameters
  const DJCodecParameter *cparam;

  /// decompression structure
  jpeg_decompress_struct *cinfo;

  /// position of last suspend
  int suspension;

  /// temporary storage for row buffer during suspension
  void *jsampBuffer;

  /// Flag indicating if DICOM photometric interpretation is YCbCr
  OFBool dicomPhotometricInterpretationIsYCbCr;
*/

  /// color model after decompression
  EP_Interpretation decompressedColorModel;
};

#endif