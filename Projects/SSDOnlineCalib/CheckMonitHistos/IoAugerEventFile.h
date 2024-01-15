#ifndef _io_IoAugerEventFile_h_
#define _io_IoAugerEventFile_h_

#include <io/VEventFile.h>
#include <AugerFile.h>


namespace io {

  /**
    \class IoAugerEventFile IoAugerEventFile.h "io/IoAugerEventFile.h"

    \brief

    \author Stefano Argiro`
    \date 22 Jun 2004
    \ingroup cdas
  */

  class IoAugerEventFile : public VEventFile {

  public:
    IoAugerEventFile() { }
    IoAugerEventFile(const std::string& filename, const Mode mode = eRead, utl::Branch* const b = nullptr);
    virtual ~IoAugerEventFile();

    void Open(const std::string& filename, const Mode mode = eRead, utl::Branch* const b = nullptr) override;
    void Close() override;
    Status Read(evt::Event& event) override;
    void Write(const evt::Event& event) override;
    Status FindEvent(const unsigned int n) override;
    Status GotoPosition(const unsigned int n) override;
    int GetNEvents() override;

  private:
    IoAugerEventFile(const IoAugerEventFile&);
    IoAugerEventFile& operator=(const IoAugerEventFile&);

    AugerFile::Mode ConvertMode(const Mode m);
    AugerFile* fAugerFile = nullptr;

  };

}


#endif
