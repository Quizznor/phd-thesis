#ifndef IOSDDATA_H
#define IOSDDATA_H

#include <vector>
#include <string>
#include <time.h>
#include <stdio.h>
#include <iostream>
#include <sstream>
using namespace std;

#include "counted_ptr.h"
#include "TH1.h"
#include "TObject.h"

class AugerEvent;

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

/// IoSd constants
namespace kIoSd {
 /// Los Leones Coordinates Northing, Easting, Altitude
const int N_LL = 6071872;
const int E_LL  = 459208;
const int A_LL  =   1416;
  /// Coihueco Coordinates Northing, Easting, Altitude
const int N_CO = 6114142;
const int E_CO =  445346;
const int A_CO =    1714;
  /// Los Morados Coordinates Northing, Easting, Altitude
const int N_LM = 6094570; 
const int E_LM =  498902;
const int A_LM =    1415;
  /// Loma Amarilla Coordinates Northing, Easting, Altitude
const int N_LA = 6134066;
const int E_LA =  480747; 
const int A_LA =   1455;
  /// HEAT Coordinates Northing, Easting, Altitude
const int N_HEAT = 6114210;
const int E_HEAT =  445498; 
const int A_HEAT =   1707;
  /// Ids of stations in doublets
const unsigned int CARMEN  =  49;
const unsigned int MIRANDA =  64;
const unsigned int MOULIN  = 140;
const unsigned int ROUGE   = 185;
const unsigned int DIA     = 139;
const unsigned int NOCHE   = 186;
  /// Structural constansts
const unsigned int NPMT = 3;
const unsigned int MAXSAMPLE = 768;
const unsigned int NGAIN = 2;
const unsigned int NB_HISTO_CALIB = 10;
const unsigned int SINGLE_MUON_SIZE = 20;
const double TIMETOSTARTOFTRACE = 12.8; /* MicroSeconds */ ;
const unsigned int N_WEATHER_STATIONS=6;

  /// FD building coordinates in proper x,y,z frame
double FdX(int);
double FdY(int);
double FdZ(int);



}


typedef int EventPos;
typedef unsigned long long EventId;
typedef int KeyPos;

// forward definitiotns
class IoSdT3Trigger;

/// Header of the data file
class IoSdHeader:public TObject {
public:
  IoSdHeader() {}
  ~IoSdHeader() {}
  /// Begining of the data taking by Eb
  UInt_t startofrun() const {
    return StartOfRun;
  }
  UInt_t StartOfRun;
  /// End of the data taking by Eb
  UInt_t endofrun() const {
    return EndOfRun;
  }
  UInt_t EndOfRun;

  ClassDef(IoSdHeader, 1)
};

/// Surface detector station T2 trigger class
class IoSdT2Trigger {
public:
  IoSdT2Trigger() {
    Type = Window = Offset = 0;
    Name = "";
  }
  virtual ~ IoSdT2Trigger() {}
  UInt_t type() const;              ///< Returns trigger type (see PLD documentation)
  UInt_t window() const;            ///< Returns window around the trigger time
  Int_t offset() const;             ///< Returns window offset with respect of T3 time

  UInt_t Type;                ///< Trigger type, 16 bits of 2 8 bits words
  // For UUB, type has changed to 2 unsigned ints
  UInt_t Type1;
  UInt_t Type2;
  // FIXME these functions have to be adapted to UUB (if (Type==0) ... function of Type1 and Type2)
  bool IsTOT() const {
    return ((Type & 2) || ((Type >> 8) & 2));
  }
  bool IsT2Threshold() const {
    return (!Window && (Type & ( 0x1) || ((Type >> 8) & 0x1) ));
  }

  bool IsT1Threshold() const {
    return (Window && (Type & (0x1) || ( (Type >> 8) & 0x1) ));
  }

  bool IsTOTd() const {
    return ((Type & 4) || ((Type >> 8) & 4));
  }

