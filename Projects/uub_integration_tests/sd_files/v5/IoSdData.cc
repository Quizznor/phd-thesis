#include "TCdasUtil.h"
#include "IoSdData.h"
#include "IoSdRootI.h"
#include "LatLong-UTMConversion.h"
#include <cmath>
#include <cstdlib>
#include <arpa/inet.h>

//**************** IoSd constants ***************************************

static void fd2xyz(int i,double *x, double *y, double *z) {
  double n = 0, e = 0, a=0;
  switch (i) {
  case 1:
    n = kIoSd::N_LL;
    e = kIoSd::E_LL;
    a = kIoSd::A_LL;
    break;
  case 2:
    n = kIoSd::N_LM;
    e = kIoSd::E_LM;
    a = kIoSd::A_LM;
    break;
  case 3:
    n = kIoSd::N_LA;
    e = kIoSd::E_LA;
    a = kIoSd::A_LA;
    break;
  case 4:
    n = kIoSd::N_CO;
    e = kIoSd::E_CO;
    a = kIoSd::A_CO;
    break;
  case 5:
    n = kIoSd::N_HEAT;
    e = kIoSd::E_HEAT;
    a = kIoSd::A_HEAT;
    break;
  }
  nea2xyz(n, e, a, x, y, z);
};

double kIoSd::FdX(int i) {
  double x, y, z;
  fd2xyz(i, &x, &y, &z);
  return x;
};

double kIoSd::FdY(int i) {
  double x, y, z;
  fd2xyz(i, &x, &y, &z);
  return y;
};

double kIoSd::FdZ(int i) {
  double x, y, z;
  fd2xyz(i, &x, &y, &z);
  return z;
};

IoSdHistoCoinc::IoSdHistoCoinc() :
  HasHisto(0),
  SSDThreshold(0)
{
  for (unsigned int i = 0; i < kIoSd::NPMT; i++) {
    for (int j = 0; j < 150; j++)
      Peak[i][j] = 0;
    for (int j = 0; j < 600; j++)
      Charge[i][j] = 0;
  }
}
 
IoSdPMQuality::IoSdPMQuality() {
  Version=-1;
  TubeMask=-1;
  AnodeMask=-1;
  RainingMask=-1;

}

IoSdAntenna::IoSdAntenna() {
  Trigger=0;
  Type=0;           //Antenna type
  Candidate=0;
  IsDynodeSat=0;
  IsAnodeSat=0;
  SignalInVem=0;
  DistToAxis=0;
  Baseline=0;
  BaselineSigma=0;
   Peak=0;
  PeakTimeBin=0;
  for (int i=0;i<kIoSd::MAXSAMPLE;i++)Trace[i]=0; 
}

//***********************************************************************
// IoSdCalib member functions>

IoSdCalib::IoSdCalib():
  Version(0),
  TubeMask(0),
  StartSecond(0), 
  EndSecond(0),
  SsdRate(0),   
  MipPeak(0),
  MipCharge(0),
  NbT1(0),        
  NbT2(0),  
  NbTOT(0),           
  NTubesOk(0),
  T1Rate(0),
  T2Rate(0),
  TOTRate(0){
  
  for (unsigned int i = 0; i < kIoSd::NPMT; i++){
    DADt[i] = 0;
    TDARate[i] = 0;
    TubeOk[i] = 0;
    Stability[i] = 0;
    DAChi2[i] = 0;
    SigmaDADt[i] = 0;
    SigmaDA[i] = 0; 
    DA[i] = 0;
    VemCharge[i] = 0;  
    NbTDA[i] = 0;
    Evolution[i] = 0;
    Rate[i] = 0;
    VemPeak[i] = 0;
    Base[i] = 0;  
    SigmaBase[i] = 0;
    
  }
  for (unsigned int i =  kIoSd::NPMT; i < 2*kIoSd::NPMT; ++i){
    Base[i] = 0;  
    SigmaBase[i] = 0;
  }
  
}

