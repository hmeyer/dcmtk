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
#include <algorithm>
#include <exception>

using namespace lucene::index;
using namespace lucene::analysis;
using namespace lucene::document;
using namespace lucene::search;
namespace fs =boost::filesystem;

typedef std::map< DcmTagKey, LuceneSmallDcmElmt > LuceneElementMapType;
typedef std::map< DcmTagKey, LuceneString > TagValueMapType;

struct luceneData {
  Analyzer *analyzer;
  IndexWriter *indexwriter;
  IndexSearcher *indexsearcher;
  Document *imageDoc;
  Hits *findResponseHits;
  unsigned int findResponseHitCounter;
  LuceneElementMapType findRequestMap;
  std::string queryLevelString;
};

const OFConditionConst DcmQRLuceneIndexErrorC(OFM_imagectn, 0x001, OF_error, "DcmQR Lucene Index Error");
const OFCondition DcmQRLuceneIndexError(DcmQRLuceneIndexErrorC);
const OFConditionConst DcmQRLuceneNoSOPIUIDErrorC(OFM_imagectn, 0x002, OF_error, "DcmQR Lucene no DCM_SOPInstanceUID");
const OFCondition DcmQRLuceneNoSOPIUIDError(DcmQRLuceneNoSOPIUIDErrorC);
const OFConditionConst DcmQRLuceneDoubleSOPIUIDErrorC(OFM_imagectn, 0x003, OF_error, "DcmQR Lucene double DCM_SOPInstanceUID");
const OFCondition DcmQRLuceneDoubleSOPIUIDError(DcmQRLuceneDoubleSOPIUIDErrorC);

void testFindRequestList(const LuceneElementMapType &findRequestMap, Lucene_LEVEL queryLevel, Lucene_LEVEL infLevel, Lucene_LEVEL lowestLevel);


DcmQueryRetrieveLuceneIndexHandle::DcmQueryRetrieveLuceneIndexHandle(
  const OFString &storageArea,
  DcmQRLuceneIndexType indexType,
  OFCondition& result):storageArea(storageArea),indexType(indexType),doCheckFindIdentifier(OFFalse),doCheckMoveIdentifier(OFFalse),debugLevel(10)
{
  
  ldata = new luceneData();
  ldata->analyzer = new SimpleAnalyzer();
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
    throw std::runtime_error(std::string(__FUNCTION__) + ": not Implemented yet!");
}

OFCondition DcmQueryRetrieveLuceneIndexHandle::nextMoveResponse(char* SOPClassUID, char* SOPInstanceUID, char* imageFileName, short unsigned int* numberOfRemainingSubOperations, DcmQueryRetrieveDatabaseStatus* status)
{
    throw std::runtime_error(std::string(__FUNCTION__) + ": not Implemented yet!");
}

OFCondition DcmQueryRetrieveLuceneIndexHandle::startMoveRequest(const char* SOPClassUID, DcmDataset* moveRequestIdentifiers, DcmQueryRetrieveDatabaseStatus* status)
{
    throw std::runtime_error(std::string(__FUNCTION__) + ": not Implemented yet!");
}

