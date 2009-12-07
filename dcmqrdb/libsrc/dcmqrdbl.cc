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

#include <iostream>

#define INCLUDE_CCTYPE
#define INCLUDE_CSTDARG
#include "dcmtk/ofstd/ofstdinc.h"
#include "dcmtk/dcmdata/dcfilefo.h"
#include "dcmtk/dcmdata/dcdict.h"
#include "dcmtk/dcmnet/dimse.h"
#include "dcmtk/dcmnet/diutil.h"
#include "dcmtk/dcmqrdb/dcmqrdbs.h"


#include "dcmtk/dcmqrdb/dcmqrcnf.h"
#include "dcmtk/dcmqrdb/dcmqrdbl.h"
#include "dcmtk/dcmqrdb/dcmqrdbl-taglist.h"


#include "CLucene.h"
#include "boost/filesystem/operations.hpp"
#include "boost/filesystem/path.hpp"
#include "boost/format.hpp"
#include "boost/lambda/lambda.hpp"
#include "boost/bind.hpp"
#include <string>
#include <map>
#include <list>
#include <algorithm>
#include <exception>

using namespace lucene::index;
using namespace lucene::analysis;
using namespace lucene::document;
using namespace lucene::search;
namespace fs =boost::filesystem;

typedef std::list< DcmTagKey > TagListType;
typedef std::map< DcmTagKey, LuceneString > TagValueMapType;
typedef std::map< DcmTagKey, std::string > TagStdValueMapType;
typedef std::multimap< DcmTagKey, std::string > TagMultiStdValueMapType;

struct luceneData { // TODO: implement Singleton based IndexWriter and IndexSearcher
  Analyzer *analyzer;
  IndexWriter *indexwriter;
  IndexSearcher *indexsearcher;
  Document *imageDoc;
  Hits *findResponseHits;
  unsigned int findResponseHitCounter;
  TagListType findRequestList;
  std::string queryLevelString;
  Hits *moveResponseHits;
  unsigned int moveResponseHitCounter;
};

const OFConditionConst DcmQRLuceneIndexErrorC(OFM_imagectn, 0x001, OF_error, "DcmQR Lucene Index Error");
const OFCondition DcmQRLuceneIndexError(DcmQRLuceneIndexErrorC);
const OFConditionConst DcmQRLuceneNoSOPIUIDErrorC(OFM_imagectn, 0x002, OF_error, "DcmQR Lucene no DCM_SOPInstanceUID");
const OFCondition DcmQRLuceneNoSOPIUIDError(DcmQRLuceneNoSOPIUIDErrorC);
const OFConditionConst DcmQRLuceneDoubleSOPIUIDErrorC(OFM_imagectn, 0x003, OF_error, "DcmQR Lucene double DCM_SOPInstanceUID");
const OFCondition DcmQRLuceneDoubleSOPIUIDError(DcmQRLuceneDoubleSOPIUIDErrorC);

class LowerCaseAnalyzer: public Analyzer {
public:  
  LowerCaseAnalyzer();
  virtual ~LowerCaseAnalyzer();
    TokenStream* tokenStream(const TCHAR* fieldName, CL_NS(util)::Reader* reader);
	TokenStream* reusableTokenStream(const TCHAR* fieldName, CL_NS(util)::Reader* reader);
};
  
DcmQueryRetrieveLuceneIndexHandle::DcmQueryRetrieveLuceneIndexHandle(
  const OFString &storageArea,
  DcmQRLuceneIndexType indexType,
  OFCondition& result):storageArea(storageArea),indexType(indexType),doCheckFindIdentifier(OFFalse),doCheckMoveIdentifier(OFFalse),debugLevel(10)
{
  ldata = new luceneData();
  ldata->analyzer = new LowerCaseAnalyzer();
  if (indexType == DcmQRLuceneWriter) {
    ldata->imageDoc = new Document();
    bool indexExists = false;
    if ( IndexReader::indexExists( getIndexPath().c_str() ) ) {
	//Force unlock index incase it was left locked
	if ( IndexReader::isLocked(getIndexPath().c_str()) )
	    IndexReader::unlock(getIndexPath().c_str());
	indexExists = true;
    }
    try {
      ldata->indexwriter = new IndexWriter( getIndexPath().c_str(),
					    ldata->analyzer, !indexExists);
    } catch(CLuceneError &e) {
      CERR << "Exception while creation of IndexWriter caught:" << e.what() << endl;
      result = DcmQRLuceneIndexError;
    }
    try {
      ldata->indexsearcher = new IndexSearcher( ldata->indexwriter->getDirectory());
    } catch(CLuceneError &e) {
      CERR << "Exception while creation of IndexSearcher caught:" << e.what() << endl;
      result = DcmQRLuceneIndexError;
    }
  } else if (indexType == DcmQRLuceneReader) {
    try {
      ldata->indexsearcher = new IndexSearcher( getIndexPath().c_str());
    } catch(CLuceneError &e) {
      CERR << "Exception while creation of IndexSearcher caught:" << e.what() << endl;
      result = DcmQRLuceneIndexError;
    }
  }
}

