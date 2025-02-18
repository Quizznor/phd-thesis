#include <iostream>
#include <fstream>
#include "IoSd.h"

int main(int argc, char *argv[]) 
{
  const int gpsOffset = 315964800;
  EventPos pos; IoSd input(argc - 1, argv + 1);

  // make this multi-threaded?
  for (pos = input.FirstEvent(); pos < input.LastEvent(); pos = input.NextEvent())
  {
    const IoSdEvent event(pos);

    const vector<IoSdStation>& stations = event.Stations;
    for (const IoSdStation& station : stations)
    {
      const UInt_t stationId = station.id();
      const IoSdCalib* const stationCalib = station.calib();
      // if (!stationCalib || stationCalib->Version <= 262) continue;                // ensure SSD Histograms are present
    
      std::cout << stationCalib->StartSecond + gpsOffset << " " << stationId 
                << " MIP (ADC): " << stationCalib->MipPeak
                << "; T70 rate (Hz): " << stationCalib->SsdRate
                << std::endl;
    }
  }
  return 0;
}