OFCondition DcmQueryRetrieveLuceneIndexHandle::cancelFindRequest(DcmQueryRetrieveDatabaseStatus* status)
{
  ldata->findRequestMap.clear();
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
      
      for( LuceneElementMapType::const_iterator i=ldata->findRequestMap.begin(); i!=ldata->findRequestMap.end(); i++) {
	LuceneString fieldName = LuceneString(i->first);
	std::string responseValue;
	for(Document::FieldsType::const_iterator fi = responseFields->begin(); fi!= responseFields->end(); fi++) {
	  const TCHAR* fname = (*fi)->name();
	  if (fieldName.compare( fname )==0) {
	    responseValue = LuceneString(responseDoc.get( fname )).toStdString();
	  }
	}
	DcmElement *dce = newDicomElement( i->first );
	if (dce == NULL) {
	    status->setStatus(STATUS_FIND_Refused_OutOfResources);
	    return DcmQRLuceneIndexError;
	}
	if (i->second.Value.length() > 0) {
	    OFCondition ec = dce->putString(responseValue.c_str());
	    if (ec != EC_Normal) {
		CERR << __FUNCTION__ << ": DB_nextFindResponse: cannot put()" << endl;
		status->setStatus(STATUS_FIND_Failed_UnableToProcess);
		return DcmQRLuceneIndexError;
	    }
	}
	OFCondition ec = (*findResponseIdentifiers)->insert(dce, OFTrue /*replaceOld*/);
	if (ec != EC_Normal) {
	    CERR << __FUNCTION__ << ": DB_nextFindResponse: cannot insert()" << endl;
	    status->setStatus(STATUS_FIND_Failed_UnableToProcess);
	    return DcmQRLuceneIndexError;
	}
      }
      DU_putStringDOElement(*findResponseIdentifiers,
			    DCM_QueryRetrieveLevel, ldata->queryLevelString.c_str());
			    
    (*findResponseIdentifiers)->print( CERR );
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

  
bool DcmQueryRetrieveLuceneIndexHandle::tagSupported (DcmTagKey tag)
{
  return DcmQRLuceneTagKeyMap.find( tag ) != DcmQRLuceneTagKeyMap.end();
}

OFCondition DcmQueryRetrieveLuceneIndexHandle::startFindRequest(const char* SOPClassUID, DcmDataset* findRequestIdentifiers, DcmQueryRetrieveDatabaseStatus* status)
{
  Lucene_QUERY_CLASS rootLevel;
  if (strcmp( SOPClassUID, UID_FINDPatientRootQueryRetrieveInformationModel) == 0)
      rootLevel = PATIENT_ROOT ;
  else if (strcmp( SOPClassUID, UID_FINDStudyRootQueryRetrieveInformationModel) == 0)
      rootLevel = STUDY_ROOT ;
#ifndef NO_PATIENTSTUDYONLY_SUPPORT
  else if (strcmp( SOPClassUID, UID_FINDPatientStudyOnlyQueryRetrieveInformationModel) == 0)
      rootLevel = PATIENT_STUDY ;
#endif
  else {
      dbdebug(1, "startFindRequest: STATUS_FIND_Refused_SOPClassNotSupported") ;
      status->setStatus(STATUS_FIND_Refused_SOPClassNotSupported);
      return (DcmQRLuceneIndexError) ;
  }

  /**** Parse Identifiers in the Dicom Object
  **** Find Query Level and construct a list
  **** of query identifiers
  ***/
  bool qrLevelFound = false;
  LuceneElementMapType &findRequestMap = ldata->findRequestMap;
  findRequestMap.clear();
  Lucene_LEVEL queryLevel = PATIENT_LEVEL;
  int elemCount = (int)(findRequestIdentifiers->card());
  for (int elemIndex=0; elemIndex<elemCount; elemIndex++) {
    LuceneSmallDcmElmt elem;
    DcmElement* dcelem = findRequestIdentifiers->getElement(elemIndex);
    elem.XTag = dcelem->getTag().getXTag();
    { 
      char *s = NULL;
      dcelem->getString(s);
      if (s) elem.Value = s; 
    }

    /** If element is the Query Level, store it in handle
      */
    if (elem.XTag == DCM_QueryRetrieveLevel) {
	// Skip this line if you want strict comparison
	std::transform(elem.Value.begin(), elem.Value.end(), elem.Value.begin(), (int(*)(int))toupper);
	if (PatientLevelString == elem.Value) {
	    queryLevel = PATIENT_LEVEL ;
	    ldata->queryLevelString = PatientLevelString;
	}
	else if (StudyLevelString == elem.Value) {
	    queryLevel = STUDY_LEVEL ;
	    ldata->queryLevelString = StudyLevelString;
	}
	else if (SerieLevelString == elem.Value) {
	    queryLevel = SERIE_LEVEL ;
	    ldata->queryLevelString = SerieLevelString;
	}
	else if (ImageLevelString == elem.Value) {
	    queryLevel = IMAGE_LEVEL ;
	    ldata->queryLevelString = ImageLevelString;
	}
	else {
	    dbdebug(1, "DB_startFindRequest () : Illegal query level (%s)\n") ;
	    status->setStatus(STATUS_FIND_Failed_UnableToProcess);
	    return (DcmQRLuceneIndexError) ;
	}
	qrLevelFound = true;
    } else if (tagSupported (elem.XTag)) {
      findRequestMap[elem.XTag] = elem;
    }
  }

  if (!qrLevelFound) {
      /* The Query/Retrieve Level is missing */
      status->setStatus(STATUS_FIND_Failed_IdentifierDoesNotMatchSOPClass);
      dbdebug(1,"startFindRequest(): missing Query/Retrieve Level");
//        handle->idxCounter = -1 ;
//        DB_FreeElementList (handle->findRequestList) ;
//        handle->findRequestList = NULL ;
      return (DcmQRLuceneIndexError) ;
  }
  Lucene_LEVEL qLevel = PATIENT_LEVEL;
  Lucene_LEVEL lLevel = IMAGE_LEVEL; 
  switch (rootLevel)
  {
    case PATIENT_ROOT :
      qLevel = PATIENT_LEVEL ;
      lLevel = IMAGE_LEVEL ;
      break ;
    case STUDY_ROOT :
      qLevel = STUDY_LEVEL ;
      lLevel = IMAGE_LEVEL ;
      break ;
    case PATIENT_STUDY:
      qLevel = PATIENT_LEVEL ;
      lLevel = STUDY_LEVEL ;
      break ;
  }

  dbdebug(2,"startFindRequest(): rootLevel=%d queryLevel=%d qLevel=%d lLevel=%d", rootLevel, queryLevel, qLevel, lLevel);

    /**** Test the consistency of the request list
    ***/

    if (doCheckFindIdentifier) {
      try {
        testFindRequestList( findRequestMap, queryLevel, qLevel, lLevel) ;
      } catch ( std::runtime_error &e ) {
#ifdef DEBUG
            dbdebug(1, "DB_startFindRequest () : STATUS_FIND_Failed_IdentifierDoesNotMatchSOPClass - Invalid RequestList\n") ;
            dbdebug(1, e.what() ) ;
#endif
            status->setStatus(STATUS_FIND_Failed_IdentifierDoesNotMatchSOPClass);
            return (DcmQRLuceneIndexError) ;
      }
    }
    
    BooleanQuery boolQuery;
    Query *findQuery = &boolQuery;
    int countingLevel = queryLevel;
    while(countingLevel < qLevel) {
      const Lucene_Entry &UIDTag = LevelToUIDTag.find( (Lucene_LEVEL)countingLevel )->second;
      TermQuery *tq = new TermQuery( new Term( UIDTag.tagStr.c_str(), LuceneString( findRequestMap[ UIDTag.tag ].Value ).c_str() ));
      boolQuery.add( tq, BooleanClause::MUST );
      countingLevel++;
    }
    if (boolQuery.getClauseCount() == 0) { // No Restrictions - search full Index - could be optimized
      findQuery = new WildcardQuery( new Term( LuceneString(DCM_SOPInstanceUID).c_str(), LuceneString("*").c_str()  ));
    }
// TODO: Match other tags!
  dbdebug(2, "startFindRequest searching index: %s", LuceneString((const TCHAR*)findQuery->toString(NULL)).toStdString().c_str());

  ldata->findResponseHitCounter = 0;
  ldata->findResponseHits = ldata->indexsearcher->search(findQuery);
  if (boolQuery.getClauseCount() == 0) { // delete WildcardQuery
    delete findQuery;
  }

  dbdebug(1, "startFindRequest found %i items", ldata->findResponseHits->length());

  if (ldata->findResponseHits->length() == 0) {
    cancelFindRequest(status);
    dbdebug(1, "DB_startFindRequest () : STATUS_Success\n") ;
    status->setStatus(STATUS_Success);
    return (EC_Normal) ;
  }
  status->setStatus(STATUS_Pending);
  return (EC_Normal) ;
}

/************
**      Test a Find Request List
**      Returns EC_Normal if ok, else returns DcmQRLuceneIndexError
 */

void testFindRequestList (
                const LuceneElementMapType &findRequestMap,
                Lucene_LEVEL        queryLevel,
                Lucene_LEVEL        infLevel,
                Lucene_LEVEL        lowestLevel
                )
{
    LuceneElementMapType plist ;

    /**** Query level must be at least the infLevel
    ***/

    if ( (queryLevel < infLevel) || (queryLevel > lowestLevel) ){
	throw std::runtime_error( "Level incompatible with Information Model (level " + toString( queryLevel ) + ")" ) ;
    }

    for (int level = PATIENT_LEVEL ; level <= IMAGE_LEVEL ; ++level) {

        /**** Manage exception due to StudyRoot Information Model :
        **** In this information model, queries may include Patient attributes
        **** but only if they are made at the study level
        ***/

        if ((level == PATIENT_LEVEL) && (infLevel == STUDY_LEVEL)) {
            /** In Study Root Information Model, accept only Patient Tags
            ** if the Query Level is the Study level
            */

            bool atLeastOneKeyFound = false ;
	    for (LuceneElementMapType::const_iterator i = findRequestMap.begin(); i != findRequestMap.end() ; i++) {
		if (LuceneSmallDcmElmtToLevel(i->second) != level) continue ;
                atLeastOneKeyFound = true ;
            }
            if (atLeastOneKeyFound && (queryLevel != STUDY_LEVEL))
	      throw std::runtime_error( "Key found in Study Root Information Model (level " + toString( level ) + ")" ) ;
        }

        /**** If current level is above the QueryLevel
        ***/
        else if (level < queryLevel) {

            /** For this level, only unique keys are allowed
            ** Parse the request list elements reffering to
            ** this level.
            ** Check that only unique key attr are provided
            */

            bool uniqueKeyFound = false;
	    for (LuceneElementMapType::const_iterator i = findRequestMap.begin(); i != findRequestMap.end() ; i++) {
		if (LuceneSmallDcmElmtToLevel(i->second) != level) continue ;
		if (LuceneSmallDcmElmtToKeyType(i->second) != Lucene_Entry::UNIQUE_KEY)
		  throw std::runtime_error( "Non Unique Key found (level " + toString( level ) + ")" ) ;
		else if (uniqueKeyFound)
		  throw std::runtime_error( "More than one Unique Key found (level " + toString( level ) + ")" ) ;
		else uniqueKeyFound = true;
            }
        }

        /**** If current level is the QueryLevel
        ***/

        else if (level == queryLevel) {

            /** For this level, all keys are allowed
            ** Parse the request list elements reffering to
            ** this level.
            ** Check that at least one key is provided
            */

            bool atLeastOneKeyFound = false;
	    for (LuceneElementMapType::const_iterator i = findRequestMap.begin(); i != findRequestMap.end() ; i++) {
		if (LuceneSmallDcmElmtToLevel(i->second) != level) continue ;
                atLeastOneKeyFound = true;
            }
            if (! atLeastOneKeyFound)
	      throw std::runtime_error( "No Key found at query level (level " + toString( level ) + ")" ) ;
        }

        /**** If current level beyond the QueryLevel
        ***/

        else if (level > queryLevel) {

            /** For this level, no key is allowed
            ** Parse the request list elements reffering to
            ** this level.
            ** Check that no key is provided
            */

	    for (LuceneElementMapType::const_iterator i = findRequestMap.begin(); i != findRequestMap.end() ; i++) {
		if (LuceneSmallDcmElmtToLevel(i->second) != level) continue ;
		throw std::runtime_error( "No Key found beyond query level (level " + toString( level ) + ")" ) ;
            }
        }

    }
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
    ldata->imageDoc->add( *new Field( FieldNameDocumentDicomLevel.c_str(), LevelStringMap.find( level )->second.c_str(), Field::STORE_YES| Field::INDEX_UNTOKENIZED| Field::TERMVECTOR_NO ) );
    for(TagValueMapType::const_iterator i=dataset.begin(); i != dataset.end(); i++) {
      const Lucene_Entry &tag = DcmQRLuceneTagKeyMap.find( i->first )->second;
      if (tag.level <= level) {
	int tokenizeFlag =  (tag.fieldType == Lucene_Entry::TEXT_TYPE) ? Field::INDEX_TOKENIZED : Field::INDEX_UNTOKENIZED;
	ldata->imageDoc->add( *new Field( DcmQRLuceneTagKeyMap.find( i->first )->second.tagStr.c_str(), i->second.c_str() , Field::STORE_YES| tokenizeFlag | Field::TERMVECTOR_NO ) );
      }
    }
    ldata->indexwriter->addDocument(ldata->imageDoc);
    return false;
  }
}


