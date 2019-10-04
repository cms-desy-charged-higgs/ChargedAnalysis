#include <ChargedAnalysis/Analysis/include/utils.h>
#include <ChargedAnalysis/Analysis/include/plotter1D.h>

int main(int argc, char *argv[]){
    //Extract informations of command line
    std::string histDir = std::string(argv[1]);
    std::vector<std::string> xParameters = Utils::SplitString(std::string(argv[2]));
    std::string channel = std::string(argv[3]);
    std::vector<std::string> processes = Utils::SplitString(std::string(argv[4]));
    std::vector<std::string> outDirs = Utils::SplitString(std::string(argv[5]));

    //Call and run Plotter1D class
    Plotter1D plotter(histDir, xParameters, channel);
    plotter.ConfigureHists(processes);
    plotter.Draw(outDirs);
}
