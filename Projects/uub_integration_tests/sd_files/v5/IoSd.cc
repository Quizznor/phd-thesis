#include "IoSd.h"

static EventPos s_pos = 0;

// ---------------------------------------------------------
IoSd::IoSd(const char *filename, const char *options) {
  if (!strcmp(options, "") || !strcmp(options, "r")) {
    theRootInterface()->Input(filename);
    IsInput = true;
  } else if (!strcmp(options, "w")) {
    IsInput = false;
    WFile.FileName = filename;
    WFile.File = new TFile(filename, "RECREATE");
    if (!(WFile.File) || WFile.File->IsZombie()) {
      cerr << "Impossible to open output file " << filename << endl;
      return;
    }
    WFile.File->SetCompressionLevel(2);
  } else
    cerr <<
    "IoSd(const char *filename, const char *options)  unsupported option : "
    << options << endl;
};

// ---------------------------------------------------------
IoSd::IoSd(vector < string > names) {
  unsigned int size = names.size();
  for (unsigned int i = 0; i < size; i++)
    theRootInterface()->Input((char *) names[i].c_str());
  IsInput = true;
};

// ---------------------------------------------------------
IoSd::IoSd(int n, char *fileNames[], const char *options) {
  for (int i = 0; i < n; i++)
    theRootInterface()->Input(fileNames[i]);
  IsInput = true;
  if (strcmp(options, "VERBOSE"))
    cerr << "Number of files: " << theRootInterface()->Files.size()
    << ", number of events: " << NumberOfEvents() << endl;
};


// ---------------------------------------------------------
void
IoSd::Close() {
  if (IsInput) {
    s_pos = 0;
    theRootInterface()->Reset();
  } else {
    WFile.File->Close();
  }
};

// ---------------------------------------------------------
IoSd::~IoSd() {
  Close();
};

// ---------------------------------------------------------
void
IoSd::SetCurrentPosition(EventPos pos) {
  s_pos = pos;
};

// ---------------------------------------------------------
EventPos IoSd::FirstEvent() {
  s_pos = 0;
  return s_pos;
};

// ---------------------------------------------------------
EventPos IoSd::NextEvent() {
  return (++s_pos < LastEvent())? s_pos : LastEvent();
};

// ---------------------------------------------------------
EventPos IoSd::PreviousEvent() {
  return (--s_pos >= 0) ? s_pos : 0;
};

// ---------------------------------------------------------
EventPos IoSd::LastEvent() {
  return (EventPos) theRootInterface()->NtotEntries;
};

// ---------------------------------------------------------
EventPos IoSd::NumberOfEvents() {
  return (EventPos) theRootInterface()->NtotEntries;
};

// ---------------------------------------------------------
KeyPos IoSd::FirstKey() {
  s_pos = 0;
  return s_pos;
};

// ---------------------------------------------------------
KeyPos IoSd::NextKey() {
  return (++s_pos < LastKey())? s_pos : LastKey();
};

// ---------------------------------------------------------
KeyPos IoSd::LastKey() {
  return (KeyPos) theRootInterface()->NtotEntries;
};

// ---------------------------------------------------------
KeyPos IoSd::NumberOfKeys() {
  return (KeyPos) theRootInterface()->NtotEntries;
};

// ---------------------------------------------------------
const char *IoSd::CurrentFileName() {
  //cerr<<"in IoSd "<<theRootInterface()->CurrentFileName()<<endl;
  //return theRootInterface()->CurrentFileName();
  if (IsInput)
    return theRootInterface()->CurrentFileName();
  else
    return WFile.FileName.c_str();
};

// ---------------------------------------------------------
void IoSd::WriteModified(IoSdEvent & ev, const char *options) {
  if (IsInput) {
    cerr << "Impossible to write in file !!! " << endl;
    return;
  }
  if (strcmp(options, "VERBOSE") == 0)
    cerr << "Writing file " << CurrentFileName() << " with event " <<
    ev.Key() << endl;
  WFile.File->cd();
  ev.Write(ev.Key());
};

// ---------------------------------------------------------
void IoSd::Write(IoSdEvent & ev, const char *options) {
  IoSdEvent *toWrite = &ev;
  if (IsInput) {
    cerr << "Impossible to write in file !!! " << CurrentFileName() <<
    " which is an input file !!! " << endl;
    return;
  }
  if (ev.RawEvent())
    toWrite = ev.RawEvent();
  if (strcmp(options, "VERBOSE") == 0)
    cerr << "Writing file " << CurrentFileName() << " with event " <<
    toWrite->Key() << endl;
  WFile.File->cd();
  toWrite->Write(toWrite->Key());
};

// ---------------------------------------------------------
void IoSd::DumpInAscii(const IoSdEvent & ev) {
  int nbStat = 0;
  for (unsigned int i = 0; i < ev.Trigger.NumberOfStation; i++) {
    if (ev.Stations[i].Error == 0)
      nbStat++;
  }

  cout << ev.Id << " " << ev.Trigger.Second << " " << nbStat << endl;
  for (unsigned int i = 0; i < ev.Trigger.NumberOfStation; i++) {
    // read and write your ASCII format here.
  }
};
