#include <filesystem>
#include <algorithm>
#include <iostream>
#include <fstream>
#include <vector>
#include <cstdint>
#include <string>
#include <future>

static std::mutex trigger_guard;
std::vector<std::future<void>> results;

std::vector<float> operator+(const std::vector<int>& a, const std::vector<float>& b)
{
    // assert(a.size() == b.size());

    std::vector<float> result;
    result.reserve(a.size());

    std::transform(a.begin(), a.end(), b.begin(), 
                   std::back_inserter(result), std::plus<float>());
    return result;
}

std::vector<float> operator+=(const std::vector<float>& a, const std::vector<float>& b)
{
    // assert(a.size() == b.size());

    std::vector<float> result;
    result.reserve(a.size());

    std::transform(a.begin(), a.end(), b.begin(), 
                   std::back_inserter(result), std::plus<float>());
    return result;
}

std::vector<float> operator*(const std::vector<float>& a, const double factor)
{
    std::vector<float> result;
    result.reserve(a.size());

    for (const auto& x : a){result.push_back(factor * x);}

    return result;
}

std::vector<float> operator/(const std::vector<float>& a, const double factor)
{
    std::vector<float> result;

    for (const auto& x : a){result.push_back(x / factor);}

    return result;
}

std::vector<float> operator-(const std::vector<float>& a, const float subtractor)
{
    std::vector<float> result;
    result.reserve(a.size());

    for (const auto& x : a){result.push_back(x - subtractor);}    

    return result;
}

/*
std::vector<std::string> get_trace(std::ifstream& path)
{
    // std::vector<std::vector<int>> result;
    std::vector<std::string> lines(3);

    std::getline(path, lines[0]);
    std::getline(path, lines[1]);
    std::getline(path, lines[2]);

    for (int iPMT = 0; iPMT < 3; iPMT++)
    {
        std::vector<int> trace;
        int baseline = stoi(lines[iPMT].substr(0,3));

        for (int i = 4; i <= 8195; i+= 4)
        {
            try
            {
                trace.push_back(stoi(lines[iPMT].substr(i, i+4)) - baseline);
            }
            catch(const std::out_of_range& _){break;}  
        }

        result.push_back(trace);
    }

    return lines;
}
*/

std::vector<float> qualifies_as_t1(const std::vector<std::string>& t, std::vector<float>& thresholds)
{
    std::vector<float> result = {0., 0., 0.};
    const auto t1 = thresholds * 1.75;
    const auto t2 = thresholds * 2.5;

    const int b1 = stoi(t[0].substr(0, 3));
    const int b2 = stoi(t[1].substr(0, 3));
    const int b3 = stoi(t[2].substr(0, 3));

    for (int n = 4; n < 8195; n++)
    {
        const int p1 = stoi(t[0].substr(n, n+4)) - b1;
        const int p2 = stoi(t[1].substr(n, n+4)) - b2;
        const int p3 = stoi(t[2].substr(n, n+4)) - b3;

        if (not result[0] && p1 > t2[0])
        {
            if (p2 > t1[1] )
            {
                if (p3 > t1[2]){result[0] = 1;}
            }
        }

        if (not result[1] && p2 > t2[1])
        {
            if (p1 > t1[0])
            {
                if (p3 > t1[2]){result[1] = 1;}
            }
        }

        if (not result[2] && p3 > t2[3])
        {
            if (p1 > t1[0])
            {
                if (p2 > t1[2]){result[2] = 1;}
            }
        }

        if (result[1] && result[2] && result[3]){break;}
    }
    
    return result;
}

void calculate_triggers_in_file(std::ifstream& ifs, std::vector<float>& triggers, std::vector<float>& thresholds)
{
    std::vector<std::string> trace(3);
    std::vector<float> triggers_this_file(3,0.0);

    while (std::getline(ifs, trace[0]))
    {
        std::getline(ifs, trace[1]);
        std::getline(ifs, trace[2]);
        triggers_this_file += qualifies_as_t1(trace, thresholds);
    }

    std::lock_guard<std::mutex> lock(trigger_guard);
    triggers += triggers_this_file;
}

int main(int argc, char **argv) {

    const std::string station(argv[1]);
    const std::filesystem::path Data("/cr/tempdata01/filip/iRODS/UubRandoms/converted/" + station + "/");    

    std::vector<float> thresholds = {150, 150, 150};
    std::vector<float> increments(3, 1);
    std::vector<float> triggers;
    float t_cal = 0.01;
    float nanoseconds;

    // variables as set in the Online/Offline calibration paper
    // https://www.sciencedirect.com/science/article/pii/S0168900206013593
    // Online algorithm described on page 842 with calibration triggers

    // rate-converging algorithm    
    while (t_cal < 60.0)
    {
        triggers = {0., 0., 0.};
        nanoseconds = 0;

        for (const auto& file : std::filesystem::directory_iterator(Data))
        {
            // create file stream
            if (file.path().string().find("_WCD.dat") == std::string::npos){continue;}
            std::ifstream ifs(file.path().string());
            // std::cout << "reading " << file.path().string() << "...";

            if (nanoseconds * 1e-9 >= t_cal){break;}
            nanoseconds += 8.33 * 2048 * 5000;

            results.push_back(std::async(std::launch::async, calculate_triggers_in_file, std::ref(ifs), std::ref(triggers), std::ref(thresholds)));
        }

        for (auto& thread : results){
            thread.wait();
            std::cout << "waiting" << std::endl;}

        std::vector<float> rates = triggers / (nanoseconds * 1e-9);

        // diagnostics during rate-converging algorithm
        std::cout << "after " << nanoseconds * 1e-9 << " seconds found [";
        for (const auto& x : triggers){std::cout << x << ", ";}
        std::cout << "] triggers using thresholds (";
        for (const auto& x : thresholds){std::cout << x << ", ";}
        std::cout << ") => rates = {";
        for (const auto& x : rates){std::cout << x << " Hz, ";}
        std::cout << "}" << std::endl;

        std::vector<float> differences = rates - (float)70.0;
        bool estimate_is_bad = false;
        for (int i = 0; i < 3; i++)
        {
            if (-20 > differences[i])
            {
                estimate_is_bad = true;
                increments[i] = 1;
                thresholds[i] -= 5;
            }
            else if (-2 > differences[i])
            {
                thresholds[i] -= increments[i];
            }
            else if (differences[i] > 20)
            {
                estimate_is_bad = true;
                increments[i] = 1;
                thresholds[i] += 5;
            }
            else if (differences[i] > 2)
            {
                thresholds[i] += increments[i];
            }
            else
            {
                if (increments[i] > 0.1){increments[i] -= 0.1;}
            }
        }

        if (estimate_is_bad){t_cal = 0.1;}
        // else{t_cal += 5.0;}
    }

    //std::cout << "Best threshold for desired rate of " << desired_rate << " Hz is: " << best_threshold << " (" << best_rate << " Hz)" << std::endl;

    //std::string save_path = "/cr/tempdata01/filip/SSDCalib/OnlineCalib/OnlineThresholds/" + station + ".dat";
    //std::ofstream saveFile(save_path, std::ios_base::app);

    //saveFile << desired_rate << " " << best_rate << " " << best_threshold << " " << triggers << " " << nanoseconds * 1e-9 << std::endl;

    return 0;
}
