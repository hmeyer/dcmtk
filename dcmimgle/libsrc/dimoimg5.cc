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
 *  Module:  dcmimgle
 *
 *  Author:  Joerg Riesmeier
 *
 *  Purpose: DicomMonochromeImage (Source, getData 32 bit)
 *
 *  Last Update:      $Author: meichel $
 *  Update Date:      $Date: 2005/12/08 15:42:59 $
 *  CVS/RCS Revision: $Revision: 1.5 $
 *  Status:           $State: Exp $
 *
 *  CVS/RCS Log at end of file
 *
 */


#include "dcmtk/config/osconfig.h"

#include "dcmtk/dcmimgle/dimoimg.h"
#include "dcmtk/dcmimgle/dimoipxt.h"
#include "dcmtk/dcmimgle/dimoopxt.h"
#include "dcmtk/dcmimgle/diutils.h"


void DiMonoImage::getDataUint32(void *buffer,
                                DiDisplayFunction *disp,
                                const int samples,
                                const unsigned long frame,
                                const int bits,
                                const Uint32 low,
                                const Uint32 high)
{
    if (InterData != NULL)
    {
        if (InterData->isPotentiallySigned())
        {
            if (bits <= 8)
                OutputData = new DiMonoOutputPixelTemplate<Uint32, Sint32, Uint8>(buffer, InterData, Overlays, VoiLutData,
                    PresLutData, disp, WindowCenter, WindowWidth, low, high, Columns, Rows, frame, NumberOfFrames, samples > 1);
            else if (bits <= 16)
                OutputData = new DiMonoOutputPixelTemplate<Uint32, Sint32, Uint16>(buffer, InterData, Overlays, VoiLutData,
                    PresLutData, disp, WindowCenter, WindowWidth, low, high, Columns, Rows, frame, NumberOfFrames);
            else
                OutputData = new DiMonoOutputPixelTemplate<Uint32, Sint32, Uint32>(buffer, InterData, Overlays, VoiLutData,
                    PresLutData, disp, WindowCenter, WindowWidth, low, high, Columns, Rows, frame, NumberOfFrames);
        } else {
            if (bits <= 8)
                OutputData = new DiMonoOutputPixelTemplate<Uint32, Uint32, Uint8>(buffer, InterData, Overlays, VoiLutData,
                    PresLutData, disp, WindowCenter, WindowWidth, low, high, Columns, Rows, frame, NumberOfFrames, samples > 1);
            else if (bits <= 16)
                OutputData = new DiMonoOutputPixelTemplate<Uint32, Uint32, Uint16>(buffer, InterData, Overlays, VoiLutData,
                    PresLutData, disp, WindowCenter, WindowWidth, low, high, Columns, Rows, frame, NumberOfFrames);
            else
                OutputData = new DiMonoOutputPixelTemplate<Uint32, Uint32, Uint32>(buffer, InterData, Overlays, VoiLutData,
                    PresLutData, disp, WindowCenter, WindowWidth, low, high, Columns, Rows, frame, NumberOfFrames);
        }
    }
}


void DiMonoImage::getDataSint32(void *buffer,
                                DiDisplayFunction *disp,
                                const int samples,
                                const unsigned long frame,
                                const int bits,
                                const Uint32 low,
                                const Uint32 high)
{
    if (bits <= 8)
        OutputData = new DiMonoOutputPixelTemplate<Sint32, Sint32, Uint8>(buffer, InterData, Overlays, VoiLutData,
            PresLutData, disp, WindowCenter, WindowWidth, low, high, Columns, Rows, frame, NumberOfFrames, samples > 1);
    else if (bits <= 16)
        OutputData = new DiMonoOutputPixelTemplate<Sint32, Sint32, Uint16>(buffer, InterData, Overlays, VoiLutData,
            PresLutData, disp, WindowCenter, WindowWidth, low, high, Columns, Rows, frame, NumberOfFrames);
    else
        OutputData = new DiMonoOutputPixelTemplate<Sint32, Sint32, Uint32>(buffer, InterData, Overlays, VoiLutData,
            PresLutData, disp, WindowCenter, WindowWidth, low, high, Columns, Rows, frame, NumberOfFrames);
}


/*
 *
 * CVS/RCS Log:
 * $Log: dimoimg5.cc,v $
 * Revision 1.5  2005/12/08 15:42:59  meichel
 * Changed include path schema for all DCMTK header files
 *
 * Revision 1.4  2003/12/08 17:43:04  joergr
 * Updated copyright header.
 *
 * Revision 1.3  2001/06/01 15:49:57  meichel
 * Updated copyright header
 *
 * Revision 1.2  2000/03/08 16:24:31  meichel
 * Updated copyright header.
 *
 * Revision 1.1  1999/12/09 17:28:04  joergr
 * Split source file dimoimg.cc into 4 parts to avoid compiler problems
 * with gcc and additional optimization options.
 *
 *
 */
