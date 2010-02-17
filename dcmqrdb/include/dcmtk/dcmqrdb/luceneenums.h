#ifndef LUCENEENUMS_H
#define LUCENEENUMS_H

#include <string>

static const std::string LucenePath("lucene_index");

/** enumeration describing the levels of the DICOM Q/R information model
*/
enum Lucene_LEVEL
{
  /// DICOM Q/R patient level
  PATIENT_LEVEL=11,
  /// DICOM Q/R study level
  STUDY_LEVEL,
  /// DICOM Q/R series level
  SERIE_LEVEL,
  /// DICOM Q/R instance level
  IMAGE_LEVEL
};

enum DcmQRLuceneIndexType
{
  DcmQRLuceneReader,
  DcmQRLuceneWriter,
  DcmQRLuceneModifier
};


/** query models
 */
enum Lucene_QUERY_CLASS
{
    /// patient root Q/R model
    PATIENT_ROOT=1,
    /// study root Q/R model
    STUDY_ROOT,
    /// patient/study only Q/R model
    PATIENT_STUDY
};



#endif