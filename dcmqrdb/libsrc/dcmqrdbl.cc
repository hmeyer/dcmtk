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

#define INCLUDE_CCTYPE
#define INCLUDE_CSTDARG

#include <iostream>






#include <CLucene.h>

#include <boost/format.hpp>
#include <boost/lambda/lambda.hpp>
#include <boost/bind.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>

#include <map>
#include <list>
#include <string>
#include <algorithm>
#include <exception>


#include "dcmtk/dcmqrdb/dcmqrdbl-taglist.h"
#include "dcmtk/dcmqrdb/dcmqrdbl.h"
#include "dcmtk/dcmqrdb/dcmqrdblhimpl.h"

#include "dcmtk/ofstd/ofstdinc.h"
#include "dcmtk/dcmdata/dcfilefo.h"
#include "dcmtk/dcmdata/dcdict.h"
#include "dcmtk/dcmnet/diutil.h"
#include "dcmtk/dcmqrdb/dcmqrdbs.h"


#include "dcmtk/dcmqrdb/dcmqrcnf.h"


const int IndexRequestUpToDateMillis = 5000;

bool DcmQueryRetrieveLuceneIndexHandle::indexExists( const OFString &s ) {
  return DcmQRDBLHImpl::indexExists( s.c_str() );
}


DcmQueryRetrieveLuceneIndexHandle::DcmQueryRetrieveLuceneIndexHandle(
  const OFString &storageArea,
  DcmQRLuceneIndexType indexType,
  OFCondition& result):doCheckFindIdentifier(OFFalse),doCheckMoveIdentifier(OFFalse),debugLevel(10),verbose(false) {
    DcmQRDBLHImpl::Result r;
    impl.reset( new DcmQRDBLHImpl(storageArea.c_str(), indexType, r) );
    result = (r==DcmQRDBLHImpl::good) ? EC_Normal : DcmQRLuceneIndexError;
  }
  
    
DcmQueryRetrieveLuceneIndexHandle::~DcmQueryRetrieveLuceneIndexHandle() {}

void DcmQueryRetrieveLuceneIndexHandle::setVerbose(bool v) {
  verbose = v;
}



void DcmQueryRetrieveLuceneIndexHandle::printIndexFile(void) {
dbdebug(1, "%s: start (line %i)", __FUNCTION__, __LINE__) ;
  dbdebug(1,"%s:", __FUNCTION__) ;
  IndexReader &reader = impl->getIndexReader();
  Document myDoc;
  for( int i = 0; i < reader.numDocs(); i++) {
    reader.document( i, myDoc, NULL );
    COUT << LuceneString( (const TCHAR*)myDoc.toString() ).toStdString() << std::endl;
    myDoc.clear();
  }
}

bool DcmQueryRetrieveLuceneIndexHandle::tagSupported (DcmTagKey tag)
{
dbdebug(1, "%s: start (line %i)", __FUNCTION__, __LINE__) ;
  return DcmQRLuceneTagKeyMap.find( tag ) != DcmQRLuceneTagKeyMap.end();
}


void DcmQueryRetrieveLuceneIndexHandle::setIdentifierChecking(OFBool checkFind, OFBool checkMove)
{
dbdebug(1, "%s: start (line %i)", __FUNCTION__, __LINE__) ;
    doCheckFindIdentifier = checkFind;
    doCheckMoveIdentifier = checkMove;
}

void DcmQueryRetrieveLuceneIndexHandle::setDebugLevel(int dLevel)
{
dbdebug(1, "%s: start (line %i)", __FUNCTION__, __LINE__) ;
//    debugLevel = dLevel;
}

OFCondition DcmQueryRetrieveLuceneIndexHandle::pruneInvalidRecords()
{
dbdebug(1, "%s: start (line %i)", __FUNCTION__, __LINE__) ;
    throw std::runtime_error(std::string(__FUNCTION__) + ": not Implemented yet!");
}

OFCondition DcmQueryRetrieveLuceneIndexHandle::cancelMoveRequest(DcmQueryRetrieveDatabaseStatus* status)
{
dbdebug(1, "%s: start (line %i)", __FUNCTION__, __LINE__) ;
  impl->moveResponseHitCounter = 0;
  impl->moveResponseHits.reset(NULL);
  impl->moveRequest.reset(NULL);
  status->setStatus(STATUS_MOVE_Cancel_SubOperationsTerminatedDueToCancelIndication);
  return (EC_Normal) ;        
}

