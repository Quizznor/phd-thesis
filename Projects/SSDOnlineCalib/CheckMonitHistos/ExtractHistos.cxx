// Pauls stuff
#include <algorithm>

// stl
// #include <iostream>
// #include <vector>
// #include <string>
// #include <cstddef>
// #include <functional>
// #include <set>
// #include <exception>
// #include <fstream>
// #include <vector>
// #include <boost/filesystem.hpp>

// // from offline
// #include <RecEventFile.h>
// #include <DetectorGeometry.h>
// #include <RecEvent.h>

// #include <SdRecShower.h>
// #include <SdRecStation.h>
// #include <FdRecShower.h>
// #include <FdRecStation.h>
// #include <RdRecShower.h>

// #include <GenShower.h>
// #include <Traces.h>
// #include <TraceType.h>

// #include <utl/Point.h>
// #include <utl/UTMPoint.h>
// #include <utl/ReferenceEllipsoid.h>
// #include <utl/PhysicalConstants.h>
// #include <utl/AugerUnits.h>
// #include <utl/AugerCoordinateSystem.h>
// #include <utl/CoordinateSystem.h>
// #include <utl/CoordinateSystemPtr.h>

// #include <boost/tuple/tuple.hpp>
// #include <boost/tuple/tuple_io.hpp>

using namespace std;
using namespace utl;
namespace fs = boost::filesystem;

int ExtractDataFromAdstFiles(fs::path PathToAdst)
{
  return 1;
  // const auto csvTraceFile = pathToAdst.parent_path()/ pathToAdst.filename().replace_extension("csv"); // for testing
  // const auto csvTraceFile = PathToAdst.parent_path().parent_path() / PathToAdst.filename().replace_extension("csv");

  // // (2) start main loop
  // RecEventFile     recEventFile(PathToAdst.string());
  // RecEvent*        recEvent = nullptr;

  // // will be assigned by root
  // recEventFile.SetBuffers(&recEvent);
}


int main(int argc, char** argv)
{
  ExtractDataFromAdstFiles(argv[1]);

  return 0;
}