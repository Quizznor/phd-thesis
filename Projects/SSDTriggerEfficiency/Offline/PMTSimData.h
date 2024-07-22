#ifndef _sevt_PMTSimData_h_
#define _sevt_PMTSimData_h_

#include <utl/ShadowPtr.h>
#include <utl/TimeStamp.h>
#include <utl/MultiTrace.h>
#include <sevt/StationConstants.h>
#include <utl/MultiTimeDistribution.h>
#include <utl/IteratorRange.h>
#include <sdet/PMTConstants.h>
#include <utl/ClassHelpers.h>

#include <boost/range/adaptor/map.hpp>

#include <vector>


#warning DV remove EventIO friendship
namespace io {
  class PMTSimData_ROOT;
}

namespace sevt {

  class PMT;


  /**
    \class PMTSimData PMTSimData.h sevt/PMTSimData.h

    \brief Class to hold simulated data at %PMT level

    \author S. Argiro
    \author T. Paul
    \author T. Porter
    \ingroup sevt
    \nosubgrouping
  */

  class PMTSimData {

  private:
    typedef std::map<utl::TimeStamp, utl::MultiTraceI> TimeTraceMap;

  public:
    double GetTriggerVEMPeak(const bool isUUB) const { return isUUB ? fCompatibilityTriggerVEMPeak : fTriggerVEMPeak; }
    void SetTriggerVEMPeak(const double peak) { fTriggerVEMPeak = peak; }
    void SetCompatibilityTriggerVEMPeak(const double peak) { fCompatibilityTriggerVEMPeak = peak; }

    //! \name Photoelectron time distributions. Filled by the tank simulation
    //@{

    /// Simulated photoelectron time distribution
    utl::TimeDistributionI& GetPETimeDistribution(const StationConstants::SignalComponent source = StationConstants::eTotal)
    { return fPETimeDistribution.GetTimeDistribution(source); }

    /// Simulated photoelectron time distribution
    const utl::TimeDistributionI& GetPETimeDistribution(const StationConstants::SignalComponent source = StationConstants::eTotal) const
    { return fPETimeDistribution.GetTimeDistribution(source); }

    /// Create a PE release time distribution (optionally for given source)
    void MakePETimeDistribution(const StationConstants::SignalComponent source = StationConstants::eTotal);

    /// Check if a PE release time distribution exists (optionally for a given source)
    bool HasPETimeDistribution(const StationConstants::SignalComponent source = StationConstants::eTotal) const
    { return fPETimeDistribution.HasLabel(source); }

    /// Iterator over available PE time distribution sources.  See utl::MultiTimeDistribution
    typedef utl::MultiTimeDistributionI::Iterator PETimeDistributionsIterator;

    /// Const iterator over available PE time distribution sources
    typedef utl::MultiTimeDistributionI::ConstIterator ConstPETimeDistributionsIterator;

    /// Begin iterator over PE time distribution sources
    PETimeDistributionsIterator PETimeDistributionsBegin() { return fPETimeDistribution.Begin(); }
    /// Begin iterator over PE time distribution sources
    ConstPETimeDistributionsIterator PETimeDistributionsBegin() const { return fPETimeDistribution.Begin(); }
    /// End iterator over PE time distribution sources
    PETimeDistributionsIterator PETimeDistributionsEnd() { return fPETimeDistribution.End(); }
    /// End iterator over PE time distribution sources
    ConstPETimeDistributionsIterator PETimeDistributionsEnd() const { return fPETimeDistribution.End(); }
    OFFLINE_MAKE_BOTH_ITERATOR_RANGES(PETimeDistributionsIterator, ConstPETimeDistributionsIterator, PETimeDistributions)
    //@}

    //! \name Simulated signal at the PMT base (before the filter).  Filled by PMT simulation.
    //@{

    /// Get simulated signal at the PMT base, optionally for a given source.
    /*! A PE time distribution is read in (as a TimeDistributionI),
        some smearing is done and a new distribution (a TimeDistributionD)
        is written.  This TimeDistributionD represents the signal at the PMT base, <em> before </em>
         the filter.
    */
    utl::TimeDistributionD& GetBaseSignal(const StationConstants::SignalComponent source = StationConstants::eTotal)
    { return fBaseSignal.GetTimeDistribution(source); }