OFCondition DcmQueryRetrieveLuceneIndexHandle::nextMoveResponse(char* SOPClassUID, char* SOPInstanceUID, char* imageFileName, short unsigned int* numberOfRemainingSubOperations, DcmQueryRetrieveDatabaseStatus* status)
{
dbdebug(1, "%s: start (line %i)", __FUNCTION__, __LINE__) ;
  if (impl->moveResponseHits && impl->moveResponseHitCounter < impl->moveResponseHits->length()) {
    Document &responseDoc = impl->moveResponseHits->doc( impl->moveResponseHitCounter++ );
    std::string SOPClassUIDString;
    std::string SOPInstanceUIDString;
    std::string fileNameString;
    const TCHAR *c;

    c= responseDoc.get( FieldNameDCM_SOPClassUID.c_str() );
    if (c != NULL) SOPClassUIDString = LuceneString( c ).toStdString();
    c= responseDoc.get( FieldNameDCM_SOPInstanceUID.c_str() );
    if (c != NULL) SOPInstanceUIDString = LuceneString( c ).toStdString();
    c= responseDoc.get( FieldNameDicomFileName.c_str() );
    if (c != NULL) fileNameString = LuceneString( c ).toStdString();
    
    *numberOfRemainingSubOperations = impl->moveResponseHits->length() - impl->moveResponseHitCounter;
    
    strcpy (SOPClassUID, SOPClassUIDString.c_str()) ;
    strcpy (SOPInstanceUID, SOPInstanceUIDString.c_str()) ;
    strcpy (imageFileName, fileNameString.c_str()) ;

  dbdebug(1, "%s about to serve #%i of %i", __FUNCTION__, impl->moveResponseHitCounter, impl->moveResponseHits->length());
  dbdebug(1, "%s SOPClassUID:%s SOPInstanceUID:%s imageFileName:%s", __FUNCTION__, SOPClassUID, SOPInstanceUID, imageFileName);
  dbdebug(1, "%s doc is:%s", __FUNCTION__, LuceneString( (const TCHAR*)responseDoc.toString() ).toStdString().c_str());
    
    
    status->setStatus(STATUS_Pending);
    dbdebug(1,"%s: STATUS_Pending", __FUNCTION__) ;
    return (EC_Normal);
  } else {
  dbdebug(1, "%s no more results - ending", __FUNCTION__);
    cancelMoveRequest(status);
    dbdebug(1, "%s : STATUS_Success", __FUNCTION__) ;
    status->setStatus(STATUS_Success);
    return (EC_Normal) ;
  }
}

  

Query *generateQuery(const Lucene_Entry &entryData, const std::string &svalue) {
  LuceneString lvalue(svalue);
  if (entryData.fieldType == Lucene_Entry::NAME_TYPE || entryData.fieldType == Lucene_Entry::TEXT_TYPE) lvalue = lvalue.toLower();
  const TCHAR *fieldName = entryData.tagStr.c_str();
  if (entryData.keyClass == Lucene_Entry::UID_CLASS 
    || entryData.keyClass == Lucene_Entry::OTHER_CLASS) {
    return new TermQuery( new Term( fieldName, lvalue.c_str() ) );
  } else if (entryData.keyClass == Lucene_Entry::STRING_CLASS) {
    LuceneString::size_type wildcardPos = lvalue.find_first_of(L"?*");
    if (wildcardPos == LuceneString::npos) { // No Wildcards used
      return new TermQuery( new Term( fieldName, lvalue.c_str() ) );
    } else {
      return new WildcardQuery( new Term( fieldName, lvalue.c_str() ) );
    }
  } else if (entryData.keyClass == Lucene_Entry::DATE_CLASS
      || entryData.keyClass == Lucene_Entry::TIME_CLASS) {
    LuceneString::size_type otherPos = lvalue.find_first_not_of( L"0123456789-" );
    if (otherPos != std::string::npos) {	
      throw std::runtime_error(std::string(__FUNCTION__) + ":Invalid Queryformat for Date or Time:" + svalue);
    }
    LuceneString::size_type dashPos = lvalue.find_first_of( L'-' );
    if (dashPos == LuceneString::npos) { // No Range
      return new TermQuery( new Term( fieldName, lvalue.c_str() ) );
    } else if (entryData.keyClass == Lucene_Entry::DATE_CLASS) {
      if (lvalue.length() == 9) {
	if (dashPos == 0) {
	  return new RangeQuery( new Term( fieldName, lvalue.substr(1).c_str()), new Term( fieldName, L"99999999" ), true );
	} else if (dashPos == 8) {
	  return new RangeQuery( new Term( fieldName, L"00000000" ), new Term( fieldName, lvalue.substr(0,8).c_str()) , true );
	}
      } else if (lvalue.length() == 17 && dashPos ==8) {
	  return new RangeQuery( new Term( fieldName, lvalue.substr(0,8).c_str() ), new Term( fieldName, lvalue.substr(9,8).c_str() ), true );
      }
      throw std::runtime_error(std::string(__FUNCTION__) + ":Date-Queries (like \"" + svalue + "\")not supported");
    } else { // Time-Query
      if (lvalue.length() == 7) {
	if (dashPos == 0) {
	  return new RangeQuery( new Term( fieldName, lvalue.substr(1).c_str() ), new Term( fieldName, L"999999" ), true );
	} else if (dashPos == 6) {
	  return new RangeQuery( new Term( fieldName, L"000000" ), new Term( fieldName, lvalue.substr(0,6).c_str() ), true );
	}
      } else if (lvalue.length() == 13 && dashPos ==6) {
	  return new RangeQuery( new Term( fieldName, lvalue.substr(0,6).c_str() ), new Term( fieldName, lvalue.substr(7,6).c_str() ), true );
      }
      throw std::runtime_error(std::string(__FUNCTION__) + ":Time-Queries (like \"" + svalue + "\")not supported");  // TODO: enable RangeQueries
    }
  }// else {
    throw std::runtime_error(std::string(__FUNCTION__) + ":Key Class not supported");
//  }
}
  


