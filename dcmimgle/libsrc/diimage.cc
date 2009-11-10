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
 *  Purpose: DicomImage (Source)
 *
 *  Last Update:      $Author: meichel $
 *  Update Date:      $Date: 2005/12/08 15:42:51 $
 *  CVS/RCS Revision: $Revision: 1.32 $
 *  Status:           $State: Exp $
 *
 *  CVS/RCS Log at end of file
 *
 */


#include "dcmtk/config/osconfig.h"
#include "dcmtk/dcmdata/dctypes.h"
#include "dcmtk/dcmdata/dcdeftag.h"
#include "dcmtk/dcmdata/dcswap.h"

#include "dcmtk/dcmimgle/diimage.h"
#include "dcmtk/dcmimgle/diinpxt.h"
#include "dcmtk/dcmimgle/didocu.h"
#include "dcmtk/dcmimgle/diutils.h"
#include "dcmtk/ofstd/ofstd.h"

#define INCLUDE_CSTRING
#include "dcmtk/ofstd/ofstdinc.h"


/*----------------*
 *  constructors  *
 *----------------*/

DiImage::DiImage(const DiDocument *docu,
                 const EI_Status status,
                 const int spp)
  : ImageStatus(status),
    Document(docu),
    FirstFrame(0),
    NumberOfFrames(0),
    TotalNumberOfFrames(0),
    RepresentativeFrame(0),
    Rows(0),
    Columns(0),
    PixelWidth(1),
    PixelHeight(1),
    BitsAllocated(0),
    BitsStored(0),
    HighBit(0),
    BitsPerSample(0),
    Polarity(EPP_Normal),
    hasSignedRepresentation(0),
    hasPixelSpacing(0),
    hasImagerPixelSpacing(0),
    hasPixelAspectRatio(0),
    isOriginal(1),
    InputData(NULL)
{
    if ((Document != NULL) && (ImageStatus == EIS_Normal))
    {
        Sint32 sl = 0;
        if (Document->getValue(DCM_NumberOfFrames, sl))
        {
            if (sl < 1)
            {
                if (DicomImageClass::checkDebugLevel(DicomImageClass::DL_Warnings))
                {
                    ofConsole.lockCerr() << "WARNING: invalid value for 'NumberOfFrames' (" << sl << ") "
                                         << "... assuming 1 !" << endl;
                    ofConsole.unlockCerr();
                }
                NumberOfFrames = 1;
            } else
                NumberOfFrames = OFstatic_cast(Uint32, sl);
        } else
            NumberOfFrames = 1;
        Uint16 us = 0;
        if (Document->getValue(DCM_RepresentativeFrameNumber, us))
        {
            if (us <= FirstFrame)
            {
                if (DicomImageClass::checkDebugLevel(DicomImageClass::DL_Warnings))
                {
                    ofConsole.lockCerr() << "WARNING: invalid value for 'RepresentativeFrameNumber' (" << us << ")" << endl
                                         << "         ... assuming first frame !" << endl;
                    ofConsole.unlockCerr();
                }
                RepresentativeFrame = FirstFrame;
            }
            else if (us > NumberOfFrames)
            {
                if (DicomImageClass::checkDebugLevel(DicomImageClass::DL_Warnings))
                {
                    ofConsole.lockCerr() << "WARNING: invalid value for 'RepresentativeFrameNumber' (" << us << ")" << endl
                                         << "         ... assuming last frame !" << endl;
                    ofConsole.unlockCerr();
                }
                RepresentativeFrame = NumberOfFrames - 1;
            }
            else
                RepresentativeFrame = us - 1;
        }
        FirstFrame = (docu->getFrameStart() < NumberOfFrames) ? docu->getFrameStart() : NumberOfFrames - 1;
        /* store total number of frames in the dataset */
        TotalNumberOfFrames = NumberOfFrames;
        /* restrict to actually processed/loaded number of frames */
        NumberOfFrames -= FirstFrame;
        if ((docu->getFrameCount() > 0) && (NumberOfFrames > docu->getFrameCount()))
            NumberOfFrames = docu->getFrameCount();
        /* start from first processed frame (might still exceed number of loaded frames) */
        RepresentativeFrame -= FirstFrame;
        int ok = (Document->getValue(DCM_Rows, Rows) > 0);
        ok &= (Document->getValue(DCM_Columns, Columns) > 0);
        if (!ok || ((Rows > 0) && (Columns > 0)))
        {
            ok &= (Document->getValue(DCM_BitsAllocated, BitsAllocated) > 0);
            ok &= (Document->getValue(DCM_BitsStored, BitsStored) > 0);
            if (((Document->getValue(DCM_HighBit, HighBit) == 0) || (HighBit == 0)) && ok)
            {
                HighBit = BitsStored - 1;
                if (DicomImageClass::checkDebugLevel(DicomImageClass::DL_Warnings))
                {
                    ofConsole.lockCerr() << "WARNING: missing value for 'HighBit' "
                                         << "... assuming " << HighBit << " !" << endl;
                    ofConsole.unlockCerr();
                }
            }
            ok &= (Document->getValue(DCM_PixelRepresentation, us) > 0);
            BitsPerSample = BitsStored;
            hasSignedRepresentation = (us == 1);
            if ((us != 0) && (us != 1))
            {
                if (DicomImageClass::checkDebugLevel(DicomImageClass::DL_Warnings))
                {
                    ofConsole.lockCerr() << "WARNING: invalid value for 'PixelRepresentation' (" << us << ") "
                                         << "... assuming 'unsigned' (0) !" << endl;
                    ofConsole.unlockCerr();
                }
            }
            if (!(Document->getFlags() & CIF_UsePresentationState))
            {
                hasPixelSpacing = (Document->getValue(DCM_PixelSpacing, PixelHeight, 0) > 0);
                if (hasPixelSpacing)
                {
                    if (Document->getValue(DCM_PixelSpacing, PixelWidth, 1) < 2)
                    {
                        if (DicomImageClass::checkDebugLevel(DicomImageClass::DL_Warnings))
                        {
                            ofConsole.lockCerr() << "WARNING: missing second value for 'PixelSpacing' ... "
                                                 << "assuming 'Width' = " << PixelWidth << " !" << endl;
                            ofConsole.unlockCerr();
                        }
                    }
                } else {
                    hasImagerPixelSpacing = (Document->getValue(DCM_ImagerPixelSpacing, PixelHeight, 0) > 0);
                    if (hasImagerPixelSpacing)
                    {
                        if (Document->getValue(DCM_ImagerPixelSpacing, PixelWidth, 1) < 2)
                        {
                            if (DicomImageClass::checkDebugLevel(DicomImageClass::DL_Warnings))
                            {
                                ofConsole.lockCerr() << "WARNING: missing second value for 'ImagerPixelSpacing' ... "
                                                     << "assuming 'Width' = " << PixelWidth << " !" << endl;
                                ofConsole.unlockCerr();
                            }
                        }
                    } else {
                        Sint32 sl2;
                        hasPixelAspectRatio = (Document->getValue(DCM_PixelAspectRatio, sl2, 0) > 0);
                        if (hasPixelAspectRatio)
                        {
                            PixelHeight = sl2;
                            if (Document->getValue(DCM_PixelAspectRatio, sl2, 1) < 2)
                            {
                                if (DicomImageClass::checkDebugLevel(DicomImageClass::DL_Warnings))
                                {
                                    ofConsole.lockCerr() << "WARNING: missing second value for 'PixelAspectRatio' ... "
                                                         << "assuming 'Width' = " << PixelWidth << " !" << endl;
                                    ofConsole.unlockCerr();
                                }
                            } else
                                PixelWidth = sl2;
                        } else {
                            PixelWidth = 1;
                            PixelHeight = 1;
                        }
                    }
                }
                checkPixelExtension();
            }
            DcmStack pstack;
            // get pixel data (if present)
            if (ok && Document->search(DCM_PixelData, pstack))
            {
                DcmPixelData *pixel = OFstatic_cast(DcmPixelData *, pstack.top());
                // check whether pixel data exists unencapsulated (decompression already done in DiDocument)
                if ((pixel != NULL) && (DcmXfer(Document->getTransferSyntax()).isNotEncapsulated()))
                    convertPixelData(pixel, spp);
                else
                    ImageStatus = EIS_InvalidValue;
            } else {
                ImageStatus = EIS_MissingAttribute;
                if (DicomImageClass::checkDebugLevel(DicomImageClass::DL_Errors))
                {
                    ofConsole.lockCerr() << "ERROR: one or more mandatory attributes are missing in image pixel module !" << endl;
                    ofConsole.unlockCerr();
                }
            }
        } else {
            ImageStatus = EIS_InvalidValue;
            if (DicomImageClass::checkDebugLevel(DicomImageClass::DL_Errors))
            {
                ofConsole.lockCerr() << "ERROR: invalid value for 'Rows' (" << Rows << ") and/or 'Columns' (" << Columns << ") !" << endl;
                ofConsole.unlockCerr();
            }
        }
    } else {
        ImageStatus = EIS_InvalidDocument;
        if (DicomImageClass::checkDebugLevel(DicomImageClass::DL_Errors))
        {
            ofConsole.lockCerr() << "ERROR: this DICOM document is invalid !" << endl;
            ofConsole.unlockCerr();
        }
    }
}


