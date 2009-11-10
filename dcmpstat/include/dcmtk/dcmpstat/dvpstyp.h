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
 *    enums: DVPSoverlayActivation, DVPSVOIActivation, DVPSGraphicLayering
 *           DVPSPresentationLUTType, DVPSRotationType, 
 *           DVPSShutterType
 *
 *  Last Update:      $Author: meichel $
 *  Update Date:      $Date: 2005/12/08 16:04:10 $
 *  CVS/RCS Revision: $Revision: 1.18 $
 *  Status:           $State: Exp $
 *
 *  CVS/RCS Log at end of file
 *
 */

#ifndef __DVPSTYP_H__
#define __DVPSTYP_H__

#include "dcmtk/config/osconfig.h"    /* make sure OS specific configuration is included first */

#define INCLUDE_CSTDIO
#include "dcmtk/ofstd/ofstdinc.h"

/** describes how to handle overlays when creating a default presentation state
 *  for an image. 
 */
enum DVPSoverlayActivation
{
  /** ignore overlays even if present in the image.
   *  A presentation state without any embedded or activated overlays
   *  is created.
   */
  DVPSO_ignoreOverlays,
  /** if overlays are present in the image, activate but do not copy them.
   *  If the image contains overlays, overlay activation elements are
   *  created in the presentation state object. The overlays remain in the image.
   */
  DVPSO_referenceOverlays,
  /** if overlays are present in the image, copy them.
   *  If the image contains overlays which are not embedded in the pixel
   *  data but use the OverlayData element, the overlays are copied to the
   *  presentation state and activated. 
   *  Overlays that are embedded in the image pixel data are not copied
   *  but also activated.
   */ 
  DVPSO_copyOverlays
};


/** describes how to handle VOI transformations when creating a default presentation state
 *  for an image. 
 */
enum DVPSVOIActivation
{
  /** ignore VOI transformations even if present in the image.
   *  The presentation state will not contain any VOI transformation.
   */
  DVPSV_ignoreVOI,
  /** select the first VOI window if present, VOI LUT alternatively.
   *  If the image contains one or more settings for window center and
   *  window width, the first window center/width is copied to the presentation
   *  state. If the image contains no window center/width but does contain
   *  one or more VOI LUTs, the first VOI LUT is copied to the presentation
   *  state.
   */
  DVPSV_preferVOIWindow,
  /** select the first VOI LUT if present, VOI Window alternatively.
   *  If the image contains one or more VOI LUTs, 
   *  the first VOI LUT is copied to the presentation
   *  state. If the image contains no VOI LUT but does contain
   *  one or more settings for window center/width, the first 
   *  window center and width are copied to the presentation state.
   */
  DVPSV_preferVOILUT
};

/** describes how to handle layering of curves and overlays 
 *  when creating a default presentation state for an image. 
 */
enum DVPSGraphicLayering
{
  /** put all overlays and curves into a single graphic layer.
   *  If curves and/or overlays are present, a single graphic layer is
   *  created and all curves and overlays are assigned to this single layer.
   */
  DVPSG_oneLayer,
  /** create one graphic layer for overlays and one graphic layer for curves on top of that.
   *  If overlays are present, one layer containing all overlays is created.
   *  If curves are present, a different layer containing all curves is created.
   *  If both curves and overlays are present, the curves are assigned the higher layer order.
   */
  DVPSG_twoLayers,
  /** create individual layers for each referenced element, curves on top of overlays.
   *  For each overlay and curve present, a separate graphic layer is created.
   *  The curve layers have higher order than the overlay layers.
   */
  DVPSG_separateLayers
};

/** describes a type of presentation LUT that is currently
 *  being used or set in a presentation state.
 */
enum DVPSPresentationLUTType
{
  /** Presentation LUT Shape with value 'IDENTITY'
   */
  DVPSP_identity,
  /** Presentation LUT Shape with value 'INVERSE'
   */
  DVPSP_inverse,
  /** Presentation LUT look up table
   */
  DVPSP_table,
  /** Presentation LUT Shape with value 'LIN OD'
   */
  DVPSP_lin_od 
  
};

