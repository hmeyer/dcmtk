/*
 *
 *  Copyright (C) 1994-2005, OFFIS
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
 *  Author:  Andreas Barth
 *
 *  Purpose: create a Dicom FileFormat or DataSet from an ASCII-dump
 *
 *  Last Update:      $Author: onken $
 *  Update Date:      $Date: 2005/12/16 09:07:03 $
 *  CVS/RCS Revision: $Revision: 1.51 $
 *  Status:           $State: Exp $
 *
 *  CVS/RCS Log at end of file
 *
 */

/*
 * Input File Description:
 * The input file be an output of dcmdump. One element (Tag, VR, value) must
 * be written into one line separated by arbitrary spaces or tab characters.
 * A # begins a comment that ends at the line end. Empty lines are allowed.
 * This parts of a line have the following syntax:
 * Tag:   (gggg,eeee)
 *        with gggg and eeee are 4 character hexadecimal values representing
 *        group- and element-tag. Spaces and Tabs can be anywhere in a Tag
 *        specification
 * VR:    Value Representation must be written as 2 characters as in Part 6
 *        of the DICOM 3.0 standard. No Spaces or Tabs are allowed between the
 *        two characters. If the VR can determined from the Tag, this part of
 *        a line is optional.
 * Value: There are several rules for writing values:
 *        1. US, SS, SL, UL, FD, FL are written as
 *           decimal strings that can be read by scanf.
 *        2. AT is written as (gggg,eeee) with additional spaces stripped off
 *           automatically and gggg and eeee being decimal strings that
 *           can be read by scanf.
 *        3. OB, OW values are written as byte or word hexadecimal values
 *           separated by '\' character.  Alternatively, OB or OW values can
 *           be read from a separate file by writing the filename prefixed
 *           by a '=' character (e.g. =largepixeldata.dat).  The contents of
 *           the file will be read as is.  OW data is expected to be little
 *           endian ordered and will be swapped if necessary.  No checks will
 *           be made to ensure that the amount of data is reasonable in terms
 *           of other attributes such as Rows or Columns.
 *        4. UI is written as =Name in data dictionary or as
 *           unique identifer string (see  6.) , e.g. [1.2.840.....]
 *        5. Strings without () <> [] spaces, tabs and # can be
 *           written directly
 *        6. Other strings with must be surrounded by [ ]. No
 *           bracket structure is passed. The value ends at the last ] in
 *           the line. Anything after the ] is interpreted as comment.
 *        7. ( < are interpreted special and may not be used when writing
 *           an input file by hand as beginning characters of a string.
 *        Multiple Value are separated by \
 *        The sequence of lines must not be ordered but they can.
 *        References in DICOM Directories are not supported.
 *        Semantic errors are not detected.
 *
 * Examples:
 *  (0008,0020) DA  [19921012]          #     8,  1  StudyDate
 *  (0008,0016) UI  =MRImageStorage     #    26,  1  SOPClassUID
 *  (0002,0012) UI  [1.2.276.0.7230010.100.1.1]
 *  (0020,0032) DS  [0.0\0.0]           #     8,  2  ImagePositionPatient
 *  (0028,0009) AT  (3004,000c)         #     4,  1  FrameIncrementPointer
 *  (0028,0010) US  256                 #     4,  1  Rows
 *  (0002,0001) OB  01\00
 *
 */

#include "dcmtk/config/osconfig.h"

#define INCLUDE_CSTDLIB
#define INCLUDE_CSTDIO
#define INCLUDE_CCTYPE
#include "dcmtk/ofstd/ofstdinc.h"

#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif
#ifdef HAVE_STAT_H
#include <stat.h>
#endif
#ifdef HAVE_GUSI_H
#include <GUSI.h>
#endif

#include "dcmtk/ofstd/ofstream.h"
#include "dcmtk/dcmdata/dctk.h"
#include "dcmtk/dcmdata/dcdebug.h"
#include "dcmtk/dcmdata/cmdlnarg.h"
#include "dcmtk/ofstd/ofconapp.h"
#include "dcmtk/ofstd/ofstd.h"
#include "dcmtk/dcmdata/dcuid.h"    /* for dcmtk version name */

#ifdef WITH_ZLIB
#include <zlib.h>     /* for zlibVersion() */
#endif

#define OFFIS_CONSOLE_APPLICATION "dump2dcm"

static char rcsid[] = "$dcmtk: " OFFIS_CONSOLE_APPLICATION " v"
  OFFIS_DCMTK_VERSION " " OFFIS_DCMTK_RELEASEDATE " $";

#define SHORTCOL 3
#define LONGCOL 21

// Maximum Line Size

const unsigned int DCM_DumpMaxLineSize = 4096;


// Field definition separators

#define DCM_DumpCommentChar '#'
#define DCM_DumpTagDelim ')'
#define DCM_DumpOpenString '['
#define DCM_DumpCloseString ']'
#define DCM_DumpOpenDescription '('
#define DCM_DumpOpenFile '<'
#define DCM_DumpCloseFile '>'

