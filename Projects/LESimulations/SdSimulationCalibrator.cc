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

#include <sstream>

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

    const auto chargePeakFilename = "dump_charge_peak-" + basename + ".dat";
    fDumpFile.open(chargePeakFilename);
    if (!fDumpFile.is_open()) {
      ERROR("Cannot open charge/peak dump file!");
      return eFailure;
    }
    fDumpFile << "# pmtId phase peak charge compatPeak\n";

    if (dumpTraces) {
      const auto traceFilename = "dump_trace-" + basename + ".dat";
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

    ProcessStation(station, dStation);

    return eSuccess;
  }


  VModule::ResultFlag
  SdSimulationCalibrator::Finish()
  {
    TabularStream tab(" r r r | . . . | . . .");
    tab << "PMT" << endc << "N" << endc << "NPar" << endc << "<Peak>" << endc
        << "Err<>" << endc << "StdDev" << endc << "<Charge>" << endc << "Err<>"
        << endc << "StdDev" << endr << hline;
    int pmtId = 0;
    unsigned int numRuns = 0;
    for (const auto& peakCharge : fPeakCharge) {
      const auto& peak = peakCharge.first;
      if (peak.GetN()) {
        const auto& charge = peakCharge.second;
        tab << pmtId << endc
            << peak.GetN() << endc
            << fNParticles << endc
            << peak.GetAverage() << endc
            << peak.GetAverageError() << endc
            << peak.GetStandardDeviation() << endc
            << charge.GetAverage() << endc
            << charge.GetAverageError() << endc
            << charge.GetStandardDeviation() << endr;
        numRuns = peak.GetN();
      }
      ++pmtId;
    }
    tab << delr;

    // Pull some info from the ParticleInjector for documentation purposes.
    // NB this expects one to use ParticleInjectorNEU!
    auto injB = CentralConfig::GetInstance()->GetTopBranch("ParticleInjector");
    const double muEnergy = injB.GetChild("Energy").GetChild("Discrete").GetChild("x").Get<double>();

    ostringstream info;
    info << "The simulated calibration constants were set as follows:\n\n"
         << tab << "\n\n"
            "The following XML snipped should be pasted into "
            "Framework/SDetector/SdSimCalibrationConstants.xml.in "
            "with the other PMT calibrations from the same simModule:\n\n"
            "  <!-- " << fSignature << " tank sim, " << numRuns << " runs, " << fNParticles << " particle(s) per run, "
            "energy = " << muEnergy / GeV << " GeV -->\n"
            "  <simModule name='" << fSignature << "'>\n"
            "    <electronics isUUB='" << fIsUUB << "'>\n";

    for (int i = 1, n = fPeakCharge.size(); i < n; ++i) {
      const bool isWCD = (1 <= i && i <= 3);
      const auto& peakCharge = isWCD ? fWCDPeakCharge : fPeakCharge[i];
      const auto& peak = peakCharge.first;
      if (!peak.GetN())
        continue;
      info << "      <PMT id='" << i << "'>" << (isWCD ? "<!-- note: all WCD PMTs are averaged together -->" : "") << '\n';
      const auto& charge = peakCharge.second;
      info << "        <peak> " << peak.GetAverage() << " </peak> "
              "<!-- err=" << peak.GetAverageError() << " std=" << peak.GetStandardDeviation();
      if (isWCD)
        info << " pmt" << i << '=' << fPeakCharge[i].first.GetAverage();
      info << " -->\n"
              "        <charge> " << charge.GetAverage() << " </charge> "
              "<!-- err=" << charge.GetAverageError() << " std=" << charge.GetStandardDeviation();
      if (isWCD)
        info << " pmt" << i << '=' << fPeakCharge[i].second.GetAverage();
      info << " -->\n";
      if (fIsUUB && isWCD) {
        const auto& compatPeak = fWCDCompatibilityPeak;
        const auto& peakRatio = fWCDPeakRatio;
        info << "        <compatibilityPeak> " << compatPeak.GetAverage() << " </compatibilityPeak> "
                "<!-- err=" << compatPeak.GetAverageError() << " std=" << compatPeak.GetStandardDeviation()
             << " pmt" << i << '=' << fCompatibilityPeak[i].GetAverage() << " -->\n"
                "        <!-- FBW/compatibility peak ratio=" << peakRatio.GetAverage() << " err=" << peakRatio.GetAverageError() << " "
                "std=" << peakRatio.GetStandardDeviation() << " pmt" << i << '=' << fPeakRatio[i].GetAverage() << " -->\n";
      }
      info << "      </PMT>\n";
    }
    info << "    </electronics>\n"
            "  </simModule>\n";

    INFO(info);

    Clear();

    return eSuccess;
  }


  pair<int, int>
  FindSignal(const TimeDistributionI& trace, const double baseline, const double threshold)
  {
    const double trigger = baseline + threshold;
    const int a = trace.GetStart();
    const int b = trace.GetStop() + 1;
    int begin;
    for (begin = a; begin < b; ++begin)
      if (trace.At(begin) >= trigger)
        break;
    // run to the end?
    if (begin == b)
      return make_pair(0, 0);  // not found
    int end;
    for (end = begin + 1; end < b; ++end)
      if (trace.At(end) < trigger)
        break;
    return make_pair(begin, end);
  }


  pair<double, double>
  ProcessTrace(const TimeDistributionI& trace, const int begin, const int end, const double baseline,
               const int nParticles)
  {
    const double val = trace.At(begin) - baseline;
    double peak = val;
    double charge = val;
    for (int i = begin+1; i < end; ++i) {
      const double val = trace.At(i) - baseline;
      charge += val;
      if (val > peak)
        peak = val;
    }

    return make_pair(peak, charge / nParticles);
  }


  inline
  void
  DumpTrace(std::ofstream* const file,
            const TimeDistributionI& trace, const int begin, const int end, const double baseline,
            const int nParticles)
  {
    if (!file)
      return;
    const int start = trace.GetStart();
    // output begin and end relative to the start of the trace dump
    *file << baseline << ' ' << begin-start << ' ' << end-start << ' ' << nParticles;
    for (int i = start, n = trace.GetStop()+1; i < n; ++i)
      *file << ' ' << trace.At(i);
    *file << '\n';
  }


  void
  SdSimulationCalibrator::ProcessStation(const sevt::Station& station, const sdet::Station& dStation)
  {
    fNParticles = station.GetSimData().GetNParticles();

    if (fNParticles <= 0)
      return;

    if (fNParticles != 1 && fHardwareType != sdet::PMTConstants::eWaterCherenkovSmall) {
      ERROR("Simulation calibrator is throwing more than one particle. Only use with smallPMT!");
      exit(1);
    }

    // this tries to mimic the muon-buffer trigger
    const double signalThreshold = 30; // adc for both
    const int beforeSignal = fIsUUB ? 20 : 1;
    const int afterSignal = fIsUUB ? 49 : 19;
    const int uubTimeFactor = 3;

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

      const bool isWCD = (pmt.GetType() == sdet::PMTConstants::eWaterCherenkovLarge);

      const double baseline = pmtCalib.GetBaseline(sdet::PMTConstants::eHighGain);

      const auto& trace = pmtSim.GetFADCTrace();

      const auto beginEnd = FindSignal(trace, baseline, signalThreshold);
      const auto begin = beginEnd.first;
      const auto end = beginEnd.second;
      if (!begin && !end)
        continue;  // no signal found

      const int signalBegin = max(begin - beforeSignal, trace.GetStart());
      const int signalEnd = min(begin + afterSignal, trace.GetStop()+1);

      DumpTrace(fDumpTraceFile, trace, signalBegin, signalEnd, baseline, fNParticles);

      const auto peakCharge = ProcessTrace(trace, signalBegin, signalEnd, baseline, fNParticles);
      const auto& peak = peakCharge.first;
      const auto& charge = peakCharge.second;
      ResizeArrays(index + 1);
      fPeakCharge[index].first(peak);
      fPeakCharge[index].second(charge);
      if (isWCD) {
        fWCDPeakCharge.first(peak);
        fWCDPeakCharge.second(charge);
      }

      if (!fIsUUB || !isWCD) {
        fDumpFile << index << " -1 " << peak << ' ' << charge << " -1\n";
      } else {
        // this is only for UUB and WCD
        // increase statistics by downsampling with all 3 possible phases
        for (int phase = 0; phase < 3; ++phase) {
          const auto compatTrace = sdet::UUBDownsampleFilter(trace, phase);
          const auto compatPeakCharge =
            ProcessTrace(compatTrace, signalBegin/uubTimeFactor, signalEnd/uubTimeFactor, baseline, fNParticles);
          const auto& compatPeak = compatPeakCharge.first;
          fCompatibilityPeak[index](compatPeak);
          fPeakRatio[index](peak/compatPeak);
          fWCDCompatibilityPeak(compatPeak);
          fWCDPeakRatio(peak/compatPeak);
          fDumpFile << index << ' ' << phase << ' ' << peak << ' ' << charge << ' ' << compatPeak << '\n';
        }
      }

    }
  }


  void
  SdSimulationCalibrator::ResizeArrays(const unsigned int n)
  {
    if (n > fPeakCharge.size()) {
      fPeakCharge.resize(n);
      fCompatibilityPeak.resize(n);
      fPeakRatio.resize(n);
    }
  }


  void
  SdSimulationCalibrator::Clear()
  {
    fPeakCharge.clear();
    fCompatibilityPeak.clear();
    fPeakRatio.clear();
    fWCDPeakCharge.first.Clear();
    fWCDPeakCharge.second.Clear();
    fWCDCompatibilityPeak.Clear();
    fWCDPeakRatio.Clear();

    fDumpFile.close();
  }

}
