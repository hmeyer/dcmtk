/*
 *
 *  Copyright (C) 2000-2005, OFFIS
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
 *  Module: dcmsr
 *
 *  Author: Joerg Riesmeier
 *
 *  Purpose:
 *    classes: DSRWaveformChannelItem, DSRWaveformChannelList
 *
 *  Last Update:      $Author: meichel $
 *  Update Date:      $Date: 2005/12/08 16:05:32 $
 *  CVS/RCS Revision: $Revision: 1.12 $
 *  Status:           $State: Exp $
 *
 *  CVS/RCS Log at end of file
 *
 */


#ifndef DSRWAVCH_H
#define DSRWAVCH_H

#include "dcmtk/config/osconfig.h"   /* make sure OS specific configuration is included first */

#include "dcmtk/dcmsr/dsrtypes.h"
#include "dcmtk/dcmsr/dsrtlist.h"


/*---------------------*
 *  class declaration  *
 *---------------------*/

/** Class for waveform channel items
 */
class DSRWaveformChannelItem
{
  public:

    /** (default) constructor
     ** @param  multiplexGroupNumber  optional multiplex group number
     *  @param  channelNumber         optional channel number
     */
    DSRWaveformChannelItem(const Uint16 multiplexGroupNumber = 0,
                           const Uint16 channelNumber = 0)
      : MultiplexGroupNumber(multiplexGroupNumber),
        ChannelNumber(channelNumber)
    {
    }

    /** comparison operator
     ** @param  item  item with which the (m,c) pair should be compared
     ** @return OFTrue if both pairs are equal, OFFalse otherwise
     */
    inline OFBool operator==(const DSRWaveformChannelItem &item) const
    {
        return (item.MultiplexGroupNumber == MultiplexGroupNumber) && (item.ChannelNumber == ChannelNumber);
    }

    /** comparison operator
     ** @param  item  item with which the (m,c) pair should be compared
     ** @return true if not equal, false if equal
     */
    inline OFBool operator!=(const DSRWaveformChannelItem &item) const
    {
        return (item.MultiplexGroupNumber != MultiplexGroupNumber) || (item.ChannelNumber != ChannelNumber);
    }

    /** comparison operator
     ** @param  item  item with which the (m,c) pair should be compared
     ** @return true if smaller. ChannelNumber takes precedence over MultiplexGroupNumber.
     */
    inline OFBool operator<(const DSRWaveformChannelItem &item) const
    {
        return (ChannelNumber < item.ChannelNumber) || ((ChannelNumber == item.ChannelNumber) && (MultiplexGroupNumber < item.MultiplexGroupNumber));
    }

    /** comparison operator
     ** @param  item  item with which the (m,c) pair should be compared
     ** @return true if greater. ChannelNumber takes precedence over MultiplexGroupNumber.
     */
    inline OFBool operator>(const DSRWaveformChannelItem &item) const
    {
        return (ChannelNumber > item.ChannelNumber) || ((ChannelNumber == item.ChannelNumber) && (MultiplexGroupNumber > item.MultiplexGroupNumber));
    }

    /* copy constructor and assignment operator are defined implicitly */

    /// multiplex group number value (VR=US)
    Uint16 MultiplexGroupNumber;
    /// channel number value (VR=US)
    Uint16 ChannelNumber;
};


/** Class for waveform channel lists
 */
