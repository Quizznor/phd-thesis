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
      // const UInt_t stationId = station.id();
      // if (stationId != 943 || stationId != 944 || stationId != 949 || stationId != 954) continue;

      const IoSdCalib* const stationCalib = station.calib();
      // if (!stationCalib || stationCalib->Version <= 262) continue; // ensure SSD Histograms are present
      
      // std::cout << stationCalib->StartSecond + gpsOffset << " " << stationId 
      //           << " MIP (ADC): " << stationCalib->MipPeak
      //           << "; T70 rate (Hz): " << stationCalib->SsdRate
      //           << std::endl;

      std::cout << "StationId " << station.id() << "\n";
      std::cout << "Version " << stationCalib->Version << "\n";
      std::cout << "TubeMask " << stationCalib->TubeMask << "\n";
      std::cout << "StartSecond " << stationCalib->StartSecond << "\n";
      std::cout << "EndSecond " << stationCalib->EndSecond << "\n";
      std::cout << "NbT1 " << stationCalib->NbT1 << "\n";
      std::cout << "NbT2 " << stationCalib->NbT2 << "\n";
      for (int i=0; i < 3; i++) std::cout << "Evolution" << i << " " << stationCalib->Evolution[i] << "\n";
      for (int i=0; i < 6; i++) std::cout << "Base" << i << " "<< stationCalib->Base[i] << "\n";
      for (int i=0; i < 6; i++) std::cout << "SigmaBase" << i << " "<< stationCalib->SigmaBase[i] << "\n";
      for (int i=0; i < 3; i++) std::cout << "VemPeak" << i << " "<< stationCalib->VemPeak[i] << "\n";
      // std::cout << "MipPeak " << stationCalib->MipPeak << "\n";
      for (int i=0; i < 3; i++) std::cout << "VemRate" << i << " "<< stationCalib->Rate[i] << "\n";
      // std::cout << "MipRate " << stationCalib->SsdRate << "\n";
      for (int i=0; i < 3; i++) std::cout << "NbTDA" << i << " " << stationCalib->NbTDA[i] << "\n";
      for (int i=0; i < 3; i++) std::cout << "DA" << i << " " << stationCalib->DA[i] << "\n";
      for (int i=0; i < 3; i++) std::cout << "SigmaDA" << i << " " << stationCalib->SigmaDA[i] << "\n";
      for (int i=0; i < 3; i++) std::cout << "VemCharge" << i << " " << stationCalib->VemCharge[i] << "\n";
      std::cout << "NbTOT " << stationCalib->NbTOT << "\n\n\n\n";



    }
  }
  return 0;
}

// _CurrentStation->Calib->NbT1 = ntohs(*sP++);
// _CurrentStation->Calib->NbT2 = ntohs(*sP++);
// for (unsigned int j=0; j<kIoSd::NPMT; j++) _CurrentStation->Calib->Evolution[j] = ntohs(*sP++);
// for (unsigned int j=0; j<2*kIoSd::NPMT; j++) _CurrentStation->Calib->Base[j] = ntohs(*sP++)/100.;
// for (unsigned int j=0; j<2*kIoSd::NPMT; j++) _CurrentStation->Calib->SigmaBase[j] = ntohs(*sP++)/100.;
// for (unsigned int j=0; j<kIoSd::NPMT; j++) _CurrentStation->Calib->VemPeak[j] = ntohs(*sP++)/10.;
// _CurrentStation->Calib->MipPeak = ntohs(*sP++)/10.;
// for (unsigned int j=0; j<kIoSd::NPMT; j++) _CurrentStation->Calib->Rate[j] = ntohs(*sP++)/100.;
// _CurrentStation->Calib->SsdRate = ntohs(*sP++)/10.;
// for (unsigned int j=0; j<kIoSd::NPMT; j++) _CurrentStation->Calib->NbTDA[j] = ntohs(*sP++);
// for (unsigned int j=0; j<kIoSd::NPMT; j++) _CurrentStation->Calib->DA[j] = ntohs(*sP++)/100.;
// for (unsigned int j=0; j<kIoSd::NPMT; j++) _CurrentStation->Calib->SigmaDA[j] = ntohs(*sP++)/100.;
// for (unsigned int j=0; j<kIoSd::NPMT; j++) _CurrentStation->Calib->VemCharge[j] = ntohs(*sP++)/10.;
// _CurrentStation->Calib->NbTOT = ntohs(*sP++); 
// if (calsize==104) {
//   for (unsigned int j=0; j<kIoSd::NPMT; j++) _CurrentStation->Calib->DADt[j] = ntohs(*sP++)/100.;
//   for (unsigned int j=0; j<kIoSd::NPMT; j++) _CurrentStation->Calib->SigmaDADt[j] = ntohs(*sP++)/100.;
//   for (unsigned int j=0; j<kIoSd::NPMT; j++) _CurrentStation->Calib->DAChi2[j] = ntohs(*sP++)/100.;
// sP++; // (we have 9 shorts. To int-pad it, we need a 10th short.
// } else {
//   for (unsigned int j=0; j<kIoSd::NPMT; j++) _CurrentStation->Calib->DADt[j] = 0;
//   for (unsigned int j=0; j<kIoSd::NPMT; j++) _CurrentStation->Calib->SigmaDADt[j] = 0;
//   for (unsigned int j=0; j<kIoSd::NPMT; j++) _CurrentStation->Calib->DAChi2[j] = 0;
