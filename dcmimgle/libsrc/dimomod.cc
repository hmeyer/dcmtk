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
 *  Purpose: DicomMonochromeModality (Source)
 *
 *  Last Update:      $Author: meichel $
 *  Update Date:      $Date: 2005/12/08 15:43:00 $
 *  CVS/RCS Revision: $Revision: 1.21 $
 *  Status:           $State: Exp $
 *
 *  CVS/RCS Log at end of file
 *
 */


#include "dcmtk/config/osconfig.h"
#include "dcmtk/dcmdata/dcdeftag.h"
#include "dcmtk/dcmdata/dcuid.h"

#include "dcmtk/dcmimgle/dimomod.h"
#include "dcmtk/dcmimgle/didocu.h"
#include "dcmtk/dcmimgle/diinpx.h"


/*----------------*
 *  constructors  *
 *----------------*/

DiMonoModality::DiMonoModality(const DiDocument *docu,
                               DiInputPixel *pixel)
  : Representation(EPR_MaxSigned),
    MinValue(0),
    MaxValue(0),
    Bits(0),
    AbsMinimum(0),
    AbsMaximum(0),
    RescaleIntercept(0),
    RescaleSlope(0),
    LookupTable(0),
    Rescaling(0),
    TableData(NULL)
{
    if (Init(docu, pixel))
    {
        if (!(docu->getFlags() & CIF_UsePresentationState) &&           // ignore modality LUT and rescaling
            !(docu->getFlags() & CIF_IgnoreModalityTransformation))
        {
            const char *sopClassUID = NULL;                             // check for XA and XRF image (ignore MLUT)
            if ((docu->getValue(DCM_SOPClassUID, sopClassUID) == 0) || (sopClassUID == NULL) ||
               ((strcmp(sopClassUID, UID_XRayAngiographicImageStorage) != 0) &&
                (strcmp(sopClassUID, UID_XRayFluoroscopyImageStorage) != 0)))
            {
                TableData = new DiLookupTable(docu, DCM_ModalityLUTSequence, DCM_LUTDescriptor, DCM_LUTData,
                    DCM_LUTExplanation, (docu->getFlags() & CIF_IgnoreModalityLutBitDepth) > 0);
                checkTable();
                Rescaling = (docu->getValue(DCM_RescaleIntercept, RescaleIntercept) > 0);
                Rescaling &= (docu->getValue(DCM_RescaleSlope, RescaleSlope) > 0);
                checkRescaling(pixel);
            } else {
                if (DicomImageClass::checkDebugLevel(DicomImageClass::DL_Informationals))
                {
                    ofConsole.lockCerr() << "INFO: processing XA or XRF image ... ignoring possible modality transform !" << endl;
                    ofConsole.unlockCerr();
                }
            }
        } else {
            if (DicomImageClass::checkDebugLevel(DicomImageClass::DL_Informationals))
            {
                ofConsole.lockCerr() << "INFO: configuration flag set ... ignoring possible modality transform !" << endl;
                ofConsole.unlockCerr();
            }
        }
        Representation = DicomImageClass::determineRepresentation(MinValue, MaxValue);
    }
}


DiMonoModality::DiMonoModality(const DiDocument *docu,
                               DiInputPixel *pixel,
                               const double slope,
                               const double intercept)
  : Representation(EPR_MaxSigned),
    MinValue(0),
    MaxValue(0),
    Bits(0),
    AbsMinimum(0),
    AbsMaximum(0),
    RescaleIntercept(intercept),
    RescaleSlope(slope),
    LookupTable(0),
    Rescaling(0),
    TableData(NULL)
{
    if (Init(docu, pixel))
    {
        Rescaling = 1;
        checkRescaling(pixel);
        Representation = DicomImageClass::determineRepresentation(MinValue, MaxValue);
    }
}


