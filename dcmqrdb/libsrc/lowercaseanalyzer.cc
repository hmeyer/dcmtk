#include "dcmtk/dcmqrdb/lowercaseanalyzer.h"
#include "ctype.h"

using namespace lucene::analysis;
using namespace lucene::util;


LowerCaseWhiteSpaceTokenizer::LowerCaseWhiteSpaceTokenizer(lucene::util::Reader* in):CharTokenizer(in) {
}
LowerCaseWhiteSpaceTokenizer::~LowerCaseWhiteSpaceTokenizer() {}
bool LowerCaseWhiteSpaceTokenizer::isTokenChar(const TCHAR c) const {
  if (ispunct(c) || isspace(c) || iscntrl(c)) return false;
  return true;
}
TCHAR LowerCaseWhiteSpaceTokenizer::normalize(const TCHAR chr) const {
  return tolower( chr );
}


LowerCaseWhiteSpaceAnalyzer::LowerCaseWhiteSpaceAnalyzer(){}
LowerCaseWhiteSpaceAnalyzer::~LowerCaseWhiteSpaceAnalyzer(){}
TokenStream* LowerCaseWhiteSpaceAnalyzer::tokenStream(const TCHAR* fieldName, CL_NS(util)::Reader* reader){
    return new LowerCaseWhiteSpaceTokenizer(reader);
}
TokenStream* LowerCaseWhiteSpaceAnalyzer::reusableTokenStream(const TCHAR* fieldName, CL_NS(util)::Reader* reader)
{
	Tokenizer* tokenizer = static_cast<Tokenizer*>(getPreviousTokenStream());
	if (tokenizer == NULL) {
		tokenizer = new LowerCaseWhiteSpaceTokenizer(reader);
		setPreviousTokenStream(tokenizer);
	} else
		tokenizer->reset(reader);
	return tokenizer;
}