int IoSdCalib::ComputeParameters() {
  if (!Version) {
    //cerr<< "No calibration available, using estimations..."<< endl;
    StartSecond = 999999999;
    EndSecond = 0;
    NTubesOk = 0;
    SsdRate = 0;
    MipPeak = 50;
    MipCharge = MipPeak * 3.7; 
    T1Rate = T2Rate = TOTRate = 0;
    for (unsigned int i = 0; i < kIoSd::NPMT; i++) {
      NbTDA[i] = 0;
      DA[i] = 0;
      Base[i] = 0;
      TubeOk[i] = 0;
      Stability[i] = 0;
      ShowEvolution[i] = "????????";
      VemPeak[i] = 50;
      VemCharge[i] = VemPeak[i] * 3.7;
    }
  } else {
    if (Version < 4)
      cerr << "????????????? ERROR IN CALIB VERSION !!!!" << endl;
    for (unsigned int i = 0; i < kIoSd::NPMT; i++) {
      TubeOk[i] = ((TubeMask & (1 << i)) == (1 << i));
      ShowEvolution[i] = "";
      Stability[i] = 0;
      for (unsigned int j = 0; j < 8; j++) {
        if (((Evolution[i] >> 2 * j) & 0x3) == 1) {
          ShowEvolution[i] += "-";
          Stability[i]--;
        } else if (((Evolution[i] >> 2 * j) & 0x3) == 2) {
          ShowEvolution[i] += "+";
          Stability[i]++;
        } else if (((Evolution[i] >> 2 * j) & 0x3) == 0)
          ShowEvolution[i] += "=";
        else
          ShowEvolution[i] += "?";
      }
    }
    NTubesOk = TubeOk[0] + TubeOk[1] + TubeOk[2];

    ////////////////////////////////////////////////////////////////////////
    if (EndSecond > StartSecond + 1) {
      T1Rate = (1.0 * NbT1) / (EndSecond - StartSecond - 1);
      T2Rate = (1.0 * NbT2) / (EndSecond - StartSecond - 1);
      TOTRate = (1.0 * NbTOT) / (EndSecond - StartSecond - 1);
    } else {
      T1Rate = -1;
      T2Rate = -1;
      TOTRate = -1;
    }
  }
  /*   WE WANT RAW VALUES... No more change. Now it goes in Er */
  return 1;
  /*for (unsigned int i=0; i<kIoSd::NPMT; i++) { */
  /* Corrections from known deviation of Calibration to histogram maximum */
  /*if(Version == 4) { */
  /*if(NTubesOk == 3) { */
  /* correction factor from histograms */
  /*VemCharge[i] *= 1.04; */
  /*VemPeak[i] *= 1.01; */
  /*} else if(NTubesOk == 2) { */
  /* correction factor from histograms */
  /*VemCharge[i] *= 1.12; */
  /*VemPeak[i] *= 1.2; */
  /*} */
  /*} */
  /* Last corrections: here we are calibrating individual tubes.
   * Then, one has to apply the any-direction-muon to vertical-central-muon
   * factor. In Laura, we have the preliminary values used below */
  /*VemPeak[i] /= 0.87;  // From Laura, very dependant on cut, but anyway, we don't care about the peak... */
  /*VemCharge[i] /= 1.01; // From Laura */
  /*} */
  /*return 1; */
}


//**************************************************************************************************
// IoSdStation member functions

// Histograms....
//
TH1F *IoSdStation::HBase(int i) {
  char name[27];
  name[26] = 0;
  TH1F *histo;
  if (!Histo)
    return NULL;
  snprintf(name, 26, "Base PMT %d, LS %u", i + 1, Id);
  if (IsUUB && Calib->Version<259) {
      Histo->Offset[i]=0; // was buggued
  }
  int offset=Histo->Offset[i];
  if (IsUUB && i==3) offset=Histo->Offset3[0];  // SSD special treatment
  if (gDirectory->GetList()->FindObject(name)) {
    histo = (TH1F *) gDirectory->GetList()->FindObject(name);
    histo->GetXaxis()->Set(20, offset, offset + 20);
  } else {
    histo =
      new TH1F(name, name, 20, offset, offset + 20);
    histo->SetLineColor(i + 2);
    histo->SetStats(0);
  }
  for (int j = 0; j < 20; j++) {
    int v;
    if (IsUUB && i==3) v=Histo->Base3[j];  // SSD special treatment
    else v=Histo->Base[i][j];
    histo->SetBinContent(j + 1, v);
  }
  return histo;
}

TH1F *IoSdStation::HPeak(int i) {
  // updated to take into account UUB
  char name[27];
  name[26] = 0;
  TH1F *histo;
  if (!Histo)
    return NULL;
  if (IsUUB && Calib->Version && Calib->Version<257) return NULL;
  // setting UB/UUB specific variables
  int mult=1;
  int offset=0;
  float bigbins=3.;
  if (IsUUB) {
    mult=4;
    bigbins=4.;
    if (Calib->Version>258) offset=Histo->Offset[i + 3];
    if (Calib->Version>258 && i==3) offset=Histo->Offset3[1];
  } else offset=Histo->Offset[i + 3];
  // Building axis
  double xp[151];
  for (int j = 0; j < 102; j++) {
    // if (IsUUB) xp[j] = j * 4;
    // else xp[j] = j + Histo->Offset[i + 3];
    xp[j] = j*mult + offset;
  }
  for (int j = 0; j < 51; j++) {
    // if (IsUUB) xp[102 + j] = 102 * 4 + j * 4 * 4;
    // else xp[100 + j] = 100 + 3 * j + Histo->Offset[i + 3];
    xp[100 + j] = 100 * mult + bigbins * j * mult + offset;
  }
  snprintf(name, 26, "Peak PMT %d, LS %u", i + 1, Id);
  if (gDirectory->GetList()->FindObject(name)) {
    histo = (TH1F *) gDirectory->GetList()->FindObject(name);
    histo->GetXaxis()->Set(150, xp);
  } else {
    histo = new TH1F(name, name, 150, xp);
    histo->SetLineColor(i + 2);
    histo->SetStats(0);
  }
  for (int j = 0; j < 102; j++) {
    double v=0;
    if (IsUUB && i==3 && Calib->Version>258) v=Histo->Peak3[j];
    else v=Histo->Peak[i][j];
    histo->SetBinContent(j + 1, v);
  }
  for (int j = 0; j < 50; j++) {
    double v=0;
    if (IsUUB && i==3 && Calib->Version>258) v=Histo->Peak3[j+100];
    else v=Histo->Peak[i][j+100];
    // if (IsUUB) histo->SetBinContent(j + 1 + 102, Histo->Peak[i][j + 102] / 4.);
    // else histo->SetBinContent(j + 1 + 100, Histo->Peak[i][j + 100] / 3.);
    histo->SetBinContent(j + 1 + 100, v / bigbins);
  }
  return histo;
}