DiImage::DiImage(const DiDocument *docu,
                 const EI_Status status)
  : ImageStatus(status),
    Document(docu),
    FirstFrame(0),
    NumberOfFrames(0),
    TotalNumberOfFrames(0),
    RepresentativeFrame(0),
    Rows(0),
    Columns(0),
    PixelWidth(1),
    PixelHeight(1),
    BitsAllocated(0),
    BitsStored(0),
    HighBit(0),
    BitsPerSample(0),
    Polarity(EPP_Normal),
    hasSignedRepresentation(0),
    hasPixelSpacing(0),
    hasImagerPixelSpacing(0),
    hasPixelAspectRatio(0),
    isOriginal(1),
    InputData(NULL)
{
}


DiImage::DiImage(const DiImage *image,
                 const unsigned long fstart,
                 const unsigned long fcount)
  : ImageStatus(image->ImageStatus),
    Document(image->Document),
    FirstFrame(image->FirstFrame + fstart),
    NumberOfFrames(fcount),
    TotalNumberOfFrames(image->TotalNumberOfFrames),
    RepresentativeFrame(image->RepresentativeFrame),
    Rows(image->Rows),
    Columns(image->Columns),
    PixelWidth(image->PixelWidth),
    PixelHeight(image->PixelHeight),
    BitsAllocated(image->BitsAllocated),
    BitsStored(image->BitsStored),
    HighBit(image->HighBit),
    BitsPerSample(image->BitsPerSample),
    Polarity(image->Polarity),
    hasSignedRepresentation(image->hasSignedRepresentation),
    hasPixelSpacing(image->hasPixelSpacing),
    hasImagerPixelSpacing(image->hasImagerPixelSpacing),
    hasPixelAspectRatio(image->hasPixelAspectRatio),
    isOriginal(0),
    InputData(NULL)
{
}


/* constructor: image scaled */

DiImage::DiImage(const DiImage *image,
                 const Uint16 columns,
                 const Uint16 rows,
                 const int aspect)
  : ImageStatus(image->ImageStatus),
    Document(image->Document),
    FirstFrame(image->FirstFrame),
    NumberOfFrames(image->NumberOfFrames),
    TotalNumberOfFrames(image->TotalNumberOfFrames),
    RepresentativeFrame(image->RepresentativeFrame),
    Rows(rows),
    Columns(columns),
    PixelWidth(1),
    PixelHeight(1),
    BitsAllocated(image->BitsAllocated),
    BitsStored(image->BitsStored),
    HighBit(image->HighBit),
    BitsPerSample(image->BitsPerSample),
    Polarity(image->Polarity),
    hasSignedRepresentation(image->hasSignedRepresentation),
    hasPixelSpacing(0),
    hasImagerPixelSpacing(0),
    hasPixelAspectRatio(0),
    isOriginal(0),
    InputData(NULL)
{
    /* we do not check for "division by zero", this is already done somewhere else */
    const double xfactor = OFstatic_cast(double, Columns) / OFstatic_cast(double, image->Columns);
    const double yfactor = OFstatic_cast(double, Rows) / OFstatic_cast(double, image->Rows);
    /* re-compute pixel width and height */
    if (image->hasPixelSpacing)
    {
        hasPixelSpacing = image->hasPixelSpacing;
        PixelWidth = image->PixelWidth / xfactor;
        PixelHeight = image->PixelHeight / yfactor;
    }
    else if (image->hasImagerPixelSpacing)
    {
        hasImagerPixelSpacing = image->hasImagerPixelSpacing;
        PixelWidth = image->PixelWidth / xfactor;
        PixelHeight = image->PixelHeight / yfactor;
    }
    else if (image->hasPixelAspectRatio && !aspect /*recognize pixel aspect ratio*/)
    {
        hasPixelAspectRatio = image->hasPixelAspectRatio;
        PixelWidth = image->PixelWidth * xfactor;
        PixelHeight = image->PixelHeight * yfactor;
        /* do not store pixel aspect ratio for square pixels */
        if (PixelWidth == PixelHeight)
            hasPixelAspectRatio = 0;
    }
}


DiImage::DiImage(const DiImage *image,
                 const int degree)
  : ImageStatus(image->ImageStatus),
    Document(image->Document),
    FirstFrame(image->FirstFrame),
    NumberOfFrames(image->NumberOfFrames),
    TotalNumberOfFrames(image->TotalNumberOfFrames),
    RepresentativeFrame(image->RepresentativeFrame),
    Rows(((degree == 90) ||(degree == 270)) ? image->Columns : image->Rows),
    Columns(((degree == 90) ||(degree == 270)) ? image->Rows : image->Columns),
    PixelWidth(((degree == 90) ||(degree == 270)) ? image->PixelHeight : image->PixelWidth),
    PixelHeight(((degree == 90) ||(degree == 270)) ? image-> PixelWidth : image->PixelHeight),
    BitsAllocated(image->BitsAllocated),
    BitsStored(image->BitsStored),
    HighBit(image->HighBit),
    BitsPerSample(image->BitsPerSample),
    Polarity(image->Polarity),
    hasSignedRepresentation(image->hasSignedRepresentation),
    hasPixelSpacing(image->hasPixelSpacing),
    hasImagerPixelSpacing(image->hasImagerPixelSpacing),
    hasPixelAspectRatio(image->hasPixelAspectRatio),
    isOriginal(0),
    InputData(NULL)
{
}


DiImage::DiImage(const DiImage *image,
                 const unsigned long frame,
                 const int stored,
                 const int alloc)
  : ImageStatus(image->ImageStatus),
    Document(image->Document),
    FirstFrame(frame),
    NumberOfFrames(1),
    TotalNumberOfFrames(image->TotalNumberOfFrames),
    RepresentativeFrame(0),
    Rows(image->Rows),
    Columns(image->Columns),
    PixelWidth(image->PixelWidth),
    PixelHeight(image->PixelHeight),
    BitsAllocated(alloc),
    BitsStored(stored),
    HighBit(stored - 1),
    BitsPerSample(image->BitsPerSample),
    Polarity(image->Polarity),
    hasSignedRepresentation(0),
    hasPixelSpacing(image->hasPixelSpacing),
    hasImagerPixelSpacing(image->hasImagerPixelSpacing),
    hasPixelAspectRatio(image->hasPixelAspectRatio),
    isOriginal(0),
    InputData(NULL)
{
}


