#include <filesystem>
#include <algorithm>
#include <iostream>
#include <fstream>
#include <vector>
#include <cstdint>
#include <string>
#include <cmath>

const std::filesystem::path Data("/cr/tempdata01/filip/SSDCalib/RawNoCut");

int main(int argc, char **argv) {

    // rate based macro-variables
    const float desired_rate = 100;
    const float tolerance = 0.01;

    const int skip = std::atof(argv[1]);            // how many bins to skip after one trigger
    int threshold = 95;                             // first guess for SB trigger threshold
    float current_rate = 0;                         // ensure algorithm runs AT LEAST once
    int increment;
    float best_rate;
    int best_threshold;
    std::vector<int> past_guesses;
    float closest_difference = std::numeric_limits<float>::max();
    
    float nanoseconds = 0;
    int triggers = 0;

    // rate-converging algorithm    
    while (current_rate < (1-tolerance)*desired_rate || current_rate > (1+tolerance)*desired_rate)
    {
        nanoseconds = 0;
        triggers = 0;
        
        for (const auto& file : std::filesystem::directory_iterator(Data))
        {
            if (file.path().string().compare(50, 57, "_SSD.dat") != 0){continue;};
            // std::cout << "reading " << file.path().string().substr(39,57) << "...";

            // create file stream
            std::ifstream ifs(file.path().string());
            std::string line;

            // iterate over file line by line
            while (std::getline(ifs, line))
            {
                int baseline = stoi(line.substr(0, 3));

                for (int i = 4; i <= 8195; i+=4)
                {
                    try
                    {
                        int temp = stoi(line.substr(i, i + 4));

                        if (temp >= threshold + baseline)
                        {
                            triggers += 1;
                            i += 4*skip;
                        }

                    }
                    catch (const std::out_of_range& _){break;}
                }    

                nanoseconds += 2*8533.33;             // easier to hardcode this
            }

            // std::cout << "done!" << std::endl;
        }

        current_rate = (float)triggers / nanoseconds * 1e9;
        const auto difference = abs(current_rate - desired_rate);

        if (difference < closest_difference)
        {
            closest_difference = difference;
            best_threshold = threshold;
            best_rate = current_rate;
        }

        // std::cout << "Examined " << nanoseconds * 1e-9 << " s : rate is " << current_rate << " Hz (" << triggers << " triggers) : threshold was " << threshold << std::endl;

        const int sign = ((current_rate - desired_rate) > 0) - ((current_rate - desired_rate) < 0);
        past_guesses.push_back(threshold);

        if (abs(current_rate - desired_rate) > 20){increment = 20;}
        else {increment = 1;}

        switch(sign)
        {
            case -1:
                threshold -= increment;
                break;
            case +1:
                threshold += increment;
                break;         
        }

        if (std::find(past_guesses.begin(), past_guesses.end(), threshold) != std::end(past_guesses)){break;}
    }

    // std::cout << "Best threshold for desired rate of " << desired_rate << " Hz is: " << best_threshold << " (" << best_rate << " Hz)" << std::endl;

    std::string save_path = "/cr/tempdata01/filip/SSDCalib/OnlineCalib/skip_bias_raw.dat";
    std::ofstream saveFile(save_path, std::ios_base::app);

    saveFile << skip << " " << best_threshold << " " << triggers << " " << nanoseconds * 1e-9 << std::endl;

    return 0;
}