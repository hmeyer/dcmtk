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
 *  Purpose: DicomColorPixel (Source)
 *
 *  Last Update:      $Author: meichel $
 *  Update Date:      $Date: 2005/12/08 15:42:23 $
 *  CVS/RCS Revision: $Revision: 1.15 $
 *  Status:           $State: Exp $
 *
 *  CVS/RCS Log at end of file
 *
 */


#include "dcmtk/config/osconfig.h"
#include "dcmtk/dcmdata/dctypes.h"
#include "dcmtk/dcmdata/dcdeftag.h"

#include "dcmtk/dcmimage/dicopx.h"
#include "dcmtk/dcmimgle/dimopx.h"
#include "dcmtk/dcmimgle/diinpx.h"
#include "dcmtk/dcmimgle/didocu.h"


/*----------------*
 *  constructors  *
 *----------------*/

DiColorPixel::DiColorPixel(const DiDocument *docu,
                           const DiInputPixel *pixel,
                           const Uint16 samples,
                           EI_Status &status,
                           const Uint16 sample_rate)
  : DiPixel(0),
    PlanarConfiguration(0)
{
    if (docu != NULL)
    {
        Uint16 us = 0;
        if (docu->getValue(DCM_SamplesPerPixel, us))
        {
            if (us != samples)
            {
                if (DicomImageClass::checkDebugLevel(DicomImageClass::DL_Warnings))
                {
                    ofConsole.lockCerr() << "WARNING: invalid value for 'SamplesPerPixel' (" << us
                                         << ") ... assuming " << samples << " !" << endl;
                    ofConsole.unlockCerr();
                }
            }
            if (docu->getValue(DCM_PlanarConfiguration, us))
            {
                /* only use Planar Configuration attribute if there are multiple planes */
                if (samples > 1)
                {
                    PlanarConfiguration = (us == 1);
                    if ((us != 0) && (us != 1))
                    {
                        if (DicomImageClass::checkDebugLevel(DicomImageClass::DL_Warnings))
                        {
                            ofConsole.lockCerr() << "WARNING: invalid value for 'PlanarConfiguration' (" << us
                                                 << ") ... assuming 'color-by-pixel' (0) !" << endl;
                            ofConsole.unlockCerr();
                        }
                    }
                } else {
                    if (DicomImageClass::checkDebugLevel(DicomImageClass::DL_Warnings))
                    {
                        ofConsole.lockCerr() << "WARNING: unexpected attribute 'PlanarConfiguration' (" << us
                                             << ") ... ignoring !" << endl;
                        ofConsole.unlockCerr();
                    }
                }
            }
            else if (samples > 1)
            {
                status = EIS_MissingAttribute;
                if (DicomImageClass::checkDebugLevel(DicomImageClass::DL_Errors))
                {
                    ofConsole.lockCerr() << "ERROR: mandatory attribute 'PlanarConfiguration' is missing !" << endl;
                    ofConsole.unlockCerr();
                }
                return;
            }
            if (pixel != NULL)
            {
                // number of pixels (per plane) computed from the length of the PixelData attribute
                InputCount = pixel->getPixelCount() / ((sample_rate == 0) ? samples : sample_rate);
                // number of pixels allocated for the intermediate buffer
                Count = pixel->getComputedCount() / ((sample_rate == 0) ? samples : sample_rate);
            }
        } else {
            status = EIS_MissingAttribute;
            if (DicomImageClass::checkDebugLevel(DicomImageClass::DL_Errors))
            {
                ofConsole.lockCerr() << "ERROR: mandatory attribute 'SamplesPerPixel' is missing !" << endl;
                ofConsole.unlockCerr();
            }
        }
    }
}

DiColorPixel::DiColorPixel(const DiColorPixel *pixel,
                           const unsigned long count)
  : DiPixel(count, pixel->InputCount),
    PlanarConfiguration(pixel->PlanarConfiguration)
{
}


/*--------------*
 *  destructor  *
 *--------------*/

DiColorPixel::~DiColorPixel()
{
}


/*
 *
 * CVS/RCS Log:
 * $Log: dicopx.cc,v $
 * Revision 1.15  2005/12/08 15:42:23  meichel
 * Changed include path schema for all DCMTK header files
 *
 * Revision 1.14  2004/06/03 09:08:01  joergr
 * Changed error message on unexpected attribute PlanarConfiguration into a
 * warning message.
 *
 * Revision 1.13  2003/12/23 10:54:28  joergr
 * Updated copyright header.
 *
 * Revision 1.12  2002/06/26 16:29:45  joergr
 * Enhanced handling of corrupted pixel data and/or length.
 *
 * Revision 1.11  2001/11/09 16:49:38  joergr
 * Removed unused constructor.
 *
 * Revision 1.10  2001/06/01 15:49:35  meichel
 * Updated copyright header
 *
 * Revision 1.9  2000/04/28 12:40:03  joergr
 * DebugLevel - global for the module - now derived from OFGlobal (MF-safe).
 *
 * Revision 1.8  2000/04/27 13:15:57  joergr
 * Dcmimage library code now consistently uses ofConsole for error output.
 *
 * Revision 1.7  2000/03/08 16:21:56  meichel
 * Updated copyright header.
 *
 * Revision 1.6  2000/03/03 14:07:55  meichel
 * Implemented library support for redirecting error messages into memory
 *   instead of printing them to stdout/stderr for GUI applications.
 *
 * Revision 1.5  1999/04/28 13:20:13  joergr
 * Introduced new scheme for the debug level variable: now each level can be
 * set separately (there is no "include" relationship).
 *
 * Revision 1.4  1998/11/27 14:29:32  joergr
 * Added copyright message.
 * Introduced global debug level for dcmimage module to control error output.
 *
 * Revision 1.3  1998/05/11 14:52:27  joergr
 * Added CVS/RCS header to each file.
 *
 *
 */
