#include "StationTriggerData.h"

namespace sevt {

  StationTriggerData::Algorithm
  StationTriggerData::GetAlgorithm()
    const
  {
    if (IsSilent())
      return eSilent;
    if (IsTimeOverThreshold())
      return eTimeOverThreshold;
    if (IsTimeOverThresholdDeconvoluted())
      return eTimeOverThresholdDeconvoluted;
    if (IsMultiplicityOfPositiveSteps())
      return eMultiplicityOfPositiveSteps;
    if (IsT2Threshold())
      return eT2Threshold;
    if (IsT1Threshold())
      return eT1Threshold;
    if (IsRandom())
      return eRandom;
    if (IsMuon())
      return eMuon;
    if (IsRDThreshold())
      return eRDThreshold;
    if (IsForced())
      return eForced;

    return eNone;
  }


  void
  StationTriggerData::SetAlgorithm(const Algorithm algo)
  {
    switch (algo) {
    default:
    case eNone:
      fWindowMicroSecond = -1;
      fPLDTrigger = ePLDNone;
      fErrorCode = -1;
      break;
    case eT1Threshold:
      fWindowMicroSecond = 30;
      fPLDTrigger = ePLDLatchThreshold;
      fErrorCode = eNoError;
      break;
    case eT2Threshold:
      fWindowMicroSecond = 0;
      fPLDTrigger = ePLDLatchThreshold;
      fErrorCode = eNoError;
      break;
    case eTimeOverThreshold:
      fWindowMicroSecond = 0;
      fPLDTrigger = ePLDLatchTOTA;
      fErrorCode = eNoError;
      break;
    case eTimeOverThresholdDeconvoluted:
      fWindowMicroSecond = 0;
      fPLDTrigger = ePLDLatchTOTB;
      fErrorCode = eNoError;
      break;
    case eMultiplicityOfPositiveSteps:
      fWindowMicroSecond = 0;
      fPLDTrigger = ePLDLatchTOTC;
      fErrorCode = eNoError;
      break;
    case eRandom:
      fWindowMicroSecond = -1;
      fPLDTrigger = ePLDLatchRandom;
      fErrorCode = eNoError;
      break;
    case eMuon:
      fWindowMicroSecond = 30;
      fPLDTrigger = ePLDMuon;
      fErrorCode = eNoError;
      break;
    case eSilent:
      fWindowMicroSecond = 30;
      fOffsetMicroSecond = 0;
      fPLDTrigger = eNone;
      fErrorCode = eT3NotFound;
      break;
    case eRDThreshold:
      fWindowMicroSecond = 30;
      fPLDTrigger = ePLDLatchRD;
      fErrorCode = eNoError;
      break;
    case eForced:
      fWindowMicroSecond = 0;
      fPLDTrigger = ePLDRandom;
      fErrorCode = eNoError;
    }
  }


  const std::string StationTriggerData::fgAlgorithmLookup[] = {
    "None",
    "T1 Threshold",
    "T2 Threshold",
    "Time Over Threshold",
    "Time Over Threshold Deconvoluted",
    "Multiplicity of Positive Steps",
    "Random",
    "Muon",
    "Silent",
    "RD Threshold",
    "Forced"
  };

}
