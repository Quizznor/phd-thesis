#ifndef IOSDFILE_H
#define IOSDFILE_H

#include <string.h>
#include <TFile.h>
#include <IoSdData.h>
#include <TKey.h>

/// A key to retrive objects in sd files. Has id, multiplicity, and trigger algorithm
class IoSdKey {
public:
  IoSdKey() {
    Name = "";
    EvtId = 0;
    Multiplicity = 0;
    EvtGps=0;
    T3Algo = "";
    FileNumber = -1;
  };
  IoSdKey(TKey *, int filenumber);

  string ClassName;
  string Name;
  int FileNumber;
  EventId EvtId;
  unsigned int EvtGps;
  unsigned int Multiplicity;
  string T3Algo;
};


/// a sd_ file, used internally by IoSd
class IoSdFile {
public:
  IoSdFile() {
    File = NULL;
    FileName = "";
    Nentries = 0;
  };
  IoSdFile(const char *);
  IoSdFile(const IoSdFile &);
  ~IoSdFile();

  TFile *File;
  string FileName;
  int Nentries;
};

#endif
