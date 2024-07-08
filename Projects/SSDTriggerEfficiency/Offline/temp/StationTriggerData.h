#ifndef _sevt_StationTriggerData_h_
#define _sevt_StationTriggerData_h_

#include <utl/ShadowPtr_fwd.h>
#include <string>
#include <cstdint>

namespace sevt {

  /*!
    \class StationTriggerData StationTriggerData.h "sevt/StationTriggerData.h"
    \brief %Station Trigger Data description
    \author Stefano Argiro, Darko Veberic
    \date 27 january 2003
    \ingroup sevt
  */

  class StationTriggerData {

  public:
    enum Algorithm {
      // force explicit mapping for string array index, see StationTrigger.cc
      eNone                           = 0,
      eT1Threshold                    = 1,
      eT2Threshold                    = 2,
      eTimeOverThreshold              = 3,
      eTimeOverThresholdDeconvoluted  = 4,
      eMultiplicityOfPositiveSteps    = 5,
      eRandom                         = 6,
      eMuon                           = 7,  // muons are here just for backward compatibility
      eSilent                         = 8,
      eRDThreshold                    = 9
    };

    enum PLDTrigger {
      ePLDNone           = 0x00000000,
      // Latch triggers
      ePLDLatchThreshold = 0x00000001,  // T1 and T2 threshold
      ePLDLatchTOTA      = 0x00000002,  // TOT
      ePLDLatchTOTB      = 0x00000004,  // TOTd
      ePLDLatchTOTC      = 0x00000008,  // MOPS
      ePLDLatchRandom    = 0x00000010,  // Random trigger
      ePLDLatchRD        = 0x00100000,  // RD trigger
      // Post-latch triggers
      ePLDThreshold      = 0x00000100,  // T1 and T2 threshold
      ePLDTOTA           = 0x00000200,  // TOT
      ePLDTOTB           = 0x00000400,  // TOTd
      ePLDTOTC           = 0x00000800,  // MOPS
      ePLDRandom         = 0x00001000,  // Random trigger
      ePLDRD             = 0x10000000,  // RD trigger
      // (DV) Muon: this is not a PLD bit. It is set only explicitly via
      // SetAlgorithm(). This feature is maintained on behalf of some
      // modules (TankSimulator).
      ePLDMuon           = 0x80000000
    };

    enum ErrorCode {
      eNoError = 0,
      eT3Lost,
      eT3NotFound,
      eT3TooYoung,
      eT3Already,
      eT1Stopped,
      eBadCompress,
      eDataLost
    };

    int GetOffsetMicroSecond() const { return fOffsetMicroSecond; }
    int GetWindowMicroSecond() const { return fWindowMicroSecond; }
    int GetErrorCode() const { return fErrorCode; }

    typedef std::uint32_t PLDType;
    PLDType GetPLDTrigger() const { return fPLDTrigger; }

    /// T1 TOT is always promoted to T2 TOT
    bool IsTimeOverThreshold() const
    { return fPLDTrigger & ePLDTotalTOT; }

    /// Time Over Threshold deconvoluted
    bool IsTimeOverThresholdDeconvoluted() const
    { return fPLDTrigger & ePLDTotalTOTD; }

    // Multiplicity of positive steps
    bool IsMultiplicityOfPositiveSteps() const
    { return fPLDTrigger & ePLDTotalMOPS; }

    /// Any of the PLD triggers (threshold or TOT)
    bool IsThresholdOrTimeOverThreshold() const
    { return fPLDTrigger & ePLDTotalThresholdOrTOT; }

    bool IsRDThreshold() const
    { return fPLDTrigger & ePLDTotalRD; }

    /// T1 events have non-zero window size
    bool
    IsT1()
      const
    {
      return fWindowMicroSecond && (
        IsThresholdOrTimeOverThreshold() ||
        IsTimeOverThresholdDeconvoluted() ||
        IsMultiplicityOfPositiveSteps() ||
        IsRDThreshold()
      );
    }

    /// T2 events have zero window size
    bool
    IsT2()
      const
    {
      return IsT2Threshold() ||
        IsTimeOverThreshold() ||
        IsTimeOverThresholdDeconvoluted() ||
        IsMultiplicityOfPositiveSteps() ||
        IsRDThreshold();
    }

    /// the only flag CDAS T3 central trigger cares about
    bool
    IsT2TimeOverThresholdLike()
      const
    {
      // no T2Threshold
      return IsTimeOverThreshold() ||
        IsTimeOverThresholdDeconvoluted() ||
        IsMultiplicityOfPositiveSteps() ||
        IsRDThreshold();
    }

    /// T1 threshold
    bool IsT1Threshold() const
    { return IsThresholdBit() && fWindowMicroSecond; }

    /// T2 threshold
    bool IsT2Threshold() const
    { return IsThresholdBit() && !fWindowMicroSecond; }

    bool IsRandom() const { return fPLDTrigger & ePLDTotalRandom; }

    bool IsMuon() const
    { return fWindowMicroSecond && (fPLDTrigger & ePLDMuon); }

    bool IsSilent() const
    { return fWindowMicroSecond && fErrorCode == eT3NotFound; }

    Algorithm GetAlgorithm() const;
    const std::string& GetAlgorithmName() const { return fgAlgorithmLookup[GetAlgorithm()]; }

    int GetPLDTimeOffset() const { return fPLDTimeOffset; }
    std::string GetPLDVersion() const { return fPLDVersion; }

    void SetOffsetMicroSecond(const int offset) { fOffsetMicroSecond = offset; }
    void SetWindowMicroSecond(const int window) { fWindowMicroSecond = window; }
    void SetErrorCode(const int errorCode) { fErrorCode = errorCode; }
    void SetPLDTrigger(const PLDType trigger) { fPLDTrigger = trigger; }
    void SetAlgorithm(const Algorithm algo);
    void SetPLDTimeOffset(const int offset) { fPLDTimeOffset = offset; }
    void SetPLDVersion(const std::string version) { fPLDVersion = version; }

  private:
    StationTriggerData() = default;

    bool IsThresholdBit() const { return fPLDTrigger & ePLDTotalThreshold; }

    enum PLDTotalTrigger {
      // combined (see SD Calibration report by X. Bertou and PLD trigger guide)
      ePLDTotalThreshold      = (ePLDLatchThreshold | ePLDThreshold),
      ePLDTotalTOT            = (ePLDLatchTOTA | ePLDTOTA),  // according to IoSdData
      ePLDTotalTOTD           = (ePLDLatchTOTB | ePLDTOTB),
      ePLDTotalMOPS           = (ePLDLatchTOTC | ePLDTOTC),
      ePLDTotalThresholdOrTOT = (ePLDTotalThreshold | ePLDTotalTOT),
      ePLDTotalRandom         = (ePLDLatchRandom | ePLDRandom),
      ePLDTotalRD             = (ePLDLatchRD | ePLDRD)
    };

    int fOffsetMicroSecond = -1;
    int fWindowMicroSecond = -1;
    int fErrorCode = -1;
    PLDType fPLDTrigger = ePLDNone;
    int fPLDTimeOffset = 0;
    std::string fPLDVersion = "Unknown";

    static const std::string fgAlgorithmLookup[];

    friend class Station;
    friend class StationSimData;
    friend class utl::ShadowPtr<StationTriggerData>;

  };

}


#endif