/** some Print SCPs which support Presentation LUTs require that the number
 *  of entries in a Presentation LUT matches the bit depth of the image pixel
 *  data (4096 entries for 12 bit pixel data, 256 entries for 8 bit pixel 
 *  data). An instance of this enumeration describes the characteristics
 *  of a Presentation LUT with regard to this matching rule.
 */
enum DVPSPrintPresentationLUTAlignment
{
  /** Presentation LUT Shape, matches all kinds of image data
   */
  DVPSK_shape,
  /** Presentation LUT with 256 entries and first entry mapped to 0, 
   *  matches 8 bit image data
   */
  DVPSK_table8,
  /** Presentation LUT with 4096 entries and first entry mapped to 0, 
   *  matches 12 bit image data
   */
  DVPSK_table12,
  /** Presentation LUT Shape with number of entries other than 256 or 4096
   *  or with first entry mapped to anything but 0.
   */
  DVPSK_other
};

/** describes the rotation status of a presentation state.
 */
enum DVPSRotationType
{
  /** no rotation
   */
  DVPSR_0_deg,
  /** rotation of 90 degrees
   */
  DVPSR_90_deg,
  /** rotation of 180 degrees
   */
  DVPSR_180_deg,
  /** rotation of 270 degrees
   */
  DVPSR_270_deg
};

/** describes the different types of display shutters
 */
enum DVPSShutterType
{
  /** rectangular shutter
   */
  DVPSU_rectangular,
  /** circular shutter
   */
  DVPSU_circular,
  /** polygonal shutter
   */
  DVPSU_polygonal,
  /** bitmap shutter
   */
  DVPSU_bitmap 
};

/** describes the different types of annotation units
 */
enum DVPSannotationUnit
{
  /** pixels
   */
  DVPSA_pixels,
  /** fraction of specified display area
   */
  DVPSA_display
};

/** describes the specific character set of a DICOM element.
 *  The defined terms for code extension techniques are
 *  not supported.
 */
enum DVPScharacterSet
{
  /** ISO 646 (ISO-IR 6): ASCII
   */
  DVPSC_ascii,
  /** ISO-IR 100: Latin alphabet No. 1
   */
  DVPSC_latin1,
  /** ISO-IR 101: Latin alphabet No. 2
   */
  DVPSC_latin2,
  /** ISO-IR 109: Latin alphabet No. 3
   */
  DVPSC_latin3,
  /** ISO-IR 110: Latin alphabet No. 4
   */
  DVPSC_latin4,
  /** ISO-IR 148: Latin alphabet No. 5
   */
  DVPSC_latin5,
  /** ISO-IR 144: Cyrillic
   */
  DVPSC_cyrillic,
  /** ISO-IR 127: Arabic
   */
  DVPSC_arabic,
  /** ISO-IR 126: Greek
   */
  DVPSC_greek,
  /** ISO-IR 138: Hebrew
   */
  DVPSC_hebrew,
  /** ISO-IR 13: Japanese (Katakana/Romaji)
   */
  DVPSC_japanese,
  /** unrecognized term or code extension
   */
  DVPSC_other 
};

/** describes the different types of graphic objects
 */
enum DVPSGraphicType
{
  /** single point
   */
  DVPST_point,
  /** non-interpolated polygonal line
   */
  DVPST_polyline,
  /** interpolated polygonal line
   */
  DVPST_interpolated,
  /** circle
   */
  DVPST_circle,
  /** ellipse
   */
  DVPST_ellipse
};

/** describes a curve type
 */
enum DVPSCurveType
{
  /** region of interest (ROI) - a closed polygonal line
   */
  DVPSL_roiCurve,
  /** polyline - an open polygonal line
   */
  DVPSL_polylineCurve
};

/** describes the horizontal justification of a text box
 */
enum DVPSTextJustification
{
  /** left justified text
   */
  DVPSX_left,
  /** right justified text
   */
  DVPSX_right,
  /** centered text
   */
  DVPSX_center
};

/** describes the images and frames to which
 *  an object (graphic layer, displayed area selection or VOI) is applicable
 */