DcmQueryRetrieveLuceneIndexHandle::~DcmQueryRetrieveLuceneIndexHandle() {
  if (indexType == DcmQRLuceneWriter) {
    ldata->imageDoc->clear();
    delete(ldata->imageDoc);
  }
  ldata->indexsearcher->close();
  delete( ldata->indexsearcher );
  if (indexType == DcmQRLuceneWriter) {
    ldata->indexwriter->optimize();
    ldata->indexwriter->close();
    delete( ldata->indexwriter);
  }
  delete( ldata->analyzer );
  delete( ldata );
}


OFString DcmQueryRetrieveLuceneIndexHandle::getIndexPath(void) {
  fs::path storagePath( storageArea.c_str() );
  fs::path indexPath = storagePath / LUCENEPATH;
  if (!fs::is_directory( indexPath ))
    fs::create_directory( indexPath );
  return indexPath.string().c_str();
}

void DcmQueryRetrieveLuceneIndexHandle::printIndexFile(void) {
  IndexReader *reader = ldata->indexsearcher->getReader();
  Document myDoc;
  for( int i = 0; i < reader->numDocs(); i++) {
    reader->document( i, myDoc, NULL );
    const Document::FieldsType *fields = myDoc.getFields();
    for(Document::FieldsType::const_iterator fi = fields->begin(); fi!= fields->end(); fi++) {
      const TCHAR* fname = (*fi)->name();
      const TCHAR* fvalue = myDoc.get( fname );
      const LuceneString fvaluestr( fvalue );
      if (fvaluestr.length() > 0) {

	if (dcmDataDict.isDictionaryLoaded()) {
	  
	}
	COUT << LuceneString( fname ).toStdString() << ":" << fvaluestr.toStdString();
	if (fi+1!=fields->end()) COUT << "|";
      }
    }
    COUT << endl;
    myDoc.clear();
  }
}

bool DcmQueryRetrieveLuceneIndexHandle::tagSupported (DcmTagKey tag)
{
  return DcmQRLuceneTagKeyMap.find( tag ) != DcmQRLuceneTagKeyMap.end();
}


void DcmQueryRetrieveLuceneIndexHandle::setIdentifierChecking(OFBool checkFind, OFBool checkMove)
{
    doCheckFindIdentifier = checkFind;
    doCheckMoveIdentifier = checkMove;
}

void DcmQueryRetrieveLuceneIndexHandle::setDebugLevel(int dLevel)
{
//    debugLevel = dLevel;
}

OFCondition DcmQueryRetrieveLuceneIndexHandle::pruneInvalidRecords()
{
    throw std::runtime_error(std::string(__FUNCTION__) + ": not Implemented yet!");
}

OFCondition DcmQueryRetrieveLuceneIndexHandle::cancelMoveRequest(DcmQueryRetrieveDatabaseStatus* status)
{
  ldata->moveResponseHitCounter = 0;
  if (ldata->moveResponseHits) {
    delete ldata->moveResponseHits;
    ldata->moveResponseHits =  NULL;
  }
  status->setStatus(STATUS_MOVE_Cancel_SubOperationsTerminatedDueToCancelIndication);
  return (EC_Normal) ;        
}