OFCondition DcmQueryRetrieveLuceneIndexHandle::startMoveRequest(const char* SOPClassUID, DcmDataset* moveRequestIdentifiers, DcmQueryRetrieveDatabaseStatus* status)
{
dbdebug(1, "%s: start (line %i)", __FUNCTION__, __LINE__) ;
  Lucene_QUERY_CLASS rootLevel;
  /**** Is SOPClassUID supported ?
  ***/
  StringQRClassMapType::const_iterator qrClassI = StringQRClassMap.find( SOPClassUID );
  if (qrClassI != StringQRClassMap.end() && (qrClassI->second.qtype == QueryInfo::MOVE || qrClassI->second.qtype == QueryInfo::GET)) {
    rootLevel = qrClassI->second.qclass ;
  } else {
    dbdebug(1, "%s: STATUS_FIND_Refused_SOPClassNotSupported", __FUNCTION__) ;
    status->setStatus(STATUS_MOVE_Failed_SOPClassNotSupported);
    return (DcmQRLuceneIndexError) ;
  }
  // Gather all data from Request
  TagMultiStdValueMapType dataMap;
  int elemCount = (int)(moveRequestIdentifiers->card());
  for (int elemIndex=0; elemIndex<elemCount; elemIndex++) {
    DcmElement* dcelem = moveRequestIdentifiers->getElement(elemIndex);
    DcmTagKey elementTag = dcelem->getTag().getXTag();
    if (tagSupported(elementTag) || elementTag == DCM_QueryRetrieveLevel) {
      char *s = NULL;
      dcelem->getString(s);
      if (s==NULL) s = (char*)"";
      dataMap.insert( TagStdValueMapType::value_type( elementTag, s ) );
    } else {
	dbdebug(1, "%s : unsupported Tag %s in Move-Request - dropping Tag", __FUNCTION__, elementTag.toString().c_str()) ;
    }
  }

  // Find the QueryLevel
  Lucene_LEVEL queryLevel = PATIENT_LEVEL;
  TagMultiStdValueMapType::iterator dataMapIter = dataMap.find( DCM_QueryRetrieveLevel );
  if (dataMapIter == dataMap.end()) {
      status->setStatus(STATUS_FIND_Failed_IdentifierDoesNotMatchSOPClass);
      dbdebug(1,"%s: missing Query/Retrieve Level",__FUNCTION__);
      return (DcmQRLuceneIndexError) ;
  } else {
    std::string qrLevelString = dataMapIter->second;
    // Skip this line if you want strict comparison
    std::transform(qrLevelString.begin(), qrLevelString.end(), qrLevelString.begin(), (int(*)(int))toupper);
    StringQRLevelMapType::const_iterator li = StringQRLevelMap.find( qrLevelString );
    if (li != StringQRLevelMap.end()) {
      queryLevel = li->second; impl->queryLevelString = qrLevelString;
    } else {
      dbdebug(1, "%s : Illegal query level (%s)",__FUNCTION__, qrLevelString.c_str()) ;
      status->setStatus(STATUS_MOVE_Failed_UnableToProcess);
      return (DcmQRLuceneIndexError) ;
    }
    dataMap.erase( dataMapIter ); // Remove the QueryLevel - since we found it
  }
    
  // Lucene Query
  impl->moveRequest.reset(new BooleanQuery);
  BooleanQuery &baseQuery = *impl->moveRequest;
  // only look for image level objects
  baseQuery.add( new TermQuery( new Term( FieldNameDocumentDicomLevel.c_str(), ImageLevelLuceneString.c_str() ) ), BooleanClause::MUST );

  
  Lucene_LEVEL baseLevel = PATIENT_LEVEL;
  if (rootLevel == STUDY_ROOT) baseLevel = STUDY_LEVEL;
  Lucene_LEVEL maxLevel = IMAGE_LEVEL;
  if (rootLevel == PATIENT_STUDY) maxLevel = STUDY_LEVEL;
  if (doCheckFindIdentifier && queryLevel > maxLevel) {
    status->setStatus(STATUS_MOVE_Failed_UnableToProcess);
    dbdebug(1, "%s : QR-Level incompatible with Information Model (level %i)",__FUNCTION__,queryLevel) ;
    return (DcmQRLuceneIndexError) ;
  }
  queryLevel = std::min(maxLevel,queryLevel);
  
  DicomUID mostRestrictiveUID;
  // add UIDs above Level to Lucene Query
  for( int l = baseLevel; l < maxLevel; l++) {
    dataMapIter = dataMap.find( LevelToUIDTag.find( (Lucene_LEVEL)l )->second.tag );
    if ( dataMapIter !=  dataMap.end() && dataMapIter->second.length() > 0) {
      const Lucene_Entry &UIDTag = LevelToUIDTag.find( (Lucene_LEVEL)l )->second;
      LuceneString uidString( dataMapIter->second );
      mostRestrictiveUID = DicomUID( (Lucene_LEVEL)l, uidString );
      TermQuery *tq = new TermQuery( new Term( UIDTag.tagStr.c_str(), uidString.c_str() ));
      baseQuery.add( tq, BooleanClause::MUST );
      dataMap.erase( dataMapIter );
    }
  }
  
  dbdebug(3, "%s :MoveRequest: Information Model:%i QueryLevel:%i",__FUNCTION__,rootLevel, queryLevel) ;
  
  // Iterate through Query Data and add to Lucene Query
  impl->findRequestList.clear();
  BooleanQuery *multiQuery = new BooleanQuery; // Query for List of IDs
  for( dataMapIter = dataMap.begin(); dataMapIter != dataMap.end(); dataMapIter++) {
    const Lucene_Entry &entryData = DcmQRLuceneTagKeyMap.find( dataMapIter->first )->second;
    if ( entryData.level == queryLevel ) {
      if (entryData.keyAttr == Lucene_Entry::UNIQUE_KEY) {
	if (dataMapIter->second.length() > 0) {
	  Query *query = generateQuery( entryData, dataMapIter->second );
	  multiQuery->add( query , BooleanClause::SHOULD );
	} else {
	  dbdebug(1, "%s :empty unique Key found in Move-Request - ignoring",__FUNCTION__) ;
	}
      } else {
	dbdebug(1, "%s :Non-unique Key found in Move-Request: %s - ignoring",__FUNCTION__, entryData.tagStr.toStdString().c_str()) ;
      }
    } else if (entryData.level < queryLevel) {
	dbdebug(1, "%s :Multiple Unique Key found above Query Level (level %i)",__FUNCTION__,entryData.level) ;
	status->setStatus(STATUS_MOVE_Failed_IdentifierDoesNotMatchSOPClass);
	return (DcmQRLuceneIndexError) ;
    } else { // entryData.level > queryLevel
      dbdebug(1, "%s :Key (%s,level %i)found beyond query level (level %i)",__FUNCTION__,entryData.tag.toString().c_str(), entryData.level, queryLevel) ;
      status->setStatus(STATUS_MOVE_Failed_UnableToProcess);
      return (DcmQRLuceneIndexError) ;
    }
  }
  if (multiQuery->getClauseCount() != 0)
    baseQuery.add( multiQuery, BooleanClause::MUST );
  else delete(multiQuery);

  dbdebug(2, "%s: searching index: %s", __FUNCTION__, LuceneString((const TCHAR*)baseQuery.toString(NULL)).toStdString().c_str());
  impl->moveQuery(&baseQuery, IndexRequestUpToDateMillis, mostRestrictiveUID);

  dbdebug(1, "%s found %i items", __FUNCTION__, impl->moveResponseHits->length());

  if (impl->moveResponseHits->length() == 0) {
    cancelMoveRequest(status);
    dbdebug(1, "%s : STATUS_Success", __FUNCTION__) ;
    status->setStatus(STATUS_Success);
    return (EC_Normal) ;
  }
  status->setStatus(STATUS_Pending);
  return (EC_Normal) ;
}