static void
stripWhitespace(char* s)
{
    char* p;

    if (s == NULL) return;

    p = s;
    while (*s != '\0') {
        if (isspace(*s) == OFFalse) {
            *p++ = *s;
        }
        s++;
    }
    *p = '\0';
}

static char*
stripTrailingWhitespace(char* s)
{
    int i, n;
    if (s == NULL) return s;

    n = strlen(s);
    for (i = n - 1; i >= 0 && isspace(s[i]); i--)
        s[i] = '\0';
    return s;
}


static char *
stripPrecedingWhitespace(char * s)
{
    char * p;
    if (s == NULL) return s;

    for(p = s; *p && isspace(*p); p++)
        ;

    return p;
}

static OFBool
onlyWhitespace(const char* s)
{
    int len = strlen(s);
    int charsFound = OFFalse;

    for (int i=0; (!charsFound) && (i<len); i++) {
        charsFound = !isspace(s[i]);
    }
    return (!charsFound)?(OFTrue):(OFFalse);
}

static char*
getLine(char* line, int maxLineLen, FILE* f, const unsigned long lineNumber)
{
    char* s;

    s = fgets(line, maxLineLen, f);

    // if line is too long, throw rest of it away
    if (s && strlen(s) == size_t(maxLineLen-1) && s[maxLineLen-2] != '\n')
    {
        int c = fgetc(f);
        while(c != '\n' && c != EOF)
            c = fgetc(f);
        CERR << "line " << lineNumber << " too long." << endl;
    }

    /* strip any trailing white space */
    stripTrailingWhitespace(s);

    return s;
}

static OFBool
isaCommentLine(const char* s)
{
    OFBool isComment = OFFalse; /* assumption */
    int len = strlen(s);
    int i = 0;
    for (i=0; i<len && isspace(s[i]); i++) /*loop*/;
        isComment = (s[i] == DCM_DumpCommentChar);
    return isComment;
}

static OFBool
parseTag(char* & s, DcmTagKey& key)
{
    OFBool ok = OFTrue;
    char * p;
    unsigned int g, e;

    // find tag begin
    p = strchr(s, DCM_DumpTagDelim);
    if (p)
    {
        // string all white spaces and read tag
        int len = p-s+1;
        p = new char[len+1];
        OFStandard::strlcpy(p, s, len+1);
        stripWhitespace(p);
        s += len;

        if (sscanf(p, "(%x,%x)", &g, &e) == 2) {
            key.set(g, e);
        } else {
            ok = OFFalse;
        }
        delete[] p;
    }
        else ok = OFFalse;

    return ok;
}


static OFBool
parseVR(char * & s, DcmEVR & vr)
{
    OFBool ok = OFTrue;

    s = stripPrecedingWhitespace(s);

    // Are there two upper characters?
    if (isupper(*s) && isupper(*(s+1)))
    {
        char c_vr[3];
        OFStandard::strlcpy(c_vr, s, 3);
        // Convert to VR
        DcmVR dcmVR(c_vr);
        vr = dcmVR.getEVR();
        s+=2;
    }
    else if ((*s == 'p')&&(*(s+1) == 'i'))
    {
        vr = EVR_pixelItem;
        s+=2;
    }
    else if (((*s == 'o')&&(*(s+1) == 'x')) || ((*s == 'x')&&(*(s+1) == 's')) ||
         (*s == 'n')&&(*(s+1) == 'a') || ((*s == 'u')&&(*(s+1) == 'p')))
    {
        // swallow internal VRs
        vr = EVR_UNKNOWN;
        s+=2;
    }
    else ok = OFFalse;

    return ok;
}


static int
searchLastClose(char *s, char closeChar)
{
    // search last close bracket in a line
    // no bracket structure will be considered

    char * found = NULL;
    char * p = s;

    while(p && *p)
    {
        p = strchr(p, closeChar);
        if (p)
        {
            found = p;
            p++;
        }
    }

    if (found)
        return (found - s) + 1;
    else
        return 0;
}


static int
searchCommentOrEol(char *s)
{
    char * comment = strchr(s, DCM_DumpCommentChar);
    if (comment)
        return comment - s;
    else if (s)
        return strlen(s);
    else
        return 0;
}


static char*
convertNewlineCharacters(char* s)
{
    // convert the string "\n" into the \r\n combination required by DICOM
    if (s == NULL || s[0] == '\0') return s;
    int len = strlen(s);
    int i = 0;
    for (i=0; i<(len-1); i++) {
        if (s[i] == '\\' && s[i+1] == 'n') {
            s[i] = '\r';
            s[i+1] = '\n';
            i++;
        }
    }

    return s;
}

