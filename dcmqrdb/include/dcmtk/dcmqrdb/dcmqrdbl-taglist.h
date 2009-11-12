#ifndef DCMQRDBL_TAGLIST_H
#define DCMQRDBL_TAGLIST_H

#include <vector>
#include <string>
#include "dcmtk/dcmdata/dcdeftag.h"
#include <boost/assign/list_of.hpp>
#include <CLucene/clucene-config.h>





typedef std::basic_string< TCHAR > LuceneString;
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

struct Lucene_Entry
{

  /** enumeration describing the levels of the DICOM Q/R information model
  */
  enum Lucene_LEVEL
  {
    /// DICOM Q/R patient level
    PATIENT_LEVEL,
    /// DICOM Q/R study level
    STUDY_LEVEL,
    /// DICOM Q/R series level
    SERIE_LEVEL,
    /// DICOM Q/R instance level
    IMAGE_LEVEL
  };
  /** types of query keys 
  */
  enum Lucene_KEY_TYPE 
  {
      UNIQUE_KEY,
      REQUIRED_KEY,
      OPTIONAL_KEY
  };
  /** types of database keys
  */
  enum Lucene_KEY_CLASS
  {
      DATE_CLASS,
      TIME_CLASS,
      UID_CLASS,
      STRING_CLASS,
      /// an entry not belonging to any other class
      OTHER_CLASS
  };
  enum Lucene_FIELD_TYPE
  {
      TEXT_TYPE,
      KEYWORD_TYPE,
      DATE_TYPE,
      TIME_TYPE,
      ID_TYPE,
      NUMBER_TYPE,
      UID_TYPE
  };  DcmTagKey tag ;
  LuceneString tagStr;
  Lucene_LEVEL level ;
  Lucene_KEY_TYPE keyAttr ;
  Lucene_KEY_CLASS keyClass ;
  Lucene_FIELD_TYPE fieldType ;
  Lucene_Entry(const DcmTagKey& t, Lucene_LEVEL l, Lucene_KEY_TYPE kt, Lucene_KEY_CLASS kc, Lucene_FIELD_TYPE ft) 
      : tag(t), tagStr(str2LuceneStr( (tag).toString().c_str() )), level(l), keyAttr(kt), keyClass(kc), fieldType(ft) { }
};

typedef std::vector< Lucene_Entry > DcmQRLuceneTagListType;
typedef DcmQRLuceneTagListType::const_iterator DcmQRLuceneTagListIterator;



const LuceneString FieldNameObjectStatus( str2LuceneStr("ObjectStatus") );
const LuceneString ObjectStatusIsNew( str2LuceneStr("ObjectIsNew") );
const LuceneString ObjectStatusIsNotNew( str2LuceneStr("ObjectIsNotNew") );
const LuceneString ObjectStatusContainsNewSubobjects( str2LuceneStr("ObjectContainsNewSubobjects") );
const LuceneString FieldNameInstanceDescription( str2LuceneStr("InstanceDescription") );
const LuceneString FieldNameDicomFileName( str2LuceneStr("DicomFileName") );
const LuceneString FieldNameDCM_SOPInstanceUID( str2LuceneStr( (DCM_SOPInstanceUID).toString().c_str() ));


