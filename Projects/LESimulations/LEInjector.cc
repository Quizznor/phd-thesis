#include <sstream>
#include <vector>

#include <utl/config.h>

#include <det/Detector.h>

#include <sdet/SDetector.h>
#include <sdet/Station.h>

#include <evt/Event.h>


#include <fwk/CentralConfig.h>
#include <fwk/RandomEngineRegistry.h>
#include <io/CorsikaUtilities.h>

#include <sevt/SEvent.h>
#include <sevt/Header.h>
#include <sevt/Station.h>
#include <sevt/StationSimData.h>

#include <utl/AugerCoordinateSystem.h>
#include <utl/AugerUnits.h>
#include <utl/Math.h>
#include <utl/MathConstants.h>
#include <utl/Particle.h>
#include <utl/PhysicalConstants.h>
#include <utl/Point.h>
#include <utl/TimeStamp.h>
#include <utl/ErrorLogger.h>
#include <utl/Reader.h>
#include <utl/Vector.h>
#include <utl/GeometryUtilities.h>
#include <utl/MathConstants.h>

#include <CLHEP/Random/Randomize.h>

#include "LEInjector.h"

using namespace det;
using namespace evt;
using namespace fwk;
using namespace sevt;
using namespace utl;
// using namespace utl::GeometryUtilities;
using namespace std;
using CLHEP::RandFlat;
using namespace LEInjectorOG;


LEInjector::LEInjector() :
  fStationId(0),
  fInputStream(0),
  fVerticalMuonMode(false),
  fRandomEngine(0)
{ }


VModule::ResultFlag
LEInjector::Init()
{

  const Branch topB =
    CentralConfig::GetInstance()->GetTopBranch("LEInjector");

  topB.GetChild("StationId").GetData(fStationId);
  topB.GetChild("Filename").GetData(fFileName);
  topB.GetChild("VerticalMuonMode").GetData(fVerticalMuonMode);

  if (fVerticalMuonMode)
    INFO("Operating LE Injector in vertical muon mode!");


  fRandomEngine =
    &RandomEngineRegistry::GetInstance().
    Get(RandomEngineRegistry::eDetector).GetEngine();

  fInputStream = new ifstream(fFileName.c_str());
  if (!fInputStream->is_open()) {
    ostringstream msg;
    msg << "Unable to open the requested file : " << fFileName;
    ERROR(msg);
    throw IOFailureException("unable to open file");
  }
  return eSuccess;
}