/*--------------*
 *  destructor  *
 *--------------*/

DiImage::~DiImage()
{
    delete InputData;
}


/********************************************************************/


int DiImage::rotate(const int degree)
{
    if ((degree == 90) || (degree == 270))
    {
        Uint16 us = Rows;                   // swap image width and height
        Rows = Columns;
        Columns = us;
        double db = PixelWidth;             // swap pixel width and height
        PixelWidth = PixelHeight;
        PixelHeight = db;
        return 1;
    }
    return 0;
}


/********************************************************************/


void DiImage::deleteInputData()
{
    delete InputData;
    InputData = NULL;
}


void DiImage::checkPixelExtension()
{
    if (hasPixelSpacing || hasImagerPixelSpacing || hasPixelAspectRatio)
    {
        if (PixelHeight == 0)
        {
            if (DicomImageClass::checkDebugLevel(DicomImageClass::DL_Warnings))
            {
                ofConsole.lockCerr() << "WARNING: invalid value for 'PixelHeight' (" << PixelHeight << ") "
                                     << "... assuming 1 !" << endl;
                ofConsole.unlockCerr();
            }
            PixelHeight = 1;
        }
        else if (PixelHeight < 0)
        {
            if (DicomImageClass::checkDebugLevel(DicomImageClass::DL_Warnings))
            {
                ofConsole.lockCerr() << "WARNING: negative value for 'PixelHeight' (" << PixelHeight << ") "
                                     << "... assuming " << -PixelHeight << " !" << endl;
                ofConsole.unlockCerr();
            }
            PixelHeight = -PixelHeight;
        }
        if (PixelWidth == 0)
        {
            if (DicomImageClass::checkDebugLevel(DicomImageClass::DL_Warnings))
            {
                ofConsole.lockCerr() << "WARNING: invalid value for 'PixelWidth' (" << PixelWidth << ") "
                                     << "... assuming 1 !" << endl;
                ofConsole.unlockCerr();
            }
            PixelWidth = 1;
        }
        else if (PixelWidth < 0)
        {
            if (DicomImageClass::checkDebugLevel(DicomImageClass::DL_Warnings))
            {
                ofConsole.lockCerr() << "WARNING: negative value for 'PixelWidth' (" << PixelWidth << ") "
                                     << "... assuming " << -PixelWidth << " !" << endl;
                ofConsole.unlockCerr();
            }
            PixelHeight = -PixelHeight;
        }
    }
}


void DiImage::convertPixelData(/*const*/ DcmPixelData *pixel,
                               const int spp /*samplePerPixel*/)
{
    /* check for valid/supported pixel data encoding */
    if ((pixel->getVR() == EVR_OW) || ((pixel->getVR() == EVR_OB) && (BitsAllocated <= 16)))
    {
        const unsigned long start = FirstFrame * OFstatic_cast(unsigned long, Rows) *
            OFstatic_cast(unsigned long, Columns) * OFstatic_cast(unsigned long, spp);
        const unsigned long count = NumberOfFrames * OFstatic_cast(unsigned long, Rows) *
            OFstatic_cast(unsigned long, Columns) * OFstatic_cast(unsigned long, spp);
        if ((BitsAllocated < 1) || (BitsStored < 1) || (BitsAllocated < BitsStored) ||
            (BitsStored > OFstatic_cast(Uint16, HighBit + 1)))
        {
            ImageStatus = EIS_InvalidValue;
            if (DicomImageClass::checkDebugLevel(DicomImageClass::DL_Errors))
            {
                ofConsole.lockCerr() << "ERROR: invalid values for 'BitsAllocated' (" << BitsAllocated << "), "
                                     << "'BitsStored' (" << BitsStored << ") and/or 'HighBit' (" << HighBit << ") !" << endl;
                ofConsole.unlockCerr();
            }
            return;
        }
        else if ((pixel->getVR() == EVR_OB) && (BitsAllocated <= 8))
        {
            if (hasSignedRepresentation)
                InputData = new DiInputPixelTemplate<Uint8, Sint8>(pixel, BitsAllocated, BitsStored, HighBit, start, count);
            else
                InputData  = new DiInputPixelTemplate<Uint8, Uint8>(pixel, BitsAllocated, BitsStored, HighBit, start, count);
        }
        // allow non-standard encoding of pixel data
        else if ((pixel->getVR() == EVR_OB) && (BitsAllocated <= 16))
        {
            // report a warning message on this standard violation
            if (DicomImageClass::checkDebugLevel(DicomImageClass::DL_Warnings))
            {
                ofConsole.lockCerr() << "WARNING: invalid value for 'BitsAllocated' (" << BitsAllocated
                                     << "), > 8 for OB encoded 'PixelData' !" << endl;
                ofConsole.unlockCerr();
            }
            if (hasSignedRepresentation)
                InputData = new DiInputPixelTemplate<Uint8, Sint16>(pixel, BitsAllocated, BitsStored, HighBit, start, count);
            else
                InputData  = new DiInputPixelTemplate<Uint8, Uint16>(pixel, BitsAllocated, BitsStored, HighBit, start, count);
        }
        else if (BitsStored <= bitsof(Uint8))
        {
            if (hasSignedRepresentation)
                InputData = new DiInputPixelTemplate<Uint16, Sint8>(pixel, BitsAllocated, BitsStored, HighBit, start, count);
            else
                InputData  = new DiInputPixelTemplate<Uint16, Uint8>(pixel, BitsAllocated, BitsStored, HighBit, start, count);
        }
        else if (BitsStored <= bitsof(Uint16))
        {
            if (hasSignedRepresentation)
                InputData = new DiInputPixelTemplate<Uint16, Sint16>(pixel, BitsAllocated, BitsStored, HighBit, start, count);
            else
                InputData = new DiInputPixelTemplate<Uint16, Uint16>(pixel, BitsAllocated, BitsStored, HighBit, start, count);
        }
        else if (BitsStored <= bitsof(Uint32))
        {
            if (hasSignedRepresentation)
                InputData = new DiInputPixelTemplate<Uint16, Sint32>(pixel, BitsAllocated, BitsStored, HighBit, start, count);
            else
                InputData = new DiInputPixelTemplate<Uint16, Uint32>(pixel, BitsAllocated, BitsStored, HighBit, start, count);
        }
        else    /* BitsStored > 32 !! */
        {
            ImageStatus = EIS_NotSupportedValue;
            if (DicomImageClass::checkDebugLevel(DicomImageClass::DL_Errors))
            {
                ofConsole.lockCerr() << "ERROR: invalid value for 'BitsStored' (" << BitsStored << ") "
                                     << "... exceeds " << MAX_BITS << " bit !" << endl;
                ofConsole.unlockCerr();
            }
            return;
        }
        if (InputData == NULL)
        {
            ImageStatus = EIS_MemoryFailure;
            if (DicomImageClass::checkDebugLevel(DicomImageClass::DL_Errors))
            {
                ofConsole.lockCerr() << "ERROR: can't allocate memory for input-representation !" << endl;
                ofConsole.unlockCerr();
            }
        }
        else if (InputData->getPixelStart() >= InputData->getCount())
        {
            ImageStatus = EIS_InvalidValue;
            if (DicomImageClass::checkDebugLevel(DicomImageClass::DL_Errors))
            {
                ofConsole.lockCerr() << "ERROR: start offset (" << InputData->getPixelStart()
                                     << ") exceeds number of pixels stored (" << InputData->getCount() << ") " << endl;
                ofConsole.unlockCerr();
            }
        }
    }
    else
    {
        ImageStatus = EIS_NotSupportedValue;
        if (DicomImageClass::checkDebugLevel(DicomImageClass::DL_Errors))
        {
            ofConsole.lockCerr() << "ERROR: 'PixelData' has an other value representation than OB "
                                 << "(with 'BitsAllocated' <= 16) or OW !" << endl;
            ofConsole.unlockCerr();
        }
    }
}


