/*
 *
 *  Copyright (C) 1999-2005, OFFIS
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
 *  Module:  dcmpstat
 *
 *  Authors: Marco Eichelberg
 *
 *  Purpose: Presentation State Viewer - Network Send Component (Store SCU)
 *
 *  Last Update:      $Author: meichel $
 *  Update Date:      $Date: 2005/12/08 15:46:11 $
 *  Source File:      $Source: /share/dicom/cvs-depot/dcmtk/dcmpstat/apps/dcmpssnd.cc,v $
 *  CVS/RCS Revision: $Revision: 1.38 $
 *  Status:           $State: Exp $
 *
 *  CVS/RCS Log at end of file
 *
 */

#include "dcmtk/config/osconfig.h"    /* make sure OS specific configuration is included first */

#ifdef HAVE_GUSI_H
#include <GUSI.h>
#endif

BEGIN_EXTERN_C
#ifdef HAVE_FCNTL_H
#include <fcntl.h>       /* for O_RDONLY */
#endif
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>   /* required for sys/stat.h */
#endif
#ifdef HAVE_SYS_STAT_H
#include <sys/stat.h>    /* for stat, fstat */
#endif
END_EXTERN_C

#include "dcmtk/dcmpstat/dvpsdef.h"     /* for constants */
#include "dcmtk/dcmpstat/dvpscf.h"      /* for class DVConfiguration */
#include "dcmtk/ofstd/ofbmanip.h"    /* for OFBitmanipTemplate */
#include "dcmtk/dcmdata/dcuid.h"       /* for dcmtk version name */
#include "dcmtk/dcmnet/diutil.h"
#include "dcmtk/dcmdata/cmdlnarg.h"
#include "dcmtk/ofstd/ofconapp.h"
#include "dcmtk/dcmpstat/dvpshlp.h"     /* for class DVPSHelper */
#include "dcmtk/dcmqrdb/dcmqrdbi.h"    /* for LOCK_IMAGE_FILES */
#include "dcmtk/dcmqrdb/dcmqrdbs.h"    /* for DcmQueryRetrieveDatabaseStatus */
#include "dcmtk/dcmpstat/dvpsmsg.h"
#include "dcmtk/dcmdata/dcdebug.h"

#ifdef WITH_OPENSSL
#include "dcmtk/dcmtls/tlstrans.h"
#include "dcmtk/dcmtls/tlslayer.h"
#endif

#ifdef WITH_ZLIB
#include <zlib.h>        /* for zlibVersion() */
#endif

#include "dcmtk/ofstd/ofstream.h"

#define OFFIS_CONSOLE_APPLICATION "dcmpssnd"

static char rcsid[] = "$dcmtk: " OFFIS_CONSOLE_APPLICATION " v"
  OFFIS_DCMTK_VERSION " " OFFIS_DCMTK_RELEASEDATE " $";

DVPSIPCClient *messageClient  = NULL; // global pointer to IPC message client, if present

/** sends a single DICOM instance over an association which must be already established.
 *  @param assoc DICOM network association
 *  @param sopClass SOP Class UID of the image (used for the C-Store-RQ)
 *  @param sopInstance SOP Instance UID of the image (used for the C-Store-RQ)
 *  @param imgFile path to the image file to be transmitted
 *  @param opt_verbose flag indicating whether "verbose mode" is active.
 *  @return EC_Normal if successful, a different DIMSE code otherwise.
 */
static OFCondition sendImage(T_ASC_Association *assoc, const char *sopClass, const char *sopInstance, const char *imgFile, int opt_verbose)
{
    DcmDataset *statusDetail = NULL;
    T_ASC_PresentationContextID presId=0;
    T_DIMSE_C_StoreRQ req;
    T_DIMSE_C_StoreRSP rsp;

    if (assoc == NULL) return DIMSE_NULLKEY;
    if ((sopClass == NULL)||(strlen(sopClass) == 0)) return DIMSE_NULLKEY;
    if ((sopInstance == NULL)||(strlen(sopInstance) == 0)) return DIMSE_NULLKEY;
    if ((imgFile == NULL)||(strlen(imgFile) == 0)) return DIMSE_NULLKEY;

#ifdef LOCK_IMAGE_FILES
    /* shared lock image file */
#ifdef O_BINARY
    int lockfd = open(imgFile, O_RDONLY | O_BINARY, 0666);
#else
    int lockfd = open(imgFile, O_RDONLY, 0666);
#endif
    if (lockfd < 0)
    {
      if (opt_verbose) CERR << "error: unable to lock image file '" << imgFile << "'" << endl;
      return DIMSE_BADDATA;
    }
    dcmtk_flock(lockfd, LOCK_SH);
#endif

    /* which presentation context should be used */
    presId = ASC_findAcceptedPresentationContextID(assoc, sopClass);
    if (presId == 0)
    {
      if (opt_verbose) CERR << "error: no presentation context for: (" << dcmSOPClassUIDToModality(sopClass) << ") " << sopClass << endl;
      if (messageClient)
      {
        OFString buf("unable to send image: no presentation context for ");
      	const char *sopClassName = dcmFindNameOfUID(sopClass);
        if (sopClassName == NULL) buf.append(sopClass); else buf.append(sopClassName);
        buf.append("\n");
      	messageClient->notifySentDICOMObject(DVPSIPCMessage::statusWarning, buf.c_str());
      }
      return DIMSE_NOVALIDPRESENTATIONCONTEXTID;
    }

    /* start store */
    OFBitmanipTemplate<char>::zeroMem((char *)&req, sizeof(req));
    req.MessageID = assoc->nextMsgID++;
    strcpy(req.AffectedSOPClassUID, sopClass);
    strcpy(req.AffectedSOPInstanceUID, sopInstance);
    req.DataSetType = DIMSE_DATASET_PRESENT;
    req.Priority = DIMSE_PRIORITY_MEDIUM;

    OFCondition cond = DIMSE_storeUser(assoc, presId, &req,
        imgFile, NULL, NULL, NULL, DIMSE_BLOCKING, 0, &rsp, &statusDetail);

#ifdef LOCK_IMAGE_FILES
    /* unlock image file */
    dcmtk_flock(lockfd, LOCK_UN);
    close(lockfd);
#endif

    if (cond.good())
    {
       if (opt_verbose) CERR << "[MsgID " << req.MessageID << "] Complete [Status: "
          << DU_cstoreStatusString(rsp.DimseStatus) << "]" << endl;
    } else {
       if (opt_verbose)
       {
          CERR << "[MsgID " << req.MessageID << "] Failed [Status: "
          << DU_cstoreStatusString(rsp.DimseStatus) << "]" << endl;
          DimseCondition::dump(cond);
       }
    }
    if (statusDetail) delete statusDetail;

    if (messageClient)
    {
      OFOStringStream out;
      Uint32 operationStatus = DVPSIPCMessage::statusError;
      if (cond.good())
      {
        if (rsp.DimseStatus == STATUS_Success) operationStatus = DVPSIPCMessage::statusOK;
        else operationStatus = DVPSIPCMessage::statusWarning;
      }
      const char *sopClassName = dcmFindNameOfUID(sopClass);
      const char *successName = "failed";
      if (operationStatus == DVPSIPCMessage::statusOK) successName = "successful";
      if (sopClassName==NULL) sopClassName = sopClass;
      unsigned long fileSize = 0;
      struct stat fileStat;
      if (0 == stat(imgFile, &fileStat)) fileSize = fileStat.st_size;
      out << "DICOM C-STORE transmission " << successName << ": " << endl
          << "\tSOP class UID          : " << sopClassName << endl
          << "\tSOP instance UID       : " << sopInstance << endl
          << "\tSource file path       : " << imgFile << endl
          << "\tSource file size (kB)  : " << (fileSize+1023)/1024 << endl
          << "\tDIMSE presentation ctx : " << (int)presId << endl
          << "\tDIMSE message ID       : " << req.MessageID << endl
          << "\tDIMSE status           : " << DU_cstoreStatusString(rsp.DimseStatus) << endl
          << OFStringStream_ends;
      OFSTRINGSTREAM_GETSTR(out, theString)
      messageClient->notifySentDICOMObject(operationStatus, theString);
      OFSTRINGSTREAM_FREESTR(theString)
    }
    return cond;
}

