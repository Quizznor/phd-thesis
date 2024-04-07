#include "SdSimulationCalibrator.h"

#include <det/Detector.h>

#include <sdet/SDetector.h>
#include <sdet/Station.h>
#include <sdet/PMTConstants.h>
#include <sdet/UUBDownsampleFilter.h>

#include <evt/Event.h>

#include <fwk/CentralConfig.h>

#include <sevt/SEvent.h>
#include <sevt/Station.h>
#include <sevt/StationSimData.h>
#include <sevt/StationCalibData.h>
#include <sevt/PMTCalibData.h>
#include <sevt/PMTSimData.h>
#include <sevt/PMT.h>

#include <utl/AugerUnits.h>
#include <utl/MathConstants.h>
#include <utl/Particle.h>
#include <utl/PhysicalConstants.h>
#include <utl/TimeStamp.h>
#include <utl/ErrorLogger.h>
#include <utl/Point.h>
#include <utl/Vector.h>
#include <utl/Reader.h>
#include <utl/TimeDistribution.h>
#include <utl/TimeDistributionAlgorithm.h>
#include <utl/TabularStream.h>

#include <fwk/RunController.h>
#include <fwk/RandomEngineRegistry.h>

#include <sstream>

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

using namespace utl;
using namespace sevt;
using namespace fwk;
using namespace evt;
using namespace det;
using namespace std;

namespace SdSimulationCalibratorOG {

  VModule::ResultFlag
  SdSimulationCalibrator::Init()
  {

    srand (time(NULL));

    auto topBranch = CentralConfig::GetInstance()->GetTopBranch("SdSimulationCalibrator");

    topBranch.GetChild("singleTankId").GetData(fSingleTankId);

    const auto hardwareType = topBranch.GetChild("hardware").Get<string>();

    auto dumpTraces = topBranch.GetChild("dumpTraces").Get<bool>();

    if (hardwareType == "wcdLarge")
      fHardwareType = sdet::PMTConstants::eWaterCherenkovLarge;
    else if (hardwareType == "wcdSmall")
      fHardwareType = sdet::PMTConstants::eWaterCherenkovSmall;
    else if (hardwareType == "ssd")
      fHardwareType = sdet::PMTConstants::eScintillator;
    else if (hardwareType == "all")
      fHardwareType = sdet::PMTConstants::eAnyType;
    else {
      ERROR("Hardware type not recognized or supported!");
      return eFailure;
    }

    const bool isUUB = det::Detector::GetInstance().GetSDetector().GetStation(fSingleTankId).IsUUB();
    auto g4SimBranch = CentralConfig::GetInstance()->GetTopBranch("G4StationSimulator");
    const auto fullTrackMode = g4SimBranch.GetChild("fullTrackMode").Get<string>();
    const auto fastMode = g4SimBranch.GetChild("fastMode").Get<string>();

    const auto basename =
      std::string(isUUB ? "uub" : "ub") +
      "-hw_" + hardwareType +
      "-full_" + fullTrackMode +
      "-fast_" + fastMode;

    // const auto chargePeakFilename = "dump_charge_peak-" + basename + ".dat";
    // fDumpFile.open(chargePeakFilename);
    // if (!fDumpFile.is_open()) {
    //   ERROR("Cannot open charge/peak dump file!");
    //   return eFailure;
    // }
    // fDumpFile << "# pmtId phase peak charge compatPeak\n";

    if (dumpTraces) {
      const auto traceFilename = "/cr/tempdata01/filip/LESimulations/run5/dump_trace-" + basename + ".dat";
      fDumpTraceFile = new std::ofstream(traceFilename);
      if (!fDumpTraceFile->is_open()) {
        ERROR("Cannot open trace dump file!");
        return eFailure;
      }
      *fDumpTraceFile << "# baseline begin end num_particles <whole_trace>\n";
    }

    return eSuccess;
  }


  VModule::ResultFlag
  SdSimulationCalibrator::Run(Event& event)
  {
    if (!event.HasSEvent()) {
      ERROR("Non-existent SEvent.");
      return eFailure;
    }

    auto& sEvent = event.GetSEvent();

    if (!sEvent.HasStation(fSingleTankId)) {
      ostringstream err;
      err << "Non-existent station " << fSingleTankId;
      ERROR(err);
      return eFailure;
    }

    auto& station = sEvent.GetStation(fSingleTankId);
    const auto& dStation = Detector::GetInstance().GetSDetector().GetStation(fSingleTankId);

    fSignature = station.GetSimData().GetSimulatorSignature();
    fIsUUB = dStation.IsUUB();

    const int particleId = rand();
    ProcessStation(station, dStation, particleId);

    return eSuccess;
  }


  VModule::ResultFlag
  SdSimulationCalibrator::Finish()
  {
    return eSuccess;
  }


  inline
  void
  DumpTrace(std::ofstream* const file,
            const TimeDistributionI& trace, 
            const double baseline,
            const int pmtId,
            const int pId)
  {
    if (!file)
      return;
    const int start = trace.GetStart();
    *file << pId << ',' << pmtId << ',' << baseline;
    for (int i = start + 600; i < start + 1000; ++i)
      *file << ',' << trace.At(i);
    *file << '\n';
  }


  void
  SdSimulationCalibrator::ProcessStation(const sevt::Station& station, const sdet::Station& dStation, const int pId)
  {
    fNParticles = station.GetSimData().GetNParticles();

    if (fNParticles <= 0)
      return;

    if (fNParticles != 1 && fHardwareType != sdet::PMTConstants::eWaterCherenkovSmall) {
      ERROR("Simulation calibrator is throwing more than one particle. Only use with smallPMT!");
      exit(1);
    }

    for (const auto& pmt : station.PMTsRange(sdet::PMTConstants::eAnyType)) {

      if (!pmt.HasSimData())
        continue;
      const PMTSimData& pmtSim = pmt.GetSimData();

      if (!pmtSim.HasFADCTrace())
        continue;

      const sdet::PMT& dPMT = dStation.GetPMT(pmt);
      // only process desired pmt type, unless all
      if (!(fHardwareType == sdet::PMTConstants::eAnyType || dPMT.GetType() == fHardwareType))
        continue;

      if (!pmt.HasCalibData())
        continue;
      const auto& pmtCalib = pmt.GetCalibData();

      const auto index = pmt.GetId();
      if (index == 4)
        continue;

      const double baseline = pmtCalib.GetBaseline(sdet::PMTConstants::eHighGain);

      const auto& trace = pmtSim.GetFADCTrace();

      DumpTrace(fDumpTraceFile, trace, baseline, index, pId);
    }
  }

  void
  SdSimulationCalibrator::Clear()
  {
  }

}
