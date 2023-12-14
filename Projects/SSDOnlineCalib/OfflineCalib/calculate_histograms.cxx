#include <filesystem>
#include <algorithm>
#include <iostream>
#include <fstream>
#include <vector>
#include <cstdint>
#include <string>
#include <cmath>

int main(int argc, char **argv) {

    std::vector<int> rates(1000, 0);

    const std::string station(argv[1]);
    const std::string date(argv[2]);
    const std::string file(argv[3]);
    const std::string analyze_T1s(argv[4]);

    // for debugging:
    // const std::string station("NuriaJrFilteredDownsampled");
    // const std::string date("Nov2022");
    // const std::string file("1");
    // const std::string analyze_T1s("0");


    const auto random_file = std::string(4 - std::min((size_t)4, file.length()), '0') + file;
    const auto full_path = "/cr/tempdata01/filip/UubRandoms/"+date+"/converted/"+station+"/randoms"+random_file+"_SSD.dat";
    const auto t1_path = "/cr/tempdata01/filip/SSDCalib/WCDT1Calib/"+date+"/"+station+"/randoms"+random_file+"_WCD.dat";

    std::cout << "reading from " << full_path << std::endl;
    std::cout << "reading from " << t1_path << std::endl;

    std::ifstream ifs(full_path);
    std::string line;
    std::ifstream t1(t1_path);
    std::string is_t1;

    while (std::getline(ifs, line))
    {

        if (analyze_T1s == "1"){std::getline(t1, is_t1);}
        else {is_t1 = "1";}

        int max_bin = std::numeric_limits<int>::min();

        if (is_t1 != "1"){continue;}

        // get the baseline for SSD
        const auto first_break = line.find(' ');
        const int ssd_baseline = stoi(line.substr(0, first_break));
        line = line.substr(first_break + 1, std::string::npos);

        for (int n = 0; n < 2049; n++)                          // hardcoded to receive 2048 bins
        {
            const auto next_break = line.find(' ');
            const int this_bin = stoi(line.substr(0, next_break));
            line = line.substr(next_break + 1, std::string::npos);

            if (this_bin - ssd_baseline > max_bin){max_bin = this_bin - ssd_baseline;}
        }

        for (int j=max_bin-1; j>=0; j--){rates[j] += 1;}
    }
 
    const std::string specifier = analyze_T1s == "1" ? "T1" : "";
    std::cout << "writing to /cr/tempdata01/filip/SSDCalib/SsdRates"+specifier+"/"+station+"/randoms"+random_file+".dat" << std::endl;
    std::ofstream saveFile("/cr/tempdata01/filip/SSDCalib/SsdRates"+specifier+"/"+station+"/randoms"+random_file+".dat", std::ios_base::app);
    for (const auto& bin : rates){saveFile << bin << "\n";}
    
    return 0;
}