VModule::ResultFlag
LEInjector::Run(Event& event)
{
  INFO("LEInjector::Run");
  if (!event.HasSEvent()) {
    ERROR("No Event. giving up");
    throw NonExistentComponentException("No SEvent present.");
  }


  SEvent& sEvent = event.GetSEvent();

  if (!sEvent.HasStation(fStationId)) 
   sEvent.MakeStation(fStationId);
  
  Station& station = sEvent.GetStation(fStationId);
  
  if (!station.HasSimData())
    station.MakeSimData();

  station.GetSimData().ClearParticleList();

  const sdet::Station& dStation = 
    det::Detector::GetInstance().GetSDetector().GetStation(station);
  const CoordinateSystemPtr stationCS = dStation.GetLocalCoordinateSystem();

  int particleId;
  double px;
  double py;
  double pz;
  double x;
  double y;
  double time;
  unsigned primaryShowerId;
  int primaryParticleId;
  double primaryEnergy;
  double primaryTheta;
  double primaryPhi;
 
  string line;

  do {

    if (!getline(*fInputStream, line)) {
      fInputStream->close();
      return eBreakLoop;
    }

    istringstream iss(line);

    if (!(iss >> particleId
              >> px >> py >> pz
              >> x >> y
              >> time
              >> primaryShowerId
              >> primaryParticleId
              >> primaryEnergy
              >> primaryTheta
              >> primaryPhi)){
      ERROR("Malformed line.");
      return eFailure;
    }

  } while (particleId == 201 || particleId == 301 || particleId == 402);  // for G4's sake skip Deuterium, Tritium or Helium 
  
  // fix up the units, particle id's
  particleId = io::Corsika::CorsikaToPDG(particleId);
  px *= GeV; py *= GeV; pz *= -GeV;
  x *= cm; y *= cm;
  time *= second;
  primaryParticleId = io::Corsika::CorsikaToPDG(primaryParticleId);
  primaryEnergy *= GeV;
  primaryTheta *= degree;
  primaryPhi *= degree;

  double pMomentum = sqrt(Sqr(px) + Sqr(py) + Sqr(pz)); // particle momentum (GeV)
  double pTheta = acos(-pz / pMomentum); // particle's zenith angle
  double pPhi = atan2(-py, -px); // particle's azimuth angle
   

  /*
  pPhi has to be modified. 
  Rotate form the Corsika coordinate system to Auger standard.
  Auger places the x axis east and the y axis north. Corsika places
  the x axis in the magnetic north and the y axis west. Therefore,
  the geomagnetic field direction for the location and time of an
  event has to be taken into account for the correct transformation.

  using io::Corsika::CorsikaAzimuthToAuger()
  */

  // change these values to also include SSD
  const double sThickness = dStation.GetThickness();
  const double sRadius = dStation.GetRadius() + sThickness;
  const double sHeight = dStation.GetHeight() + 2*sThickness;

  // Check if scintillator requires larger injection cylinder
  const double scintRadius = dStation.GetScintillator().GetMaxRadius();
  const double scintHeight = dStation.GetScintillator().GetMaxHeight();
  const auto& injectionRadius = std::max(sRadius, scintRadius);
  const auto& injectionHeight = std::max(sHeight, scintHeight);


  // Scintillator dimensions eventually will be replaced by actual SSD dimensions
  double scinLength = 1800*mm; 
  int nScinBars = 27;
  double barWidth = 80*mm;
  double scinWidth =  nScinBars*barWidth;

   
  /*
  We define a virtual cylinder of radius = sRadius and h = sHeight where the particles
  will be injected.

  From the particle's point of view, the "effective" area seen by the particle is a projection
  of the top area (circle) and a projection of the side area (cylinder)

  Then we compute the probability that the particle hits the top area dividing the value
  of the EffectiveTopArea by the total effective area (EffTop+EffSide).
  
  Then we "decide" which area is hit by the particle just throwing random numbers and comparing
  them with the probability defined above.

  If the particle hits the top area, we randomize its X,Y coordinates over a circle of radius sRadius
  and fixing Z = sHeight.

  If the particle hits the side area, we randomize Z between 0 and sHeight the and phi
  to give the position around the tank (with the radius fixed to sRadius)

  (These ideas were taken from CachedShowerRegenerator module).
  */

  if (fVerticalMuonMode) {
    // to inject vertical muons all over the scintillator
    const double x = scinLength/2 * RandFlat::shoot(fRandomEngine, -1, 1);
    const double y = scinWidth/2 * RandFlat::shoot(fRandomEngine, -1, 1);
   
    const Point position = Point(x, y, injectionHeight, stationCS);
    const Vector momentum = Vector(0, 0, -1*GeV, stationCS, Vector::kCartesian);
    const Particle particle = 
      Particle(13, utl::Particle::eBackground, position, momentum, 0, 1); // For this purpose particle's time is irrelevant 

    station.AddParticle(particle);
    
  } // in case of VerticalMuonMode = ON


  else {

    const double EffTopArea = kPi*Sqr(injectionRadius)*cos(pTheta); // pTheta is measured from the station reference plane
    const double EffSideArea = 2*injectionRadius*injectionHeight*sin(pTheta);
    const double ProbSeeTop = EffTopArea / (EffTopArea + EffSideArea); 
    const double rand1 = RandFlat::shoot(fRandomEngine, 0, 1);
    
    
    
    if (rand1 <= ProbSeeTop) { // Particle injected on the top

      const double rand2 = RandFlat::shoot(fRandomEngine, 0, 1);
      const double r = injectionRadius*sqrt(rand2);
      const double phi = RandFlat::shoot(fRandomEngine, 0, kTwoPi);
      
      const double x = r * cos(phi);
      const double y = r * sin(phi);
      const double z = injectionHeight;

      const Point position = Point(x, y, z, stationCS);
      const Vector momentum = Vector(px, py, pz, stationCS, Vector::kCartesian);
      const Particle particle = 
        Particle(particleId, utl::Particle::eBackground, position, momentum, 0, 1);

      station.AddParticle(particle);
    }
      
    else { // Particle injected on the side
      
      // For the X and Y component we need to find the angle between
      // the (tank) X-Y axis and the incoming particle direction. Then add
      // an angle sampled from an arcsin distribution
      

      const double rand2 = RandFlat::shoot(fRandomEngine, 0, 1);
      const double rand3 = RandFlat::shoot(fRandomEngine, 0, 1);
      
      const double phi = pPhi + asin(1-2*rand2);

      const double z = injectionHeight*rand3;
      const double x = injectionRadius * cos(phi);
      const double y = injectionRadius * sin(phi);

      const Point position = Point(x, y, z, stationCS);
      const Vector momentum = Vector(px, py, pz, stationCS, Vector::kCartesian);
      const Particle particle = 
        Particle(particleId, utl::Particle::eBackground, position, momentum, 0, 1);

      station.AddParticle(particle);
  
    }   
  }   
return eSuccess;
} // end  LEInjector::Run()
  


VModule::ResultFlag
LEInjector::Finish()
{
  delete fInputStream;
  return eSuccess;
}

// Configure (x)emacs for this file ...
// Local Variables:
// mode: c++
// End:
