#include <iostream>
#include <boost/scoped_ptr.hpp>
#include <boost/filesystem.hpp>
#include "dcmtk/dcmqrdb/dcmqrdbl.h"
#include "../libsrc/dcmqrdblhimpl.h"
#include "dcmtk/dcmqrdb/dcmqrdbs.h"
#include "dcmtk/dcmnet/diutil.h"
#include "dcmtk/dcmdata/dcdeftag.h"

#include "CLucene.h"

using namespace lucene::analysis;
using namespace lucene::index;
using namespace lucene::document;
using namespace lucene::search;

using namespace std;
using namespace boost::filesystem;

const char iPath[]="lucene_index";
const TCHAR key[] = L"DocumentDicomLevel";
const TCHAR value[] = L"STUDY";

int main( int argc, char *argv[] )
{
    OFCondition dbcond = EC_Normal;
    remove_all( iPath );
    create_directory( iPath );

    KeywordAnalyzer a;
    IndexWriter w(iPath,&a,true);
    Document d;
    for(int i=0;i<101;i++) {
      d.clear();
      d.add( *new Field(key, value, Field::INDEX_UNTOKENIZED  ));
      w.addDocument( &d );
    }
    w.close();
    
	    
    scoped_ptr<IndexSearcher> indexsearcher( new IndexSearcher( iPath ));
	    
    scoped_ptr<Hits> hits;
    
    {
      BooleanQuery boolQuery;
      boolQuery.add( new TermQuery( new Term( key, value ) ) , BooleanClause::MUST );
      hits.reset( indexsearcher->search(&boolQuery) );
    }

    unsigned int c=0;
    while(c < hits->length()) {
cerr << "before" << endl;      
      hits->doc( c++ );
cerr << "after" << endl;      
    }
    return 0;
}

