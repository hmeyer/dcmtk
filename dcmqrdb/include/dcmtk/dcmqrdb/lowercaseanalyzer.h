#ifndef LOWERCASEANALYZER_H
#define LOWERCASEANALYZER_H

#include <CLucene.h>

class LowerCaseWhiteSpaceTokenizer: public lucene::analysis::CharTokenizer {
public:
    LowerCaseWhiteSpaceTokenizer(lucene::util::Reader* in);
    virtual ~LowerCaseWhiteSpaceTokenizer();
protected:
    bool isTokenChar(const TCHAR c) const;
    TCHAR normalize(const TCHAR chr) const;
};

class LowerCaseWhiteSpaceAnalyzer: public lucene::analysis::Analyzer {
public:  
  LowerCaseWhiteSpaceAnalyzer();
  virtual ~LowerCaseWhiteSpaceAnalyzer();
  lucene::analysis::TokenStream* tokenStream(const TCHAR* fieldName, CL_NS(util)::Reader* reader);
  lucene::analysis::TokenStream* reusableTokenStream(const TCHAR* fieldName, CL_NS(util)::Reader* reader);
};


#endif