OFCondition DcmQueryRetrieveLuceneIndexHandle::cancelFindRequest(DcmQueryRetrieveDatabaseStatus* status)
{
dbdebug(1, "%s: start (line %i)", __FUNCTION__, __LINE__) ;
  impl->findRequestList.clear();
  impl->findResponseHitCounter = 0;
  impl->findResponseHits.reset(NULL);
  impl->findRequest.reset(NULL);
  status->setStatus(STATUS_FIND_Cancel_MatchingTerminatedDueToCancelRequest);
  return (EC_Normal) ;    
}

OFCondition DcmQueryRetrieveLuceneIndexHandle::nextFindResponse(DcmDataset** findResponseIdentifiers, DcmQueryRetrieveDatabaseStatus* status)
{
dbdebug(1, "%s: start (line %i)", __FUNCTION__, __LINE__) ;
    dbdebug(1, "%s : about to deliver hit #%i/%i\n", __FUNCTION__, impl->findResponseHitCounter, impl->findResponseHits->length()) ;
    *findResponseIdentifiers = new DcmDataset ;
    if ( *findResponseIdentifiers == NULL ) {
	dbdebug(1, "%s : could allocate ResponseIdentifiers DataSet - STATUS_FIND_Refused_OutOfResources\n", __FUNCTION__) ;
	status->setStatus(STATUS_FIND_Refused_OutOfResources);
        return (DcmQRLuceneIndexError);
    }

    /*** Put responses
    **/
    if (impl->findResponseHitCounter < impl->findResponseHits->length()) {
      Document &responseDoc = impl->findResponseHits->doc( impl->findResponseHitCounter++ );
      const Document::FieldsType *responseFields = responseDoc.getFields();

      dbdebug(3, "%s response:", __FUNCTION__);
      for(Document::FieldsType::const_iterator fi = responseFields->begin(); fi!= responseFields->end(); fi++) {
	const TCHAR* fname = (*fi)->name();
	dbdebug(3, "%s %s: %s", __FUNCTION__, LuceneString( fname ).toStdString().c_str(), LuceneString( responseDoc.get( fname ) ).toStdString().c_str() );
      }
      
      
      for( TagListType::const_iterator i=impl->findRequestList.begin(); i!=impl->findRequestList.end(); i++) {
	LuceneString fieldName = LuceneString(*i);
	std::string responseValue;
	for(Document::FieldsType::const_iterator fi = responseFields->begin(); fi!= responseFields->end(); fi++) {
	  const TCHAR* fname = (*fi)->name();
	  if (fieldName.compare( fname )==0) {
	    responseValue = LuceneString(responseDoc.get( fname )).toStdString();
	  }
	}
	DcmElement *dce = newDicomElement( *i );
	if (dce == NULL) {
	    status->setStatus(STATUS_FIND_Refused_OutOfResources);
	    return DcmQRLuceneIndexError;
	}
	OFCondition ec = dce->putString(responseValue.c_str());
	if (ec != EC_Normal) {
	    CERR << __FUNCTION__ << ": cannot putString()" << endl;
	    status->setStatus(STATUS_FIND_Failed_UnableToProcess);
	    return DcmQRLuceneIndexError;
	}
	ec = (*findResponseIdentifiers)->insert(dce, OFTrue /*replaceOld*/);
	if (ec != EC_Normal) {
	    CERR << __FUNCTION__ << ": cannot insert()" << endl;
	    status->setStatus(STATUS_FIND_Failed_UnableToProcess);
	    return DcmQRLuceneIndexError;
	}
      }
      DU_putStringDOElement(*findResponseIdentifiers,
			    DCM_QueryRetrieveLevel, impl->queryLevelString.c_str());
			    
      dbdebug(1, "%s () : STATUS_Pending\n", __FUNCTION__) ;
      status->setStatus(STATUS_Pending);
      return (EC_Normal) ;
    } else { // no more Responses
      dbdebug(1, "%s () : no more Responses - STATUS_Success\n", __FUNCTION__) ;
      *findResponseIdentifiers = NULL ;
      cancelFindRequest(status);
      status->setStatus(STATUS_Success);
      return (EC_Normal) ;
    }
}