  bool IsMoPS() const {
    return ((Type & 8) || ((Type >> 8) & 8));
  }

  UInt_t Window;              ///< Window around the trigger where stations should look for a trace to send back. Window=0 means that the station participated to the T3 trigger, an therefore has a T2
  Int_t Offset;               ///< Offset for the window. Contains the offset of the station with respect to the earliest one for Window=0 stations. Contains the average offset of all Window=0 stations for the others
  string Name;           ///< Trigger name.

  ClassDef(IoSdT2Trigger, 5)
};


/// GPS information of a tank.
class IoSdGps {
public:
  IoSdGps() {
    Second = Tick = TickFall = NanoSecond = 0;
    Current100 = Next100 = Current40 = Next40 = PreviousST =
                                         CurrentST = NextST = Offset = 0;
    IsUUB = 0;
  }
  virtual ~ IoSdGps() {}
  UInt_t second() const;            ///< Returns End of traces raw time
  Int_t offset() const;             ///< Returns GPS offset of this card compared to a reference
  Int_t nanosecond() const;         ///< Returns Trigger nanosecond time of end of trace fully corrected

  UInt_t Second;
  UInt_t Tick;                ///< 100 MHz Tick value of rising edge
  UInt_t TickFall;            ///< Tick of the falling edge of the trigger. For experts only
  /// Saw Tooth and Clock pulses information
  Int_t Current100;
  Int_t Next100;
  Int_t Current40;
  Int_t Next40;
  Int_t PreviousST;
  Int_t CurrentST;
  Int_t NextST;
  Int_t Offset;
  // Members that don't go to file
  Int_t NanoSecond;           //!

  // For UUB: FIXME, right now use "Second","Tick" and "TickFall" for second, ticks and ticks_prev_pps, but we are missing a lot of information (sawtooth...)
  // Adding a IsUUB at 0/1 to compute timing information properly, given the different frequencies, etc.
  int IsUUB;   ///< UUB GPS are different than UB ones (and clocks are different, 120MHz)
  
  ClassDef(IoSdGps, 3)
};

/// Surface detector Muon histograms
class IoSdHisto {
public:
  IoSdHisto() {}
  virtual ~ IoSdHisto() {}
  Int_t type; //1-> normal SSD histogram;
              //2 -> SSD working with SiPM
              //3 -> SiPM calibration histogram

  UShort_t Offset[kIoSd::NB_HISTO_CALIB];
  UShort_t Base[kIoSd::NPMT][20];     ///< Histogram of baseline
  UShort_t Peak[kIoSd::NPMT][150];    ///< Peak histogram
  UShort_t Charge[kIoSd::NPMT + 1][600];      ///< Charge histogram (4th histogram is for the sum of the PMTs)
  UInt_t Shape[kIoSd::NPMT][kIoSd::SINGLE_MUON_SIZE]; ///< Average muon shape
  // UUB: have 5 channels, but right now SPMT is not in slow buffers (normal, never has signal)
  // So we end up with 4 channels
  // 3 usual PMTs
  // SSD is like a normal PMT, but sharper
  // no sum histogram, as it is useless
  // FIXME: we need for the UUB:
  // 4 baseline histograms (we are missing the SSD right now)
  // 4 peak/charge histograms, with same binning as before (missing 1 peak, and eventually 1 charge)
  // 4 shapes with ~3 times the number of bins (Set in FPGA, currently 70), need to replace the Shape
  //
  // EA version: has extra UShape
  //
  Int_t UShape[4][70]; // currently only 69 are used...
  UShort_t Base3[20];  // SSD baseline histogram, would be Base[3][20] if ROOT allowed evolution...
  UShort_t Peak3[150];  // SSD peak histogram, would be Peak[3][20] if...
  UShort_t Offset3[3]; // SSD baseline offset, peak offset, charge offset

  float BaseAvg[4];

  Char_t Pk_bit_shift[4],Ch_bit_shift[4];
  UShort_t Pk_bin[2][4],Ch_bin[2][4];

