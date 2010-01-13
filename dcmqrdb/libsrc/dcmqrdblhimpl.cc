/*
    Copyright (c) <year>, <copyright holder>
    All rights reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions are met:
        * Redistributions of source code must retain the above copyright
        notice, this list of conditions and the following disclaimer.
        * Redistributions in binary form must reproduce the above copyright
        notice, this list of conditions and the following disclaimer in the
        documentation and/or other materials provided with the distribution.
        * Neither the name of the <organization> nor the
        names of its contributors may be used to endorse or promote products
        derived from this software without specific prior written permission.

    THIS SOFTWARE IS PROVIDED BY <copyright holder> ''AS IS'' AND ANY
    EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
    WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
    DISCLAIMED. IN NO EVENT SHALL <copyright holder> BE LIABLE FOR ANY
    DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
    (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
    LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
    ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
    (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
    SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*/

#include "dcmtk/ofstd/ofstdinc.h"
#include "dcmtk/dcmdata/dcfilefo.h"
#include "dcmtk/dcmdata/dcdict.h"
#include "dcmtk/dcmnet/diutil.h"
#include "dcmtk/dcmqrdb/dcmqrdbs.h"


#include "dcmtk/dcmqrdb/dcmqrcnf.h"

#include "dcmqrdblhimpl.h"

#include <iostream>

bool DcmQRDBLHImpl::indexExists( const OFString &s ) {
  return IndexReader::indexExists( storageAreaToIndexPath( s ).c_str() );
}

DcmQRDBLHImpl::DcmQRDBLHImpl(const OFString &storageArea,
  DcmQRLuceneIndexType indexType,
  OFCondition& result):storageArea(storageArea), analyzer( new LowerCaseAnalyzer()),imageDoc( new Document),indexType( indexType ) 
  {
  if (indexType == DcmQRLuceneWriter) {
    bool indexExists = false;
    if ( IndexReader::indexExists( getIndexPath().c_str() ) ) {
	//Force unlock index incase it was left locked
	if ( IndexReader::isLocked(getIndexPath().c_str()) )
	    IndexReader::unlock(getIndexPath().c_str());
	indexExists = true;
    }
    try {
      indexwriter.reset( new IndexWriter( getIndexPath().c_str(),
					    analyzer.get(), !indexExists) );
    } catch(CLuceneError &e) {
      CERR << "Exception while creation of IndexWriter caught:" << e.what() << endl;
      result = DcmQRLuceneIndexError;
    }
  }
  result = recreateSearcher();
}

void DcmQRDBLHImpl::refreshForSearch(void) {
  if (indexType == DcmQRLuceneWriter)
    indexwriter->flush();
  recreateSearcher();
}


OFCondition DcmQRDBLHImpl::recreateSearcher(void) {
  if (indexType == DcmQRLuceneWriter) {
    try {
      indexsearcher.reset( new IndexSearcher( indexwriter->getDirectory() ) );
    } catch(CLuceneError &e) {
      CERR << "Exception while creation of IndexSearcher caught:" << e.what() << endl;
      return DcmQRLuceneIndexError;
    }
  } else if (indexType == DcmQRLuceneReader) {
    try {
      indexsearcher.reset( new IndexSearcher( getIndexPath().c_str()) );
    } catch(CLuceneError &e) {
      CERR << "Exception while creation of IndexSearcher caught:" << e.what() << endl;
      return DcmQRLuceneIndexError;
    }
  }
  return EC_Normal;
}


DcmQRDBLHImpl::~DcmQRDBLHImpl() {
  if (indexType == DcmQRLuceneWriter) {
    imageDoc->clear();
  }
  indexsearcher->close();
  if (indexType == DcmQRLuceneWriter) {
    indexwriter->optimize();
    indexwriter->close();
  }
}

const std::string DcmQRDBLHImpl::storageAreaToIndexPath(const OFString &storageArea) {
  fs::path storagePath( storageArea.c_str() );
  fs::path indexPath = storagePath / LUCENEPATH;
  return indexPath.string();
}


OFString DcmQRDBLHImpl::getIndexPath(void) {
  fs::path indexPath( storageAreaToIndexPath( storageArea ) );
  if (!fs::exists( indexPath ))
    fs::create_directory( indexPath );
  else if (!fs::is_directory( indexPath )) throw new std::runtime_error("Index Path " + indexPath.string() + " is not a directory");
  return indexPath.string().c_str();
}

bool DcmQRDBLHImpl::checkAndStoreDataForLevel( Lucene_LEVEL level, TagValueMapType &dataset ) { // returns true if Object already existed in level
  LevelTagMapType::const_iterator uidTagIter = LevelToUIDTag.find( level );
  if (uidTagIter == LevelToUIDTag.end() ) std::runtime_error(std::string(__FUNCTION__) + ": level " + toString(level) +" not found!");
  const Lucene_Entry &UIDTagEntry = uidTagIter->second;
  TagValueMapType::const_iterator uidDataIter = dataset.find( UIDTagEntry.tag );
  if (uidDataIter == dataset.end() ) std::runtime_error(std::string(__FUNCTION__) + ": tag " + UIDTagEntry.tagStr.toStdString() + " not found!");

  BooleanQuery lookupQuery;
  lookupQuery.add( new TermQuery( new Term( FieldNameDocumentDicomLevel.c_str(), QRLevelStringMap.find( level )->second.c_str() ) ), BooleanClause::MUST );
  lookupQuery.add( new TermQuery( new Term( UIDTagEntry.tagStr.c_str(), uidDataIter->second.c_str() ) ), BooleanClause::MUST );

// TODO: remove this dumb thing ---- snip -----  
refreshForSearch();
// TODO: remove this dumb thing ---- snap -----
  scoped_ptr<Hits> hits( indexsearcher->search(&lookupQuery) );
  if (hits->length()>0) {
    return true;
  } else {
    imageDoc->clear();
    imageDoc->add( *new Field( FieldNameDocumentDicomLevel.c_str(), QRLevelStringMap.find( level )->second.c_str(), Field::STORE_YES| Field::INDEX_UNTOKENIZED| Field::TERMVECTOR_NO ) );
    for(TagValueMapType::const_iterator i=dataset.begin(); i != dataset.end(); i++) {
      if (i->second.length() > 0) {
	const Lucene_Entry &tag = DcmQRLuceneTagKeyMap.find( i->first )->second;
	if (tag.level <= level) {
	  int tokenizeFlag =  (tag.fieldType == Lucene_Entry::NAME_TYPE || tag.fieldType == Lucene_Entry::TEXT_TYPE) ? Field::INDEX_TOKENIZED : Field::INDEX_UNTOKENIZED;
	  imageDoc->add( *new Field( DcmQRLuceneTagKeyMap.find( i->first )->second.tagStr.c_str(), i->second.c_str() , Field::STORE_YES| tokenizeFlag | Field::TERMVECTOR_NO ) );
	}
      }
    }
    indexwriter->addDocument(imageDoc.get());
    return false;
  }
}