int DiImage::detachPixelData()
{
    if ((Document != NULL) && (Document->getFlags() & CIF_MayDetachPixelData))
    {
        /* get DICOM dataset */
        DcmDataset *dataset = OFstatic_cast(DcmDataset *, Document->getDicomObject());
        if (dataset != NULL)
        {
            /* insert new, empty PixelData element */
            dataset->putAndInsertUint16Array(DCM_PixelData, NULL, 0, OFTrue /*replaceOld*/);
#ifdef DEBUG
            if (DicomImageClass::checkDebugLevel(DicomImageClass::DL_Informationals))
            {
                ofConsole.lockCerr() << "INFO: detach pixel data" << endl;
                ofConsole.unlockCerr();
            }
#endif
            return 1;
        }
    }
    return 0;
}


int DiImage::setColumnRowRatio(const double ratio)
{
    hasPixelAspectRatio = 1;
    hasPixelSpacing = hasImagerPixelSpacing = 0;
    PixelWidth = ratio;
    PixelHeight = 1;
    checkPixelExtension();
    return 1;
}


int DiImage::setRowColumnRatio(const double ratio)
{
    hasPixelAspectRatio = 1;
    hasPixelSpacing = hasImagerPixelSpacing = 0;
    PixelWidth = 1;
    PixelHeight = ratio;
    checkPixelExtension();
    return 1;
}


int DiImage::setPolarity(const EP_Polarity polarity)
{
    if (polarity != Polarity)
    {
        Polarity = polarity;
        return 1;
    }
    return 2;
}


void DiImage::updateImagePixelModuleAttributes(DcmItem &dataset)
{
    /* remove outdated attributes from the dataset */
    delete dataset.remove(DCM_SmallestImagePixelValue);
    delete dataset.remove(DCM_LargestImagePixelValue);
/*
    delete dataset.remove(DCM_PixelPaddingValue);
    delete dataset.remove(DCM_SmallestPixelValueInSeries);
    delete dataset.remove(DCM_LargestPixelValueInSeries);
*/
    /* update PixelAspectRatio & Co. */
    char buffer[32];
    OFStandard::ftoa(buffer, 15, PixelHeight, OFStandard::ftoa_format_f);
    strcat(buffer, "\\");
    OFStandard::ftoa(strchr(buffer, 0), 15, PixelWidth, OFStandard::ftoa_format_f);

    if (hasPixelSpacing)
    {
        dataset.putAndInsertString(DCM_PixelSpacing, buffer);
        dataset.putAndInsertString(DCM_PixelSpacing, buffer);
    } else
        delete dataset.remove(DCM_PixelSpacing);
    if (hasImagerPixelSpacing)
    {
        dataset.putAndInsertString(DCM_ImagerPixelSpacing, buffer);
        dataset.putAndInsertString(DCM_ImagerPixelSpacing, buffer);
    } else
        delete dataset.remove(DCM_ImagerPixelSpacing);
    if (hasPixelAspectRatio)
    {
        dataset.putAndInsertString(DCM_PixelAspectRatio, buffer);
        dataset.putAndInsertString(DCM_PixelAspectRatio, buffer);
    } else
        delete dataset.remove(DCM_PixelAspectRatio);
}