OFCondition DcmQueryRetrieveLuceneIndexHandle::startFindRequest(const char* SOPClassUID, DcmDataset* findRequestIdentifiers, DcmQueryRetrieveDatabaseStatus* status)
{
dbdebug(1, "%s: start (line %i)", __FUNCTION__, __LINE__) ;
  // Find the QR-Information Model
  Lucene_QUERY_CLASS rootLevel;
  StringQRClassMapType::const_iterator qrClassI = StringQRClassMap.find( SOPClassUID );
  if (qrClassI != StringQRClassMap.end() && qrClassI->second.qtype == QueryInfo::FIND ) {
    rootLevel = qrClassI->second.qclass ;
  } else {
      dbdebug(1, "%s: STATUS_FIND_Refused_SOPClassNotSupported", __FUNCTION__) ;
      status->setStatus(STATUS_FIND_Refused_SOPClassNotSupported);
      return (DcmQRLuceneIndexError) ;
  }

  // Gather all data from Request
  TagStdValueMapType dataMap;
  int elemCount = (int)(findRequestIdentifiers->card());
  for (int elemIndex=0; elemIndex<elemCount; elemIndex++) {
    DcmElement* dcelem = findRequestIdentifiers->getElement(elemIndex);
    DcmTagKey elementTag = dcelem->getTag().getXTag();
    if (tagSupported(elementTag) || elementTag == DCM_QueryRetrieveLevel) {
      char *s = NULL;
      dcelem->getString(s);
      if (s==NULL) s = (char*)"";
      dataMap.insert( TagStdValueMapType::value_type( elementTag, s ) );
    } else {
	dbdebug(1, "%s : unsupported Tag %s in Find-Request - dropping Tag", __FUNCTION__, elementTag.toString().c_str()) ;
    }
  }

  // Find the QueryLevel
  Lucene_LEVEL queryLevel = PATIENT_LEVEL;
  TagStdValueMapType::iterator dataMapIter = dataMap.find( DCM_QueryRetrieveLevel );
  if (dataMapIter == dataMap.end()) {
      status->setStatus(STATUS_FIND_Failed_IdentifierDoesNotMatchSOPClass);
      dbdebug(1,"%s: missing Query/Retrieve Level",__FUNCTION__);
      return (DcmQRLuceneIndexError) ;
  } else {
    std::string qrLevelString = dataMapIter->second;
    // Skip this line if you want strict comparison
    std::transform(qrLevelString.begin(), qrLevelString.end(), qrLevelString.begin(), (int(*)(int))toupper);
    StringQRLevelMapType::const_iterator li = StringQRLevelMap.find( qrLevelString );
    if (li != StringQRLevelMap.end()) {
      queryLevel = li->second; impl->queryLevelString = qrLevelString;
    } else {
      dbdebug(1, "%s : Illegal query level (%s)",__FUNCTION__, qrLevelString.c_str()) ;
      status->setStatus(STATUS_FIND_Failed_UnableToProcess);
      return (DcmQRLuceneIndexError) ;
    }
    dataMap.erase( dataMapIter ); // Remove the QueryLevel - since we found it
  }

  // Lucene Query
  impl->findRequest.reset(new BooleanQuery);
  BooleanQuery &boolQuery = *impl->findRequest;
  Lucene_LEVEL baseLevel = PATIENT_LEVEL;
  if (rootLevel == STUDY_ROOT) baseLevel = STUDY_LEVEL;
  Lucene_LEVEL maxLevel = IMAGE_LEVEL;
  if (rootLevel == PATIENT_STUDY) maxLevel = STUDY_LEVEL;
  if (doCheckFindIdentifier && queryLevel > maxLevel) {
    status->setStatus(STATUS_MOVE_Failed_UnableToProcess);
    dbdebug(1, "%s : QR-Level incompatible with Information Model (level %i)",__FUNCTION__,queryLevel) ;
    return (DcmQRLuceneIndexError) ;
  }
  queryLevel = std::min(maxLevel,queryLevel);
  
  // add Level to Lucene Query
  boolQuery.add( new TermQuery( new Term( FieldNameDocumentDicomLevel.c_str(), QRLevelStringMap.find( queryLevel)->second.c_str() ) ) , BooleanClause::MUST );
  DicomUID mostRestrictiveUID;
  // add UIDs above Level to Lucene Query
  for( int l = baseLevel; l < maxLevel; l++) {
    dataMapIter = dataMap.find( LevelToUIDTag.find( (Lucene_LEVEL)l )->second.tag );
    if ( dataMapIter !=  dataMap.end() && dataMapIter->second.length() > 0) {
      const Lucene_Entry &UIDTag = LevelToUIDTag.find( (Lucene_LEVEL)l )->second;
      LuceneString uidString( dataMapIter->second );
      mostRestrictiveUID = DicomUID( (Lucene_LEVEL)l, uidString );
      TermQuery *tq = new TermQuery( new Term( UIDTag.tagStr.c_str(), uidString.c_str() ));
      boolQuery.add( tq, BooleanClause::MUST );
    } 
  }

  dbdebug(3, "%s :FindRequest: Information Model:%i QueryLevel:%i",__FUNCTION__,rootLevel, queryLevel) ;
  
  // Iterate through Query Data and add to Lucene Query
  impl->findRequestList.clear();
  for( dataMapIter = dataMap.begin(); dataMapIter != dataMap.end(); dataMapIter++) {
    const Lucene_Entry &entryData = DcmQRLuceneTagKeyMap.find( dataMapIter->first )->second;
    if ( entryData.level == queryLevel || (
	entryData.level < queryLevel && entryData.level == PATIENT_LEVEL && rootLevel == STUDY_ROOT && queryLevel == STUDY_LEVEL)) {
      if (dataMapIter->second.length() > 0) {
	Query *query = generateQuery( entryData, dataMapIter->second );
	boolQuery.add( query , BooleanClause::MUST );
      }
    } else if (entryData.level < queryLevel) {
      if (entryData.keyAttr != Lucene_Entry::UNIQUE_KEY) {
	dbdebug(1, "%s :Non Unique Key found (level %i)",__FUNCTION__,entryData.level) ;
	status->setStatus(STATUS_FIND_Failed_IdentifierDoesNotMatchSOPClass);
	return (DcmQRLuceneIndexError) ;
      }
    } else { // entryData.level > queryLevel
      dbdebug(1, "%s :Key (%s,level %i)found beyond query level (level %i)",__FUNCTION__,entryData.tag.toString().c_str(), entryData.level, queryLevel) ;
      status->setStatus(STATUS_FIND_Failed_UnableToProcess);
      return (DcmQRLuceneIndexError) ;
    }
    // add to findRequestList
    if (entryData.level <= queryLevel)
      impl->findRequestList.push_back( entryData.tag );
  }

  dbdebug(2, "%s: searching index: %s", __FUNCTION__, LuceneString((const TCHAR*)boolQuery.toString(NULL)).toStdString().c_str());
  impl->findQuery(&boolQuery, IndexRequestUpToDateMillis, mostRestrictiveUID);
  dbdebug(1, "%s found %i items", __FUNCTION__, impl->findResponseHits->length());

  if (impl->findResponseHits->length() == 0) {
    cancelFindRequest(status);
    dbdebug(1, "%s : STATUS_Success", __FUNCTION__) ;
    status->setStatus(STATUS_Success);
    return (EC_Normal) ;
  }
  status->setStatus(STATUS_Pending);
  return (EC_Normal) ;
}