TH1F *IoSdStation::HCharge(int i) {
  // updated to take into account UUB
  char name[27];
  name[26] = 0;
  static TH1F *histo;
  if (!Histo)
    return NULL;
  if (IsUUB && Calib->Version && Calib->Version<257) return NULL;
  // setting UB/UUB specific variables
  int mult = 1;
  if (i == 3)
    mult = 3;
  int offset=0;
  float bigbins=3.;
  if (IsUUB) {
    mult=8;
    bigbins=4.;
    if (Calib->Version>258) offset=Histo->Offset[i + 6];
    if (i==3 && Calib->Version>258) offset=Histo->Offset3[2];
  } else offset=Histo->Offset[i + 6];
  // Building axis
  double xc[601];
  for (int j = 0; j < 402; j++) {
    //if (IsUUB) xc[j] = 8 * j;
    //else xc[j] = mult * j + offset;
    xc[j] = mult * j + offset;
  }
  for (int j = 0; j < 201; j++) {
    //if (IsUUB) xc[400 + j] = 400 * 8 + 4 * 8 * j;
    //else xc[400 + j] = 400 * mult + 3 * mult * j + offset;
    xc[400 + j] = 400 * mult + bigbins * mult * j + offset;
  }
  snprintf(name, 26, "Charge PMT %d, LS %u", i + 1, Id);
  if (gDirectory->GetList()->FindObject(name)) {
    histo = (TH1F *) gDirectory->GetList()->FindObject(name);
    histo->GetXaxis()->Set(600, xc);
  } else {
    histo = new TH1F(name, name, 600, xc);
    histo->SetLineColor(i + 2);
    if (i == 3)
      histo->SetLineColor(13);    // Yellow is not very visible
    histo->SetStats(0);
  }
  for (int j = 0; j < 402; j++)
    histo->SetBinContent(j + 1, Histo->Charge[i][j]);
  for (int j = 0; j < 200; j++) {
    //if (IsUUB) histo->SetBinContent(j + 1 + 400, Histo->Charge[i][j + 400] / 4.);
    //else histo->SetBinContent(j + 1 + 400, Histo->Charge[i][j + 400] / 3.);
    histo->SetBinContent(j + 1 + 400, Histo->Charge[i][j + 400] / bigbins);
  }
  return histo;
}


TH1F *IoSdStation::HShape(int i) {
  char name[27];
  name[26] = 0;
  static TH1F *histo;
  int nbmu = 20;
  float dt=25;
  if (Calib->Version == 13)
    nbmu = 19;
  if (!Histo)
    return NULL;
  if (IsUUB && Calib->Version && Calib->Version<257) return NULL;
  if (IsUUB) {
    nbmu=69;
    dt=1000./120.;
  }
  snprintf(name, 26, "Shape PMT %d, LS %u", i + 1, Id);
  if (gDirectory->GetList()->FindObject(name)) {
    histo = (TH1F *) gDirectory->GetList()->FindObject(name);
    histo->GetXaxis()->Set(nbmu, 0, nbmu * dt);
  } else {
    histo = new TH1F(name, name, nbmu, 0, nbmu * dt);
    histo->SetLineColor(i + 2);
    histo->SetStats(0);
  }
  for (int j = 0; j < nbmu; j++)
    if (IsUUB) histo->SetBinContent(j + 1, 1.0 * Histo->UShape[i][j]);
    else histo->SetBinContent(j + 1,
                         1.0 * Histo->Shape[i][j] -
                         1.0 * Histo->Shape[i][nbmu - 1]);
  return histo;
}


TH1F *IoSdStation::HSumCharge() {
   if (IsUUB) return HCharge(3);
  char name[27];
  name[26] = 0;
  static TH1F *histo;
  if (!Histo)
    return NULL;
  snprintf(name, 26, "Sum Charge LS %u", Id);
  double xc[601];
  for (int j = 0; j < 400; j++)
    xc[j] = 3 * j + Histo->Offset[9];
  for (int j = 0; j < 201; j++)
    xc[400 + j] = 400 * 3 + 9 * j + Histo->Offset[9];
  if (gDirectory->GetList()->FindObject(name)) {
    histo = (TH1F *) gDirectory->GetList()->FindObject(name);
    histo->GetXaxis()->Set(600, xc);
  } else {
    histo = new TH1F(name, name, 600, xc);
    histo->SetLineColor(1);
    histo->SetStats(0);
  }
  for (int j = 0; j < 400; j++)
    histo->SetBinContent(j + 1, Histo->Charge[3][j]);
  for (int j = 0; j < 200; j++)
    histo->SetBinContent(j + 1 + 400, Histo->Charge[3][j + 400] / 3.);
  return histo;
}

IoSdStation::IoSdStation() {
  Error = 0;
  Id = 0;
  T2Life=-1;
  T2Life120=-1;
  AntennaFlag=0;
  IsUUB=0;
  Name = "Unknown";
  Easting = Northing = Altitude = 0.;
  Gps = NULL;
  Histo = NULL;
  HistoCoinc = NULL;
  Calib = NULL;
  UTrigParam = NULL;
  Fadc = NULL;
  UFadc = NULL;
  PMQuality = NULL;
  Antenna   = NULL;
 
}