class DSRWaveformChannelList
  : public DSRListOfItems<DSRWaveformChannelItem>
{

  public:

    /** default constructor
     */
    DSRWaveformChannelList();

    /** copy constructor
     ** @param  lst  list to be copied
     */
    DSRWaveformChannelList(const DSRWaveformChannelList &lst);

    /** destructor
     */
    virtual ~DSRWaveformChannelList();

    /** assignment operator
     ** @param  lst  list to be copied
     ** @return reference to this list after 'lst' has been copied
     */
    DSRWaveformChannelList &operator=(const DSRWaveformChannelList &lst);

    /** print list of waveform channels.
     *  The output of a typical list looks like this: 1/2,3/4,5/6
     ** @param  stream         output stream to which the list should be printed
     *  @param  flags          flag used to customize the output (see DSRTypes::PF_xxx)
     *  @param  pairSeparator  character specifying the separator between the value pairs
     *  @param  itemSeparator  character specifying the separator between the list items
     ** @return status, EC_Normal if successful, an error code otherwise
     */
    OFCondition print(ostream &stream,
                      const size_t flags = 0,
                      const char pairSeparator = '/',
                      const char itemSeparator = ',') const;

    /** read list of waveform channels
     ** @param  dataset    DICOM dataset from which the list should be read
     *  @param  logStream  pointer to error/warning output stream (output disabled if NULL)
     ** @return status, EC_Normal if successful, an error code otherwise
     */
    OFCondition read(DcmItem &dataset,
                     OFConsole *logStream);

    /** write list of waveform channels
     ** @param  dataset    DICOM dataset to which the list should be written
     *  @param  logStream  pointer to error/warning output stream (output disabled if NULL)
     ** @return status, EC_Normal if successful, an error code otherwise
     */
    OFCondition write(DcmItem &dataset,
                      OFConsole *logStream) const;

    /** check whether specified value pair is contained in the list
     ** @param  multiplexGroupNumber  multiplex group number to be checked
     *  @param  channelNumber         channel number to be checked
     ** @return OFTrue if the value pair is in the list, OFFalse otherwise
     */
    OFBool isElement(const Uint16 multiplexGroupNumber,
                     const Uint16 channelNumber) const;

    /** get copy of the specified value pair
     ** @param  idx                   index of the value pair to be returned (starting from 1)
     *  @param  multiplexGroupNumber  multiplex group number of the specified index (set to 0)
     *  @param  channelNumber         channel number of the specified index (set to 0 first)
     ** @return status, EC_Normal if successful, an error code otherwise
     */
    OFCondition getItem(const size_t idx,
                        Uint16 &multiplexGroupNumber,
                        Uint16 &channelNumber) const;

    /** add value pair to the list
     ** @param  multiplexGroupNumber  multiplex group number to be added
     *  @param  channelNumber         channel number to be added
     */
    void addItem(const Uint16 multiplexGroupNumber,
                 const Uint16 channelNumber);

    /** put list of waveform channels as a string.
     *  This function expects the same input format as created by print(), i.e. a comma
     *  separated list of numerical value pairs.
     ** @param  stringValue  string value to be set
     ** @return status, EC_Normal if successful, an error code otherwise
     */
    OFCondition putString(const char *stringValue);
};


#endif


/*
 *  CVS/RCS Log:
 *  $Log: dsrwavch.h,v $
 *  Revision 1.12  2005/12/08 16:05:32  meichel
 *  Changed include path schema for all DCMTK header files
 *
 *  Revision 1.11  2003/08/07 13:06:29  joergr
 *  Added new putString() method.
 *
 *  Revision 1.10  2003/06/04 12:33:13  meichel
 *  Added comparison operators, needed by MSVC5 with STL
 *
 *  Revision 1.9  2003/06/03 10:16:44  meichel
 *  Renamed local variables to avoid name clashes with STL
 *
 *  Revision 1.8  2001/09/26 13:04:15  meichel
 *  Adapted dcmsr to class OFCondition
 *
 *  Revision 1.7  2001/05/07 16:13:24  joergr
 *  Updated CVS header.
 *
 *  Revision 1.6  2001/01/25 11:48:44  joergr
 *  Corrected typos / enhanced comments.
 *
 *  Revision 1.5  2000/11/06 11:21:26  joergr
 *  Added parameter to print() method specifying the item separator character.
 *
 *  Revision 1.4  2000/11/01 16:22:57  joergr
 *  Updated comments/formatting.
 *
 *  Revision 1.3  2000/10/18 17:10:51  joergr
 *  Added doc++ comments.
 *
 *  Revision 1.2  2000/10/16 11:52:29  joergr
 *  Added new method checking whether a waveform content item applies to a
 *  certain channel.
 *
 *  Revision 1.1  2000/10/13 07:49:36  joergr
 *  Added new module 'dcmsr' providing access to DICOM structured reporting
 *  documents (supplement 23).  Doc++ documentation not yet completed.
 *
 *
 */