static std::vector< Lucene_Entry > DcmQRLuceneTagList = (boost::assign::list_of(
  Lucene_Entry( DCM_PatientsName,                          Lucene_Entry::PATIENT_LEVEL,  Lucene_Entry::REQUIRED_KEY,   Lucene_Entry::STRING_CLASS, Lucene_Entry::TEXT_TYPE    ) ),
  Lucene_Entry( DCM_OtherPatientNames,                     Lucene_Entry::PATIENT_LEVEL,  Lucene_Entry::OPTIONAL_KEY,   Lucene_Entry::STRING_CLASS, Lucene_Entry::TEXT_TYPE    ),
  Lucene_Entry( DCM_PatientComments,                       Lucene_Entry::PATIENT_LEVEL,  Lucene_Entry::OPTIONAL_KEY,   Lucene_Entry::STRING_CLASS, Lucene_Entry::TEXT_TYPE    ),
  Lucene_Entry( DCM_ReferringPhysiciansName,               Lucene_Entry::STUDY_LEVEL,    Lucene_Entry::OPTIONAL_KEY,   Lucene_Entry::STRING_CLASS, Lucene_Entry::TEXT_TYPE    ),
  Lucene_Entry( DCM_StudyDescription,                      Lucene_Entry::STUDY_LEVEL,    Lucene_Entry::OPTIONAL_KEY,   Lucene_Entry::STRING_CLASS, Lucene_Entry::TEXT_TYPE    ),
  Lucene_Entry( DCM_NameOfPhysiciansReadingStudy,          Lucene_Entry::STUDY_LEVEL,    Lucene_Entry::OPTIONAL_KEY,   Lucene_Entry::STRING_CLASS, Lucene_Entry::TEXT_TYPE    ),
  Lucene_Entry( DCM_AdmittingDiagnosesDescription,         Lucene_Entry::STUDY_LEVEL,    Lucene_Entry::OPTIONAL_KEY,   Lucene_Entry::STRING_CLASS, Lucene_Entry::TEXT_TYPE    ),
  Lucene_Entry( DCM_Occupation,                            Lucene_Entry::STUDY_LEVEL,    Lucene_Entry::OPTIONAL_KEY,   Lucene_Entry::STRING_CLASS, Lucene_Entry::TEXT_TYPE    ),
  Lucene_Entry( DCM_AdditionalPatientHistory,              Lucene_Entry::STUDY_LEVEL,    Lucene_Entry::OPTIONAL_KEY,   Lucene_Entry::STRING_CLASS, Lucene_Entry::TEXT_TYPE   ),
  Lucene_Entry( DCM_PatientsBirthDate ,                    Lucene_Entry::PATIENT_LEVEL,  Lucene_Entry::OPTIONAL_KEY,   Lucene_Entry::DATE_CLASS, Lucene_Entry::DATE_TYPE      ),
  Lucene_Entry( DCM_StudyDate,                             Lucene_Entry::STUDY_LEVEL,    Lucene_Entry::REQUIRED_KEY,   Lucene_Entry::DATE_CLASS, Lucene_Entry::DATE_TYPE      ),
  Lucene_Entry( DCM_PatientsBirthTime,                     Lucene_Entry::PATIENT_LEVEL,  Lucene_Entry::OPTIONAL_KEY,   Lucene_Entry::TIME_CLASS, Lucene_Entry::TIME_TYPE      ),
  Lucene_Entry( DCM_StudyTime,                             Lucene_Entry::STUDY_LEVEL,    Lucene_Entry::REQUIRED_KEY,   Lucene_Entry::TIME_CLASS, Lucene_Entry::TIME_TYPE      ),
  Lucene_Entry( DCM_PatientID,                             Lucene_Entry::PATIENT_LEVEL,  Lucene_Entry::UNIQUE_KEY,     Lucene_Entry::STRING_CLASS, Lucene_Entry::UID_TYPE    ),
  Lucene_Entry( DCM_OtherPatientIDs,                       Lucene_Entry::PATIENT_LEVEL,  Lucene_Entry::OPTIONAL_KEY,   Lucene_Entry::STRING_CLASS, Lucene_Entry::UID_TYPE    ),
  Lucene_Entry( DCM_StudyID,                               Lucene_Entry::STUDY_LEVEL,    Lucene_Entry::REQUIRED_KEY,   Lucene_Entry::STRING_CLASS, Lucene_Entry::UID_TYPE    ),
  Lucene_Entry( DCM_StudyInstanceUID,                      Lucene_Entry::STUDY_LEVEL,    Lucene_Entry::UNIQUE_KEY,     Lucene_Entry::UID_CLASS, Lucene_Entry::UID_TYPE       ),
  Lucene_Entry( DCM_SeriesInstanceUID,                     Lucene_Entry::SERIE_LEVEL,    Lucene_Entry::UNIQUE_KEY,     Lucene_Entry::UID_CLASS, Lucene_Entry::UID_TYPE       ),
  Lucene_Entry( DCM_SOPInstanceUID,                        Lucene_Entry::IMAGE_LEVEL,    Lucene_Entry::UNIQUE_KEY,     Lucene_Entry::UID_CLASS, Lucene_Entry::UID_TYPE       ),
  Lucene_Entry( DCM_PatientsSex,                           Lucene_Entry::PATIENT_LEVEL,  Lucene_Entry::OPTIONAL_KEY,   Lucene_Entry::STRING_CLASS, Lucene_Entry::KEYWORD_TYPE    ),
  Lucene_Entry( DCM_EthnicGroup,                           Lucene_Entry::PATIENT_LEVEL,  Lucene_Entry::OPTIONAL_KEY,   Lucene_Entry::STRING_CLASS, Lucene_Entry::KEYWORD_TYPE    ),
  Lucene_Entry( DCM_PatientsSize,                          Lucene_Entry::STUDY_LEVEL,    Lucene_Entry::OPTIONAL_KEY,   Lucene_Entry::OTHER_CLASS, Lucene_Entry::KEYWORD_TYPE     ),
  Lucene_Entry( DCM_PatientsWeight,                        Lucene_Entry::STUDY_LEVEL,    Lucene_Entry::OPTIONAL_KEY,   Lucene_Entry::OTHER_CLASS, Lucene_Entry::KEYWORD_TYPE     ),
  Lucene_Entry( DCM_Modality,                              Lucene_Entry::SERIE_LEVEL,    Lucene_Entry::OPTIONAL_KEY,   Lucene_Entry::STRING_CLASS, Lucene_Entry::KEYWORD_TYPE    ),
  Lucene_Entry( DCM_PatientsAge,                           Lucene_Entry::STUDY_LEVEL,    Lucene_Entry::OPTIONAL_KEY,   Lucene_Entry::STRING_CLASS, Lucene_Entry::NUMBER_TYPE    ),
  Lucene_Entry( DCM_NumberOfPatientRelatedStudies,         Lucene_Entry::PATIENT_LEVEL,  Lucene_Entry::OPTIONAL_KEY,   Lucene_Entry::STRING_CLASS, Lucene_Entry::NUMBER_TYPE    ),
  Lucene_Entry( DCM_NumberOfPatientRelatedSeries,          Lucene_Entry::PATIENT_LEVEL,  Lucene_Entry::OPTIONAL_KEY,   Lucene_Entry::STRING_CLASS, Lucene_Entry::NUMBER_TYPE    ),
  Lucene_Entry( DCM_NumberOfPatientRelatedInstances,       Lucene_Entry::PATIENT_LEVEL,  Lucene_Entry::OPTIONAL_KEY,   Lucene_Entry::STRING_CLASS, Lucene_Entry::NUMBER_TYPE   ),
  Lucene_Entry( DCM_AccessionNumber,                       Lucene_Entry::STUDY_LEVEL,    Lucene_Entry::REQUIRED_KEY,   Lucene_Entry::STRING_CLASS, Lucene_Entry::NUMBER_TYPE    ),
  Lucene_Entry( DCM_OtherStudyNumbers,                     Lucene_Entry::STUDY_LEVEL,    Lucene_Entry::OPTIONAL_KEY,   Lucene_Entry::OTHER_CLASS, Lucene_Entry::NUMBER_TYPE     ),
  Lucene_Entry( DCM_NumberOfStudyRelatedSeries,            Lucene_Entry::STUDY_LEVEL,    Lucene_Entry::OPTIONAL_KEY,   Lucene_Entry::OTHER_CLASS, Lucene_Entry::NUMBER_TYPE     ),
  Lucene_Entry( DCM_NumberOfStudyRelatedInstances,         Lucene_Entry::STUDY_LEVEL,    Lucene_Entry::OPTIONAL_KEY,   Lucene_Entry::OTHER_CLASS, Lucene_Entry::NUMBER_TYPE     ),
  Lucene_Entry( DCM_SeriesNumber,                          Lucene_Entry::SERIE_LEVEL,    Lucene_Entry::REQUIRED_KEY,   Lucene_Entry::OTHER_CLASS, Lucene_Entry::NUMBER_TYPE     ),
  Lucene_Entry( DCM_InstanceNumber,                        Lucene_Entry::IMAGE_LEVEL,    Lucene_Entry::REQUIRED_KEY,   Lucene_Entry::OTHER_CLASS, Lucene_Entry::NUMBER_TYPE     )
);




#endif // DCMQRDBL_TAGLIST_H
