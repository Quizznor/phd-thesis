// Pauls stuff
#include <algorithm>
#include <iomanip>

// stl
#include <iostream>
#include <vector>
#include <string>
#include <cstddef>
#include <functional>
#include <set>
#include <exception>
#include <fstream>
#include <vector>
#include <boost/filesystem.hpp>

// from offline
#include <RecEventFile.h>
#include <DetectorGeometry.h>
#include <RecEvent.h>

#include <SdRecShower.h>
#include <SdRecStation.h>
#include <FdRecShower.h>
#include <FdRecStation.h>
#include <RdRecShower.h>

#include <GenShower.h>
#include <Traces.h>
#include <TraceType.h>

#include <utl/Point.h>
#include <utl/UTMPoint.h>
#include <utl/ReferenceEllipsoid.h>
#include <utl/PhysicalConstants.h>
#include <utl/AugerUnits.h>
#include <utl/AugerCoordinateSystem.h>
#include <utl/CoordinateSystem.h>
#include <utl/CoordinateSystemPtr.h>

#include <boost/tuple/tuple.hpp>
#include <boost/tuple/tuple_io.hpp>

using namespace std;
using namespace utl;
namespace fs = boost::filesystem;

struct VectorWrapper
{
  vector<float> values;

  VectorWrapper(vector<float> input_values)
  {
    values = input_values;
  }

  VectorWrapper(int size, float initial_value)
  {
    vector<float> trace_container(size, initial_value);
    values = trace_container;
  }

  VectorWrapper convert_to_VEM()
  {
    // magic number 215 = conversion factor from ADC counts to VEM equivalent
    // see David's mail from 7/06/22 for more information on conversion factor
    return this->floor() / 215.9;
  }

  VectorWrapper floor()
  {
    vector<float> result;

    for (auto x = values.begin(); x < values.end(); x++)
    {
      result.push_back(std::floor(*x));
    }

    return VectorWrapper(result);
  }

  VectorWrapper operator * (const float factor)
  {
    vector<float> result;

    for (auto x = values.begin(); x < values.end(); x++)
    {
      result.push_back(*x * factor);
    }

    return VectorWrapper(result);
  }

  VectorWrapper operator / (const float factor)
  {
    vector<float> result;

    for (auto x = values.begin(); x < values.end(); x++)
    {
      result.push_back(*x / factor);
    }

    return VectorWrapper(result);
  }

  VectorWrapper operator + (const VectorWrapper trace)
  {
    vector<float> sum_of_both_vectors;

    for (unsigned long int i = 0; i < values.size(); i++)
    {
      sum_of_both_vectors.push_back(values[i] + trace.values[i]);
    }

    return VectorWrapper(sum_of_both_vectors);
  }

  VectorWrapper operator *= (float factor)
  {
    for (auto &value : this->values){value *= factor;}

    return *this;
  }

  vector<float> get_trace(int start, int end)
  {
    // the end bin should be values.begin() + end + 1 ? Keeping this for continuity
    const auto trace = std::vector<float>(values.begin() + start, values.begin() + end);
    return trace;
  }

};

void ExtractDataFromAdstFiles(fs::path pathToAdst)
{
  const auto csvTraceFile = pathToAdst.parent_path()/ pathToAdst.filename().replace_extension("csv");

  // (2) start main loop
  RecEventFile     recEventFile(pathToAdst.string());
  RecEvent*        recEvent = nullptr;

  // will be assigned by root
  recEventFile.SetBuffers(&recEvent);

  for (unsigned int i = 0; i < recEventFile.GetNEvents(); ++i) 
  {
    // skip if event reconstruction failed
    if (recEventFile.ReadEvent(i) != RecEventFile::eSuccess) continue;

    // allocate memory for data
    const SDEvent& sdEvent = recEvent->GetSDEvent();                              // contains the traces
    const GenShower& genShower = recEvent->GetGenShower();                        // contains the shower
    DetectorGeometry detectorGeometry = DetectorGeometry();                       // contains SPDistance
    recEventFile.ReadDetectorGeometry(detectorGeometry);

    // create csv file streams
    ofstream traceFile(csvTraceFile.string(), std::ios_base::trunc);

    // binaries of the generated shower
    // const auto SPD = detectorGeometry.GetStationAxisDistance(Id, Axis, Core);  // in m
    const auto showerZenith = genShower.GetZenith() * (180 / 3.141593);           // in °
    const auto showerEnergy = genShower.GetEnergy();                              // in eV
    const auto showerAxis = genShower.GetAxisSiteCS();
    const auto showerCore = genShower.GetCoreSiteCS();  

    Detector detector = Detector();

    // loop over all triggered stations
    for (const auto& recStation : sdEvent.GetStationVector())
    {
      const auto stationId = recStation.GetId();
      const auto SPD = detectorGeometry.GetStationAxisDistance(stationId, showerAxis, showerCore);  // in m

      const auto& traces = recStation.GetPMTTraces();
      for (const auto& trace : traces)
      {
        if (trace.GetType() != eTotalTrace) continue;
        if (trace.GetPMTId() == 4) continue;

        traceFile << showerEnergy << " " << showerZenith << " " << stationId << " " << SPD << " " << trace.GetPMTId();
        const auto& vemTrace = trace.GetVEMComponent();
        if (!vemTrace.size()) {
          for (int i=0; i<2048; i++) {
            traceFile << "0 ";
          }
        } else {
          for (const auto& bin : trace.GetVEMComponent())
          {
            traceFile << " " << bin;
          }
        }

        traceFile << "\n";
      }
    }

    traceFile.close();
  }
}

int main(int argc, char** argv) 
{
  ExtractDataFromAdstFiles(argv[1]);
  return 0;

}