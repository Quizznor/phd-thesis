#include <fwk/CentralConfig.h>
#include <det/Detector.h>
#include <sdet/SDetector.h>
#include <sdet/Station.h>
#include <evt/Event.h>
#include <sevt/PMT.h>
#include <sevt/PMTCalibData.h>
#include <sevt/SEvent.h>
#include <sevt/Header.h>
#include <sevt/Station.h>
#include <sevt/StationTriggerData.h>
#include <utl/AugerUnits.h>
#include <utl/ErrorLogger.h>
#include <utl/TimeDistribution.h>
#include <utl/MultiTrace.h>
#include <utl/PhysicalConstants.h>
#include <utl/Reader.h>
#include <utl/TimeStamp.h>
#include <utl/RandomEngine.h>
#include <utl/TabularStream.h>
#include <sdet/StationTriggerAlgorithm.h>
#include <sdet/StationTriggerUtil.h>

#include "DummyTankTriggerSimulator.h"

#include <cmath>

using namespace fwk;
using namespace det;
using namespace evt;
using namespace sevt;
using namespace utl;
using namespace std;


namespace DummyTankTriggerSimulatorOG {

  VModule::ResultFlag
  DummyTankTriggerSimulator::Init() {return eSuccess;}

  VModule::ResultFlag
  DummyTankTriggerSimulator::Run(evt::Event& event)
  {
    if (!event.HasSEvent()) {
      ERROR("Nonexisting SEvent.");
      return eFailure;
    }

    auto& sEvent = event.GetSEvent();

    const auto& eventTime = event.GetHeader().GetTime();

    TabularStream tab("r|r|r|r|l");
    tab << "ID" << endc
        << "(start, stop)" << endc
        << "bins" << endc
        << "latch" << endc
        << "" << endr;

    for (auto& station : sEvent.StationsRange()) {

      int startBin = 0;
      int stopBin = 0;
      bool hasPETimeDistribution = false;
      
      for (const auto& pmt : station.PMTsRange()) {

        if (!pmt.HasSimData()) continue;
        const auto PMTSimData = pmt.GetSimData();
        if (!PMTSimData.HasPETimeDistribution()) continue;

        // station has (at least) 1 PMT with photoelectrons at PMT level
        // the Dummy T2 algorithm therefore selects it to as candidate
        const auto PETimeDistribution = PMTSimData.GetPETimeDistribution();
        // startBin = PETimeDistribution.GetFirstPEBin();                       // TODO
        // stopBin = PETimeDistribution.GetLastPEBin();                         // TODO
        hasPETimeDistribution = true;
        break;
      }

      using namespace sdet::Trigger;
      if (!hasPETimeDistribution)
        Buffer(station, eventTime, StationTriggerData::eSilent, StationTriggerData::ePLDNone, 0, 0);
      else {
        tab << station.GetId() << endc
            << "(" << startBin << ", " << stopBin << ")" << endc
            << "(" << startBin << ", " << stopBin << ")" << endc
            << startBin << endc;
        tab << endr;

        Buffer(station, eventTime, 
          StationTriggerData::eForced, 
          StationTriggerData::ePLDForced, 
          startBin, stopBin);
        }
      }

    DEBUGLOG(tab);

    return eSuccess;
  }
}