    /// Get simulated signal at the PMT base (before filter)
    const utl::TimeDistributionD& GetBaseSignal(const StationConstants::SignalComponent source = StationConstants::eTotal) const
    { return fBaseSignal.GetTimeDistribution(source); }

    /// Create a TimeDistributionD representing signal at PMT base (optionally for a give source)
    void MakeBaseSignal(const StationConstants::SignalComponent source = StationConstants::eTotal);

    /// Check if signal at PMT base already exists (optionally for a given source)
    bool HasBaseSignal(const StationConstants::SignalComponent source = StationConstants::eTotal) const
    { return fBaseSignal.HasLabel(source); }

    /// Remove signal at PMT base
    void ClearBaseSignals();

    /// Iterator over available PMT base signal sources
    typedef utl::MultiTimeDistributionD::Iterator BaseSignalsIterator;

    /// Const iterator over available PMT base signal sources
    typedef utl::MultiTimeDistributionD::ConstIterator ConstBaseSignalsIterator;

    /// Begin iterator over PMT base signal sources
    BaseSignalsIterator BaseSignalsBegin() { return fBaseSignal.Begin(); }
    /// Const begin iterator over PMT base signal sources
    ConstBaseSignalsIterator BaseSignalsBegin() const { return fBaseSignal.Begin(); }
    /// End iterator over PMT base signal sources
    BaseSignalsIterator BaseSignalsEnd() { return fBaseSignal.End(); }
    /// Const end iterator over PMT base signal sources
    ConstBaseSignalsIterator BaseSignalsEnd() const { return fBaseSignal.End(); }
    OFFLINE_MAKE_BOTH_ITERATOR_RANGES(BaseSignalsIterator, ConstBaseSignalsIterator, BaseSignals)
    //@}

    //! \name Simulated signal after the filter.  Filled by filter & FADC simulation
    //@{

    /// get simulated signal after the filter, optionally for a given source.
    /*! This object should be filled by the filter simulation step.  A time distribution (TimeDistributionD)
      is read in representing the signal at the PMT base, some smearing is done, and
      a new time distribution (TimeDistributionD) is written.  This time distribution represents
      the signal after the filter but <em>before</em> the FADC sampling. */
    utl::TimeDistributionD& GetFilterSignal(const StationConstants::SignalComponent source = StationConstants::eTotal)
    { return fFilterSignal.GetTimeDistribution(source); }

    /// get const simulated signal after the filter
    const utl::TimeDistributionD& GetFilterSignal(const StationConstants::SignalComponent source = StationConstants::eTotal) const
    { return fFilterSignal.GetTimeDistribution(source); }

    /// Create a TimeDistributionD representing signal after front-end filter (optionally for a given source)
    void MakeFilterSignal(const StationConstants::SignalComponent source = StationConstants::eTotal);

    /// Check if the signal at front-end filter already exists (optionally for a given source)
    bool HasFilterSignal(const StationConstants::SignalComponent source = StationConstants::eTotal) const
    { return fFilterSignal.HasLabel(source); }

    /// Iterator over available front-end filter sources
    typedef utl::MultiTimeDistributionD::Iterator FilterSignalsIterator;

    /// Const iterator over available front-end filter sources
    typedef utl::MultiTimeDistributionD::ConstIterator ConstFilterSignalsIterator;

    /// Begin iterator over available front-end filter sources
    FilterSignalsIterator FilterSignalsBegin() { return fFilterSignal.Begin(); }
    /// Const begin iterator over available front-end filter sources
    ConstFilterSignalsIterator FilterSignalsBegin() const { return fFilterSignal.Begin(); }
    /// End iterator over available front-end filter sources
    FilterSignalsIterator FilterSignalsEnd() { return fFilterSignal.End(); }
    /// Const end iterator over available front-end filter sources
    ConstFilterSignalsIterator FilterSignalsEnd() const { return fFilterSignal.End(); }
    OFFLINE_MAKE_BOTH_ITERATOR_RANGES(FilterSignalsIterator, ConstFilterSignalsIterator, FilterSignals)
    //@}

    //! \name Simulated FADC trace before local trigger simulation. Filled by filter and FADC simulation
    //@{

    /// Get FADC trace by gain and source
    /*! The trace is stored as a TimeDistributionI (rather than a TraceI) in
        order to accommodate negative time bins. The trace(s) after local trigger is stored in PMT */
    utl::TimeDistributionI& GetFADCTrace(const sdet::PMTConstants::PMTGain gain = sdet::PMTConstants::eHighGain,
                                         const StationConstants::SignalComponent source = StationConstants::eTotal)
    { return fFADCTrace[gain].GetTimeDistribution(source); }

