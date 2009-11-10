/*
 *
 *  Copyright (C) 1996-2005, OFFIS
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
 *  Module:  dcmimage
 *
 *  Author:  Joerg Riesmeier
 *
 *  Purpose: DicomYBRPart422Image (Source)
 *
 *  Last Update:      $Author: meichel $
 *  Update Date:      $Date: 2005/12/08 15:42:38 $
 *  CVS/RCS Revision: $Revision: 1.8 $
 *  Status:           $State: Exp $
 *
 *  CVS/RCS Log at end of file
 *
 */


#include "dcmtk/config/osconfig.h"
#include "dcmtk/dcmdata/dctypes.h"

#include "dcmtk/dcmimage/diyp2img.h"
#include "dcmtk/dcmimage/diyp2pxt.h"
#include "dcmtk/dcmimgle/diinpx.h"


/*----------------*
 *  constructors  *
 *----------------*/

DiYBRPart422Image::DiYBRPart422Image(const DiDocument *docu,
                                     const EI_Status status)
  : DiColorImage(docu, status, 2)
{
    if ((Document != NULL) && (InputData != NULL) && (ImageStatus == EIS_Normal))
    {
        switch (InputData->getRepresentation())
        {
            case EPR_Uint8:
                InterData = new DiYBRPart422PixelTemplate<Uint8, Uint8>(Document, InputData, ImageStatus, BitsPerSample);
                break;
            case EPR_Sint8:
                InterData = new DiYBRPart422PixelTemplate<Sint8, Uint8>(Document, InputData, ImageStatus, BitsPerSample);
                break;
            case EPR_Uint16:
                InterData = new DiYBRPart422PixelTemplate<Uint16, Uint16>(Document, InputData, ImageStatus, BitsPerSample);
                break;
            case EPR_Sint16:
                InterData = new DiYBRPart422PixelTemplate<Sint16, Uint16>(Document, InputData, ImageStatus, BitsPerSample);
                break;
            case EPR_Uint32:
                InterData = new DiYBRPart422PixelTemplate<Uint32, Uint32>(Document, InputData, ImageStatus, BitsPerSample);
                break;
            case EPR_Sint32:
                InterData = new DiYBRPart422PixelTemplate<Sint32, Uint32>(Document, InputData, ImageStatus, BitsPerSample);
                break;
        }
        deleteInputData();
        checkInterData();
    }
}


/*--------------*
 *  destructor  *
 *--------------*/

DiYBRPart422Image::~DiYBRPart422Image()
{
}


/*
 *
 * CVS/RCS Log:
 * $Log: diyp2img.cc,v $
 * Revision 1.8  2005/12/08 15:42:38  meichel
 * Changed include path schema for all DCMTK header files
 *
 * Revision 1.7  2003/12/23 10:54:28  joergr
 * Updated copyright header.
 *
 * Revision 1.6  2001/06/01 15:49:37  meichel
 * Updated copyright header
 *
 * Revision 1.5  2000/03/08 16:21:58  meichel
 * Updated copyright header.
 *
 * Revision 1.4  1999/04/28 13:45:30  joergr
 * Corrected some typos, comments and formatting.
 *
 * Revision 1.3  1998/11/27 14:35:30  joergr
 * Added copyright message.
 * Renamed variable 'Status' to 'ImageStatus' because of possible conflicts
 * with X windows systems.
 *
 * Revision 1.2  1998/05/11 14:52:40  joergr
 * Added CVS/RCS header to each file.
 *
 *
 */
