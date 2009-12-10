#ifndef DCMQRDBL_TAGLIST_H
#define DCMQRDBL_TAGLIST_H

#include <set>
#include <map>
#include <string>
#include "dcmtk/dcmdata/dcdeftag.h"
#include "dcmtk/dcmnet/dimse.h"
#include <boost/assign/list_of.hpp>
#include <boost/iterator/transform_iterator.hpp>

#include <CLucene/clucene-config.h>
#include "dcmtk/dcmqrdb/lucenehelper.h"


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

struct Lucene_Entry
{
  /** types of query keys 
  */
  enum KEY_TYPE 
  {
      UNIQUE_KEY=21,
      REQUIRED_KEY,
      OPTIONAL_KEY
  };
  /** types of database keys
  */
  enum KEY_CLASS
  {
      DATE_CLASS=31,
      TIME_CLASS,
      UID_CLASS,
      STRING_CLASS,
      /// an entry not belonging to any other class
      OTHER_CLASS
  };
  enum FIELD_TYPE
  {
      TEXT_TYPE=41,
      NAME_TYPE,
      KEYWORD_TYPE,
      DATE_TYPE,
      TIME_TYPE,
      ID_TYPE,
      NUMBER_TYPE,
      UID_TYPE
  };  
  DcmTagKey tag ;
  LuceneString tagStr;
  Lucene_LEVEL level ;
  KEY_TYPE keyAttr ;
  KEY_CLASS keyClass ;
  FIELD_TYPE fieldType ;
  Lucene_Entry(const DcmTagKey& t, Lucene_LEVEL l, KEY_TYPE kt, KEY_CLASS kc, FIELD_TYPE ft) 
      : tag(t), tagStr( tag ), level(l), keyAttr(kt), keyClass(kc), fieldType(ft) { }
  int operator < (const Lucene_Entry& other) const {
    return this->tag < other.tag;
  }
};

typedef std::set< Lucene_Entry > DcmQRLuceneTagListType;
typedef DcmQRLuceneTagListType::const_iterator DcmQRLuceneTagListIterator;

const LuceneString FieldNameDocumentDicomLevel( "DocumentDicomLevel" );
const LuceneString FieldNameObjectStatus( "ObjectStatus" );
const LuceneString ObjectStatusIsNew( "ObjectIsNew" );
const LuceneString ObjectStatusIsNotNew( "ObjectIsNotNew" );
const LuceneString ObjectStatusContainsNewSubobjects( "ObjectContainsNewSubobjects" );
const LuceneString FieldNameInstanceDescription( "InstanceDescription" );
const LuceneString FieldNameDicomFileName( "DicomFileName" );
const LuceneString FieldNameDCM_SOPInstanceUID( DCM_SOPInstanceUID );
const LuceneString FieldNameDCM_SOPClassUID( DCM_SOPClassUID );
const std::string PatientLevelString("PATIENT");
const std::string StudyLevelString("STUDY");
const std::string SerieLevelString("SERIES");
const std::string ImageLevelString("IMAGE");
const LuceneString ImageLevelLuceneString(ImageLevelString);
const std::map< Lucene_LEVEL, LuceneString > QRLevelStringMap = boost::assign::map_list_of( PATIENT_LEVEL, LuceneString(PatientLevelString))
  (STUDY_LEVEL, LuceneString(StudyLevelString))
  (SERIE_LEVEL, LuceneString(SerieLevelString))
  (IMAGE_LEVEL, LuceneString(ImageLevelString));
  
typedef std::map< std::string, Lucene_LEVEL >   StringQRLevelMapType;
const StringQRLevelMapType StringQRLevelMap = boost::assign::map_list_of( PatientLevelString, PATIENT_LEVEL )
  (StudyLevelString, STUDY_LEVEL)
  (SerieLevelString, SERIE_LEVEL)
  (ImageLevelString, IMAGE_LEVEL);
  