static OFBool
parseValue(char * & s, char * & value, const DcmEVR & vr)
{
    OFBool ok = OFTrue;
    int len;
    value = NULL;

    s = stripPrecedingWhitespace(s);

    switch (*s)
    {
        case DCM_DumpOpenString:
            len = searchLastClose(s, DCM_DumpCloseString);
            if (len == 0)
                ok = OFFalse;
            else if (len > 2)
            {
                value = new char[len-1];
                OFStandard::strlcpy(value, s+1, len-1);
                value = convertNewlineCharacters(value);
            }
            else
                value = NULL;
            break;

        case DCM_DumpOpenDescription:
            /* need to distinguish vr=AT from description field */
            /* NB: if the vr is unknown this workaround will not succeed */
            if (vr == EVR_AT)
            {
                len = searchLastClose(s, DCM_DumpTagDelim);
                if (len >= 11)  // (gggg,eeee) allow non-significant spaces
                {
                    char *pv = s;
                    DcmTagKey tag;
                    if (parseTag(pv, tag))   // check for valid tag format
                    {
                        value = new char[len+1];
                        OFStandard::strlcpy(value, s, len+1);
                        stripWhitespace(value);
                    } else
                        ok = OFFalse;   // skip description
                }
                else
                    ok = OFFalse;   // skip description
            }
            break;

        case DCM_DumpOpenFile:
            ok = OFFalse;  // currently not supported
            break;

        case DCM_DumpCommentChar:
            break;

        default:
            len = searchCommentOrEol(s);
            if (len)
            {
                value = new char[len+1];
                OFStandard::strlcpy(value, s, len+1);
                stripTrailingWhitespace(value);
            }
            break;
    }
    return ok;
}


static unsigned long
fileSize(const char *fname)
{
    struct stat s;
    unsigned long nbytes = 0;

    if (stat(fname, &s) == 0) {
        nbytes = s.st_size;
    }
    return nbytes;
}

static OFCondition
putFileContentsIntoElement(DcmElement* elem, const char* filename)
{
    FILE* f = NULL;
    OFCondition ec = EC_Normal;

    if ((f = fopen(filename, "rb")) == NULL) {
        CERR << "ERROR: cannot open binary data file: " << filename << endl;
        return EC_InvalidTag;
    }

    unsigned long len = fileSize(filename);
    unsigned long buflen = len;
    if (buflen & 1) buflen++; /* if odd then make even (DICOM required even length values) */

    Uint8* buf = new Uint8[buflen];
    if (buf == NULL) {
        CERR << "ERROR: out of memory reading binary data file: " << filename << endl;
        ec = EC_MemoryExhausted;
    } else if (fread(buf, 1, OFstatic_cast(size_t, len), f) != len) {
        CERR << "ERROR: error reading binary data file: " << filename << ": ";
        perror(NULL);
        ec = EC_CorruptedData;
    } else {
        /* assign buffer to attribute */
        DcmEVR evr = elem->getVR();
        if (evr == EVR_OB) {
            /* put 8 bit OB data into the attribute */
            ec = elem->putUint8Array(buf, buflen);
        } else if (evr == EVR_OW) {
            /* put 16 bit OW data into the attribute */
            swapIfNecessary(gLocalByteOrder, EBO_LittleEndian, buf, buflen, sizeof(Uint16));
            ec = elem->putUint16Array(OFreinterpret_cast(Uint16 *, buf), buflen / 2);
        } else if (evr == EVR_pixelItem) {
            /* pixel item not yet supported */
        }
    }

    fclose(f);
    delete[] buf;
    return ec;
}

