#include <filesystem>
#include <algorithm>
#include <iostream>
#include <fstream>
#include <vector>
#include <cstdint>
#include <string>
#include <cmath>

int main(int argc, char **argv) {

    std::vector<int> rates(350, 0);

    const std::string station(argv[1]);
    const std::string file(argv[2]);
    const std::string analyze_T1s(argv[3]);

    const auto random_file = std::string(4 - std::min((size_t)4, file.length()), '0') + file;
    const auto full_path = "/cr/tempdata01/filip/iRODS/UubRandoms/converted/"+station+"/randoms"+random_file+"_SSD.dat";
    const auto t1_path = "/cr/tempdata01/filip/SSDCalib/WCDT1Calib/"+station+"/randoms"+random_file+"_WCD.dat";

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

        const int baseline = stoi(line.substr(0, 3));
        int max_bin = std::numeric_limits<int>::min();

        for (int i=4; i<=8195; i+=4)
        {
            const auto this_bin = stoi(line.substr(i, i+4)) - baseline;
            if (this_bin > max_bin){max_bin = this_bin;}
        }

        if (is_t1 == "1")
        {
            for (int j=max_bin-1; j>=0; j--){rates[j] += 1;}
        }

    }
 
    const std::string specifier = analyze_T1s == "1" ? "T1" : "";
    std::cout << "writing to /cr/tempdata01/filip/SSDCalib/UubRates"+specifier+"/"+station+"/randoms"+random_file+".dat" << std::endl;
    std::ofstream saveFile("/cr/tempdata01/filip/SSDCalib/UubRates"+specifier+"/"+station+"/randoms"+random_file+".dat", std::ios_base::app);
    for (const auto& bin : rates){saveFile << bin << "\n";}
    
    return 0;
}