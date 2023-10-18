#include <filesystem>
#include <algorithm>
#include <iostream>
#include <fstream>
#include <vector>
#include <cstdint>
#include <string>
#include <future>
#include <chrono>

static std::mutex trigger_guard;

std::vector<float> operator+(const std::vector<float>& a, const std::vector<float>& b)
{
    // assert(a.size() == b.size());

    std::vector<float> result;
    result.reserve(a.size());

    std::transform(a.begin(), a.end(), b.begin(), 
                   std::back_inserter(result), std::plus<float>());
    return result;
}

std::vector<float> operator+=(std::vector<float>& a, const std::vector<float>& b)
{
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

std::vector<float> qualifies_as_t1(std::vector<std::string>& t, std::vector<float>& thresholds)
{
    std::vector<float> result = {0., 0., 0.};
    const auto t1 = thresholds * 1.75;
    const auto t2 = thresholds * 2.5;

    // allocate memory pointers, find baseline
    int temp1, temp2, temp3;
    temp1 = t[0].find(' ');
    temp2 = t[1].find(' ');
    temp3 = t[2].find(' ');

    const int b1 = stoi(t[0].substr(0, temp1));
    const int b2 = stoi(t[1].substr(0, temp2));
    const int b3 = stoi(t[2].substr(0, temp3));

    t[0] = t[0].substr(temp1 + 1, std::string::npos);
    t[1] = t[1].substr(temp2 + 1, std::string::npos);
    t[2] = t[2].substr(temp3 + 1, std::string::npos);

    // skip baseline, receive 2048/682 values afterwards
    // distinction is made on string length, not ideal...
    int n_bins = t[0].size() > 5000 ? 2049 : 683;
    for (int n = 0; n < n_bins; n++)
    {
        const auto temp1 = t[0].find(' ');
        const auto temp2 = t[1].find(' ');
        const auto temp3 = t[2].find(' ');

        const int p1 = stoi(t[0].substr(0, temp1)) - b1;
        const int p2 = stoi(t[1].substr(0, temp2)) - b2;
        const int p3 = stoi(t[2].substr(0, temp3)) - b3;

        if (not result[0] && (p1 > t2[0]) && (p2 > t1[1]) && (p3 > t1[2])){result[0] = 1;}
        if (not result[1] && (p2 > t2[1]) && (p1 > t1[0]) && (p3 > t1[2])){result[1] = 1;}
        if (not result[2] && (p3 > t2[2]) && (p1 > t1[0]) && (p2 > t1[1])){result[2] = 1;}

        t[0] = t[0].substr(temp1 +1, std::string::npos);
        t[1] = t[1].substr(temp2 +1, std::string::npos);
        t[2] = t[2].substr(temp3 +1, std::string::npos);

        if (result[1] && result[2] && result[3]){break;}
    }
    
    return result;
}

void calculate_triggers_in_file(std::string* path, std::vector<float>& thresholds, std::vector<float>& triggers)
{
    std::ifstream ifs(*path);
    std::vector<std::string> trace(3);
    std::vector<float> triggers_this_file(3,0.0);

    while (std::getline(ifs, trace[0]))
    {
        std::getline(ifs, trace[1]);
        std::getline(ifs, trace[2]);
        // std::cout << trace[0].size() << ' ' << trace[1].size() << ' ' << trace[2].size() << std::endl;
        triggers_this_file = triggers_this_file + qualifies_as_t1(trace, thresholds);
    }

    std::lock_guard<std::mutex> lock(trigger_guard);
    triggers = triggers + triggers_this_file;
    std::cout << "thread finished: (" << triggers_this_file[0] << ", " << triggers_this_file[1] << ", " << triggers_this_file[2] << ") triggers found in " << *path << std::endl;
    delete path;
}

int main(int argc, char **argv) {

    const std::string station(argv[1]);
    // const std::string station("LeQuiDon");
    const std::filesystem::path Data("/cr/tempdata01/filip/iRODS/UubRandoms/converted/" + station + "/");
    std::vector<std::string> active_files;
    std::vector<float> thresholds;

    // use online means for faster convergense
    if (station == "NuriaJr"){thresholds = {160.5, 173.0, 159.8};}
    else if (station == "Peru"){thresholds = {153.7, 121.2, 152.4};}
    else if (station == "Granada"){thresholds = {152.9, 145.8, 157.9};}
    else if (station == "Jaco"){thresholds = {181.9, 154.7, 144.3};}
    else if (station == "LeQuiDon"){thresholds = {89.6, 88.4, 166.6};}
    else if (station == "Svenja"){thresholds = {141.3, 147.0, 146.5};}
    else if (station == "SvenjaLate"){thresholds = {141.3, 147.0, 146.5};}
    else if (station == "SvenjaDownsampled"){thresholds = {141.3, 147.0, 146.5};}
    else {thresholds = {150, 150, 150};}

    std::vector<float> increments(3, 1);
    std::vector<float> triggers;
    float nanoseconds;

    // variables as set in the Online/Offline calibration paper
    // https://www.sciencedirect.com/science/article/pii/S0168900206013593q
    // Online algorithm described on page 842 with calibration triggers
    float t_cal = 5.0;                  // starting calibration window
    float t_inc = 5.0;                  // per-step increase of window
    float t_max = 60.;                  // maximum duration of calibration
    float t_res = 10.;                  // reset to this value if estimate is shit


    // rate-converging algorithm    
    while (t_cal < t_max)
    {
        std::vector<std::future<void>> results;
        triggers = {0., 0., 0.};
        nanoseconds = 0;

        for (const auto& file : std::filesystem::directory_iterator(Data))
        {
            // create file stream
            std::string *path = new std::string(file.path().string());
            if (path->find("_WCD.dat") == std::string::npos){continue;}

            if (nanoseconds * 1e-9 >= t_cal){break;}
            nanoseconds += 8.33 * 2048 * 5000;

            /* Concurrent version */
            // we should limit the number of threads here
            // otherwise people will start complaining
            const int max_threads = 18;
            while (results.size() > max_threads)
            {
                for (int i = 0; i < results.size(); i++)
                {
                    const auto response = results[i].wait_for(std::chrono::seconds(0));
                    if (response == std::future_status::ready){results.erase(results.begin() + i);}
                }
            }

            results.push_back(std::async(std::launch::async, calculate_triggers_in_file, path, std::ref(thresholds), std::ref(triggers)));

            // // /* Non-concurrent version */
            // calculate_triggers_in_file(path, thresholds, triggers);
        }

        for (auto& thread : results){thread.wait();}

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
        bool estimate_is_shit = false;
        bool estimate_is_bad = false;
        for (int i = 0; i < 3; i++)
        {
            if (-20 > differences[i])
            {
                estimate_is_shit = true;
                increments[i] = 1;
                thresholds[i] -= 5;
            }
            else if (-2 > differences[i])
            {
                thresholds[i] -= increments[i];
                increments[i] = 1;
                estimate_is_bad = true;
            }
            else if (differences[i] > 20)
            {
                estimate_is_shit = true;
                increments[i] = 1;
                thresholds[i] += 5;
            }
            else if (differences[i] > 2)
            {
                thresholds[i] += increments[i];
                increments[i] = 1;
                estimate_is_bad = true;
            }
            else
            {
                if (increments[i] > 0.1){increments[i] -= 0.1;}
            }
        }

        if (estimate_is_shit){t_cal = t_res;}
        else if (not estimate_is_bad){t_cal += t_inc;}
    }

    return 0;
}
