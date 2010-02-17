#ifndef LOWERCASEANALYZER_H
#define LOWERCASEANALYZER_H

#include <CLucene.h>



class LowerCaseAnalyzer: public lucene::analysis::Analyzer {
public:  
  LowerCaseAnalyzer();
  virtual ~LowerCaseAnalyzer();
  lucene::analysis::TokenStream* tokenStream(const TCHAR* fieldName, CL_NS(util)::Reader* reader);
  lucene::analysis::TokenStream* reusableTokenStream(const TCHAR* fieldName, CL_NS(util)::Reader* reader);
};


#endif