IoSdStation & IoSdStation::operator =(const IoSdStation & st) {
  if (this == &st)
    return *this;
   Error = st.Error;
  Id = st.Id;
  Name = st.Name;
  Trigger = st.Trigger;
  Easting = st.Easting;
  Northing = st.Northing;
  Altitude = st.Altitude;
  T2Life120=st.T2Life120;
  T2Life=st.T2Life;
  AntennaFlag=st.AntennaFlag;
  IsUUB=st.IsUUB;
  for (int i=0;i<32;i++) Extra[i]=st.Extra[i];
  
  if (Gps) {
    delete Gps;
    Gps = NULL;
  }
  if (Histo) {
    delete Histo;
    Histo = NULL;
  }
  if (HistoCoinc) {
    delete HistoCoinc;
    HistoCoinc = NULL;
  }
  if (Calib) {
    delete Calib;
    Calib = NULL;
  }
  if (UTrigParam) {
    delete UTrigParam;
    UTrigParam = NULL;
  }
  if (Fadc) {
    delete Fadc;
    Fadc = NULL;
  }
  if (UFadc) {
    delete UFadc;
    UFadc = NULL;
  }
  if (st.Gps) {
    Gps = new IoSdGps;
    *Gps = *st.Gps;
  }
 if (st.PMQuality) {
    PMQuality = new IoSdPMQuality;
    *PMQuality = *st.PMQuality;
  }
 if (st.Antenna) {
   Antenna = new IoSdAntenna;
   *Antenna = *st.Antenna;
  }
  if (st.Calib) {
    Calib = new IoSdCalib;
    *Calib = *st.Calib;
  }
  if (st.UTrigParam) {
    UTrigParam = new IoUSdTrigParam;
    *UTrigParam = *st.UTrigParam;
  }

  if (st.Histo) {
    Histo = new IoSdHisto;
    *Histo = *st.Histo;
  }
  if (st.HistoCoinc) {
    HistoCoinc = new IoSdHistoCoinc;
    *HistoCoinc = *st.HistoCoinc;
  }
  if (st.Fadc) {
    Fadc = new IoSdFadc;
    *Fadc = *st.Fadc;
  }
  if (st.UFadc) {
    UFadc = new IoUSdFadc;
    *UFadc = *st.UFadc;
  }
  return *this;
}

IoSdStation::IoSdStation(const IoSdStation & st) {
  Error = st.Error;
  Id = st.Id;
  Name = st.Name;
  Trigger = st.Trigger;
  Easting = st.Easting;
  Northing = st.Northing;
  Altitude = st.Altitude;
  T2Life=st.T2Life;
  T2Life120=st.T2Life120;
  AntennaFlag=st.AntennaFlag;
  IsUUB=st.IsUUB;
  for (int i=0;i<32;i++) Extra[i]=st.Extra[i];
  Gps = NULL;
  PMQuality = NULL;
  Antenna=NULL;
  Calib = NULL;
  UTrigParam = NULL;
  Histo = NULL;
  HistoCoinc = NULL;
  Fadc = NULL;
  UFadc = NULL;
 if (st.Gps) {
    Gps = new IoSdGps;
    *Gps = *st.Gps;
  }
  if (st.PMQuality) {
    PMQuality = new IoSdPMQuality;
    *PMQuality = *st.PMQuality;
  }

  if (st.Antenna) {
    Antenna = new IoSdAntenna;
    *Antenna = *st.Antenna;
  }
 
  if (st.Calib) {
    Calib = new IoSdCalib;
    *Calib = *st.Calib;
  }
  if (st.UTrigParam) {
    UTrigParam = new IoUSdTrigParam;
    *UTrigParam = *st.UTrigParam;
  }

  if (st.Histo) {
    Histo = new IoSdHisto;
    *Histo = *st.Histo;
  }
  if (st.HistoCoinc) {
    HistoCoinc = new IoSdHistoCoinc;
    *HistoCoinc = *st.HistoCoinc;
  }
  if (st.Fadc) {
    Fadc = new IoSdFadc;
    *Fadc = *st.Fadc;
  }
  if (st.UFadc) {
    UFadc = new IoUSdFadc;
    *UFadc = *st.UFadc;
  }
}

IoSdStation::~IoSdStation() {
  if (Gps)
    delete Gps;
  if (Histo)
    delete Histo;
  if (HistoCoinc)
    delete HistoCoinc;
  if (Calib)
    delete Calib;
  if (UTrigParam)
    delete UTrigParam;
  if (Fadc)
    delete Fadc;
  if (UFadc)
    delete UFadc;
  if (PMQuality)
   delete PMQuality;
  if (Antenna)
   delete Antenna;
  char name[50];
  TList *l = gDirectory->GetList();
  for (unsigned int i = 0; i < kIoSd::NPMT; i++) {
    snprintf(name, 26, "Base PMT %u, LS %u", i + 1, Id);
    if (l->FindObject(name))
      delete l->FindObject(name);
    snprintf(name, 26, "Peak PMT %u, LS %u", i + 1, Id);
    if (l->FindObject(name))
      delete l->FindObject(name);
    snprintf(name, 26, "Charge PMT %u, LS %u", i + 1, Id);
    if (l->FindObject(name))
      delete l->FindObject(name);
    snprintf(name, 26, "Shape PMT %u, LS %u", i + 1, Id);
    if (l->FindObject(name))
      delete l->FindObject(name);
  }
  snprintf(name, 26, "Sum Charge LS %u", Id);
  if (l->FindObject(name))
    delete l->FindObject(name);
}