  Int_t tStart,tEnd,Entries;
  ClassDef(IoSdHisto, 4)
};

/// Surface detector muon histograms with WCD-SSD coincidence condition
class IoSdHistoCoinc {
public:
  IoSdHistoCoinc();
  virtual ~ IoSdHistoCoinc() {}

  UShort_t HasHisto;
  UShort_t SSDThreshold;

  UShort_t Peak[kIoSd::NPMT][150];
  UShort_t Charge[kIoSd::NPMT][600];

  ClassDef(IoSdHistoCoinc, 1)
};

/// Surface detector tank Calibration class
class IoSdCalib {
public:
  IoSdCalib();
  virtual ~ IoSdCalib() {}
  const Short_t *stability();
  const UShort_t *tubeok();   ///< Returns Status of the tubes
  UShort_t ntubesok() const;        ///< Returns Number of valid tubes
  Float_t t1rate() const;
  Float_t t2rate() const;
  Float_t totrate() const;
  const Float_t *tdarate();
  const string * showevolution();        ///< Returns Evolution of the calibration

  int ComputeParameters();    ///< Initialises some usefull fields
  UShort_t Version;           ///< Version of the calibration data
  UShort_t TubeMask;          ///< Tells which tubes are working.
  UInt_t StartSecond;         ///< Beginning of the calibration data acquisition
  UInt_t EndSecond;           ///< End of the calibration data acquisition
  UShort_t NbT1;              ///< Numer of T1 trigger during calibration
  UShort_t NbT2;              ///< Numer of T2 trigger during calibration
  UShort_t NbTOT;             ///< Numer of TOT trigger during calibration
  UShort_t NbTOTD;
  UShort_t NbMOPS;
  UShort_t NbFullBuff;
  
  UShort_t NbTDA[kIoSd::NPMT];
  UShort_t Evolution[kIoSd::NPMT];
  Float_t Rate[kIoSd::NPMT];
  // Float_t SsdRate;
  // Float_t MipPeak;                    ///< MIP Peak in ADC counts estimated by the LS.
  // Float_t MipCharge;                  ///< MIP Charge estimated by the LS [[ NOT IMPLEMENTED YET! ]]
  Float_t VemPeak[kIoSd::NPMT];       ///< VEM Peak in ADC counts estimated by the LS. It is the one used for local triggering
  Float_t VemCharge[kIoSd::NPMT];     ///< VEM Charge estimated by the LS
  Float_t Base[2 * kIoSd::NPMT];      ///< Baselines of the 6 channels
  Float_t SigmaBase[2 * kIoSd::NPMT]; ///< Standard deviation of the baselines
  Float_t UBase[ 4 ];                 //sPMT, spare, ssd HG, ssd LG
  Float_t SigmaUBase[ 4 ];

  Float_t DA[kIoSd::NPMT];    ///< Dynode - Anode ratio
  Float_t SigmaDA[kIoSd::NPMT];       ///< Standard deviation of DA ratio
  Float_t DADt[kIoSd::NPMT];  ///< Dynode - Anode delay
  Float_t SigmaDADt[kIoSd::NPMT];     ///< Standard deviation of the DA delay
  Float_t DAChi2[kIoSd::NPMT];        ///< Chi2 of the fit

  // Members that don't go to file
  Short_t Stability[kIoSd::NPMT];     //!
  UShort_t TubeOk[kIoSd::NPMT];       //!
  UShort_t NTubesOk;          //!
  Float_t T1Rate;             //!
  Float_t T2Rate;             //!
  Float_t TOTRate;            //!
  Float_t TDARate[kIoSd::NPMT];       //!
  string ShowEvolution[kIoSd::NPMT];     //!

  // UUB: we need to rethink a bit all that
  //
  // EA: calibration will be hand made and very basic and none of this info is available, almost
  UInt_t NEntries; ///< Number of entries in calibration histograms
  ClassDef(IoSdCalib, 5)
};