OFCondition DcmQueryRetrieveLuceneIndexHandle::nextMoveResponse(char* SOPClassUID, char* SOPInstanceUID, char* imageFileName, short unsigned int* numberOfRemainingSubOperations, DcmQueryRetrieveDatabaseStatus* status)
{
  if (ldata->moveResponseHitCounter > 0) {
    Document &responseDoc = ldata->findResponseHits->doc( ldata->moveResponseHitCounter++ );
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
    
    *numberOfRemainingSubOperations = ldata->moveResponseHits->length() - ldata->moveResponseHitCounter;
    
    strcpy (SOPClassUID, SOPClassUIDString.c_str()) ;
    strcpy (SOPInstanceUID, SOPInstanceUIDString.c_str()) ;
    strcpy (imageFileName, fileNameString.c_str()) ;

    status->setStatus(STATUS_Pending);
    dbdebug(1,"%s: STATUS_Pending", __FUNCTION__) ;
    return (EC_Normal);
  } else {
    cancelMoveRequest(status);
    dbdebug(1, "%s : STATUS_Success", __FUNCTION__) ;
    status->setStatus(STATUS_Success);
    return (EC_Normal) ;
  }
}

  

Query *generateQuery(const Lucene_Entry &entryData, const std::string &svalue) {
  LuceneString lvalue(svalue);
  if (entryData.fieldType == Lucene_Entry::NAME_TYPE || entryData.fieldType == Lucene_Entry::TEXT_TYPE) lvalue = lvalue.toLower();
  
std::cerr << "fieldType: " << entryData.fieldType << std::endl;
std::cerr << "value: " << lvalue.toStdString() << std::endl;
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
  } else {
    throw std::runtime_error(std::string(__FUNCTION__) + ":Key Class not supported");
  }
}
  


OFCondition DcmQueryRetrieveLuceneIndexHandle::startMoveRequest(const char* SOPClassUID, DcmDataset* moveRequestIdentifiers, DcmQueryRetrieveDatabaseStatus* status)
{
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
	dbdebug(1, "%s : unsupported Tag %s in Find-Request - dropping Tag", __FUNCTION__, elementTag.toString().c_str()) ;
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
      queryLevel = li->second; ldata->queryLevelString = qrLevelString;
    } else {
      dbdebug(1, "%s : Illegal query level (%s)",__FUNCTION__, qrLevelString.c_str()) ;
      status->setStatus(STATUS_MOVE_Failed_UnableToProcess);
      return (DcmQRLuceneIndexError) ;
    }
    dataMap.erase( dataMapIter ); // Remove the QueryLevel - since we found it
  }
    
  // Lucene Query
  BooleanQuery baseQuery;
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
  
  // add UIDs above Level to Lucene Query
  for( int l = baseLevel; l < maxLevel; l++) {
    dataMapIter = dataMap.find( LevelToUIDTag.find( (Lucene_LEVEL)l )->second.tag );
    if ( dataMapIter !=  dataMap.end() && dataMapIter->second.length() > 0) {
      const Lucene_Entry &UIDTag = LevelToUIDTag.find( (Lucene_LEVEL)l )->second;
      TermQuery *tq = new TermQuery( new Term( UIDTag.tagStr.c_str(), LuceneString( dataMapIter->second.c_str() ).c_str() ));
      baseQuery.add( tq, BooleanClause::MUST );
      dataMap.erase( dataMapIter );
    }
  }
  
  dbdebug(3, "%s :MoveRequest: Information Model:%i QueryLevel:%i",__FUNCTION__,rootLevel, queryLevel) ;
  
  // Iterate through Query Data and add to Lucene Query
  ldata->findRequestList.clear();
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
	dbdebug(1, "%s :Non-unique Key found in Move-Request",__FUNCTION__) ;
	status->setStatus(STATUS_MOVE_Failed_UnableToProcess);
	return (DcmQRLuceneIndexError) ;
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
  baseQuery.add( multiQuery, BooleanClause::MUST );

  dbdebug(2, "%s: searching index: %s", __FUNCTION__, LuceneString((const TCHAR*)baseQuery.toString(NULL)).toStdString().c_str());
  ldata->moveResponseHitCounter = 0;
  ldata->moveResponseHits = ldata->indexsearcher->search(&baseQuery);

  dbdebug(1, "%s found %i items", __FUNCTION__, ldata->moveResponseHits->length());

  if (ldata->moveResponseHits->length() == 0) {
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
  ldata->findRequestList.clear();
  ldata->findResponseHitCounter = 0;
  if (ldata->findResponseHits) {
    delete ldata->findResponseHits;
    ldata->findResponseHits =  NULL;
  }
  status->setStatus(STATUS_FIND_Cancel_MatchingTerminatedDueToCancelRequest);
  return (EC_Normal) ;    
}

