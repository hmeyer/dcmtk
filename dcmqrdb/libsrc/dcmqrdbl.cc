/*
   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include <iostream>

#define INCLUDE_CCTYPE
#define INCLUDE_CSTDARG
#include "dcmtk/ofstd/ofstdinc.h"
#include "dcmtk/dcmdata/dcfilefo.h"
#include "dcmtk/dcmnet/dimse.h"
#include "dcmtk/dcmqrdb/dcmqrdbs.h"


#include "dcmtk/dcmqrdb/dcmqrcnf.h"
#include "dcmtk/dcmqrdb/dcmqrdbl.h"
#include "dcmtk/dcmqrdb/dcmqrdbl-taglist.h"


#include "CLucene.h"
#include "boost/filesystem/operations.hpp"
#include "boost/filesystem/path.hpp"
#include "boost/format.hpp"
#include <string>
#include <list>

using namespace lucene::index;
using namespace lucene::analysis;
using namespace lucene::document;
using namespace lucene::search;
namespace fs =boost::filesystem;

struct luceneData {
  Analyzer *analyzer;
  IndexWriter *indexwriter;
  IndexSearcher *indexsearcher;
  Document *imageDoc;
};

const OFConditionConst DcmQRLuceneIndexErrorC(OFM_imagectn, 0x001, OF_error, "DcmQR Lucene Index Error");
const OFCondition DcmQRLuceneIndexError(DcmQRLuceneIndexErrorC);
const OFConditionConst DcmQRLuceneNoSOPIUIDErrorC(OFM_imagectn, 0x002, OF_error, "DcmQR Lucene no DCM_SOPInstanceUID");
const OFCondition DcmQRLuceneNoSOPIUIDError(DcmQRLuceneNoSOPIUIDErrorC);
const OFConditionConst DcmQRLuceneDoubleSOPIUIDErrorC(OFM_imagectn, 0x003, OF_error, "DcmQR Lucene double DCM_SOPInstanceUID");
const OFCondition DcmQRLuceneDoubleSOPIUIDError(DcmQRLuceneDoubleSOPIUIDErrorC);

DcmQueryRetrieveLuceneIndexHandle::DcmQueryRetrieveLuceneIndexHandle(
  const OFString &storageArea,
  DcmQRLuceneIndexType indexType,
  OFCondition& result):storageArea(storageArea),indexType(indexType) 
{
  
  ldata = new luceneData();
  ldata->imageDoc = new Document();
  ldata->analyzer = new SimpleAnalyzer();
  if (indexType == DcmQRLuceneWriter) {
    bool indexExists = false;
    if ( IndexReader::indexExists( getIndexPath().c_str() ) ) {
	//Force unlock index incase it was left locked
	if ( IndexReader::isLocked(getIndexPath().c_str()) )
	    IndexReader::unlock(getIndexPath().c_str());
	indexExists = true;
    }
    try {
      ldata->indexwriter = new IndexWriter( getIndexPath().c_str(),
					    ldata->analyzer, !indexExists);
    } catch(CLuceneError &e) {
      CERR << "Exception while creation of IndexWriter caught:" << e.what() << endl;
      result = DcmQRLuceneIndexError;
    }
    try {
      ldata->indexsearcher = new IndexSearcher( ldata->indexwriter->getDirectory());
    } catch(CLuceneError &e) {
      CERR << "Exception while creation of IndexSearcher caught:" << e.what() << endl;
      result = DcmQRLuceneIndexError;
    }
  }
}

DcmQueryRetrieveLuceneIndexHandle::~DcmQueryRetrieveLuceneIndexHandle() {
  ldata->imageDoc->clear();
  delete(ldata->imageDoc);
  ldata->indexsearcher->close();
  delete( ldata->indexsearcher );
  if (indexType == DcmQRLuceneWriter) {
    ldata->indexwriter->optimize();
    ldata->indexwriter->close();
    delete( ldata->indexwriter);
  }
  delete( ldata->analyzer );
  delete( ldata );
}


OFString DcmQueryRetrieveLuceneIndexHandle::getIndexPath(void) {
  fs::path storagePath( storageArea.c_str() );
  fs::path indexPath = storagePath / LUCENEPATH;
  if (!fs::is_directory( indexPath ))
    fs::create_directory( indexPath );
  return indexPath.string().c_str();
}

void DcmQueryRetrieveLuceneIndexHandle::setIdentifierChecking(OFBool checkFind, OFBool checkMove)
{

}

void DcmQueryRetrieveLuceneIndexHandle::setDebugLevel(int debugLevel)
{

}

OFCondition DcmQueryRetrieveLuceneIndexHandle::pruneInvalidRecords()
{

}

OFCondition DcmQueryRetrieveLuceneIndexHandle::cancelMoveRequest(DcmQueryRetrieveDatabaseStatus* status)
{

}

OFCondition DcmQueryRetrieveLuceneIndexHandle::nextMoveResponse(char* SOPClassUID, char* SOPInstanceUID, char* imageFileName, short unsigned int* numberOfRemainingSubOperations, DcmQueryRetrieveDatabaseStatus* status)
{

}

OFCondition DcmQueryRetrieveLuceneIndexHandle::startMoveRequest(const char* SOPClassUID, DcmDataset* moveRequestIdentifiers, DcmQueryRetrieveDatabaseStatus* status)
{

}

OFCondition DcmQueryRetrieveLuceneIndexHandle::cancelFindRequest(DcmQueryRetrieveDatabaseStatus* status)
{

}

OFCondition DcmQueryRetrieveLuceneIndexHandle::nextFindResponse(DcmDataset** findResponseIdentifiers, DcmQueryRetrieveDatabaseStatus* status)
{

}

OFCondition DcmQueryRetrieveLuceneIndexHandle::startFindRequest(const char* SOPClassUID, DcmDataset* findRequestIdentifiers, DcmQueryRetrieveDatabaseStatus* status)
{

}

OFCondition DcmQueryRetrieveLuceneIndexHandle::storeRequest(const char* SOPClassUID, const char* SOPInstanceUID, const char* imageFileName, DcmQueryRetrieveDatabaseStatus* status, OFBool isNew)
{
    dbdebug(1,"DB_storeRequest () : storage request of file : %s",imageFileName);
    /**** Get IdxRec values from ImageFile
    ***/

    DcmFileFormat dcmff;
    if (dcmff.loadFile(imageFileName).bad())
    {
      CERR << "Cannot open file: " << imageFileName << ": "
           << strerror(errno) << endl;
      status->setStatus(STATUS_STORE_Error_CannotUnderstand);
      return (DcmQRLuceneIndexError);
    }
    {
      if (SOPInstanceUID == NULL) {
	  CERR << "storeRequest():\"" << imageFileName << "\" - no DCM_SOPInstanceUID, rejecting" << endl;
	  return DcmQRLuceneNoSOPIUIDError;
      }
      TermQuery tq( new Term( FieldNameDCM_SOPInstanceUID.c_str(), str2LuceneStr( SOPInstanceUID ).c_str() ) );
      Hits *hits = ldata->indexsearcher->search(&tq);
      if (hits->length()>0) {
	CERR << "storeRequest():\"" << imageFileName << "\" - DCM_SOPInstanceUID already exists, rejecting" << endl;
	delete hits;
	return DcmQRLuceneDoubleSOPIUIDError;
      }
      delete hits;
    }

    DcmDataset *dset = dcmff.getDataset();

    for(DcmQRLuceneTagListIterator i = DcmQRLuceneTagList.begin(); i!=DcmQRLuceneTagList.end(); i++) {
      OFCondition ec = EC_Normal;
      const char *strPtr = NULL;
      ec = dset->findAndGetString(i->tag, strPtr);

      
      if ((ec != EC_Normal) || (strPtr == NULL)) {
	  dbdebug(1,"storeRequest (): %s: value empty or not found",i->tag.toString().c_str());
	  strPtr = "";
      }
      int tokenizeFlag =  (i->fieldType == Lucene_Entry::TEXT_TYPE) ? Field::INDEX_TOKENIZED : Field::INDEX_UNTOKENIZED;
      ldata->imageDoc->add( *new Field( i->tagStr.c_str(), str2LuceneStr( strPtr ).c_str() , Field::STORE_YES| tokenizeFlag | Field::TERMVECTOR_NO ) );
    }
    ldata->imageDoc->add( *new Field( FieldNameObjectStatus.c_str(), ((isNew) ? ObjectStatusIsNew : ObjectStatusIsNotNew).c_str(), Field::STORE_YES| Field::INDEX_UNTOKENIZED| Field::TERMVECTOR_NO ) );
    ldata->imageDoc->add( *new Field( FieldNameDicomFileName.c_str(), str2LuceneStr(imageFileName).c_str(), Field::STORE_YES| Field::INDEX_UNTOKENIZED| Field::TERMVECTOR_NO ) );

    /* InstanceDescription */
    OFBool useDescrTag = OFTrue;
    DcmTagKey descrTag = DCM_ImageComments;
    LuceneString description;
    if (SOPClassUID != NULL)
    {
        /* fill in value depending on SOP class UID (content might be improved) */
        if (strcmp(SOPClassUID, UID_GrayscaleSoftcopyPresentationStateStorage) == 0)
        {
            descrTag = DCM_ContentDescription;
        } else if (strcmp(SOPClassUID, UID_HardcopyGrayscaleImageStorage) == 0)
        {
	    description = str2LuceneStr("Hardcopy Grayscale Image");
            useDescrTag = OFFalse;
        } else if ((strcmp(SOPClassUID, UID_BasicTextSR) == 0) ||
                   (strcmp(SOPClassUID, UID_EnhancedSR) == 0) ||
                   (strcmp(SOPClassUID, UID_ComprehensiveSR) == 0))
        {
            OFString string;
            description = str2LuceneStr("unknown SR");
            const char *name = dcmFindNameOfUID(SOPClassUID);
            if (name != NULL)
                description = str2LuceneStr(name);
            if (dset->findAndGetOFString(DCM_VerificationFlag, string) == EC_Normal)
            {
                description += str2LuceneStr(", ");
                description += str2LuceneStr(string.c_str());
            }
            if (dset->findAndGetOFString(DCM_CompletionFlag, string) == EC_Normal)
            {
                description += str2LuceneStr(", ");
                description += str2LuceneStr(string.c_str());
            }
            if (dset->findAndGetOFString(DCM_CompletionFlagDescription, string) == EC_Normal)
            {
                description += str2LuceneStr(", ");
                description += str2LuceneStr(string.c_str());
            }
            useDescrTag = OFFalse;
        } else if (strcmp(SOPClassUID, UID_StoredPrintStorage) == 0)
        {
	    description = str2LuceneStr("Stored Print");
            useDescrTag = OFFalse;
        }
    }
    /* get description from attribute specified above */
    if (useDescrTag)
    {
        OFString string;
        /* return value is irrelevant */
        dset->findAndGetOFString(descrTag, string);
	description = str2LuceneStr(string.c_str());
    }
    /* is dataset digitally signed? */
        DcmStack stack;
        if (dset->search(DCM_DigitalSignaturesSequence, stack, ESM_fromHere, OFTrue /* searchIntoSub */) == EC_Normal)
        {
            /* in principle it should be checked whether there is _any_ non-empty digital signatures sequence, but ... */
            if (((DcmSequenceOfItems *)stack.top())->card() > 0)
            {
                if (description.length() > 0)
		    description += str2LuceneStr(" (Signed)");
                else
		    description += str2LuceneStr("Signed Instance");
            }
        }
    ldata->imageDoc->add( *new Field( FieldNameInstanceDescription.c_str(), description.c_str(), Field::STORE_YES| Field::INDEX_TOKENIZED| Field::TERMVECTOR_NO ) );
    ldata->indexwriter->addDocument(ldata->imageDoc);
    ldata->imageDoc->clear();
    return EC_Normal;
}

