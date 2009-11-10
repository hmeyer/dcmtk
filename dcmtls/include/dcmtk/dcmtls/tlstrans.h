/*
 *
 *  Copyright (C) 1998-2005, OFFIS
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
 *  Module: dcmnet
 *
 *  Author: Marco Eichelberg
 *
 *  Purpose:
 *    classes: DcmTransportConnection
 *
 *  Last Update:      $Author: meichel $
 *  Update Date:      $Date: 2005/12/08 16:05:39 $
 *  CVS/RCS Revision: $Revision: 1.6 $
 *  Status:           $State: Exp $
 *
 *  CVS/RCS Log at end of file
 *
 */

#ifndef TLSTRANS_H
#define TLSTRANS_H

#include "dcmtk/config/osconfig.h"    /* make sure OS specific configuration is included first */
#include "dcmtk/dcmnet/dcmtrans.h"    /* for DcmTransportConnection */
#include "dcmtk/ofstd/ofstream.h"    /* for ostream */

#ifdef WITH_OPENSSL

BEGIN_EXTERN_C
#include <openssl/ssl.h>
END_EXTERN_C


/** this class represents a TLS (Transport Layer Security) V1 based secure
 *  transport connection.
 */
class DcmTLSConnection: public DcmTransportConnection
{
public:

  /** constructor.
   *  @param openSocket TCP/IP socket to be used for the transport connection.
   *    the connection must already be establised on socket level. This object
   *    takes over control of the socket.
   *  @param newTLSConnection pointer to intialized OpenSSL connection object
   *    to be used for this connection.
   */
  DcmTLSConnection(int openSocket, SSL *newTLSConnection);

  /** destructor
   */
  virtual ~DcmTLSConnection();

  /** performs server side handshake on established socket.
   *  This function is used to establish a secure transport connection
   *  over the established TCP connection.
   *  @return TCS_ok if successful, an error code otherwise.
   */
  virtual DcmTransportLayerStatus serverSideHandshake();

  /** performs client side handshake on established socket.
   *  This function is used to establish a secure transport connection
   *  over the established TCP connection.
   *  @return TCS_ok if successful, an error code otherwise.
   */
  virtual DcmTransportLayerStatus clientSideHandshake();

  /** performs a re-negotiation of the connection with different
   *  connection parameters. Used to change the parameters of the
   *  secure transport connection.
   *  @param newSuite string identifying the ciphersuite to be negotiated.
   *  @return TCS_ok if successful, an error code otherwise.
   */
  virtual DcmTransportLayerStatus renegotiate(const char *newSuite);

  /** attempts to read nbyte bytes from the transport connection and
   *  writes them into the given buffer.
   *  @param buf buffer
   *  @param nbyte number of bytes to read
   *  @return number of bytes read, negative number if unsuccessful.
   */
  virtual ssize_t read(void *buf, size_t nbyte);

  /** attempts to write nbyte bytes from the given buffer
   *  to the transport connection.
   *  @param buf buffer
   *  @param nbyte number of bytes to write
   *  @return number of bytes written, negative number if unsuccessful.
   */
  virtual ssize_t write(void *buf, size_t nbyte);

  /** Closes the transport connection. If a secure connection
   *  is used, a closure alert is sent before the connection
   *  is closed.
   */
  virtual void close();

  /** returns the size in bytes of the peer certificate of a secure connection.
   *  @return peer certificate length in bytes
   */
  virtual unsigned long getPeerCertificateLength();

  /* copies the peer certificate of a secure connection into a buffer
   * specified by the caller. If the buffer is too small to hold the
   * certificate, nothing is copied and zero is returned.
   * @param buf buffer into which the certificate is written
   * @param bufLen size of the buffer in bytes
   * @return number of bytes written, always less or equal bufLen.
   */
  virtual unsigned long getPeerCertificate(void *buf, unsigned long bufLen);

  /** checks if data is available to be read on the transport connection.
   *  @param timeout maximum number of seconds to wait if no data is available.
   *     If this parameter is 0, the function does not block.
   *  @returns OFTrue if data is available, OFFalse otherwise.
   */
  virtual OFBool networkDataAvailable(int timeout);

  /** returns OFTrue if this connection is a transparent TCP connection,
   *  OFFalse if the connection is a secure connection.
   */
  virtual OFBool isTransparentConnection();

  /** prints the characteristics of the current connection
   *  on the given output stream.
   *  @param out output stream
   */
  virtual void dumpConnectionParameters(ostream &out);

  /** returns an error string for a given error code.
   *  @param code error code
   *  @return description for error code
   */
  virtual const char *errorString(DcmTransportLayerStatus code);

private:

  /// private undefined copy constructor
  DcmTLSConnection(const DcmTLSConnection&);

  /// private undefined assignment operator
  DcmTLSConnection& operator=(const DcmTLSConnection&);

  /// pointer to the TLS connection structure used by the OpenSSL library
  SSL *tlsConnection;

  /// last error code returned by the OpenSSL library
  unsigned long lastError;
};

#endif /* WITH_OPENSSL */

#endif

/*
 *  $Log: tlstrans.h,v $
 *  Revision 1.6  2005/12/08 16:05:39  meichel
 *  Changed include path schema for all DCMTK header files
 *
 *  Revision 1.5  2003/12/05 10:38:55  joergr
 *  Removed leading underscore characters from preprocessor symbols (reserved
 *  symbols).
 *
 *  Revision 1.4  2003/07/04 13:28:32  meichel
 *  Added include for ofstream.h, to make sure ofstream is correctly defined
 *
 *  Revision 1.3  2001/06/01 15:51:12  meichel
 *  Updated copyright header
 *
 *  Revision 1.2  2000/10/10 12:13:32  meichel
 *  Added routines for printing certificates and connection parameters.
 *
 *  Revision 1.1  2000/08/10 14:50:27  meichel
 *  Added initial OpenSSL support.
 *
 *
 */

