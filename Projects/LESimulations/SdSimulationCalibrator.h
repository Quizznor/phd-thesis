#ifndef _SdSimulationCalibratorOG_SdSimulationCalibrator_h_
#define _SdSimulationCalibratorOG_SdSimulationCalibrator_h_

#include <fwk/VModule.h>
#include <utl/Accumulator.h>
#include <utl/TimeDistribution.h>
#include <utl/Particle.h>
#include <utl/TimeStamp.h>
#include <vector>
#include <fstream>


namespace sevt {
  class Station;
}
namespace sdet {
  class Station;
}

// namespace utl {
//   class RandomEngine;
// }

namespace SdSimulationCalibratorOG {

  /**
    \class SdSimulationCalibrator

    \brief Extracts calibration constants for a tank/electronics sim combo.

    \author Tom Paul
    \author Darko Veberic
    \date April 2004
  */

  class SdSimulationCalibrator : public fwk::VModule {

  public:
    fwk::VModule::ResultFlag Init() override;
    fwk::VModule::ResultFlag Run(evt::Event& event) override;
    fwk::VModule::ResultFlag Finish() override;

  private:
    void ProcessStation(const sevt::Station& station, const sdet::Station& dStation, const int);

    void ResizeArrays(const unsigned int n);

    void Clear();

    unsigned int fSingleTankId = 0;
    bool fIsUUB = false;
    int fHardwareType = 0;
    int fNParticles = 0;

    std::string fSignature;

    typedef utl::Accumulator::SampleStandardDeviation Std;
    typedef std::pair<Std, Std> StdPair;

    // bundle all WCD PMTs together to increase the statistics
    // FBW or UB peak and charge
    StdPair fWCDPeakCharge;
    // downsampled-filtered peak
    Std fWCDCompatibilityPeak;
    // ratio of FBW and downsampled-filtered peak
    Std fWCDPeakRatio;

    // for separate PMT IDs
    std::vector<StdPair> fPeakCharge;
    std::vector<Std> fCompatibilityPeak;
    std::vector<Std> fPeakRatio;

    std::ofstream fDumpFile;
    std::ofstream* fDumpTraceFile = nullptr;

    REGISTER_MODULE("SdSimulationCalibratorOG", SdSimulationCalibrator);
  };

}


#endif
