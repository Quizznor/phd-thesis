// std library
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <memory>
#include <iterator>
#include <chrono>
#include <string>
#include <cmath>

// offline includes
#include <RecEventFile.h>
#include <RecEvent.h>
#include <SDEvent.h>
#include <SdRecShower.h>
#include <AnalysisConsts.h>
#include <SdRecStation.h>
#include <MDEventADST.h>
#include <MdRecCounter.h>
#include <MdSimCounter.h>
#include <MdRecModule.h>
#include <MdSimScintillator.h>
#include <MdRecChannel.h>

#include <GenShower.h>
#include <GenStation.h>
#include <DetectorGeometry.h>
#include <Traces.h>

// 3rd party libraries
#include "json.hpp"
#include <TFile.h>
#include <TMath.h>
#include <TGraph.h>

using json = nlohmann::json;
/*
#############
## HELPERS ##
#############
*/

class Timer
{
public:
    Timer()
    {	
    	// start timing at instantiation
        m_StartTimePoint = std::chrono::high_resolution_clock::now();
   
    }
    
    ~Timer()
    {	
    	// destructor stops timing at the end of the scope
        Stop();
    }

    void Stop()
    {
        auto endTimePoint = std::chrono::high_resolution_clock::now();
        
        auto start = std::chrono::time_point_cast<std::chrono::microseconds>(m_StartTimePoint).time_since_epoch().count();
        auto end = std::chrono::time_point_cast<std::chrono::microseconds>(endTimePoint).time_since_epoch().count();
        
        // time interval in microsecs
        auto duration = end - start;
        // time interval in milisecs
        double ms = duration * 0.001;
        // print time interval to console
        std::cout << "The analysis took " <<duration << "us (" << ms << "ms)\n";
    }

private:
    std::chrono::time_point<std::chrono::high_resolution_clock> m_StartTimePoint;
};


void usage() {

std::cerr <<  "\n  Usage: analysis <Index> <OutputDir> <ADSTPath1> <ADSTPath2> ... <ADSTPathN> \n"
<<  "         Index is the path to a .csv file. \n"
<<  "         OutputDir is the place to dump th JSON files. \n"
<<  "         <ADSTfilePath> is the path to an ADST, e.g. ADST.root or a list of ADSTs' locations. \n" 
<<  "          \n " <<std::endl;

}

std::string categorizeEnergy(double number) {
    if (number >= 16.0 && number < 16.5) {
        return "16.0_16.5";
    } else if (number >= 16.5 && number < 17.0) {
        return "16.5_17.0";
    } else if (number >= 17.0 && number <= 17.5) {
        return "17.0_17.5";
    } else if (number >= 17.5 && number <= 18.) {
        return "17.5_18.0";
    } else if (number >= 17.5 && number <= 18.) {
        return "18_18.5";
    } else {
        return "Number is not in the specified range";
    }
}

void printWrappedString(const std::string& input) {
    // Output the top row of "#" symbols
    std::cout << std::string(input.length() + 6, '#') << std::endl;

    // Output the wrapped string with "#" symbols on each side
    std::cout << "## " << input << " ##" << std::endl;

    // Output the bottom row of "#" symbols
    std::cout << std::string(input.length() + 6, '#') << std::endl;
}


std::string formatNumber(int number, int spaces) {
    // Using std::setw to set the width of the output and std::setfill to fill with '0'
    std::ostringstream oss;
    oss << std::setw(spaces) << std::setfill('0') << number;
    return oss.str();
}

// Function to join paths (simple implementation)
std::string join_paths(const std::string& dir, const std::string& file) {
    if (dir.back() == '/' || dir.back() == '\\') {
        return dir + file;
    } else {
        return dir + "/" + file;
    }
}

double GetSPHeight(double spDistance, double zenith, double azimuthSP) {
    return std::cos(azimuthSP) * std::tan(zenith) * spDistance;
}

/*
##########
## MAIN ##
##########
*/


