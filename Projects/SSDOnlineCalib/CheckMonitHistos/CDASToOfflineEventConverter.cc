/**
  \file CDASToOfflineEventConverter.cc

  \author Troy Porter
  \author Darko Veberic
  \author Ralf Ulrich
  \date 28 May 2003
*/

#include <io/CDASToOfflineEventConverter.h>

#include <det/Detector.h>
#include <sdet/SDetector.h>
#include <sdet/Station.h>
#include <sdet/PMT.h>
#include <sdet/PMTConstants.h>

#include <evt/Event.h>
#include <evt/ShowerRecData.h>
#include <evt/ShowerSRecData.h>
#include <evt/ShowerFRecData.h>
#include <evt/VGaisserHillasParameter.h>
#include <evt/GaisserHillas2Parameter.h>

#include <sevt/EventTrigger.h>
#include <sevt/Header.h>
#include <sevt/SEvent.h>
#include <sevt/Station.h>
#include <sevt/StationCalibData.h>
#include <sevt/StationRecData.h>
#include <sevt/StationGPSData.h>
#include <sevt/StationTriggerData.h>
#include <sevt/PMT.h>
#include <sevt/PMTCalibData.h>
#include <sevt/PMTRecData.h>
#include <sevt/Meteo.h>

#include <fwk/LocalCoordinateSystem.h>
#include <fwk/CoordinateSystemRegistry.h>

#include <utl/Math.h>
#include <utl/MathConstants.h>
#include <utl/CoordinateSystemPtr.h>
#include <utl/Point.h>
#include <utl/AugerCoordinateSystem.h>
#include <utl/ErrorLogger.h>
#include <utl/TimeStamp.h>
#include <utl/UTCDateTime.h>
#include <utl/Trace.h>
#include <utl/TraceAlgorithm.h>
#include <utl/UTMPoint.h>
#include <utl/String.h>
#include <utl/Test.h>
#include <utl/Is.h>

// CDAS Classes
#include <IoSdData.h>
#include <Ec.h>
#include <Es.h>
#include <Er.h>
#include <MdEvent.h>

// Offline
#include <mdet/MDetector.h>
#include <mdet/Counter.h>
#include <mdet/Module.h>
#include <mdet/FrontEnd.h>

#include <mevt/MEvent.h>
#include <mevt/Counter.h>
#include <mevt/Module.h>
#include <mevt/Channel.h>

#include <rdet/RDetector.h>
#include <rdet/Station.h>
#include <rdet/Channel.h>

#include <revt/REvent.h>
#include <revt/Station.h>
#include <revt/Channel.h>
#include <revt/Header.h>
#include <revt/StationTriggerData.h>
#include <revt/StationGPSData.h>
#include <revt/EventTrigger.h>
#include <revt/StationHeader.h>
#include <revt/StationConstants.h>

#include <iostream>
#include <iomanip>
#include <string>
#include <vector>
#include <set>

using namespace evt;
using namespace sevt;
using namespace fwk;
using namespace utl;
using namespace io;
using namespace std;


//#define DEBUGRAW(x) cout << x
#define DEBUGRAW(x)
//#define DEBUGEC(x) cout << x
#define DEBUGEC(x)
//#define DEBUGES(x) cout << x
#define DEBUGES(x)
//#define DEBUGER(x) cout << x
#define DEBUGER(x)


namespace io {

  template<typename T1, typename T2>
  void
  ConditionalCopy(const T1& src,
                  T2* const dst, const unsigned int dstSize,
                  const string& what)
  {
    static bool beenHere = false;
    const unsigned int srcSize = src.size();
    if (!beenHere && srcSize != dstSize) {
      ostringstream warn;
      warn << what << " size mismatch: source " << srcSize << ", "
              "destination " << dstSize << '.';
      WARNING(warn);
      beenHere = true;
    }
    const unsigned int n = min(srcSize, dstSize);
    unsigned int i;
    for (i = 0; i < n; ++i)
      dst[i] = src[i];
    for (; i < dstSize; ++i)
      dst[i] = 0;
  }


  bool
  HasRDetectorConfig(const bool printMsg = false)
  {
    const auto& managerRRegister = det::Detector::GetInstance().GetRManagerRegister();

    // no RManager registered
    if (managerRRegister.ManagersBegin() == managerRRegister.ManagersEnd()) {
      if (printMsg)
        INFO("No RDetector is configured. No read-in of RD data possible!");
      return false;
    }
    return true;
  }


  bool
  HasMDetectorConfig(const bool printMsg = false)
  {
    const auto& managerMRegister = det::Detector::GetInstance().GetMManagerRegister();

    // no MManager registered
    if (managerMRegister.ManagersBegin() == managerMRegister.ManagersEnd()) {
      if (printMsg)
        INFO("No MDetector is configured. No read-in of UMD data possible!");
      return false;
    }
    return true;
  }

}


namespace sdet {

  /// friend class to unlock private methods of sdet::Station
  class Unlock {
  public:
    Unlock(const sdet::Station& station) : fStation(station) { }
    void UpdateElectronics(const bool isUUB) const { fStation.UpdateElectronics(isUUB); }
    void MakeScintillator() const { fStation.MakeScintillator(); }
    void RemoveScintillator() const { fStation.RemoveScintillator(); }
    void MakeSmallPMT() const { fStation.MakeSmallPMT(); }
    void RemoveSmallPMT() const { fStation.RemoveSmallPMT(); }
  private:
    const sdet::Station& fStation;
  };

}


namespace evt {

  /// grabs the data of an IoSdEvent and stores it in evt::Event

  void
  operator<<(Event& event, const IoSdEvent& rawEvent)
  {
    ConvertIoSdToEvent(event, rawEvent);
  }


  static const std::set<int> gCycloneStations({600, 610, 620, 627, 630, 648});
  static const std::set<int> gCycloneYears({2007, 2008, 2009});


  /// sanitized version of the error flag (ie no UUB bit)
  constexpr
  int
  GetError(const IoSdStation& station)
  {
    // UUB sends bit 7 (8th bit) set when error = 0, use only lowest 8 bits:
    return station.Error & 0xff;
  }


  constexpr
  int
  DuplicatePriority(const IoSdStation& station)
  {
    switch (GetError(station)) {
    case IoSdEvent::eNoError:     return 8;
    case IoSdEvent::eT3Lost:      return 7;
    case IoSdEvent::eT3TooYoung:  return 6;
    case IoSdEvent::eT3Already:   return 5;
    case IoSdEvent::eT1Stopped:   return 4;
    case IoSdEvent::eBadCompress: return 3;
    case IoSdEvent::eDataLost:    return 2;
    case IoSdEvent::eT3NotFound:  return 1;
    default:                      return 0;
    }
  }