/** sends a complete DICOM study, series or a single instance
 *  over an association which must be already established.
 *  The instances (files) to be transmitted are derived from the database.
 *  @param handle open database handle
 *  @param assoc DICOM network association
 *  @param studyUID Study Instance UID of the study/series/image to be transmitted.
 *  @param seriesUID Series Instance UID of the series/image to be transmitted.
 *    If NULL, a complete study is transmitted.
 *  @param instanceUID SOP Instance UID of the image to be transmitted.
 *    If NULL, a complete series is transmitted.
 *  @param opt_verbose flag indicating whether "verbose mode" is active.
 *  @return EC_Normal if successful, a different DIMSE code otherwise.
 */

static OFCondition sendStudy(
  DcmQueryRetrieveIndexDatabaseHandle &handle,
  T_ASC_Association *assoc,
  const char *studyUID,
  const char *seriesUID,
  const char *instanceUID,
  int opt_verbose)
{
    if ((assoc==NULL)||(studyUID==NULL)) return DIMSE_NULLKEY;

    /* build query */
    DcmDataset query;
    OFCondition cond = DVPSHelper::putStringValue(&query, DCM_StudyInstanceUID, studyUID);
    if (cond.bad()) return cond;
    if (seriesUID && instanceUID)
    {
      cond = DVPSHelper::putStringValue(&query, DCM_QueryRetrieveLevel, "IMAGE");
      if (cond.bad()) return cond;
      cond = DVPSHelper::putStringValue(&query, DCM_SeriesInstanceUID, seriesUID);
      if (cond.bad()) return cond;
      cond = DVPSHelper::putStringValue(&query, DCM_SOPInstanceUID, instanceUID);
      if (cond.bad()) return cond;
      if (opt_verbose)
      {
        CERR << "Sending at IMAGE level:" << endl
             << "  Study Instance UID : " << studyUID << endl
             << "  Series Instance UID: " << seriesUID << endl
             << "  SOP Instance UID   : " << instanceUID << endl << endl;
      }
    }
    else if (seriesUID)
    {
      cond = DVPSHelper::putStringValue(&query, DCM_QueryRetrieveLevel, "SERIES");
      if (cond.bad()) return cond;
      cond = DVPSHelper::putStringValue(&query, DCM_SeriesInstanceUID, seriesUID);
      if (cond.bad()) return cond;
      if (opt_verbose)
      {
        CERR << "Sending at SERIES level:" << endl
             << "  Study Instance UID : " << studyUID << endl
             << "  Series Instance UID: " << seriesUID << endl << endl;
      }
    }
    else
    {
      cond = DVPSHelper::putStringValue(&query, DCM_QueryRetrieveLevel, "STUDY");
      if (cond.bad()) return cond;
      if (opt_verbose)
      {
        CERR << "Sending at STUDY level:" << endl
             << "  Study Instance UID : " << studyUID << endl << endl;
      }
    }

    DcmQueryRetrieveDatabaseStatus dbStatus(STATUS_Pending);
    DIC_UI sopClass;
    DIC_UI sopInstance;
    char imgFile[MAXPATHLEN+1];
    DIC_US nRemaining = 0;

    cond = handle.startMoveRequest(UID_MOVEStudyRootQueryRetrieveInformationModel, &query, &dbStatus);
    if (cond.bad()) return cond;

    while (dbStatus.status() == STATUS_Pending)
    {
      cond = handle.nextMoveResponse(sopClass, sopInstance, imgFile, &nRemaining, &dbStatus);
      if (cond.bad()) return cond;

      if (dbStatus.status() == STATUS_Pending)
      {
        cond = sendImage(assoc, sopClass, sopInstance, imgFile, opt_verbose);
        if (cond.bad())
        {
          handle.cancelMoveRequest(&dbStatus);
          return cond;
        }
      }
    }
    return cond;
}

