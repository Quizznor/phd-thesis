#include <algorithm>
#include <iostream>
#include <fstream>
#include <future>
#include "IoSd.h"

static std::mutex fileWriteLock;
UInt_t nEventsFound = 0;

void readRootFile(std::vector<std::string> filePaths, UInt_t threadNumber)
{
  std::cout << "launched thread #" << threadNumber << " to analyze file: " << filePaths[threadNumber] << '\n';
  EventPos pos; IoSd input(filePaths[threadNumber].c_str());

  // for (pos = input.FirstEvent(); pos < input.LastEvent(); pos = input.NextEvent())
  // {
  //   IoSdEvent event(pos);
  //   std::cout << event.Id << '\n';
  // }

  // input.Close();
  filePaths[threadNumber] = "";
}

int main(int argc, char *argv[])
{
  // we should limit number of threads here
  // otherwise people will start complaining
  // about performance of the IAP computers
  const UInt_t maxThreads = 2;
  vector<std::future<void>> results;
  std::vector<std::string> filePaths(maxThreads);
  ofstream outFile("out_concurrent.txt", std::ios_base::app);

  for (char **file = argv + 1; *file != argv[argc]; file++)
  {
    // eliminate threads that have finished their work
    while (results.size() > maxThreads - 1)
    {
      for (unsigned int iThread = 0; iThread < results.size(); iThread++)
      {
        const auto response = results[iThread].wait_for(std::chrono::seconds(0));
        if (response == std::future_status::ready) results.erase(results.begin() + iThread);
      }
    }

    // create new threads
    UInt_t freeThread;
    for (UInt_t iFreeThread = 0; iFreeThread < filePaths.size(); iFreeThread++)
    {
      if (filePaths[iFreeThread].size() == 0)
      {
        freeThread = iFreeThread;
        break;
      }
    }

    filePaths[freeThread] = *file;
    results.push_back(std::async(std::launch::async, readRootFile, filePaths, freeThread));
  }


  return 0;
}

int test(int argc, char *argv[]) 
{
  ofstream outFile("out_wcd.txt", ios_base::app);
  EventPos pos; IoSd input(argc - 1, argv + 1);
  UInt_t nData = 0;

  // make this multi-threaded?
  for (pos = input.FirstEvent(); pos < input.LastEvent(); pos = input.NextEvent())
  {
    IoSdEvent event(pos);
    
    const vector<IoSdStation>& stations = event.Stations;

    for (const IoSdStation& station : stations)
    {
      const UInt_t stationId = station.id();
      if (stationId != 56) continue;                                  // skip stations that are not NuriaJr
      if (!station.IsUUB) continue;                                   // skip non-UUB stations, unneccesary
      const IoSdCalib* const stationCalib = station.calib();
      if (stationCalib->Version <= 262) continue;                     // ensure SSD Histograms are present
      const IoSdHisto* const calibrationHistograms = station.histo();
      if (calibrationHistograms->type != 1) continue;                 // ensure SSD Histograms are "normal"

      nData += 1;
      const UInt_t startSecond = stationCalib->StartSecond;
      const UInt_t endSecond = startSecond + stationCalib->EndSecond;
      const UInt_t average = 0.5 * (startSecond + endSecond);
      const auto wcdPeakHisto = calibrationHistograms->Peak;
      // const UShort_t* const ssdPeakHisto = calibrationHistograms->Peak3;

      for (unsigned int iPMT = 0; iPMT < 3; iPMT++)
      {
        const Float_t* vemPeak = (&stationCalib->VemPeak)[iPMT];
        outFile << stationId << " " << average << " " << *vemPeak << " " << iPMT + 1 << " ";

        for (unsigned int i = 0; i < sizeof(IoSdHisto::Peak)/sizeof(UShort_t)/3; i++)
        {
          outFile << wcdPeakHisto[iPMT][i] << ' ';
        }
        outFile << '\n';
      }

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
