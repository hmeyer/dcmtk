#include "dcmtk/dcmqrdb/lucenestring.h"
#include <algorithm>


LuceneString::LuceneString() {}
LuceneString::LuceneString( const TCHAR* t):BaseType( t ) {}
LuceneString::LuceneString( DcmTagKey t ) {
  const std::string tstring( t.toString().c_str() );
  this->assign( tstring.begin(), tstring.end() );
}
std::string LuceneString::toStdString(void) const {
  std::string out;
  out.assign( this->begin(), this->end() );
  return out;
}

LuceneString LuceneString::toLower(void) const {
  LuceneString out;
  std::transform( this->begin(),
		  this->end(), 
		  std::back_inserter(out), 
		  (int(*)(int))tolower);
  return out;
}
