#include <fstream>

int main(int argc, char *argv[])
{
    const short int normalBins = 400;
    const short int step = 2;
    const short int stridedBins = 200;
    const short int stride = 4;

    std::ofstream outFile("binning.txt", std::ios_base::trunc);

    for (short i = 0; i < normalBins; ++i)
    {
        outFile << i << ' ' << step*i << '\n';
    }
    for (short i = 0; i <= stridedBins; ++i)
    {
        outFile << normalBins + i << ' ' << step * (normalBins + stride*i) << '\n'; 
    }

    return 0;
}