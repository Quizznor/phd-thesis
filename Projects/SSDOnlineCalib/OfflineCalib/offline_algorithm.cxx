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

    const auto random_file = std::string(4 - std::min((size_t)4, file.length()), '0') + file;
    const auto full_path = "/cr/tempdata01/filip/iRODS/UubRandoms/converted/"+station+"/randoms"+random_file+"_SSD.dat";

    std::ifstream ifs(full_path);
    std::string line;

    while (std::getline(ifs, line))
    {
        const int baseline = stoi(line.substr(0, 3));
        int max_bin = std::numeric_limits<int>::min();

        for (int i=4; i<=8195; i+=4)
        {
            const auto this_bin = stoi(line.substr(i, i+4)) - baseline;
            if (this_bin > max_bin){max_bin = this_bin;}
        }

        for (int j=max_bin-1; j>=0; j--){rates[j] += 1;}

    }
        
    // works =)
    int step = 1;

    for (const auto& bin : rates)
    {
        std::cout << step << " " << bin << std::endl;
        step += 1;
    }

    std::cout << "reading " << full_path << std::endl;
    return 0;
}