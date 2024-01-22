#include <iostream>
#include "IoSd.h"

// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
//
// // command line for all files of 1, 2 and 3 may as input:
// // [shell]> ./analyse.exe $(DATAPATH)/2002/05/eb_2002_05_0[123]*.root
// // command line for all inputs:
// // [shell]> ./analyse.exe $(DATAPATH)/*/*/*.root
// /**************************************************************************/

constexpr int GetError(const IoSdStation& station)
{
  return station.Error & 0xff;
}

int main(int argc, char *argv[]) {

  EventPos pos; IoSd input(argc - 1, argv + 1);

  for (pos = input.FirstEvent(); pos < input.LastEvent(); pos = input.NextEvent())
  {
    IoSdEvent event(pos);
    
    const vector<IoSdStation>& stations = event.Stations;
    for (const IoSdStation& station : stations)
    {
      if (!station.IsUUB) continue;
      const IoSdCalib* const stationCalib = station.calib();
      if (stationCalib->Version <= 259) continue;
      const IoSdHisto* const calibrationHistograms = station.histo();
      if (calibrationHistograms->type != 1) continue;

      const UShort_t* const ssdPeakHisto = calibrationHistograms->Peak3;

      for (unsigned int i = 0; i < sizeof(IoSdHisto::Peak3)/sizeof(UShort_t); i++)
      {
        cout << ssdPeakHisto[i] << " ";
      }

      cout << '\n';
    }
  }
  return 0;
}