    /// Get const FADC trace by gain and source
    const utl::TimeDistributionI& GetFADCTrace(const sdet::PMTConstants::PMTGain gain = sdet::PMTConstants::eHighGain,
                                               const StationConstants::SignalComponent source = StationConstants::eTotal) const
    { return fFADCTrace[gain].GetTimeDistribution(source); }

    /// Make FADC trace prior to local trigger simulation
    void MakeFADCTrace(StationConstants::SignalComponent source = StationConstants::eTotal);

    /// Check if FADC trace (prior to local trigger simulation) exists
    bool HasFADCTrace(StationConstants::SignalComponent source = StationConstants::eTotal) const
    { return fFADCTrace[sdet::PMTConstants::eHighGain].HasLabel(source); }

    /// Iterator over available high gain channel FADC sources.
    /*! Returns a LabeledTrace when dereferenced */
    typedef utl::MultiTimeDistributionI::Iterator FADCTraceIterator;

    /// Const iterator over available high gain channel FADC sources.
    typedef utl::MultiTimeDistributionI::ConstIterator ConstFADCTraceIterator;

    /// Begin iterator over available FADC traces (you specify the gain)
    FADCTraceIterator FADCTracesBegin(const sdet::PMTConstants::PMTGain gain = sdet::PMTConstants::eHighGain)
    { return fFADCTrace[gain].Begin(); }
    /// Const begin iterator over available FADC traces (you specify the gain)
    ConstFADCTraceIterator FADCTracesBegin(const sdet::PMTConstants::PMTGain gain = sdet::PMTConstants::eHighGain) const
    { return fFADCTrace[gain].Begin(); }
    /// End iterator over available FADC traces (you specify the gain)
    FADCTraceIterator FADCTracesEnd(const sdet::PMTConstants::PMTGain gain = sdet::PMTConstants::eHighGain)
    { return fFADCTrace[gain].End(); }
    /// Const end iterator over available FADC traces (you specify the gain)
    ConstFADCTraceIterator FADCTracesEnd(const sdet::PMTConstants::PMTGain gain = sdet::PMTConstants::eHighGain) const
    { return fFADCTrace[gain].End(); }
    OFFLINE_MAKE_BOTH_ITERATOR_RANGES_ARG(FADCTraceIterator, ConstFADCTraceIterator, FADCTraces, const sdet::PMTConstants::PMTGain arg = sdet::PMTConstants::eHighGain)
    //@}

    /// Get FADC trace by gain and source (these are double-valued traces so we sidestep the truncation of digital traces.
    /*! The trace is stored as a TimeDistributionD (rather than a TraceI) in
        order to accommodate negative time bins. The trace(s) after local trigger is stored in PMT */
    utl::TimeDistributionD& GetFADCTraceD(const sdet::PMTConstants::PMTGain gain = sdet::PMTConstants::eHighGain,
                                          const StationConstants::SignalComponent source = StationConstants::eTotal)
    { return fFADCTraceD[gain].GetTimeDistribution(source); }

    /// Get const FADC trace by gain and source
    const utl::TimeDistributionD& GetFADCTraceD(const sdet::PMTConstants::PMTGain gain = sdet::PMTConstants::eHighGain,
                                                const StationConstants::SignalComponent source = StationConstants::eTotal) const
    { return fFADCTraceD[gain].GetTimeDistribution(source); }

    /// Iterator over available high gain channel FADC sources.
    /*! Returns a LabeledTrace when dereferenced */
    typedef utl::MultiTimeDistributionD::Iterator FADCTraceDIterator;

    /// Const iterator over available high gain channel FADC sources.
    typedef utl::MultiTimeDistributionD::ConstIterator ConstFADCTraceDIterator;

