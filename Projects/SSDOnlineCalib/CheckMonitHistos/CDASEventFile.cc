#include <io/CDASEventFile.h>

#include <io/CDASToOfflineEventConverter.h>

#include <evt/Event.h>
#include <evt/Header.h>

#include <sevt/Header.h>
#include <sevt/SEvent.h>
#include <sevt/Station.h>

#include <utl/AugerCoordinateSystem.h>
#include <utl/ErrorLogger.h>
#include <utl/TimeStamp.h>
#include <utl/UTMPoint.h>
#include <utl/SaveCurrentTDirectory.h>

#include <config.h>

#include <IoSdData.h>
#include <AugerEvent.h>

#include <TKey.h>
#include <TFile.h>

#include <cstddef>
#include <iostream>
#include <sstream>

using namespace std;
using namespace io;
using namespace evt;
using namespace sevt;
using namespace utl;


void
CDASEventFile::Open(const std::string& filename, const Mode mode, utl::Branch* const b)
{
  Close();

  VROOTFile::Open(filename, mode, b);

  if (!fFile)
    throw utl::IOFailureException("Cannot create file for CDAS io");

  if (fFile->IsZombie())
    throw utl::IOFailureException("CDAS file in zombie io state");

  fCurrentEvent = 0;
}


void
CDASEventFile::Close()
{
  if (!fFile)
    return;

  VROOTFile::Close();

  delete fFile;
  fFile = nullptr;

  fCurrentEvent = -1;
}


void
CDASEventFile::Write(const evt::Event& event)
{
  if (fMode == eRead)
    throw utl::IOFailureException("Attempted write into read-only CDAS file");

  if (fFile) {

    if (!event.HasRawEvent() || !event.GetRawEvent().HasSd()) {

      INFO("No raw SD event to write");
      return;

    }

    IoSdEvent* const rawSEvent = &const_cast<evt::Event&>(event).GetRawEvent().Sd();

    const SaveCurrentTDirectory save;
    fFile->cd();
    rawSEvent->Write(rawSEvent->Key());
    ++fCurrentEvent;

  }
}


Status
CDASEventFile::Read(evt::Event& event)
{
  if (fMode == eWrite || fMode == eNew || fMode == eAppend) 
    throw utl::IOFailureException("Attempted read from write-only CDAS file");

  if (fFile) {

    const SaveCurrentTDirectory save;
    fFile->cd();

    TKey* const theKey = 
      dynamic_cast<TKey*>(fFile->GetListOfKeys()->At(fCurrentEvent));

    if (theKey) {

      const IoSdEvent* const theRawSdEvent =
        dynamic_cast<const IoSdEvent*>(fFile->Get(theKey->GetName()));

      if (theRawSdEvent) {

        // put the raw in
        if (!event.HasRawEvent())
          event.MakeRawEvent();

        if (event.GetRawEvent().HasSd())
          event.GetRawEvent().Sd() = *theRawSdEvent;
        else
          event.GetRawEvent().PushEvent(*theRawSdEvent);

        if (!event.HasSEvent())
          event.MakeSEvent();

        sevt::SEvent& sEvent = event.GetSEvent();

        const IoSdT3Trigger* const rawTrigger = &theRawSdEvent->Trigger;
        const unsigned int second = rawTrigger->Second;
        const unsigned int microSecond = 
          int(rawTrigger->MicroSecond*microsecond/nanosecond);
        const TimeStamp currentTime(second, microSecond);

        evt::Header& header = event.GetHeader();
        ostringstream id_str;
        if (!header.GetId().empty())
          id_str << header.GetId() << "__";
        id_str << "sd_" << theRawSdEvent->Id;

        sevt::Header& sHeader = sEvent.GetHeader();
        sHeader.SetId(theRawSdEvent->Id);
        sHeader.SetTime(currentTime);

        header.SetTime(currentTime);
        header.SetId(id_str.str());

        ++fCurrentEvent;

        // Remember to do this because we get handed a new copy of the object
        // when grabbing it from file. This is not documented within ROOT, so
        // be careful when doing this.

        delete theRawSdEvent;
        return eSuccess;

      }
    
    }

  }

  return eEOF;
}


Status
CDASEventFile::FindEvent(const unsigned int id)
{
  if (fFile) {

    const SaveCurrentTDirectory save;
    fFile->cd();
    fFile->Seek(0); 
    fCurrentEvent = 0;

    while (fCurrentEvent < fFile->GetNkeys()) {

      TKey* const theKey = 
        dynamic_cast<TKey*>(fFile->GetListOfKeys()->At(fCurrentEvent));

      if (theKey) {

        // The key is of the form id#?#.... so just take the number before
        // the first '#' to get the event id.

        const string keyName(theKey->GetName()); 
        const string eventNumber(keyName.substr(keyName.find_first_of("#")));

        if ((unsigned long)atol(eventNumber.c_str()) == id)
          return eSuccess;

        ++fCurrentEvent;
      
      }
    
    }

  }

  return eEOF;
}


Status
CDASEventFile::GotoPosition(const unsigned int position)
{
  if (fFile) {

    if (position >= (unsigned int)fFile->GetNkeys())
      return eEOF;

    fFile->Seek(position);
    fCurrentEvent = position;
    return eSuccess;
  
  }

  return eEOF;
}


int
CDASEventFile::GetNEvents()
{
  return fFile->GetNkeys();
}
