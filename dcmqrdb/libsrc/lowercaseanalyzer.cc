#include "dcmtk/dcmqrdb/lowercaseanalyzer.h"

using namespace lucene::analysis;
using namespace lucene::util;

LowerCaseAnalyzer::LowerCaseAnalyzer(){}
LowerCaseAnalyzer::~LowerCaseAnalyzer(){}
TokenStream* LowerCaseAnalyzer::tokenStream(const TCHAR* fieldName, CL_NS(util)::Reader* reader){
    return new LowerCaseTokenizer(reader);
}
TokenStream* LowerCaseAnalyzer::reusableTokenStream(const TCHAR* fieldName, CL_NS(util)::Reader* reader)
{
	Tokenizer* tokenizer = static_cast<Tokenizer*>(getPreviousTokenStream());
	if (tokenizer == NULL) {
		tokenizer = new LowerCaseTokenizer(reader);
		setPreviousTokenStream(tokenizer);
	} else
		tokenizer->reset(reader);
	return tokenizer;
}