#include <iostream>
#include <fstream>
#include "IoSd.h"
#include "utl/QuadraticFitter.h"

int main(int argc, char *argv[]) 
{
  const int gpsOffset = 315964800;




  ofstream peakFile("peak.txt", ios_base::trunc);
  // ofstream chargeFile("charge.txt", ios_base::trunc);
  EventPos pos; IoSd input(argc - 1, argv + 1);
  int nData = 0;

  // make this multi-threaded?
  for (pos = input.FirstEvent(); pos < input.LastEvent(); pos = input.NextEvent())
  {
    IoSdEvent event(pos);

    const vector<IoSdStation>& stations = event.Stations;
    for (const IoSdStation& station : stations)
    {
      const UInt_t stationId = station.id();
      const Double_t stationEasting = station.easting();
      const Double_t stationNorthing = station.northing();
      // const auto& peakHistoBinning = station.GetMuonPeakHistogramBinning<short>(type, calibVersion);

      // if (inInfillSquare(stationEasting, stationNorthing)) continue;              // skip stations in Infill    
      if (!station.IsUUB) continue;                                               // skip non-UUB stations, unneccesary
      const IoSdCalib* const stationCalib = station.calib();
      if (!stationCalib || stationCalib->Version <= 262) continue;                // ensure SSD Histograms are present
      const IoSdHisto* const calibrationHistograms = station.histo();
      if (!calibrationHistograms || calibrationHistograms->type != 1) continue;   // ensure SSD Histograms are "normal"
      const UShort_t* const ssdPeakHisto = calibrationHistograms->Peak3;
      if (!ssdPeakHisto) continue;

      const UInt_t startSecond = stationCalib->StartSecond;
      const UInt_t endSecond = startSecond + stationCalib->EndSecond;
      const UInt_t average = 0.5 * (startSecond + endSecond);
      const UInt_t tubeMask = stationCalib->TubeMask;

      // calibration histograms for WCD PMTs
      const auto wcdPeakHisto = calibrationHistograms->Peak;
      const auto wcdChargeHisto = calibrationHistograms->Charge;
      for (unsigned int iPMT = 0; iPMT < 3; iPMT++)
      {
        string peakHisto = "";
        for (unsigned int bin = 0; bin < sizeof(IoSdHisto::Peak)/sizeof(UShort_t)/3; bin++)
        {
          peakHisto += to_string(wcdPeakHisto[iPMT][bin]) + ' ';
        }

        string chargeHisto = "";
        for (unsigned int bin = 0; bin < sizeof(IoSdHisto::Charge)/sizeof(UShort_t)/4; bin++)
        {
          chargeHisto += to_string(wcdChargeHisto[iPMT][bin]) + ' ';
        }

        peakFile << stationId << " " << stationCalib->EndSecond << " " << average + gpsOffset << " " << tubeMask << " " << iPMT << " " << peakHisto << '\n';
        // chargeFile << stationId << " " << stationCalib->EndSecond << " " << average + gpsOffset << " " << tubeMask << " " << iPMT << " " << chargeHisto << '\n';
      }

      string peakHisto = "";
      for (unsigned int bin = 0; bin < sizeof(IoSdHisto::Peak3)/sizeof(UShort_t); bin++)
      {
        peakHisto += to_string(ssdPeakHisto[bin]) + ' ';
      }

      string chargeHisto = "";
      for (const auto& bin : wcdChargeHisto[3])
      {
        chargeHisto += to_string(bin) + ' ';
      }

      peakFile << stationId << " " << stationCalib->EndSecond << " " << average + gpsOffset << " " << tubeMask <<" 3 " << peakHisto << '\n';
      // chargeFile << stationId << " " << stationCalib->EndSecond << " " << average + gpsOffset << " " << tubeMask << " 3 " << chargeHisto << '\n';
      nData += 1;
    }
    // break;
  }
  cout << "wrote " << nData << " histos to disk" << '\n';
  return 0;
}
