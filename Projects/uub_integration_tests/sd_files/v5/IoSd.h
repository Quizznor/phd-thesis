#ifndef IOSD_H
#define IOSD_H


#include "IoSdData.h"
#include "IoSdRootI.h"

/// Basic I/O class
class IoSd {
public:
  IoSd(const char *, const char *options = "");
  IoSd(int, char *[], const char *options = "");
  IoSd(vector < string >);
  ~IoSd();

  void Close();
  void SetCurrentPosition(EventPos pos);
  EventPos FirstEvent();
  EventPos NextEvent();
  EventPos PreviousEvent();
  EventPos LastEvent();
  EventPos NumberOfEvents();
  KeyPos FirstKey();
  KeyPos NextKey();
  KeyPos LastKey();
  KeyPos NumberOfKeys();

  vector < IoSdKey > *Keys() {
    return &theRootInterface()->Keys;
  };
  void DumpInAscii(const IoSdEvent &);
  /// Writes event to file, as it was when read, discarding any modification
  void Write(IoSdEvent &, const char *options = "");
  /// Writes event to file, as it is now, keeping modifications to the fields
  void WriteModified(IoSdEvent &, const char *options = "");

  const char *CurrentFileName();
  IoSdFile WFile;
  bool IsInput;
};


#endif