    /// Begin iterator over available FADC traces (you specify the gain)
    FADCTraceDIterator FADCTracesDBegin(const sdet::PMTConstants::PMTGain gain = sdet::PMTConstants::eHighGain)
    { return fFADCTraceD[gain].Begin(); }
    /// Const begin iterator over available FADC traces (you specify the gain)
    ConstFADCTraceDIterator FADCTracesDBegin(const sdet::PMTConstants::PMTGain gain = sdet::PMTConstants::eHighGain) const
    { return fFADCTraceD[gain].Begin(); }
    /// End iterator over available FADC traces (you specify the gain)
    FADCTraceDIterator FADCTracesDEnd(const sdet::PMTConstants::PMTGain gain = sdet::PMTConstants::eHighGain)
    { return fFADCTraceD[gain].End(); }
    /// Const end iterator over available FADC traces (you specify the gain)
    ConstFADCTraceDIterator FADCTracesDEnd(const sdet::PMTConstants::PMTGain gain = sdet::PMTConstants::eHighGain) const
    { return fFADCTraceD[gain].End(); }
    OFFLINE_MAKE_BOTH_ITERATOR_RANGES_ARG(FADCTraceDIterator, ConstFADCTraceDIterator, FADCTracesD, const sdet::PMTConstants::PMTGain arg = sdet::PMTConstants::eHighGain)
    //@}

    //! \name Simulated FADC traces after local trigger simulation. Filled by the local trigger simulator (obviously)
    //@{
    typedef decltype(std::begin(boost::adaptors::keys(TimeTraceMap()))) TriggerTimeIterator;

    TriggerTimeIterator TriggerTimesBegin() const { return std::begin(boost::adaptors::keys(fFADCTraces[sdet::PMTConstants::eHighGain])); }
    TriggerTimeIterator TriggerTimesEnd() const { return std::end(boost::adaptors::keys(fFADCTraces[sdet::PMTConstants::eHighGain])); }
    OFFLINE_MAKE_CONST_ITERATOR_RANGE(TriggerTimeIterator, TriggerTimes)

    utl::TraceI& GetFADCTrace(const utl::TimeStamp& stamp,
                              const sdet::PMTConstants::PMTGain gain = sdet::PMTConstants::eHighGain,
                              const StationConstants::SignalComponent source = StationConstants::eTotal)
    { return GetMultiTrace(stamp, gain).GetTrace(source); }

    const utl::TraceI& GetFADCTrace(const utl::TimeStamp& stamp,
                                    const sdet::PMTConstants::PMTGain gain = sdet::PMTConstants::eHighGain,
                                    const StationConstants::SignalComponent source = StationConstants::eTotal) const
    { return GetMultiTrace(stamp, gain).GetTrace(source); }

    /// Make FADC trace after local trigger simulation
    void MakeFADCTrace(const utl::TimeStamp& stamp,
                       StationConstants::SignalComponent source = StationConstants::eTotal);

    /// Check if there is an FADC trace (after local trigger simulation)
    bool HasFADCTrace(const utl::TimeStamp& stamp,
                      StationConstants::SignalComponent source = StationConstants::eTotal) const;

    /// Iterator over available high gain channel FADC sources.
    typedef utl::MultiTraceI::Iterator FADCTriggeredTraceIterator;
    typedef utl::MultiTraceI::ConstIterator ConstFADCTriggeredTraceIterator;

    FADCTriggeredTraceIterator FADCTriggeredTracesBegin(const utl::TimeStamp& stamp, const sdet::PMTConstants::PMTGain gain)
    { return GetMultiTrace(stamp, gain).Begin(); }
    FADCTriggeredTraceIterator FADCTriggeredTracesEnd(const utl::TimeStamp& stamp, const sdet::PMTConstants::PMTGain gain)
    { return GetMultiTrace(stamp, gain).End(); }
    ConstFADCTriggeredTraceIterator FADCTriggeredTracesBegin(const utl::TimeStamp& stamp, const sdet::PMTConstants::PMTGain gain) const
    { return GetMultiTrace(stamp, gain).Begin(); }
    ConstFADCTriggeredTraceIterator FADCTriggeredTracesEnd(const utl::TimeStamp& stamp, const sdet::PMTConstants::PMTGain gain) const
    { return GetMultiTrace(stamp, gain).End(); }
    OFFLINE_MAKE_BOTH_ITERATOR_RANGES_ARG2(FADCTriggeredTraceIterator, ConstFADCTriggeredTraceIterator, FADCTriggeredTraces, const utl::TimeStamp& arg1, const sdet::PMTConstants::PMTGain arg2)
    //@}

    //! \name Simulated FADC double-valued traces after local trigger simulation. Filled by the local trigger simulator (obviously)
    //@{