  void
  ConvertIoSdToEvent(evt::Event& oEvent, const IoSdEvent& rEvent)
  {
    DEBUGRAW(" PullEventRaw - IoSdEvent " << endl);

    if (!oEvent.HasSEvent())
      oEvent.MakeSEvent();

    auto& oSEvent = oEvent.GetSEvent();

    if (!oSEvent.HasTrigger())
      oSEvent.MakeTrigger();

    oSEvent.SetNErrorZeroStations(rEvent.NumberOfErrorZeroStation);

    // sevt header
    auto& osHeader = oSEvent.GetHeader();
    osHeader.SetId(rEvent.Id);

    const auto& rTrigger = rEvent.Trigger;
    const TimeStamp currentTime(rTrigger.Second, rTrigger.MicroSecond * 1000);
    osHeader.SetTime(currentTime);

    // evt header
    auto& oHeader = oEvent.GetHeader();
    oHeader.SetTime(currentTime);

    {
      ostringstream idos;
      if (!oHeader.GetId().empty())
        idos << oHeader.GetId() << "__";
      idos << "sd_" << rEvent.Id;
      const auto& id = idos.str();
      oHeader.SetId(id);
      INFO(id);
    }

    // trigger
    auto& oTrigger = oSEvent.GetTrigger();

    oTrigger.SetId(rTrigger.Id);
    oTrigger.SetPreviousId(rTrigger.PreviousId);
    oTrigger.SetTime(currentTime);
    oTrigger.SetSender(rTrigger.Sender);
    oTrigger.SetAlgorithm(rTrigger.Algo);
    oTrigger.SetSDPAngle(rTrigger.SDPAngle);

    const auto& detector = det::Detector::GetInstance();
    const auto& rDetector = detector.GetRDetector();

    /*
      With the AugerPrime Upgrade, radio traces are part of the "normal" Sd data stream.
      To save ordinary (Sd) people from configuring the RDetector every time radio data is present,
      it is required that the RDetector is configured (in the first place) (therefore it is an intentional act)!
      To ensure that the AugerPrime Radio Detector is configured an not AERA,
      we require "AddStationListFromSManager" to be true. However in case of a simulated shower, this should not be done!
      When using the Offline file format with a radio simulation the REvent was here initialized unintentionally.
    */
    const bool readInRdData =
      HasRDetectorConfig() && rDetector.AddStationListFromSManager() && !oEvent.HasSimShower();

    if (readInRdData) {
      if (!oEvent.HasREvent())
        oEvent.MakeREvent();

      auto& rdEvent = oEvent.GetREvent();
      auto& rdHeader = rdEvent.GetHeader();

      if (!rdEvent.HasTrigger())
        rdEvent.MakeTrigger();
      rdEvent.GetTrigger().SetSDTrigger(true);  // always true for RD data

      // TH: for now: Radio Event ID and time is only set to SD values if there is not already an AERA event ID
      if (!rdHeader.GetId()) {
        rdHeader.SetId(rEvent.Id);
        rdHeader.SetTime(currentTime);
      }
    }

    // meteo information from CDAS >= v5r0
    if (!oSEvent.HasMeteo())
      oSEvent.MakeMeteo();

    if (rEvent.MeteoFlag) {

      const auto& rMeteo = rEvent.meteo();

      auto& oMeteo = oSEvent.GetMeteo();
      float pressures[kIoSd::N_WEATHER_STATIONS];
      float dailyPressures[kIoSd::N_WEATHER_STATIONS];

      for (unsigned int k = 0; k < kIoSd::N_WEATHER_STATIONS; ++k) {
        pressures[k] = rMeteo.Pressure[k]*hPa;
        dailyPressures[k] = rMeteo.DayPressure[k]*hPa;
      }

      oMeteo.SetPressures(pressures, kIoSd::N_WEATHER_STATIONS);
      oMeteo.SetTemperatures(rMeteo.Temperature, kIoSd::N_WEATHER_STATIONS);
      oMeteo.SetHumidities(rMeteo.Humidity, kIoSd::N_WEATHER_STATIONS);
      oMeteo.SetDayPressures(dailyPressures, kIoSd::N_WEATHER_STATIONS);
      oMeteo.SetDayTemperatures(rMeteo.DayTemperature, kIoSd::N_WEATHER_STATIONS);
      oMeteo.SetDayHumidities(rMeteo.DayHumidity, kIoSd::N_WEATHER_STATIONS);

    }

    const auto& rStations = rEvent.Stations;

    /*
      DV: *Problem of repeated stations*
      CDAS does not optimize T3 requests in any way and adds stations
      to the station vector in order the replies are received. In many
      conditions this results in the same station ID being added
      multiple times, some of the copies will actually contain data,
      some stations will just have multiple error codes reported.
      Merging makes this situation even worse since the stations
      might have already replied to CDAS when the SD-only T3 was
      requested so that the FD-T3 gets only eT3Already or eT3NotFound
      replies.
      Offline cannot hold multiple copies of the same station in its
      event structure. From all the multiple copies we thus have to
      either find the one with data or the one with most "usefull"
      error message. The enum in CDAS IoSdData.h is as follows:
        enum LSErrorTypes {
          eNoError, eT3Lost, eT3NotFound, eT3TooYoung,
          eT3Already, eT1Stopped, eBadCompress, eDataLost
        };
      This is ufortunately not the order of preference for inclusion
      of a station into the Offline event. Instead we want the
      hierarchy to be:
        eNoError > eT3Lost > eT3TooYoung > eT3Already > eT1Stopped >
          > eBadCompress > eDataLost > eT3NotFound
    */
    map<int, const IoSdStation*> rStationsSelected;
    {
      multimap<int, int> discardedStations;
      for (const auto& station : rStations) {
        const auto id = station.Id;
        const auto it = rStationsSelected.find(id);
        if (it == rStationsSelected.end()) {
          rStationsSelected.insert(make_pair(id, &station));
          continue;
        }
        auto& current = it->second;
        if (DuplicatePriority(station) > DuplicatePriority(*current)) {
          discardedStations.insert(make_pair(id, current->Error));
          current = &station;
        } else
          discardedStations.insert(make_pair(id, station.Error));
      }
      if (!discardedStations.empty()) {
        ostringstream info;
        info << "Discarded duplicated stations id(error):";
        for (const auto& ie : discardedStations)
          info << ' ' << ie.first << '(' << ie.second << ')';
        INFO(info);
      }
    }

    const auto& sDetector = detector.GetSDetector();
    set<int> missingDStations;
    int nConvertedStations = 0;

    for (const auto rStationIdPtr : rStationsSelected) {

      const int id = rStationIdPtr.first;
      const auto& rStation = *rStationIdPtr.second;

      try {
        sDetector.GetStation(id);
      } catch (utl::NonExistentComponentException&) {
        missingDStations.insert(id);
        continue;
      }
      auto& dStation = sDetector.GetStation(id);

      ++nConvertedStations;

      /*
        When a mismatch occurs between what hardware exists
        in the SDetector Station and what is seen in CDAS station,
        the SDetector Station is updated to reflect that which is
        seen in the CDAS station. This must be done before the
        SEvent Station is created as the SEvent Station constructor
        creates hardware which just mirrors what exists in the
        SDetector Station. Note that stations with a T3 error
        status of eNoError (i.e. 0 (256) for UBs (UUBs)) have the
        IsUUB member of the IoSdStation class correctly set, whereas
        those with a T3 error status of eT3NotFound (i.e. 2) do not.
        Thus, the hardware defined in the SDetector is only
        overwritten if the IoSdStation T3 error is 0 or 256 (meaning
        the station had a trigger and sent data to CDAS).
      */
      const bool isUUB = rStation.IsUUB;
      const int hasSSD = const_cast<IoSdStation&>(rStation).HasSSD();
      const bool noError = !GetError(rStation);

      if (noError) {

        if (dStation.IsUUB() != isUUB)
          sdet::Unlock(dStation).UpdateElectronics(isUUB);

        /*
          At present, EA UUB stations will return 0 for
          HasSSD() whether they have an SSD or not since
          HasSSD() just checks if one of large WCD PMT classes
          is filled with SSD E-Kit information. As such,
          we need to check both for a masked WCD PMT (used for SSD)
          or a dedicated SSD PMT.

          When reading in WCD+SSD UB simulations, the CDASToOffline
          converter will RemoveScintillator even if the SSD was
          simulated as in the IoSdStation in CDAS, HasSSD() will
          return false. It returns false because the TubeMask doesn't
          reflect the SSD PPA setup (where one WCD PMT is not okay as
          it is being used for the SSD PMT). Unless we want to
          task one PMT as not okay in WCD+SSD UB simulations and
          update the TubeMask in IoSdStation accordingly when
          streaming the Offline event to a raw CDAS event, I don't
          see a way the !oEvent.HasSimShower() check below. Once the
          IoSdStation class in CDAS has another way of checking if an
          SSD exists (other than the HasSSD() function, which also
          returns 0 when UUB stations have an SSD), this can be
          revisted.
        */
        if (hasSSD || isUUB) {
          if (!dStation.HasScintillator())
            sdet::Unlock(dStation).MakeScintillator();
        } else if (dStation.HasScintillator() && !oEvent.HasSimShower())
          sdet::Unlock(dStation).RemoveScintillator();

        // A SmallPMT should always be created if a station has a UUB
        if (isUUB) {
          if (!dStation.HasSmallPMT())
            sdet::Unlock(dStation).MakeSmallPMT();
        } else if (dStation.HasSmallPMT())
          sdet::Unlock(dStation).RemoveSmallPMT();

      }

      if (!oSEvent.HasStation(id))
        oSEvent.MakeStation(id);
      auto& oStation = oSEvent.GetStation(id);
      /*
        - IsUUB flag is only available for cdas-version >= v5r6
        - UFadc (get FADC traces for UUB stations) only available for cdas-version >= v5r8
        - Calibration info (Base3, Offset3, Shape3 ...) available for cdas-version >= v5r9
        - Latest updates were done in cdas release v6r0p1.
        code for the upgrade will not compile for old cdas versions.
      */

      // T2 Trigger
      if (!oStation.HasTriggerData())
        oStation.MakeTriggerData();
      auto& oStationTrigger = oStation.GetTriggerData();

      oStationTrigger.SetErrorCode(GetError(rStation));

      const auto& rStationTrigger = rStation.Trigger;
      oStationTrigger.SetWindowMicroSecond(rStationTrigger.Window);
      oStationTrigger.SetOffsetMicroSecond(rStationTrigger.Offset);
      if (!isUUB)
        oStationTrigger.SetPLDTrigger(rStationTrigger.Type);
      else {
        /* DV 2021-02-27: in cdas for UUB they do
            IoSdT2Trigger.Type1 = pld & 0xff00ff;
            IoSdT2Trigger.Type2 = (pld & 0xff00ff00) >> 8;
          here we are trying to undo this so that the lower 16 bits are
          the same as for UB */
        const StationTriggerData::PLDType pld =
          StationTriggerData::PLDType(rStationTrigger.Type1) |
          (StationTriggerData::PLDType(rStationTrigger.Type2) << 8);
        oStationTrigger.SetPLDTrigger(pld);
      }

      // T2Life info
      if (rStation.T2Life > -1 && rStation.T2Life120 > -1)
        oStation.SetT2Life(bool(rStation.T2Life) + bool(rStation.T2Life120));
      else
        oStation.SetT2Life(2);  // info not available, assume alive

      // Upgrade Trigger Parameter Information (UUB only)
      /*if (isUUB) {
        const IoUSdTrigParam* const rUTrigParam = rStation.UTrigParam;
        if (rUTrigParam) {
          // Placeholder
          DEBUGRAW("IoUSdTrigParam exists");
        }
      }*/

      // GPS
      const auto* const rGPS = rStation.Gps;

      if (rGPS) {
        DEBUGRAW(" has-GPS " << flush);

        if (!oStation.HasGPSData())
          oStation.MakeGPSData();

        auto& oGPS = oStation.GetGPSData();

        oGPS.SetSecond(rGPS->Second);
        oGPS.SetTick(rGPS->Tick);
        oGPS.SetCurrent100(rGPS->Current100);
        oGPS.SetNext100(rGPS->Next100);
        oGPS.SetCurrent40(rGPS->Current40);
        oGPS.SetNext40(rGPS->Next40);
        oGPS.SetPreviousST(rGPS->PreviousST);
        oGPS.SetCurrentST(rGPS->CurrentST);
        oGPS.SetNextST(rGPS->NextST);
        oGPS.SetOffset(rGPS->Offset);
        //oGPS.SetTickFall() is set below
      }

      /*
        Stations with Cyclone FE boards running Zbigniew Szadkowski's firmware:
        In years {2007, 2008, 2009} the stations with IDs {600, 610, 620, 627, 630, 648}
        were sending some telemetry data in the last 8 bins of the PMT traces. For
        these we have to set the IsCyclone status so that the calibration can avoid it.
      */
      // if (!isUUB &&
      //     Is(id).In(gCycloneStations) &&
      //     Is(UTCDateTime(currentTime).GetYear()).In(gCycloneYears))
      //   oStation.SetIsCyclone();

      // Calibration
      const auto* const rCalib = rStation.Calib;

      if (rCalib) {

        DEBUGRAW(" has-Calib ("
                 << "v" << rCalib->Version << ")"
                 << flush);

        if (!oStation.HasCalibData())
          oStation.MakeCalibData();
        auto& oStationCalib = oStation.GetCalibData();

        const auto& calibVersion = rCalib->Version;
        oStationCalib.SetVersion(calibVersion);

        if (rGPS) {
          oStation.GetGPSData().SetTickFall(
            // for calibVersion <= 12 just get false from (Tick == TickFall)
            (calibVersion > 12) ? rGPS->TickFall : !rGPS->Tick
          );
          ApplyTimeCorrection(oStation.GetGPSData());
          //oStation.GetGPSData().SetCorrectedNanosecond(rGPS->NanoSecond);
        }

        oStationCalib.SetStartSecond(rCalib->StartSecond);
        oStationCalib.SetEndSecond(rCalib->EndSecond);
        oStationCalib.SetNT1(rCalib->NbT1);
        oStationCalib.SetNT2(rCalib->NbT2);
        oStationCalib.SetNTot(rCalib->NbTOT);

        const unsigned int tubeMask = rCalib->TubeMask;
        int nTubesOK = 0;

        for (auto& oPMT : oStation.PMTsRange(sdet::PMTConstants::eAnyType)) {

          /*
            Comment on the loops over all PMTs and converting data from CDAS PMTs to Offline PMTs:

            CDAS iterates over PMTs with id = 0, 1, 2
            Offline iterates over PMTs with id = 1, 2, 3
            - In case of isUUB stations, 5 PMTs are created by the Offline station
              i.e. eWaterCherenkovSmall (id = 4) and eScintillator (id = 5)
            - In case of hasSSD stations (SSD connected to UB through kit box), 4 PMTs
              are created (3 eWaterCherenkovLarge + eScintillator). The flag hasSSD returns
              the id of the WCD PMT channel which is now used by the Scintillator PMT. Data
              of this PMT is accessed in CDAS by hasSSD-1, therefore we need to explicitly
              tell Offline which is the CDAS-PMT id that corresponds to eScintillator.

            The loop over PMTConstants::eAnyType with the logic below
            should handle all these situations.
          */

          int i = oPMT.GetId() - 1;
          if (hasSSD) {
            if (oPMT.GetType() == sdet::PMTConstants::eScintillator)
              i = hasSSD - 1;
            else if (i == hasSSD - 1)
              continue;
          }

          if (!oPMT.HasCalibData())
            oPMT.MakeCalibData();

          auto& oPMTCalib = oPMT.GetCalibData();
          // UUB information still missing in CDAS (v6r0)
          if (tubeMask & (1u << i)) {
            ++nTubesOK;
            oPMTCalib.SetIsTubeOk(true);
            oPMTCalib.SetIsLowGainOk(true);
          }

          /*
            For calibration version < 262, the UUB PMTs are all set as "not ok" in CDAS.
            In order to use data from these PMTs, here we overwrite this flag. This implies
            that for a calibration version < 262, there could be some UUB PMTs that are
            disfunctional but have IsTubeOk set to true.
          */
          if (isUUB && calibVersion < 262) {
            ++nTubesOK;
            oPMTCalib.SetIsTubeOk(true);
            oPMTCalib.SetIsLowGainOk(true);
            ostringstream warn;
            warn << "station " << oStation.GetId() << " has calibration version " << calibVersion << "; "
                    "PMT id " << oPMT.GetId() << " had flag IsTubeOK overwritten with \"true\"";
            WARNING(warn);
          }

          /*
            22-06-2022: For calibration versions 262 to 268, the SSD PMTs are set as "not ok" in CDAS.
            Here we also overwrite this flag for such PMTs so that we can analyse their data.
            Again this implies that, for the mentioned calibration versions, there could be some SSD PMTs
            that are disfunctional having their IsTubeOk as true.
          */
          if (isUUB &&
              oPMT.GetType() == sdet::PMTConstants::eScintillator &&
              262 <= calibVersion && calibVersion <= 268) {
            ++nTubesOK;
            oPMTCalib.SetIsTubeOk(true);
            oPMTCalib.SetIsLowGainOk(true);
            ostringstream warn;
            warn << "station " << oStation.GetId() << " has calibration version " << calibVersion << "; "
                    "SSD PMT flag IsTubeOK was overwritten with \"true\"";
            WARNING(warn);
          } else if (isUUB && oPMT.GetType() == sdet::PMTConstants::eScintillator) {
            ostringstream warn;
            warn << "station " << oStation.GetId() << " has calibration version " << calibVersion << "; "
                    "if SSD PMT trace is absent, its IsTubeOK flag is possibly set to \"false\" by CDAS";
            WARNING(warn);
          }

          /*
            2023-04-13: Here we assign nominal values for MIP Peak and charge.
            Since there is no data member for the raw SSD calibration data
            in IoSdCalib (see next comment too), we need to do it before the
            next "continue" statement.
          */
          auto& dPmt = dStation.GetPMT(oPMT.GetId());
          if (oPMT.GetType() == sdet::PMTConstants::eScintillator) {
            oPMTCalib.SetGainRatio(dPmt.GetGainRatio(), 0);
            oPMTCalib.SetOnlinePeak(dPmt.GetNominalVEMPeak());
            oPMTCalib.SetOnlineCharge(0 /*dPmt.GetNominalVEMCharge()*/);
          }

          /* Since the SmallPMT does not participate to the triggers
           * in any scenario, there is not masked status information.
           * Thus the IsTubeOk status is anyway turned to true in the
           * standard PMTCalibData member if the station is a UUB.
           * Then it will be changed during the reconstruction if no
           * SmallPMT is present (for instance on the array edge). */
          if (isUUB &&
              oPMT.GetType() == sdet::PMTConstants::eWaterCherenkovSmall) {
            oPMTCalib.SetIsTubeOk(true);
            oPMTCalib.SetIsLowGainOk(true);
          }

          /*
            January 17, 2020: arrays housing PMT-specific calibration information
            in the IoSdCalib class are of length kIoSd::NPMT (which = 3). Data
            members to house sPMT and SSD PMT related calibration data have
            not yet been been implemented. Therefore, for now, we continue
            after the data for the large WCD PMTs has been read in.
          */
          if (i >= int(kIoSd::NPMT))
            continue;

          // PMT calibration
          oPMTCalib.SetNumberTDA(rCalib->NbTDA[i]);
          oPMTCalib.SetEvolution(rCalib->Evolution[i]);
          oPMTCalib.SetRate(rCalib->Rate[i]);

          // The ordering of baseline data in the Base and SigmaBase
          // arrays differs between UB and UUB.
          if (isUUB) {
            oPMTCalib.SetBaseline(rCalib->Base[2*i+1], rCalib->SigmaBase[i], sdet::PMTConstants::eHighGain);
            oPMTCalib.SetBaseline(rCalib->Base[2*i], rCalib->SigmaBase[2*i], sdet::PMTConstants::eLowGain);
          } else {
            oPMTCalib.SetBaseline(rCalib->Base[i], rCalib->SigmaBase[i], sdet::PMTConstants::eHighGain);
            oPMTCalib.SetBaseline(rCalib->Base[i+3], rCalib->SigmaBase[i+3], sdet::PMTConstants::eLowGain);
          }

          /*
            For the early calibration versions of UUB, the online gain ratio, VEM peak
            and charge may be zero or hold some meaningless value. When this happens,
            we override these values with their nominal ones to avoid crashes.
          */
          if (rCalib->DA[i] > 0)
            oPMTCalib.SetGainRatio(rCalib->DA[i], rCalib->SigmaDA[i]);
          else
            oPMTCalib.SetGainRatio(dPmt.GetGainRatio(), 0);
          oPMTCalib.SetOnlinePeak(
            (rCalib->VemPeak[i] > 0) ? rCalib->VemPeak[i] : dPmt.GetNominalVEMPeak()
          );
          oPMTCalib.SetOnlineCharge(
            (rCalib->VemCharge[i] > 0) ? rCalib->VemCharge[i] : 0 /*dPmt.GetNominalVEMCharge()*/
          );

          /*
            For PPA stations, the SSD PMT has its IsTubeOK flag always set to false.
            To analyse data from these stations, we override this flag to true.
            This implies the possibility that a dysfunctional PPA SSD PMT end up with a true
            flag for IsTubeOK.
            Their online values for MIP peak and charge coming from CDAS are meaninless,
            therefore we set them to nominal values
          */
          if (!isUUB && oPMT.GetType() == sdet::PMTConstants::eScintillator) {
            oPMTCalib.SetIsTubeOk(true);
            oPMTCalib.SetIsLowGainOk(true);
            oPMTCalib.SetOnlinePeak(dPmt.GetNominalVEMPeak());
            oPMTCalib.SetOnlineCharge(0 /*dPmt.GetNominalVEMCharge()*/);

            ostringstream warn;
            warn << "PPA station " << oStation.GetId() << " had its SSD PMT "
                    "flag IsTubeOK overridden with \"true\" and its online MIP peak and charge "
                    "set to nominal values " << oPMTCalib.GetOnlinePeak() << " adc and "
                    << oPMTCalib.GetOnlineCharge() << " adct, respectively";
            WARNING(warn);
          }

          // remove zeroing when IoSdCalib empty ctor is fixed in CDAS
          // as of CDAS v3r4 this has not been fixed, yet, if ever...
          if (calibVersion > 12) {
            oPMTCalib.SetHighGainDelay(rCalib->DADt[i], rCalib->SigmaDADt[i]);
            oPMTCalib.SetHighGainDelayChi2(rCalib->DAChi2[i]);
          } else {
            oPMTCalib.SetHighGainDelay(0, 0);
            oPMTCalib.SetHighGainDelayChi2(0);
          }

        } // i loop PMTs
        oStationCalib.SetNTubesOk(nTubesOK);
      } // if station has calib

      // SmallPMT Calibration
      const auto* const rCalibSPMT = rStation.CalibSPMT;
      if (rCalibSPMT) {
        DEBUGRAW(" has-Calib-SPMT (v" << rCalibSPMT->Version << ')' << flush);

        if (!oStation.HasSmallPMTData()) {
          if (isUUB) {
            ostringstream warn;
            warn << "SmallPMTData does not exists for UUB station " << oStation.GetId()
                 << " but should have been created for every UUB!";
            WARNING(warn);
          }
        } else {

          auto& oSmallPMTData = oStation.GetSmallPMTData();
          oSmallPMTData.SetIsTubeOk(true);

          if (!oSmallPMTData.HasCalibData())
            oSmallPMTData.MakeCalibData();
          auto& oStationCalibSPMT = oSmallPMTData.GetCalibData();

          oStationCalibSPMT.SetVersion(rCalibSPMT->Version);
          // oStationCalibSPMT.SetStartEndGPSSecond(rCalibSPMT->StartGPSSecond, rCalibSPMT->EndGPSSecond);
          // oStationCalibSPMT.SetUptime(rCalibSPMT->Uptime);
          // oStationCalibSPMT.SetGoodEventsRatio(rCalibSPMT->GoodEventsRatio);
          oStationCalibSPMT.SetBeta(rCalibSPMT->Beta);
          oStationCalibSPMT.SetBetaError(rCalibSPMT->BetaUncertainty);
          for (unsigned int lpmt = 0; lpmt < kIoSd::NPMT; ++lpmt) {
            oStationCalibSPMT.SetBeta(rCalibSPMT->Beta_LPMT[lpmt], lpmt+1);
            oStationCalibSPMT.SetBetaError(rCalibSPMT->BetaUncertainty_LPMT[lpmt], lpmt+1);
          }

        }
      }

      // PMT quality
      const IoSdPMQuality* const rPMQuality = rStation.pmquality();
      if (rPMQuality) {
        const auto& version = rPMQuality->Version;
        const auto& tube = rPMQuality->TubeMask;
        const auto& anode = rPMQuality->AnodeMask;
        const auto& raining = rPMQuality->RainingMask;

        for (auto& oPMT : oStation.PMTsRange(sdet::PMTConstants::eAnyType)) {
          int i = oPMT.GetId() - 1;
          if (hasSSD) {
            if (oPMT.GetType() == sdet::PMTConstants::eScintillator)
              i = hasSSD - 1;
            else if (i == hasSSD - 1)
              continue;
          }
          if (!oPMT.HasQuality())
            oPMT.MakeQuality();
          auto& oPMTQuality = oPMT.GetQuality();
          oPMTQuality.SetVersion(version);
          const auto pmtMask = 1 << i;
          oPMTQuality.SetIsTubeOk(tube & pmtMask);
          oPMTQuality.SetHasAnode(anode & pmtMask);
          oPMTQuality.SetIsRaining(~raining & pmtMask);
        }
      }

      // Muon histograms
      const IoSdHisto* const rHisto = rStation.Histo;

      if (rHisto) {

        DEBUGRAW(" has-Histo " << flush);

        if (!oStation.HasCalibData())
          oStation.MakeCalibData();

        auto& oStationCalib = oStation.GetCalibData();

        const int calibVersion = oStationCalib.GetVersion();
        if (calibVersion > 8) {

          vector<int> mch(rHisto->Charge[3], rHisto->Charge[3] + 600);
          oStationCalib.SetMuonChargeHisto(mch, rHisto->Offset[9]);

          // NB_HISTO_CALIB comes from IoSdData.h
          // this has gone into corresponding PMTCalibData
          /*vector<int> offset(rHisto->Offset, rHisto->Offset + kIoSd::NB_HISTO_CALIB);
          oStationCalib.SetOffsetHisto(offset);*/

          for (auto& oPMT : oStation.PMTsRange(sdet::PMTConstants::eAnyType)) {

            int i = oPMT.GetId() - 1;
            if (hasSSD) {
              if (oPMT.GetType() == sdet::PMTConstants::eScintillator)
                i = hasSSD - 1;
              else if (i == hasSSD - 1)
                continue;
            }

            if (!oPMT.HasCalibData())
              oPMT.MakeCalibData();
            auto& oPMTCalib = oPMT.GetCalibData();

            if (oPMT.GetType() == sdet::PMTConstants::eWaterCherenkovSmall)
              continue; // skip small pmt until update in CDAS

            if (isUUB) {
              if (oPMT.GetType() == sdet::PMTConstants::eWaterCherenkovLarge) {
                const vector<int> base(rHisto->Base[i], rHisto->Base[i] + 20);
                oPMTCalib.SetMuonBaseHisto(base, rHisto->Offset[i]);

                const vector<int> shape(rHisto->UShape[i], rHisto->UShape[i] + 70);
                oPMTCalib.SetMuonShapeHisto(shape);

                const vector<int> peak(rHisto->Peak[i], rHisto->Peak[i] + 150);
                oPMTCalib.SetMuonPeakHisto(peak, rHisto->Offset[i+3]);

                const vector<int> charge(rHisto->Charge[i], rHisto->Charge[i] + 600);
                oPMTCalib.SetMuonChargeHisto(charge, rHisto->Offset[i+6]);

                // different treatment for ssd pmt in CDAS
              } else if (oPMT.GetType() == sdet::PMTConstants::eScintillator) {
                const vector<int> base(rHisto->Base3, rHisto->Base3 + 20);
                oPMTCalib.SetMuonBaseHisto(base, rHisto->Offset3[0]);

                const vector<int> shape(rHisto->UShape[3], rHisto->UShape[3] + 70);
                oPMTCalib.SetMuonShapeHisto(shape);

                const vector<int> peak(rHisto->Peak3, rHisto->Peak3 + 150);
                oPMTCalib.SetMuonPeakHisto(peak, rHisto->Offset3[1]);

                const vector<int> charge(rHisto->Charge[3], rHisto->Charge[3] + 600);
                oPMTCalib.SetMuonChargeHisto(charge, rHisto->Offset3[2]);
              }
            } else {
              const vector<int> base(rHisto->Base[i], rHisto->Base[i] + 20);
              oPMTCalib.SetMuonBaseHisto(base, rHisto->Offset[i]);

              const unsigned int shapeN = (calibVersion == 13) ? 19 : 20;
              const vector<int> shape(rHisto->Shape[i], rHisto->Shape[i] + shapeN);
              oPMTCalib.SetMuonShapeHisto(shape);

              const vector<int> peak(rHisto->Peak[i], rHisto->Peak[i] + 150);
              oPMTCalib.SetMuonPeakHisto(peak, rHisto->Offset[i+3]);

              const vector<int> charge(rHisto->Charge[i], rHisto->Charge[i] + 600);
              oPMTCalib.SetMuonChargeHisto(charge, rHisto->Offset[i+6]);
            }
          }
        }
      }

      // FADC traces
      if (!isUUB) {
        const IoSdFadc* const rFADC = rStation.Fadc;

        if (rFADC) {

          DEBUGRAW(" has-FADC " << flush);

          for (auto& oPMT : oStation.PMTsRange(sdet::PMTConstants::eAnyType)) {
            // PMTs always exist (otherwise something is seriously wrong)
            int i = oPMT.GetId() - 1;
            if (hasSSD) {
              if (oPMT.GetType() == sdet::PMTConstants::eScintillator)
                i = hasSSD - 1;
              else if (i == hasSSD - 1)
                continue;
            }
            const auto& trace = rFADC->Trace[i];
            // FADC traces
            if (!oPMT.HasFADCTrace()) {
              oPMT.MakeFADCTrace();
              oPMT.GetFADCTrace(sdet::PMTConstants::eHighGain).Adopt(trace[IoSdEvent::eHigh], kIoSd::MAXSAMPLE);
              oPMT.GetFADCTrace(sdet::PMTConstants::eLowGain).Adopt(trace[IoSdEvent::eLow], kIoSd::MAXSAMPLE);
            }
          }
          DEBUGRAW(endl);

        }
      } else {
        // different method to get FADC traces for the UUB
        if (rStation.UFadc) {
          const int nGain = 2;
          const int nSample = rStation.UFadc->NSample;
          DEBUGRAW(" upgraded station has-FADC " << flush);
          for (auto& oPMT : oStation.PMTsRange(sdet::PMTConstants::eAnyType)) {
            int i = oPMT.GetId() - 1;
            if (!oPMT.HasFADCTrace())
              oPMT.MakeFADCTrace();
            for (int j = 0; j < nGain; ++j) {
              unsigned short trace[nSample];
              for (int k = 0; k < nSample; ++k) {
                trace[k] = rStation.UFadc->GetValue(i, j, k);
              }
              oPMT.GetFADCTrace(
                (j == 0) ? sdet::PMTConstants::eHighGain : sdet::PMTConstants::eLowGain
              ).Adopt(trace, nSample);
            }
          }
          /*
           * For now revt::Station and rdet::Station are only and always created if
           * a station has a valid radio trace.
           * To check the existence of the radio traces in UUB stations
           * the lenght of the buffer memory of all the FADC is used.
           * If the CDAS developsers will introduce a new specific function for this
           * the following line has to be changed accordingly
           */
          if (readInRdData && rStation.UFadc->Traces.size() > 20480) {

            // data parity check
            const int nSample = rStation.UFadc->NSample;
            unsigned int totalParity = 0;
            for (int i = 0; i < nSample; ++i) {
              totalParity += rStation.UFadc->GetValueParity(5, 0, i);
              totalParity += rStation.UFadc->GetValueParity(5, 1, i);
            }
            const bool parityOkay = (totalParity == 4096);

            auto& rdEvent = oEvent.GetREvent();

            // Id of Rd station
            const unsigned int rdId = rDetector.GetRdSdStationIdLink() + id;

            // make revt::Station
            if (!rdEvent.HasStation(rdId))
              rdEvent.MakeStation(rdId);

            auto& rdStation = rdEvent.GetStation(rdId);
            if (!rdStation.HasTriggerData())
              rdStation.MakeTriggerData();

            if (!rdStation.HasGPSData())
              rdStation.MakeGPSData();

            // start-time is set at the beginning of the SD trace
            auto& gps = rdStation.GetGPSData();
            gps.SetSecond(rGPS->Second);
            gps.SetCorrectedNanosecond(oStation.GetGPSData().GetCorrectedNanosecond());

            const utl::TimeStamp ts(gps.GetSecond(), gps.GetCorrectedNanosecond());

            const double fadcBinSize = dStation.GetFADCBinSize();

            const auto& rdetStation = rDetector.GetStation(rdStation);

            /*
              - GetRdValue/GetValueParity only available for cdas-version >= v6r2p1.
              - code for the Rd upgrade will not compile for older cdas versions.
              - Rd iterates over Channel with id = 0, 1. Ch0 EW-aligned antenna, Ch1 NS-aligned antenna.
            */
            for (int chId = 0; chId < rdetStation.GetNChannels(); ++chId) {
              auto& chan = rdStation.GetChannel(chId);
              chan.SetActive();
              auto& trace = chan.GetChannelADCTimeSeries();

              const auto& rdetChannel = rdetStation.GetChannel(chId);
              const double samplingfreq = rdetChannel.GetSamplingFrequency();
              trace.SetBinning(1 / samplingfreq);

              unsigned int parityChannel = 0;
              for (int i = 0; i < nSample; ++i) {
                const double val = rStation.UFadc->GetValueRd(5, chId, i);
                trace.PushBack(val);
                parityChannel += rStation.UFadc->GetValueParity(5, chId, i);
              }
              chan.SetParity(parityChannel);
            }
            const TimeStamp traceTime = ts - TimeInterval(nSample * fadcBinSize);
            rdStation.SetRawTraceStartTime(traceTime);

            if (!parityOkay) {
              rdStation.SetExcludedReason(revt::eParityError);
              ostringstream err;
              err << "Wrong parity count for traces of RD station " << rdId
                  << ". Excluded it from reconstruction.";
              ERROR(err);
            }
          }
          DEBUGRAW(endl);
        }
      }
    }

    if (!missingDStations.empty()) {
      ostringstream warn;
      // warn << "Station" << String::Plural(missingDStations) << ' '
          //  << String::OfSortedIds(missingDStations)
        warn
           << " missing from the SD detector description, not including.";
      WARNING(warn);
    }

    oTrigger.SetNStations(nConvertedStations);

    DEBUGRAW(" PullEventRaw - IoSdEvent (END) " << endl);
  }