enum DVPSObjectApplicability
{
  /** the object only applies to the current (selected) frame of the current (attached) image
   */
  DVPSB_currentFrame,
  /** the object applies to all frames of the current (attached) image
   */
  DVPSB_currentImage,
  /** the object applies to all frames of all referenced images
   */
  DVPSB_allImages
};

/** describes the presentation size mode for a displayed area selection
 */
enum DVPSPresentationSizeMode
{
  /** the displayed area should be scaled to fill the screen
   */
  DVPSD_scaleToFit,
  /** the displayed area should be scaled to its true physical size
   */
  DVPSD_trueSize,
  /** the displayed area should be scaled to a fixed scaling factor
   */
  DVPSD_magnify
};

/** describes the service type supported by a DICOM communication peer
 */
enum DVPSPeerType
{
  /** Storage SCP peer
   */
  DVPSE_storage,
  /** local Storage SCP
   */
  DVPSE_receiver,
  /** remote Print Management SCP
   */
  DVPSE_printRemote,
  /** local Print Management SCP
   */
  DVPSE_printLocal,
  /** local or remote Print Management SCP
   */
  DVPSE_printAny,
  /** any type of peer
   */
  DVPSE_any
};

/** describes the orientation (portrait or landscape) of a basic film box
 */
enum DVPSFilmOrientation
{
  /** portrait orientation
   */
  DVPSF_portrait,
  /** landscape orientation
   */
  DVPSF_landscape,
  /** printer default
   */
  DVPSF_default
};

/** describes the trim mode (printing of borders around image boxes) for a basic film box
 */
enum DVPSTrimMode
{
  /** print with trims (borders)
   */
  DVPSH_trim_on,
  /** print without trims (borders)
   */
  DVPSH_trim_off,
  /** printer default
   */
  DVPSH_default
};

/** describes the decimate/crop behaviour for a basic image box
 */
enum DVPSDecimateCropBehaviour
{
  /** a magnification factor less than one to be applied to the image.
   */
  DVPSI_decimate,
  /** some image rows and/or columns are to be deleted before printing.
   */
  DVPSI_crop,
  /** the SCP shall not crop or decimate
   */
  DVPSI_fail,
  /** printer default
   */
  DVPSI_default
};

/** describes the type of display function
 */
enum DVPSDisplayTransform
{
  /** first entry
   */
  DVPSD_first=0,
  /** Grayscale Standard Display Function (defined in DICOM part 14)
   */
  DVPSD_GSDF=DVPSD_first,
  /** CIE Lab
   */
  DVPSD_CIELAB=1,
  /** no display transform
   */
  DVPSD_none=2,
  /** number of display transforms
   */
  DVPSD_max=DVPSD_none
};

/** describes the different status levels used for the log file messages.
 *  The assigned values need to be the same as in OFLogFile:LF_Level!
 */
enum DVPSLogMessageLevel
{
  /** no log messages (only used as a filter)
   */  
  DVPSM_none = 0,
  /** only error messages
   */  
  DVPSM_error = 1,
  /** warning messages (includes DVPSM_error)
   */  
  DVPSM_warning = 2,
  /** informational messages (includes DVPSM_warning)
   */  
  DVPSM_informational = 3,
  /** debug messages (includes DVPSM_informational)
   */  
  DVPSM_debug = 4
};

/** describes the result of an association negotiation
 */
enum DVPSAssociationNegotiationResult
{
  /** negotiation was successful
   */
  DVPSJ_success,
  /** negotiation was unsuccessful
   */
  DVPSJ_error,
  /** peer requests termination of server process
   */
  DVPSJ_terminate
};

/** describes the bit depth of a Basic Grayscale Image Box
 */
enum DVPSImageDepth
{
  /** not yet assigned
   */
  DVPSN_undefined,
  /** 8 bit
   */
  DVPSN_8bit,
  /** 12 bit
   */
  DVPSN_12bit
};

/** describes the certificate verification policy for TLS association negotiation
 */
enum DVPSCertificateVerificationType
{
  /** verify peer certificate, refuse transmission if absent
   */
  DVPSQ_require,
  /** verify peer certificate if present
   */
  DVPSQ_verify,
  /** don't verify peer certificate
   */
  DVPSQ_ignore
};


