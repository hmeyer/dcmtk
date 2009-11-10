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
 *  Module: dcmpstat
 *
 *  Author: Marco Eichelberg
 *
 *  Purpose:
 *    classes: DVPSPresentationLUT 
 *
 *  Last Update:      $Author: meichel $
 *  Update Date:      $Date: 2005/12/08 15:46:39 $
 *  CVS/RCS Revision: $Revision: 1.3 $
 *  Status:           $State: Exp $
 *
 *  CVS/RCS Log at end of file
 *
 */

#include "dcmtk/config/osconfig.h"    /* make sure OS specific configuration is included first */
#include "dcmtk/ofstd/ofstring.h"
#include "dcmtk/dcmpstat/dvpspl.h"
#include "dcmtk/dcmimgle/dcmimage.h"    /* for class DiLookupTable, DicomImage */
#include "dcmtk/dcmpstat/dvpsdef.h"     /* for constants and macros */
#include "dcmtk/dcmnet/dimse.h"

/* --------------- class DVPSPresentationLUT --------------- */

OFBool DVPSPresentationLUT::compareDiLookupTable(DiLookupTable *lut)
{
  if ((presentationLUT == DVPSP_table) && lut 
     && (0 == lut->compareLUT(presentationLUTData, presentationLUTDescriptor))) return OFTrue;
  return OFFalse;
}

DiLookupTable *DVPSPresentationLUT::createDiLookupTable()
{
  DiLookupTable *result = NULL;
  if (presentationLUT == DVPSP_table) result = new DiLookupTable(presentationLUTData, presentationLUTDescriptor);
  return result;
}  

OFCondition DVPSPresentationLUT::invert()
{
  OFCondition status = EC_Normal;
  switch (presentationLUT)
  {
      case DVPSP_identity:
          presentationLUT = DVPSP_inverse;
          break;
      case DVPSP_inverse:
          presentationLUT = DVPSP_identity;
          break;
      case DVPSP_table:
          status = EC_IllegalCall;
          if (haveTable())
          {
              DiLookupTable *lut = new DiLookupTable(presentationLUTData, presentationLUTDescriptor);
              if (lut && (lut->mirrorTable(0x2))) status = EC_Normal;       // flag = 0x2: mirror only original LUT data
              delete lut;
          }
          break;
      case DVPSP_lin_od:
          status = EC_IllegalCall;
          break;
          
  }
  return status;
}

OFBool DVPSPresentationLUT::activate(DicomImage *image, OFBool printLUT)
{
  if (image==NULL) return OFFalse;

  int result=0;
  switch (presentationLUT)
  {   
    case DVPSP_identity:
      if (printLUT)
      {
        // in DICOM print, IDENTITY should not invert a MONOCHROME1 image
        result = image->setPresentationLutShape(ESP_Default);
      }
      else 
      {
      	result = image->setPresentationLutShape(ESP_Identity);
      }
      if ((!result) && verboseMode)
      {
        logstream->lockCerr() << "warning: unable to set identity presentation LUT shape, ignoring." << endl;
        logstream->unlockCerr();
      }
      break;
    case DVPSP_inverse:
      if (!printLUT)
        result = image->setPresentationLutShape(ESP_Inverse);
      if ((!result) && verboseMode)
      {
        logstream->lockCerr() << "warning: unable to set inverse presentation LUT shape, ignoring." << endl;
        logstream->unlockCerr();
      }
      break;      
    case DVPSP_lin_od:
      result = image->setPresentationLutShape(ESP_LinOD);
      if ((!result) && verboseMode)
      {
        logstream->lockCerr() << "warning: unable to set linear optical density presentation LUT shape, ignoring." << endl;
        logstream->unlockCerr();
      }
      break;
    case DVPSP_table:
      if (printLUT)
        result = image->setVoiLut(presentationLUTData, presentationLUTDescriptor, &presentationLUTExplanation);
      else
        result = image->setPresentationLut(presentationLUTData, presentationLUTDescriptor, &presentationLUTExplanation);
      if ((!result) && verboseMode)
      {
        logstream->lockCerr() << "warning: unable to set presentation LUT, ignoring." << endl;
        logstream->unlockCerr();
      }
      break;
  }
  if (result) return OFTrue; else return OFFalse;
}

OFBool DVPSPresentationLUT::activateInverseLUT(DicomImage *image)
{
  int result = 0;
  if ((image != NULL) && (presentationLUT == DVPSP_table))
  {
      result = image->setInversePresentationLut(presentationLUTData, presentationLUTDescriptor);
      if ((!result) && verboseMode)
      {
        logstream->lockCerr() << "warning: unable to set inverse presentation LUT, ignoring." << endl;
        logstream->unlockCerr();
      }
  }
  if (result) return OFTrue; else return OFFalse;
}

OFBool DVPSPresentationLUT::isInverse()
{
  OFBool result = OFFalse;
  switch (presentationLUT)
  {
    case DVPSP_identity:
    case DVPSP_lin_od:
      break;
    case DVPSP_inverse:
      result = OFTrue;
      break;
    case DVPSP_table:
      if ((presentationLUTDescriptor.getVM()==3)&&(presentationLUTData.getLength() > 0))
      {
        DiLookupTable *lut = new DiLookupTable(presentationLUTData, presentationLUTDescriptor);
        if (lut && (lut->getFirstValue() > lut->getLastValue())) result = OFTrue;
        delete lut;
      }
      break;
  }
  return result;
}

/*
 *  $Log: dvpspl2.cc,v $
 *  Revision 1.3  2005/12/08 15:46:39  meichel
 *  Changed include path schema for all DCMTK header files
 *
 *  Revision 1.2  2003/12/18 17:14:47  meichel
 *  Fixed print preview for MONOCHROME1 images with IDENTITY P-LUT shape
 *
 *  Revision 1.1  2003/08/27 14:59:08  meichel
 *  Moved all methods of class DVPSPresentationLUT that depend on module dcmimgle
 *    into a separate implementation file
 *
 *
 */