// --- write given frame of the current image to DICOM dataset

int DiImage::writeFrameToDataset(DcmItem &dataset,
                                 const unsigned long frame,
                                 const int bits,
                                 const int planar)
{
    int result = 0;
    const int bitsStored = getBits(bits);
    /* get output pixel data */
    const void *pixel = getOutputData(frame, bitsStored, planar);
    if (pixel != NULL)
    {
        char buffer[32];
        unsigned long count;
        /* write color model dependent attributes */
        if ((getInternalColorModel() == EPI_Monochrome1) || (getInternalColorModel() == EPI_Monochrome2))
        {
            /* monochrome image */
            count = OFstatic_cast(unsigned long, Columns) * OFstatic_cast(unsigned long, Rows);
            dataset.putAndInsertString(DCM_PhotometricInterpretation, "MONOCHROME2");
            dataset.putAndInsertUint16(DCM_SamplesPerPixel, 1);
        } else {
            /* color image */
            count = OFstatic_cast(unsigned long, Columns) * OFstatic_cast(unsigned long, Rows) * 3 /*samples per pixel*/;
            if (getInternalColorModel() == EPI_YBR_Full)
                dataset.putAndInsertString(DCM_PhotometricInterpretation, "YBR_FULL");
            else
                dataset.putAndInsertString(DCM_PhotometricInterpretation, "RGB");
            dataset.putAndInsertUint16(DCM_PlanarConfiguration, planar != 0);
            dataset.putAndInsertUint16(DCM_SamplesPerPixel, 3);
        }
        /* write remaining attributes */
        dataset.putAndInsertUint16(DCM_Columns, Columns);
        dataset.putAndInsertUint16(DCM_Rows, Rows);
        dataset.putAndInsertString(DCM_NumberOfFrames, "1");
        if (bitsStored <= 8)
            dataset.putAndInsertUint16(DCM_BitsAllocated, 8);
        else if (bitsStored <= 16)
            dataset.putAndInsertUint16(DCM_BitsAllocated, 16);
        else
            dataset.putAndInsertUint16(DCM_BitsAllocated, 32);
        dataset.putAndInsertUint16(DCM_BitsStored, bitsStored);
        dataset.putAndInsertUint16(DCM_HighBit, bitsStored - 1);
        dataset.putAndInsertUint16(DCM_PixelRepresentation, 0);
        /* handle VOI transformations */
        if (dataset.tagExists(DCM_WindowCenter) ||
            dataset.tagExists(DCM_WindowWidth) ||
            dataset.tagExists(DCM_VOILUTSequence))
        {
            delete dataset.remove(DCM_VOILUTSequence);
            sprintf(buffer, "%lu", DicomImageClass::maxval(bitsStored, 0) / 2);
            dataset.putAndInsertString(DCM_WindowCenter, buffer);
            sprintf(buffer, "%lu", DicomImageClass::maxval(bitsStored, 0));
            dataset.putAndInsertString(DCM_WindowWidth, buffer);
        }
        delete dataset.remove(DCM_WindowCenterWidthExplanation);
        /* write pixel data (OB or OW) */
        if (bitsStored <= 8)
            dataset.putAndInsertUint8Array(DCM_PixelData, OFstatic_cast(Uint8 *, OFconst_cast(void *, pixel)), count);
        else
            dataset.putAndInsertUint16Array(DCM_PixelData, OFstatic_cast(Uint16 *, OFconst_cast(void *, pixel)), count);
        /* update other DICOM attributes */
        updateImagePixelModuleAttributes(dataset);
        result = 1;
    }
    return result;
}


