/*
 *
 *  Copyright (C) 2003, OFFIS
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
 *  Update Date:      $Date: 2005/12/08 15:43:24 $
 *  CVS/RCS Revision: $Revision: 1.2 $
 *  Status:           $State: Exp $
 *
 *  CVS/RCS Log at end of file
 *
 */


#include "dcmtk/config/osconfig.h"    /* make sure OS specific configuration is included first */

#include "dcmtk/dcmjpeg/ddpiimpl.h"
#include "dcmtk/dcmimgle/dcmimage.h"    /* for class DicomImage */
#include "dcmtk/dcmimgle/discalet.h"    /* for direct image scaling */

#include "dcmtk/ofstd/ofcast.h"


/*------------------*
 *  implementation  *
 *------------------*/

// constructor
DicomDirImageImplementation::DicomDirImageImplementation()
{
}


// destructor
DicomDirImageImplementation::~DicomDirImageImplementation()
{
}


// scale pixel data
OFBool DicomDirImageImplementation::scaleData(const Uint8 *srcData,
                                              const unsigned int srcWidth,
                                              const unsigned int srcHeight,
                                              Uint8 *dstData,
                                              const unsigned int dstWidth,
                                              const unsigned int dstHeight) const
{
    OFBool result = OFFalse;
    /* check parameters (at least the pointers) */
    if ((srcData != NULL) && (dstData != NULL))
    {
        DiScaleTemplate<Uint8> scale(1, OFstatic_cast(Uint16, srcWidth), OFstatic_cast(Uint16, srcHeight),
                                        OFstatic_cast(Uint16, dstWidth), OFstatic_cast(Uint16, dstHeight), 1);
        scale.scaleData(OFstatic_cast(const Uint8 **, &srcData), &dstData, 1 /* interpolate */);
        result = OFTrue;
    }
    return result;
}


// get scaled pixel data from DICOM image (monochrome only)
OFBool DicomDirImageImplementation::scaleImage(DcmItem *dataset,
                                               Uint8 *pixel,
                                               const unsigned long count,
                                               const unsigned long frame,
                                               const unsigned int width,
                                               const unsigned int height) const
{
    OFBool result = OFFalse;
    /* check parameters (at least the pointers) */
    if ((dataset != NULL) && (pixel != NULL) && (frame > 0))
    {
        /* open referenced image */
        DicomImage *image = new DicomImage(dataset, EXS_Unknown, 0 /*flags*/, frame - 1 /*fstart*/, 1 /*fcount*/);
        if ((image != NULL) && (image->getStatus() == EIS_Normal))
        {
            /* check if image is monochrome */
            if (!image->isMonochrome())
            {
                /* ... if not create one */
                DicomImage *mono = image->createMonochromeImage();
                /* replace image by monochrome one */
                delete image;
                image = mono;
            }
            if (image != NULL)
            {
                /* create icon */
                DicomImage *scaled = image->createScaledImage(OFstatic_cast(unsigned long, width),
                                                              OFstatic_cast(unsigned long, height), 1 /*interpolate*/);
                if (scaled != NULL)
                {
                    /* set VOI window */
                    if (!scaled->setWindow(0))
                        scaled->setMinMaxWindow();
                    /* get pixel data */
                    void *data = OFstatic_cast(void *, pixel);
                    if (scaled->getOutputData(data, count, 8))
                        result = OFTrue;
                    delete scaled;
                }
            }
        }
        delete image;
    }
    return result;
}


/*
 *  CVS/RCS Log:
 *  $Log: ddpiimpl.cc,v $
 *  Revision 1.2  2005/12/08 15:43:24  meichel
 *  Changed include path schema for all DCMTK header files
 *
 *  Revision 1.1  2003/08/12 13:14:54  joergr
 *  Added plugable image support for the new DICOMDIR class.
 *
 *
 *
 */
