#ifndef _io_CDASEventFile_h_
#define _io_CDASEventFile_h_

#include <io/VROOTFile.h>


namespace io {

 /**
   \class CDASEventFile

   \brief Interface for CDAS file reading

   \author Troy Porter
   \date 31 Jan 2004
   \ingroup cdas
  */

  class CDASEventFile : public VROOTFile {

  public:
    CDASEventFile() = default;

    CDASEventFile(const std::string& filename, const Mode mode = eRead, utl::Branch* const b = nullptr)
    { Open(filename, mode, b); }

    virtual ~CDASEventFile() = default;

    void Open(const std::string& filename, const Mode mode = eRead, utl::Branch* const b = nullptr) override;

    void Close() override;

    Status Read(evt::Event& event) override;

    void Write(const evt::Event& event) override;

    Status FindEvent(const unsigned int eventId) override;

    Status GotoPosition(const unsigned int position) override;

    int GetNEvents() override;

  private:
    int fCurrentEvent = -1;

  };

}


#endif