struct QueryInfo {
  enum Query_Type
  {
      FIND=51,
      MOVE,
      GET
  };
  Lucene_QUERY_CLASS qclass;
  Query_Type qtype;
  QueryInfo(Lucene_QUERY_CLASS qc, Query_Type qt) 
      : qclass(qc), qtype( qt ) { }
};

typedef std::map< std::string, QueryInfo >   StringQRClassMapType;
const StringQRClassMapType StringQRClassMap = boost::assign::map_list_of( UID_FINDPatientRootQueryRetrieveInformationModel, QueryInfo( PATIENT_ROOT, QueryInfo::FIND ) )
  (UID_FINDStudyRootQueryRetrieveInformationModel, QueryInfo( STUDY_ROOT, QueryInfo::FIND ))
#ifndef NO_PATIENTSTUDYONLY_SUPPORT
  (UID_FINDPatientStudyOnlyQueryRetrieveInformationModel, QueryInfo( PATIENT_STUDY, QueryInfo::FIND ))
#endif
  (UID_MOVEPatientRootQueryRetrieveInformationModel, QueryInfo( PATIENT_ROOT, QueryInfo::MOVE ))
  (UID_MOVEStudyRootQueryRetrieveInformationModel, QueryInfo( STUDY_ROOT, QueryInfo::MOVE ))
#ifndef NO_PATIENTSTUDYONLY_SUPPORT
  (UID_MOVEPatientStudyOnlyQueryRetrieveInformationModel, QueryInfo( PATIENT_STUDY, QueryInfo::MOVE ))
#endif
#ifndef NO_GET_SUPPORT
  (UID_GETPatientRootQueryRetrieveInformationModel, QueryInfo( PATIENT_ROOT, QueryInfo::GET ))
  (UID_GETStudyRootQueryRetrieveInformationModel, QueryInfo( STUDY_ROOT, QueryInfo::GET ))
#ifndef NO_PATIENTSTUDYONLY_SUPPORT
  (UID_GETPatientStudyOnlyQueryRetrieveInformationModel, QueryInfo( PATIENT_STUDY, QueryInfo::GET ))
#endif
#endif
  ;


