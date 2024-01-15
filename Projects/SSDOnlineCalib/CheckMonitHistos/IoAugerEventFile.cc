#include <evt/Header.h>
#include <evt/Event.h>

#include <fevt/FEvent.h>
#include <fevt/Eye.h>

#include <sevt/SEvent.h>
#include <sevt/Header.h>
#include <sevt/EventTrigger.h>

#include <io/CDASToOfflineEventConverter.h>
#include <io/FDasToOfflineEventConverter.h>
#include <io/IoAugerEventFile.h>

#include <utl/ErrorLogger.h>
#include <utl/AugerException.h>
#include <utl/config.h>

// CDAS classes
#include <AugerEvent.h>
#include <IoSdData.h>

#include <MdEvent.h>
#include <mevt/MEvent.h>

#include <RadioFileIO.h>
#include <revt/REvent.h>
#include <revt/Station.h>
#include <revt/Channel.h>
#include <revt/Header.h>
#include <revt/StationTriggerData.h>
#include <revt/StationGPSData.h>
#include <revt/EventTrigger.h>
#include <revt/StationHeader.h>
#include <det/Detector.h>
#include <rdet/RDetector.h>
#include <rdet/Station.h>

#include <sstream>
#include <cstddef>

using namespace fevt;
using namespace utl;
using namespace revt;


namespace io {

  IoAugerEventFile::IoAugerEventFile(const std::string& filename,
                                     const Mode mode, utl::Branch* const b) :
    VEventFile(filename, mode, b)
  {
    IoAugerEventFile::Open(filename, mode, b);
  }


  IoAugerEventFile::~IoAugerEventFile()
  {
    delete fAugerFile;
  }


  void
  IoAugerEventFile::Open(const std::string& filename, const Mode mode, utl::Branch* const /*b*/)
  {
    if (fAugerFile)
      Close();

    const AugerFile::Mode ioauger_mode = this->ConvertMode(mode);

    fAugerFile = new AugerFile;

    if (fAugerFile->Open(filename.c_str(), ioauger_mode) != AugerFile::eSuccess)
      throw utl::IOFailureException("Could not open file");
  }


