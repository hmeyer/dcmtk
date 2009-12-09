/*
   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#ifndef DCMQRDBL_H
#define DCMQRDBL_H

#include "dcmtk/ofstd/ofstdinc.h"
#include "dcmtk/dcmqrdb/dcmqrdba.h"    /* for class DcmQueryRetrieveDatabaseHandle */
#include "dcmtk/dcmdata/dctagkey.h"
#include <boost/scoped_ptr.hpp>
#include <string>

#define LUCENEPATH "lucene_index"

class DcmQueryRetrieveConfig;
class DcmQRDBLHImpl;

enum DcmQRLuceneIndexType
{
  DcmQRLuceneReader,
  DcmQRLuceneWriter,
  DcmQRLuceneModifier
};


class DcmQueryRetrieveLuceneIndexHandle : public DcmQueryRetrieveDatabaseHandle
{
public:
  DcmQueryRetrieveLuceneIndexHandle(
    const OFString &storageArea,
    DcmQRLuceneIndexType indexType,
    OFCondition& result);
  ~DcmQueryRetrieveLuceneIndexHandle();
  void printIndexFile(void);
  virtual OFCondition storeRequest(const char* SOPClassUID, const char* SOPInstanceUID, const char* imageFileName, DcmQueryRetrieveDatabaseStatus* status, OFBool isNew = OFTrue);
private:
  virtual void setIdentifierChecking(OFBool checkFind, OFBool checkMove);
  virtual void setDebugLevel(int debugLevel);
  virtual OFCondition pruneInvalidRecords();
  virtual OFCondition cancelMoveRequest(DcmQueryRetrieveDatabaseStatus* status);
  virtual OFCondition nextMoveResponse(char* SOPClassUID, char* SOPInstanceUID, char* imageFileName, short unsigned int* numberOfRemainingSubOperations, DcmQueryRetrieveDatabaseStatus* status);
  virtual OFCondition startMoveRequest(const char* SOPClassUID, DcmDataset* moveRequestIdentifiers, DcmQueryRetrieveDatabaseStatus* status);
  virtual OFCondition cancelFindRequest(DcmQueryRetrieveDatabaseStatus* status);
  virtual OFCondition nextFindResponse(DcmDataset** findResponseIdentifiers, DcmQueryRetrieveDatabaseStatus* status);
  virtual OFCondition startFindRequest(const char* SOPClassUID, DcmDataset* findRequestIdentifiers, DcmQueryRetrieveDatabaseStatus* status);
  virtual OFCondition makeNewStoreFileName(const char* SOPClassUID, const char* SOPInstanceUID, char* newImageFileName);

  bool tagSupported(DcmTagKey tag);
  
  

  /// flag indicating whether or not the check function for FIND requests is enabled
  OFBool doCheckFindIdentifier;

  /// flag indicating whether or not the check function for MOVE requests is enabled
  OFBool doCheckMoveIdentifier;

  /// current debug level
  int debugLevel;
    
  boost::scoped_ptr<DcmQRDBLHImpl> impl;

  void dbdebug(int level, const char* format, ...) const;

  
};

/** CLucene database factory class. Instances of this class are able to create database
 *  handles for a given called application entity title.
 */
class DcmQueryRetrieveLuceneIndexReaderHandleFactory: public DcmQueryRetrieveDatabaseHandleFactory
{
public:

  /** constructor
   *  @param config system configuration object, must not be NULL.
   */
  DcmQueryRetrieveLuceneIndexReaderHandleFactory(const DcmQueryRetrieveConfig *config);

  /// destructor
  virtual ~DcmQueryRetrieveLuceneIndexReaderHandleFactory();

  /** this method creates a new database handle instance on the heap and returns
   *  a pointer to it, along with a result that indicates if the instance was
   *  successfully initialized, i.e. connected to the database
   *  @param callingAETitle calling aetitle
   *  @param calledAETitle called aetitle
   *  @param result result returned in this variable
   *  @return pointer to database object, must not be NULL if result is EC_Normal.
   */
  virtual DcmQueryRetrieveDatabaseHandle *createDBHandle(
    const char *callingAETitle, 
    const char *calledAETitle,
    OFCondition& result) const;

protected:

  /// pointer to system configuration
  const DcmQueryRetrieveConfig *config_;
};

class DcmQueryRetrieveLuceneIndexWriterHandleFactory: public DcmQueryRetrieveLuceneIndexReaderHandleFactory
{
public:
  virtual DcmQueryRetrieveDatabaseHandle *createDBHandle(
    const char *callingAETitle, 
    const char *calledAETitle,
    OFCondition& result) const;
};

#endif // DCMQRDBL_H
