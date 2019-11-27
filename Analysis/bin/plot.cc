#include <string>
#include <vector>

#include <ChargedAnalysis/Analysis/include/utils.h>
#include <ChargedAnalysis/Analysis/include/plotter1D.h>
#include <ChargedAnalysis/Analysis/include/plotter2D.h>

int main(int argc, char *argv[]){
    //Extract informations of command line
    std::string histDir = std::string(argv[1]);
    std::vector<std::string> xParameters = Utils::SplitString(std::string(argv[2]), " ");
    std::vector<std::string> yParameters = Utils::SplitString(std::string(argv[3]), " ");
    std::string channel = std::string(argv[4]);
    std::vector<std::string> processes = Utils::SplitString(std::string(argv[5]), " ");
    std::vector<std::string> outDirs = Utils::SplitString(std::string(argv[6]), " ");

    //Call and run Plotter1D class
    Plotter1D plotter1D(histDir, xParameters, channel, processes);
    plotter1D.ConfigureHists();
    plotter1D.Draw(outDirs);
    
    if(!yParameters.empty()){
        Plotter2D plotter2D(histDir, xParameters, yParameters, channel, processes);
        plotter2D.ConfigureHists();
        plotter2D.Draw(outDirs);
    }
}