int  IoSdStation::IsRaining(){
  int nr=0;
    if(PMQuality->RainingMask<7) {
      for (unsigned int ipm = 0; ipm < kIoSd::NPMT; ipm++) {
      if(((PMQuality->RainingMask & (1 << ipm)) != (1 << ipm)))
	nr++;
    }
    return nr;
  }
  else return 0; 
  
}

int IoSdStation::HasRainingPMT(int ipm){
  int r=0;
     if(PMQuality->RainingMask<7) {
      if(((PMQuality->RainingMask & (1 << ipm)) != (1 << ipm)))
	r++;
      return r;
    }
    else return 0;
}

//**************************************************************************************************
// IoSdT2Trigger member functions
UInt_t IoSdT2Trigger::type() const {
  return Type;
}

UInt_t IoSdT2Trigger::window() const {
  return Window;
}

Int_t IoSdT2Trigger::offset() const {
  return Offset;
}

//**************************************************************************************************
// IoSdGps member functions
UInt_t IoSdGps::second() const {
  return Second;
}

Int_t IoSdGps::offset() const {
  return Offset;
}

Int_t IoSdGps::nanosecond() const {
  return (int) NanoSecond;
}



//**************************************************************************************************
// IoSdCalib member functions
const Short_t *IoSdCalib::stability() {
  return Stability;
}

const UShort_t *IoSdCalib::tubeok() {
  return TubeOk;
}

UShort_t IoSdCalib::ntubesok() const {
  return NTubesOk;
}

Float_t IoSdCalib::t1rate() const {
  return T1Rate;
}

Float_t IoSdCalib::t2rate() const {
  return T2Rate;
}

Float_t IoSdCalib::totrate() const {
  return TOTRate;
}

const Float_t *IoSdCalib::tdarate() {
  return TDARate;
}

const string * IoSdCalib::showevolution() {
  return ShowEvolution;
}






//**************************************************************************************************
// IoSdStation member functions
Int_t IoSdStation::error() const {
  return Error;
}

UInt_t IoSdStation::id() const {
  return Id;
}

string IoSdStation::name() const {
  if (Name.find("(Calib:") < Name.size())
    return Name.substr(0, Name.find("(Calib:") - 1);
  return Name;
}

Double_t IoSdStation::easting() const {
  return Easting;
}

Double_t IoSdStation::northing() const {
  return Northing;
}

Double_t IoSdStation::altitude() const {
  return Altitude;
}

IoSdT2Trigger IoSdStation::trigger() const {
  return Trigger;
}

IoSdGps *IoSdStation::gps() const {
  return Gps;
}

IoSdHisto *IoSdStation::histo() const {
  return Histo;
}

IoSdHistoCoinc *IoSdStation::histocoinc() const {
  return HistoCoinc;
}

IoSdCalib *IoSdStation::calib() const {
  return Calib;
}

 IoSdPMQuality *IoSdStation::pmquality() const {
   return PMQuality;
 }
IoSdAntenna *IoSdStation::antenna() const {
  return Antenna;
 }

IoSdFadc *IoSdStation::fadc() const {
  return Fadc;
}

int IoSdStation::TriggerType() {
  if (Trigger.IsTOT())
    return 3;
  else if  (IsTOT(0.2, 13, 120))
    return 3;
   else if (Trigger.IsTOTd())
    return 4;
  else if (Trigger.IsMoPS())
    return 5;
  else if (Trigger.IsT2Threshold())
    return 2;
  else if (Trigger.IsT1Threshold())
    return 1;
   else return 0;
}
  
bool IoSdStation::IsTOT() const {
  if (Trigger.IsTOT())
    return 1;
  if (Calib->Version < 13 || Id == 600 || Id == 610 || Id == 620 || Id == 630 || Id == 648 || Id == 625 || Id == 627) // Old calibration or test tanks
    return IsTOT(0.2, 13, 120);
  return 0;
}

bool IoSdStation::IsTOT(float threshold, int nbbins, int window) const {
  // Looking at FADC trace to get the information. Note: this is "kind of"
  // analysis. This info should be in the FADC trace. This is to be used
  // for old (<April 2004) data and/or to play with other TOT
  int nbtotbin = 0;
  float thresh[kIoSd::NPMT];
  for (unsigned int j = 0; j < kIoSd::NPMT; j++) {
    thresh[j] = rint(Calib->Base[j] + threshold * Calib->VemPeak[j]);
  }
  for (unsigned int i = 0; i < kIoSd::MAXSAMPLE; i++) {
    int ok = 0;
    for (unsigned int j = 0; j < kIoSd::NPMT; j++)
      if (Fadc->Trace[j][IoSdEvent::eHigh][i] > thresh[j])
        ok++;
    if (ok > 1)             // 2 fold
      nbtotbin++;
    if ((int) i >= window) {        // removing "old" bins for TOT outside of window
      ok = 0;
      for (unsigned int j = 0; j < kIoSd::NPMT; j++)
        if (Fadc->Trace[j][IoSdEvent::eHigh][i - window] >
            thresh[j])
          ok++;
      if (ok > 1)
        nbtotbin--;
    }
    //cerr << "#debug " << i << " " << nbtotbin << endl;
    if (nbtotbin >= nbbins)
      return 1;
  }
  return 0;
}

bool IoSdStation::CalibSaved() {
  return (Name.find("(Calib:") < Name.size());
}


double IoSdStation::TimeToT3(IoSdT3Trigger & t3) {
  return (Gps->NanoSecond / 1000.0 - t3.MicroSecond - Trigger.Offset);
}

