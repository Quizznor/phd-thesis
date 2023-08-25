#include <filesystem>
#include <iostream>
#include <vector>
#include <cstdint>
#include <string>

// header files for data structures, utilities, etc.
#include "Traces.h"
#include "Randoms.h"
#include "Trigger.h"

const std::filesystem::path RadioCutData("/cr/tempdata01/filip/SSDCalib/RadioCut");

int main(int argc, char **argv) {

    float desired_rate = std::atof(argv[1]);
    float current_rate = 0;
    float tolerance = 0.05;

    for (const auto& file : std::filesystem::directory_iterator(RadioCutData))
    {
        if (file.path().string().compare(50, 57, "_SSD.dat") != 0){continue;} ;

        

        std::cout << file << " " << typeid(file).name() << " " << std::endl;

        // read data in randoms
        // calculate no of SB triggers
        // relate to rate, error on MIP
    }

    // rate-converging algorithm    
    // while (current_rate < (1-tolerance)*desired_rate || current_rate > (1+tolerance)*desired_rate)
    // {
    //     float microseconds = 0;

    //     

    //     current_rate += 1.1;
    //     std::cout << current_rate << std::endl;
    // }


    return 0;
}