//Surface detector PMQuality data class
class IoSdPMQuality{
public:
  IoSdPMQuality();
  virtual ~ IoSdPMQuality() {}
 
  Short_t Version;           ///< Version of the calibration data
  Short_t TubeMask;          ///< Tells which tubes are working.
  Short_t AnodeMask;          ///< Tells which anodes are working.
  Short_t RainingMask;
  
  ClassDef(IoSdPMQuality,1)
};

class IoSdAntenna{
public:
  IoSdAntenna();
  virtual ~ IoSdAntenna() {}
  Short_t Trigger;
  Short_t Type;           //Antenna type
  Short_t Candidate;
  Short_t IsDynodeSat;
  Short_t IsAnodeSat;
  Float_t SignalInVem;
  Float_t DistToAxis;
  Float_t Baseline;
  Float_t BaselineSigma;
  Float_t Peak;
  Float_t PeakTimeBin;
  Short_t Trace[kIoSd::MAXSAMPLE]; 
  ClassDef(IoSdAntenna,1)
 };

/// Surface detector tank Flash ADC class
class IoSdFadc {
public:
  IoSdFadc() {
    NSample = 0;
    for (unsigned int i = 0; i < kIoSd::NPMT; ++i)
      for (unsigned int j = 0; j < kIoSd::NGAIN ; ++j) 
	for (unsigned int k = 0; k < kIoSd::MAXSAMPLE; ++k)
	  Trace[i][j][k] = 0;
  }
  virtual ~ IoSdFadc() {}
  UInt_t NSample;             ///< Number of samples
  Short_t Trace[kIoSd::NPMT][kIoSd::NGAIN][kIoSd::MAXSAMPLE]; ///< Flash ADC traces [pmt(0-2)][high(dynode)/low(anode)][sample(0-767)]

  ClassDef(IoSdFadc, 1)
};

/// Upgraded Surface detector Flash ADC class
class IoUSdFadc {
public:
  IoUSdFadc() {
    NSample = 0;
    CodingVersion=0;
  }
  virtual ~ IoUSdFadc() {}
  UInt_t NSample;             ///< Number of samples
  UInt_t CodingVersion;       ///< How the FADC have been encoded
  UInt_t TraceStart;          ///< Internal, where the trace starts in the buffer
  UInt_t ShwrBufSt;
  UInt_t RdBufSt;
  vector <unsigned short> Traces;  ///< Buffer memory where all the FADC are, one after another
  short int GetValue(int pmt, int gain, int bin); ///< returns FADC value for PMT pmt (0-2: SD PMTs, 3: SPMT, 4: SSD PMT, 5:RD-when there is trace in the event.  To be checked)
  short GetValueRd(int pmt, int gain, int bin);//will be removed in the future
  bool GetValueParity(int pmt, int gain, int bin); ///< For RD: returns true if number of bits including parity is odd

  ClassDef(IoUSdFadc, 3)
};


class IoUSdTrigParam {
public:
  IoUSdTrigParam() {
    PL_version=0;
  }
  virtual ~ IoUSdTrigParam() {}

  UInt_t PL_version;
  UInt_t TrigMask;

  //Compatibility Single bin trigger
  UInt_t csbt_th[3];
  UInt_t csbt_enable;

  //Compatibility TOT
  UInt_t ctot_th[3];
  UInt_t ctot_enable;
  UInt_t ctot_occ;

  //Compatibility TOTD
  UInt_t ctotd_thmin[3];
  UInt_t ctotd_thmax[3];
  UInt_t ctotd_enable;
  UInt_t ctotd_occ;
  UInt_t ctotd_fd;
  UInt_t ctotd_fn;
  UInt_t ctotd_int;

  //Compatibility MOPS
  UInt_t cmops_thmin[3];
  UInt_t cmops_thmax[3];
  UInt_t cmops_enable;
  UInt_t cmops_occ;
  UInt_t cmops_ofs;
  UInt_t cmops_int;