/** adds presentation contexts for all storage SOP classes
 *  to the association parameters.
 *  If the opt_implicitOnly flag is set, only Implicit VR Little Endian
 *  is offered as transfer syntax. Otherwise, three xfer syntaxes are offered:
 *  first the explicit VR with local byte ordering, followed by explicit VR
 *  with opposite byte ordering, followed by implicit VR little endian.
 *  @param params parameter set to which presentation contexts are added
 *  @param opt_implicitOnly flag defining whether only Implicit VR Little Endian
 *    should be offered as xfer syntax.
 *  @return EC_Normal upon success, an error code otherwise.
 */
static OFCondition addAllStoragePresentationContexts(T_ASC_Parameters *params, int opt_implicitOnly)
{
    OFCondition cond = EC_Normal;
    int pid = 1;

    const char* transferSyntaxes[3];
    int transferSyntaxCount = 0;

    if (opt_implicitOnly)
    {
        transferSyntaxes[0] = UID_LittleEndianImplicitTransferSyntax;
        transferSyntaxCount = 1;
    } else {
        /* gLocalByteOrder is defined in dcxfer.h */
        if (gLocalByteOrder == EBO_LittleEndian) {
            /* we are on a little endian machine */
            transferSyntaxes[0] = UID_LittleEndianExplicitTransferSyntax;
            transferSyntaxes[1] = UID_BigEndianExplicitTransferSyntax;
        } else {
            /* we are on a big endian machine */
            transferSyntaxes[0] = UID_BigEndianExplicitTransferSyntax;
            transferSyntaxes[1] = UID_LittleEndianExplicitTransferSyntax;
        }
        transferSyntaxes[2] = UID_LittleEndianImplicitTransferSyntax;
        transferSyntaxCount = 3;
    }

    for (int i=0; i<numberOfDcmLongSCUStorageSOPClassUIDs && cond.good(); i++) {
        cond = ASC_addPresentationContext(
            params, pid, dcmLongSCUStorageSOPClassUIDs[i],
            transferSyntaxes, transferSyntaxCount);
        pid += 2;       /* only odd presentation context id's */
    }

    return cond;
}

// ********************************************

#define SHORTCOL 2
#define LONGCOL 10