OFCondition DcmQueryRetrieveLuceneIndexHandle::makeNewStoreFileName(const char* SOPClassUID, const char* SOPInstanceUID, char* newImageFileName)
{

}

void DcmQueryRetrieveLuceneIndexHandle::dbdebug(int level, const char* format, ...) const
{
#ifdef DEBUG
    va_list ap;
    char buf[4096]; /* we hope a message never gets larger */

    if (level <= debugLevel) {
        CERR << "DB:";
        va_start(ap, format);
        vsprintf(buf, format, ap);
        va_end(ap);
        CERR << buf << endl;
    }
#endif    
}


DcmQueryRetrieveLuceneIndexReaderHandleFactory::DcmQueryRetrieveLuceneIndexReaderHandleFactory(const DcmQueryRetrieveConfig *config)
: DcmQueryRetrieveDatabaseHandleFactory()
, config_(config)
{
}

DcmQueryRetrieveLuceneIndexReaderHandleFactory::~DcmQueryRetrieveLuceneIndexReaderHandleFactory()
{
}

DcmQueryRetrieveDatabaseHandle *DcmQueryRetrieveLuceneIndexReaderHandleFactory::createDBHandle(
    const char * /* callingAETitle */,
    const char *calledAETitle,
    OFCondition& result) const
{
  return new DcmQueryRetrieveLuceneIndexHandle(
    config_->getStorageArea(calledAETitle),
    DcmQRLuceneReader,
    result);
}

DcmQueryRetrieveDatabaseHandle *DcmQueryRetrieveLuceneIndexWriterHandleFactory::createDBHandle(
    const char * /* callingAETitle */,
    const char *calledAETitle,
    OFCondition& result) const
{
  return new DcmQueryRetrieveLuceneIndexHandle(
    config_->getStorageArea(calledAETitle),
    DcmQRLuceneWriter,
    result);
}