/** describes the types of objects handled by the dcmpstat signature routines
 */
enum DVPSObjectType
{
  /** structured report
   */
  DVPSS_structuredReport,
  /** image
   */
  DVPSS_image,
  /** grayscale softcopy presentation state
   */
  DVPSS_presentationState
};


/** describes the types of objects handled by the dcmpstat signature routines
 */
enum DVPSSignatureStatus
{
  /** no digital signatures are present
   */
  DVPSW_unsigned,

  /** one or more digital signatures are present and have been successfully verified
   */
  DVPSW_signed_OK,
  
  /** one or more digital signatures are present, and all of them are valid.
   *  However, at least one of them was created
   *  with a certificate issued by an unknown CA.
   */
  DVPSW_signed_unknownCA,

  /** one or more digital signatures are present and at least one of them 
   *  could not be successfully verified because it was corrupt.
   */
  DVPSW_signed_corrupt
};


/** describes the mode to verify and sign structured reports
 */
enum DVPSVerifyAndSignMode
{
  /** verify the document only
   */
  DVPSY_verify,

  /** verify and digitally sign the document (apart from VerifyingObserver and SOPInstanceUID)
   */
  DVPSY_verifyAndSign,

  /** verify and digitally sign the entire document (finalize it)
   */
  DVPSY_verifyAndSign_finalize
};


#endif

/*
 *  $Log: dvpstyp.h,v $
 *  Revision 1.18  2005/12/08 16:04:10  meichel
 *  Changed include path schema for all DCMTK header files
 *
 *  Revision 1.17  2002/11/27 15:48:01  meichel
 *  Adapted module dcmpstat to use of new header file ofstdinc.h
 *
 *  Revision 1.16  2001/01/29 17:32:31  joergr
 *  Added method to verify and digitally sign structured reports.
 *
 *  Revision 1.15  2001/01/26 10:43:11  meichel
 *  Introduced additional (fourth) status flag for signature validation
 *    describing signatures that are valid but untrustworthy (unknown CA).
 *
 *  Revision 1.14  2001/01/25 15:18:05  meichel
 *  Added initial support for verification of digital signatures
 *    in presentation states, images and structured reports to module dcmpstat.
 *
 *  Revision 1.13  2000/10/10 12:23:41  meichel
 *  Added extensions for TLS encrypted communication
 *
 *  Revision 1.12  2000/07/07 13:38:45  joergr
 *  Removed unused enum type.
 *
 *  Revision 1.11  2000/06/05 16:22:52  joergr
 *  Implemented log message methods.
 *
 *  Revision 1.10  2000/05/31 12:56:40  meichel
 *  Added initial Print SCP support
 *
 *  Revision 1.9  2000/05/30 13:48:00  joergr
 *  Added interface methods to support the following new features:
 *    - write/filter log messages (not yet implemented)
 *
 *  Revision 1.8  2000/03/08 16:28:58  meichel
 *  Updated copyright header.
 *
 *  Revision 1.7  1999/09/10 09:02:33  joergr
 *  Added support for CIELAB display function. New methods to handle display
 *  functions. Old methods are marked as retired and should be removed asap.
 *
 *  Revision 1.6  1999/09/10 07:36:39  thiel
 *  Added Presentation LUT Shape LIN OD
 *
 *  Revision 1.5  1999/08/31 14:09:13  meichel
 *  Added get/set methods for stored print attributes
 *
 *  Revision 1.4  1999/07/22 16:39:15  meichel
 *  Adapted dcmpstat data structures and API to supplement 33 letter ballot text.
 *
 *  Revision 1.3  1998/12/22 17:57:08  meichel
 *  Implemented Presentation State interface for overlays,
 *    VOI LUTs, VOI windows, curves. Added test program that
 *    allows to add curve data to DICOM images.
 *
 *  Revision 1.2  1998/12/14 16:10:37  meichel
 *  Implemented Presentation State interface for graphic layers,
 *    text and graphic annotations, presentation LUTs.
 *
 *  Revision 1.1  1998/11/27 14:50:36  meichel
 *  Initial Release.
 *
 *
 */