OFCondition DcmQueryRetrieveLuceneIndexHandle::makeNewStoreFileName(const char* SOPClassUID, const char* SOPInstanceUID, char* newImageFileName)
{
dbdebug(1, "%s: start (line %i)", __FUNCTION__, __LINE__) ;
    throw std::runtime_error(std::string(__FUNCTION__) + ": not Implemented yet!");
}

OFCondition DcmQueryRetrieveLuceneIndexHandle::storeRequest(const char* SOPClassUID, const char* SOPInstanceUID, const char* imageFileName, DcmQueryRetrieveDatabaseStatus* status, OFBool isNew)
{
dbdebug(1, "%s: start (line %i)", __FUNCTION__, __LINE__) ;
    dbdebug(1,"%s: storage request of file : %s", __FUNCTION__, imageFileName);
    /**** Get IdxRec values from ImageFile
    ***/

    DcmFileFormat dcmff;
    if (dcmff.loadFile(imageFileName).bad())
    {
      CERR << "Cannot open file: " << imageFileName << ": "
           << strerror(errno) << endl;
      status->setStatus(STATUS_STORE_Error_CannotUnderstand);
      return (DcmQRLuceneIndexError);
    }
    {
      if (SOPInstanceUID == NULL) {
	  CERR << __FUNCTION__ << ":\"" << imageFileName << "\" - no DCM_SOPInstanceUID, rejecting" << endl;
	  return DcmQRLuceneNoSOPIUIDError;
      }
      string existingFileName;
      if (impl->sopInstanceExists(SOPInstanceUID, existingFileName)) {
	CERR << "storeRequest():\"" << imageFileName << "\" - DCM_SOPInstanceUID already exists in \"" << existingFileName << "\", rejecting" << endl;
	return DcmQRLuceneDoubleSOPIUIDError;
      }
    }

    DcmDataset *dset = dcmff.getDataset();
    
    typedef std::map< DcmTagKey, LuceneString > DataMapType;
    TagValueMapType dataMap;

    for(DcmQRLuceneTagListIterator i = DcmQRLuceneTagList.begin(); i!=DcmQRLuceneTagList.end(); i++) {
      OFCondition ec = EC_Normal;
      const char *strPtr = NULL;
      ec = dset->findAndGetString(i->tag, strPtr);
      if ((ec != EC_Normal) || (strPtr == NULL)) {
/*	  dbdebug(1,"storeRequest (): %s: value empty or not found",i->tag.toString().c_str());*/
	  strPtr = "";
      }
      dataMap.insert( TagValueMapType::value_type( i->tag, LuceneString( strPtr ) ) );
    }
    if (!impl->checkAndStoreDataForLevel( SERIE_LEVEL, dataMap ))
      if (!impl->checkAndStoreDataForLevel( STUDY_LEVEL, dataMap ))
	impl->checkAndStoreDataForLevel( PATIENT_LEVEL, dataMap );
    
    if (verbose) {
      cout << "StoreInstance:" << imageFileName << ":" << dataMap[ DCM_PatientsName ].toStdString() << endl;
    }
      
    StringValueMapType stringDataMap;

    stringDataMap[ FieldNameObjectStatus ] = (isNew) ? ObjectStatusIsNew : ObjectStatusIsNotNew;
    stringDataMap[ FieldNameDicomFileName ] = imageFileName;
    stringDataMap[ FieldNameDCM_SOPClassUID ] = SOPClassUID;

    impl->addDocument( IMAGE_LEVEL, dataMap, stringDataMap );
    
    return EC_Normal;
}