double IoSdStation::Time(IoSdT3Trigger & t3) {
  if (fabs(1.*t3.Second - Gps->second()) <= 1)
    return (t3.Second - Gps->second()) * 1000000 +
           (t3.MicroSecond - Gps->nanosecond() * 0.001) + Trigger.Offset;
  else
    return -1000;
}


double IoSdStation::DistToSDP(IoSdT3Trigger & t3) {
  if (t3.IsFd())
    return fabs((t3.FdSiteEasting() - Easting) * sin(t3.SDPAngle) +
                (Northing - t3.FdSiteNorthing()) * cos(t3.SDPAngle));
  return -1;
}

//**************************************************************************************************
// IoSdSimul member functions
void IoSdSimul::Clear() {
  Primary = 0;
  Theta = 0;
  Phi = 0;
  E = 0;
  XMax = 0;
  XFirst = 0;
  CoreNorthing = 0;
  CoreEasting = 0;
  sprintf(MotherProgram, "Unknown");
  sprintf(ShowerName, "Unknown");

}

void IoSdSimul::Print(ostream & o) const {
  o << ShowerName << "(" << MotherProgram << ")  P=" << Primary << " E="
  << E << " TH=" << Theta << " Ph=" << Phi << " Xmx=" << XMax <<
  " X0=" << XFirst << " No=" << CoreNorthing << " Ea=" << CoreEasting
  << endl;
}

Int_t IoSdSimul::primary() const {
  return Primary;
}

Double_t IoSdSimul::theta() const {
  return Theta;
}

Double_t IoSdSimul::phi() const {
  return Phi;
}

Double_t IoSdSimul::e() const {
  return E;
}

Double_t IoSdSimul::xmax() const {
  return XMax;
}

Double_t IoSdSimul::xfirst() const {
  return XFirst;
}

Double_t IoSdSimul::corenorthing() const {
  return CoreNorthing;
}

Double_t IoSdSimul::coreeasting() const {
  return CoreEasting;
}

const char *IoSdSimul::motherprogram() {
  return MotherProgram;
}

const char *IoSdSimul::showername() {
  return ShowerName;
}







//**************************************************************************************************
// IoSdT3Trigger member functions
UInt_t IoSdT3Trigger::id() const {
  return Id;
}

UInt_t IoSdT3Trigger::previousid() const {
  return PreviousId;
}

UInt_t IoSdT3Trigger::numberofstation() const {
  return NumberOfStation;
}

UInt_t IoSdT3Trigger::second() const {
  return Second;
}

UInt_t IoSdT3Trigger::microsecond() const {
  return MicroSecond;
}

string IoSdT3Trigger::sender() const {
  return Sender;
}

string IoSdT3Trigger::algo() const {
  return Algo;
}

Float_t IoSdT3Trigger::sdpangle() const {
  return SDPAngle;
}

int IoSdT3Trigger::EyeId() {
  if (strstr(Algo.c_str(), "Los Leones"))
    return 1;
  if (strstr(Algo.c_str(), "Los Morados"))
    return 2;
  if (strstr(Algo.c_str(), "Loma Amarilla"))
    return 3;
  if (strstr(Algo.c_str(), "Coihueco"))
    return 4;
  if (strstr(Algo.c_str(), "HEAT"))
    return 5;
  if (strstr(Algo.c_str(), "Laser") != NULL
      || strstr(Algo.c_str(), "Hybrid") != NULL
      || strstr(Algo.c_str(), "Stereo") != NULL
      || strstr(Algo.c_str(), "Trio") != NULL
      || strstr(Algo.c_str(), "Quadri") != NULL
      || strstr(Algo.c_str(), "Penta") != NULL)
    return 100;
  return 0;
}

bool IoSdT3Trigger::IsFd() {
  return (EyeId() != 0);
}

double IoSdT3Trigger::FdSiteNorthing() {
  if (!IsFd())
    return -1;
  if (EyeId() == 1)
    return kIoSd::N_LL;
  else if (EyeId() == 2)
    return kIoSd::N_LM;
  else if (EyeId() == 3)
    return kIoSd::N_LA;
  else if (EyeId() == 4)
    return kIoSd::N_CO;
  else if (EyeId() == 5)
    return kIoSd::N_HEAT;
  cerr << "IoSdT3Trigger::FdSiteNorthing : error unknown Algo name: " <<
  Algo << endl;
  return -1;
}

double IoSdT3Trigger::FdSiteEasting() {
  if (!IsFd())
    return -1;
  if (EyeId() == 1)
    return kIoSd::E_LL;
  else if (EyeId() == 2)
    return kIoSd::E_LM;
  else if (EyeId() == 3)
    return kIoSd::E_LA;
  else if (EyeId() == 4)
    return kIoSd::E_CO;
  else if (EyeId() == 5)
    return kIoSd::E_HEAT;
  cerr << "IoSdT3Trigger::FdSiteEasting : error unknown Algo name: " <<
  Algo << endl;
  return -1;
}

//**************************************************************************************************
// IoSdEvent member functions
//

