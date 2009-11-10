/*
 *
 *  Copyright (C) 2003-2005, OFFIS
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
 *  Author:  Michael Onken
 *
 *  Purpose: Class for modifying DICOM files from comandline
 *
 *  Last Update:      $Author: meichel $
 *  Update Date:      $Date: 2005/12/08 15:46:50 $
 *  CVS/RCS Revision: $Revision: 1.12 $
 *  Status:           $State: Exp $
 *
 *  CVS/RCS Log at end of file
 *
 */

#ifndef MDFCONEN_H
#define MDFCONEN_H

#include "dcmtk/config/osconfig.h"   // make sure OS specific configuration is included first
#include "mdfdsman.h"
#include "dcmtk/ofstd/oftypes.h"
#include "dcmtk/ofstd/ofconapp.h"
#include "dcmtk/ofstd/oflist.h"
#include "dcmtk/ofstd/ofcond.h"
#include "dcmtk/dcmdata/dctagkey.h"

/** class reflecting a modify operation (called Job in this context)
 */
class MdfJob {

public :

    OFString option;
    OFString path;
    OFString value;

    /** Comparison operator between Jobs
     */
    OFBool operator==(const MdfJob& j) const;

private :

    /** private undefined copy constructor
     */
    MdfJob &operator=(const MdfJob& j);
};


/** This class encapsulates data structures and operations for modifying
 *  Dicom files from the commandline
 */
class MdfConsoleEngine
{
public:

    /** Constructor
     *  @param argc Number of commandline arguments
     *  @param argv Array holding the commandline arguments
     *  @param appl_name Name of calling application, that instantiates
     *                   this class
     */
    MdfConsoleEngine(int argc, char *argv[],
                     const char* appl_name);

    /** Destructor
     */
    ~MdfConsoleEngine();

    /** This function looks at commandline options and decides what to do.
     *  It evaluates option values from commandline and prepares them for
     *  starting the corresponding private functions.
     *  @return Returns 0 if successful, another value if errors occurreds
     */
    int startProvidingService();

protected:
    ///helper class for console applications
    OFConsoleApplication *app;
    ///helper class for commandline parsing
    OFCommandLine *cmd;
    ///ds_man holds dataset manager, that is used for modify operations
    MdfDatasetManager *ds_man;
    ///verbose mode
    OFBool verbose_option;
    ///debug mode
    OFBool debug_option;
    ///ignore errors option
    OFBool ignore_errors_option;
    //if false, metaheader UIDs are not updated when related dataset UIDs change
    OFBool update_metaheader_uids_option;
    ///read file with or without metaheader
    E_FileReadMode read_mode_option;
    ///denotes the expected transfersyntax
    E_TransferSyntax input_xfer_option;

    ///decides whether to with/without metaheader
    OFBool output_dataset_option;
    ///denotes the transfer syntax that should be written
    E_TransferSyntax output_xfer_option;
    ///option for group length recalcing
    E_GrpLenEncoding glenc_option;
    ///write explicit or implicit length encoding
    E_EncodingType enctype_option;
    ///padding output
    E_PaddingEncoding padenc_option;
    ///internal padding variables
    OFCmdUnsignedInt filepad_option;
    OFCmdUnsignedInt itempad_option;

    ///list of jobs to be executed
    OFList<MdfJob> *jobs;
    ///list of files to be modified
    OFList<OFString> *files;

    /** Checks for non-job commandline options like --debug etc. and
     *  sets corresponding internal flags
     */
    void parseNonJobOptions();

    /** Parses commandline options into corresponding file- and job lists and
     *  enables debug/verbose mode. The joblist is built in order of modify
     *  options on commandline
     */
    void parseCommandLine();

    /** This function splits a modify option (inclusive value) as
     *  found on commandline into to parts (path and value)
     *  e.g. "(0010,0010)=value" into path "(0010,0010)" and "value"
     *  @param string to be splitted
     *  @param path returns part containing the path
     *  @param value returns part containing the value(if theres one)
     */
    void splitPathAndValue(const OFString &whole,
                                 OFString &path,
                                 OFString &value);