  //Single bin trigger (full band width mode)
  UInt_t sbt_th[4]; // thresholds . 0-2: WCD PMTs; 3- SSD
  UInt_t sbt_enable;

  //Led flasher
  UInt_t led_ctrl;

  //random trigger.
  UInt_t random_mode;

  ClassDef(IoUSdTrigParam, 1)

};


/// Surface detector Station (tank) class
class IoSdStation {
public:
  IoSdStation();
  // Beware to modify these functions if you add new fields to the IoSdStation class
  IoSdStation(const IoSdStation & st);
  IoSdStation & operator =(const IoSdStation & st);
  // end of Beware
  virtual ~ IoSdStation();
  Int_t error() const;              ///< Returns the Error status of the tank. 0=has data
  UInt_t id() const;                ///< Returns the Id of the tank
  string name() const;         ///< Returns the Name of the tank
  Double_t easting() const;         ///< Easting of the tank (in meters)
  Double_t northing() const;        ///< Northing of the tank (in meters)
  Double_t altitude() const;        ///< Altitude of the tank (in meters, WGS84)
  IoSdT2Trigger trigger() const;    ///< Trigger information of the tank
  IoSdGps *gps() const;             ///< Returns the GPS block
  IoSdHisto *histo() const;         ///< Calib histograms
  IoSdHistoCoinc *histocoinc() const; ///< Calib histograms - WCD-SSD coincidence
  IoSdCalib *calib() const;         ///< Calibration block
  IoSdPMQuality *pmquality() const;     ///< pmquality data  block
  IoSdAntenna *antenna() const;     ///< pmquality data  block
  IoSdFadc *fadc() const;           ///< Flash ADC block
  //Histograms...
  TH1F *HBase(int) ;
  TH1F *HPeak(int) ;
  TH1F *HShape(int) ;
  TH1F *HCharge(int) ;
  TH1F *HSumCharge() ;

  int TriggerType();
  bool IsTOT() const;
  bool IsTOT(float threshold, int nbbins, int window) const;
  bool CalibSaved();
  /// Tells you if the station could have a 100ns jumps. If 0, time is
  /// correct. If 1, time can be NanoSecond or NanoSecond-100
  int CanHaveJumps() {
    return ((Calib->TubeMask & 0x8) == 0x8);
  }
  double TimeToT3(IoSdT3Trigger & t3);
  double Time(IoSdT3Trigger & t3);
  double DistToSDP(IoSdT3Trigger & t3);
  int IsRaining();
  int HasRainingPMT(int ipm);
  Int_t Error;
  UInt_t Id;
  Short_t T2Life;
  Short_t T2Life120;
  string Name;
  Double_t Easting;
  Double_t Northing;
  Double_t Altitude;
  IoSdT2Trigger Trigger;
  IoSdGps *Gps;
  IoSdHisto *Histo;
  IoSdHistoCoinc *HistoCoinc;
  IoSdCalib *Calib;
  IoSdFadc *Fadc;
  IoSdPMQuality *PMQuality;
  IoSdAntenna *Antenna;
  bool AntennaFlag;
  int HasASCII() { // old, returns 1 or 2 if there is one or two ASCIIs in PMT 1/2
    if (Gps->Second>1234396810) return 0; // New flags are not ASCII but SSD
    return Calib ? (Calib->TubeMask & 0x30)>>4 : 0;
  }
  int HasSSD(int expert=0) { // returns 1, 2 or 3 if there an SSD in PMT 1/2/3, the data are in channel HasSSD()-1; returns 0 otherwise
    // expert==0 (default): throws away data before March 17
    // expert==1 : throws away data from ASCII
    // expert==2 : let the expert do what he wants
    if ((!expert) && Gps && (Gps->Second<1236816000)) return 0;
    if ((expert==1) && Gps && (Gps->Second<1235854800) && ((Id==93)||(Id==688)||(Id==1570)||(Id==1574)||(Id==1622)||(Id==1764)||(Id==1773))) return 0;
    return Calib ? (Calib->TubeMask & 0x70)>>4 : 0;
  }