    utl::TraceD& GetFADCTraceD(const utl::TimeStamp& stamp,
                               const sdet::PMTConstants::PMTGain gain = sdet::PMTConstants::eHighGain,
                               const StationConstants::SignalComponent source = StationConstants::eTotal)
    { return GetMultiTraceD(stamp, gain).GetTrace(source); }
    const utl::TraceD& GetFADCTraceD(const utl::TimeStamp& stamp,
                                     const sdet::PMTConstants::PMTGain gain = sdet::PMTConstants::eHighGain,
                                     const StationConstants::SignalComponent source = StationConstants::eTotal) const
    { return GetMultiTraceD(stamp, gain).GetTrace(source); }

    /// Iterator over available high gain channel FADC sources.
    typedef utl::MultiTraceD::Iterator FADCTriggeredTraceDIterator;
    typedef utl::MultiTraceD::ConstIterator ConstFADCTriggeredTraceDIterator;

    FADCTriggeredTraceDIterator FADCTriggeredTracesDBegin(const utl::TimeStamp& stamp, const sdet::PMTConstants::PMTGain gain)
    { return GetMultiTraceD(stamp, gain).Begin(); }
    FADCTriggeredTraceDIterator FADCTriggeredTracesDEnd(const utl::TimeStamp& stamp, const sdet::PMTConstants::PMTGain gain)
    { return GetMultiTraceD(stamp, gain).End(); }
    ConstFADCTriggeredTraceDIterator FADCTriggeredTracesDBegin(const utl::TimeStamp& stamp, const sdet::PMTConstants::PMTGain gain) const
    { return GetMultiTraceD(stamp, gain).Begin(); }
    ConstFADCTriggeredTraceDIterator FADCTriggeredTracesDEnd(const utl::TimeStamp& stamp, const sdet::PMTConstants::PMTGain gain) const
    { return GetMultiTraceD(stamp, gain).End(); }
    OFFLINE_MAKE_BOTH_ITERATOR_RANGES_ARG2(FADCTriggeredTraceDIterator, ConstFADCTriggeredTraceDIterator, FADCTriggeredTracesD, const utl::TimeStamp& arg1, const sdet::PMTConstants::PMTGain arg2)
    //@}

  private:
    PMTSimData() = default;
    ~PMTSimData() = default;

    void SetFADCTraceLength(const unsigned int traceLength) { fFADCTraceLength = traceLength; }
    void SetFADCBinSize(const double binSize) { fFADCBinSize = binSize; }

    /// Utility function to retrieve the multi trace and throw an exception when it fails
    utl::MultiTraceI& GetMultiTrace(const utl::TimeStamp& stamp, const sdet::PMTConstants::PMTGain gain)
    { return OFFLINE_CAST_CONST_METHOD(GetMultiTrace(stamp, gain)); }
    const utl::MultiTraceI& GetMultiTrace(const utl::TimeStamp& stamp, const sdet::PMTConstants::PMTGain gain) const;

    utl::MultiTraceD& GetMultiTraceD(const utl::TimeStamp& stamp, const sdet::PMTConstants::PMTGain gain)
    { return OFFLINE_CAST_CONST_METHOD(GetMultiTraceD(stamp, gain)); }
    const utl::MultiTraceD& GetMultiTraceD(const utl::TimeStamp& stamp, const sdet::PMTConstants::PMTGain gain) const;

    double fTriggerVEMPeak = 0;
    double fCompatibilityTriggerVEMPeak = 0;

    utl::MultiTimeDistributionI fPETimeDistribution;
    utl::MultiTimeDistributionD fBaseSignal;
    utl::MultiTimeDistributionD fFilterSignal;

    unsigned int fFADCTraceLength = 0;
    double fFADCBinSize = 0;

    utl::MultiTimeDistributionI fFADCTrace[sdet::PMTConstants::eNumberOfGains];
    utl::MultiTimeDistributionD fFADCTraceD[sdet::PMTConstants::eNumberOfGains];

    typedef std::map<utl::TimeStamp, utl::MultiTraceI> TimeTraceIMap;
    typedef std::map<utl::TimeStamp, utl::MultiTraceD> TimeTraceDMap;
    TimeTraceIMap fFADCTraces[sdet::PMTConstants::eNumberOfGains];
    TimeTraceDMap fFADCTracesD[sdet::PMTConstants::eNumberOfGains];

    friend class sevt::PMT;
    friend class utl::ShadowPtr<sevt::PMTSimData>;
    friend class io::PMTSimData_ROOT;

  };

}


#endif