static DcmQRLuceneTagListType DcmQRLuceneTagList = (boost::assign::list_of(
  Lucene_Entry( DCM_PatientsName,                          PATIENT_LEVEL,  Lucene_Entry::REQUIRED_KEY,   Lucene_Entry::STRING_CLASS, Lucene_Entry::NAME_TYPE   ) ),
  Lucene_Entry( DCM_OtherPatientNames,                     PATIENT_LEVEL,  Lucene_Entry::OPTIONAL_KEY,   Lucene_Entry::STRING_CLASS, Lucene_Entry::NAME_TYPE   ),
  Lucene_Entry( DCM_PatientComments,                       PATIENT_LEVEL,  Lucene_Entry::OPTIONAL_KEY,   Lucene_Entry::STRING_CLASS, Lucene_Entry::TEXT_TYPE   ),
  Lucene_Entry( DCM_ReferringPhysiciansName,               STUDY_LEVEL,    Lucene_Entry::OPTIONAL_KEY,   Lucene_Entry::STRING_CLASS, Lucene_Entry::NAME_TYPE   ),
  Lucene_Entry( DCM_StudyDescription,                      STUDY_LEVEL,    Lucene_Entry::OPTIONAL_KEY,   Lucene_Entry::STRING_CLASS, Lucene_Entry::TEXT_TYPE   ),
  Lucene_Entry( DCM_NameOfPhysiciansReadingStudy,          STUDY_LEVEL,    Lucene_Entry::OPTIONAL_KEY,   Lucene_Entry::STRING_CLASS, Lucene_Entry::NAME_TYPE   ),
  Lucene_Entry( DCM_AdmittingDiagnosesDescription,         STUDY_LEVEL,    Lucene_Entry::OPTIONAL_KEY,   Lucene_Entry::STRING_CLASS, Lucene_Entry::TEXT_TYPE   ),
  Lucene_Entry( DCM_Occupation,                            STUDY_LEVEL,    Lucene_Entry::OPTIONAL_KEY,   Lucene_Entry::STRING_CLASS, Lucene_Entry::TEXT_TYPE   ),
  Lucene_Entry( DCM_AdditionalPatientHistory,              STUDY_LEVEL,    Lucene_Entry::OPTIONAL_KEY,   Lucene_Entry::STRING_CLASS, Lucene_Entry::TEXT_TYPE   ),
  Lucene_Entry( DCM_PatientsBirthDate ,                    PATIENT_LEVEL,  Lucene_Entry::OPTIONAL_KEY,   Lucene_Entry::DATE_CLASS,   Lucene_Entry::DATE_TYPE   ),
  Lucene_Entry( DCM_StudyDate,                             STUDY_LEVEL,    Lucene_Entry::REQUIRED_KEY,   Lucene_Entry::DATE_CLASS,   Lucene_Entry::DATE_TYPE   ),
  Lucene_Entry( DCM_PatientsBirthTime,                     PATIENT_LEVEL,  Lucene_Entry::OPTIONAL_KEY,   Lucene_Entry::TIME_CLASS,   Lucene_Entry::TIME_TYPE   ),
  Lucene_Entry( DCM_StudyTime,                             STUDY_LEVEL,    Lucene_Entry::REQUIRED_KEY,   Lucene_Entry::TIME_CLASS,   Lucene_Entry::TIME_TYPE   ),
  Lucene_Entry( DCM_OtherPatientIDs,                       PATIENT_LEVEL,  Lucene_Entry::OPTIONAL_KEY,   Lucene_Entry::STRING_CLASS, Lucene_Entry::UID_TYPE    ),
  Lucene_Entry( DCM_StudyID,                               STUDY_LEVEL,    Lucene_Entry::REQUIRED_KEY,   Lucene_Entry::STRING_CLASS, Lucene_Entry::UID_TYPE    ),
  Lucene_Entry( DCM_PatientID,                             PATIENT_LEVEL,  Lucene_Entry::UNIQUE_KEY,     Lucene_Entry::STRING_CLASS, Lucene_Entry::UID_TYPE    ),
  Lucene_Entry( DCM_StudyInstanceUID,                      STUDY_LEVEL,    Lucene_Entry::UNIQUE_KEY,     Lucene_Entry::UID_CLASS,    Lucene_Entry::UID_TYPE    ),
  Lucene_Entry( DCM_SeriesInstanceUID,                     SERIE_LEVEL,    Lucene_Entry::UNIQUE_KEY,     Lucene_Entry::UID_CLASS,    Lucene_Entry::UID_TYPE    ),
  Lucene_Entry( DCM_SOPInstanceUID,                        IMAGE_LEVEL,    Lucene_Entry::UNIQUE_KEY,     Lucene_Entry::UID_CLASS,    Lucene_Entry::UID_TYPE    ),
  Lucene_Entry( DCM_PatientsSex,                           PATIENT_LEVEL,  Lucene_Entry::OPTIONAL_KEY,   Lucene_Entry::STRING_CLASS, Lucene_Entry::KEYWORD_TYPE),
  Lucene_Entry( DCM_EthnicGroup,                           PATIENT_LEVEL,  Lucene_Entry::OPTIONAL_KEY,   Lucene_Entry::STRING_CLASS, Lucene_Entry::KEYWORD_TYPE),
  Lucene_Entry( DCM_PatientsSize,                          STUDY_LEVEL,    Lucene_Entry::OPTIONAL_KEY,   Lucene_Entry::OTHER_CLASS,  Lucene_Entry::KEYWORD_TYPE),
  Lucene_Entry( DCM_PatientsWeight,                        STUDY_LEVEL,    Lucene_Entry::OPTIONAL_KEY,   Lucene_Entry::OTHER_CLASS,  Lucene_Entry::KEYWORD_TYPE),
  Lucene_Entry( DCM_Modality,                              SERIE_LEVEL,    Lucene_Entry::OPTIONAL_KEY,   Lucene_Entry::STRING_CLASS, Lucene_Entry::KEYWORD_TYPE),
  Lucene_Entry( DCM_PatientsAge,                           STUDY_LEVEL,    Lucene_Entry::OPTIONAL_KEY,   Lucene_Entry::STRING_CLASS, Lucene_Entry::NUMBER_TYPE ),
  Lucene_Entry( DCM_NumberOfPatientRelatedStudies,         PATIENT_LEVEL,  Lucene_Entry::OPTIONAL_KEY,   Lucene_Entry::STRING_CLASS, Lucene_Entry::NUMBER_TYPE ),
  Lucene_Entry( DCM_NumberOfPatientRelatedSeries,          PATIENT_LEVEL,  Lucene_Entry::OPTIONAL_KEY,   Lucene_Entry::STRING_CLASS, Lucene_Entry::NUMBER_TYPE ),
  Lucene_Entry( DCM_NumberOfPatientRelatedInstances,       PATIENT_LEVEL,  Lucene_Entry::OPTIONAL_KEY,   Lucene_Entry::STRING_CLASS, Lucene_Entry::NUMBER_TYPE ),
  Lucene_Entry( DCM_NumberOfStudyRelatedSeries,            STUDY_LEVEL,    Lucene_Entry::OPTIONAL_KEY,   Lucene_Entry::OTHER_CLASS,  Lucene_Entry::NUMBER_TYPE ),
  Lucene_Entry( DCM_NumberOfStudyRelatedInstances,         STUDY_LEVEL,    Lucene_Entry::OPTIONAL_KEY,   Lucene_Entry::OTHER_CLASS,  Lucene_Entry::NUMBER_TYPE ),
  Lucene_Entry( DCM_NumberOfSeriesRelatedInstances,	   SERIE_LEVEL,    Lucene_Entry::OPTIONAL_KEY,   Lucene_Entry::OTHER_CLASS,  Lucene_Entry::NUMBER_TYPE ),
  Lucene_Entry( DCM_SOPClassesInStudy,         		   STUDY_LEVEL,    Lucene_Entry::OPTIONAL_KEY,   Lucene_Entry::STRING_CLASS, Lucene_Entry::TEXT_TYPE   ),
  Lucene_Entry( DCM_ModalitiesInStudy,         		   STUDY_LEVEL,    Lucene_Entry::OPTIONAL_KEY,   Lucene_Entry::STRING_CLASS, Lucene_Entry::TEXT_TYPE   ),
  Lucene_Entry( DCM_InstitutionName,         		   STUDY_LEVEL,    Lucene_Entry::OPTIONAL_KEY,   Lucene_Entry::STRING_CLASS, Lucene_Entry::TEXT_TYPE   ),
  Lucene_Entry( DCM_InstitutionalDepartmentName,       	   STUDY_LEVEL,    Lucene_Entry::OPTIONAL_KEY,   Lucene_Entry::STRING_CLASS, Lucene_Entry::TEXT_TYPE   ),
  Lucene_Entry( DCM_AccessionNumber,                       STUDY_LEVEL,    Lucene_Entry::REQUIRED_KEY,   Lucene_Entry::STRING_CLASS, Lucene_Entry::NUMBER_TYPE ),
  Lucene_Entry( DCM_OtherStudyNumbers,                     STUDY_LEVEL,    Lucene_Entry::OPTIONAL_KEY,   Lucene_Entry::OTHER_CLASS,  Lucene_Entry::NUMBER_TYPE ),
  Lucene_Entry( DCM_NumberOfFrames,                        SERIE_LEVEL,    Lucene_Entry::OPTIONAL_KEY,   Lucene_Entry::OTHER_CLASS,  Lucene_Entry::NUMBER_TYPE ),
  Lucene_Entry( DCM_NumberOfSlices,                        SERIE_LEVEL,    Lucene_Entry::OPTIONAL_KEY,   Lucene_Entry::OTHER_CLASS,  Lucene_Entry::NUMBER_TYPE ),
  Lucene_Entry( DCM_SeriesDescription,                     SERIE_LEVEL,    Lucene_Entry::OPTIONAL_KEY,   Lucene_Entry::STRING_CLASS, Lucene_Entry::TEXT_TYPE   ),
  Lucene_Entry( DCM_SeriesNumber,                     	   SERIE_LEVEL,    Lucene_Entry::OPTIONAL_KEY,   Lucene_Entry::OTHER_CLASS,  Lucene_Entry::NUMBER_TYPE ),
						    
						    
  Lucene_Entry( DCM_InstanceNumber,                        IMAGE_LEVEL,    Lucene_Entry::REQUIRED_KEY,   Lucene_Entry::OTHER_CLASS,  Lucene_Entry::NUMBER_TYPE )
);

