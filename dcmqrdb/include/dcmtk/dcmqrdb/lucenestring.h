#ifndef LUCENESTRING_H
#define LUCENESTRING_H

#include "dcmtk/dcmdata/dctagkey.h"
#include <CLucene/clucene-config.h>
#include <string>
#include <sstream>


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
  LuceneString toLower(void) const;
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






#endif // LUCENEHELPER_H
