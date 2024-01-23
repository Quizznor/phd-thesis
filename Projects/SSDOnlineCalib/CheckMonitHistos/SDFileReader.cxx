#include <iostream>
#include <fstream>
#include "IoSd.h"

int main(int argc, char *argv[]) {

  EventPos pos; IoSd input(argc - 1, argv + 1);
  UInt_t nData = 0;

  // make this multi-threaded?
  for (pos = input.FirstEvent(); pos < input.LastEvent(); pos = input.NextEvent())
  {
    IoSdEvent event(pos);
    ofstream outFile("out_new.txt", ios_base::app);
    
    const vector<IoSdStation>& stations = event.Stations;

    for (const IoSdStation& station : stations)
    {
      const UInt_t stationId = station.id();
      if (stationId != 56) continue;                                  // skip stations that are not NuriaJr
      if (!station.IsUUB) continue;                                   // skip non-UUB stations, unneccesary
      const IoSdCalib* const stationCalib = station.calib();
      if (stationCalib->Version <= 262) continue;                     // ensure SSD Histograms are present
      const IoSdHisto* const calibrationHistograms = station.histo();
      if (calibrationHistograms->type != 1) continue;                 // ensure SSD Histograms are normal

      nData += 1;
      const UInt_t startSecond = stationCalib->StartSecond;
      const UInt_t endSecond = startSecond + stationCalib->EndSecond;
      const UInt_t average = 0.5 * (startSecond + endSecond);

      outFile << stationId << " " << average << " ";

      // const UShort_t* const ssdPeakHisto = calibrationHistograms->Peak3;
      const UShort_t* wcdPeakHisto[kIoSd::NPMT][150] = {{calibrationHistograms->Peak}};
    //   for (unsigned int i = 0; i < sizeof(IoSdHisto::Peak3)/sizeof(UShort_t); i++)
    //   {
    //     outFile << ssdPeakHisto[i] << " ";
    //   }
    //   outFile << "\n";
    //   // break;
    }
    // break;
  }
  cout << "wrote " << nData << " data points to disk" << '\n';
  return 0;
}
