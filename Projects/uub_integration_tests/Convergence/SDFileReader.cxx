#include <iostream>
#include <fstream>
#include "IoSd.h"

bool inInfillSquare(Double_t easting, Double_t northing)
{
  // rough estimate of a square that 
  // contains all infill stations
  Double_t minX = 446800, maxX = 454400;
  Double_t minY = 6111300, maxY = 61116600;

  return (minX < easting && easting < maxX) && (minY < northing && northing < maxY);
}

int main(int argc, char *argv[]) 
{
  ofstream outFile("out.txt", ios_base::trunc);
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

      if (inInfillSquare(stationEasting, stationNorthing)) continue;              // skip stations in Infill    
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

      // calibration histograms for WCD PMTs
      const auto wcdPeakHisto = calibrationHistograms->Peak;
      for (unsigned int iPMT = 0; iPMT < 3; iPMT++)
      {
        string histo = "";
        for (unsigned int bin = 0; bin < sizeof(IoSdHisto::Peak)/sizeof(UShort_t)/3; bin++)
        {
          histo += to_string(wcdPeakHisto[iPMT][bin]) + ' ';
        }

        outFile << stationId << " " << stationCalib->EndSecond << " " << average + 315964800 << " " << iPMT << " " << histo << '\n';
      }

      string histo = "";
      // UInt_t tailSum = 0;
      for (unsigned int i = 0; i < sizeof(IoSdHisto::Peak3)/sizeof(UShort_t); ++i)
      {
        histo += to_string(ssdPeakHisto[i]) + ' ';

        // additional quality control for SSD histos here?
        // tailSum += i > 125 ? ssdPeakHisto[i] : 0;

      }

      // if (tailSum == 0) continue;

      outFile << stationId << " " << stationCalib->EndSecond << " " << average + 315964800 << " 3 " << histo.substr(0, histo.size() - 1) << '\n';
      nData += 1;
    }
    // break;
  }
  cout << "wrote " << nData << " histos to disk" << '\n';
  return 0;
}
