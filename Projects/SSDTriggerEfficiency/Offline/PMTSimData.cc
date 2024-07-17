#include <sevt/PMTSimData.h>
#include <sdet/Station.h>
#include <utl/ErrorLogger.h>
#include <utl/AugerUnits.h>

using namespace utl;
using namespace std;
using namespace sevt;


void
PMTSimData::MakePETimeDistribution(const StationConstants::SignalComponent source)
{
  fPETimeDistribution.AddTimeDistribution(1*ns, source);
}


void
PMTSimData::MakeBaseSignal(const StationConstants::SignalComponent source)
{
  fBaseSignal.AddTimeDistribution(1*ns, source);
}


void
PMTSimData::ClearBaseSignals()
{
  fBaseSignal.Clear();
}


void
PMTSimData::MakeFilterSignal(const StationConstants::SignalComponent source)
{
  fFilterSignal.AddTimeDistribution(1*ns, source);
}


void
PMTSimData::MakeFADCTrace(const StationConstants::SignalComponent source)
{
  fFADCTrace[sdet::PMTConstants::eHighGain].AddTimeDistribution(fFADCBinSize, source);
  fFADCTrace[sdet::PMTConstants::eLowGain].AddTimeDistribution(fFADCBinSize, source);
  fFADCTraceD[sdet::PMTConstants::eHighGain].AddTimeDistribution(fFADCBinSize, source);
  fFADCTraceD[sdet::PMTConstants::eLowGain].AddTimeDistribution(fFADCBinSize, source);
}


bool
PMTSimData::HasFADCTrace(const utl::TimeStamp& time,
                         const StationConstants::SignalComponent source)
  const
{
  const auto& hg = fFADCTraces[sdet::PMTConstants::eHighGain];
  const auto it = hg.find(time);
  return it != hg.end() && it->second.HasLabel(source);
}


void
PMTSimData::MakeFADCTrace(const utl::TimeStamp& time,
                          const StationConstants::SignalComponent source)
{
  const auto& m = fFADCTraces[sdet::PMTConstants::eHighGain];
  const auto it = m.find(time);
  if (it == m.end()) {
    for (const auto gain : { sdet::PMTConstants::eHighGain, sdet::PMTConstants::eLowGain }) {
      fFADCTraces[gain].emplace(time, MultiTraceI()).first->second.AddTrace(fFADCTraceLength, fFADCBinSize, source);
      fFADCTracesD[gain].emplace(time, MultiTraceD()).first->second.AddTrace(fFADCTraceLength, fFADCBinSize, source);
    }
  } else {
    const auto& t = it->second;
    if (!t.HasLabel(source)) {
      for (const auto gain : { sdet::PMTConstants::eHighGain, sdet::PMTConstants::eLowGain }) {
        fFADCTraces[gain].find(time)->second.AddTrace(fFADCTraceLength, fFADCBinSize, source);
        fFADCTracesD[gain].find(time)->second.AddTrace(fFADCTraceLength, fFADCBinSize, source);
      }
    }
  }
}


const MultiTraceI&
PMTSimData::GetMultiTrace(const TimeStamp& stamp, const sdet::PMTConstants::PMTGain gain)
  const
{
  const auto& tm = fFADCTraces[gain];
  const auto tr = tm.find(stamp);
  if (tr == tm.end()) {
    ostringstream err;
    err << "Tried to get unavailable FADC trace at time " << stamp << ". ";
    if (tm.empty())
      err << "No times are available.";
    else {
      err << "Available times are:";
      for (const auto& p : tm)
        err << ' ' << p.first;
    }
    ERROR(err);
    throw NonExistentComponentException(err.str());
  }
  return tr->second;
}


const MultiTraceD&
PMTSimData::GetMultiTraceD(const TimeStamp& stamp, const sdet::PMTConstants::PMTGain gain)
  const
{
  const auto& tm = fFADCTracesD[gain];
  const auto tr = tm.find(stamp);
  if (tr == tm.end()) {
    ostringstream err;
    err << "Tried to get unavailable FADC-d trace at time " << stamp << ". ";
    if (tm.empty())
      err << "No times are available.";
    else {
      err << "Available times are:";
      for (const auto& p : tm)
        err << ' ' << p.first;
    }
    ERROR(err);
    throw NonExistentComponentException(err.str());
  }
  return tr->second;
}