static OFCondition
insertIntoSet(DcmStack & stack, DcmTagKey tagkey, DcmEVR vr, char * value)
{
    // insert new Element into dataset or metaheader

    OFCondition l_error = EC_Normal;
    OFCondition newElementError = EC_Normal;

    if (stack.empty())
        l_error = EC_CorruptedData;

    if (l_error == EC_Normal)
    {
        DcmElement * newElement = NULL;
        DcmObject * topOfStack = stack.top();

        // convert tagkey to tag including VR
        DcmTag tag(tagkey);
        DcmVR dcmvr(vr);

        const DcmEVR tagvr = tag.getEVR();
        if (tagvr != vr && vr != EVR_UNKNOWN && tagvr != EVR_UNKNOWN &&
           (tagkey != DCM_LUTData || (vr != EVR_US && vr != EVR_SS && vr != EVR_OW)) &&
           (tagvr != EVR_xs || (vr != EVR_US && vr != EVR_SS)) &&
           (tagvr != EVR_ox || (vr != EVR_OB && vr != EVR_OW)) &&
           (tagvr != EVR_na || vr != EVR_pixelItem))
        {
            CERR << "Warning: Tag " << tag << " with wrong VR "
                 << dcmvr.getVRName() << " found, correct is "
                 << tag.getVR().getVRName() << endl;
        }

        if (vr != EVR_UNKNOWN)
            tag.setVR(dcmvr);

        // create new element
        newElementError = newDicomElement(newElement, tag);

        if (newElementError == EC_Normal)
        {
            // tag describes an Element
            if (!newElement)
                // Tag was ambiguous
                l_error = EC_InvalidVR;
            else
            {
                // fill value
                if (value)
                {
                    if (value[0] == '=' && (tag.getEVR() == EVR_OB || tag.getEVR() == EVR_OW))
                    {
                        /*
                         * Special case handling for OB or OW data.
                         * Allow a value beginning with a '=' character to represent
                         * a file containing data to be used as the attribute value.
                         * A '=' character is not a normal value since OB and OW values
                         * must be written as multivalued hexidecimal (e.g. "00\ff\0d\8f");
                         */
                        l_error = putFileContentsIntoElement(newElement, value+1);
                    }
                    else if (tag.getEVR() == EVR_pixelItem)
                    {
                        /* pixel items not yet supported */
                        l_error = EC_InvalidTag;
                    } else {
                        l_error = newElement->putString(value);
                    }
                }

                // insert element into hierarchy
                if (l_error == EC_Normal)
                {
                    switch(topOfStack->ident())
                    {
                      case EVR_item:
                      case EVR_dirRecord:
                      case EVR_dataset:
                      case EVR_metainfo:
                      {
                          DcmItem *item = OFstatic_cast(DcmItem *, topOfStack);
                          item -> insert(newElement);
                          if (newElement->ident() == EVR_SQ || newElement->ident() == EVR_pixelSQ)
                              stack.push(newElement);
                      }
                      break;
                      default:
                          l_error = EC_InvalidTag;
                      break;
                    }
                }
            }
        }
        else if (newElementError == EC_SequEnd)
        {
            // pop stack if stack object was a sequence
            if (topOfStack->ident() == EVR_SQ || topOfStack->ident() == EVR_pixelSQ)
                stack.pop();
            else
                l_error = EC_InvalidTag;
        }
        else if (newElementError == EC_ItemEnd)
        {
            // pop stack if stack object was an item
            switch (topOfStack->ident())
            {
              case EVR_item:
              case EVR_dirRecord:
              case EVR_dataset:
              case EVR_metainfo:
                stack.pop();
              break;

              default:
                l_error = EC_InvalidTag;
              break;
            }
        }
        else if (newElementError == EC_InvalidTag)
        {
            if (tag.getXTag() == DCM_Item)
            {
                DcmItem * item = NULL;
                if (topOfStack->getTag().getXTag() == DCM_DirectoryRecordSequence)
                {
                    // an Item must be pushed to the stack
                    item = new DcmDirectoryRecord(tag, 0);
                    OFstatic_cast(DcmSequenceOfItems *, topOfStack) -> insert(item);
                    stack.push(item);
                }
                else if (topOfStack->ident() == EVR_SQ)
                {
                    // an item must be pushed to the stack
                    item = new DcmItem(tag);
                    OFstatic_cast(DcmSequenceOfItems *, topOfStack) -> insert(item);
                    stack.push(item);
                }
                else
                    l_error = EC_InvalidTag;
            }
            else
                l_error = EC_InvalidTag;
        }
        else
        {
            l_error = EC_InvalidTag;
        }
    }

    return l_error;
}

static OFBool
readDumpFile(DcmMetaInfo * metaheader, DcmDataset * dataset,
         FILE * infile, const char * ifname, const OFBool stopOnErrors,
         const unsigned long maxLineLength)
{
    char * lineBuf = new char[maxLineLength];
    int lineNumber = 0;
    OFBool errorOnThisLine = OFFalse;
    char * parse = NULL;
    char * value = NULL;
    DcmEVR vr = EVR_UNKNOWN;
    int errorsEncountered = 0;
    DcmTagKey tagkey;
    DcmStack metaheaderStack;
    DcmStack datasetStack;

    if (metaheader)
    metaheaderStack.push(metaheader);

    datasetStack.push(dataset);

    while(getLine(lineBuf, int(maxLineLength), infile, lineNumber+1))
    {
        lineNumber++;

        // ignore empty lines and comment lines
        if (onlyWhitespace(lineBuf))
            continue;
        if (isaCommentLine(lineBuf))
            continue;

        errorOnThisLine = OFFalse;

        parse = &lineBuf[0];

        // parse tag an the line
        if (!parseTag(parse, tagkey))
        {
            CERR << OFFIS_CONSOLE_APPLICATION ": "<< ifname << ": "
                 << "no Tag found (line " << lineNumber << ")"<< endl;
            errorOnThisLine = OFTrue;
        }

        // parse optional VR
        if (!errorOnThisLine && !parseVR(parse, vr))
            vr = EVR_UNKNOWN;

        // parse optional value
        if (!errorOnThisLine && !parseValue(parse, value, vr))
        {
            CERR << OFFIS_CONSOLE_APPLICATION ": "<< ifname << ": "
                 << "incorrect value specification (line " << lineNumber << ")"<< endl;
            errorOnThisLine = OFTrue;
        }


        // insert new element that consists of tag, VR, and value
        if (!errorOnThisLine)
        {
            OFCondition l_error = EC_Normal;
            if (tagkey.getGroup() == 2)
            {
                if (metaheader)
                    l_error = insertIntoSet(metaheaderStack, tagkey, vr, value);
            }
            else
                l_error = insertIntoSet(datasetStack, tagkey, vr, value);

            if (value)
            {
                delete[] value;
                value = NULL;
            }
            if (l_error != EC_Normal)
            {
                errorOnThisLine = OFTrue;
                CERR << OFFIS_CONSOLE_APPLICATION ": " << ifname << ": Error in creating Element: "
                     << l_error.text() << " (line " << lineNumber << ")"<< endl;
            }

        }

        if (errorOnThisLine)
            errorsEncountered++;
    }


    // test blocking structure
    if (metaheader && metaheaderStack.card() != 1)
    {
        CERR << OFFIS_CONSOLE_APPLICATION ": " << ifname << ": Block Error in metaheader" << endl;
        errorsEncountered++;
    }

    if (datasetStack.card() != 1)
    {
        CERR << OFFIS_CONSOLE_APPLICATION ": " << ifname << ": Block Error in dataset" << endl;
        errorsEncountered++;
    }

    delete[] lineBuf;

    if (errorsEncountered)
    {
        CERR << errorsEncountered << " Errors found in " <<  ifname << endl;
        return !stopOnErrors;
    }
    else
        return OFTrue;
}

