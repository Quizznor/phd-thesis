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
#include <limits>

using namespace fwk;
using namespace det;
using namespace evt;
using namespace sevt;
using namespace utl;
using namespace std;


namespace DummyTankTriggerSimulator {

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

    using namespace sdet::Trigger;
    for (auto& station : sEvent.StationsRange()) {

      if (!station.HasSimData()) {
        Buffer(station, eventTime, StationTriggerData::eSilent, StationTriggerData::ePLDNone, 0, 0);
        continue;
      }

      // bool hasPhotoelectrons = false;
      for (const auto& pmt : station.PMTsRange(sdet::PMTConstants::eAnyType)) {        
        if (!pmt.HasSimData()) continue;
        const auto& simData =  pmt.GetSimData();
        
        if (!simData.HasPETimeDistribution()) continue;
        // hasPhotoelectrons = true;

        // const auto& trace = simData.GetPETimeDistribution();

        // for (const auto& bin : trace) {
        //   std::cout << bin << " ";
        // }
      }

      // const auto& trace = simData.GetPETimeDistribution();
      // int startBin = trace.GetStart() * trace.GetBinning();
      // int stopBin = trace.GetEnd() * trace.GetBinning();
      int startBin = 0;       // change this
      int stopBin = 120;      // change this

      tab << station.GetId() << endc
          << "(" << startBin << ", " << stopBin << ")" << endc
          << "(" << startBin << ", " << stopBin << ")" << endc
          << startBin << endc;
      tab << endr;

      Buffer(station, eventTime, 
        StationTriggerData::eRDThreshold, 
        StationTriggerData::ePLDRD, 
        startBin, stopBin);
    }

    DEBUGLOG(tab);

    return eSuccess;
  }
}
