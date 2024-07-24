#ifndef _DummyCentralTriggerNS_DummyCentralTrigger_h_
#define _DummyCentralTriggerNS_DummyCentralTrigger_h_

#include <fwk/VModule.h>
#include <utl/TimeInterval.h>


namespace evt {
  class Event;
}

namespace DummyCentralTriggerNS {

  /**
    \class DummyCentralTrigger

    \author Javier Gonzalez
    \date 08 Dec 2011
  */

  class DummyCentralTrigger : public fwk::VModule {

  public:
    virtual ~DummyCentralTrigger() = default;

    fwk::VModule::ResultFlag Init() override;
    fwk::VModule::ResultFlag Run(evt::Event& event) override;
    fwk::VModule::ResultFlag Finish() override;

  private:
    utl::TimeInterval fDefaultOffset;
    utl::TimeInterval fDefaultWindow;

    REGISTER_MODULE("DummyCentralTriggerKG", DummyCentralTrigger);

  };

}


#endif