  // Push an evt::Event to CDAS IoSdEvent
  void
  operator>>(const Event& event, IoSdEvent& rawSEvent)
  {
    ConvertEventToIoSd(event, rawSEvent);
  }


  void
  ConvertEventToIoSd(const evt::Event& theEvent, IoSdEvent& rawSEvent)
  {
    DEBUGRAW(" PushEventRaw - IoSdEvent " << endl);

    if (!theEvent.HasSEvent()) {
      ERROR("Non-existent evt::SEvent class object.");
      return;
    }

    const SEvent& sEvent = theEvent.GetSEvent();

    if (!sEvent.HasTrigger()) {
      ERROR("Non-existent central trigger in event.");
      return;
    }

    rawSEvent.NumberOfErrorZeroStation = sEvent.GetNErrorZeroStations();

    const sevt::Header& header = sEvent.GetHeader();
    rawSEvent.Id = header.GetId();

    const sevt::EventTrigger& trigger = sEvent.GetTrigger();
    IoSdT3Trigger& reTrigger = rawSEvent.Trigger;

    reTrigger.Id = trigger.GetId();
    reTrigger.PreviousId = trigger.GetPreviousId();
    reTrigger.Second = trigger.GetTime().GetGPSSecond();
    reTrigger.MicroSecond = trigger.GetTime().GetGPSNanoSecond() / 1000;
    reTrigger.Sender = trigger.GetSender();
    reTrigger.Algo = trigger.GetAlgorithm();
    reTrigger.SDPAngle = trigger.GetSDPAngle();
    reTrigger.NumberOfStation = trigger.GetNStations();

    // Loop over the triggered stations in the event

    const ReferenceEllipsoid& e = ReferenceEllipsoid::GetWGS84();

    const sdet::SDetector& theSDetector = det::Detector::GetInstance().GetSDetector();

    const SEvent::ConstStationIterator stationsEnd = sEvent.StationsEnd();
    for (SEvent::ConstStationIterator csIt = sEvent.StationsBegin();
         csIt != stationsEnd; ++csIt) {

      const int sdId = csIt->GetId();
      const bool isUUB = csIt->IsUUB();

      IoSdStation rawStation;
      rawStation.Id = sdId;
      rawStation.IsUUB = isUUB;

      try {

        rawStation.Name = theSDetector.GetStation(csIt->GetId()).GetName();
        const UTMPoint pos =
          UTMPoint(theSDetector.GetStation(csIt->GetId()).GetPosition(), e);

        boost::tie(rawStation.Northing, rawStation.Easting, rawStation.Altitude) =
          pos.GetCoordinates3();

      } catch (NonExistentComponentException&) {

        // TODO: where should we take this info from?

        DEBUGRAW(" raw-station: " << setw(4) << rawStation.Id
                 << " is not available in Offline. ...  " << endl);

        //continue;

        rawStation.Name = "unkown by Offline";
        rawStation.Northing = 0;
        rawStation.Easting = 0;
        rawStation.Altitude = 0;

      }

      DEBUGRAW(" raw-station: " << setw(4) << rawStation.Id
               << " " << setw (20) << left << rawStation.Name);

      if (csIt->HasTriggerData()) {

        DEBUGRAW(" has-trig-data " << flush);

        const StationTriggerData& localTrigger = csIt->GetTriggerData();

        IoSdT2Trigger& rTrigger = rawStation.Trigger;

        rTrigger.Type = localTrigger.GetPLDTrigger();
        rTrigger.Offset = localTrigger.GetOffsetMicroSecond();
        rTrigger.Window = localTrigger.GetWindowMicroSecond();
        rTrigger.Name = localTrigger.GetAlgorithmName();
        {
          auto errorCode = localTrigger.GetErrorCode();
          // put back the UUB flag for error-zero stations
          if (isUUB)
            errorCode |= 0x100;
          rawStation.Error = errorCode;
        }

        DEBUGRAW(" Ecode " << rawStation.Error << flush);

        if (!localTrigger.GetErrorCode()) {

          IoSdFadc* const rfadc = rawStation.Fadc = new IoSdFadc;

          rfadc->NSample = theSDetector.GetStation(csIt->GetId()).GetFADCTraceLength();

          for (unsigned int i = 0; i < 3; ++i) {

            const PMT& pmt = csIt->GetPMT(i+1);

            if (pmt.HasFADCTrace()) {

              {
                Short_t* const rTraceHigh = rfadc->Trace[i][IoSdEvent::eHigh];
                const TraceI& traceHigh = pmt.GetFADCTrace(sdet::PMTConstants::eHighGain);
                const TraceI::ConstIterator traceHighEnd = traceHigh.End();
                unsigned int j = 0;
                for (TraceI::ConstIterator hIt = traceHigh.Begin();
                     hIt != traceHighEnd && j < kIoSd::MAXSAMPLE; ++hIt, ++j)
                  rTraceHigh[j] = Short_t(*hIt);
              }
              {
                Short_t* const rTraceLow = rfadc->Trace[i][IoSdEvent::eLow];
                const TraceI& traceLow = pmt.GetFADCTrace(sdet::PMTConstants::eLowGain);
                const TraceI::ConstIterator traceLowEnd = traceLow.End();
                unsigned int j = 0;
                for (TraceI::ConstIterator lIt = traceLow.Begin();
                     lIt != traceLowEnd && j < kIoSd::MAXSAMPLE; ++lIt, ++j)
                  rTraceLow[j] = Short_t(*lIt);
              }

            } // has FADC trace

          } // loop 3 PMTs

        } // if Error code == 0

        if (csIt->HasGPSData()) {

          DEBUGRAW(" has-gps-data " << flush);

          IoSdGps* const rgps = rawStation.Gps = new IoSdGps;

          const StationGPSData& gps = csIt->GetGPSData();

          rgps->Second = gps.GetSecond();
          rgps->Tick = gps.GetTick();
          rgps->Offset = gps.GetOffset();
          rgps->Current100 = gps.GetCurrent100();
          rgps->Next100 = gps.GetNext100();
          rgps->Current40 = gps.GetCurrent40();
          rgps->Next40 = gps.GetNext40();
          rgps->PreviousST = gps.GetPreviousST();
          rgps->CurrentST = gps.GetCurrentST();
          rgps->NextST = gps.GetNextST();
          rgps->NanoSecond = gps.GetCorrectedNanosecond();
          // let us assume the TickFall is correct inside offline
          rgps->TickFall = gps.GetTickFall();

        }

        if (csIt->HasCalibData()) {

          DEBUGRAW(" has-cal-data " << flush);

          // Station calibration data

          IoSdCalib* const rcalib = rawStation.Calib = new IoSdCalib;

          const StationCalibData& calib = csIt->GetCalibData();

          rcalib->Version = calib.GetVersion();
          rcalib->StartSecond = calib.GetStartSecond();
          rcalib->EndSecond = calib.GetEndSecond();
          rcalib->NbT1 = calib.GetNT1();
          rcalib->NbT2 = calib.GetNT2();
          rcalib->NbTOT = calib.GetNTot();
          rcalib->TubeMask = 0;

          for (unsigned int i = 0; i < kIoSd::NPMT; ++i) {

            // PMTs always exist (otherwise something is seriously wrong)
            const PMT& pmt = csIt->GetPMT(i+1);

            if (pmt.HasCalibData()) {

              const auto& pmtCalib = pmt.GetCalibData();

              if (pmtCalib.IsTubeOk())
                rcalib->TubeMask |= (1U << i);

              rcalib->NbTDA[i] = UShort_t(pmtCalib.GetNumberTDA());
              rcalib->Evolution[i] = pmtCalib.GetEvolution();
              rcalib->Rate[i] = pmtCalib.GetRate();
              rcalib->VemCharge[i] = pmtCalib.GetOnlineCharge();
              rcalib->VemPeak[i] = pmtCalib.GetOnlinePeak();
              rcalib->Base[i] = pmtCalib.GetBaseline();
              rcalib->Base[i+3] = pmtCalib.GetBaseline(sdet::PMTConstants::eLowGain);
              rcalib->SigmaBase[i] = pmtCalib.GetBaselineRMS();
              rcalib->SigmaBase[i+3] = pmtCalib.GetBaselineRMS(sdet::PMTConstants::eLowGain);
              rcalib->DA[i] = pmtCalib.GetGainRatio();
              rcalib->SigmaDA[i] = pmtCalib.GetGainRatioRMS();

              if (calib.GetVersion() > 12) {
                rcalib->DADt[i] = pmtCalib.GetHighGainDelay();
                rcalib->SigmaDADt[i] = pmtCalib.GetHighGainDelayRMS();
                rcalib->DAChi2[i] = pmtCalib.GetHighGainDelayChi2();
              } else {
                // this is for the broken IoSd ctor
                rcalib->DADt[i] = 0;
                rcalib->SigmaDADt[i] = 0;
                rcalib->DAChi2[i] = 0;
              }

              // non-persitent data member
              rcalib->TubeOk[i] = bool(rawStation.calib()->TubeMask & (1U << i));

            }

          }

          // non-persitent data member
          rcalib->NTubesOk = rcalib->TubeOk[0] + rcalib->TubeOk[1] + rcalib->TubeOk[2];

          // Muon histograms

          if (calib.GetVersion() > 8) {

            IoSdHisto* const rhisto = rawStation.Histo = new IoSdHisto;

            const vector<int>& mch = calib.GetMuonChargeHisto();
            ConditionalCopy(mch, rhisto->Charge[3], 600,
                            "Muon charge (sum)");
            rhisto->Offset[9] = calib.GetMuonChargeHistoOffset();

            // this is set from PMTCalibData now
            /*const vector<int>& offset = calib.GetOffsetHisto();
            ConditionalCopy(offset, rhisto->Offset, kIoSd::NB_HISTO_CALIB,
                            "Offset histogram");*/

            for (unsigned int i = 0; i < kIoSd::NPMT; ++i) {

              const PMT& pmt = csIt->GetPMT(i+1);

              if (pmt.HasCalibData()) {

                const PMTCalibData& pmtCalib = pmt.GetCalibData();

                const vector<int>& base = pmtCalib.GetMuonBaseHisto();
                ConditionalCopy(base, rhisto->Base[i], 20, "Muon base histogram");
                rhisto->Offset[i] = pmtCalib.GetMuonBaseHistoOffset();

                const vector<int>& shape = pmtCalib.GetMuonShapeHisto();
                ConditionalCopy(shape, rhisto->Shape[i], kIoSd::SINGLE_MUON_SIZE, "Muon shape histogram");

                const vector<int>& peak = pmtCalib.GetMuonPeakHisto();
                ConditionalCopy(peak, rhisto->Peak[i], 150, "Muon peak histogram");
                rhisto->Offset[i+3] = pmtCalib.GetMuonPeakHistoOffset();

                const vector<int>& mch = pmtCalib.GetMuonChargeHisto();
                ConditionalCopy(mch, rhisto->Charge[i], 600, "Muon charge histogram");
                rhisto->Offset[i+6] = pmtCalib.GetMuonChargeHistoOffset();

              }

            }

            if (isUUB) {

              const PMT& pmt = csIt->GetScintillatorPMT();

              if (pmt.HasCalibData()) {

                const PMTCalibData& pmtCalib = pmt.GetCalibData();

                const vector<int>& base = pmtCalib.GetMuonBaseHisto();
                ConditionalCopy(base, rhisto->Base3, 20, "Muon base histogram");
                rhisto->Offset3[0] = pmtCalib.GetMuonBaseHistoOffset();

                const vector<int>& shape = pmtCalib.GetMuonShapeHisto();
                ConditionalCopy(shape, rhisto->UShape[3], 70, "Muon shape histogram");

                const vector<int>& peak = pmtCalib.GetMuonPeakHisto();
                ConditionalCopy(peak, rhisto->Peak3, 150, "Muon peak histogram");
                rhisto->Offset3[1] = pmtCalib.GetMuonPeakHistoOffset();

                const vector<int>& mch = pmtCalib.GetMuonChargeHisto();
                ConditionalCopy(mch, rhisto->Charge[3], 600, "Muon charge histogram");
                rhisto->Offset3[2] = pmtCalib.GetMuonChargeHistoOffset();

              }
            }

          } // if version

        } // if has calib

        if (csIt->HasSmallPMTData()) {

          // Station SmallPMT inter-calibration data
          IoSdCalibSPMT* const rCalibSPMT = rawStation.CalibSPMT = new IoSdCalibSPMT;
          const auto& spmtData = csIt->GetSmallPMTData();

          if (spmtData.HasCalibData()) {
            DEBUGRAW("has-spmt-cal-data" << flush);

            const auto& calibSPMT = spmtData.GetCalibData();
            rCalibSPMT->Version = calibSPMT.GetVersion();
            // rCalibSPMT->StartGPSSecond = calibSPMT.GetStartGPSSecond();
            // rCalibSPMT->EndGPSSecond = calibSPMT.GetEndGPSSecond();
            // rCalibSPMT->Uptime = calibSPMT.GetUptime();
            // rCalibSPMT->GoodEventsRatio = calibSPMT.GetGoodEventsRatio();
            rCalibSPMT->Beta = calibSPMT.GetBeta();
            rCalibSPMT->BetaUncertainty = calibSPMT.GetBetaError();
            for (unsigned int lpmt = 0; lpmt < kIoSd::NPMT; ++lpmt) {
              rCalibSPMT->Beta_LPMT[lpmt] = calibSPMT.GetBeta(lpmt+1);
              rCalibSPMT->BetaUncertainty_LPMT[lpmt] = calibSPMT.GetBetaError(lpmt+1);
            }
          }
        }

        rawSEvent.Stations.push_back(rawStation);

      } // if has trigger data

      DEBUGRAW(endl);

    } // end station loop

    DEBUGRAW(" PushEventRaw - IoSdEvent (DONE)" << endl);
  }


