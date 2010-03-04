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

#ifndef DCMQRDBLHIMPL_H
#define DCMQRDBLHIMPL_H

#include <CLucene.h>

#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/date_time/posix_time/posix_time_types.hpp>

#include <map>
#include <list>
#include <set>
#include <string>


#include "dcmtk/dcmqrdb/lucenestring.h"
#include "dcmtk/dcmqrdb/luceneenums.h"



using namespace lucene::index;
using namespace lucene::analysis;
using namespace lucene::document;
using namespace lucene::search;
using namespace std;
using boost::scoped_ptr;
using boost::shared_ptr;
namespace fs =boost::filesystem;

#include "lowercaseanalyzer.h"


typedef std::list< DcmTagKey > TagListType;
typedef std::map< DcmTagKey, LuceneString > TagValueMapType;
typedef std::map< LuceneString, LuceneString > StringValueMapType;
typedef std::map< DcmTagKey, std::string > TagStdValueMapType;
typedef std::multimap< DcmTagKey, std::string > TagMultiStdValueMapType;

struct DicomUID {
  Lucene_LEVEL level;
  LuceneString uid;
  DicomUID( Lucene_LEVEL l, const LuceneString &id ): level( l), uid(id) {}
  DicomUID() {}
  bool operator<(const DicomUID &other) const;
};

typedef set< DicomUID > UIDSetType;

class DcmQRDBLHImpl { // TODO: implement Singleton based IndexWriter and IndexSearcher
  protected:
  static const std::string storageAreaToIndexPath(const string &storageArea);
  shared_ptr<Analyzer> analyzer;
  shared_ptr<IndexWriter> indexwriter;
  shared_ptr<IndexSearcher> indexsearcher;
  shared_ptr<boost::posix_time::ptime> first_modified;
  UIDSetType newUIDSet; // set of UIDs modified since last searcher flush
  string getIndexPath(void);
  void flushIndex(bool force=false);
  public:
  enum Result {
    good,
    error
  };
  const string storageArea;
  
  IndexReader& getIndexReader();
  void addDocument( Lucene_LEVEL level, const TagValueMapType &tagDataset, const StringValueMapType &stringDataset=StringValueMapType() );
  bool sopInstanceExists( const LuceneString &sopInstanceUID, string &existingFileName );
  void findQuery(Query* query, int upToDateMillis, const DicomUID &uid);
  void moveQuery(Query* query, int upToDateMillis, const DicomUID &uid);

  scoped_ptr<Document> imageDoc;
  scoped_ptr<Hits> findResponseHits;
  scoped_ptr<BooleanQuery> findRequest;
  unsigned int findResponseHitCounter;
  TagListType findRequestList;
  std::string queryLevelString;
  scoped_ptr<Hits> moveResponseHits;
  scoped_ptr<BooleanQuery> moveRequest;
  unsigned int moveResponseHitCounter;
  const DcmQRLuceneIndexType indexType;

  bool checkAndStoreDataForLevel( Lucene_LEVEL level, TagValueMapType &dataset);
  DcmQRDBLHImpl(const string &s, DcmQRLuceneIndexType i, Result &r);
  ~DcmQRDBLHImpl();
  static bool indexExists( const string &s );
};



#endif // DCMQRDBLHIMPL_H