  bool HasRd() {
    if(IsUUB){
      if( 261 < Calib->Version ){
        if(UFadc != NULL)
          return(0<UFadc->RdBufSt);
        return(true);
      } else {
        return 0<Extra[31];
      }
    }
    return(false);
  }

  // For UUB
  int IsUUB;
  IoUSdFadc *UFadc;
  IoUSdTrigParam *UTrigParam ;
  unsigned int Extra[32]; // for future use
  ClassDef(IoSdStation, 8)
};

/// Surface detector T3 trigger class
class IoSdT3Trigger {
public:
  IoSdT3Trigger() {
    Id = 0;
    PreviousId = 0;
    NumberOfStation = 0;
    Second = 0;
    MicroSecond = 0;
    SDPAngle = 0;
    Sender = Algo = "";
  }
  virtual ~ IoSdT3Trigger() {}
  UInt_t id() const;                ///< Returns the trigger Id (CDAS internal)
  UInt_t previousid() const;        ///< Returns the Id of another trigger that contains the data for this event
  UInt_t numberofstation() const;   ///< Returns the number of stations participating to the trigger (with or without data)
  UInt_t second() const;            ///< second of the trigger
  UInt_t microsecond() const;       ///< microsecond of the trigger
  string sender() const;       ///< Name of Central Trigger application
  string algo() const;         ///< Pattern found for this trigger
  Float_t sdpangle() const;         ///< Shower Detector Plane angle at ground, for FD triggers

  int EyeId() ;
  bool IsFd() ;
  double FdSiteNorthing() ;
  double FdSiteEasting() ;

  /// Id of the trigger.
  UInt_t Id;
  /** Id of the FD trigger that contains the data for this event.
      This field is filled only in case of a hybrid event for which the
      surface detector alone also has a trigger. In this case, the station
      data is not resent and must be found in the event carrying the
      trigger Id = PreviousId. */
  UInt_t PreviousId;
  UInt_t NumberOfStation;
  UInt_t Second;
  UInt_t MicroSecond;
  string Sender;
  string Algo;
  Float_t SDPAngle;

  ClassDef(IoSdT3Trigger, 1)
};

class IoSdSimul;



class IoSdMeteo{
 public:
  /* enum MeteoSite {eCLF,  */
  /*              eLosLeones,  */
  /*              eLosMorados,  */
  /*              eLomaAmarilla,  */
  /*              eCoihueco,  */
   /* 		eBLS}; */
  
  IoSdMeteo(){
    for(unsigned int i=0;i<kIoSd::N_WEATHER_STATIONS;i++){
      Pressure[i]=-111;
      Temperature[i]=-111;
      Humidity[i]=-111;
      DayPressure[i]=-111;
      DayTemperature[i]=-111;
      DayHumidity[i]=-111;
    }
  } 
  virtual ~ IoSdMeteo() {}
  
  Float_t Pressure[kIoSd::N_WEATHER_STATIONS];
  Float_t Temperature[kIoSd::N_WEATHER_STATIONS];
  Float_t Humidity[kIoSd::N_WEATHER_STATIONS];
  Float_t DayPressure[kIoSd::N_WEATHER_STATIONS];
  Float_t DayTemperature[kIoSd::N_WEATHER_STATIONS];
  Float_t DayHumidity[kIoSd::N_WEATHER_STATIONS];
  
  
  ClassDef(IoSdMeteo,2)
};

class IoSdEasier{
 public:
  