typedef std::map< DcmTagKey, Lucene_Entry > DcmQRLuceneTagKeyMapType;
struct Lucene_EntryToTagKeyMapAdaptor {
    typedef DcmQRLuceneTagKeyMapType::value_type result_type;
    result_type operator()(const Lucene_Entry &e) const {
      return result_type( e.tag, e );
    }
};
typedef boost::transform_iterator<Lucene_EntryToTagKeyMapAdaptor, DcmQRLuceneTagListType::const_iterator> Lucene_EntryToTagKeyMapTransformer;
const DcmQRLuceneTagKeyMapType DcmQRLuceneTagKeyMap( Lucene_EntryToTagKeyMapTransformer(DcmQRLuceneTagList.begin(), Lucene_EntryToTagKeyMapAdaptor() ),
					       Lucene_EntryToTagKeyMapTransformer( DcmQRLuceneTagList.end(), Lucene_EntryToTagKeyMapAdaptor() ) );

typedef std::map< LuceneString, Lucene_Entry > DcmQRLuceneTagKeyStrMapType;
struct Lucene_EntryToTagKeyStrMapAdaptor {
    typedef DcmQRLuceneTagKeyStrMapType::value_type result_type;
    result_type operator()(const Lucene_Entry &e) const {
      return result_type( e.tagStr, e );
    }
};
typedef boost::transform_iterator<Lucene_EntryToTagKeyStrMapAdaptor, DcmQRLuceneTagListType::const_iterator> Lucene_EntryToTagKeyStrMapTransformer;
const DcmQRLuceneTagKeyStrMapType DcmQRLuceneTagKeyStrMap( Lucene_EntryToTagKeyStrMapTransformer(DcmQRLuceneTagList.begin(), Lucene_EntryToTagKeyStrMapAdaptor() ),
					       Lucene_EntryToTagKeyStrMapTransformer( DcmQRLuceneTagList.end(), Lucene_EntryToTagKeyStrMapAdaptor() ) );


typedef std::map< Lucene_LEVEL, Lucene_Entry > LevelTagMapType;
const LevelTagMapType LevelToUIDTag = boost::assign::map_list_of(PATIENT_LEVEL, DcmQRLuceneTagKeyMap.find(DCM_PatientID)->second)
(STUDY_LEVEL, DcmQRLuceneTagKeyMap.find(DCM_StudyInstanceUID)->second)
(SERIE_LEVEL, DcmQRLuceneTagKeyMap.find(DCM_SeriesInstanceUID)->second)
(IMAGE_LEVEL, DcmQRLuceneTagKeyMap.find(DCM_SOPInstanceUID)->second);

					       
Lucene_LEVEL LuceneSmallDcmElmtToLevel(const LuceneSmallDcmElmt &e);
Lucene_Entry::KEY_TYPE LuceneSmallDcmElmtToKeyType(const LuceneSmallDcmElmt &e);


#endif // DCMQRDBL_TAGLIST_H