  /// convert evt::Event to CDAS/Ec

  void
  operator>>(const Event& event, TEcEvent& ec)
  {
    DEBUGEC(" PushEventEc - TEcEvent" << endl);

    // first do the IoSd part
    event >> (IoSdEvent&)ec;
    ConvertEventToEc(event, ec);
  }


  void
  ConvertEventToEc(const evt::Event& theEvent, TEcEvent& theEc)
  {
    if (!theEvent.HasSEvent()) {
      ERROR ("Non-existent evt::SEvent class object.");
      return;
    }

    const SEvent& theSEvent = theEvent.GetSEvent();

    // set reference point: PampaAmarilla
    // all CDAS coordinates are relative to it
    CoordinateSystemPtr pampaCS = CoordinateSystemRegistry::Get("PampaAmarilla");

    const ReferenceEllipsoid& wgs84 = ReferenceEllipsoid::Get(ReferenceEllipsoid::eWGS84);

    const Point refPoint(0,0,0, pampaCS);
    const UTMPoint refPointUTM(refPoint, wgs84);
    const CoordinateSystemPtr refCS = LocalCoordinateSystem::Create(refPoint);
    theEc.fRefNorthing = refPointUTM.GetNorthing();
    theEc.fRefEasting = refPointUTM.GetEasting();

    DEBUGEC(" Reference Point: northing " << theEc.fRefNorthing
            << " easting " << theEc.fRefEasting << endl);

    // do the stations loop
    const SEvent::ConstStationIterator stationsEnd = theSEvent.StationsEnd();
    for (SEvent::ConstStationIterator csIt = theSEvent.StationsBegin();
         csIt != stationsEnd; ++csIt) {

      const unsigned int stationID = csIt->GetId();

      const sdet::Station& dStation =
        det::Detector::GetInstance().GetSDetector().GetStation(stationID);

      DEBUGEC(" cal-station: " << setw(4) << stationID);

      vector<IoSdStation>& rStations = theEc.Stations;
      // find the the raw station event from IoSdData
      vector<IoSdStation>::const_iterator iRawStation = rStations.begin();
      for ( ; iRawStation != rStations.end(); ++iRawStation)
        if (iRawStation->Id == stationID)
          break;

      if (iRawStation == rStations.end()) {
        ostringstream err;
        err << "Raw station " << stationID << " not found in raw event!"
               " DPA->CDAS conversion not possible!";
        ERROR(err);
        return; // -> TODO exception? Howto handle?
      }

      if (!csIt->HasCalibData() || !csIt->HasRecData()) {

        // also push silent/bad stations!
        // iRawStation.Error != 0 (not-calibrated)
        if (!iRawStation->Error) {
          ostringstream warn;
          warn << "Raw station " << stationID << " has error-code "
               << iRawStation->Error << " but is not calibrated!"
                  " (HasCalibData: " << csIt->HasCalibData() << ")"
                  " (HasRecData: " << csIt->HasRecData() << ')'
               << endl;
          WARNING (warn);
        }
        theEc.fCalibStations.push_back(*iRawStation);

        DEBUGEC("NO-cal-data. SKIP" << endl);

        continue;
      }

      // the DPA cal and rec station
      const auto& stationCalib = csIt->GetCalibData();
      const auto& stationRec = csIt->GetRecData();

      // create the cdas calibrated station
      TCalibStation cdasCal(*iRawStation);

      DEBUGEC("pos: north " << cdasCal.Northing/m
              << " east " << cdasCal.Easting/m
              << " alti " << cdasCal.Altitude/m << endl);

      // first fill the trivial stuff
      // station position
      const UTMPoint stationPosUTM(cdasCal.Northing,
                                   cdasCal.Easting,
                                   cdasCal.Altitude,
                                   19, 'H', wgs84);

      const Point stationPos = stationPosUTM.GetPoint();

      cdasCal.fRefSecond = theEc.Trigger.Second; // redundant info
      cdasCal.fX = stationPos.GetX(refCS)/m;
      cdasCal.fY = stationPos.GetY(refCS)/m;
      cdasCal.fZ = stationPos.GetZ(refCS)/m;
      const ReferenceEllipsoid::Triple latLonH =
        wgs84.PointToLatitudeLongitudeHeight(stationPos);
      cdasCal.fLatitude = latLonH.get<0>()/deg;
      cdasCal.fLongitude = latLonH.get<1>()/deg;

      int nSaturatedPMTs = 0;
      for (unsigned int iPMT = 0; iPMT < kIoSd::NPMT; ++iPMT) {

        if (!csIt->HasPMT(iPMT+1)) {
          ostringstream err;
          err << "Raw station " << stationID << " has no PMT #" << iPMT+1 << "!"
                 " CDAS event might be currupted!";
          ERROR(err);
          continue; // -> skip (TODO exception? its a real problem)
        }

        const PMT& pmt = csIt->GetPMT(iPMT+1);
        if (!pmt.HasCalibData() || !pmt.HasRecData()) {
          ostringstream err;
          err << "Raw station " << stationID << " PMT #" << iPMT+1
              << " has no calib/rec data!" ;
          ERROR(err);
          continue; // -> skip (TODO exception? its a real problem)
        }

        const PMTCalibData& pmtCal = pmt.GetCalibData();
        const PMTRecData& pmtRec = pmt.GetRecData();

        TCalibSdPmt& pmti = cdasCal.fPmt[iPMT];

        const int saturationValue = dStation.GetSaturationValue();

        int nBinsSaturatedHighGain = 0;
        int nBinsSaturatedLowGain = 0;

        if (pmtRec.HasVEMTrace()) { // get PMT total vem trace

          const TraceD& vemTrace = pmtRec.GetVEMTrace();
          const TraceI& lowTrace = pmt.GetFADCTrace(sdet::PMTConstants::eLowGain);
          const TraceI& highTrace = pmt.GetFADCTrace(sdet::PMTConstants::eHighGain);
          const unsigned int vemTraceSize = vemTrace.GetSize();

          for (unsigned int iTraceBin = 0; iTraceBin < kIoSd::MAXSAMPLE; ++iTraceBin) {

            pmti.fTrace[iTraceBin] = (iTraceBin < vemTraceSize) ? vemTrace[iTraceBin] : 0;

            if (lowTrace[iTraceBin] >= int(saturationValue))
              ++nBinsSaturatedLowGain;

            if (highTrace[iTraceBin] >= int(saturationValue))
              ++nBinsSaturatedHighGain;

          }

          // now fill the PMT's trace info
          // TODO: make PMT integration start/stop bin available
          const int start = stationRec.GetSignalStartSlot();
          const int end = stationRec.GetSignalEndSlot();
          pmti.fT90 = TraceAlgorithm::TimeAtRelativeSignalX(vemTrace, start, end, 90)/ns;
          pmti.fT70 = TraceAlgorithm::TimeAtRelativeSignalX(vemTrace, start, end, 70)/ns;
          pmti.fT50 = TraceAlgorithm::TimeAtRelativeSignalX(vemTrace, start, end, 50)/ns;
          pmti.fT10 = TraceAlgorithm::TimeAtRelativeSignalX(vemTrace, start, end, 10)/ns;
          pmti.fSigInVEM = pmtRec.GetTotalCharge();
          pmti.fPeakInVEM = pmtRec.GetPeakAmplitude();
          pmti.fStartIntegration = start;
          pmti.fEndIntegration = end;

        } // pmt has vem trace

        // no of saturated channels
        pmti.fHighGainSat = nBinsSaturatedHighGain;
        pmti.fLowGainSat = nBinsSaturatedLowGain;

        // 0: no cal, 1: rough, 2: online, 3: histo, 4: extra water info
        pmti.fCalibratedState = 3;  // TODO, non existing in DPA
        //pmti.fCalibratedState = 0;

        // from online cal
        pmti.fVemPeak = pmtCal.GetOnlinePeak();
        // from online cal or histos (best available)
        pmti.fVemCharge = pmtCal.GetOnlineCharge();

        // from offline calibration (FADC)
        pmti.fBaseline[IoSdEvent::eLow] = pmtCal.GetBaseline(sdet::PMTConstants::eLowGain);
        pmti.fBaselineSigma[IoSdEvent::eLow] = pmtCal.GetBaselineRMS(sdet::PMTConstants::eLowGain);
        pmti.fBaseline[IoSdEvent::eHigh] = pmtCal.GetBaseline(sdet::PMTConstants::eHighGain);
        pmti.fBaselineSigma[IoSdEvent::eHigh] = pmtCal.GetBaselineRMS(sdet::PMTConstants::eHighGain);

        // PMT
        pmti.fPmtBaselineSigma = 0;  // TODO, non existing in DPA
        //TraceAlgorithm::Mean(rawtrace, fFirstMeanSlot, fLastMeanSlot);

        pmti.fDA = pmtCal.GetGainRatio();

        /* USED FOR WATER LEVEL STUDIES, SKIPPED NOW (CDAS XB experimental)
        pmti.fPulseSlope = ;
        pmti.fHistSlope = ;
        pmti.fChi = ; */

      } // end loop PMTs

      // now fill the station's trace info
      if (csIt->HasVEMTrace()) {

        const int start = stationRec.GetSignalStartSlot();
        const int end = stationRec.GetSignalEndSlot();
        const TraceD& vemTrace = csIt->GetVEMTrace();

        cdasCal.fT90 = TraceAlgorithm::TimeAtRelativeSignalX(vemTrace, start, end, 90)/ns;
        cdasCal.fT70 = TraceAlgorithm::TimeAtRelativeSignalX(vemTrace, start, end, 70)/ns;
        cdasCal.fT50 = TraceAlgorithm::TimeAtRelativeSignalX(vemTrace, start, end, 50)/ns;
        cdasCal.fT10 = TraceAlgorithm::TimeAtRelativeSignalX(vemTrace, start, end, 10)/ns;

        cdasCal.fSigInVEM = stationRec.GetTotalSignal();
        cdasCal.fPeakInVEM = stationRec.GetPeakAmplitude();

        cdasCal.fStartIntegration = start;
        cdasCal.fEndIntegration = end;

      }

      /* NOT YET USED (experimental)
      TSignal signal;
      signal.fPMT = ;//pmtID;
      signal.fStart = ;
      signal.fEnd = ;
      signal.fSignal = ;
      signal.fPeak = ;
      signal.fNoise = ;
      cdasCal.fSignals.push_back(signal);
      cdasCal.fCasuals.push_back(signal); */

      // rest of the TCalibStation

      cdasCal.fPmTStatus = stationCalib.GetNTubesOk();  // good PMTs in tank
      cdasCal.fSat = nSaturatedPMTs;  // # of low gain saturated pmts

      // redundant
      cdasCal.fStartBin = cdasCal.fStartIntegration;

      cdasCal.fSeveralSignals = 0;  // NOT USED YET (CDAS experimental)

      /* THIS IS USED FOR RECONSTRUCTION, NOT FILLED HERE: PLACEHOLDERS (see TErEvent)
      cdasCal.fDist = ;
      cdasCal.fDcore = ;
      cdasCal.fCosZeta = ;
      cdasCal.fDt = ;
      cdasCal.fdt = ;
      cdasCal.fWgt = ; */

      theEc.fCalibStations.push_back(cdasCal);

    } // end loop calibrated stations

    theEc.fNCalibStations = theEc.fCalibStations.size();

    DEBUGEC(" PushEventEc - TEcEvent (DONE) " << endl);
  }