OFCondition DcmQueryRetrieveLuceneIndexHandle::nextFindResponse(DcmDataset** findResponseIdentifiers, DcmQueryRetrieveDatabaseStatus* status)
{
    dbdebug(1, "%s : about to deliver hit #%i/%i\n", __FUNCTION__, ldata->findResponseHitCounter, ldata->findResponseHits->length()) ;
    *findResponseIdentifiers = new DcmDataset ;
    if ( *findResponseIdentifiers == NULL ) {
	dbdebug(1, "%s : could allocate ResponseIdentifiers DataSet - STATUS_FIND_Refused_OutOfResources\n", __FUNCTION__) ;
	status->setStatus(STATUS_FIND_Refused_OutOfResources);
        return (DcmQRLuceneIndexError);
    }

    /*** Put responses
    **/
    if (ldata->findResponseHitCounter < ldata->findResponseHits->length()) {
      Document &responseDoc = ldata->findResponseHits->doc( ldata->findResponseHitCounter++ );
      const Document::FieldsType *responseFields = responseDoc.getFields();
      
      for( TagListType::const_iterator i=ldata->findRequestList.begin(); i!=ldata->findRequestList.end(); i++) {
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
			    DCM_QueryRetrieveLevel, ldata->queryLevelString.c_str());
			    
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
      queryLevel = li->second; ldata->queryLevelString = qrLevelString;
    } else {
      dbdebug(1, "%s : Illegal query level (%s)",__FUNCTION__, qrLevelString.c_str()) ;
      status->setStatus(STATUS_FIND_Failed_UnableToProcess);
      return (DcmQRLuceneIndexError) ;
    }
    dataMap.erase( dataMapIter ); // Remove the QueryLevel - since we found it
  }

  // Lucene Query
  BooleanQuery boolQuery;
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
  // add UIDs above Level to Lucene Query
  for( int l = baseLevel; l < maxLevel; l++) {
    dataMapIter = dataMap.find( LevelToUIDTag.find( (Lucene_LEVEL)l )->second.tag );
    if ( dataMapIter !=  dataMap.end() && dataMapIter->second.length() > 0) {
      const Lucene_Entry &UIDTag = LevelToUIDTag.find( (Lucene_LEVEL)l )->second;
      TermQuery *tq = new TermQuery( new Term( UIDTag.tagStr.c_str(), LuceneString( dataMapIter->second.c_str() ).c_str() ));
      boolQuery.add( tq, BooleanClause::MUST );
    } 
  }

  dbdebug(3, "%s :FindRequest: Information Model:%i QueryLevel:%i",__FUNCTION__,rootLevel, queryLevel) ;
  
  // Iterate through Query Data and add to Lucene Query
  ldata->findRequestList.clear();
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
      ldata->findRequestList.push_back( entryData.tag );
  }

  dbdebug(2, "%s: searching index: %s", __FUNCTION__, LuceneString((const TCHAR*)boolQuery.toString(NULL)).toStdString().c_str());
  ldata->findResponseHitCounter = 0;
  ldata->findResponseHits = ldata->indexsearcher->search(&boolQuery);

  dbdebug(1, "%s found %i items", __FUNCTION__, ldata->findResponseHits->length());

  if (ldata->findResponseHits->length() == 0) {
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
    throw std::runtime_error(std::string(__FUNCTION__) + ": not Implemented yet!");
}

