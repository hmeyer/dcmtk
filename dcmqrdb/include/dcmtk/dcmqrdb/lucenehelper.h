#ifndef LUCENEHELPER_H
#define LUCENEHELPER_H

#include "dcmtk/dcmdata/dctagkey.h"
#include "dcmtk/ofstd/ofcond.h"
#include <CLucene/clucene-config.h>
#include <string>
#include <sstream>


//typedef std::basic_string< TCHAR > LuceneString;

class LuceneString:public std::basic_string< TCHAR > {
  public:
  typedef std::basic_string< TCHAR > BaseType;
  LuceneString();
  LuceneString( const TCHAR* );
  LuceneString( DcmTagKey t );
  template< class SomeStringType >
  LuceneString( SomeStringType in ) {
    const std::string istring(in);
    this->assign( istring.begin(), istring.end() );
  }
  std::string toStdString(void) const;
//  private:
//    std::string stdversion;
};

/** this class provides a primitive interface for handling a flat DICOM element,
 *  similar to DcmElement, but only for use within the lucene module
 */
struct LuceneSmallDcmElmt
{
    /// string of the value field
    std::string Value;
    /// attribute tag
    DcmTagKey XTag ;
};

template <typename T>
std::string toString(const T & value)
{ 
  std::stringstream ss;
  ss << value;
  return ss.str();
}


/*
template< class SomeStringType >
void str2LuceneStr( SomeStringType in, LuceneString &out ) {
  const std::string istring(in);
  out.assign( istring.begin(), istring.end() );
}
template< class SomeStringType >
LuceneString str2LuceneStr( const SomeStringType &i ) {
  LuceneString lstring;
  str2LuceneStr( i, lstring );
  return lstring;
}

std::string LuceneStr2str( LuceneString in );
*/
/*
DcmTagKey LuceneStr2Tag( const LuceneString &in );
std::string LuceneTagStr2DictStr( const LuceneString &in );
DcmTagKey LuceneStr2Tag( const LuceneString &in, OFCondition &cond );
*/




#endif // LUCENEHELPER_H
