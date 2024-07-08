/**
   \file
   declaration of CentralTriggerSimulatorXb

   \author Darko Veberic
   \author CtXb code by Xavier Bertou
   \date 4 May 2006
*/

#ifndef _CentralTriggerSimulatorXb_CentralTriggerSimulator_h_
#define _CentralTriggerSimulatorXb_CentralTriggerSimulator_h_

#include <vector>
#include <map>
#include <fwk/VModule.h>
#include <utl/TimeInterval.h>
#include "/cr/data01/filip/offline/offline/Modules/SdSimulation/CentralTriggerSimulatorXb/XbArray.h"


namespace CentralTriggerSimulatorXb {

  /**
     \class CentralTriggerSimulator CentralTriggerSimulator.h "CentralTriggerSimulatorXb/CentralTriggerSimulator.h"

     \brief Emulates the second generation of CDAS Ct. Check the overview at \ref CentralTriggerSimulatorXb

     A rumor has it that "Xb" stands for "eXtremely Beautiful".

     Notes:
     <ul>
       <li> The module treats only in-grid (or doublet) and non-dense
            stations, ie all the other stations are discarded from the trigger
            consideration.
       <li> Information about the doublets is hardcoded into the original
            CDAS files (not intended to be changed in any way). Future
            deployment of doublets will need update of the code.
     </ul>

     \author Darko Veberic
     \author Javier Gonzalez
     \date 4 May 2006, Xb v3r3
     \date 25 Apr 2018, update Xb
     \ingroup SDSimModules
  */

  class CentralTriggerSimulator : public fwk::VModule {

  public:
    fwk::VModule::ResultFlag Init();
    fwk::VModule::ResultFlag Run(evt::Event& event);
    fwk::VModule::ResultFlag Finish();

  private:
    void ArrayInit();

    utl::TimeInterval fDefaultOffset;
    utl::TimeInterval fDefaultWindow;

    std::vector<Station> fStations;

    // for vector.reserve()
    static const int fgMaxStations = 2000;

    REGISTER_MODULE("CentralTriggerSimulatorXb", CentralTriggerSimulator);

  };

}


#endif