  // convert CDAS/Ec to evt::Event
  void
  operator<<(Event& event, const TEcEvent& ec)
  {
    DEBUGEC(" PullEventEc - TEcEvent " << endl);

    // first fill the IoSd part
    event << (IoSdEvent&)ec;
    ConvertEcToEvent(event, ec);
  }


  void
  ConvertEcToEvent(evt::Event& event, const TEcEvent& ec)
  {
    auto& oSEvent = event.GetSEvent();

    // set reference point: PampaAmarilla
    const auto& wgs84 = ReferenceEllipsoid::Get(ReferenceEllipsoid::eWGS84);
    const auto& pampaCS = CoordinateSystemRegistry::Get("PampaAmarilla");
    const Point refPoint(0,0,0, pampaCS);
    const double refHeight = wgs84.PointToLatitudeLongitudeHeight(refPoint).get<2>();
    const UTMPoint cdasRefPointUTM(ec.fRefNorthing, ec.fRefEasting, refHeight, 19, 'H', wgs84);

    const auto& cdasRefCS = LocalCoordinateSystem::Create(cdasRefPointUTM);

    DEBUGEC(" Reference Point: north/east/alt " << ec.fRefNorthing << '/'
            << ec.fRefEasting << '/' << refHeight << endl);

    bool firstMsg = true;

    for (auto iCDASCalStation = ec.fCalibStations.begin(); iCDASCalStation != ec.fCalibStations.end(); ++iCDASCalStation) {

      const unsigned int stationID = iCDASCalStation->id();
      const int errorCode = GetError(*iCDASCalStation);

      if (!oSEvent.HasStation(stationID)) {
        ostringstream err;
        err << "Missing station " << stationID << " in event!";
        ERROR(err);
        continue;
      }

      auto& oStation = oSEvent.GetStation(stationID);

      // skip station (it's a non-good/uncalibratable station)
      if (errorCode == 2 && iCDASCalStation->Trigger.Window) {
        oStation.SetSilent();
        continue;
      }

      if (!oStation.HasCalibData())
        oStation.MakeCalibData();

      auto& oStationCalib = oStation.GetCalibData();

      if (!oStation.HasGPSData()) {
        ERROR("Raw station with signal, but without GPS data !");
        oStation.MakeGPSData();
      }
      auto& gpsData = oStation.GetGPSData();

      if (oStation.HasRecData() && firstMsg) {
        ERROR ("The SEvent should be completely cleared before refilling!");
        firstMsg = false;
      } else
        oStation.MakeRecData();

      auto& oStationRec = oStation.GetRecData();

      DEBUGEC("cal-station: " << setw(4) << stationID);

      const auto& sDetector = det::Detector::GetInstance().GetSDetector();

      const unsigned int fadcTraceLength = sDetector.GetStation(stationID).GetFADCTraceLength();

      const double fadcBinSize = sDetector.GetStation(stationID).GetFADCBinSize();

      for (unsigned int iPMT = 0; iPMT < kIoSd::NPMT; ++iPMT) {

        if (!oStation.HasPMT(iPMT+1)) {
          ostringstream err;
          err << "PMT " << iPMT+1 << " missing in station " << stationID << "!";
          ERROR(err);
          continue;
        }
        auto& oPMT = oStation.GetPMT(iPMT+1);

        if (!oPMT.HasRecData())
          oPMT.MakeRecData();
        auto& oPMTRec = oPMT.GetRecData();

        if (!oPMTRec.HasVEMTrace())
          oPMTRec.MakeVEMTrace();
        auto& oPMTRecTrace = oPMTRec.GetVEMTrace();

        const auto& pmt = iCDASCalStation->fPmt[iPMT];

        // copy the trace
        oPMTRecTrace = TraceD(fadcTraceLength, fadcBinSize);

        for (unsigned int iTraceBin = 0; iTraceBin < kIoSd::MAXSAMPLE; ++iTraceBin) {
          if (iTraceBin < fadcTraceLength)
            oPMTRecTrace[iTraceBin] = pmt.fTrace[iTraceBin];
          else
            ERROR("Trace out of bounds!");
        }

        // now fill the PMT's trace info
        oPMTRec.SetT50(pmt.T50()*ns);

        // not available (identical to station start/end slot)
        //oPMTRec.SetSignalStartSlot(pmt.fStartIntegration)
        //oPMTRec.SetSignalEndSlot(pmt.fEndIntegration);
        oPMTRec.SetTotalCharge(pmt.fSigInVEM);
        oPMTRec.SetPeakAmplitude(pmt.fPeakInVEM);

        // number of saturated channels -> not supported ?
        // just a flag for high/low gain saturation
        oStation.SetHighGainSaturation(pmt.fHighGainSat);
        oStation.SetLowGainSaturation(pmt.fLowGainSat);

        // 0: no cal, 1: rough, 2: online, 3: histo, 4: extra water info
        //pmt.fCalibratedState = 3;  // TODO !!!!!! missing in DPA event.
        //pmt.fCalibratedState = 0;

        /* USED FOR WATER LEVEL STUDIES, SKIPPED NOW  (CDAS experimental)
        pmt.fPulseSlope = ;
        pmt.fHistSlope = ;
        pmt.fChi = ; */

      } // end loop PMTs

      // copy the total station trace
      TraceD sumTrace(fadcTraceLength, fadcBinSize);

      for (unsigned int iTraceBin = 0; iTraceBin < kIoSd::MAXSAMPLE; ++iTraceBin) {
        if (iTraceBin < fadcTraceLength) {
          sumTrace[iTraceBin] =
            iCDASCalStation->fPmt[0].fTrace[iTraceBin] +
            iCDASCalStation->fPmt[1].fTrace[iTraceBin] +
            iCDASCalStation->fPmt[2].fTrace[iTraceBin];
        } else
          ERROR("Trace out of bounds!");
      }

      if (!oStation.HasVEMTrace())
        oStation.MakeVEMTrace();
      oStation.GetVEMTrace() = sumTrace;

      // timing of the trace END
      const TimeStamp gpsTime(gpsData.GetSecond(), gpsData.GetCorrectedNanosecond());

      DEBUGEC("bin: " << iCDASCalStation->fStartIntegration
              << " sec " << gpsTime.GetGPSSecond()
              << " nsec " << gpsTime.GetNanoSecond() << flush);

      // timing of the trace BEGINNING
      const TimeStamp traceTime = gpsTime - TimeInterval(fadcTraceLength * fadcBinSize);

      oStation.SetTraceStartTime(traceTime);

      const TimeStamp signalTime = traceTime +
        TimeInterval((iCDASCalStation->fStartIntegration - 0.5) * fadcBinSize);

      oStationRec.SetSignalStartTime(signalTime);

      // now fill the station's trace info
      oStationRec.SetSignalStartSlot(iCDASCalStation->fStartIntegration);
      oStationRec.SetSignalEndSlot(iCDASCalStation->fEndIntegration);

      oStationRec.SetT50(iCDASCalStation->T50()*ns, 0.);
      oStationRec.SetTotalSignal(iCDASCalStation->fSigInVEM);
      oStationRec.SetPeakAmplitude(iCDASCalStation->fPeakInVEM);

      oStationCalib.SetNTubesOk(iCDASCalStation->fPmTStatus);

      // redundant: iCDASCalStation->fStartBin

      DEBUGEC("signal " << iCDASCalStation->fSigInVEM
              << " peak " << iCDASCalStation->fPeakInVEM
              << " t50 " << iCDASCalStation->T50()*ns
              << " start " << iCDASCalStation->fStartIntegration
              << " end " << iCDASCalStation->fEndIntegration
              << flush);

      /* NOT YET USED (experimental)
      TSignal signal;
      signal.fPMT = ;//pmtID;
      signal.fStart = ;
      signal.fEnd = ;
      signal.fSignal = ;
      signal.fPeak = ;
      signal.fNoise = ;
      cdasCal.fSignals.push_back (signal);
      cdasCal.fCasuals.push_back (signal); */

      DEBUGEC(endl);

    } // end loop calibrated stations

    DEBUGEC(" PullEventEc - TEcEvent (END) " << endl);
  }