  IoSdEasier(){
    IsT4=0;
    IsT5=0;
    xCore=0;
    yCore=0;
    SDEnergy=0;
    Theta=0;
    Phi=0;
    S1000=0;
    N_MHz=0;
    N_GHz=0;
    N_MHz_candidate=0;
    N_GHz_candidate=0;
    N_MHz_signal=0;
    N_GHz_signal=0;
    QualityLevel=0;
  }
  virtual ~ IoSdEasier() {}
  int IsT4;
  int IsT5;
  float xCore;
  float yCore;
  float SDEnergy;
  float Theta;
  float Phi;
  float S1000;
  int N_MHz;
  int N_GHz;
  int N_MHz_candidate;
  int N_MHz_signal;
  int N_MHz_time;
  int N_GHz_candidate;
  int N_GHz_signal;
  int N_GHz_time;
  int QualityLevel;
  ClassDef(IoSdEasier,1)
};


/// Surface detector event class
class IoSdEvent:public TObject {
  private:
   void __fromRootToMem(IoSdEvent * from);
   void __buildT3s();
  public:
  /// enums are now in IoSdEvent name space rather than global...
    /// No Error,T3 Lost,T3 Not Found,T3 Too Young,T3 Already,T1 stopped,Bad compressed data,Data Lost
  enum LSErrorTypes { eNoError, eT3Lost, eT3NotFound, eT3TooYoung,
			eT3Already, eT1Stopped, eBadCompress, eDataLost};
  enum Gain { eLow, eHigh };
    
    
  counted_ptr < IoSdEvent > _rawEvent;       //!
  IoSdEvent() {
    Id = 0;
    AugerId = 0;
    QualityFlag=false;
    EasierFlag=false;
    MeteoFlag=false;
    Simul = NULL;
    NumberOfErrorZeroStation = 0;
    UNumberOfErrorZeroStation = 0;
    UTCTime = 0;
 }
  IoSdEvent(EventPos pos, const char *options = "");
  IoSdEvent(EventId id);
  IoSdEvent(unsigned int gps);
  virtual ~ IoSdEvent();
  void _SetUp();              ///< To compute from the read data some of the members that are only available in memory representation
  /// Generates the key of the event as used for writing the Root file
  char *Key() {
    static char key[256];
  
    key[255] = 0;
    if (!AugerId)
      AugerId = Id;
    snprintf(key, 255, "%llu#%u#%s#%u#%u", AugerId,
	     numberoferrorzerostation(), Trigger.Algo.c_str(), Trigger.Second,
             Unumberoferrorzerostation());
    return key;
  }
  /// Returns the pointer to the inital event read on file, this one is written on output
  IoSdEvent *RawEvent() {
    if (RootClassName == "IoSdEvent")
      return _rawEvent.get();
    else
      return 0;
	}
  /// Returns the Id (the AugerId if it exits) of the event.
  EventId id() const;
  /// Returns the number of stations with error zero of the event.
  /// Should not be used anymore...
  UInt_t numberoferrorzerostation() const {
    return NumberOfErrorZeroStation;
  }
  UInt_t Unumberoferrorzerostation() const {
    return UNumberOfErrorZeroStation;
  }