int DiImage::writeBMP(FILE *stream,
                      const unsigned long frame,
                      const int bits)
{
    int result = 0;
    if ((stream != NULL) && ((bits == 8) || (bits == 24)))
    {
        /* create device independent bitmap: palette (8) or truecolor (24) */
        void *data = NULL;
        const unsigned long bytes = createDIB(data, 0, frame, bits, 1 /*upsideDown*/);
        if ((data != NULL) && (bytes > 0))
        {
            /* number of bytes */
            SB_BitmapFileHeader fileHeader;
            SB_BitmapInfoHeader infoHeader;
            Uint32 *palette = (bits == 8) ? new Uint32[256] : OFstatic_cast(Uint32 *, NULL);
            /* fill bitmap file header with data */
            fileHeader.bfType[0] = 'B';
            fileHeader.bfType[1] = 'M';
            fileHeader.bfSize = 14 /*sizeof(SB_BitmapFileHeader)*/ + 40 /*sizeof(SB_BitmapInfoHeader)*/ + bytes;
            fileHeader.bfReserved1 = 0;
            fileHeader.bfReserved2 = 0;
            fileHeader.bfOffBits = 14 /*sizeof(SB_BitmapFileHeader)*/ + 40 /*sizeof(SB_BitmapInfoHeader)*/;
            /* fill bitmap info header with data */
            infoHeader.biSize = 40 /*sizeof(SB_BitmapInfoHeader)*/;
            infoHeader.biWidth = Columns;
            infoHeader.biHeight = Rows;
            infoHeader.biPlanes = 1;
            infoHeader.biBitCount = bits;
            infoHeader.biCompression = 0;
            infoHeader.biSizeImage = 0;
            infoHeader.biXPelsPerMeter = 0;
            infoHeader.biYPelsPerMeter = 0;
            infoHeader.biClrUsed = 0;
            infoHeader.biClrImportant = 0;
            /* create and fill color palette */
            if (palette != NULL)
            {
                /* add palette size */
                fileHeader.bfSize += 256 * 4;
                fileHeader.bfOffBits += 256 * 4;
                /* fill palette entries with gray values, format: B-G-R-0 */
                for (Uint32 i = 0; i < 256; ++i)
                    palette[i] = (i << 16) | (i << 8) | i;
            }
            /* swap bytes if necessary */
            if (gLocalByteOrder != EBO_LittleEndian)
            {
                /* other data elements are always '0' and, therefore, can remain as they are */
                swap4Bytes(OFreinterpret_cast(Uint8 *, &fileHeader.bfSize));
                swap4Bytes(OFreinterpret_cast(Uint8 *, &fileHeader.bfOffBits));
                swap4Bytes(OFreinterpret_cast(Uint8 *, &infoHeader.biSize));
                swap4Bytes(OFreinterpret_cast(Uint8 *, &infoHeader.biWidth));
                swap4Bytes(OFreinterpret_cast(Uint8 *, &infoHeader.biHeight));
                swap2Bytes(OFreinterpret_cast(Uint8 *, &infoHeader.biPlanes));
                swap2Bytes(OFreinterpret_cast(Uint8 *, &infoHeader.biBitCount));
                if (palette != NULL)
                    swapBytes(OFreinterpret_cast(Uint8 *, palette), 256 * 4 /*byteLength*/, 4 /*valWidth*/);
            }
            /* write bitmap file header: do not write the struct because of 32-bit alignment */
            fwrite(&fileHeader.bfType, sizeof(fileHeader.bfType), 1, stream);
            fwrite(&fileHeader.bfSize, sizeof(fileHeader.bfSize), 1, stream);
            fwrite(&fileHeader.bfReserved1, sizeof(fileHeader.bfReserved1), 1, stream);
            fwrite(&fileHeader.bfReserved2, sizeof(fileHeader.bfReserved2), 1, stream);
            fwrite(&fileHeader.bfOffBits, sizeof(fileHeader.bfOffBits), 1, stream);
            /* write bitmap info header: do not write the struct because of 32-bit alignment  */
            fwrite(&infoHeader.biSize, sizeof(infoHeader.biSize), 1, stream);
            fwrite(&infoHeader.biWidth, sizeof(infoHeader.biWidth), 1, stream);
            fwrite(&infoHeader.biHeight, sizeof(infoHeader.biHeight), 1, stream);
            fwrite(&infoHeader.biPlanes, sizeof(infoHeader.biPlanes), 1, stream);
            fwrite(&infoHeader.biBitCount, sizeof(infoHeader.biBitCount), 1, stream);
            fwrite(&infoHeader.biCompression, sizeof(infoHeader.biCompression), 1, stream);
            fwrite(&infoHeader.biSizeImage, sizeof(infoHeader.biSizeImage), 1, stream);
            fwrite(&infoHeader.biXPelsPerMeter, sizeof(infoHeader.biXPelsPerMeter), 1, stream);
            fwrite(&infoHeader.biYPelsPerMeter, sizeof(infoHeader.biYPelsPerMeter), 1, stream);
            fwrite(&infoHeader.biClrUsed, sizeof(infoHeader.biClrUsed), 1, stream);
            fwrite(&infoHeader.biClrImportant, sizeof(infoHeader.biClrImportant), 1, stream);
            /* write color palette (if applicable) */
            if (palette != NULL)
                fwrite(palette, 4, 256, stream);
            /* write pixel data */
            fwrite(data, 1, OFstatic_cast(size_t, bytes), stream);
            /* delete color palette */
            delete[] palette;
            result = 1;
        }
        /* delete pixel data */
        delete OFstatic_cast(char *, data);     // type cast necessary to avoid compiler warnings using gcc >2.95
    }
    return result;
}