  Status
  IoAugerEventFile::Read(evt::Event& event)
  {
    AugerEvent raw;
    const AugerFile::Status s = fAugerFile->ReadNext(raw);

    if (s == AugerFile::eEOF)
      return eEOF;

    if (s == AugerFile::eFailure)
      return eFail;

    // copy the raw AugerEvent to evt::Event
    if (!event.HasRawEvent())
      event.MakeRawEvent();

    event.GetRawEvent() = raw;

    // place the FDas part
    for (AugerEvent::EyeIterator eyeIter = raw.EyesBegin(), eyesEnd = raw.EyesEnd();
         eyeIter != eyesEnd; ++eyeIter)
      event << *eyeIter;

    ostringstream id_str;
    id_str << "auger_" << raw.Id();

    evt::Header& header = event.GetHeader();

    // if event contains SD part take its time!
    // FD times are handled in FDasToOfflineEventConverter

    if (raw.HasSd()) {

      const IoSdT3Trigger* const rawTrigger = &raw.Sd().Trigger;
      const unsigned int second = rawTrigger->Second;
      const unsigned int microSecond =
        int(rawTrigger->MicroSecond * microsecond/nanosecond);
      const TimeStamp currentTime(second, microSecond);
      header.SetTime(currentTime);

      if (!event.HasSEvent())
        event.MakeSEvent();

      sevt::SEvent& sEvent = event.GetSEvent();
      sevt::Header& sHeader = sEvent.GetHeader();
      sHeader.SetId(raw.Sd().Id);
      sHeader.SetTime(currentTime);

      if (raw.HasMd()) {

        const md::Event mdRawEvt = raw.Md();
        const md::Trigger mdTrg = mdRawEvt.GetTrigger();
        const unsigned int t3Sec = mdTrg.GetT3Sec();
        const unsigned int t3Mic = int(mdTrg.GetT3Mic() * utl::microsecond/utl::nanosecond);
        const TimeStamp mdTime(t3Sec, t3Mic);

        md::ID mdId = mdTrg.GetId();

        if (!event.HasMEvent())
          event.MakeMEvent();

        mevt::MEvent& mEvent = event.GetMEvent();
        mevt::Header& mHeader = mEvent.GetHeader();
        mHeader.SetId(mdId);
        mHeader.SetTime(mdTime);  // by construction, sould be identical to currentTime of SD

      }

    }

    if (raw.HasRd()) {
      if (!event.HasREvent())
        event.MakeREvent();

      revt::REvent& revent = event.GetREvent();
      revt::Header& rHeader = revent.GetHeader();

      const int run_id = raw.REvent->getAeraEventRunId();
      const int event_id = raw.REvent->getAeraEventEventId();

      rHeader.SetRunNumber(run_id);
      rHeader.SetId(event_id);
      //rHeader.SetEventNumber(raw.REvent->getAeraEventRunningEventId());
      rHeader.SetFileId(raw.REvent->getAeraEventFileId());
      rHeader.SetNumberOfStations(raw.REvent->getAeraEventLsCount());
      rHeader.SetEventVersion(raw.REvent->getAeraEventEventVersion());

      // append rd run and event id to event header
#warning JN: this should probably better be done in the CDASToOfflineConverter as for SD, MD, etc.
      id_str << "__rd_" << run_id << "." << event_id;

      {
        const unsigned int ts_s = raw.REvent->getAeraEventSeconds();
        const unsigned int ts_ns = raw.REvent->getAeraEventNanoSeconds();

        utl::TimeStamp ts;
        try {
          ts.SetGPSTime(ts_s, ts_ns);
        } catch (OutOfBoundException& /*e*/) {
          WARNING("Event skipped! TimeStamp was not filled correctly.");
          return eSkipEvent;
        }

        det::Detector::GetInstance().Update(ts);
        //event.GetHeader().SetTime(ts);
        revent.GetHeader().SetTime(ts);
      }

      if (revent.HasTrigger())
        WARNING("Warning overwrites Event Trigger information");
      else
        revent.MakeTrigger();

      if (raw.REvent->getAeraEventStatus() == 2) {
        WARNING("Event skipped! Event does not have an active station.");
        return eSkipEvent;
      }

      int triggersourceIsAevb = 1;
      int triggersourceIsSelf = 0;
      int triggersourceIsCalib = 0;
      int triggersourceIsScint = 0;
      int triggersourceIsSD = 0;
      int triggersourceIsGui = 0;
      int triggersourceIsFD = 0;
      int triggersourceIsHEAT = 0;
      int triggersourceIsAERAlet = 0;
      int triggersourceIsAIRPLANE = 0;
      int triggersourceIsPeriodic = 0;
      int triggersourceIsPassThrough = 0;

      for (int i = 0; i < raw.REvent->getAeraEventLsCount(); ++i) {
        const auto aStation = raw.REvent->getAeraEventStation(i);
        if (aStation->getAeraStationTriggerIsAevb() != 1) {
          // triggersource is only set if not unknown (=0)
          if (!triggersourceIsSelf)
            triggersourceIsSelf = aStation->getAeraStationTriggerIsSelf();
          if (!triggersourceIsCalib)
            triggersourceIsCalib = aStation->getAeraStationTriggerIsCalibration();
          if (!triggersourceIsScint)
            triggersourceIsScint = aStation->getAeraStationTriggerIsScintillator();
          if (!triggersourceIsSD)
            triggersourceIsSD = aStation->getAeraStationTriggerIsSD();
          if (!triggersourceIsGui)
            triggersourceIsGui = aStation->getAeraStationTriggerIsGUI();
          if (!triggersourceIsFD)
            triggersourceIsFD = aStation->getAeraStationTriggerIsFD();
          if (!triggersourceIsHEAT)
            triggersourceIsHEAT = aStation->getAeraStationTriggerIsHEAT();
          if (!triggersourceIsAERAlet)
            triggersourceIsAERAlet = aStation->getAeraStationTriggerIsAERAlet();
          if (!triggersourceIsAIRPLANE)
            triggersourceIsAIRPLANE = aStation->getAeraStationTriggerIsAirplane();
          if (!triggersourceIsPeriodic)
            triggersourceIsPeriodic = aStation->getAeraStationTriggerIsPeriodic();
          if (!triggersourceIsPassThrough)
            triggersourceIsPassThrough = aStation->getAeraStationTriggerIsPassThrough();
          revent.GetTrigger().SetBitPattern(aStation->getAeraStationTriggerFlag());
          //triggersourceIsAevb = 0;
          //break;
        }
      }

      if (triggersourceIsSelf || triggersourceIsCalib || triggersourceIsScint || triggersourceIsSD ||
          triggersourceIsGui || triggersourceIsFD || triggersourceIsHEAT || triggersourceIsAERAlet ||
          triggersourceIsAIRPLANE || triggersourceIsPeriodic || triggersourceIsPassThrough)
        triggersourceIsAevb = 0;

      if (triggersourceIsSelf == 1) {
        revent.GetTrigger().SetSelfTrigger(true);
        WARNING("Event IsSelf");
      }
      if (triggersourceIsCalib == 1) {
        revent.GetTrigger().SetCalibrationTrigger(true);
        WARNING("Event IsCalib");
      }
      if (triggersourceIsScint == 1) {
        revent.GetTrigger().SetScintillatorTrigger(true);
        WARNING("Event IsScint");
      }
      if (triggersourceIsSD == 1) {
        revent.GetTrigger().SetSDTrigger(true);
        WARNING("Event IsSD");
      }
      if (triggersourceIsGui == 1) {
        revent.GetTrigger().SetGUITrigger(true);
        WARNING("Event IsGUI");
      }
      if (triggersourceIsFD == 1) {
        revent.GetTrigger().SetFDTrigger(true);
        WARNING("Event IsFD");
      }
      if (triggersourceIsHEAT == 1) {
        revent.GetTrigger().SetHEATTrigger(true);
        WARNING("Event IsHEAT");
      }
      if (triggersourceIsAERAlet == 1) {
        revent.GetTrigger().SetAERAletTrigger(true);
        WARNING("Event IsAERALET");
      }
      if (triggersourceIsAIRPLANE == 1) {
        revent.GetTrigger().SetAIRPLANETrigger(true);
        WARNING("Event IsAIRPLANE");
      }
      if (triggersourceIsPeriodic == 1) {
        revent.GetTrigger().SetPeriodicTrigger(true);
        WARNING("Event IsPeriodic");
      }
      if (triggersourceIsPassThrough == 1) {
        revent.GetTrigger().SetPassThroughTrigger(true);
        WARNING("Event IsPassThrough");
      }
      if (triggersourceIsSD == 1 || triggersourceIsGui == 1 || triggersourceIsFD == 1 ||
          triggersourceIsHEAT == 1 || triggersourceIsAERAlet == 1 || triggersourceIsAIRPLANE == 1 ||
          triggersourceIsPeriodic == 1 || triggersourceIsPassThrough == 1)
        revent.GetTrigger().SetExternalTrigger(true);
      if (triggersourceIsAevb == 1) {
        WARNING("Event skipped! No known trigger source found.");
        return eSkipEvent;
      }

      /////////////////////////////////////////////////////////////////////////////////////

      ostringstream stationname;
      for (int i = 0; i < raw.REvent->getAeraEventLsCount(); ++i) { //LS station loop

        const auto aStation = raw.REvent->getAeraEventStation(i);

        const int station_id = aStation->getAeraStationLsID();
        if (aStation->getAeraStationTriggerIsAevb() == 1 ||
            aStation->getAeraStationTracelength() == 0)
          continue;  // skip station if aevb Event or no channel data

        stationname.str("");

        if (station_id > 200 && station_id < 210)
          stationname << "M" << (station_id - 200);
        else
          stationname << "AERA_" << station_id;
        try {
          if (revent.HasStation(stationname.str()))
            WARNING("Overwriting station information");
          else
            revent.MakeStation(stationname.str());

          revt::Station& station = revent.GetStationByName(stationname.str());
          // Get the RDetector::Station from revt::Station
          const rdet::Station& detstation = det::Detector::GetInstance().GetRDetector().GetStation(station);
          if (station.HasTriggerData())
            WARNING("Overwriting station trigger information");
          else
            station.MakeTriggerData();

          triggersourceIsSelf = aStation->getAeraStationTriggerIsSelf();
          triggersourceIsCalib = aStation->getAeraStationTriggerIsCalibration();
          triggersourceIsScint = aStation->getAeraStationTriggerIsScintillator();
          triggersourceIsSD = aStation->getAeraStationTriggerIsSD();
          triggersourceIsGui = aStation->getAeraStationTriggerIsGUI();
          triggersourceIsFD = aStation->getAeraStationTriggerIsFD();
          triggersourceIsHEAT = aStation->getAeraStationTriggerIsHEAT();
          triggersourceIsAERAlet = aStation->getAeraStationTriggerIsAERAlet();
          triggersourceIsAIRPLANE = aStation->getAeraStationTriggerIsAirplane();
          triggersourceIsPeriodic = aStation->getAeraStationTriggerIsPeriodic();
          triggersourceIsPassThrough = aStation->getAeraStationTriggerIsPassThrough();
          triggersourceIsAevb = aStation->getAeraStationTriggerIsAevb();

          if (triggersourceIsSelf)
            station.GetTriggerData().SetTriggerSource(StationTriggerData::eSelf);
          else if (triggersourceIsCalib)
            station.GetTriggerData().SetTriggerSource(StationTriggerData::eCalib);
          else if (triggersourceIsScint)
            station.GetTriggerData().SetTriggerSource(StationTriggerData::eScint);
          else if (triggersourceIsSD)
            station.GetTriggerData().SetTriggerSource(StationTriggerData::eSD);
          else if (triggersourceIsGui)
            station.GetTriggerData().SetTriggerSource(StationTriggerData::eGUI);
          else if (triggersourceIsFD)
            station.GetTriggerData().SetTriggerSource(StationTriggerData::eFD);
          else if (triggersourceIsHEAT)
            station.GetTriggerData().SetTriggerSource(StationTriggerData::eHEAT);
          else if (triggersourceIsAERAlet)
            station.GetTriggerData().SetTriggerSource(StationTriggerData::eAERAlet);
          else if (triggersourceIsAIRPLANE)
            station.GetTriggerData().SetTriggerSource(StationTriggerData::eAIRPLANE);
          else if (triggersourceIsPeriodic)
            station.GetTriggerData().SetTriggerSource(StationTriggerData::ePeriodic);
          else if (triggersourceIsPassThrough)
            station.GetTriggerData().SetTriggerSource(StationTriggerData::ePassThrough);
          else if (triggersourceIsSD || triggersourceIsGui || triggersourceIsFD || triggersourceIsHEAT ||
                   triggersourceIsAERAlet || triggersourceIsAIRPLANE || triggersourceIsPeriodic ||
                   triggersourceIsPassThrough)
            station.GetTriggerData().SetTriggerSource(StationTriggerData::eExternal);
          else if (triggersourceIsAevb)
            WARNING("Trigger source unknown on station level.");
          station.GetTriggerData().SetOffset(aStation->getAeraStationTriggerPos());
          if (station.HasGPSData())
            WARNING("Overwriting station GPS information");
          else
            station.MakeGPSData();
          StationGPSData& gps = station.GetGPSData();

          if (station.HasStationHeader())
            WARNING("Overwriting station header");
          else
            station.MakeStationHeader();
          if (aStation->getAeraStationHWType() == 1)
            station.GetStationHeader().SetHardwareType(StationHeader::eNL);
          else if (aStation->getAeraStationHWType() == 2)
            station.GetStationHeader().SetHardwareType(StationHeader::eFR);
          else if (aStation->getAeraStationHWType() == 3)
            station.GetStationHeader().SetHardwareType(StationHeader::eDE);
          else {
            ostringstream msg;
            msg << " Unknown Hardware type :: getAeraEvbHardwareType() = "
                << aStation->getAeraStationHWType() << "\n"
                   " Skip to next event";
            ERROR(msg);
            break;  // skip to next event -> break and not continue?
          }
          station.GetStationHeader().SetHardwareVersion(aStation->getAeraStationHWVersion());

          const double samplingfreq = aStation->getAeraStationSamplingFreq() * utl::megahertz;
          utl::TimeStamp stationts;
          {
            const unsigned int ts_s = aStation->getAeraStationSeconds();
            try {
              stationts.SetGPSTime(ts_s, aStation->getAeraStationNanoseconds());
              gps.SetSecond(ts_s);  // ? Why was ts_gps_s
              // corrected from what ?
              gps.SetCorrectedNanosecond(aStation->getAeraStationNanoseconds());
            } catch (utl::OutOfBoundException& e) {
              WARNING("Event skipped! TimeStamp was not filled correctly.");
              return eSkipEvent;
            }
          }
          double pretrigtime = 0;

          if (aStation->getAeraStationHWType() == 1) {
            if ((aStation->getAeraStationHWVersion() < 7 &&
                 aStation->getAeraStationHWVersion() > 0) ||
                (raw.REvent->getAeraEventRunId() > 200000 && raw.REvent->getAeraEventRunId() < 200300)) {  // New NL data
              const unsigned int s = aStation->getAeraStationMessagePreTriggerWindow();
              const unsigned int t = aStation->getAeraStationMessageCoincidenceWindow();
              pretrigtime = (s + t) / samplingfreq;
            } else if (aStation->getAeraStationHWVersion() >= 7) {
              union {
                unsigned short int pre;
                unsigned char uc[2];
              };
              uc[0] = aStation->getAeraStationMessageV2PPSWindows(0);
              uc[1] = aStation->getAeraStationMessageV2PPSWindows(1);
              const unsigned int coinc = aStation->getAeraStationMessageV2PPSControl(6);
              pretrigtime = (pre + coinc) / samplingfreq;
            }
          } else
            pretrigtime = aStation->getAeraStationTriggerPos() / samplingfreq;

          utl::TimeInterval pretrig(pretrigtime);
          try {
            station.SetRawTraceStartTime(stationts - pretrig);
          } catch (utl::OutOfBoundException& e) {
            WARNING("Event skipped! TimeStamp was not filled correctly.");
            return eSkipEvent;
          }

          // read temperature from additional station info, available for NL stations from LS version 3 (last 4 chars of additional info)
          if (aStation->getAeraStationHWType() == 1 && aStation->getAeraStationVersion() >= 3) {
            union {
              unsigned char uc[4];
              float addTemp;
            };
            for (int k = 36; k < 36+4; ++k)
              uc[k - 36] = aStation->getAeraStationMessageV2PPSGPS(k);
            station.SetHWTemperature(addTemp);
          } else {
            const float addTempInt = aStation->getAeraStationGerTemperatureInt();
            const float addTempExt = aStation->getAeraStationGerTemperatureExt();
            station.SetHWTemperatureInt(addTempInt + 273.15);
            station.SetHWTemperatureExt(addTempExt + 273.15);
          }

          ////// Need additional check for new NL HW type! e.g. sd200272 -> 676 ok, other events: (nl) /4?
          const int numberofsamples = aStation->getAeraStationTracelength();

          unsigned int thresh = 0;  //// UINT16 in RadioFileAERA
          ostringstream info;
          for (int ch_id = 1; ch_id <= detstation.GetNChannels(); ++ch_id) {
            info.str("");
            revt::Channel& chan = station.GetChannel(ch_id);
            //If Channel is not present in data, it will be set as inactive;
            if ((aStation->getAeraStationChannelMask() & (1 << (ch_id-1))) == 0) {
              chan.SetNotActive();
              continue;
            }
            chan.SetActive();
            revt::ChannelADCTimeSeries& thistrace = chan.GetChannelADCTimeSeries();  // read & write mode
            if (aStation->getAeraStationHWType() == 3 || aStation->getAeraStationHWType() == 1) {
              thistrace.SetBinning(1 / samplingfreq);
              
              if (aStation->getAeraStationTracelength() == 0) {
                info.str("");
                info << " No data in channel: " << ch_id << ". "
                        " Channel will be marked as not active.";
                WARNING(info);
                chan.SetNotActive();
                continue;
              }
              for (int ii = 0; ii < numberofsamples; ++ii)
                thistrace.PushBack(aStation->getAeraStationADC(ch_id-1)->GetADCValues(ii));
            } else {
              info.str("");
              info << "Electronic readout is not implemented. Electronic type is "
                   << aStation->getAeraStationHWType() << ". "
                      ". Channel is set to not active\n";
              chan.SetNotActive();
              WARNING(info);
            }
            chan.SetNyquistZone(1);

//#warning SJ: if statement is workaround to avoid needless output of aevread for DE data, should be fixed in next version
            if (aStation->getAeraStationHWType() == 1) {
              if (((aStation->getAeraStationHWVersion() < 7) &&
                   (aStation->getAeraStationHWVersion() > 0)) ||
                  ((raw.REvent->getAeraEventRunId() > 200000) && (raw.REvent->getAeraEventRunId() < 200300 )) ) {  // New NL data
                if (ch_id == 1)
                  thresh = aStation->getAeraStationMessageSignalThresholdCH0();
                if (ch_id == 2)
                  thresh = aStation->getAeraStationMessageSignalThresholdCH1();
                if (ch_id == 3)
                  thresh = aStation->getAeraStationMessageSignalThresholdCH2();
                if (ch_id == 4)
                  thresh = aStation->getAeraStationMessageSignalThresholdCH3();
                // 0xFFFF is the Error code returned by the library in case no threshold is found
                if (thresh != 0xFFFF)
                  chan.SetSignalThreshold(thresh);
                if (ch_id == 1)
                  thresh = aStation->getAeraStationMessageNoiseThresholdCH0();
                if (ch_id == 2)
                  thresh = aStation->getAeraStationMessageNoiseThresholdCH1();
                if (ch_id == 3)
                  thresh = aStation->getAeraStationMessageNoiseThresholdCH2();
                if (ch_id == 4)
                  thresh = aStation->getAeraStationMessageNoiseThresholdCH3();
                if (thresh != 0xFFFF)
                  chan.SetNoiseThreshold(thresh);
              } else if (aStation->getAeraStationHWVersion() >= 7) {
                if (ch_id == 1)
                  thresh = aStation->getAeraStationMessageV2Threshold1CH1();
                if (ch_id == 2)
                  thresh = aStation->getAeraStationMessageV2Threshold1CH2();
                if (ch_id == 3)
                  thresh = aStation->getAeraStationMessageV2Threshold1CH3();
                if (ch_id == 4)
                  thresh = aStation->getAeraStationMessageV2Threshold1CH4();
                // 0xFFFF is the Error code returned by the library in case no threshold is found
                if (thresh != 0xFFFF)
                  chan.SetSignalThreshold(thresh);
                if (ch_id == 1)
                  thresh = aStation->getAeraStationMessageV2Threshold2CH1();
                if (ch_id == 2)
                  thresh = aStation->getAeraStationMessageV2Threshold2CH2();
                if (ch_id == 3)
                  thresh = aStation->getAeraStationMessageV2Threshold2CH3();
                if (ch_id == 4)
                  thresh = aStation->getAeraStationMessageV2Threshold2CH4();
                if (thresh != 0xFFFF)
                  chan.SetNoiseThreshold(thresh);
              }
            } else {
              chan.SetSignalThreshold(thresh);
              chan.SetNoiseThreshold(thresh);
            }
          } //for (channel) ...
        } catch (utl::NonExistentComponentException& e) {
          ERROR(e.what());
        }
      } // for (station)
    }

    header.SetId(id_str.str());

    return eSuccess;
  }