DiMonoModality::DiMonoModality(const DiDocument *docu,
                               DiInputPixel *pixel,
                               const DcmUnsignedShort &data,
                               const DcmUnsignedShort &descriptor,
                               const DcmLongString *explanation)
  : Representation(EPR_MaxSigned),
    MinValue(0),
    MaxValue(0),
    Bits(0),
    AbsMinimum(0),
    AbsMaximum(0),
    RescaleIntercept(0),
    RescaleSlope(0),
    LookupTable(0),
    Rescaling(0),
    TableData(NULL)
{
    if (Init(docu, pixel))
    {
        TableData = new DiLookupTable(data, descriptor, explanation, (docu->getFlags() & CIF_IgnoreModalityLutBitDepth) > 0);
        checkTable();
        Representation = DicomImageClass::determineRepresentation(MinValue, MaxValue);
    }
}


DiMonoModality::DiMonoModality(const int bits)
  : Representation(EPR_MaxSigned),
    MinValue(0),
    MaxValue(0),
    Bits(bits),
    AbsMinimum(0),
    AbsMaximum(DicomImageClass::maxval(bits)),
    RescaleIntercept(0),
    RescaleSlope(0),
    LookupTable(0),
    Rescaling(0),
    TableData(NULL)
{
}


/*--------------*
 *  destructor  *
 *--------------*/

DiMonoModality::~DiMonoModality()
{
    delete TableData;
}


/*********************************************************************/


int DiMonoModality::Init(const DiDocument *docu,
                         DiInputPixel *pixel)
{
    if ((docu != NULL) && (pixel != NULL))
    {
        pixel->determineMinMax();
        MinValue = pixel->getMinValue(1 /* selected range of pixels only */);
        MaxValue = pixel->getMaxValue(1 /* selected range of pixels only */);
        Bits = pixel->getBits();
        AbsMinimum = pixel->getAbsMinimum();
        AbsMaximum = pixel->getAbsMaximum();
        Uint16 us;
        if (docu->getValue(DCM_SamplesPerPixel, us) && (us != 1))
        {
            if (DicomImageClass::checkDebugLevel(DicomImageClass::DL_Warnings))
            {
                ofConsole.lockCerr() << "WARNING: invalid value for 'SamplesPerPixel' (" << us << ") ... assuming 1 !" << endl;
                ofConsole.unlockCerr();
            }
        }
        return 1;
    }
    return 0;
}


void DiMonoModality::checkTable()
{
    if (TableData != NULL)
    {
        LookupTable = TableData->isValid();
        if (LookupTable)
        {
            MinValue = TableData->getMinValue();
            MaxValue = TableData->getMaxValue();
            Bits = TableData->getBits();
            AbsMinimum = 0;
            AbsMaximum = DicomImageClass::maxval(Bits);
        }
    }
}


void DiMonoModality::checkRescaling(const DiInputPixel *pixel)
{
    if (Rescaling)
    {
        if (LookupTable) {
            if (DicomImageClass::checkDebugLevel(DicomImageClass::DL_Warnings))
            {
                ofConsole.lockCerr() << "WARNING: redundant values for 'RescaleSlope/Intercept'"
                                     << " ... using modality LUT transformation !" << endl;
                ofConsole.unlockCerr();
            }
            Rescaling = 0;
        } else {
            if (RescaleSlope == 0)
            {
                if (DicomImageClass::checkDebugLevel(DicomImageClass::DL_Warnings))
                {
                    ofConsole.lockCerr() << "WARNING: invalid value for 'RescaleSlope' (" << RescaleSlope
                                         << ") ... ignoring modality transformation !" << endl;
                    ofConsole.unlockCerr();
                }
                Rescaling = 0;
            }
            else
            {
                if (RescaleSlope < 0)                                       // negative slope value
                {
                    const double temp = MinValue;
                    MinValue = MaxValue * RescaleSlope + RescaleIntercept;
                    MaxValue = temp * RescaleSlope + RescaleIntercept;
                    AbsMinimum = pixel->getAbsMaximum() * RescaleSlope + RescaleIntercept;
                    AbsMaximum = pixel->getAbsMinimum() * RescaleSlope + RescaleIntercept;
                } else {                                                    // positive slope value
                    MinValue = MinValue * RescaleSlope + RescaleIntercept;
                    MaxValue = MaxValue * RescaleSlope + RescaleIntercept;
                    AbsMinimum = pixel->getAbsMinimum() * RescaleSlope + RescaleIntercept;
                    AbsMaximum = pixel->getAbsMaximum() * RescaleSlope + RescaleIntercept;
                }                
                Bits = DicomImageClass::rangeToBits(AbsMinimum, AbsMaximum);
            }
        }
    }
}


