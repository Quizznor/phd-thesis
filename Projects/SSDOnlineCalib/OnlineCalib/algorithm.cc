#include <filesystem>
#include <iostream>
#include <fstream>
#include <vector>
#include <cstdint>
#include <string>
#include <cmath>

const std::filesystem::path RadioCutData("/cr/tempdata01/filip/SSDCalib/RadioCut");

int main(int argc, char **argv) {

    // rate based macro-variables
    const float desired_rate = std::atof(argv[1]);
    const float tolerance = 0.05;

    const int skip = 20;                            // how many bins to skip after one trigger
    int threshold = 50;                             // first guess for SB trigger threshold
    float current_rate = 0;                         // ensure algorithm runs AT LEAST once

    // rate-converging algorithm    
    while (current_rate < (1-tolerance)*desired_rate || current_rate > (1+tolerance)*desired_rate)
    {
        float nanoseconds = 0;
        int triggers = 0;
        
        for (const auto& file : std::filesystem::directory_iterator(RadioCutData))
        {
            if (file.path().string().compare(50, 57, "_SSD.dat") != 0){continue;} ;
            // std::cout << "reading " << file.path().string().substr(39,57) << "...";

            // create file stream
            std::ifstream ifs(file.path().string());
            std::string line;

            // iterate over file line by line
            while (std::getline(ifs, line))
            {
                int baseline = stoi(line.substr(0, 3));

                for (int i = 4; i <= 4097; i+=4)
                {
                    int temp = stoi(line.substr(i, i + 4));

                    if (temp >= threshold + baseline)
                    {
                        triggers += 1;
                        i += 4*skip;

                    }
                }    

                nanoseconds += 8533.33;             // hardcoded to increment by 1024 bins (half of the trace)
            }
            
            // std::cout << "done!" << std::endl;
        }

        current_rate = (float)triggers / nanoseconds * 1e9;

        const int sign = ((current_rate - desired_rate) > 0) - ((current_rate - desired_rate) < 0);
        // const int magnitude = abs(current_rate - desired_rate);

        switch(sign)
        {
            case -1:
                threshold -= 1;
                break;
            case +1:
                threshold += 1;
                break;         
        }

        std::cout << "Examined " << nanoseconds * 1e-9 << " s : rate is " << current_rate << " Hz (" << triggers << " triggers) : threshold is now " << threshold << std::endl;
    }
    return 0;
}