// ********************************************

int main(int argc, char *argv[])
{
#ifdef HAVE_GUSI_H
    GUSISetup(GUSIwithSIOUXSockets);
    GUSISetup(GUSIwithInternetSockets);
#endif

    SetDebugLevel(( 0 ));

    OFConsoleApplication app(OFFIS_CONSOLE_APPLICATION , "Convert ASCII dump to DICOM file", rcsid);
    OFCommandLine cmd;

    cmd.setOptionColumns(LONGCOL, SHORTCOL);
    cmd.setParamColumn(LONGCOL + SHORTCOL + 4);

    cmd.addParam("dumpfile-in", "dump input filename");
    cmd.addParam("dcmfile-out", "DICOM output filename");

    cmd.addGroup("general options:", LONGCOL, SHORTCOL + 2);
     cmd.addOption("--help",                      "-h",        "print this help text and exit");
     cmd.addOption("--version",                                "print version information and exit", OFTrue /* exclusive */);
     cmd.addOption("--verbose",                   "-v",        "verbose mode, print processing details");
     cmd.addOption("--debug",                     "-d",        "debug mode, print debug information");

    cmd.addGroup("input options:", LONGCOL, SHORTCOL + 2);
      cmd.addOption("--line",                     "+l",    1,  "[m]ax-length: integer",
                                                               "maximum line length m (default 4096)");

    cmd.addGroup("output options:");
      cmd.addSubGroup("output file format:");
        cmd.addOption("--write-file",             "+F",        "write file format (default)");
        cmd.addOption("--write-dataset",          "-F",        "write data set without file meta information");
      cmd.addSubGroup("output transfer syntax:");
        cmd.addOption("--write-xfer-little",      "+te",       "write with explicit VR little endian (default)");
        cmd.addOption("--write-xfer-big",         "+tb",       "write with explicit VR big endian TS");
        cmd.addOption("--write-xfer-implicit",    "+ti",       "write with implicit VR little endian TS");
      cmd.addSubGroup("error handling:");
        cmd.addOption("--stop-on-error",          "-E",        "do not write if dump is damaged (default)");
        cmd.addOption("--ignore-errors",          "+E",        "attempt to write even if dump is damaged");
      cmd.addSubGroup("post-1993 value representations:");
        cmd.addOption("--enable-new-vr",          "+u",        "enable support for new VRs (UN/UT) (default)");
        cmd.addOption("--disable-new-vr",         "-u",        "disable support for new VRs, convert to OB");
      cmd.addSubGroup("group length encoding:");
        cmd.addOption("--group-length-recalc",    "+g=",       "recalculate group lengths if present (default)");
        cmd.addOption("--group-length-create",    "+g",        "always write with group length elements");
        cmd.addOption("--group-length-remove",    "-g",        "always write without group length elements");
      cmd.addSubGroup("length encoding in sequences and items:");
        cmd.addOption("--length-explicit",        "+e",        "write with explicit lengths (default)");
        cmd.addOption("--length-undefined",       "-e",        "write with undefined lengths");
      cmd.addSubGroup("data set trailing padding (not with --write-dataset):");
        cmd.addOption("--padding-retain",         "-p=",       "do not change padding\n(default if not --write-dataset)");
        cmd.addOption("--padding-off",            "-p",        "no padding (implicit if --write-dataset)");
        cmd.addOption("--padding-create",         "+p",    2,  "[f]ile-pad [i]tem-pad: integer",
                                                               "align file on multiple of f bytes\nand items on multiple of i bytes");

    int opt_debugMode = 0;
    const char* ifname = NULL;
    const char* ofname = NULL;
    E_TransferSyntax oxfer = EXS_LittleEndianExplicit;
    E_EncodingType oenctype = EET_ExplicitLength;
    E_GrpLenEncoding oglenc = EGL_recalcGL;
    E_PaddingEncoding opadenc = EPD_withoutPadding;
    OFCmdUnsignedInt opt_filepad = 0;
    OFCmdUnsignedInt opt_itempad = 0;
    OFCmdUnsignedInt opt_linelength = DCM_DumpMaxLineSize;
    OFBool verbosemode = OFFalse;
    OFBool stopOnErrors = OFTrue;
    OFBool createFileFormat = OFTrue;

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
#ifdef WITH_ZLIB
            CERR << endl << "- ZLIB, Version " << zlibVersion() << endl;
#else
            CERR << " none" << endl;
#endif
            return 0;
         }
      }

      /* command line parameters */

      cmd.getParam(1, ifname);
      cmd.getParam(2, ofname);

      if (cmd.findOption("--verbose")) verbosemode = OFTrue;
      if (cmd.findOption("--debug")) opt_debugMode = 5;

      if (cmd.findOption("--line"))
      {
          app.checkValue(cmd.getValueAndCheckMin(opt_linelength, 80));
      }

      cmd.beginOptionBlock();
      if (cmd.findOption("--write-file")) createFileFormat = OFTrue;
      if (cmd.findOption("--write-dataset")) createFileFormat = OFFalse;
      cmd.endOptionBlock();

      cmd.beginOptionBlock();
      if (cmd.findOption("--write-xfer-little")) oxfer = EXS_LittleEndianExplicit;
      if (cmd.findOption("--write-xfer-big")) oxfer = EXS_BigEndianExplicit;
      if (cmd.findOption("--write-xfer-implicit")) oxfer = EXS_LittleEndianImplicit;
      cmd.endOptionBlock();

      cmd.beginOptionBlock();
      if (cmd.findOption("--stop-on-error")) stopOnErrors = OFTrue;
      if (cmd.findOption("--ignore-errors")) stopOnErrors = OFFalse;
      cmd.endOptionBlock();

      cmd.beginOptionBlock();
      if (cmd.findOption("--enable-new-vr"))
      {
        dcmEnableUnknownVRGeneration.set(OFTrue);
        dcmEnableUnlimitedTextVRGeneration.set(OFTrue);
      }
      if (cmd.findOption("--disable-new-vr"))
      {
        dcmEnableUnknownVRGeneration.set(OFFalse);
        dcmEnableUnlimitedTextVRGeneration.set(OFFalse);
      }
      cmd.endOptionBlock();

      cmd.beginOptionBlock();
      if (cmd.findOption("--group-length-recalc")) oglenc = EGL_recalcGL;
      if (cmd.findOption("--group-length-create")) oglenc = EGL_withGL;
      if (cmd.findOption("--group-length-remove")) oglenc = EGL_withoutGL;
      cmd.endOptionBlock();

      cmd.beginOptionBlock();
      if (cmd.findOption("--length-explicit")) oenctype = EET_ExplicitLength;
      if (cmd.findOption("--length-undefined")) oenctype = EET_UndefinedLength;
      cmd.endOptionBlock();

      cmd.beginOptionBlock();
      if (cmd.findOption("--padding-retain"))
      {
        if (!createFileFormat) app.printError("--padding-retain not allowed with --write-dataset");
        opadenc = EPD_noChange;
      }
      if (cmd.findOption("--padding-off")) opadenc = EPD_withoutPadding;
      if (cmd.findOption("--padding-create"))
      {
          if (!createFileFormat) app.printError("--padding-create not allowed with --write-dataset");
          app.checkValue(cmd.getValueAndCheckMin(opt_filepad, 0));
          app.checkValue(cmd.getValueAndCheckMin(opt_itempad, 0));
          opadenc = EPD_withPadding;
      }
      cmd.endOptionBlock();

    }

    DcmFileFormat fileformat;
    DcmMetaInfo * metaheader = fileformat.getMetaInfo();
    DcmDataset * dataset = fileformat.getDataset();

    SetDebugLevel((opt_debugMode));

    /* make sure data dictionary is loaded */
    if (!dcmDataDict.isDictionaryLoaded()) {
    CERR << "Warning: no data dictionary loaded, "
         << "check environment variable: "
         << DCM_DICT_ENVIRONMENT_VARIABLE << endl;
    }

    if (verbosemode)
        COUT << "reading dump file: " << ifname << endl;

    // open input dump file
    if ((ifname == NULL) || (strlen(ifname) == 0))
    {
        CERR << "invalid input filename: <empty string>" << endl;
        return 1;
    }
    FILE * dumpfile = fopen(ifname, "r");
    if (!dumpfile)
    {
        CERR << "input file does not exist: " << ifname << endl;
        return 1;
    }

    // read dump file into metaheader and dataset
    if (readDumpFile(metaheader, dataset, dumpfile, ifname, stopOnErrors,
        OFstatic_cast(unsigned long, opt_linelength)))
    {
        // write into file format or dataset
        if (verbosemode)
            COUT << "writing DICOM file" << endl;

        OFCondition l_error = fileformat.saveFile(ofname, oxfer, oenctype, oglenc, opadenc,
            OFstatic_cast(Uint32, opt_filepad), OFstatic_cast(Uint32, opt_itempad), !createFileFormat);

        if (l_error == EC_Normal)
            COUT << "dump successfully converted." << endl;
        else
        {
            CERR << "Error: " << l_error.text()
                 << ": writing file: "  << ofname << endl;
            return 1;
        }
    }

    return 0;
}