void IoSdEvent::__buildT3s() {
  char *sender;
  int n, pos, senderSize, nscan, nalgo;
  int id, nos, sec, micro;
  float sdp;
  char snd[256], algo[256];
  senderSize = strlen(Trigger.Sender.c_str());
  sender = (char *) malloc(senderSize + 1);
  char *initial = sender;
  strcpy(sender, Trigger.Sender.c_str());
  sscanf(sender, "Merger %d%n", &n, &pos);
  sender += pos;
  T3s.resize(n);
  for (int i = 0; i < n; i++) {
    pos = 0;
    nscan =
      sscanf(sender, "# %d %d %d %d %f %s %s %n", &id, &nos, &sec,
             &micro, &sdp, snd, algo, &pos);
    nalgo = strlen(algo);
    pos--;
    while (sender[pos] != '#')
      algo[nalgo++] = sender[pos++];
    sender += pos + 1;
    algo[nalgo - 1] = '\0'; //supress extra space in algo
    T3s[i].Id = id;
    T3s[i].NumberOfStation = nos;
    T3s[i].Second = sec;
    T3s[i].MicroSecond = micro;
    T3s[i].SDPAngle = sdp;
    T3s[i].Sender = snd;
    T3s[i].Algo = algo;
  }
  free(initial);
}

void IoSdEvent::_SetUp() {  
  // fills the values that are only available in memory...
  UTCTime = (time_t) kCdasUtil::UTCsecond(Trigger.Second);
  UTCDate = kCdasUtil::UTCDate(Trigger.Second);
  // Fixing IDs for 2012 CDAS crash
  if (UTCTime>1344604966 && Id<10000000) Id=Id+17000000;
  // special treatment for merged event to build the list of original T3s...
  if (strstr(Trigger.Sender.c_str(), "Merger") != NULL)
    __buildT3s();
  else
    T3s.push_back(Trigger);
  for (unsigned int i = 0; i < Trigger.NumberOfStation; i++) {
    if (Stations[i].Error==0 || Stations[i].Error==256) {
      IoSdGps *gps = Stations[i].Gps;
      // taking into account Offsets...
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
      gps->NanoSecond =
        (int) ((gps->Tick *
                (1000000000.0 + gps->NextST - gps->CurrentST)
                / gps->Next100) + gps->CurrentST +
               0.01 * (short) (gps->Offset & 0xffff))
        - 100 * (gps->TickFall == gps->Tick);
      if (!Stations[i].IsUUB) {
        if (!Stations[i].Calib)
          Stations[i].Calib = new IoSdCalib;
        Stations[i].Calib->ComputeParameters();
      }
      // redefining the trigger name as : First (+Secondary+...)   
      // P.B. 18-nov-2011
      string trigger_name = "Trigger ";
      int trigger_type;
      if (Stations[i].IsUUB)
        trigger_type = Stations[i].Trigger.Type1;
      else
        trigger_type = Stations[i].Trigger.Type;
      // first trigger
      if(trigger_type&1) trigger_name += "Th";
      if(trigger_type&2) trigger_name += "ToT";
      if(trigger_type&4) trigger_name += "ToTd";
      if(trigger_type&8) trigger_name += "MoPS";
      // secondary triggers (if any)
      int second_trigger_type;
      if (Stations[i].IsUUB) trigger_type = Stations[i].Trigger.Type2;
      else trigger_type = trigger_type>>8;
      if(second_trigger_type) {
        trigger_name += " (";
        if(second_trigger_type&1) trigger_name += "+Th";
        if(second_trigger_type&2) trigger_name += "+ToT";
        if(second_trigger_type&4) trigger_name += "+ToTd";
        if(second_trigger_type&8) trigger_name += "+MoPS";
        trigger_name += ")";
      }
      Stations[i].Trigger.Name = trigger_name;
    }
  }
}



void IoSdEvent::__fromRootToMem(IoSdEvent * from) {
  if (!from)
    return;
  (*from).Trigger.NumberOfStation = (*from).Stations.size();
  *this = *from;
  from->_rawEvent = counted_ptr < IoSdEvent > (0);    // because we just stored it in this->_rawEvent
  //filling additional values
  _SetUp();
}


IoSdEvent::IoSdEvent(EventPos pos, const char *option) {
  if (strcmp(option, "VERBOSE") == 0)
    cerr << "Building event" << endl;
  IoSdEvent *tmp = theRootInterface()->ReadEventFromFile(pos);
  __fromRootToMem(tmp);
  GetNumberOfErrorZeroStation();
 }

IoSdEvent::IoSdEvent(EventId id) {
  vector < IoSdKey > *keys = &theRootInterface()->Keys;
  unsigned int sz = keys->size();  
  int goodkey = -1;
  unsigned int i;
  QualityFlag=false;
  EasierFlag=false;
  MeteoFlag=false;
  
  for (i = 0; i < sz; i++)
    if ((*keys)[i].EvtId == id) {
      goodkey = i;
      break;
    }
  if (goodkey == -1)
    return;
  
  IoSdEvent *tmp = theRootInterface()->ReadEventFromFile((*keys)[i]);
  __fromRootToMem(tmp);
  GetNumberOfErrorZeroStation();
}

IoSdEvent::IoSdEvent(unsigned int  gps) {

  vector < IoSdKey > *keys = &theRootInterface()->Keys;
  unsigned int sz = keys->size();
  int goodkey = -1;
  unsigned int i;
  QualityFlag=false;
  EasierFlag=false;
  MeteoFlag=false;
  
  for (i = 0; i < sz; i++)
    if ((*keys)[i].EvtGps == gps) {
      goodkey = i;
      break;
    }
  if (goodkey == -1)
    return;
  
  IoSdEvent *tmp = theRootInterface()->ReadEventFromFileGps((*keys)[i]);
  __fromRootToMem(tmp);
  GetNumberOfErrorZeroStation();
}