bool checkAndStoreDataForLevel( Lucene_LEVEL level, TagValueMapType &dataset, luceneData *ldata  ) { // returns true if Object already existed in level
  LevelTagMapType::const_iterator uidTagIter = LevelToUIDTag.find( level );
  if (uidTagIter == LevelToUIDTag.end() ) std::runtime_error(std::string(__FUNCTION__) + ": level " + toString(level) +" not found!");
  const Lucene_Entry &UIDTagEntry = uidTagIter->second;
  TagValueMapType::const_iterator uidDataIter = dataset.find( UIDTagEntry.tag );
  if (uidDataIter == dataset.end() ) std::runtime_error(std::string(__FUNCTION__) + ": tag " + UIDTagEntry.tagStr.toStdString() + " not found!");

  TermQuery tq( new Term( UIDTagEntry.tagStr.c_str(), uidDataIter->second.c_str() ) );
  Hits *hits = ldata->indexsearcher->search(&tq);
  if (hits->length()>0) {
    delete hits;
    return true;
  } else {
    delete hits;
    ldata->imageDoc->clear();
    ldata->imageDoc->add( *new Field( FieldNameDocumentDicomLevel.c_str(), QRLevelStringMap.find( level )->second.c_str(), Field::STORE_YES| Field::INDEX_UNTOKENIZED| Field::TERMVECTOR_NO ) );
    for(TagValueMapType::const_iterator i=dataset.begin(); i != dataset.end(); i++) {
      const Lucene_Entry &tag = DcmQRLuceneTagKeyMap.find( i->first )->second;
      if (tag.level <= level) {
	int tokenizeFlag =  (tag.fieldType == Lucene_Entry::NAME_TYPE || tag.fieldType == Lucene_Entry::TEXT_TYPE) ? Field::INDEX_TOKENIZED : Field::INDEX_UNTOKENIZED;
	ldata->imageDoc->add( *new Field( DcmQRLuceneTagKeyMap.find( i->first )->second.tagStr.c_str(), i->second.c_str() , Field::STORE_YES| tokenizeFlag | Field::TERMVECTOR_NO ) );
      }
    }
    ldata->indexwriter->addDocument(ldata->imageDoc);
    return false;
  }
}


