#include "DummyCentralTrigger.h"
#include <utl/ErrorLogger.h>
#include <utl/TabularStream.h>
#include <utl/TimeStamp.h>

#include <evt/ShowerSimData.h>
#include <evt/Event.h>
#include <sevt/SEvent.h>
#include <sevt/T3.h>

#include <det/Detector.h>
#include <sdet/SDetector.h>
#include <sdet/Station.h>

using namespace sdet;
using namespace std;
using namespace sevt;
using namespace evt;
using namespace fwk;
using namespace utl;


namespace DummyCentralTriggerNS {

  VModule::ResultFlag
  DummyCentralTrigger::Init()
  {
    INFO("DummyCentralTrigger::Init()");
    fDefaultOffset = TimeInterval(0);
    fDefaultWindow = TimeInterval(30*microsecond);
    return eSuccess;
  }


  VModule::ResultFlag
  DummyCentralTrigger::Run(evt::Event& event)
  {
    INFO("DummyCentralTrigger::Run()");

    if (!event.HasSEvent())
      return eSuccess;
    auto& sEvent = event.GetSEvent();

    if (!event.HasSimShower())
      return eContinueLoop;
    const auto& simShower = event.GetSimShower();

    T3 simT3;
    const auto& trigTime = simShower.GetTimeStamp();
    simT3.SetTime(trigTime);
    simT3.SetAlgorithm("Dummy");

    ostringstream info;
    info << "Dummy T3 trigger, "
            "reference time " << trigTime << " "
            "(" << trigTime.GetGPSSecond() << " s, " << int(trigTime.GetGPSNanoSecond()/1e3) << " us)" << '\n';
    INFO(info);
    TabularStream tab("r|r|l");
    tab <<              endc << "time"                       << endr
        << "station" << endc << "offset" << endc << "energy" << endr
        << hline;
    const auto& sDetector = det::Detector::GetInstance().GetSDetector();
    for (const auto& s : sDetector.StationsRange()) {
      tab << s.GetId() << ' ' << endc
          << ' ' << fDefaultOffset << ' ' << endc
          << " !w" << fDefaultWindow << endr;
      simT3.AddStation(s.GetId(), fDefaultOffset, fDefaultWindow);
    }

    tab << delr;
    DEBUGLOG(tab);

    if (!sEvent.HasSimData())
      sEvent.MakeSimData();
    sEvent.GetSimData().AddT3(simT3);

    return eSuccess;
  }


  VModule::ResultFlag
  DummyCentralTrigger::Finish()
  {
    INFO("DummyCentralTrigger::Finish()");
    return eSuccess;
  }

}