void DcmQueryRetrieveLuceneIndexHandle::dbdebug(int level, const char* format, ...) const
{
#ifdef DEBUG
    va_list ap;
    char buf[4096]; /* we hope a message never gets larger */

    if (level <= debugLevel) {
        CERR << "Lucene Index:";
        va_start(ap, format);
        vsprintf(buf, format, ap);
        va_end(ap);
        CERR << buf << endl;
    }
#endif    
}


DcmQueryRetrieveLuceneIndexReaderHandleFactory::DcmQueryRetrieveLuceneIndexReaderHandleFactory(const DcmQueryRetrieveConfig *config)
: DcmQueryRetrieveDatabaseHandleFactory()
, config_(config)
{
}

DcmQueryRetrieveLuceneIndexReaderHandleFactory::~DcmQueryRetrieveLuceneIndexReaderHandleFactory()
{
}

DcmQueryRetrieveDatabaseHandle *DcmQueryRetrieveLuceneIndexReaderHandleFactory::createDBHandle(
    const char * /* callingAETitle */,
    const char *calledAETitle,
    OFCondition& result) const
{
  return new DcmQueryRetrieveLuceneIndexHandle(
    config_->getStorageArea(calledAETitle),
    DcmQRLuceneReader,
    result);
}

DcmQueryRetrieveDatabaseHandle *DcmQueryRetrieveLuceneIndexWriterHandleFactory::createDBHandle(
    const char * /* callingAETitle */,
    const char *calledAETitle,
    OFCondition& result) const
{
  return new DcmQueryRetrieveLuceneIndexHandle(
    config_->getStorageArea(calledAETitle),
    DcmQRLuceneWriter,
    result);
}


    
