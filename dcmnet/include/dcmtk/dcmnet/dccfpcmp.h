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
 *  Module:  dcmnet
 *
 *  Author:  Marco Eichelberg
 *
 *  Purpose: 
 *    class DcmPresentationContextItem
 *    class DcmPresentationContextMap
 *
 *  Last Update:      $Author: meichel $
 *  Update Date:      $Date: 2005/12/08 16:02:11 $
 *  Source File:      $Source: /share/dicom/cvs-depot/dcmtk/dcmnet/include/dcmtk/dcmnet/dccfpcmp.h,v $
 *  CVS/RCS Revision: $Revision: 1.3 $
 *  Status:           $State: Exp $
 *
 *  CVS/RCS Log at end of file
 *
 */

#ifndef DCCFPCMP_H
#define DCCFPCMP_H

#include "dcmtk/config/osconfig.h"
#include "dcmtk/ofstd/oflist.h"   /* for class OFList<> */
#include "dcmtk/ofstd/ofstring.h" /* for class OFString */
#include "dcmtk/ofstd/ofcond.h"   /* for class OFCondition */
#include "dcmtk/dcmnet/dcmsmap.h"  /* for class DcmSimpleMap<> */
#include "dcmtk/dcmnet/dccfuidh.h" /* for class DcmUIDHandler */


/** this helper class is a presentation context list entry.
 *  Not intended for use by the end user.
 */
class DcmPresentationContextItem
{
public:
  /** constructor
   *  @param abstractSyntax abstract syntax
   *  @param xferSyntaxGroup symbolic key for transfer syntax list
   */
  DcmPresentationContextItem(
    const DcmUIDHandler& abstractSyntax,
    const OFString& xferSyntaxGroup );

  /// copy constructor
  DcmPresentationContextItem(const DcmPresentationContextItem& arg);
 
  /// destructor
  ~DcmPresentationContextItem();

  /** checks if the given argument matches the abstract syntax UID
   *  maintained by this object
   *  @param arg argument to compare to
   *  @return true if equal, false otherwise
   */
  OFBool matches(const DcmUIDHandler& arg) const
  {
    return arg == uid_;
  }

  /** returns the abstract syntax UID
   *  @return abstract syntax UID, never NULL
   */
  const char *getAbstractSyntax() const
  {
    return uid_.c_str();
  }

  /** returns the key for the list of transfer syntaxes maintained
   *  by this object
   *  @return transfer syntax key
   */
  const char *getTransferSyntaxKey() const
  {
    return xferSyntaxGroup_.c_str();
  }

  /** comparison operator.
   *  @param arg object to compare with
   *  @return true if equal
   */
  OFBool operator==(const DcmPresentationContextItem& arg) const
  {
    return (uid_ == arg.uid_) && (xferSyntaxGroup_ == arg.xferSyntaxGroup_); 
  }

private:

  /// private undefined copy assignment operator
  DcmPresentationContextItem& operator=(const DcmPresentationContextItem& arg);

  /// abstract syntax UID
  DcmUIDHandler uid_;

  /// key of the transfer syntax group
  OFString xferSyntaxGroup_;
};


/** this helper class is a simple list of presentation context list entries.
 *  Not intended for use by the end user.
 */
typedef OFList<DcmPresentationContextItem> DcmPresentationContextList;


/** this helper class maintains a map of presentation context lists.
 *  Not intended for use by the end user.
 */
class DcmPresentationContextMap
{
public:
  /// constructor
  DcmPresentationContextMap();

  /// destructor
  ~DcmPresentationContextMap();

  /** add new entry to list within map.
   *  If key is new, new list is created. Otherwise value
   *  is appended to existing list.
   *  @param key map key
   *  @param abstractSyntaxUID abstract syntax UID
   *  @param transferSyntaxKey symbolic name of transfer syntax UID list, not checked here.
   *  @return EC_Normal if successful, an error code otherwise
   */
  OFCondition add(
    const char *key,
    const char *abstractSyntaxUID,
    const char *transferSyntaxKey);

  /** checks if the key is known
   *  @param key key name, must not be NULL
   *  @return true if key is known, false otherwise
   */
  OFBool isKnownKey(const char *key) const;

  /** checks if the given abstract syntax is contained in the list
   *  identified by the given key.
   *  @param key presentation context list to search
   *  @param abstractSyntax abstract syntax UID to search
   *  @return true if abstract syntax is in list, false otherwise
   */
  OFBool isKnownAbstractSyntax(const char *key, const DcmUIDHandler& abstractSyntax) const;

  /** returns the list of presentation contexts identified by the given key
   *  @param key presentation context list to search
   *  @return pointer to presentation context list if found, NULL otherwise
   */
  const DcmPresentationContextList *getPresentationContextList(const char *key) const;

private:
  /// private undefined copy constructor
  DcmPresentationContextMap(const DcmPresentationContextMap& arg);

  /// private undefined copy assignment operator
  DcmPresentationContextMap& operator=(const DcmPresentationContextMap& arg);

  /// map of presentation context lists
  DcmSimpleMap<DcmPresentationContextList *> map_;

};

#endif

/*
 * CVS/RCS Log
 * $Log: dccfpcmp.h,v $
 * Revision 1.3  2005/12/08 16:02:11  meichel
 * Changed include path schema for all DCMTK header files
 *
 * Revision 1.2  2003/06/18 08:16:16  meichel
 * Added comparison operators to keep MSVC5 compiler happy
 *
 * Revision 1.1  2003/06/10 14:27:33  meichel
 * Initial release of class DcmAssociationConfiguration and support
 *   classes. This class maintains a list of association negotiation
 *   profiles that can be addressed by symbolic keys. The profiles may
 *   be read from a configuration file.
 *
 *
 */