int main(int argc, char *argv[])
{

#ifdef HAVE_GUSI_H
    GUSISetup(GUSIwithSIOUXSockets);
    GUSISetup(GUSIwithInternetSockets);
#endif

#ifdef HAVE_WINSOCK_H
    WSAData winSockData;
    /* we need at least version 1.1 */
    WORD winSockVersionNeeded = MAKEWORD( 1, 1 );
    WSAStartup(winSockVersionNeeded, &winSockData);
#endif

    int         opt_debugMode   = 0;                   /* default: no debug */
    int         opt_verbose     = 0;                   /* default: not verbose */
    const char *opt_cfgName     = NULL;                /* config file name */
    const char *opt_target      = NULL;                /* send target name */
    const char *opt_studyUID    = NULL;                /* study instance UID */
    const char *opt_seriesUID   = NULL;                /* series instance UID */
    const char *opt_instanceUID = NULL;                /* instance instance UID */

    SetDebugLevel(( 0 ));

    OFConsoleApplication app(OFFIS_CONSOLE_APPLICATION , "Network send for presentation state viewer", rcsid);
    OFCommandLine cmd;
    cmd.setOptionColumns(LONGCOL, SHORTCOL);
    cmd.setParamColumn(LONGCOL + SHORTCOL + 4);

    cmd.addParam("config-file",  "configuration file to be read");
    cmd.addParam("target",       "symbolic identifier of send target in config file");
    cmd.addParam("study",        "study instance UID of study in database to be sent");
    cmd.addParam("series",       "series instance UID (default: send complete study)", OFCmdParam::PM_Optional);
    cmd.addParam("instance",     "SOP instance UID (default: send complete series)", OFCmdParam::PM_Optional);

    cmd.addGroup("general options:", LONGCOL, SHORTCOL + 2);
     cmd.addOption("--help",                      "-h",        "print this help text and exit");
     cmd.addOption("--version",                                "print version information and exit", OFTrue /* exclusive */);
     cmd.addOption("--verbose",                   "-v",        "verbose mode, print processing details");
     cmd.addOption("--debug",                     "-d",        "debug mode, print debug information");

    /* evaluate command line */
    prepareCmdLineArgs(argc, argv, OFFIS_CONSOLE_APPLICATION);
    if (app.parseCommandLine(cmd, argc, argv, OFCommandLine::ExpandWildcards))
    {
      /* check exclusive options first */
      if (cmd.getParamCount() == 0)
      {
        if (cmd.findOption("--version"))
        {
            app.printHeader(OFTrue /*print host identifier*/);          // uses ofConsole.lockCerr()
            CERR << endl << "External libraries used:";
#if !defined(WITH_ZLIB) && !defined(WITH_OPENSSL)
            CERR << " none" << endl;
#else
            CERR << endl;
#endif
#ifdef WITH_ZLIB
            CERR << "- ZLIB, Version " << zlibVersion() << endl;
#endif
#ifdef WITH_OPENSSL
            CERR << "- " << OPENSSL_VERSION_TEXT << endl;
#endif
            return 0;
         }
      }

      /* command line parameters */
      cmd.getParam(1, opt_cfgName);
      cmd.getParam(2, opt_target);
      cmd.getParam(3, opt_studyUID);
      if (cmd.getParamCount() >= 4) cmd.getParam(4, opt_seriesUID);
      if (cmd.getParamCount() >= 5) cmd.getParam(5, opt_instanceUID);

      if (cmd.findOption("--verbose")) opt_verbose = 1;
      if (cmd.findOption("--debug")) opt_debugMode = 3;
    }

    if (opt_verbose)
    {
      CERR << rcsid << endl << endl;
    }

    if (opt_cfgName)
    {
      FILE *cfgfile = fopen(opt_cfgName, "rb");
      if (cfgfile) fclose(cfgfile); else
      {
        CERR << "error: can't open configuration file '" << opt_cfgName << "'" << endl;
        return 10;
      }
    } else {
        CERR << "error: missing configuration file name" << endl;
        return 10;
    }

    SetDebugLevel((opt_debugMode));

    /* make sure data dictionary is loaded */
    if (!dcmDataDict.isDictionaryLoaded())
    {
        CERR << "Warning: no data dictionary loaded, check environment variable: " << DCM_DICT_ENVIRONMENT_VARIABLE << endl;
    }

    DVConfiguration dvi(opt_cfgName);

    /* get send target from configuration file */
    const char *targetHostname    = dvi.getTargetHostname(opt_target);
    const char *targetDescription = dvi.getTargetDescription(opt_target);
    const char *targetAETitle     = dvi.getTargetAETitle(opt_target);
    unsigned short targetPort     = dvi.getTargetPort(opt_target);
    unsigned long  targetMaxPDU   = dvi.getTargetMaxPDU(opt_target);
    OFBool targetImplicitOnly     = dvi.getTargetImplicitOnly(opt_target);
    OFBool targetDisableNewVRs    = dvi.getTargetDisableNewVRs(opt_target);

    unsigned short messagePort    = dvi.getMessagePort();   /* port number for IPC */
    OFBool keepMessagePortOpen    = dvi.getMessagePortKeepOpen();
    OFBool useTLS = dvi.getTargetUseTLS(opt_target);

    Sint32 timeout = dvi.getTargetTimeout(opt_target);
    if (timeout > 0) dcmConnectionTimeout.set(timeout);

#ifdef WITH_OPENSSL
    /* TLS directory */
    const char *current = NULL;
    const char *tlsFolder = dvi.getTLSFolder();
    if (tlsFolder==NULL) tlsFolder = ".";

    /* certificate file */
    OFString tlsCertificateFile(tlsFolder);
    tlsCertificateFile += PATH_SEPARATOR;
    current = dvi.getTargetCertificate(opt_target);
    if (current) tlsCertificateFile += current; else tlsCertificateFile.clear();

    /* private key file */
    OFString tlsPrivateKeyFile(tlsFolder);
    tlsPrivateKeyFile += PATH_SEPARATOR;
    current = dvi.getTargetPrivateKey(opt_target);
    if (current) tlsPrivateKeyFile += current; else tlsPrivateKeyFile.clear();

    /* private key password */
    const char *tlsPrivateKeyPassword = dvi.getTargetPrivateKeyPassword(opt_target);

    /* certificate verification */
    DcmCertificateVerification tlsCertVerification = DCV_requireCertificate;
    switch (dvi.getTargetPeerAuthentication(opt_target))
    {
      case DVPSQ_require:
        tlsCertVerification = DCV_requireCertificate;
        break;
      case DVPSQ_verify:
        tlsCertVerification = DCV_checkCertificate;
        break;
      case DVPSQ_ignore:
        tlsCertVerification = DCV_ignoreCertificate;
        break;
    }

    /* DH parameter file */
    OFString tlsDHParametersFile;
    current = dvi.getTargetDiffieHellmanParameters(opt_target);
    if (current)
    {
      tlsDHParametersFile = tlsFolder;
      tlsDHParametersFile += PATH_SEPARATOR;
      tlsDHParametersFile += current;
    }

    /* random seed file */
    OFString tlsRandomSeedFile(tlsFolder);
    tlsRandomSeedFile += PATH_SEPARATOR;
    current = dvi.getTargetRandomSeed(opt_target);
    if (current) tlsRandomSeedFile += current; else tlsRandomSeedFile += "siteseed.bin";

    /* CA certificate directory */
    const char *tlsCACertificateFolder = dvi.getTLSCACertificateFolder();
    if (tlsCACertificateFolder==NULL) tlsCACertificateFolder = ".";

    /* key file format */
    int keyFileFormat = SSL_FILETYPE_PEM;
    if (! dvi.getTLSPEMFormat()) keyFileFormat = SSL_FILETYPE_ASN1;

    /* ciphersuites */
#if OPENSSL_VERSION_NUMBER >= 0x0090700fL
    OFString tlsCiphersuites(TLS1_TXT_RSA_WITH_AES_128_SHA ":" SSL3_TXT_RSA_DES_192_CBC3_SHA);
#else
    OFString tlsCiphersuites(SSL3_TXT_RSA_DES_192_CBC3_SHA);
#endif
    Uint32 tlsNumberOfCiphersuites = dvi.getTargetNumberOfCipherSuites(opt_target);
    if (tlsNumberOfCiphersuites > 0)
    {
      tlsCiphersuites.clear();
      OFString currentSuite;
      const char *currentOpenSSL;
      for (Uint32 ui=0; ui<tlsNumberOfCiphersuites; ui++)
      {
        dvi.getTargetCipherSuite(opt_target, ui, currentSuite);
        if (NULL == (currentOpenSSL = DcmTLSTransportLayer::findOpenSSLCipherSuiteName(currentSuite.c_str())))
        {
          CERR << "ciphersuite '" << currentSuite << "' is unknown. Known ciphersuites are:" << endl;
          unsigned long numSuites = DcmTLSTransportLayer::getNumberOfCipherSuites();
          for (unsigned long cs=0; cs < numSuites; cs++)
          {
            CERR << "    " << DcmTLSTransportLayer::getTLSCipherSuiteName(cs) << endl;
          }
          return 1;
        } else {
          if (tlsCiphersuites.length() > 0) tlsCiphersuites += ":";
          tlsCiphersuites += currentOpenSSL;
        }
      }
    }
#else
    if (useTLS)
    {
        CERR << "error: not compiled with OpenSSL, cannot use TLS." << endl;
        return 10;
    }
#endif

    if (targetHostname==NULL)
    {
        CERR << "error: no hostname for send target '" << opt_target << "'" << endl;
        return 10;
    }

    if (targetAETitle==NULL)
    {
        CERR << "error: no aetitle for send target '" << opt_target << "'" << endl;
        return 10;
    }

    if (targetPort==0)
    {
        CERR << "error: no or invalid port number for send target '" << opt_target << "'" << endl;
        return 10;
    }

    if (targetMaxPDU==0) targetMaxPDU = DEFAULT_MAXPDU;
    else if (targetMaxPDU > ASC_MAXIMUMPDUSIZE)
    {
        CERR << "warning: max PDU size " << targetMaxPDU << " too big for send target '"
             << opt_target << "', using default: " << DEFAULT_MAXPDU << endl;
        targetMaxPDU = DEFAULT_MAXPDU;
    }

    if (targetDisableNewVRs)
    {
      dcmEnableUnknownVRGeneration.set(OFFalse);
      dcmEnableUnlimitedTextVRGeneration.set(OFFalse);
    }

    OFOStringStream verboseParameters;

    verboseParameters << "Send target parameters:" << endl
         << "\thostname        : " << targetHostname << endl
         << "\tport            : " << targetPort << endl
         << "\tdescription     : ";
    if (targetDescription) verboseParameters << targetDescription; else verboseParameters << "(none)";
    verboseParameters << endl
         << "\taetitle         : " << targetAETitle << endl
         << "\tmax pdu         : " << targetMaxPDU << endl
         << "\ttimeout         : " << timeout << endl
         << "\toptions         : ";
    if (targetImplicitOnly && targetDisableNewVRs) verboseParameters << "implicit xfer syntax only, disable post-1993 VRs";
    else if (targetImplicitOnly) verboseParameters << "implicit xfer syntax only";
    else if (targetDisableNewVRs) verboseParameters << "disable post-1993 VRs";
    else verboseParameters << "none.";
    verboseParameters << endl;

    verboseParameters << "\tTLS             : ";
    if (useTLS) verboseParameters << "enabled" << endl; else verboseParameters << "disabled" << endl;

#ifdef WITH_OPENSSL
    if (useTLS)
    {
      verboseParameters << "\tTLS certificate : " << tlsCertificateFile << endl
           << "\tTLS key file    : " << tlsPrivateKeyFile << endl
           << "\tTLS DH params   : " << tlsDHParametersFile << endl
           << "\tTLS PRNG seed   : " << tlsRandomSeedFile << endl
           << "\tTLS CA directory: " << tlsCACertificateFolder << endl
           << "\tTLS ciphersuites: " << tlsCiphersuites << endl
           << "\tTLS key format  : ";
      if (keyFileFormat == SSL_FILETYPE_PEM) verboseParameters << "PEM" << endl; else verboseParameters << "DER" << endl;
      verboseParameters << "\tTLS cert verify : ";
      switch (tlsCertVerification)
      {
          case DCV_checkCertificate:
            verboseParameters << "verify" << endl;
            break;
          case DCV_ignoreCertificate:
            verboseParameters << "ignore" << endl;
            break;
          default:
            verboseParameters << "require" << endl;
            break;
      }
    }
#endif

    verboseParameters << OFStringStream_ends;
    OFSTRINGSTREAM_GETOFSTRING(verboseParameters, verboseParametersString)
    if (opt_verbose) CERR << verboseParametersString << endl;

    /* open database */
    const char *dbfolder = dvi.getDatabaseFolder();

    if (opt_verbose)
    {
      CERR << "Opening database in directory '" << dbfolder << "'" << endl;
    }

    OFCondition result;
    DcmQueryRetrieveIndexDatabaseHandle dbhandle(dbfolder, PSTAT_MAXSTUDYCOUNT, PSTAT_STUDYSIZE, result);    
    if (result.bad())
    {
      CERR << "Unable to access database '" << dbfolder << "'" << endl;
      return 1;
    }

#ifdef WITH_OPENSSL

    DcmTLSTransportLayer *tLayer = NULL;
    if (useTLS)
    {
      tLayer = new DcmTLSTransportLayer(DICOM_APPLICATION_REQUESTOR, tlsRandomSeedFile.c_str());
      if (tLayer == NULL)
      {
        app.printError("unable to create TLS transport layer");
      }

      if (tlsCACertificateFolder && (TCS_ok != tLayer->addTrustedCertificateDir(tlsCACertificateFolder, keyFileFormat)))
      {
        CERR << "warning unable to load certificates from directory '" << tlsCACertificateFolder << "', ignoring" << endl;
      }
      if ((tlsDHParametersFile.size() > 0) && ! (tLayer->setTempDHParameters(tlsDHParametersFile.c_str())))
      {
        CERR << "warning unable to load temporary DH parameter file '" << tlsDHParametersFile << "', ignoring" << endl;
      }
      tLayer->setPrivateKeyPasswd(tlsPrivateKeyPassword); // never prompt on console

      if ((tlsPrivateKeyFile.length() > 0) && (tlsCertificateFile.length() > 0))
      {
        if (TCS_ok != tLayer->setPrivateKeyFile(tlsPrivateKeyFile.c_str(), keyFileFormat))
        {
          CERR << "unable to load private TLS key from '" << tlsPrivateKeyFile<< "'" << endl;
          return 1;
        }
        if (TCS_ok != tLayer->setCertificateFile(tlsCertificateFile.c_str(), keyFileFormat))
        {
          CERR << "unable to load certificate from '" << tlsCertificateFile << "'" << endl;
          return 1;
        }
        if (! tLayer->checkPrivateKeyMatchesCertificate())
        {
          CERR << "private key '" << tlsPrivateKeyFile << "' and certificate '" << tlsCertificateFile << "' do not match" << endl;
          return 1;
        }
      }
      if (TCS_ok != tLayer->setCipherSuites(tlsCiphersuites.c_str()))
      {
        CERR << "unable to set selected cipher suites" << endl;
        return 1;
      }

      tLayer->setCertificateVerification(tlsCertVerification);

      // a generated UID contains the process ID and current time.
      // Adding it to the PRNG seed guarantees that we have different seeds for different processes.
      char randomUID[65];
      dcmGenerateUniqueIdentifier(randomUID);
      tLayer->addPRNGseed(randomUID, strlen(randomUID));
    }

#endif

    /* open network connection */
    T_ASC_Network *net=NULL;
    T_ASC_Parameters *params=NULL;
    DIC_NODENAME localHost;
    DIC_NODENAME peerHost;
    T_ASC_Association *assoc=NULL;

    OFCondition cond = ASC_initializeNetwork(NET_REQUESTOR, 0, 30, &net);
    if (cond.bad())
    {
      DimseCondition::dump(cond);
      return 1;
    }

#ifdef WITH_OPENSSL
    if (tLayer)
    {
      cond = ASC_setTransportLayer(net, tLayer, 0);
      if (cond.bad())
      {
        DimseCondition::dump(cond);
        return 1;
      }
    }
#endif

    cond = ASC_createAssociationParameters(&params, targetMaxPDU);
    if (cond.bad())
    {
      DimseCondition::dump(cond);
      return 1;
    }

    cond = ASC_setTransportLayerType(params, useTLS);
    if (cond.bad())
    {
      DimseCondition::dump(cond);
      return 1;
    }

    ASC_setAPTitles(params, dvi.getNetworkAETitle(), targetAETitle, NULL);

    gethostname(localHost, sizeof(localHost) - 1);
    sprintf(peerHost, "%s:%d", targetHostname, (int)targetPort);
    ASC_setPresentationAddresses(params, localHost, peerHost);

    cond = addAllStoragePresentationContexts(params, targetImplicitOnly);
    if (cond.bad())
    {
      DimseCondition::dump(cond);
      return 1;
    }

    if (messagePort > 0)
    {
      messageClient = new DVPSIPCClient(DVPSIPCMessage::clientStoreSCU, verboseParametersString.c_str(), messagePort, keepMessagePortOpen);
      if (! messageClient->isServerActive())
      {
        CERR << "Warning: no IPC message server found at port " << messagePort << ", disabling IPC." << endl;
      }
    }

    /* create association */
    if (opt_verbose) CERR << "Requesting Association" << endl;

    cond = ASC_requestAssociation(net, params, &assoc);
    if (cond.bad())
    {
        if (cond == DUL_ASSOCIATIONREJECTED)
        {
            T_ASC_RejectParameters rej;

            ASC_getRejectParameters(params, &rej);
            CERR << "Association Rejected" << endl;
            ASC_printRejectParameters(stderr, &rej);
            if (messageClient)
            {
              // notify about rejected association
              OFOStringStream out;
              out << "DIMSE Association Rejected:" << endl
                  << "\t";
              ASC_printRejectParameters(out, &rej);
              out << "\tcalled presentation address: " << assoc->params->DULparams.calledPresentationAddress << endl
                  << "\tcalling AE title: " << assoc->params->DULparams.callingAPTitle << endl
                  << "\tcalled AE title: " << assoc->params->DULparams.calledAPTitle << endl;
              ASC_dumpConnectionParameters(assoc, out);
              out << OFStringStream_ends;
              OFSTRINGSTREAM_GETSTR(out, theString)
              if (useTLS)
                messageClient->notifyRequestedEncryptedDICOMConnection(DVPSIPCMessage::statusError, theString);
                else messageClient->notifyRequestedUnencryptedDICOMConnection(DVPSIPCMessage::statusError, theString);
              messageClient->notifyApplicationTerminates(DVPSIPCMessage::statusError);
              OFSTRINGSTREAM_FREESTR(theString)
              delete messageClient;
            }
            return 1;
        } else {
            CERR << "Association Request Failed" << endl;
            DimseCondition::dump(cond);
            if (messageClient)
            {
              // notify about rejected association
              OFOStringStream out;
              out << "DIMSE Association Request Failed:" << endl;
              out << "\tcalled presentation address: " << assoc->params->DULparams.calledPresentationAddress << endl
                  << "\tcalling AE title: " << assoc->params->DULparams.callingAPTitle << endl
                  << "\tcalled AE title: " << assoc->params->DULparams.calledAPTitle << endl;
              ASC_dumpConnectionParameters(assoc, out);
              out << cond.text() << endl << OFStringStream_ends;
              OFSTRINGSTREAM_GETSTR(out, theString)
              if (useTLS)
                messageClient->notifyRequestedEncryptedDICOMConnection(DVPSIPCMessage::statusError, theString);
                else messageClient->notifyRequestedUnencryptedDICOMConnection(DVPSIPCMessage::statusError, theString);
              messageClient->notifyApplicationTerminates(DVPSIPCMessage::statusError);
              OFSTRINGSTREAM_FREESTR(theString)
              delete messageClient;
            }
            return 1;
        }
    }

    if (ASC_countAcceptedPresentationContexts(params) == 0)
    {
      CERR << "No Acceptable Presentation Contexts" << endl;
      cond = ASC_abortAssociation(assoc);
      if (cond.bad())
      {
        CERR << "Association Abort Failed" << endl;
        DimseCondition::dump(cond);
      }
      if (messageClient)
      {
        // notify about rejected association
        OFOStringStream out;
        out << "DIMSE association accepted, but no acceptable presentation contexts - aborting." << endl
            << "\tcalled presentation address: " << assoc->params->DULparams.calledPresentationAddress << endl
            << "\tcalling AE title: " << assoc->params->DULparams.callingAPTitle << endl
            << "\tcalled AE title: " << assoc->params->DULparams.calledAPTitle << endl;
        ASC_dumpConnectionParameters(assoc, out);
        out << OFStringStream_ends;
        OFSTRINGSTREAM_GETSTR(out, theString)
        if (useTLS)
          messageClient->notifyRequestedEncryptedDICOMConnection(DVPSIPCMessage::statusError, theString);
          else messageClient->notifyRequestedUnencryptedDICOMConnection(DVPSIPCMessage::statusError, theString);
        messageClient->notifyApplicationTerminates(DVPSIPCMessage::statusError);
        OFSTRINGSTREAM_FREESTR(theString)
        delete messageClient;
      }
      return 1;
    }

    if (opt_verbose) CERR << "Association accepted (Max Send PDV: " << assoc->sendPDVLength << ")" << endl;

    if (messageClient)
    {
      // notify about successfully negotiated association
      OFOStringStream out;
      out << "DIMSE Association Accepted:" << endl
          << "\tcalled presentation address: " << assoc->params->DULparams.calledPresentationAddress << endl
          << "\tcalling AE title: " << assoc->params->DULparams.callingAPTitle << endl
          << "\tcalled AE title: " << assoc->params->DULparams.calledAPTitle << endl
          << "\tmax send PDV: " << assoc->sendPDVLength << endl
          << "\tpresentation contexts: " << ASC_countAcceptedPresentationContexts(assoc->params) << endl;
      ASC_dumpConnectionParameters(assoc, out);
      out << OFStringStream_ends;
      OFSTRINGSTREAM_GETSTR(out, theString)
      if (useTLS)
        messageClient->notifyRequestedEncryptedDICOMConnection(DVPSIPCMessage::statusOK, theString);
        else messageClient->notifyRequestedUnencryptedDICOMConnection(DVPSIPCMessage::statusOK, theString);
      OFSTRINGSTREAM_FREESTR(theString)
    }


    /* do the real work */
    cond = sendStudy(dbhandle, assoc, opt_studyUID, opt_seriesUID, opt_instanceUID, opt_verbose);

    /* tear down association */
    if (cond.good())
    {
        /* release association */
        if (opt_verbose) CERR << "Releasing Association" << endl;
        cond = ASC_releaseAssociation(assoc);
        if (cond.bad())
        {
          CERR << "Association Release Failed" << endl;
          DimseCondition::dump(cond);
          if (messageClient)
          {
            messageClient->notifyApplicationTerminates(DVPSIPCMessage::statusError);
            delete messageClient;
          }
          return 1;
        }
        if (messageClient) messageClient->notifyConnectionClosed(DVPSIPCMessage::statusOK);
    }
    else if (cond == DUL_PEERREQUESTEDRELEASE)
    {
        CERR << "Protocol Error: peer requested release (Aborting)" << endl;
        if (opt_verbose) CERR << "Aborting Association" << endl;
        cond = ASC_abortAssociation(assoc);
        if (cond.bad())
        {
            CERR << "Association Abort Failed" << endl;
            DimseCondition::dump(cond);
            if (messageClient)
            {
              messageClient->notifyApplicationTerminates(DVPSIPCMessage::statusError);
              delete messageClient;
            }
            return 1;
        }
        if (messageClient) messageClient->notifyConnectionAborted(DVPSIPCMessage::statusError, "Protocol error: peer requested release, aborting association.");
    }
    else if (cond == DUL_PEERABORTEDASSOCIATION)
    {
        if (opt_verbose) CERR << "Peer Aborted Association" << endl;
        if (messageClient) messageClient->notifyConnectionAborted(DVPSIPCMessage::statusError, "Peer aborted association.");
    }
    else
    {
        CERR << "SCU Failed" << endl;
        DimseCondition::dump(cond);
        if (opt_verbose) CERR << "Aborting Association" << endl;
        cond = ASC_abortAssociation(assoc);
        if (cond.bad())
        {
            CERR << "Association Abort Failed" << endl;
            DimseCondition::dump(cond);
            if (messageClient)
            {
              messageClient->notifyApplicationTerminates(DVPSIPCMessage::statusError);
              delete messageClient;
            }
            return 1;
        }
        if (messageClient) messageClient->notifyConnectionAborted(DVPSIPCMessage::statusError, "Storage SCU failed, aborting association.");
    }

    cond = ASC_destroyAssociation(&assoc);
    if (cond.bad())
    {
      DimseCondition::dump(cond);
      if (messageClient)
      {
        messageClient->notifyApplicationTerminates(DVPSIPCMessage::statusError);
        delete messageClient;
      }
      return 1;
    }
    cond = ASC_dropNetwork(&net);
    if (cond.bad())
    {
      DimseCondition::dump(cond);
      if (messageClient)
      {
        messageClient->notifyApplicationTerminates(DVPSIPCMessage::statusError);
        delete messageClient;
      }
      return 1;
    }

    // tell the IPC server that we're going to terminate.
    // We need to do this before we shutdown WinSock.
    if (messageClient)
    {
      messageClient->notifyApplicationTerminates(DVPSIPCMessage::statusOK);
      delete messageClient;
    }

#ifdef HAVE_WINSOCK_H
    WSACleanup();
#endif

#ifdef WITH_OPENSSL
    if (tLayer)
    {
      if (tLayer->canWriteRandomSeed())
      {
        if (!tLayer->writeRandomSeed(tlsRandomSeedFile.c_str()))
        {
          CERR << "Error while writing back random seed file '" << tlsRandomSeedFile << "', ignoring." << endl;
        }
      } else {
        CERR << "Warning: cannot write back random seed, ignoring." << endl;
      }
    }
    delete tLayer;
#endif

#ifdef DEBUG
    dcmDataDict.clear();  /* useful for debugging with dmalloc */
#endif

    return 0;
}