IoSdEvent::~IoSdEvent() {}

EventId IoSdEvent::id() const {
  return AugerId;
}


IoSdT3Trigger IoSdEvent::trigger() const {
  return Trigger;
}

IoSdSimul *IoSdEvent::simul() const {
  return Simul;
}

IoSdMeteo  IoSdEvent::meteo() const {
  return Meteo;
}

IoSdEasier  IoSdEvent::easier() const {
  return Easier;
}


time_t IoSdEvent::utctime() const {
  return UTCTime;
}

string IoSdEvent::utcdate() const {
  return UTCDate;
}

UInt_t IoSdEvent::numberoft1stations() {
  UInt_t n = 0;
  unsigned int sz;
  sz = Stations.size();
  for (unsigned int i = 0; i < sz; i++) {
    if (Stations[i].Trigger.window() != 0 && Stations[i].error() == 0)
      n = n + 1;
  }
  return n;
}

UInt_t IoSdEvent::numberoft2stations() {
  UInt_t n = 0;
  unsigned int sz;
  sz = Stations.size();
  for (unsigned int i = 0; i < sz; i++) {
    if (Stations[i].Trigger.window() == 0)
      n = n + 1;
  }
  return n;
}

int IoSdEvent::FdT3Time(UInt_t * sec, UInt_t * micro, int eye) {
  IoSdT3Trigger *t3 = NULL;
  int nT3 = T3s.size();
  *sec = 0;
  *micro = 0;
  if (!Trigger.IsFd())
    return 0;
  if (nT3 == 1 || eye == 0)
    t3 = &(this->T3s[0]);
  else
    switch (eye) {
    case 1:
      for (int i = 0; i < nT3; i++)
        if (strstr(T3s[i].Algo.c_str(), "Leones") != NULL)
          t3 = &(T3s[i]);
      break;
    case 2:
      for (int i = 0; i < nT3; i++)
        if (strstr(T3s[i].Algo.c_str(), "Morados") != NULL)
          t3 = &(T3s[i]);
      break;
    case 3:
      for (int i = 0; i < nT3; i++)
        if (strstr(T3s[i].Algo.c_str(), "Amarillo") != NULL)
          t3 = &(T3s[i]);
      break;
    case 4:
      for (int i = 0; i < nT3; i++)
        if (strstr(T3s[i].Algo.c_str(), "Coihueco") != NULL)
          t3 = &(T3s[i]);
      break;
    case 5:
      for (int i = 0; i < nT3; i++)
        if (strstr(T3s[i].Algo.c_str(), "HEAT") != NULL)
          t3 = &(T3s[i]);
      break;
      
    }
  if (!t3)
    return 0;
  double dm;
  dm = t3->MicroSecond + Stations[0].Trigger.Offset
       + sqrt(pow(t3->FdSiteEasting() - Stations[0].Easting, 2)
              + pow(t3->FdSiteNorthing() - Stations[0].Northing,
                    2)) / kCdasUtil::CMICRO - kIoSd::TIMETOSTARTOFTRACE;
  *sec = t3->Second;
  if (dm >= 1e6) {
    (*micro) = (UInt_t) (dm - 1e6);
    (*sec) += 1;
  } else if (dm < 0) {
    (*micro) = (UInt_t) (dm + 1e6);
    (*sec) -= 1;
  } else
    *micro = (UInt_t) dm;
  // cout << t3->Second << " " << t3->MicroSecond << " "<< *sec << " "<< *micro << endl;
  return 1;
}

short int IoUSdFadc::GetValue(int pmt, int gain, int bin) {
  //pmt: 0,1,2: for WCD PMT
  //     3 -
  //     4: SSD PMT
  //     5: RD trace (when there are trace)
  if ((CodingVersion==3) &&
      (bin>=0) && (bin<NSample) &&
      (0<=pmt) && (pmt<6)){
    int g=(gain)?1:0;
    int pos;
    if(pmt<5){
      /* it is the trace of UUB */
      pos=bin+TraceStart;
      if (pos>=NSample)
        pos-=NSample;
      return (ntohs(Traces[2*pos+2*pmt*NSample+gain])&0xFFF);
    } else {
      /*RD trace */
      if(20480<Traces.size()){
        int adc=(int)((ntohs(Traces[2*pos+2*pmt*NSample+gain])>>1)&0x0FFF);
        return( (adc<2048) ? adc : adc - 4096 );
      } else
        return -9999;
    }
  }
  return -9998;
}

short IoUSdFadc::GetValueRd(int pmt, int gain, int bin) {
#warning - this function is deprecated, use GetValue instead
  if ((CodingVersion==3) && (Traces.size()>20480) && (bin>=0) && (bin<NSample)) {
    int pos=bin;
    int adc=(int)((ntohs(Traces[2*pos+2*pmt*NSample+gain])>>1)&0x0FFF);
    return ( adc > 2047 ) ? adc - 4096 : adc;
  } else return -9999;
}


bool IoUSdFadc::GetValueParity(int pmt, int gain, int bin) {
  if ((CodingVersion==3) && (Traces.size()>20480) && (bin>=0) && (bin<NSample)) {
    int pos=bin;
    return  __builtin_popcount(Traces[2*pos+2*pmt*NSample+gain])%2;
  } else
    return 0;
}