/*
 *
 * CVS/RCS Log:
 * $Log: diimage.cc,v $
 * Revision 1.32  2005/12/08 15:42:51  meichel
 * Changed include path schema for all DCMTK header files
 *
 * Revision 1.31  2005/12/01 09:52:07  joergr
 * Added heuristics for images where the value of HighBit is 0.
 *
 * Revision 1.30  2005/03/09 17:41:16  joergr
 * Added heuristics for images where the attribute HighBit is missing.
 * Fixed possibly uninitialized variable when determining the pixel height.
 *
 * Revision 1.29  2004/09/22 11:35:01  joergr
 * Introduced new member variable "TotalNumberOfFrames".
 *
 * Revision 1.28  2004/03/16 08:18:54  joergr
 * Added support for non-standard encoding of pixel data (OB with BitsAllocated
 * > 8 and <= 16).
 *
 * Revision 1.27  2004/02/06 11:09:21  joergr
 * Fixed inconsistency in re-calculation of PixelSpacing and ImagerPixelSpacing
 * when scaling images.
 *
 * Revision 1.26  2003/12/23 16:03:18  joergr
 * Replaced post-increment/decrement operators by pre-increment/decrement
 * operators where appropriate (e.g. 'i++' by '++i').
 *
 * Revision 1.25  2003/12/08 17:17:04  joergr
 * Adapted type casts to new-style typecast operators defined in ofcast.h.
 *
 * Revision 1.24  2002/12/13 09:28:22  joergr
 * Added explicit type cast to pointer initialization to avoid warning reported
 * by gcc 2.7.2.1.
 *
 * Revision 1.23  2002/12/04 10:41:23  meichel
 * Changed toolkit to use OFStandard::ftoa instead of sprintf for all
 *   double to string conversions that are supposed to be locale independent
 *
 * Revision 1.22  2002/11/26 14:48:12  joergr
 * Added Smallest/LargestImagePixelValue to the list of attributes to be
 * removed from a newly created dataset.
 *
 * Revision 1.21  2002/08/02 15:04:53  joergr
 * Enhanced writeFrameToDataset() routine (remove out-data DICOM attributes
 * from the dataset).
 * Re-compute Imager/Pixel Spacing and Pixel Aspect Ratio for scaled images.
 *
 * Revision 1.20  2002/06/26 16:11:12  joergr
 * Added support for polarity flag to color images.
 * Added new method to write a selected frame to a DICOM dataset (incl. required
 * attributes from the "Image Pixel Module").
 *
 * Revision 1.19  2001/11/29 16:59:53  joergr
 * Fixed bug in dcmimgle that caused incorrect decoding of some JPEG compressed
 * images (certain DICOM attributes, e.g. photometric interpretation, might
 * change during decompression process).
 *
 * Revision 1.18  2001/11/27 18:21:38  joergr
 * Added support for plugable output formats in class DicomImage. First
 * implementation is JPEG.
 *
 * Revision 1.17  2001/11/19 12:57:17  joergr
 * Adapted code to support new dcmjpeg module and JPEG compressed images.
 *
 * Revision 1.16  2001/11/13 18:01:41  joergr
 * Added type cast to delete a void pointer to keep gcc 2.95 compiler quiet.
 *
 * Revision 1.15  2001/11/09 16:29:04  joergr
 * Added support for Windows BMP file format.
 *
 * Revision 1.14  2001/09/28 13:14:22  joergr
 * Corrected wrong warning message regarding the optional RepresentativeFrame
 * Number.
 *
 * Revision 1.13  2001/06/01 15:49:55  meichel
 * Updated copyright header
 *
 * Revision 1.12  2000/05/25 10:35:02  joergr
 * Removed ununsed variable from parameter list (avoid compiler warnings).
 *
 * Revision 1.11  2000/05/03 09:47:23  joergr
 * Removed most informational and some warning messages from release built
 * (#ifndef DEBUG).
 *
 * Revision 1.10  2000/04/28 12:33:44  joergr
 * DebugLevel - global for the module - now derived from OFGlobal (MF-safe).
 *
 * Revision 1.9  2000/04/27 13:10:27  joergr
 * Dcmimgle library code now consistently uses ofConsole for error output.
 *
 * Revision 1.8  2000/03/08 16:24:28  meichel
 * Updated copyright header.
 *
 * Revision 1.7  2000/03/03 14:09:18  meichel
 * Implemented library support for redirecting error messages into memory
 *   instead of printing them to stdout/stderr for GUI applications.
 *
 * Revision 1.6  1999/10/06 13:45:55  joergr
 * Corrected creation of PrintBitmap pixel data: VOI windows should be applied
 * before clipping to avoid that the region outside the image (border) is also
 * windowed (this requires a new method in dcmimgle to create a DicomImage
 * with the grayscale transformations already applied).
 *
 * Revision 1.5  1999/09/17 13:15:20  joergr
 * Corrected typos and formatting.
 *
 * Revision 1.4  1999/07/23 14:21:31  joergr
 * Corrected bug in method 'detachPixelData' (data has never really been
 * removed from memory).
 *
 * Revision 1.2  1999/04/28 15:01:44  joergr
 * Introduced new scheme for the debug level variable: now each level can be
 * set separately (there is no "include" relationship).
 *
 * Revision 1.1  1998/11/27 16:01:43  joergr
 * Added copyright message.
 * Added methods and constructors for flipping and rotating, changed for
 * scaling and clipping.
 * Added method to directly create java AWT bitmaps.
 * Introduced global debug level for dcmimage module to control error output.
 * Renamed variable 'Status' to 'ImageStatus' because of possible conflicts
 * with X windows systems.
 * Added method to detach pixel data if it is no longer needed.
 *
 * Revision 1.7  1998/06/25 08:51:41  joergr
 * Removed some wrong newline characters.
 *
 * Revision 1.6  1998/05/11 14:52:29  joergr
 * Added CVS/RCS header to each file.
 *
 *
 */