  void
  IoAugerEventFile::Write(const evt::Event& event)
  {
    if (fMode == eRead) {
      const string error = "Attempted to write into read-only IoAuger event file.";
      ERROR(error);
      throw utl::IOFailureException(error);
      return;
    }

    if (fAugerFile) {

      if (!event.HasRawEvent()) {
        INFO("Non-existent RawEvent, unable to stream to file.");
        return;
      }

      bool hasFD = false;
      if (event.HasFEvent()) {
        const FEvent& fdEvent = event.GetFEvent();
        for (FEvent::ConstEyeIterator eIt = fdEvent.EyesBegin(ComponentSelector::eHasData),
             eyesEnd = fdEvent.EyesEnd(ComponentSelector::eHasData); eIt != eyesEnd; ++eIt)
          if (eIt->HasTriggerData()) {
            hasFD = true;
            break;
          }
      }

      // skip non-triggered SD/Hy events ! But take FD triggered events!
      if (event.HasSEvent() && !hasFD) {

        const sevt::SEvent& sEvent = event.GetSEvent();

        if (!sEvent.HasTrigger()) {
          INFO("Non-existent sd-trigger, unable to stream to file.");
          return;
        }

        if (sEvent.GetTrigger().GetAlgorithm() == "None") {
          INFO("No trigger, skipping event in IoAuger. "
               "Use Offline-native for streaming.");
          return;
        }

      }

      const AugerEvent& raw = event.GetRawEvent();

      fAugerFile->Write(const_cast<AugerEvent&>(raw));

    }
  }


  void
  IoAugerEventFile::Close()
  {
    if (fAugerFile)
      fAugerFile->Close();

    delete fAugerFile;
    fAugerFile = nullptr;
  }


  AugerFile::Mode
  IoAugerEventFile::ConvertMode(const Mode mode)
  {
    switch (mode) {
    case io::eRead:   return AugerFile::eRead;
    case io::eWrite:  return AugerFile::eWrite;
    case io::eNew:    return AugerFile::eWrite;
    case io::eAppend: return AugerFile::eWrite;
    default:          return AugerFile::eWrite;
    }
  }


  Status
  IoAugerEventFile::FindEvent(const unsigned int)
  {
    return eFail;
  }


  Status
  IoAugerEventFile::GotoPosition(const unsigned int)
  {
    return eFail;
  }


  int
  IoAugerEventFile::GetNEvents()
  {
#ifdef IOAUGER_V0R6
     return int(fAugerFile->NumberOfEvents());
#else
     return 0;
#endif
  }

}