  bool qualityflag() const {
    return QualityFlag;
  }
  bool easierflag() const {
    return EasierFlag;
  }
  void GetNumberOfErrorZeroStation() {
    Trigger.NumberOfStation = Stations.size();
    NumberOfErrorZeroStation = 0;
    UNumberOfErrorZeroStation = 0;
    for (unsigned int i = 0; i < Trigger.NumberOfStation; i++){
      if (Stations[i].Error == eNoError)
        NumberOfErrorZeroStation++;
      else if(Stations[i].Error == (0x100 | eNoError))
        UNumberOfErrorZeroStation++;
    }
  }
  IoSdT3Trigger trigger() const;    ///< T3 trigger block of the event
  IoSdSimul *simul() const;         ///< pointer to the IoSdSimul block
  IoSdMeteo meteo() const;         ///<  IoSdMeteo block of the event
  IoSdEasier easier() const;         ///<  IoSdMeteo block of the event
  time_t utctime() const;           ///< UTC Time of the event
  string utcdate() const;      ///< UTC Date of the event (text format)
  /// Returns the number of T1 stations. Corresponds to the number of stations with window!=0 and errorcode=0
  UInt_t numberoft1stations() ;
  /// Returns the number of T2 stations. Corresponds to the number of stations with window=0.
  UInt_t numberoft2stations() ;
  /// When read from an AugerFile the rawEvent pointer is in fact an AugerEvent
  AugerEvent *RawAugerEvent() {
    if (RootClassName == "AugerEvent")
      return (AugerEvent *) _rawEvent.get();
    else
      return 0;
  }
  /// Returns in sec and micro the T3 time difference to the Eye site
  int FdT3Time(UInt_t * sec, UInt_t * micro, int site = 0);
  
  
  UInt_t Id;
  UInt_t NumberOfErrorZeroStation;
  UInt_t UNumberOfErrorZeroStation; /*number of triggered station with UUB */
  bool QualityFlag;
  bool EasierFlag;
  bool MeteoFlag;
  IoSdT3Trigger Trigger;
  IoSdSimul *Simul;
  // New meteo block
  IoSdMeteo Meteo;
  IoSdEasier Easier;
  /// Vector of stations participating to the event.
  vector < IoSdStation > Stations;
  // Members that don't go to file
  time_t UTCTime;             //!
  string UTCDate;        //!
  /// List of T3 for merged events
  vector < IoSdT3Trigger > T3s;  //!
  /// List of Ids for merged events
  vector <unsigned int> Ids;  
  EventId AugerId;            //!
  /// The root event class that was read
  /// can be IoSdEvent or AugerEvent
  string RootClassName;  //!
  ClassDef(IoSdEvent, 9)
};
  
  

/// Surface detector Simulation class
class IoSdSimul {
public:
  IoSdSimul() {
    Primary = 0;
    Theta = 0;
    Phi = 0;
    E = 0;
    XMax = 0;
    XFirst = 0;
    CoreNorthing = 0;
    CoreEasting = 0;
  }
  virtual ~ IoSdSimul() {}
  /// Returns the Nature of the primary.
  Int_t primary() const;
  /// Returns the Zenith angle of the primary (in radians).
  Double_t theta() const;
  /// Returns the Azimuth of the primary (in radians).
  Double_t phi() const;
  /// Returns the Energy of the primary (in GeV).
  Double_t e() const;
  /// Returns the Atmospheric depth of the maximum developpement of the shower (in g.cm^-3).
  Double_t xmax() const;
  /// Returns the Atmospheric depth of the first interaction (in g.cm^-3).
  Double_t xfirst() const;
  /// Returns the Northing coordinate of the core of the shower (in meters).
  Double_t corenorthing() const;
  /// Returns the Easting coordinate of the core of the shower (in meters).
  Double_t coreeasting() const;
  /// Returns the Name of the programe that produced the shower : Aires or Corsika with version number.
  const char *motherprogram();
  /// Returns the Shower name
  const char *showername();
  /// Initializes the variables
  void Clear();
  /// Print function
  void Print(ostream & o) const;

  /// Nature of the primary.
  Int_t Primary;
  /// Zenith angle of the primary (in radians).
  Double_t Theta;
  /// Azimuth of the primary (in radians).
  Double_t Phi;
  /// Energy of the primary (in GeV).
  Double_t E;
  /// Atmospheric depth of the maximum developpement of the shower (in g.cm^-3).
  Double_t XMax;
  /// Atmospheric depth of the first interaction (in g.cm^-3).
  Double_t XFirst;
  /// Northing coordinate of the core of the shower (in meters).
  Double_t CoreNorthing;
  /// Easting coordinate of the core of the shower (in meters).
  Double_t CoreEasting;
  /// Name of the programe that produced the shower : Aires or Corsika with version number.
  char MotherProgram[50];
  /// Shower name
  char ShowerName[10];

  ClassDef(IoSdSimul, 1)
    };

/// Redefinition of operator << for class IoSdSimul
inline ostream & operator<<(ostream & o, IoSdSimul & h) {
  h.Print(o);
  return o;
}



#endif
