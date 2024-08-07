#include <algorithm>
#include <iostream>
#include <fstream>
#include "IoSd.h"
#include <vector>

int main(int argc, char *argv[]) 
{
    
  ofstream outSSDFile("/cr/data01/filip/Data/muonAcquisition/T3events/day/t3_histos.ssd", ios_base::trunc);
  ofstream outWCDFile("/cr/data01/filip/Data/muonAcquisition/T3events/day/t3_histos.wcd", ios_base::trunc);
  EventPos pos; IoSd input(argc - 1, argv + 1);

  std::vector<UInt_t> positions = {1840, 1823, 660, 1845, 643, 1839, 659, 1844};

  // make this multi-threaded?
  for (pos = input.FirstEvent(); pos < input.LastEvent(); pos = input.NextEvent())
  {
    IoSdEvent event(pos);

    const vector<IoSdStation>& stations = event.Stations;
    for (const IoSdStation& station : stations)
    {
      const UInt_t stationId = station.id();

      if (std::find(positions.begin(), positions.end(), stationId) == positions.end()) continue;

      const IoSdCalib* const stationCalib = station.calib();

      if (!stationCalib || stationCalib->Version <= 262) continue;                // ensure SSD Histograms are present
      const IoSdHisto* const calibrationHistograms = station.histo();

      const unsigned int tubeMask = stationCalib->TubeMask;
      cout << stationId << ' ' << tubeMask << '\n';

      if (!calibrationHistograms || calibrationHistograms->type != 1) continue;   // ensure SSD Histograms are "normal"
      const UShort_t* const ssdPeakHisto = calibrationHistograms->Peak3;
      if (!ssdPeakHisto) continue;

      const UInt_t startSecond = stationCalib->StartSecond;
      const UInt_t endSecond = startSecond + stationCalib->EndSecond;
      const UInt_t average = 0.5 * (startSecond + endSecond);

      string histo = "";
      UInt_t tailSum = 0;
      for (unsigned int i = 0; i < sizeof(IoSdHisto::Peak3)/sizeof(UShort_t); ++i)
      {
        histo += to_string(ssdPeakHisto[i]) + ' ';
      }
      const auto& ssdPmt = stationCalib->DA[5];
      std::cout << ssdPmt << '\n';

      // calibration histograms for WCD PMTs
      const auto wcdPeakHisto = calibrationHistograms->Peak;
      for (unsigned int iPMT = 0; iPMT < 3; iPMT++)
      {
        const Float_t* vemPeak = (&stationCalib->VemPeak)[iPMT];
        const auto* peakHisto = wcdPeakHisto[iPMT];
        outWCDFile << stationId << " " << average << " " << *vemPeak << " " << iPMT + 1;

        for (unsigned int i = 0; i < sizeof(IoSdHisto::Peak)/sizeof(UShort_t)/3; i++)
        {
          outWCDFile << " " << wcdPeakHisto[iPMT][i];
        }

        outWCDFile << '\n';
      }

      outSSDFile << stationId << " " << average << " " << histo.substr(0, histo.size() - 1) << '\n';
    }
    // break;
  }
  return 0;
}