//*******************************

/*
** CVS/RCS Log:
** $Log: dump2dcm.cc,v $
** Revision 1.51  2005/12/16 09:07:03  onken
** - Added variable initialization to avoid compiler warning
**
** Revision 1.50  2005/12/08 15:40:50  meichel
** Changed include path schema for all DCMTK header files
**
** Revision 1.49  2004/07/13 09:43:10  meichel
** Fixed memory leak occuring when raw data is read from file.
**
** Revision 1.48  2004/03/05 09:59:00  joergr
** Avoid wrong warning for LUTData (0028,3006) having a VR of US or SS.
** Added initial "hooks" for (compressed) pixel items.
** Added "ignore errors" option (similar to dcmdump).
**
** Revision 1.47  2004/01/16 10:53:16  joergr
** Adapted type casts to new-style typecast operators defined in ofcast.h.
** Removed acknowledgements with e-mail addresses from CVS log.
**
** Revision 1.46  2003/11/05 16:15:27  meichel
** Removed useless "--write-xfer-same" command line option
**
** Revision 1.45  2002/12/05 13:59:29  joergr
** Fixed typo.
**
** Revision 1.44  2002/11/27 12:07:18  meichel
** Adapted module dcmdata to use of new header file ofstdinc.h
**
** Revision 1.43  2002/11/26 08:43:02  meichel
** Replaced all includes for "zlib.h" with <zlib.h>
**   to avoid inclusion of zlib.h in the makefile dependencies.
**
** Revision 1.42  2002/09/23 17:52:04  joergr
** Prepared code for future support of 'config.guess' host identifiers.
**
** Revision 1.41  2002/09/23 13:50:42  joergr
** Added new command line option "--version" which prints the name and version
** number of external libraries used.
**
** Revision 1.40  2002/08/21 10:14:16  meichel
** Adapted code to new loadFile and saveFile methods, thus removing direct
**   use of the DICOM stream classes.
**
** Revision 1.39  2002/04/16 13:38:55  joergr
** Added configurable support for C++ ANSI standard includes (e.g. streams).
**
** Revision 1.38  2001/12/11 14:00:39  joergr
** Fixed bug in 'dump2dcm' parser causing AT attribute values to be ignored.
**
** Revision 1.37  2001/11/09 15:50:53  joergr
** Renamed some of the getValue/getParam methods to avoid ambiguities reported
** by certain compilers.
**
** Revision 1.36  2001/09/25 17:21:01  meichel
** Adapted dcmdata to class OFCondition
**
** Revision 1.35  2001/06/01 15:48:30  meichel
** Updated copyright header
**
** Revision 1.34  2000/04/14 15:42:54  meichel
** Global VR generation flags are now derived from OFGlobal and, thus,
**   safe for use in multi-thread applications.
**
** Revision 1.33  2000/03/08 16:26:06  meichel
** Updated copyright header.
**
** Revision 1.32  2000/03/06 18:09:38  joergr
** Avoid empty statement in the body of if-statements (MSVC6 reports warnings).
**
** Revision 1.31  2000/03/03 14:05:16  meichel
** Implemented library support for redirecting error messages into memory
**   instead of printing them to stdout/stderr for GUI applications.
**
** Revision 1.30  2000/02/29 11:48:51  meichel
** Removed support for VS value representation. This was proposed in CP 101
**   but never became part of the standard.
**
** Revision 1.29  2000/02/23 15:11:36  meichel
** Corrected macro for Borland C++ Builder 4 workaround.
**
** Revision 1.28  2000/02/10 16:02:51  joergr
** Enhanced handling of PixelData/Item element. Externally stored raw data is
** now always imported as little endian and swapped if necessary. This change
** reflects the new 'export' feature of dcmdump.
**
** Revision 1.27  2000/02/01 10:11:58  meichel
** Avoiding to include <stdlib.h> as extern "C" on Borland C++ Builder 4,
**   workaround for bug in compiler header files.
**
** Revision 1.26  1999/05/03 14:13:40  joergr
** Minor code purifications to keep Sun CC 2.0.1 quiet.
**
** Revision 1.25  1999/04/27 17:50:53  joergr
** Adapted console applications to new OFCommandLine and OFConsoleApplication
** functionality.
**
** Revision 1.24  1999/04/27 12:23:27  meichel
** Prevented dcmdata applications from opening a file with empty filename,
**   leads to application crash on Win32.
**
** Revision 1.23  1999/03/31 09:24:23  meichel
** Updated copyright header in module dcmdata
**
** Revision 1.22  1999/03/29 10:14:15  meichel
** Adapted command line options of dcmdata applications to new scheme.
**
** Revision 1.21  1999/03/22 16:16:01  meichel
** dump2dcm now allows to include the contents of binary files
**   as OB/OW values while converting a dump to a DICOM file.
**
** Revision 1.20  1999/01/07 14:13:12  meichel
** Corrected bug in dump2dcm that prevented the correct processing of
**   dumps created with dcmdump if they contained the "internal" VR markers
**   "xs" (US or SS) or "ox" (OB or OW).
**
** Revision 1.19  1998/01/27 10:51:27  meichel
** Removed some unused variables, meaningless const modifiers
**   and unreached statements.
**
** Revision 1.18  1998/01/14 14:41:15  hewett
** Modified existing -u command line option to also disable generation
** of UT and VS (previously just disabled generation of UN).
**
** Revision 1.17  1997/08/05 07:34:54  andreas
** Corrected Error handling of SQ in dump2dcm
**
** Revision 1.16  1997/07/21 07:59:02  andreas
** - Deleted support for DcmPixelItems and DcmPixelSequences in dump2dcm
**   ToDo: New support should be added in the future compatible to
**   the new DcmPixel class.
** - Replace all boolean types (BOOLEAN, CTNBOOLEAN, DICOM_BOOL, BOOL)
**   with one unique boolean type OFBool.
**
** Revision 1.15  1997/07/03 15:09:40  andreas
** - removed debugging functions Bdebug() and Edebug() since
**   they write a static array and are not very useful at all.
**   Cdebug and Vdebug are merged since they have the same semantics.
**   The debugging functions in dcmdata changed their interfaces
**   (see dcmdata/include/dcdebug.h)
**
** Revision 1.14  1997/05/30 06:44:57  andreas
** - fixed scanf format problem leading to warnings on 64 bit machines.
**
** Revision 1.13  1997/05/29 15:52:52  meichel
** Added constant for dcmtk release date in dcuid.h.
** All dcmtk applications now contain a version string
** which is displayed with the command line options ("usage" message)
** and which can be queried in the binary with the "ident" command.
**
** Revision 1.12  1997/05/22 13:26:25  hewett
** Modified the test for presence of a data dictionary to use the
** method DcmDataDictionary::isDictionaryLoaded().
**
** Revision 1.11  1997/05/20 07:57:12  andreas
** - Removed obsolete applications file2ds and ds2file. The functionality of these
**   applications is now peformed by dcmconv. Unified calling parameters
**   are implemented in dump2dcm, dcmdump and dcmconv.
**
** Revision 1.10  1997/05/16 08:31:06  andreas
** - Revised handling of GroupLength elements and support of
**   DataSetTrailingPadding elements. The enumeratio E_GrpLenEncoding
**   got additional enumeration values (for a description see dctypes.h).
**   addGroupLength and removeGroupLength methods are replaced by
**   computeGroupLengthAndPadding. To support Padding, the parameters of
**   element and sequence write functions changed.
**
** Revision 1.9  1997/04/18 08:06:56  andreas
** - Minor corrections: correct some warnings of the SUN-C++ Compiler
**   concerning the assignments of wrong types and inline compiler
**   errors
** - The put/get-methods for all VRs did not conform to the C++-Standard
**   draft. Some Compilers (e.g. SUN-C++ Compiler, Metroworks
**   CodeWarrier, etc.) create many warnings concerning the hiding of
**   overloaded get methods in all derived classes of DcmElement.
**   So the interface of all value representation classes in the
**   library are changed rapidly, e.g.
**   OFCondition get(Uint16 & value, const unsigned long pos);
**   becomes
**   OFCondition getUint16(Uint16 & value, const unsigned long pos);
**   All (retired) "returntype get(...)" methods are deleted.
**   For more information see dcmdata/include/dcelem.h
**
** Revision 1.8  1997/03/27 15:47:25  hewett
** Added command line switche to allow generation of UN to be
** disabled (it is enabled by default).
**
** Revision 1.7  1996/09/24 16:13:51  hewett
** Added preliminary support for the Macintosh environment (GUSI library).
**
** Revision 1.6  1996/05/02 17:00:23  hewett
** Corrected program name in usage description.
**
** Revision 1.5  1996/05/02 15:55:11  hewett
** Stopped whitespace being stripped from inside value strings when
** no [] delimiter present.  Now only leading and trailing whitespace
** is stripped.
**
** Revision 1.4  1996/04/27 12:13:01  hewett
** Corrected bug in last bug-fix.  A tag value [some text] was being
** parsed as an empty string.  Now both [] and [some text] appear to
** work as intended.
**
** Revision 1.3  1996/03/22 12:38:44  andreas
** Correct some mistakes: handling [] as empty string (no value field)
**                        handling =Name correct if Name is not correct
**
** Revision 1.2  1996/03/12 15:11:39  hewett
** Added call to prepareCmdLineArgs to enable command line arguments
** in environments which do not provide them.
**
** Revision 1.1  1996/01/29 13:36:38  andreas
** dump2dcm added convert ASCII descriptions into DICOM files
**
**
*/
