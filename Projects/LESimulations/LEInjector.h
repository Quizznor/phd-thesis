#ifndef _LEInjectorOG_LEInjector_h_
#define _LEInjectorOG_LEInjector_h_

#include <string>
#include <vector>
#include <iostream>

#include <fwk/VModule.h>

#include <utl/Particle.h>
#include <utl/TimeStamp.h>

namespace evt {
  class Event;
}

namespace sevt {
  class Station;
}

namespace utl {
  class RandomEngine;
}

namespace LEInjectorOG {

  /**
    \class LEInjector LEInjector.h LEInjectorNEU/LEInjector.h

    \author Tom Paul
    \date 9 May 2015

    Simple-minded injector for low energy particles simulated by Corsika.
    This module is really only meant for quick-and-dirty testing, and should
    be replaced by a proper FileReader which handles these low energy Corsika files.

    The format of the 'Corsika-lite' files is as follows:
    \verbatim
    $1 Corsika Particle ID
    $2 p_x
    $3 p_y
    $4 p_z
    $5 x
    $6 y
    $7 time_at_ground
    $8 Primary Shower Id
    $9 Primary particle
    $10 Primary Energy
    $11 Primary Theta (deg)
    $12 Primary Phi (deg)
    $13 1st_interaction_point
    \endverbatim
    \ingroup SDSimModules
  */


  class LEInjector : public fwk::VModule {
  public:
    LEInjector();
    virtual ~LEInjector() { }

    fwk::VModule::ResultFlag Init();
    fwk::VModule::ResultFlag Run(evt::Event& event);
    fwk::VModule::ResultFlag Finish();

  private:

    unsigned fStationId;
    std::string fFileName;
    std::ifstream* fInputStream;
    bool fVerticalMuonMode;

    utl::RandomEngine::RandomEngineType* fRandomEngine;

    REGISTER_MODULE("LEInjectorCustom", LEInjector);

  };

}


#endif // _LEInjectorOG_LEInjector_h_

// Configure (x)emacs for this file ...
// Local Variables:
// mode: c++
// End:
