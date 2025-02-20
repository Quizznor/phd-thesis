// std library
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <memory>
#include <iterator>
#include <chrono>
#include <string>

// 3rd party libraries
#include "json.hpp"

// offline includes
#include "RecEvent.h"
#include "DetectorGeometry.h"
#include "RecEventFile.h"


using json = nlohmann::json;

/*
#############
## HELPERS ##
#############
*/

// Function to join paths (simple implementation)
std::string join_paths(const std::string& dir, const std::string& file) {
    if (dir.back() == '/' || dir.back() == '\\') {
        return dir + file;
    } else {
        return dir + "/" + file;
    }
}

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


std::vector<double> GetRhoMuRef(double x, double dx, double E, double dE) {
         //The argument of rho0 is x = cos2(theta) - cos2(30deg)
         TF1* fRho0 = new TF1("fRho0","[0]+[1]*x+[2]*pow(x,2)",-0.6,0.25);
         fRho0->SetParameters(-1.07078e-01,2.39602e-01,-7.29162e-01);
         fRho0->SetParError(0,1.98110e-03);
         fRho0->SetParError(1,1.00872e-02);
         fRho0->SetParError(2,1.00872e-02);
    
         double rho0 = fRho0->Eval(x);
         double varRho0 = pow(fRho0->GetParError(0),2) + pow(x*fRho0->GetParError(1),2) + pow(x*x*fRho0->GetParError(2),2) + pow(fRho0->GetParameter(1)+2*x*fRho0->GetParameter(2),     2)*pow(dx,2);
         double dRho0 = sqrt(varRho0);
         delete fRho0;

         double c = 8.67599e-01;
         double dc = 5.60162e-03;

        //E in units of 100 PeV
         double LogRho = rho0 + c * log10(E);
         double varLogRho = pow(dRho0,2) + pow(log10(E)*dc,2) + pow(c*dE/(E*log(10)),2);
         double dLogRho = sqrt(varLogRho);

         double Rho = pow(10,LogRho);
         double dRho = log(10)*Rho*dLogRho;

         std::vector<double> v = {Rho,dRho};

         return v;
}

    std::vector<double> GetFattPh (double sec, double dsec)
    {
        TF1* func = new TF1("func","[0] * pow(1+(x-[2])/[1],[1]/[3]) * exp(-(x-[2])/[3])",1,1.7);

        func->SetParameter(0,3.88940e+01);
        func->SetParameter(1,6.23117e-01);
        func->SetParameter(2,1.00999e+00);
        func->SetParameter(3,9.04938e-02);

        func->SetParError(0,5.62431e-02);
        func->SetParError(1,5.26757e-02);
        func->SetParError(2,2.32738e-03);
        func->SetParError(3,4.79750e-03);

        double f = func->Eval(sec);

        double f0 = func->GetParameter(0);
        double f1 = func->GetParameter(1);
        double f2 = func->GetParameter(2);
        double f3 = func->GetParameter(3);

        double df0 = func->GetParError(0);
        double df1 = func->GetParError(1);
        double df2 = func->GetParError(2);
        double df3 = func->GetParError(3);

        delete func;

        double df = 0;

        double factor = 1+(sec-f2)/f1;
        double var0 = pow(f*df0/f0,2);
        double var1 = pow(f*(log(factor)/f3 - (sec-f2)/(f1*f3*factor))*df1,2);
        double var2 = pow(f*(f2-sec)*df2/(f3*(f2-sec-f1)),2);
        double var3 = pow(f*(f1*log(factor)-sec+f2)*df3/pow(f3,2),2);
        double var4 = pow(-f*(sec-f2)*dsec/(f3*(sec-f2+f1)),2);
        df = sqrt(var0+var1+var2+var3+var4);

        std::vector<double> v = {f,df};
        return v;
    }

    std::vector<double> GetNormFattPh (double sec, double dsec)
    {
        std::vector<double> vFattPh = GetFattPh(sec,dsec);
        double secref = pow(cos(30*TMath::Pi()/180),-1);
        double norm = GetFattPh(secref,0).at(0);
        std::vector<double> v = {vFattPh.at(0)/norm,vFattPh.at(1)/norm};
        return v;
    }

    std::vector<double> GetFattPr (double x, double dx)
    {
        //x = cos2 - cos2ref
        TF1* func = new TF1("func","1 + [0]*x + [1]*pow(x,2) + [2]*pow(x,3)",0,1);

        func->SetParameters(1.88357e+00,-1.74331e+00,-3.45984e+00);

        double f = func->Eval(x);

        func->SetParError(0,2.81332e-03);
        func->SetParError(1,1.45271e-02);
        func->SetParError(2,5.38753e-02);

        double f1 = func->GetParameter(1);
        double f2 = func->GetParameter(2);
        double f3 = func->GetParameter(3);

        double df1 = func->GetParError(1);
        double df2 = func->GetParError(2);
        double df3 = func->GetParError(3);

        delete func;

        double df = 0;

        double var1 = pow(x*df1,2);
        double var2 = pow(pow(x,2)*df2,2);
        double var3 = pow(pow(x,3)*df3,2);
        double varx = pow((f1+2*f2*x+3*f3*pow(x,2))*dx,2);
        df = sqrt(var1+var2+var3+varx);

        std::vector<double> v = {f,df};
        return v;

    }



int main(int argc, char** argv)
{   
    {   

       Timer timer;
	
	    /* Start of the analysis */
	
	    // vector to store paths for ADST files
	    std::vector<std::string> dataFilePaths;
	    // event counter
	    unsigned long nEvents(0);
        // index and outputDir paths
        std::string indexPath = argv[1]; // e.g. /sps/pauger/users/erodriguez/PhotonDiscrimination/index.csv
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

            // ids
            int eventId2 = theSdEvent.GetEventId();
            int atmModel = static_cast<int>(eventId2/pow(10,8));
            int showerId = static_cast<int>((eventId2 - atmModel * pow(10,8))/pow(10,4));
            int useId = static_cast<int>(eventId2 - atmModel * pow(10,8) -showerId * pow(10,4) - 100);
            std::string eventId = theRecEvent->GetEventId();

            const GenShower& sdGenShower = theRecEvent->GetGenShower();
            std::string primaryName = sdGenShower.GetPrimaryName();
            primaryName.erase(std::remove(primaryName.begin(), primaryName.end(), ' '));

            //std::cout << "Processing event " << eventId <<" " << eventId2<< std::endl;

            // Specify the file path using the provided output directory
            std::string eBin = categorizeEnergy(log10(sdGenShower.GetEnergy()));
            std::string oFileName = primaryName + "_" + eBin +  "_"; // energy bin
            oFileName += formatNumber(atmModel, 2) + formatNumber(showerId, 4); // shower id
            oFileName += "_" + formatNumber(useId, 2); // use id

            std::string filePath = join_paths(outputDir, (oFileName + ".json"));

            // reading the original JSON
            std::ifstream inputFile(filePath);
            
            if (!inputFile.is_open()) {
                std::cerr << "Error opening the file." << std::endl;
                return 1;
            }

            json data;
            inputFile >> data;
            inputFile.close();

            // SD Event
            const SdRecShower& sdRecShower = theSdEvent.GetSdRecShower();

            // Add a new key-value pair
            double photonEnergy = sdRecShower.GetEnergy();
            data["photon_energy"] = photonEnergy;
            const double s250 = sdRecShower.GetShowerSize();
           
            
            double M1Obs = 0;
            //rhoMuRef
            const double x = pow(std::cos(sdRecShower.GetZenith()), 2) - pow(std::cos(0.523599),2);
            const double dx = std::sqrt(pow(-2+std::sin(sdRecShower.GetZenith())*std::cos(sdRecShower.GetZenith())*sdRecShower.GetZenithError(), 2));
            const double photonNorm = GetFattPh(pow(cos(30*TMath::Pi()/180),-1),0).at(0);
            const double alpha = 1.15 * (1 - 0.192 * pow(pow(std::cos(sdRecShower.GetZenith()), 2), 2.96));
            const double s30 = s250 / (GetFattPr(x, dx)[0] * photonNorm);
            const double equivalentEnergy = pow(10,17) * pow(s30, (1/alpha));
            data["equivalent_energy"] = equivalentEnergy;
            std::vector<double> rhoMuRefVec = GetRhoMuRef(x, dx, equivalentEnergy/pow(10,17), sdRecShower.GetEnergyError()/pow(10,17));
            data["rhoMuRef"] = rhoMuRefVec[0];
            data["rhoMuRef_error"] = rhoMuRefVec[1];
            
            /*
            ###################
            ## Station Level ##
            ###################
            */
           
            const std::vector<SdRecStation>& sdRecStationVector = theSdEvent.GetStationVector();
            
            // loop trough stations : get all necessary infos
            for (unsigned int j = 0; j < sdRecStationVector.size(); ++j ) {
                const int stationId = sdRecStationVector[j].GetId();

                /*
                ########
                ## SD ##
                ########
                */

                // skip all the imaginary stations from Dense Rings
                if (9000 < stationId)
                    continue;

                // WE ARE ONLY KEEPING CANDIDATE STATIONS. REJECTED AND SILENT STATIONS WILL NOT BE INCLUDED
                if (!sdRecStationVector[j].IsCandidate()) {
                //    std::cout << ": station is not a candidate." << std::endl;
                    continue;
                }

                /*
                ########
                ## MD ##
                ########
                */

                MdRecCounter& mdRecCounter = *theMdEvent.GetCounterBySdPartnerId(stationId);
                const int nModules = mdRecCounter.GetNumberOfModules();

                for (unsigned int mId = 1; mId <= nModules; ++mId) {                   
                    double Nmu = mdRecCounter.IsSaturated() == true ? mdRecCounter.GetNumberOfMuonsLowLimit() : mdRecCounter.GetNumberOfMuons();
                    double effArea = mdRecCounter.GetActiveArea() / std::cos(sdRecShower.GetZenith());
                    M1Obs += ((Nmu/effArea)/ rhoMuRefVec[0])*(sdRecStationVector[j].GetSPDistance()/200);
                }
                
            }
            
            data["M1"] = std::log10(M1Obs);

            // Save the modified JSON back to the same file
            std::ofstream outputFile(filePath);

            // Check if the file stream is open
            if (outputFile.is_open()) {
                // Write the JSON data to the file
                outputFile << std::setw(4) << data.dump() << std::endl;

                outputFile.close();
                std::cout << std::endl;
                std::cout << "JSON data saved to " << filePath << std::endl;
                
                index << oFileName << ',' << atmModel << ',' << showerId << ',' << useId << ','
                    << photonEnergy << ',' << s250 << ',' << equivalentEnergy << ',' << std::log10(M1Obs) << std::endl;
                
                
            } else {
                std::cerr << "Error opening file: " << filePath << std::endl;
            }
        }
        
        index.close();
    }
}