  /// convert evt::Event to CDAS/Es
  void
  operator>>(const Event& event, TEsEvent& es)
  {
    DEBUGES(" PushEventEs - TEsEvent " << endl);

    // first do the Ec part
    event >> (TEcEvent&)es;
    ConvertEventToEs(event, es);
  }


  void
  ConvertEventToEs(const evt::Event& event, TEsEvent& es)
  {
    if (!event.HasSEvent()) {
      ERROR ("Non-existent evt::SEvent class object.");
      return;
    }
    const auto& oSEvent = event.GetSEvent();

    auto& calStations = es.fCalibStations;
    auto& randomStations = es.fRandomStations;
    auto& selectedStations = es.fSelectedStations;
    auto& silentStations = es.fSilentStations;
    auto& badStations = es.fBadStations;

    for (auto osIt = oSEvent.StationsBegin(), end = oSEvent.StationsEnd(); osIt != end; ++osIt) {

      const unsigned int stationID = osIt->GetId();

      DEBUGES("sel-station: " << setw(4) << stationID);

      // get the corresponding TCalibStation
      auto csIt = calStations.begin();
      for ( ; csIt != calStations.end(); ++csIt)
        if (csIt->Id == stationID)
          break;

      if (csIt == calStations.end()) {
        ostringstream err;
        err << "Raw station " << stationID << " "
               "not found in cdas event (accidental)! "
               "DPA->CDAS conversion may not be possible!";
        ERROR(err);
      } else {
        if (osIt->IsRejected()) {
          randomStations.push_back(&(*csIt));
          DEBUGES(" accidental" << endl);
        } else if (osIt->IsCandidate()) {
          selectedStations.push_back(&(*csIt));
          DEBUGES(" candidate" << endl);
        } else if (osIt->IsSilent()) {
          silentStations.push_back(&(*csIt));
          DEBUGES(" silent" << endl);
        } else {
          // if we reach to this point, the station must be bad!
          badStations.push_back(&(*csIt));
          DEBUGES(" bad" << endl);
        }
      }

    } // end station loop

    // mark the seed triangle
    if (!event.HasRecShower() || !event.GetRecShower().HasSRecShower()) {
      ERROR("No Seed triangle available!");
      return; // exception ?
    }

    const auto& showerRec = event.GetRecShower();
    const auto& showerSRec = showerRec.GetSRecShower();
    const auto& seed = showerSRec.GetReconstructionSeed();

    DEBUGES("Mark SEED stations: ");

    for (auto seedIt = seed.begin(); seedIt != seed.end(); ++seedIt) {

      // get the corresponding TCalibStation
      auto sIt = calStations.begin();
      for ( ; sIt != calStations.end(); ++sIt)
        if (int(sIt->Id) == *seedIt)
          break;

      if (sIt == calStations.end()) {
        ostringstream err;
        err << "Calib station " << *seedIt
            << " not found in cdas event!"
               " DPA->CDAS conversion not possible!";
        ERROR(err);
        continue;
      }

      sIt->Trigger.Type |= 0x400; // set bit20

      DEBUGES(" " << *seedIt);

    } // end loop seed-stations

    DEBUGES(endl);

    /* No SEED IN CDAS ES YET, just internal stuff (experimental)
    TSeed cdasSeed(theEs);
    cdasSeed.fStations.push_back(&(*iCalStation));
    } // enf of the seed id loop
    cdasSeed.fValid = true;
    cdasSeed.fVem = 00000;
    cdasSeed.fTot = 00000;
    cdasSeed.fNbCompat = 00000; */

    // set reference point: PampaAmarilla
    const auto& cdasRefCS = CoordinateSystemRegistry::Get("PampaAmarilla");

    // fill the estimated shower reconstruction
    const auto& axis = showerSRec.GetSeedAxis();
    const auto& core = showerSRec.GetSeedBarycenter();

    auto& estimated = es.fEstimated;

    estimated.fGPSSecond = es.Trigger.Second; // more redundancy
    estimated.fU = sin(axis.GetTheta(cdasRefCS)) * cos(axis.GetPhi(cdasRefCS));
    estimated.fV = sin(axis.GetTheta(cdasRefCS)) * sin(axis.GetPhi(cdasRefCS));
    estimated.fXCore = core.GetX(cdasRefCS)/m;
    estimated.fYCore = core.GetY(cdasRefCS)/m;
    estimated.fZCore = core.GetZ(cdasRefCS)/m; // not used in CDAS ...

    DEBUGES(" PushEventEs - TEsEvent (END) " << endl);
  }


