/**
  \file
  CDASToOfflineEventConverter.h

  \author Troy Porter
  \author Darko Veberic
  \author Ralf Ulrich
  \date 18 Jul 2003
*/

#ifndef _io_CDASToOfflineEventConverter_h_
#define _io_CDASToOfflineEventConverter_h_

#include <utl/config.h>

#include <MdEvent.h>
#include <mevt/Module.h>


namespace evt {
  class Event;
}

namespace sevt {
  class StationGPSData;
}

class IoSdEvent;
class TEcEvent;
class TEsEvent;
class TErEvent;

namespace md {
 class Event;
}

namespace evt {

  /// \ingroup cdas
  void operator>>(const evt::Event& event, IoSdEvent& rawData);
  /// \ingroup cdas
  void operator<<(evt::Event& event, const IoSdEvent& rawData);
  /// \ingroup cdas
  void ConvertEventToIoSd(const evt::Event& event, IoSdEvent& rawData);
  /// \ingroup cdas
  void ConvertIoSdToEvent(evt::Event& event, const IoSdEvent& rawData);

  /// \ingroup cdas
  void operator>>(const evt::Event& event, md::Event& rawData);
  /// \ingroup cdas
  void operator<<(evt::Event& event, const md::Event& rawData);
  /// \ingroup cdas
  void ConvertEventToIoMd(const evt::Event& event, md::Event& rawData);
  /// \ingroup cdas
  void ConvertIoMdToEvent(evt::Event& event, const md::Event& rawData);
  /// \ingroup cdas
  void FillMdTraces(const md::Module& mdModule, mevt::Module& oModule, const unsigned int numberOfSamples);

  /// \ingroup cdas
  void operator>>(const evt::Event& event, TEcEvent& rawData);
  /// \ingroup cdas
  void operator<<(evt::Event& event, const TEcEvent& rawData);
  /// \ingroup cdas
  void ConvertEventToEc(const evt::Event& event, TEcEvent& rawData);
  /// \ingroup cdas
  void ConvertEcToEvent(evt::Event& event, const TEcEvent& rawData);

  /// \ingroup cdas
  void operator>>(const evt::Event& event, TEsEvent& rawData);
  /// \ingroup cdas
  void operator<<(evt::Event& event, const TEsEvent& rawData);
  /// \ingroup cdas
  void ConvertEventToEs(const evt::Event& event, TEsEvent& rawData);
  /// \ingroup cdas
  void ConvertEsToEvent(evt::Event& event, const TEsEvent& rawData);

  /// \ingroup cdas
  void operator>>(const evt::Event& event, TErEvent& rawData);
  /// \ingroup cdas
  void operator<<(evt::Event& event, const TErEvent& rawData);
  /// \ingroup cdas
  void ConvertEventToEr(const evt::Event& event, TErEvent& rawData);
  /// \ingroup cdas
  void ConvertErToEvent(evt::Event& event, const TErEvent& rawData);

}

namespace io {

  /// \ingroup cdas
  void ApplyTimeCorrection(sevt::StationGPSData& gpsData);

}


#endif
