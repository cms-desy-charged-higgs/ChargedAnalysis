#include <string>
#include <vector>

#include <ChargedAnalysis/Analysis/include/utils.h>
#include <ChargedAnalysis/Analysis/include/plotterLimit.h>

int main(int argc, char* argv[]){
    //Extract informations of command line
    std::vector<std::string> massStrings = Utils::SplitString(std::string(argv[1]), " ");
    std::string limitDir = std::string(argv[2]);
    std::vector<std::string> outDirs = Utils::SplitString(std::string(argv[3]), " ");
    
    //Turn mass string into int vector
    std::vector<int> masses;
    
    for(std::string& mass: massStrings){
        masses.push_back(std::atoi(mass.c_str()));
    }

    //Call and run Plotter1D class
    PlotterLimit plotter(limitDir, masses);
    plotter.ConfigureHists();
    plotter.Draw(outDirs);
}
