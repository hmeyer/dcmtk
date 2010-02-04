/*
 *
 *  Module:  dcmjpeg
 *
 *  Author:  Henning Meyer
 *
 *  Purpose: decompression routines of the Jasper JPEG2000 library
 *
 */

#include "dcmtk/config/osconfig.h"
#include "dcmtk/dcmjpeg/djdjp2.h"
#include <jasper/jasper.h>
//#include "dcmtk/dcmjpeg/djcparam.h"

//#include "dcmtk/ofstd/ofconsol.h"

//#define INCLUDE_CSTDIO
//#define INCLUDE_CSETJMP
//#include "dcmtk/ofstd/ofstdinc.h"

#include <iostream>
using namespace std;


DJDecompressJPEG2000::DJDecompressJPEG2000(const DJCodecParameter& cp, OFBool isYBR, Uint8 bitsPerSample)
: DJDecoder()
, decompressedColorModel(EPI_Unknown)
/*, cparam(&cp)
, cinfo(NULL)
, suspension(0)
, jsampBuffer(NULL)
, dicomPhotometricInterpretationIsYCbCr(isYBR)
, decompressedColorModel(EPI_Unknown)*/
{
  cerr << __FUNCTION__ << endl;
  jas_init();
}

DJDecompressJPEG2000::~DJDecompressJPEG2000()
{
  cerr << __FUNCTION__ << endl;
  cleanup();
}


OFCondition DJDecompressJPEG2000::init()
{
  cerr << __FUNCTION__ << endl;
  return EC_Normal;
}


void DJDecompressJPEG2000::cleanup()
{
  cerr << __FUNCTION__ << endl;
  jas_cleanup();
}


OFCondition DJDecompressJPEG2000::decode(
  Uint8 *compressedFrameBuffer,
  Uint32 compressedFrameBufferSize,
  Uint8 *uncompressedFrameBuffer,
  Uint32 uncompressedFrameBufferSize,
  OFBool isSigned)
{
  cerr << __FUNCTION__ << ": starting with " << compressedFrameBufferSize << " bytes" << endl;
  jas_stream_t* jp2Stream = jas_stream_memopen( (char*)compressedFrameBuffer, compressedFrameBufferSize );
//  jas_stream_t* outStream = jas_stream_memopen( (char*)uncompressedFrameBuffer, uncompressedFrameBufferSize );
  if (!jp2Stream){// || !outStream) {
    cleanup();
    return EJ_JPEG2000DecoderError;
  }
  char *jp2opt = NULL;
  jas_image_t *image = jpc_decode(jp2Stream, jp2opt);
  jas_stream_close( jp2Stream );
  if (!image) {
    cleanup();
    return EJ_JPEG2000DecoderError;
  }
  cerr << "decoded JPEG2000: " << jas_image_numcmpts(image) << " components" <<  endl;
  if (jas_image_numcmpts(image)!=1) { //only do Grayscale at the moment
    cleanup();
    return EJ_UnsupportedNumberOfComponents;
  }
  jas_image_cmpt_t *cpt = image->cmpts_[0];
  if (cpt->tlx_ != 0 || cpt->tly_ != 0 || cpt->hstep_ != 1 || cpt->vstep_ != 1) {
    cerr << __FILE__ << ":" << __FUNCTION__ << " warning - strange Component:";
    cerr << "tlx:" << cpt->tlx_ << " tly:" << cpt->tly_ 
      << " hstep:" << cpt->hstep_ << " vstep:" << cpt->vstep_ << endl;
  }
  decompressedColorModel = EPI_Monochrome2;
  if (image->cmpts_[0]->sgnd_ && !isSigned ) {
    cleanup();
    return EJ_ConflictingSignedness;
  }
  jas_stream_seek(cpt->stream_, 0, SEEK_SET );
  int numPix = std::min( (jas_image_coord_t) uncompressedFrameBufferSize / cpt->cps_ , cpt->width_ * cpt->height_);

  for(int c = 0; c < numPix; c++) {
    uchar b1 = jas_stream_getc( cpt->stream_ );
    if (b1==EOF) break;
    uchar b2 = jas_stream_getc( cpt->stream_ );
    if (b2==EOF) break;
    *(uncompressedFrameBuffer++) = b2;
    *(uncompressedFrameBuffer++) = b1;
  }
//  jas_stream_copy(outStream, cpt->stream_, -1);
//  jas_stream_seek(outStream, 0, SEEK_SET );

//  jas_stream_close( outStream );
  jas_image_destroy( image );
  return EC_Normal;
}

void DJDecompressJPEG2000::outputMessage() const
{
  cerr << __FUNCTION__ << endl;
/*
  if (cinfo && cparam->isVerbose())
  {
    ofConsole.lockCerr() << "JPEG2000 decoder" << endl;
    ofConsole.unlockCerr();
  }
*/  
}
