#ifndef _TankTriggerSimulatorOG_TankTriggerSimulator_h_
#define _TankTriggerSimulatorOG_TankTriggerSimulator_h_

#include <fwk/VModule.h>
#include <sevt/StationTriggerData.h>
#include <utl/TimeDistribution-fwd.h>
#include <vector>
#include <limits>


namespace sevt {
  class PMT;
  class Station;
}

namespace utl {
  class TimeStamp;
}

namespace DummyTankTriggerSimulatorOG {

  /**
    \class DummyTankTriggerSimulator
    \brief This module is a dummy instance of T2 triggers.

    Check the overview at \ref TankTriggerSimulatorOverview               // TODO!!!

    \author Troy Porter, Sylvie Dagoret-Campagne, Javier Gonzalez, Darko Veberic, Paul Filip
    \ingroup SDSimModules
  */

  class DummyTankTriggerSimulator : public fwk::VModule {

  public:
    DummyTankTriggerSimulator() = default;
    virtual ~DummyTankTriggerSimulator() = default;

    fwk::VModule::ResultFlag Init() override;
    fwk::VModule::ResultFlag Run(evt::Event& event) override;
    fwk::VModule::ResultFlag Finish() override { return eSuccess; }

  private:

    REGISTER_MODULE("DummyTankTriggerSimulatorOG", DummyTankTriggerSimulator);

  };

}


#endif