  /// convert CDAS/Es to evt::Event
  void
  operator<<(Event& event, const TEsEvent& es)
  {
    DEBUGES("PullEventEs - TEsEvent" << endl);

    event << (TEcEvent&)es;
    ConvertEsToEvent(event, es);
  }


  void
  ConvertEsToEvent(evt::Event& event, const TEsEvent& es)
  {
    auto& oSEvent = event.GetSEvent();

    /* tp
    // remove lonely stations
    theEs.RemoveLonelyStations();
    // end tp */

    vector<int> seed;

    // set candidates
    const auto& selectedStations = es.fSelectedStations;
    for (auto sIt = selectedStations.begin(); sIt != selectedStations.end(); ++sIt) {

      const auto* const cStat = *sIt;
      const unsigned int sId = cStat->id();

      DEBUGES("sel-station: " << setw (4) << sId);

      // check for beeing part of seed triangle
      if (cStat->Trigger.Type & 0x400) {
        seed.push_back(sId);
        DEBUGES(" seed" << flush);
      }

      if (!oSEvent.HasStation(sId)) {
        ERROR("SelectedStations: Can't use Event Selection with DPA Event not completely set.");
      } else {
        oSEvent.GetStation(sId).SetCandidate();
        DEBUGES(" candidate" << endl);
      }

    }

    // set accidental (fRandomStations in Es)
    for (const auto rs : es.fRandomStations) {

      const unsigned int sId = rs->id();

      DEBUGES("sel-station: " << setw (4) << sId);

      if (!oSEvent.HasStation(sId)) {
        ERROR("RandomStations: Can't use Event Selection with DPA Event not completely set.");
      } else {
        oSEvent.GetStation(sId).SetRejected(StationConstants::eRandom);
        DEBUGES(" accidental" << endl);
      }

    }

    // set accidental (fBadStations in Es)
    for (const auto bs : es.fBadStations) {

      const unsigned int sId = bs->id();

      DEBUGES("sel-station: " << setw (4) << sId << " bad" << endl);

      if (oSEvent.HasStation(sId))
        oSEvent.GetStation(sId).SetSilent();

    }

    // set silent
    for (const auto ss : es.fSilentStations) {

      const unsigned int sId = ss->id();

      DEBUGES("sel-station: " << setw(4) << sId);

      if (!oSEvent.HasStation(sId)) {
        ERROR("SilentStations: Can't use Event Selection with DPA Event not completely set.");
      } else {
        oSEvent.GetStation(sId).SetSilent();

        DEBUGES(" silent" << endl);
      }

    }

    if (!event.HasRecShower())
      event.MakeRecShower();

    auto& showerRec = event.GetRecShower();

    if (!showerRec.HasSRecShower())
      showerRec.MakeSRecShower();

    auto& srecData = showerRec.GetSRecShower();

    if (seed.size() != 3U)
      WARNING("Seed size != 3.");
    srecData.SetReconstructionSeed(seed);

    srecData.SetT4Trigger(const_cast<TEsEvent&>(es).IsT4());
    srecData.SetT5Trigger(const_cast<TEsEvent&>(es).IsT5());

    // set reference point: PampaAmarilla
    const auto& cdasRefCS = CoordinateSystemRegistry::Get("PampaAmarilla");

    const auto& estimated = es.fEstimated;

    const double u = estimated.fU;  // sintheta*cosphi
    const double v = estimated.fV;  // sintheta*sinphi
    const double w = sqrt(1 - Sqr(u) - Sqr(v));

    const Point core(estimated.fXCore*m,
                     estimated.fYCore*m,
                     estimated.fZCore*m, cdasRefCS);
    const Vector axis(u, v, w, cdasRefCS);

    srecData.SetSeedAxis(axis);
    srecData.SetSeedBarycenter(core);

    DEBUGES("PullEventEs - TEsEvent (END) " << endl);
  }


  /// convert evt::Event to CDAS/Er

  void
  operator>>(const Event& event, TErEvent& er)
  {
    DEBUGER(" PushEventEr - TErEvent " << endl);

    // first do the Es part
    event >> (TEsEvent&)er;
    ConvertEventToEr(event, er);
  }


  void
  ConvertEventToEr(const evt::Event& theEvent, TErEvent& theEr)
  {
    if (!theEvent.HasRecShower() || !theEvent.GetRecShower().HasSRecShower())
      return;

    const auto& siteCS = CoordinateSystemRegistry::Get("PampaAmarilla");

    /* if (!theEvent.HasSEvent()) {
      ERROR("Non-existent evt::SEvent class object.");
      return;
    }
    const sevt::SEvent& oSEvent = theEvent.GetSEvent(); */

    const auto& showerRec = theEvent.GetRecShower();
    const auto& showerSRec = showerRec.GetSRecShower();

    // now fill the TShowerParams part
    const auto& time = showerSRec.GetCoreTime();

    TShowerParams& cRec = theEr.fRec;

    cRec.fT0 = time.GetGPSNanoSecond();
    cRec.fGPSSecond = time.GetGPSSecond();

    const auto& core = showerSRec.GetCorePosition();
    const auto& axis = showerSRec.GetAxis();
    const auto& localCS = LocalCoordinateSystem::Create(core);

    cRec.fU = axis.GetX(localCS);
    cRec.fV = axis.GetY(localCS);

    cRec.fXCore = core.GetX(siteCS)/m;
    cRec.fYCore = core.GetY(siteCS)/m;
    cRec.fZCore = core.GetZ(siteCS)/m;

    cRec.fSRef = showerSRec.GetShowerSize();
    switch (showerSRec.GetShowerSizeType()) {
    case ShowerSRecData::eS1000:
      cRec.fRefDist = 1000;
      break;
    case ShowerSRecData::eS450:
      cRec.fRefDist = 450;
      break;
    default:
      ostringstream msg;
      msg << "Could not determine the reference distance for shower size."
             "ShowerSizeType = " << showerSRec.GetShowerSizeType();
      WARNING(msg);
      // I don't really know what to do in this situation, but ...
      cRec.fRefDist = 0;
    }

    double xMax = 0;
    double xMaxError = 0;
    if (showerRec.HasFRecShower() &&
        showerRec.GetFRecShower().HasGHParameters()) {
      const VGaisserHillasParameter& gh = showerRec.GetFRecShower().GetGHParameters();
      xMax = gh.GetXMax();
      xMaxError = gh.GetXMaxError();
    }

    cRec.fXmax = xMax/(g/cm2);

    const double curv = showerSRec.GetCurvature()*m;
    cRec.fR = (curv ? 1/curv : 0);

    // Last fit information
    cRec.fChi2 = showerSRec.GetLDFChi2();
    cRec.frChi2 = showerSRec.GetLDFChi2()/showerSRec.GetLDFNdof();
    cRec.fddl = showerSRec.GetLDFNdof();

    // Relating to the ldf
    cRec.fBeta = showerSRec.GetBeta();
    cRec.fGamma = showerSRec.GetGamma();

    // Energy
    cRec.fE = showerSRec.GetEnergy()/EeV;
    cRec.fdE = showerSRec.GetEnergyError()/EeV;

    // covariance matric
    for (int i = 0; i < 8; ++i)
      for (int j = 0; j < 8; ++j)
        cRec.fVarCov[i][j] = 0;

    const double theta = axis.GetTheta(localCS);
    const double sinTheta = sin(theta);
    const double cosTheta = cos(theta);
    const double phi = axis.GetPhi(localCS);
    const double sinPhi = sin(phi);
    const double cosPhi = cos(phi);

    cRec.fVarCov[0][0] = Sqr(showerSRec.GetCoreTimeError().GetNanoSecond());  // T0
    cRec.fVarCov[1][1] = Sqr(showerSRec.GetThetaError() * cosTheta * cosPhi) -
      Sqr(showerSRec.GetPhiError() * sinTheta * sinPhi);  // u
    cRec.fVarCov[2][2] = Sqr(showerSRec.GetThetaError() * cosTheta * sinPhi) +
      Sqr(showerSRec.GetPhiError() * sinTheta * cosPhi);  // v
    cRec.fVarCov[3][3] = Sqr(showerSRec.GetCoreError().GetX(localCS)/m);  // x
    cRec.fVarCov[4][4] = Sqr(showerSRec.GetCoreError().GetY(localCS)/m);  // y
    cRec.fVarCov[5][5] = Sqr(showerSRec.GetShowerSizeError());  //s1000
    cRec.fVarCov[6][6] = Sqr(xMaxError/(g/cm2));  // xmax
    cRec.fVarCov[7][7] = Sqr(showerSRec.GetCurvatureError()/m);  //R

    DEBUGER("PushEventEr - TErEvent (END) " << endl);
  }


  /// convert CDAS/Er to evt::Event
  void
  operator<<(Event& event, const TErEvent& er)
  {
    DEBUGER(" PullEventEr - TErEvent " << endl);

    event << (TEsEvent&)er;
    ConvertErToEvent(event, er);
  }


  void
  ConvertErToEvent(evt::Event& event, const TErEvent& er)
  {
    const auto& pampaCS = CoordinateSystemRegistry::Get("PampaAmarilla");

    if (!event.HasRecShower())
      event.MakeRecShower();
    auto& recData = event.GetRecShower();

    if (!recData.HasSRecShower())
      recData.MakeSRecShower();
    auto& srecData = recData.GetSRecShower();

    if (!recData.HasFRecShower())
      recData.MakeFRecShower();
    auto& frecData = recData.GetFRecShower();

    const auto& cRec = er.fRec;

    // get the errors out of the covariance matrix
    const double nsecError = sqrt(cRec.fVarCov[0][0])*ns;
    //const double uError = sqrt(cRec.fVarCov[1][1]);
    //const double vError = sqrt(cRec.fVarCov[2][2]);
    const double xError = sqrt(cRec.fVarCov[3][3])*m;
    const double yError = sqrt(cRec.fVarCov[4][4])*m;
    //const double s1000Error = sqrt(cRec.fVarCov[5][5]);
    const double xMaxError = sqrt(cRec.fVarCov[6][6])*g/cm2;
    const double rcError = sqrt(cRec.fVarCov[7][7])*m;

    GaisserHillas2Parameter gh2;
    gh2.SetXMax(cRec.fXmax*g/cm2, xMaxError);
    if (!frecData.HasGHParameters())
      frecData.MakeGHParameters(gh2);
    else
      frecData.GetGHParameters() = gh2;

    const TimeStamp coreTime =
      TimeStamp((unsigned int)(cRec.fGPSSecond),
                (unsigned int)(cRec.fT0));
    const TimeInterval coreTimeErr(nsecError*ns);
    srecData.SetCoreTime(coreTime, coreTimeErr);

    const Point core(cRec.fXCore*m,
                     cRec.fYCore*m,
                     cRec.fZCore*m, pampaCS);
    const Vector coreError(xError, yError, 0, pampaCS);
    const auto& localCS = LocalCoordinateSystem::Create(core);

    const double u = cRec.fU;  // sintheta*cosphi
    const double v = cRec.fV;  // sintheta*sinphi
    const double w = sqrt(1 - Sqr(u) - Sqr(v));
    const Vector axis(u,v,w, localCS);

    // TODO: do error propagation
    const double thetaError = 0;
    const double phiError = 0;

    srecData.SetAxis(axis);
    srecData.SetCorePosition(core);
    srecData.SetCoreError(coreError);
    srecData.SetThetaError(thetaError);
    srecData.SetPhiError(phiError);

    srecData.SetShowerSize(cRec.fSRef, cRec.dSRef());
    if (Test<CloseTo>(cRec.fRefDist, 450*m))
      srecData.SetShowerSizeType(ShowerSRecData::eS450);
    else if (Test<CloseTo>(cRec.fRefDist, 1000*m))
      srecData.SetShowerSizeType(ShowerSRecData::eS1000);
    else {
      ostringstream msg;
      msg << "Could not determine the ShowerSizeType "
             "CDAS fRefDist = " << cRec.fRefDist << endl;
      srecData.SetShowerSizeType(ShowerSRecData::eUndefined);
    }

    if (cRec.fR)
      srecData.SetCurvature(1/(cRec.fR*m), rcError);
    else
      srecData.SetCurvature(0, 0);

    // Last fit information
    srecData.SetLDFChi2(cRec.fChi2, cRec.fddl);

    // Relating to the ldf
    const double betaError = -1;
    const double gammaError = -1;
    srecData.SetBeta(cRec.fBeta, betaError);
    srecData.SetGamma(cRec.fGamma, gammaError);

    // Energy
    srecData.SetEnergy(cRec.fE*EeV, cRec.fdE*EeV);

    // Number of Stations used in LDF fit?
    srecData.SetNumberOfActiveStations(int(cRec.fddl));

    // This stuff will never be part of an 'official' CDAS realease, but it was
    // existing for some time in CDAS v4r4pxx
    if (!event.HasSEvent())
      event.MakeSEvent();

    auto& sevent = event.GetSEvent();

    // loop all stations and get StationRecData
    const auto& calibStations = er.fCalibStations;
    for (auto sIt = calibStations.begin(); sIt != calibStations.end(); ++sIt) {

      const unsigned int sId = sIt->id();

      if (!sevent.HasStation(sId)) {
        ostringstream err;
        err << "Missing station " << sId << " in event!";
        ERROR(err);
        continue;
      }

      auto& oStation = sevent.GetStation(sId);

      if (!oStation.HasRecData())
        oStation.MakeRecData();

      auto& oStationRec = oStation.GetRecData();

      oStationRec.SetSPDistance(sIt->fDist, 0);
      //cdasCal.fDcore = ;   // not avail. in DPA
      //cdasCal.fCosZeta = ; // not clear
      oStationRec.SetResidual(sIt->fDt);
      oStationRec.SetLDFResidual(sIt->fDt);
      //cdasCal.fdt = ;      // unclear
      //cdasCal.fWgt = ;     // only internal use ?

    }

    DEBUGER("PullEventEr - TErEvent (END) " << endl);
  }