/*
 *
 * CVS/RCS Log:
 * $Log: dimomod.cc,v $
 * Revision 1.21  2005/12/08 15:43:00  meichel
 * Changed include path schema for all DCMTK header files
 *
 * Revision 1.20  2005/03/09 17:37:08  joergr
 * Fixed bug in calculation of bits stored value after modality transformation.
 *
 * Revision 1.19  2003/12/17 16:18:34  joergr
 * Added new compatibility flag that allows to ignore the third value of LUT
 * descriptors and to determine the bits per table entry automatically.
 *
 * Revision 1.18  2003/12/08 17:38:27  joergr
 * Updated CVS header.
 *
 * Revision 1.17  2003/12/08 14:36:35  joergr
 * Adapted type casts to new-style typecast operators defined in ofcast.h.
 *
 * Revision 1.16  2003/05/20 09:25:08  joergr
 * Added new configuration/compatibility flag that allows to ignore the
 * modality transform stored in the dataset.
 *
 * Revision 1.15  2001/09/28 13:17:24  joergr
 * Enhanced algorithm to determine the min and max value.
 *
 * Revision 1.14  2001/06/01 15:49:58  meichel
 * Updated copyright header
 *
 * Revision 1.13  2000/12/14 13:46:45  joergr
 * Ignore modality LUT transform for XA and XRF images (report message on that
 * in verbose mode).
 *
 * Revision 1.12  2000/08/31 15:51:39  joergr
 * Corrected bug: min and max value were reversed for images with negative
 * rescale slope.
 *
 * Revision 1.11  2000/04/28 12:33:46  joergr
 * DebugLevel - global for the module - now derived from OFGlobal (MF-safe).
 *
 * Revision 1.10  2000/04/27 13:10:30  joergr
 * Dcmimgle library code now consistently uses ofConsole for error output.
 *
 * Revision 1.9  2000/03/08 16:24:31  meichel
 * Updated copyright header.
 *
 * Revision 1.8  2000/03/03 14:09:21  meichel
 * Implemented library support for redirecting error messages into memory
 *   instead of printing them to stdout/stderr for GUI applications.
 *
 * Revision 1.7  1999/05/31 12:35:58  joergr
 * Corrected bug concerning the conversion of color images to grayscale.
 *
 * Revision 1.6  1999/04/28 15:04:48  joergr
 * Introduced new scheme for the debug level variable: now each level can be
 * set separately (there is no "include" relationship).
 *
 * Revision 1.5  1999/02/03 17:41:45  joergr
 * Moved global functions maxval() and determineRepresentation() to class
 * DicomImageClass (as static methods).
 * Added member variable and related methods to store number of bits used for
 * pixel data.
 *
 * Revision 1.4  1998/12/22 13:41:04  joergr
 * Changed calculation of AbsMinimum/Maximum.
 * Removed member variable and method for isPotentiallySigned.
 *
 * Revision 1.3  1998/12/16 16:16:50  joergr
 * Added explanation string to LUT class (retrieved from dataset).
 *
 * Revision 1.2  1998/12/14 17:38:18  joergr
 * Added support for correct scaling of input/output values for grayscale
 * transformations.
 *
 * Revision 1.1  1998/11/27 16:14:35  joergr
 * Added copyright message.
 * Introduced global debug level for dcmimage module to control error output.
 * Added constructors to use external modality transformations.
 *
 * Revision 1.4  1998/05/11 14:52:33  joergr
 * Added CVS/RCS header to each file.
 *
 *
 */