OFCondition DcmQueryRetrieveLuceneIndexHandle::storeRequest(const char* SOPClassUID, const char* SOPInstanceUID, const char* imageFileName, DcmQueryRetrieveDatabaseStatus* status, OFBool isNew)
{
    dbdebug(1,"DB_storeRequest () : storage request of file : %s",imageFileName);
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
	  CERR << "storeRequest():\"" << imageFileName << "\" - no DCM_SOPInstanceUID, rejecting" << endl;
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
    ldata->imageDoc->add( *new Field( FieldNameDocumentDicomLevel.c_str(), LevelStringMap.find( IMAGE_LEVEL )->second.c_str(), Field::STORE_YES| Field::INDEX_UNTOKENIZED| Field::TERMVECTOR_NO ) );
    for(TagValueMapType::const_iterator i=dataMap.begin(); i != dataMap.end(); i++) {
      const Lucene_Entry &tag = DcmQRLuceneTagKeyMap.find( i->first )->second;
      int tokenizeFlag =  (tag.fieldType == Lucene_Entry::TEXT_TYPE) ? Field::INDEX_TOKENIZED : Field::INDEX_UNTOKENIZED;
      ldata->imageDoc->add( *new Field( DcmQRLuceneTagKeyMap.find( i->first )->second.tagStr.c_str(), i->second.c_str() , Field::STORE_YES| tokenizeFlag | Field::TERMVECTOR_NO ) );
    }
    ldata->imageDoc->add( *new Field( FieldNameObjectStatus.c_str(), ((isNew) ? ObjectStatusIsNew : ObjectStatusIsNotNew).c_str(), Field::STORE_YES| Field::INDEX_UNTOKENIZED| Field::TERMVECTOR_NO ) );
    ldata->imageDoc->add( *new Field( FieldNameDicomFileName.c_str(), LuceneString(imageFileName).c_str(), Field::STORE_YES| Field::INDEX_UNTOKENIZED| Field::TERMVECTOR_NO ) );

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


  
