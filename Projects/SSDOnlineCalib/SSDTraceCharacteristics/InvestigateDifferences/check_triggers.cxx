#include <filesystem>
#include <algorithm>
#include <iostream>
#include <fstream>
#include <vector>
#include <cstdint>
#include <string>

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

std::vector<float> is_calibration_trigger(std::vector<std::string>& t, std::vector<float>& thresholds)
{
    std::vector<float> result = {0., 0., 0.};
    const auto t1 = thresholds * 1.75;
    const auto t2 = thresholds * 2.5;

    int p1, p2, p3;

    // UB/UUB distinction is made on string length, not ideal...
    int n_bins = t[0].size() > 4000 ? 2049 : 683;
    for (int n = 0; n < n_bins; n = n+2)
    {
        const auto temp1 = t[0].find(' ');
        const auto temp2 = t[1].find(' ');
        const auto temp3 = t[2].find(' ');

        const int p1 = stoi(t[0].substr(0, temp1));
        const int p2 = stoi(t[1].substr(0, temp2));
        const int p3 = stoi(t[2].substr(0, temp3));
        t[0] = t[0].substr(temp1 +1, std::string::npos);
        t[1] = t[1].substr(temp2 +1, std::string::npos);
        t[2] = t[2].substr(temp3 +1, std::string::npos);

        if (not result[0] && (p1 > t2[0]) && (p2 > t1[1]) && (p3 > t1[2])){result[0] = 1;}
        if (not result[1] && (p2 > t2[1]) && (p1 > t1[0]) && (p3 > t1[2])){result[1] = 1;}
        if (not result[2] && (p3 > t2[2]) && (p1 > t1[0]) && (p2 > t1[1])){result[2] = 1;}

        if (result[1] && result[2] && result[3]){break;}
    }
    
    return result;
}

int is_t1_trigger(std::vector<std::string>& t, std::vector<float>& thresholds)
{
    const auto t1 = thresholds * 1.75;

    int p1, p2, p3;

    // UB/UUB distinction is made on string length, not ideal...
    int n_bins = t[0].size() > 4000 ? 2049 : 683;
    for (int n = 0; n < n_bins; n++)
    {
        const auto temp1 = t[0].find(' ');
        const auto temp2 = t[1].find(' ');
        const auto temp3 = t[2].find(' ');

        const int p1 = stoi(t[0].substr(0, temp1));
        const int p2 = stoi(t[1].substr(0, temp2));
        const int p3 = stoi(t[2].substr(0, temp3));
        t[0] = t[0].substr(temp1 +1, std::string::npos);
        t[1] = t[1].substr(temp2 +1, std::string::npos);
        t[2] = t[2].substr(temp3 +1, std::string::npos);

        if ((p1 > t1[0]) && (p2 > t1[1]) && (p3 > t1[2]))
        {
            return 1;
        }
    }
    
    return 0;
}

void calculate_triggers_in_file(std::string* path, std::vector<float>& thresholds, int& triggers)
{
    std::ifstream ifs(*path);
    std::vector<std::string> trace(3);
    int triggers_this_file = 0;
    int trace_counter = 0;

    while (std::getline(ifs, trace[0]))
    {
        std::getline(ifs, trace[1]);
        std::getline(ifs, trace[2]);
        // std::cout << trace[0].size() << ' ' << trace[1].size() << ' ' << trace[2].size() << std::endl;
        const auto result = is_t1_trigger(trace, thresholds);
        triggers_this_file = triggers_this_file + result;
        if (result)
        {std::cout << trace_counter << ' ';}
        trace_counter += 1;
    }

    triggers = triggers + triggers_this_file;
    delete path;
}

int main(int argc, char **argv) {

    std::vector<float> thresholds = {144, 149.7, 149.2};
    int triggers = 0;

    std::string *path = new std::string("/cr/tempdata01/filip/UubRandoms/Nov2022/converted/SvenjaFiltered/randoms0009_WCD.dat");

    // // /* Non-concurrent version */
    calculate_triggers_in_file(path, thresholds, triggers);

    printf("\n!!!! C++ found %i triggers in file\n", triggers);

    return 0;
}
