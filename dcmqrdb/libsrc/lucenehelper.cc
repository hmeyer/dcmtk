#include "dcmtk/dcmqrdb/lucenehelper.h"
#include <boost/lexical_cast.hpp>
#include <algorithm>
#include "dcmtk/dcmdata/dcdict.h"

/*
const OFConditionConst DcmQRBadCastC(OFM_imagectn, 0x010, OF_error, "DcmQR Bad Cast");
const OFCondition DcmQRBadCast(DcmQRBadCastC);
*/


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


/*
DcmTagKey LuceneStr2Tag( const LuceneString &in, OFCondition &cond ) {
  cond = EC_Normal;
  try {
    return LuceneStr2Tag( in );
  } catch ( ... ) {
    cond = DcmQRBadCast;
    return DcmTagKey();
  }
}

DcmTagKey LuceneStr2Tag( const LuceneString &in ) {
  if (in.length()==11) {
    if (in[0] == L'(' && in[5] == ',' && in[10] == L')') {
      DcmTagKey(Uint16 g, Uint16 e);
      return DcmTagKey( boost::lexical_cast<Uint16>( in.substr(1,4) ), boost::lexical_cast<Uint16>( in.substr(6,4) ) );
    }
  }
}

std::string LuceneTagStr2DictStr( const LuceneString &in ) {
  DcmTagKey tag;
  try {
    DcmTagKey tag = LuceneStr2Tag( in );
  } catch ( ... ) {
    return LuceneStr2str( in );
  }
  if (dcmDataDict.isDictionaryLoaded()) {
    dcmDataDict
  } else return LuceneStr2str( in );
  
  if (in.length()==11) {
    if (in[0] == L'(' && in[5] == ',' && in[10] == L')') {
      DcmTagKey(Uint16 g, Uint16 e);
      return DcmTagKey( boost::lexical_cast<Uint16>( in.substr(1,4) ), boost::lexical_cast<Uint16>( in.substr(6,4) ) );
    }
  }
}
*/