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

#include <boost/date_time/posix_time/posix_time.hpp>
#include "dcmtk/dcmqrdb/dcmqrdbl-taglist.h"
#include "dcmtk/dcmqrdb/dcmqrdblhimpl.h"


#include <iostream>

using namespace std;


namespace pt = boost::posix_time;


bool DicomUID::operator<(const DicomUID &other) const {
  if (this->level < other.level) return true;
  if (this->level > other.level) return false;
  if (this->uid.compare(other.uid) < 0) return true;
  return false;
}


bool DcmQRDBLHImpl::indexExists( const string &s ) {
  return IndexReader::indexExists( storageAreaToIndexPath( s ).c_str() );
}

DcmQRDBLHImpl::DcmQRDBLHImpl(const string &storageArea,
  DcmQRLuceneIndexType indexType, Result& result)
  :analyzer( new LowerCaseAnalyzer()), first_modified(new pt::ptime(pt::pos_infin)), storageArea(storageArea), imageDoc( new Document),
  indexType( indexType )
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
      cerr << "Exception while creation of IndexWriter caught:" << e.what() << endl;
      result = error;
    }
  }
  flushIndex();
  result = good;
}

void DcmQRDBLHImpl::flushIndex(bool force) {
  if (force || newUIDSet.size() > 0)
    if (indexType == DcmQRLuceneWriter)
      indexwriter->flush();

  if (!indexsearcher || force || newUIDSet.size() > 0) {
    if (indexType == DcmQRLuceneWriter) {
      try {
	indexsearcher.reset( new IndexSearcher( indexwriter->getDirectory() ) );
      } catch(CLuceneError &e) {
	cerr << "Exception while creation of IndexSearcher caught:" << e.what() << endl;
      }
    } else if (indexType == DcmQRLuceneReader) {
      try {
	indexsearcher.reset( new IndexSearcher( getIndexPath().c_str()) );
      } catch(CLuceneError &e) {
	cerr << "Exception while creation of IndexSearcher caught:" << e.what() << endl;
      }
    }
  }
  newUIDSet.clear();
  *first_modified = pt::pos_infin;
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

const std::string DcmQRDBLHImpl::storageAreaToIndexPath(const string &storageArea) {
  fs::path storagePath( storageArea );
  fs::path indexPath = storagePath / LucenePath;
  return indexPath.string();
}


string DcmQRDBLHImpl::getIndexPath(void) {
  fs::path indexPath( storageAreaToIndexPath( storageArea ) );
  if (!fs::exists( indexPath ))
    fs::create_directory( indexPath );
  else if (!fs::is_directory( indexPath )) throw new std::runtime_error("Index Path " + indexPath.string() + " is not a directory");
  return indexPath.string();
}

bool DcmQRDBLHImpl::checkAndStoreDataForLevel( Lucene_LEVEL level, TagValueMapType &dataset ) { // returns true if Object already existed in level
  LevelTagMapType::const_iterator uidTagIter = LevelToUIDTag.find( level );
  if (uidTagIter == LevelToUIDTag.end() ) throw new std::runtime_error(std::string(__FUNCTION__) + ": level " + toString(level) +" not found!");
  const Lucene_Entry &UIDTagEntry = uidTagIter->second;
  TagValueMapType::const_iterator uidDataIter = dataset.find( UIDTagEntry.tag );
  if (uidDataIter == dataset.end() ) throw new std::runtime_error(std::string(__FUNCTION__) + ": tag " + UIDTagEntry.tagStr.toStdString() + " not found!");

  BooleanQuery lookupQuery;
  lookupQuery.add( new TermQuery( new Term( FieldNameDocumentDicomLevel.c_str(), QRLevelStringMap.find( level )->second.c_str() ) ), BooleanClause::MUST );
  lookupQuery.add( new TermQuery( new Term( UIDTagEntry.tagStr.c_str(), uidDataIter->second.c_str() ) ), BooleanClause::MUST );

// TODO: remove this dumb thing ---- snip -----  
flushIndex();
// TODO: remove this dumb thing ---- snap -----
  scoped_ptr<Hits> hits( indexsearcher->search(&lookupQuery) );
  if (hits->length()>0) {
    return true;
  } else {
    addDocument( level, dataset );
    return false;
  }
}

void DcmQRDBLHImpl::addDocument( Lucene_LEVEL level, const TagValueMapType &tagDataset, const StringValueMapType &stringDataset) {
  imageDoc->clear();
  imageDoc->add( *new Field( FieldNameDocumentDicomLevel.c_str(), 
    QRLevelStringMap.find( level )->second.c_str(), 
    Field::STORE_YES| Field::INDEX_UNTOKENIZED| Field::TERMVECTOR_NO ) );
  
  for( int l = level; l >= PATIENT_LEVEL; l--) {
    TagValueMapType::const_iterator UIDIterator = tagDataset.find( LevelToUIDTag.find( (Lucene_LEVEL)l )->second.tag );
    if ( UIDIterator == tagDataset.end() ) throw runtime_error("No UID defined for Document");
    newUIDSet.insert( DicomUID( (Lucene_LEVEL)l, UIDIterator->second ) );
  }

  for(StringValueMapType::const_iterator i=stringDataset.begin(); i != stringDataset.end(); i++)
    imageDoc->add( *new Field( i->first.c_str(), i->second.c_str(), Field::STORE_YES| Field::INDEX_UNTOKENIZED| Field::TERMVECTOR_NO ) );

  for(TagValueMapType::const_iterator i=tagDataset.begin(); i != tagDataset.end(); i++) {
    if (i->second.length() > 0) {
      const Lucene_Entry &tag = DcmQRLuceneTagKeyMap.find( i->first )->second;
      if (tag.level <= level) {
	int tokenizeFlag =  (tag.fieldType == Lucene_Entry::NAME_TYPE || tag.fieldType == Lucene_Entry::TEXT_TYPE) ? Field::INDEX_TOKENIZED : Field::INDEX_UNTOKENIZED;
	imageDoc->add( *new Field( tag.tagStr.c_str(), i->second.c_str() , Field::STORE_YES| tokenizeFlag | Field::TERMVECTOR_NO ) );
      }
    }
  }
  indexwriter->addDocument(imageDoc.get());
  if (*first_modified == pt::pos_infin) *first_modified = pt::microsec_clock::local_time();
}

bool DcmQRDBLHImpl::sopInstanceExists( const LuceneString &sopInstanceUID ) {
  if ( newUIDSet.find( DicomUID( IMAGE_LEVEL, sopInstanceUID ) ) != newUIDSet.end() ) return true;

  TermQuery tq( new Term( FieldNameDCM_SOPInstanceUID.c_str(), sopInstanceUID.c_str() ) );
  scoped_ptr<Hits> hits( indexsearcher->search(&tq) );
  if (hits->length()>0) return true;
  return false;
}


void DcmQRDBLHImpl::findQuery(Query* query, int upToDateMillis, const DicomUID &uid) {
  bool queriedDataFlushed = false;
  if (uid.uid.size() > 0) {
    if ( newUIDSet.find( uid ) == newUIDSet.end() ) queriedDataFlushed = true;
  }
  if (!queriedDataFlushed) {
    if ( *first_modified + pt::millisec( upToDateMillis ) < pt::microsec_clock::local_time() )
      flushIndex();
  }
  findResponseHitCounter = 0;
  findResponseHits.reset( indexsearcher->search(query) );
}

void DcmQRDBLHImpl::moveQuery(Query* query, int upToDateMillis, const DicomUID &uid) {
  bool queriedDataFlushed = false;
  if (uid.uid.size() > 0) {
    if ( newUIDSet.find( uid ) == newUIDSet.end() ) queriedDataFlushed = true;
  }
  if (!queriedDataFlushed) {
    if ( *first_modified + pt::millisec( upToDateMillis ) < pt::microsec_clock::local_time())
      flushIndex();
  }
  moveResponseHitCounter = 0;
  moveResponseHits.reset( indexsearcher->search(query) );
}



IndexReader& DcmQRDBLHImpl::getIndexReader() {
  flushIndex();
  return *indexsearcher->getReader();
}


