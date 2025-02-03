#ifndef IOSDROOTI_H
#define IOSDROOTI_H

#include <vector>
#include <string>
#include "TFile.h"
#include "TKey.h"
#include "TList.h"

#include <IoSdData.h>


#include "IoSdFile.h"

/// ROOT interface used internally to map events from disk to memory
class IoSdRootI {
private:
  IoSdEvent * (*_Reader) (TFile * f, IoSdKey & k);
public:
  IoSdRootI() {
    NtotEntries = 0;
    CurrentFileId = -1;
  }
  void Input(const char *);
  const char *CurrentFileName();
  void DeleteFile(int);
  void Reset();
  IoSdEvent *ReadEventFromFile(IoSdKey &);
   IoSdEvent *ReadEventFromFileGps(IoSdKey &);
  IoSdEvent *ReadEventFromFile(string);
  IoSdEvent *ReadEventFromFile(EventPos pos = 0);
  IoSdEvent *ReadEventFromFile(unsigned int);
   IoSdEvent *ReadEventFromFileGps(unsigned int);
  static IoSdRootI *Instance();
  static IoSdRootI *instance;

  unsigned int NtotEntries;
  vector < IoSdFile > Files;
  vector < IoSdKey > Keys;
  int CurrentFileId;
  void SetReader(IoSdEvent * (*reader) (TFile * f, IoSdKey & k)) {
    _Reader = reader;
  }
};

IoSdRootI *theRootInterface();

#endif
