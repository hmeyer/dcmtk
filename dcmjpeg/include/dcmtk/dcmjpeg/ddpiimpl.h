/*
 *
 *  Copyright (C) 2005, OFFIS
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
 *  Author:  Joerg Riesmeier
 *
 *  Purpose: Implementation of DICOMDIR image support (plugin)
 *
 *  Last Update:      $Author: meichel $
 *  Update Date:      $Date: 2005/12/08 16:59:09 $
 *  Source File:      $Source: /share/dicom/cvs-depot/dcmtk/dcmjpeg/include/dcmtk/dcmjpeg/ddpiimpl.h,v $
 *  CVS/RCS Revision: $Revision: 1.2 $
 *  Status:           $State: Exp $
 *
 *  CVS/RCS Log at end of file
 *
 */


#ifndef DDPIIMPL_H
#define DDPIIMPL_H

#include "dcmtk/config/osconfig.h"

#include "dcmtk/dcmdata/dcddirif.h"


/*---------------------*
 *  class declaration  *
 *---------------------*/

/** Implementation of the plugable image support for the DICOMDIR class
 */
class DicomDirImageImplementation
  : public DicomDirImagePlugin
{
  public:

    /** constructor
     */
    DicomDirImageImplementation();

    /** destructor
     */
    virtual ~DicomDirImageImplementation();

    /** scale given pixel data (monochrome only).
     *  The destination pixel data array needs to be allocated by the caller.
     *  @param srcData source pixel data (byte array)
     *  @param srcWidth width of the source pixel data (in pixels)
     *  @param srcHeight height of the source pixel data (in pixels)
     *  @param dstData destination pixel data (resulting byte array, not NULL)
     *  @param dstWidth width of the scaled pixel data (in pixels)
     *  @param dstHeight height of the scaled pixel data (in pixels)
     *  @return OFTrue if successful, OFFalse otherwise
     */
    virtual OFBool scaleData(const Uint8 *srcData,
                             const unsigned int srcWidth,
                             const unsigned int srcHeight,
                             Uint8 *dstData,
                             const unsigned int dstWidth,
                             const unsigned int dstHeight) const;

    /** get scaled pixel data from DICOM image.
     *  The resulting scaled image (pixel array) is always monochrome.
     *  The resulting pixel data array needs to be allocated by the caller.
     *  @param dataset DICOM dataset in which the DICOM image is stored
     *  @param pixel resulting pixel data array (not NULL)
     *  @param count number of pixels allocated for the resulting array
     *  @param frame index of the frame to be scaled (1..n)
     *  @param width width of the scaled image (in pixels)
     *  @param height height of the scaled image (in pixels)
     *  @return OFTrue if successful, OFFalse otherwise
     */
    virtual OFBool scaleImage(DcmItem *dataset,
                              Uint8 *pixel,
                              const unsigned long count,
                              const unsigned long frame,
                              const unsigned int width,
                              const unsigned int height) const;
};


#endif


/*
 *
 * CVS/RCS Log:
 * $Log: ddpiimpl.h,v $
 * Revision 1.2  2005/12/08 16:59:09  meichel
 * Changed include path schema for all DCMTK header files
 *
 * Revision 1.1  2003/08/12 13:15:27  joergr
 * Added plugable image support for the new DICOMDIR class.
 *
 *
 *
 */