OFCondition DcmQueryRetrieveLuceneIndexHandle::storeRequest(const char* SOPClassUID, const char* SOPInstanceUID, const char* imageFileName, DcmQueryRetrieveDatabaseStatus* status, OFBool isNew)
{
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
      TermQuery tq( new Term( FieldNameDCM_SOPInstanceUID.c_str(), LuceneString( SOPInstanceUID ).c_str() ) );
      Hits *hits = ldata->indexsearcher->search(&tq);
      if (hits->length()>0) {
	CERR << "storeRequest():\"" << imageFileName << "\" - DCM_SOPInstanceUID already exists, rejecting" << endl;
	delete hits;
	return DcmQRLuceneDoubleSOPIUIDError;
      }
      delete hits;
    }

    DcmDataset *dset = dcmff.getDataset();
    
    typedef std::map< DcmTagKey, LuceneString > DataMapType;
    TagValueMapType dataMap;

    for(DcmQRLuceneTagListIterator i = DcmQRLuceneTagList.begin(); i!=DcmQRLuceneTagList.end(); i++) {
      OFCondition ec = EC_Normal;
      const char *strPtr = NULL;
      ec = dset->findAndGetString(i->tag, strPtr);
      if ((ec != EC_Normal) || (strPtr == NULL)) {
	  dbdebug(1,"storeRequest (): %s: value empty or not found",i->tag.toString().c_str());
	  strPtr = "";
      }
      dataMap.insert( TagValueMapType::value_type( i->tag, LuceneString( strPtr ) ) );
    }
    if (!checkAndStoreDataForLevel( SERIE_LEVEL, dataMap, ldata ))
      if (!checkAndStoreDataForLevel( STUDY_LEVEL, dataMap, ldata ))
	checkAndStoreDataForLevel( PATIENT_LEVEL, dataMap, ldata );

    ldata->imageDoc->clear();
    ldata->imageDoc->add( *new Field( FieldNameDocumentDicomLevel.c_str(), QRLevelStringMap.find( IMAGE_LEVEL )->second.c_str(), Field::STORE_YES| Field::INDEX_UNTOKENIZED| Field::TERMVECTOR_NO ) );
    for(TagValueMapType::const_iterator i=dataMap.begin(); i != dataMap.end(); i++) {
      const Lucene_Entry &tag = DcmQRLuceneTagKeyMap.find( i->first )->second;
      int tokenizeFlag =  (tag.fieldType == Lucene_Entry::NAME_TYPE || tag.fieldType == Lucene_Entry::TEXT_TYPE) ? Field::INDEX_TOKENIZED : Field::INDEX_UNTOKENIZED;
      ldata->imageDoc->add( *new Field( DcmQRLuceneTagKeyMap.find( i->first )->second.tagStr.c_str(), i->second.c_str() , Field::STORE_YES| tokenizeFlag | Field::TERMVECTOR_NO ) );
    }
    ldata->imageDoc->add( *new Field( FieldNameObjectStatus.c_str(), ((isNew) ? ObjectStatusIsNew : ObjectStatusIsNotNew).c_str(), Field::STORE_YES| Field::INDEX_UNTOKENIZED| Field::TERMVECTOR_NO ) );
    ldata->imageDoc->add( *new Field( FieldNameDicomFileName.c_str(), LuceneString(imageFileName).c_str(), Field::STORE_YES| Field::INDEX_UNTOKENIZED| Field::TERMVECTOR_NO ) );
    ldata->imageDoc->add( *new Field( FieldNameDCM_SOPClassUID.c_str(), LuceneString(SOPClassUID).c_str(), Field::STORE_YES| Field::INDEX_UNTOKENIZED| Field::TERMVECTOR_NO ) );
    

    /* InstanceDescription */
    OFBool useDescrTag = OFTrue;
    DcmTagKey descrTag = DCM_ImageComments;
    LuceneString description;
    if (SOPClassUID != NULL)
    {
        /* fill in value depending on SOP class UID (content might be improved) */
        if (strcmp(SOPClassUID, UID_GrayscaleSoftcopyPresentationStateStorage) == 0)
        {
            descrTag = DCM_ContentDescription;
        } else if (strcmp(SOPClassUID, UID_HardcopyGrayscaleImageStorage) == 0)
        {
	    description = "Hardcopy Grayscale Image";
            useDescrTag = OFFalse;
        } else if ((strcmp(SOPClassUID, UID_BasicTextSR) == 0) ||
                   (strcmp(SOPClassUID, UID_EnhancedSR) == 0) ||
                   (strcmp(SOPClassUID, UID_ComprehensiveSR) == 0))
        {
            OFString string;
            description = "unknown SR";
            const char *name = dcmFindNameOfUID(SOPClassUID);
            if (name != NULL)
                description = name;
            if (dset->findAndGetOFString(DCM_VerificationFlag, string) == EC_Normal)
            {
                description += LuceneString( ", " );
                description += LuceneString( string.c_str() );
            }
            if (dset->findAndGetOFString(DCM_CompletionFlag, string) == EC_Normal)
            {
                description += LuceneString(", ");
                description += LuceneString(string.c_str());
            }
            if (dset->findAndGetOFString(DCM_CompletionFlagDescription, string) == EC_Normal)
            {
                description += LuceneString(", ");
                description += LuceneString(string.c_str());
            }
            useDescrTag = OFFalse;
        } else if (strcmp(SOPClassUID, UID_StoredPrintStorage) == 0)
        {
	    description = LuceneString("Stored Print");
            useDescrTag = OFFalse;
        }
    }
    /* get description from attribute specified above */
    if (useDescrTag)
    {
        OFString string;
        /* return value is irrelevant */
        dset->findAndGetOFString(descrTag, string);
	description = string.c_str();
    }
    /* is dataset digitally signed? */
        DcmStack stack;
        if (dset->search(DCM_DigitalSignaturesSequence, stack, ESM_fromHere, OFTrue /* searchIntoSub */) == EC_Normal)
        {
            /* in principle it should be checked whether there is _any_ non-empty digital signatures sequence, but ... */
            if (((DcmSequenceOfItems *)stack.top())->card() > 0)
            {
                if (description.length() > 0)
		    description += LuceneString(" (Signed)");
                else
		    description += LuceneString("Signed Instance");
            }
        }
    ldata->imageDoc->add( *new Field( FieldNameInstanceDescription.c_str(), description.c_str(), Field::STORE_YES| Field::INDEX_TOKENIZED| Field::TERMVECTOR_NO ) );
    ldata->indexwriter->addDocument(ldata->imageDoc);
    ldata->imageDoc->clear();
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



LowerCaseAnalyzer::LowerCaseAnalyzer(){}
LowerCaseAnalyzer::~LowerCaseAnalyzer(){}
TokenStream* LowerCaseAnalyzer::tokenStream(const TCHAR* fieldName, CL_NS(util)::Reader* reader){
    return new LowerCaseTokenizer(reader);
}
TokenStream* LowerCaseAnalyzer::reusableTokenStream(const TCHAR* fieldName, CL_NS(util)::Reader* reader)
{
	Tokenizer* tokenizer = static_cast<Tokenizer*>(getPreviousTokenStream());
	if (tokenizer == NULL) {
		tokenizer = new LowerCaseTokenizer(reader);
		setPreviousTokenStream(tokenizer);
	} else
		tokenizer->reset(reader);
	return tokenizer;
}
    
