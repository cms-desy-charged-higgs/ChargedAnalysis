#include <ChargedAnalysis/Analysis/include/plotter1D.h>

std::vector<std::string> SplitString(std::string splitString){
    std::istringstream iss(splitString);
    std::vector<std::string> results(std::istream_iterator<std::string>{iss},
                                 std::istream_iterator<std::string>());

    return results;
}

int main(int argc, char *argv[]){
    //Extract informations of command line
    std::string histDir = std::string(argv[1]);
    std::vector<std::string> xParameters = SplitString(std::string(argv[2]));
    std::string channel = std::string(argv[3]);
    std::vector<std::string> processes = SplitString(std::string(argv[4]));
    std::vector<std::string> outDirs = SplitString(std::string(argv[5]));

    //Call and run Plotter1D class
    Plotter1D plotter(histDir, xParameters, channel);
    plotter.ConfigureHists(processes);
    plotter.Draw(outDirs);
}