  //From CDAS to Offline
  void
  operator<<(evt::Event& event, const md::Event& rawEvent)
  {
    DEBUGRAW(" PullEventRaw - md::Event " << endl);
    ConvertIoMdToEvent(event, rawEvent);
  }


  void
  ConvertIoMdToEvent(evt::Event& oEvent, const md::Event& rEvent)
  {
    // if no MDetector is configured do nothing
    if (!HasMDetectorConfig(rEvent.HasCounters()))
      return;

    ostringstream msg;

    if (!oEvent.HasMEvent())
      oEvent.MakeMEvent();

    auto& omEvent = oEvent.GetMEvent();
    if (rEvent.HasCounters()) {
      msg.str("");
      msg << "Number of Counters: " << rEvent.CountersSize();
      INFO(msg);
    } else
      return;

    /* md trigger from real event (CDAS) */
    const auto& rTrigger = rEvent.GetTrigger();
    const unsigned int t3Sec = rTrigger.GetT3Sec();
    const unsigned int t3NanoSec = int(rTrigger.GetT3Mic() * 1000);
    const auto mdId = rTrigger.GetId();
    const auto t3algorithm = rTrigger.GetT3Algorithm();

    const TimeStamp mdT3GPS(t3Sec, t3NanoSec);

    /* mevt header (Offline) */
    auto& omHeader = omEvent.GetHeader();
    omHeader.SetId(mdId);
    omHeader.SetTime(mdT3GPS);  // by construction, sould be identical to currentTime of SD
    /* mevt trigger (Offline) */
    auto& omTrigger = omHeader.GetTrigger();
    omTrigger.SetAlgorithm(t3algorithm);

    ostringstream idss;
    /* evt header (append md id to the end of the string) (Offline) */
    auto& oHeader = oEvent.GetHeader();
    if (!oHeader.GetId().empty())
      idss << oHeader.GetId() << "__";
    idss << "md_" << mdId;

    oHeader.SetTime(mdT3GPS);
    oHeader.SetId(idss.str());

    const auto& mDetector = det::Detector::GetInstance().GetMDetector();

    // Loop on CDAS data and creates/fill Offline structure
    for (auto cIt = rEvent.CountersBegin(), end = rEvent.CountersEnd(); cIt != end; ++cIt) {

      const auto& mdCter = **cIt;
      const int cId = mdCter.GetId();
      const unsigned int t3delay = mdCter.GetT3Delay(); // if -1 T3NotFound on-ground
      const size_t mdWindow = mdCter.GetWindow();

      int sdError = -2;  // -2 No SD station, -1 No SD trigger data, otherwise has the same codification of SDError [from 0 to 7]
      int sdWindow = -1;
      // To check SD station error (not available in MEvent data)
      const auto& oSEvent = oEvent.GetSEvent();
      if (oSEvent.HasStation(cId)) {
        ++sdError;
        const auto& oStation = oSEvent.GetStation(cId);
        // T2 Trigger
        if (oStation.HasTriggerData()) {
          const auto& oStationTrigger = oStation.GetTriggerData();
          sdError = oStationTrigger.GetErrorCode();
          sdWindow = oStationTrigger.GetWindowMicroSecond();
        }
      }

      if (!omEvent.HasCounter(cId))
        omEvent.MakeCounter(cId);

      // retrieves mevt::Counter and mdet::Counter (Offline) just created
      auto& oCounter = omEvent.GetCounter(cId);
      const auto& mdetCounter = mDetector.GetCounter(cId);

      for (auto mIt = mdCter.ModulesBegin(), end = mdCter.ModulesEnd(); mIt != end; ++mIt) {

        const auto& mdMod = **mIt;
        const int mId = mdMod.GetId();

        if (!oCounter.HasModule(mId))
          oCounter.MakeModule(mId);

        // retrieve mevt::Module (Offline) just created
        auto& oModule = oCounter.GetModule(mId);

        // retrieve mdet::Module
        const auto& mdetModule = mdetCounter.GetModule(mId);

        msg.str("");
        msg << "Counter " << oCounter.GetId() << " module " << oModule.GetId() << " is ";

        std::ostringstream errorFlag;
        errorFlag << "SDError:" << sdError <<":SDWindow:"<< sdWindow << ":MDWindow:" << mdWindow;

        if (t3delay == (unsigned int)(-1)) {

          errorFlag << ":T3NotFound";

          if (sdError == 2 && sdWindow != 0) {  // Silent station
            oModule.SetSilent();
            msg << "silent because \"" << errorFlag.str() << "\".";
          } else {
            oModule.SetRejected(errorFlag.str());  // T1-LTS pair not found in the on-ground (SBC) table, no request is broadcast to modules underground
            msg << "rejected because \"" << errorFlag.str() << "\".";
          }
          WARNING(msg);
          continue;  // no more to do for this module
        } else if (!mdMod.HasT1()) {  // and if T3NotFound
          errorFlag << ":T1NotFound";
          oModule.SetRejected(errorFlag.str());  // The LTS broadcasted to under-ground modules have not been found
          msg << "rejected because \"" << errorFlag.str() << "\".";
          WARNING(msg);
          continue;  // no more to do for this module
        } else if (sdError != 0 && sdError != 256) {
          errorFlag << ":T1Found";
          oModule.SetRejected(errorFlag.str());  // Inconstistency between SD and MD: for SD this station maybe Silent or Bad, for MD the counter is candidate (plausible argument: different buffer length)
          msg << "rejected because \"" << errorFlag.str() << "\".";
          WARNING(msg);
          // continue because MD may have data to load :-(
        } else {
          msg << "candidate.";
          oModule.SetCandidate();
        }

        const auto mdmask = mdetModule.IsSiPM() ?
          mdetModule.GetFrontEndSiPM().GetMask() : mdetModule.GetFrontEnd().GetMask();

        const bitset<64> bit_mask = mdmask;
        oModule.SetChannelMask(bit_mask);

        const auto& dyn = (*mIt)->GetIntegrator();
        if (!dyn.empty()) {
          INFO("*** This module has dynode-integrated signal ***");
          if (!oModule.HasDynodeTrace())
            oModule.MakeDynodeTrace();
          auto& oDynodeTrace = oModule.GetDynodeTrace();
          oDynodeTrace.Assign(dyn.begin(), dyn.end());
        }

        const auto& integratorA = (*mIt)->GetIntegratorA();
        if (!integratorA.empty()) {
          INFO("*** This module has dynode-integrated signal for SiPM electronics ***");
          if (!oModule.HasIntegratorATrace())
            oModule.MakeIntegratorATrace();
          auto& oIntegratorATrace = oModule.GetIntegratorATrace();
          oIntegratorATrace.Assign(integratorA.begin(), integratorA.end());

          if (!oModule.HasIntegratorBTrace())
            oModule.MakeIntegratorBTrace();
          auto& oIntegratorBTrace = oModule.GetIntegratorBTrace();
          const auto& integratorB = (*mIt)->GetIntegratorB();
          oIntegratorBTrace.Assign(integratorB.begin(), integratorB.end());
        }

        // Create all channels
        for (unsigned int bit = 0, chId = 1; bit < 64; ++bit, ++chId) {

          if (!oModule.HasChannel(chId))
            oModule.MakeChannel(chId);

          // retrieves mevt::Channel and mdet::Channel/FrontEnd (Offline) just created
          mevt::Channel& oChannel = oModule.GetChannel(chId);
          oChannel.SetMask(bit_mask[chId-1]);  // set the channel mask

          if (!oChannel.HasTrace()) {
            oChannel.MakeTrace();
            //oChannel.GetTrace().SetBinning(sRate);
            oChannel.GetTrace().SetStart(0);
            oChannel.GetTrace().SetStop(0);
          }

        }

        const unsigned int numberOfSamples = mdetModule.IsSiPM() ?
          mdetModule.GetFrontEndSiPM().GetBufferLength() :
          mdetModule.GetFrontEnd().GetBufferLength();
        FillMdTraces(mdMod, oModule, numberOfSamples);

      }  // end modules loop
    }  // end counters loop
  }


  void
  FillMdTraces(const md::Module& mdMod, mevt::Module& oModule, unsigned int numberOfSamples)
  {
    // Fill time bins of all traces
    auto rawsigIt = mdMod.RawSignalBegin();
    for (unsigned int nTimeBin = 0; nTimeBin < numberOfSamples; ++nTimeBin) {

      if (rawsigIt != mdMod.RawSignalEnd() && nTimeBin == (*rawsigIt)->GetTimeBin()) { // fill traces with data

       const bitset<64> bc = (*rawsigIt)->GetSample();

       ostringstream msg;
       msg << setw(10) << setfill(' ')
           << nTimeBin << ' '
           << bc << " "
              " ==> (" << bc.count() << ")\n";
       DEBUGLOG(msg);

       //Now fill samples on corresponding channels for this time bin
       for (unsigned int bit = 0, chId = 1, n = bc.size(); bit < n; ++bit, ++chId) {

         /* retrieves mevt::Channel and mdet::Channel/FrontEnd (Offline) just created */
         auto& oChannel = oModule.GetChannel(chId);
         auto& trace = oChannel.GetTrace();

         //first "1" of this channel
         if (!trace.GetStart() && bc[bit])
           trace.SetStart(nTimeBin);

         //last "1" of this channel
         if (bc[bit])
           trace.SetStop(nTimeBin);

         /* Signal in MD is a boolean trace of 0s and 1s */
         trace.PushBack(bool(bc[bit]));  // Load the time sample in channel number chId

       }

       ++rawsigIt;  // move to the next RawSignal record

      } else {

        // fill traces with zeros
        for (unsigned int bit = 0, chId = 1; bit < 64; ++bit, ++chId) {
          /* Signal in MD is a boolean trace */
          auto& oChannel = oModule.GetChannel(chId);
          auto& trace = oChannel.GetTrace();
          trace.PushBack(false);
        }

      }

    }
  }


  // From Offline to CDAS
  void
  operator>>(const evt::Event& event, md::Event& rawEvent)
  {
    ConvertEventToIoMd(event, rawEvent);
  }


  void
  ConvertEventToIoMd(const evt::Event& /*event*/, md::Event& /*rawEvent*/)
  {
    // do nothing
  }

} // namespace evt


namespace io {

  void
  ApplyTimeCorrection(StationGPSData& gpsData)
  {
    // NEW : TAP 26/04/2003 -> From CDAS v1r2 : taking into account Offsets...
    // Warning, part of the field is used for the tick offset:
    // GPS Offset = 0.01*(short)(gps->Offset & 0xffff)
    // Tick Offset = (short)(gps->Offset>>16)
    // New: taking into account 100ns jumps
    // From Moulin Rouge and Dia Noche we found that the TickFall-Tick
    // can be 0, 9, 10, 11 or a big number. The big number could be
    // understood if it is the trigger of another event. It was found
    // that if the dt is 0, there is a 100ns jump in the event, and not
    // in any other case, including big values. Hence this empiric
    // correction
    //
    // This is the code from IoSd v1r2 :
    // gps->NanoSecond =
    //   (unsigned int)(
    //     (gps->Tick*(1000000000.0 + gps->NextST - gps->CurrentST) / gps->Next100) +
    //     gps->CurrentST + 0.01*(short)(gps->Offset & 0xffff)
    //   ) - 100*(gps->TickFall == gps->Tick);

    const unsigned int tick = gpsData.GetTick();
    const int currentST = gpsData.GetCurrentST();
    const int next100 = gpsData.GetNext100();
    const int nextST = gpsData.GetNextST();

#ifndef IOSD_V1R0
    const unsigned int tickFall = gpsData.GetTickFall();
    const int offset = gpsData.GetOffset();

    const unsigned int nanosecond =
      (unsigned int)(
        tick * (1e9 + nextST - currentST) / next100 +
        currentST +
        0.01 * (offset & 0xffff)
      ) - 100 * (tickFall == tick);
#else
    const unsigned int nanosecond =
      tick * (1e9 + nextST - currentST) / next100 + currentST;
#endif

    gpsData.SetCorrectedNanosecond(nanosecond);
  }

}
