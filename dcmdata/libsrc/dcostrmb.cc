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
 *  Author:  Marco Eichelberg
 *
 *  Purpose: DcmOutputBufferStream and related classes,
 *    implements output to blocks of memory as needed in the dcmnet module.
 *
 *  Last Update:      $Author: meichel $
 *  Update Date:      $Date: 2005/12/08 15:41:21 $
 *  CVS/RCS Revision: $Revision: 1.4 $
 *  Status:           $State: Exp $
 *
 *  CVS/RCS Log at end of file
 *
 */

#include "dcmtk/config/osconfig.h"
#include "dcmtk/dcmdata/dcostrmb.h"
#include "dcmtk/ofstd/ofconsol.h"
#include "dcmtk/dcmdata/dcerror.h"


DcmBufferConsumer::DcmBufferConsumer(void *buf, Uint32 bufLen)
: DcmConsumer()
, buffer_(OFstatic_cast(unsigned char *, buf))
, bufSize_(bufLen)
, filled_(0)
, status_(EC_Normal)
{
  if ((buffer_ == NULL) || (bufSize_ == 0)) status_ = EC_IllegalCall;
}

DcmBufferConsumer::~DcmBufferConsumer()
{
}

OFBool DcmBufferConsumer::good() const
{
  return status_.good();
}

OFCondition DcmBufferConsumer::status() const
{
  return status_;
}

OFBool DcmBufferConsumer::isFlushed() const
{
  return (filled_ == 0);
}

Uint32 DcmBufferConsumer::avail() const
{
  return bufSize_ - filled_;
}

Uint32 DcmBufferConsumer::write(const void *buf, Uint32 buflen)
{
  Uint32 result = 0;
  if (status_.good() && buf && buflen)
  {
    result = bufSize_ - filled_;
    if (result > buflen) result = buflen;
    memcpy(buffer_+ filled_, buf, OFstatic_cast(size_t, result));
    filled_ += result;
  }
  return result;
}

void DcmBufferConsumer::flush()
{
  // nothing to flush
}

void DcmBufferConsumer::flushBuffer(void *& buffer, Uint32& length)
{
  buffer = buffer_;
  length = filled_;
  filled_ = 0;
}

/* ======================================================================= */

DcmOutputBufferStream::DcmOutputBufferStream(void *buf, Uint32 bufLen)
: DcmOutputStream(&consumer_) // safe because DcmOutputStream only stores pointer
, consumer_(buf, bufLen)
{
}

DcmOutputBufferStream::~DcmOutputBufferStream()
{
#ifdef DEBUG
  if (! isFlushed())
  {
    ofConsole.lockCerr() << "Warning: closing unflushed DcmOutputBufferStream, loss of data!" << endl;
    ofConsole.unlockCerr();
  }
#endif
}

void DcmOutputBufferStream::flushBuffer(void *& buffer, Uint32& length)
{
  consumer_.flushBuffer(buffer, length);
}


/*
 * CVS/RCS Log:
 * $Log: dcostrmb.cc,v $
 * Revision 1.4  2005/12/08 15:41:21  meichel
 * Changed include path schema for all DCMTK header files
 *
 * Revision 1.3  2004/02/04 16:36:47  joergr
 * Adapted type casts to new-style typecast operators defined in ofcast.h.
 *
 * Revision 1.2  2002/09/19 08:32:28  joergr
 * Added explicit type casts to keep Sun CC 2.0.1 quiet.
 *
 * Revision 1.1  2002/08/27 16:55:53  meichel
 * Initial release of new DICOM I/O stream classes that add support for stream
 *   compression (deflated little endian explicit VR transfer syntax)
 *
 *
 */