/*
 * CVS/RCS Log:
 * $Log: dcmpssnd.cc,v $
 * Revision 1.38  2005/12/08 15:46:11  meichel
 * Changed include path schema for all DCMTK header files
 *
 * Revision 1.37  2005/11/28 15:29:05  meichel
 * File dcdebug.h is not included by any other header file in the toolkit
 *   anymore, to minimize the risk of name clashes of macro debug().
 *
 * Revision 1.36  2005/11/23 16:10:32  meichel
 * Added support for AES ciphersuites in TLS module. All TLS-enabled
 *   tools now support the "AES TLS Secure Transport Connection Profile".
 *
 * Revision 1.35  2005/11/16 14:58:23  meichel
 * Set association timeout in ASC_initializeNetwork to 30 seconds. This improves
 *   the responsiveness of the tools if the peer blocks during assoc negotiation.
 *
 * Revision 1.34  2005/10/25 08:55:59  meichel
 * Updated list of UIDs and added support for new transfer syntaxes
 *   and storage SOP classes.
 *
 * Revision 1.33  2005/04/04 10:11:53  meichel
 * Module dcmpstat now uses the dcmqrdb API instead of imagectn for maintaining
 *   the index database
 *
 * Revision 1.32  2004/02/04 15:44:38  joergr
 * Removed acknowledgements with e-mail addresses from CVS log.
 *
 * Revision 1.31  2002/11/29 13:16:28  meichel
 * Introduced new command line option --timeout for controlling the
 *   connection request timeout.
 *
 * Revision 1.30  2002/11/26 08:44:29  meichel
 * Replaced all includes for "zlib.h" with <zlib.h>
 *   to avoid inclusion of zlib.h in the makefile dependencies.
 *
 * Revision 1.29  2002/09/23 19:06:32  joergr
 * Fixed typo in pre-processor directive.
 *
 * Revision 1.28  2002/09/23 18:26:09  joergr
 * Added new command line option "--version" which prints the name and version
 * number of external libraries used (incl. preparation for future support of
 * 'config.guess' host identifiers).
 *
 * Revision 1.27  2002/06/14 10:20:53  meichel
 * Removed dependency from class DVInterface. Significantly reduces
 *   size of binary.
 *
 * Revision 1.26  2002/05/02 14:10:05  joergr
 * Added support for standard and non-standard string streams (which one is
 * supported is detected automatically via the configure mechanism).
 *
 * Revision 1.25  2002/04/16 14:01:28  joergr
 * Added configurable support for C++ ANSI standard includes (e.g. streams).
 *
 * Revision 1.24  2002/01/08 10:31:47  joergr
 * Corrected spelling of function dcmGenerateUniqueIdentifier().
 *
 * Revision 1.23  2001/10/12 13:46:49  meichel
 * Adapted dcmpstat to OFCondition based dcmnet module (supports strict mode).
 *
 * Revision 1.22  2001/09/28 13:48:43  joergr
 * Replaced "cerr" by "CERR".
 *
 * Revision 1.21  2001/06/07 14:34:09  joergr
 * Removed comment.
 *
 * Revision 1.19  2001/06/01 15:50:10  meichel
 * Updated copyright header
 *
 * Revision 1.18  2000/11/08 18:38:04  meichel
 * Updated dcmpstat IPC protocol for additional message parameters
 *
 * Revision 1.17  2000/10/10 12:23:45  meichel
 * Added extensions for TLS encrypted communication
 *
 * Revision 1.16  2000/05/31 13:02:25  meichel
 * Moved dcmpstat macros and constants into a common header file
 *
 * Revision 1.15  2000/04/14 16:34:38  meichel
 * Global VR generation flags are now derived from OFGlobal and, thus,
 *   safe for use in multi-thread applications.
 *
 * Revision 1.14  2000/03/08 16:28:43  meichel
 * Updated copyright header.
 *
 * Revision 1.13  2000/03/06 18:21:47  joergr
 * Avoid empty statement in the body of if-statements (MSVC6 reports warnings).
 *
 * Revision 1.12  2000/03/03 14:13:28  meichel
 * Implemented library support for redirecting error messages into memory
 *   instead of printing them to stdout/stderr for GUI applications.
 *
 * Revision 1.11  2000/02/29 12:13:44  meichel
 * Removed support for VS value representation. This was proposed in CP 101
 *   but never became part of the standard.
 *
 * Revision 1.10  2000/02/03 11:50:45  meichel
 * Moved UID related functions from dcmnet (diutil.h) to dcmdata (dcuid.h)
 *   where they belong. Renamed access functions to dcmSOPClassUIDToModality
 *   and dcmGuessModalityBytes.
 *
 * Revision 1.9  1999/11/24 10:21:56  meichel
 * Fixed locking problem in dcmpssnd and dcmpsrcv on Win9x platforms.
 *
 * Revision 1.8  1999/09/17 14:29:07  meichel
 * Moved static helper functions to new class DVPSHelper, removed some unused code.
 *
 * Revision 1.7  1999/09/06 13:29:48  meichel
 * Enhanced max receive PDU range to 4-128K.
 *
 * Revision 1.6  1999/04/30 16:36:56  meichel
 * Renamed all flock calls to dcmtk_flock to avoid name clash between flock()
 * emulation based on fcntl() and a constructor for struct flock.
 *
 * Revision 1.5  1999/04/28 15:45:09  meichel
 * Cleaned up module dcmpstat apps, adapted to new command line class
 *   and added short documentation.
 *
 * Revision 1.4  1999/02/25 18:34:25  joergr
 * Added debug code (explicitly delete data dictionary).
 *
 * Revision 1.3  1999/02/08 12:52:17  meichel
 * Removed dummy parameter from DVInterface constructor.
 *
 * Revision 1.2  1999/01/25 13:05:47  meichel
 * Implemented DVInterface::startReceiver()
 *   and several config file related methods.
 *
 * Revision 1.1  1999/01/20 19:26:17  meichel
 * Implemented DICOM network send application "dcmpssnd" which sends studies/
 *   series/images contained in the local database to a remote DICOM
 *   communication peer.
 *
 *
 */