int main(int argc, char** argv)
{   
    {   

        Timer timer;
	
	    // vector to store paths for ADST files
	    std::vector<std::string> dataFilePaths;
	    // event counter
	    unsigned long nEvents(0);
        // index and outputDir paths
        std::string indexPath = argv[1]; 
        std::string outputDir = argv[2];
	
	    // store paths in vec
	    for (int i = 3; i < argc; ++i) {
            dataFilePaths.push_back(argv[i]);
        }
           // Event
	    RecEvent* theRecEvent = new RecEvent();
	    // Detector 
	    DetectorGeometry* theGeometry = new DetectorGeometry();
	    // Info 
	    FileInfo fileInfo;
	    // create RecEventFile
	    RecEventFile dataFile(dataFilePaths);
	    // Get Event 
	    dataFile.SetBuffers(&theRecEvent);
	    // Get Detector 
	    dataFile.ReadDetectorGeometry(*theGeometry);
	    // Get Info 
	    dataFile.ReadFileInfo(fileInfo);
	    // Get SDEvent 
	    SDEvent& theSdEvent = theRecEvent->GetSDEvent();
        // Get MDEvent
        MDEvent& theMdEvent = theRecEvent->GetMDEvent();
        
        // index file
        std::ofstream index;
        index.open(indexPath, std::ios::app);

        while (dataFile.ReadNextEvent() == RecEventFile::eSuccess){

            json jsonSerializer;

            /*
            ########
            ## Id ##
            ########
            */

            int eventId2 = theSdEvent.GetEventId();

            int atmModel = static_cast<int>(eventId2/pow(10,8));
            jsonSerializer["atm_model"] = atmModel;
            int showerId = static_cast<int>((eventId2 - atmModel * pow(10,8))/pow(10,4));
            jsonSerializer["shower_id"] = showerId;
            int useId = static_cast<int>(eventId2 - atmModel * pow(10,8) - showerId * pow(10,4) - 100);
            jsonSerializer["use_id"] = useId;
            bool isSaturated = theSdEvent.IsSaturated();
            jsonSerializer["isSaturated"]= isSaturated;

            std::string eventId = theRecEvent->GetEventId();
            std::string sizeLabel = theSdEvent.GetSdRecShower().GetShowerSizeLabel();

            /*
            ##############
            ## Triggers ##
            ##############
            */

            std::string trigger = theSdEvent.GetTriggerAlgorithm();
            jsonSerializer["trigger"] =  trigger;
            bool isT5 = theSdEvent.IsT5();
            jsonSerializer["isT5"] = isT5;
            //bool is5T5 = theSdEvent.Is5T5();
            //jsonSerializer["is5T5"] = is5T5;
            bool is6T5 = theSdEvent.Is6T5();
            jsonSerializer["is6T5"] = is6T5;
            bool bLDF = theSdEvent.HasLDF();
            jsonSerializer["bLDF"] = bLDF;
            unsigned int hottestId = theSdEvent.GetT5HottestStation();
            jsonSerializer["hottest_id"] = hottestId;
            unsigned int closestId = theSdEvent.GetT5ClosestStation();
            jsonSerializer["nearest_id"] = closestId;
            int nCandidates = 0;

            /*
            #################
            ## Event Level ##
            #################
            */

            // MC quantities

            const GenShower& sdGenShower = theRecEvent->GetGenShower();
            std::string primaryName = sdGenShower.GetPrimaryName();
            primaryName.erase(std::remove(primaryName.begin(), primaryName.end(), ' '));
            jsonSerializer["primary"] = primaryName;

            std::string eBin = categorizeEnergy(log10(sdGenShower.GetEnergy()));
            std::string oFileName = primaryName + "_" + eBin +  "_"; // energy bin
            oFileName += formatNumber(atmModel, 2) + formatNumber(showerId, 4); // shower id
            oFileName += "_" + formatNumber(useId, 2); // use id
               
            std::cout << std::endl;
            printWrappedString("Processing " + oFileName);
            std::cout << std::endl;

            double energyMC = sdGenShower.GetEnergy();
            jsonSerializer["energy_MC"] = energyMC;
            double zenithMC = sdGenShower.GetZenith();
            jsonSerializer["zenith_MC"] = zenithMC;
            jsonSerializer["azimuth_MC"] = sdGenShower.GetAzimuth();
            jsonSerializer["X0"] = sdGenShower.GetX0();
            jsonSerializer["X1"] = sdGenShower.GetX1();
            double Xmax = sdGenShower.GetXmaxGaisserHillas();
            jsonSerializer["Xmax"] = Xmax; 
            jsonSerializer["Xmax_interpolated"] = sdGenShower.GetXmaxInterpolated();
            jsonSerializer["Nmax"] = sdGenShower.GetNmaxGaisserHillas();
            const double muonNumber = sdGenShower.GetMuonNumber();
            jsonSerializer["muon_number"] = muonNumber;
            const double electromagneticEnergy = sdGenShower.GetElectromagneticEnergy();
            jsonSerializer["electromagnetic_energy"] = electromagneticEnergy;

            const TVector3& coreSiteMC = sdGenShower.GetCoreSiteCS();
            const TVector3 simAxisCoreCS = sdGenShower.GetAxisCoreCS();

            jsonSerializer["coreX_MC"] = coreSiteMC.x();
            jsonSerializer["coreY_MC"] = coreSiteMC.y();
            jsonSerializer["coreZ_MC"] = coreSiteMC.z();
            jsonSerializer["coreTimeSecond_MC"] = sdGenShower.GetCoreTimeSecond();
            jsonSerializer["coreTimeNSecond_MC"] = sdGenShower.GetCoreTimeNanoSecond();

            // Rec quantities
            const SdRecShower& sdRecShower = theSdEvent.GetSdRecShower();

            jsonSerializer["energy"] = sdRecShower.GetEnergy();
            jsonSerializer["zenith"] = sdRecShower.GetZenith();
            jsonSerializer["azimuth"] = sdRecShower.GetAzimuth();
            jsonSerializer["seed_stations"] = sdRecShower.GetSeedStations(); 

            const TVector3 coreSite = sdRecShower.GetCoreSiteCS();
            const TVector3 recAxisCoreCS = sdRecShower.GetAxisCoreCS();

            jsonSerializer["coreX"] = coreSite.x();
            jsonSerializer["coreY"] = coreSite.y();
            jsonSerializer["coreZ"] = coreSite.z();
            jsonSerializer["coreTimeSecond"] = sdRecShower.GetCoreTimeSecond();
            jsonSerializer["coreTimeNSecond"] = sdRecShower.GetCoreTimeNanoSecond();
            jsonSerializer["curvature"] = sdRecShower.GetCurvature();
            jsonSerializer["curvatureError"] = sdRecShower.GetCurvatureError();
            double showerSize = sdRecShower.GetShowerSize();
            jsonSerializer["showerSize"] = showerSize;
            double coreTime = sdRecShower.GetCoreTimeSecond() + sdRecShower.GetCoreTimeNanoSecond();

            // Errors
            jsonSerializer["energy_error"] = sdRecShower.GetEnergyError();
            jsonSerializer["zenith_error"] = sdRecShower.GetZenithError();
            jsonSerializer["azimuth_error"] = sdRecShower.GetAzimuthError();
            jsonSerializer["CoreX_error"] = sdRecShower.GetCoreEastingError();
            jsonSerializer["CoreX_error"] = sdRecShower.GetCoreNorthingError();
            jsonSerializer["corr_CoreXCoreY_error"] = sdRecShower.GetCoreNorthingEastingCorrelation();
            double showerSizeError = sdRecShower.GetShowerSizeError();
            jsonSerializer["showerSize_error"] = showerSizeError;

            /*
            ###################
            ## Station Level ##
            ###################
            */
           
            const std::vector<SdRecStation>& sdRecStationVector = theSdEvent.GetStationVector();
            const int timeBins =  120;
            float vemTraceAv[timeBins];
            double sbObs = 0;

            // loop trough stations : get all necessary infos
            for (unsigned int j = 0; j < sdRecStationVector.size(); ++j ) {
                const int stationId = sdRecStationVector[j].GetId();
                const std::string sdId = "sd_" + std::to_string(stationId);

                /*
                ########
                ## SD ##
                ########
                */
                
                // set status and saturation flag
                std::string status = "";
                if (sdRecStationVector[j].IsCandidate())
                    status = "Cand";
                else if (sdRecStationVector[j].IsSilent())
                    status = "Sile";
                else if (sdRecStationVector[j].IsRejected())
                    status = "Reje";
                else
                    status = "None";
                
                int stSatFlag = -1;
                if (sdRecStationVector[j].IsLowGainSaturated())
                    stSatFlag = 2;
                else if (sdRecStationVector[j].IsHighGainSaturated())
                    stSatFlag = 1;
                else
                    stSatFlag = 0;
                
                jsonSerializer[sdId]["status"] = status;
                jsonSerializer[sdId]["satFlag"] = stSatFlag;

                //std::cout << "Extracting information from Station " << stationId;

                // skip all the imaginary stations from Dense Rings
                if (9000 < stationId)
                    continue;

                // saturation is not properly simulated, should I use it?(?)
                bool bLowGainSaturated = sdRecStationVector[j].IsLowGainSaturated();
                //if (bLowGainSaturated) {
                //    std::cout << ": station is low-gain satured at " << sdRecStationVector[j].GetSPDistance() << "m from the shower axis.";
                //}

                // WE ARE ONLY KEEPING CANDIDATE STATIONS. REJECTED AND SILENT STATIONS WILL NOT BE INCLUDED
                if (!sdRecStationVector[j].IsCandidate()) {
                //    std::cout << ": station is not a candidate." << std::endl;
                    continue;
                }
                ++nCandidates;
                //std::cout << std::endl;
                
                
                // get signal registered by the station
                double totalSignal = bLowGainSaturated == true ? sdRecStationVector[j].GetRecoveredSignal() : sdRecStationVector[j].GetTotalSignal();
                if (totalSignal <= 0 && bLowGainSaturated)
                    totalSignal = sdRecStationVector[j].GetTotalSignal();

                jsonSerializer[sdId]["totalSignal"] = totalSignal;
                jsonSerializer[sdId]["totalSignalError"] = sdRecStationVector[j].GetTotalSignalError();
                jsonSerializer[sdId]["seconds"] = sdRecStationVector[j].GetTimeSecond();
                jsonSerializer[sdId]["nanoSeconds"] = sdRecStationVector[j].GetTimeNSecond();
                jsonSerializer[sdId]["timeVariance"] = sdRecStationVector[j].GetTimeVariance();
                double stationTime = sdRecStationVector[j].GetTimeSecond() + sdRecStationVector[j].GetTimeNSecond();

                const TVector3& detectorPosition = theGeometry->GetStationPosition(stationId);

                jsonSerializer[sdId]["x"] = detectorPosition.x();
                jsonSerializer[sdId]["y"] = detectorPosition.y();
                jsonSerializer[sdId]["z"] = detectorPosition.z();

                //const GenStation& genStation = *theSdEvent.GetSimStationById(stationId);
                //jsonSerializer[sdId]["muonNumber"] = genStation.GetNumberOfMuons();
                //jsonSerializer[sdId]["muonSignal"] = genStation.GetMuonSignal();

                jsonSerializer[sdId]["riseTime"] = sdRecStationVector[j].GetRiseTime();
                jsonSerializer[sdId]["fallTime"] = sdRecStationVector[j].GetFallTime();
                jsonSerializer[sdId]["time50"] = sdRecStationVector[j].GetTime50();
                jsonSerializer[sdId]["time50RMS"] = sdRecStationVector[j].GetTime50RMS();
                jsonSerializer[sdId]["spDistance"] = sdRecStationVector[j].GetSPDistance();
                jsonSerializer[sdId]["spDistanceError"] = sdRecStationVector[j].GetSPDistanceError();
                jsonSerializer[sdId]["azimuthSP"] = sdRecStationVector[j].GetAzimuthSP();
                jsonSerializer[sdId]["spHeight"] = GetSPHeight(sdRecStationVector[j].GetSPDistance(), sdRecShower.GetZenith(), sdRecStationVector[j].GetAzimuthSP());
                jsonSerializer[sdId]["deltaTimeCore"] =  stationTime - coreTime;
                jsonSerializer[sdId]["deltaTimeSP"] = sdRecStationVector[j].GetPlaneTimeResidual();
                const int startSlot = sdRecStationVector[j].GetSignalStartSlot();
                jsonSerializer[sdId]["startSlot"] = startSlot;
                jsonSerializer[sdId]["endSlot"] = sdRecStationVector[j].GetSignalEndSlot();
                
                sbObs += totalSignal*pow(sdRecStationVector[j].GetSPDistance()/300, 4);

                int nPMT = 0;
                double averagePeakToCharge = 0;
                double totalSignalCalc = 0;
                std::vector<float> stationTrace;

                // get info on ssd as well!
                for (int pmt = 1; pmt <= 4; ++pmt) {
                    
                    const std::string pmtId = "pmt_" + std::to_string(pmt);
                    const Traces& pmtData = sdRecStationVector[j].GetPMTTraces(eTotalTrace, pmt);
                    const std::vector<float>& pmtTrace = pmtData.GetVEMComponent();
                    
                    if (pmtTrace.empty() || pmtData.GetCharge() <= 0)
                        continue;
                    
                    totalSignalCalc += pmtData.GetVEMSignal();
                    averagePeakToCharge += pmtData.GetPeak() / pmtData.GetCharge();
                    std::vector<float> pmtShortTrace(pmtTrace.begin() + startSlot, pmtTrace.begin() + startSlot + timeBins);
                    
                    if (stationTrace.empty())
                        stationTrace.resize(pmtShortTrace.size());

                    for (unsigned int i = 0, n = pmtShortTrace.size(); i < n; ++i)
                        stationTrace[i] += pmtShortTrace[i];
                    
                    jsonSerializer[sdId][pmtId]["trace"] = pmtShortTrace;
                    jsonSerializer[sdId][pmtId]["totalSignal"] = pmtData.GetVEMSignal();
                    jsonSerializer[sdId][pmtId]["charge"] = pmtData.GetCharge();
                    jsonSerializer[sdId][pmtId]["peak"] = pmtData.GetPeak();
                    
                    ++nPMT;
                }

                if (!nPMT || stationTrace.empty()) {
                    continue;
                } else {
                        if (timeBins != stationTrace.size())
                            std::cout << "stationTrace.size() is not the expected size: " << std::endl;

                        totalSignalCalc /= nPMT;
                        averagePeakToCharge /= nPMT;

                        for (int m = 0; m < timeBins; ++m)
                            vemTraceAv[m] = stationTrace[m] / nPMT;
                }

                jsonSerializer[sdId]["vemTraceAv"] = vemTraceAv;
                jsonSerializer[sdId]["totalSignalCalc"] = totalSignalCalc;
                jsonSerializer[sdId]["averagePeakToCharge"] = averagePeakToCharge;

                /*
                ########
                ## MD ##
                ########
                */

                MdRecCounter& mdRecCounter = *theMdEvent.GetCounterBySdPartnerId(stationId);
                MdSimCounter& mdSimCounter = *theMdEvent.GetSimCounterBySdPartnerId(stationId);
                //const unsigned int counterId = mdRecCounter.GetId();
                //const mdet::Counter& mdc = md.GetCounter(counterId);
                const int nModules = mdRecCounter.GetNumberOfModules();

                for (unsigned int mId = 1; mId <= nModules; ++mId) {
                    
                    const double Nmu = mdRecCounter.IsSaturated() == true ? mdRecCounter.GetNumberOfMuonsLowLimit() : mdRecCounter.GetNumberOfMuons();
                    jsonSerializer[sdId]["n_estimated_muons"] = Nmu;
                    const double effArea = mdRecCounter.GetActiveArea() / std::cos(sdRecShower.GetZenith());
                    jsonSerializer[sdId]["eff_area"] = effArea;
                    
                    //if (mId <=7){
                    //    M1Obs += ((Nmu/effArea)/ rhoMuRefVec[0])*(sdRecStationVector[j].GetSPDistance()/200);
                    //}
                    
                    MdRecModule& recModule = *mdRecCounter.GetModule(mId);

                    if (recModule.IsCandidate()) {
                        const int moduleId = recModule.GetId();
                        std::string mId = "md_" + std::to_string(moduleId);
                        
                        //const unsigned int moduleId = recModule.GetId();
                        //const mdet::Module& mdm = mdc.GetModule(moduleId);
                        jsonSerializer[sdId][mId]["sat_flag"] = recModule.IsSaturated();
                        jsonSerializer[sdId][mId]["SP_distance"] = recModule.GetSPDistance();
                        jsonSerializer[sdId][mId]["n_estimated_muons"] = recModule.IsSaturated() ? recModule.GetNumberOfMuonsLowLimit() : recModule.GetNumberOfEstimatedMuons();
                        jsonSerializer[sdId][mId]["n_estimated_muons_uncorrected"] = recModule.GetNumberOfEstimatedMuonsUncorrected();
                        jsonSerializer[sdId][mId]["active_area"] = recModule.GetActiveArea();
                        jsonSerializer[sdId][mId]["effective_area"] = recModule.GetActiveArea() * std::cos(sdRecShower.GetZenith());
                        jsonSerializer[sdId][mId]["pcc"] = (recModule.GetNumberOfEstimatedMuonsUncorrected()/recModule.GetNumberOfEstimatedMuons()) - 1.0;
                        
                        // Azimuth diff between module and shower axis
                        // Shower geometry
                        //const utl::CoordinateSystemPtr& coreLocalCS = LocalCoordinateSystem::Create(coreSite);
                        //const utl::Vector& rAxis = sRecShower.GetAxisCoreCS();
                        //const double phi = rAxis.GetPhi(coreLocalCS) / utl::rad;
                        // Orientation of module
                        //const double phi0 = mdm.GetPhi0() / utl::rad;
                        //jsonSerializer[sdId][mId]["sindphi"] = std::abs(std::sin(phi-phi0));

                        int nChannels = recModule.GetNumberOfChannels();

                        if (nChannels == 0)
                            continue;

                        double injNMu = 0.0;

                        for (int channelId = 1; channelId <= nChannels; ++channelId) {

                            const MdRecChannel& channel = *recModule.GetChannel(channelId);
                            const std::string chId = "ch_" + std::to_string(channelId);
                            const int scintillatorId = channel.GetScintillatorId();

                            if (channel.HasTrace()) {

                                //injected number of muons
                                const MdSimScintillator& simSc = *mdSimCounter.GetSimScintillatorById(moduleId, scintillatorId);
                                injNMu += simSc.GetNumberOfInjectedMuons();

                                // binary trace trace
                                std::vector<int> nonZeroIndices;
                                const std::vector<char> bTrace = channel.GetTrace();
                                const unsigned bTraceSize = bTrace.size();

                                    for (unsigned int i = 0; i < bTraceSize; ++i) {
                                        if (bTrace[i] != 0) {
                                            // If the value is different from zero, store the index
                                            nonZeroIndices.push_back(i);
                                        }
                                    }
                                jsonSerializer[sdId][mId][chId] = nonZeroIndices;

                            }
                        
                        jsonSerializer[sdId][mId]["injNMu"] = injNMu;

                        }
                    }
                }

                //std::cout << std::endl;  
            }
            
            // finaly save M1
            jsonSerializer["Sb"] = sbObs;
            //jsonSerializer["M1"] = std::log10(M1Obs);
            jsonSerializer["n_candidates"] = nCandidates;            
            
             // Open a file stream for writing           
            std::string filePath = join_paths(outputDir, (oFileName + ".json"));
            std::ofstream outputFile(filePath);

            // Check if the file stream is open
            if (outputFile.is_open()) {
                // Write the JSON data to the file
                outputFile << std::setw(4) << jsonSerializer.dump() << std::endl;

                // Close the file stream
                outputFile.close();
                std::cout << std::endl;
                std::cout << "JSON data saved to " << filePath << std::endl;

                index << oFileName << ',' << atmModel << ',' << showerId << ',' << useId << ','
                    << energyMC << ',' << zenithMC << ',' <<  showerSize << ',' <<  showerSizeError << ','
                    << trigger << ',' << isT5 << ',' << is6T5 << ','
                    << Xmax << ',' <<  hottestId << ',' << closestId << ',' << nCandidates  << ','
                    << bLDF << ',' << isSaturated << ',' << muonNumber << ',' << electromagneticEnergy << std::endl;

            } else {
                std::cout << std::endl;
                std::cerr << "Error opening file: " << filePath << std::endl;
            }

        }

        index.close();
    }
}