    /** Executes given modify job
     *  @param job job to be executed
     *  @return returns 0 if no error occured, else the number of errors
     */
    int executeJob(const MdfJob &job);

    /** Backup and load file into internal MdfDatasetManager
     *  @param filename name of file to load
     *  @return OFCondition, whether loading/backuping was successful including
     *          error description
     */
    OFCondition loadFile(const char* filename);

    /** Backup given file from file to file.bak
     *  @param file_name filename of file, that should be backuped
     *  @return OFCondition, whether backup was successful or not
     */
    OFCondition backupFile(const char *file_name);

    /** Restore given file from file.bak to original (without .bak)
     *  @param restore "filename".bak to original without .bak
     *  @return OFCondition, whether restoring was successful
     */
    OFCondition restoreFile(const char *filename);

    /** The function handles three strings, that are directly printed
     *  after another. The whole message is then terminated by \n
     *  @param condition message is printed, if condition is true
     *  @param s1 first message string
     *  @param s2 second message string
     *  @param s2 third message string
     */
    void debugMsg(const OFBool &condition,
                  const OFString &s1,
                  const OFString &s2,
                  const OFString &s3);

private:

    /** private undefined assignment operator
     */
    MdfConsoleEngine &operator=(const MdfConsoleEngine &);

    /** private undefined copy constructor
     */
    MdfConsoleEngine(const MdfConsoleEngine &);

};

#endif //MDFCONEN_H

/*
** CVS/RCS Log:
** $Log: mdfconen.h,v $
** Revision 1.12  2005/12/08 15:46:50  meichel
** Updated Makefiles to correctly install header files
**
** Revision 1.11  2005/12/02 09:19:26  joergr
** Added new command line option that checks whether a given file starts with a
** valid DICOM meta header.
**
** Revision 1.10  2004/11/05 17:17:24  onken
** Added input and output options for dcmodify. minor code enhancements.
**
** Revision 1.9  2004/10/22 16:53:26  onken
** - fixed ignore-errors-option
** - major enhancements for supporting private tags
** - removed '0 Errors' output
** - modifications to groups 0000,0001,0002,0003,0005 and 0007 are blocked,
**   removing tags with group 0001,0003,0005 and 0007 is still possible
** - UID options:
**   - generate new study, series and instance UIDs
**   - When changing UIDs in dataset, related metaheader tags are updated
**     automatically
** - minor code improvements
**
** Revision 1.8  2004/04/19 14:45:07  onken
** Restructured code to avoid default parameter values for "complex types" like
** OFString. Required for Sun CC 2.0.1.
**
** Revision 1.7  2003/12/10 16:19:20  onken
** Changed API of MdfDatasetManager, so that its transparent for user, whether
** he wants to modify itemtags or tags at 1. level.
**
** Complete rewrite of MdfConsoleEngine. It doesn't support a batchfile any more,
** but now a user can give different modify-options at the same time on
** commandline. Other purifications and simplifications were made.
**
** Revision 1.6  2003/11/11 10:55:51  onken
** - debug-mechanism doesn't use debug(..) any more
** - comments purified
** - headers adjustet to debug-modifications
**
** Revision 1.5  2003/10/13 14:51:49  onken
** improved backup-strategy
**
** Revision 1.4  2003/10/01 14:04:03  onken
** Corrected doxygen-information in headerfiles
**
** Revision 1.3  2003/09/19 12:43:54  onken
** major bug fixes, corrections for "dcmtk-coding-style", better error-handling
**
** Revision 1.2  2003/07/09 12:13:13  meichel
** Included dcmodify in MSVC build system, updated headers
**
** Revision 1.1  2003/06/26 09:17:18  onken
** Added commandline-application dcmodify.
**
**
*/
