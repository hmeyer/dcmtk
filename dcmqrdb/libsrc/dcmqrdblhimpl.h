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
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>

#include <map>
#include <list>


#include "dcmtk/dcmqrdb/dcmqrdbl-taglist.h"
#include "dcmtk/dcmqrdb/dcmqrdbl.h"


using namespace lucene::index;
using namespace lucene::analysis;
using namespace lucene::document;
using namespace lucene::search;
using boost::scoped_ptr;
namespace fs =boost::filesystem;



typedef std::list< DcmTagKey > TagListType;
typedef std::map< DcmTagKey, LuceneString > TagValueMapType;
typedef std::map< DcmTagKey, std::string > TagStdValueMapType;
typedef std::multimap< DcmTagKey, std::string > TagMultiStdValueMapType;


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



class DcmQRDBLHImpl { // TODO: implement Singleton based IndexWriter and IndexSearcher
  public:
  scoped_ptr<Analyzer> analyzer;
  scoped_ptr<IndexWriter> indexwriter;
  scoped_ptr<IndexSearcher> indexsearcher;
  scoped_ptr<Document> imageDoc;
  scoped_ptr<Hits> findResponseHits;
  unsigned int findResponseHitCounter;
  TagListType findRequestList;
  std::string queryLevelString;
  scoped_ptr<Hits> moveResponseHits;
  unsigned int moveResponseHitCounter;
  const OFString storageArea;
  const DcmQRLuceneIndexType indexType;

  DcmQRDBLHImpl(const OFString &s, DcmQRLuceneIndexType i, OFCondition &r);
  ~DcmQRDBLHImpl();
  bool checkAndStoreDataForLevel( Lucene_LEVEL level, TagValueMapType &dataset);
  OFString getIndexPath(void);
  void refreshForSearch(void);
  private:
  OFCondition recreateSearcher(void);
};



#endif // DCMQRDBLHIMPL_H
